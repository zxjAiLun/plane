#include "Renderer.hpp"
#include "InputBinding.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"
#include "DamageType.hpp"
#include "EnemyDefinition.hpp"
#include "Equipment.hpp"
#include "BossRelicEffect.hpp"
#include "Stats.hpp"
#include "SkillBar.hpp"
#include "SkillProgression.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <vector>

namespace {
sf::Color rarityColor(Rarity rarity) {
    switch (rarity) {
        case Rarity::Normal: return sf::Color(220, 220, 220);
        case Rarity::Magic: return sf::Color(90, 150, 255);
        case Rarity::Rare: return sf::Color(255, 210, 80);
        case Rarity::Unique: return sf::Color(255, 145, 45);
    }
    return sf::Color::White;
}

sf::Color enemyColor(const EnemyColor& color) {
    return sf::Color(color.r, color.g, color.b);
}

sf::Color mapColor(const MapColor& color, std::uint8_t alpha = 255) {
    return sf::Color(color.r, color.g, color.b, alpha);
}

sf::Color damageTypeColor(DamageType type) {
    switch (type) {
        case DamageType::Fire: return sf::Color(255, 125, 45);
        case DamageType::Cold: return sf::Color(105, 225, 255);
        case DamageType::Lightning: return sf::Color(190, 145, 255);
        case DamageType::Poison: return sf::Color(105, 220, 105);
        case DamageType::Physical: return sf::Color(255, 225, 105);
    }
    return sf::Color::White;
}

int multiplierPercent(float multiplier) {
    return static_cast<int>((multiplier - 1.0f) * 100.0f + 0.5f);
}

std::string statsSummary(const Stats& stats) {
    std::string summary;
    const auto multiplierText = [](float multiplier) {
        const int value = multiplierPercent(multiplier);
        return (value > 0 ? "+" : "") + std::to_string(value) + "%";
    };
    if (stats.maxHp > 0) {
        summary += "+" + std::to_string(stats.maxHp) + " HP ";
    }
    if (stats.damageMultiplier != 1.0f) {
        summary += multiplierText(stats.damageMultiplier) + " DMG ";
    }
    if (stats.attackSpeedMultiplier != 1.0f) {
        summary += multiplierText(stats.attackSpeedMultiplier) + " AS ";
    }
    if (stats.moveSpeedMultiplier != 1.0f) {
        summary += multiplierText(stats.moveSpeedMultiplier) + " MS ";
    }
    if (stats.pickupRangeMultiplier != 1.0f) {
        summary += multiplierText(stats.pickupRangeMultiplier) + " PICKUP ";
    }
    if (stats.projectileDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.projectileDamageMultiplier) + " PDMG ";
    }
    if (stats.areaDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.areaDamageMultiplier) + " ADMG ";
    }
    if (stats.areaRadiusMultiplier != 1.0f) {
        summary += multiplierText(stats.areaRadiusMultiplier) + " AREA ";
    }
    if (stats.projectileCountBonus > 0) {
        summary += "+" + std::to_string(stats.projectileCountBonus) + " PROJ ";
    }
    if (stats.lifeFlaskEffectMultiplier != 1.0f) {
        summary += multiplierText(stats.lifeFlaskEffectMultiplier) + " FLASK ";
    }
    if (stats.itemQuantityMultiplier != 1.0f) {
        summary += multiplierText(stats.itemQuantityMultiplier) + " DROP ";
    }
    if (stats.incomingDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.incomingDamageMultiplier) + " TAKEN ";
    }
    if (stats.fireDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.fireDamageMultiplier) + " FIRE DMG ";
    }
    if (stats.coldDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.coldDamageMultiplier) + " COLD DMG ";
    }
    if (stats.lightningDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.lightningDamageMultiplier) + " LIGHT DMG ";
    }
    if (stats.poisonDamageMultiplier != 1.0f) {
        summary += multiplierText(stats.poisonDamageMultiplier) + " POISON DMG ";
    }
    if (stats.armor > 0) {
        summary += "+" + std::to_string(stats.armor) + " ARM ";
    }
    if (stats.fireResistance > 0) {
        summary += std::to_string(stats.fireResistance) + "% FIRE RES ";
    }
    if (stats.coldResistance > 0) {
        summary += std::to_string(stats.coldResistance) + "% COLD RES ";
    }
    if (stats.lightningResistance > 0) {
        summary += std::to_string(stats.lightningResistance) + "% LIGHT RES ";
    }
    if (stats.poisonResistance > 0) {
        summary += std::to_string(stats.poisonResistance) + "% POISON RES ";
    }
    return summary;
}

std::optional<int> itemRequiredLevel(const Item& item) {
    const auto* base = ItemBaseLibrary::find(item.baseId);
    return base == nullptr ? std::nullopt : std::optional<int>(base->requiredLevel);
}

std::string itemRequirementSummary(const Item& item) {
    const auto requiredLevel = itemRequiredLevel(item);
    return requiredLevel
        ? "Req Lv " + std::to_string(*requiredLevel)
        : "Req Lv ?";
}

bool itemRequirementMet(const GameWorld& world, const Item& item) {
    const auto requiredLevel = itemRequiredLevel(item);
    return requiredLevel && world.player().level() >= *requiredLevel;
}

std::string itemSummary(const Item& item) {
    return item.name + " " + itemRequirementSummary(item) + " " + statsSummary(item.stats);
}

std::string bossRelicEffectSummary(const Item& item) {
    if (item.rarity != Rarity::Unique) {
        return {};
    }

    const auto* base = ItemBaseLibrary::find(item.baseId);
    if (base == nullptr || base->kind != ItemBaseKind::BossRelic) {
        return {};
    }

    const auto& effect = BossRelicEffectLibrary::forBase(*base);
    if (effect.type == BossRelicEffectType::None) {
        return {};
    }

    return effect.name + " - " + effect.description;
}

ItemBaseTheme bossRelicTheme(BossLootTheme theme) {
    switch (theme) {
        case BossLootTheme::Brimstone: return ItemBaseTheme::Brimstone;
        case BossLootTheme::Storm: return ItemBaseTheme::Storm;
        case BossLootTheme::Brood: return ItemBaseTheme::Brood;
        case BossLootTheme::Frost: return ItemBaseTheme::Frost;
    }
    return ItemBaseTheme::None;
}

std::string truncateText(const std::string& text, std::size_t maxLength) {
    if (text.size() <= maxLength) {
        return text;
    }
    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }
    return text.substr(0, maxLength - 3) + "...";
}

std::string affixTagsSummary(const std::vector<AffixTag>& tags) {
    std::string summary;
    for (std::size_t index = 0; index < tags.size(); ++index) {
        if (index > 0) {
            summary += ", ";
        }
        summary += affixTagName(tags[index]);
    }
    return summary;
}

Stats statsDelta(const Stats& next, const Stats& current) {
    return {
        next.maxHp - current.maxHp,
        next.moveSpeedMultiplier / current.moveSpeedMultiplier,
        next.damageMultiplier / current.damageMultiplier,
        next.attackSpeedMultiplier / current.attackSpeedMultiplier,
        next.pickupRangeMultiplier / current.pickupRangeMultiplier,
        next.projectileDamageMultiplier / current.projectileDamageMultiplier,
        next.areaDamageMultiplier / current.areaDamageMultiplier,
        next.areaRadiusMultiplier / current.areaRadiusMultiplier,
        next.armor - current.armor,
        next.projectileCountBonus - current.projectileCountBonus,
        next.lifeFlaskEffectMultiplier / current.lifeFlaskEffectMultiplier,
        next.itemQuantityMultiplier / current.itemQuantityMultiplier,
        next.incomingDamageMultiplier / current.incomingDamageMultiplier,
        next.fireDamageMultiplier / current.fireDamageMultiplier,
        next.coldDamageMultiplier / current.coldDamageMultiplier,
        next.lightningDamageMultiplier / current.lightningDamageMultiplier,
        next.fireResistance - current.fireResistance,
        next.coldResistance - current.coldResistance,
        next.lightningResistance - current.lightningResistance,
        next.poisonDamageMultiplier / current.poisonDamageMultiplier,
        next.poisonResistance - current.poisonResistance,
    };
}

std::string statsDeltaSummary(const Stats& delta) {
    std::string summary;
    if (delta.maxHp != 0) {
        summary += (delta.maxHp > 0 ? "+" : "") + std::to_string(delta.maxHp) + " HP ";
    }
    if (delta.damageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.damageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% DMG ";
    }
    if (delta.attackSpeedMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.attackSpeedMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% AS ";
    }
    if (delta.moveSpeedMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.moveSpeedMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% MS ";
    }
    if (delta.pickupRangeMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.pickupRangeMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% PICKUP ";
    }
    if (delta.projectileDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.projectileDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% PDMG ";
    }
    if (delta.areaDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.areaDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% ADMG ";
    }
    if (delta.areaRadiusMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.areaRadiusMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% AREA ";
    }
    if (delta.armor != 0) {
        summary += (delta.armor > 0 ? "+" : "") + std::to_string(delta.armor) + " ARM ";
    }
    if (delta.projectileCountBonus != 0) {
        summary += (delta.projectileCountBonus > 0 ? "+" : "")
            + std::to_string(delta.projectileCountBonus) + " PROJ ";
    }
    if (delta.lifeFlaskEffectMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.lifeFlaskEffectMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% FLASK ";
    }
    if (delta.itemQuantityMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.itemQuantityMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% DROP ";
    }
    if (delta.incomingDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.incomingDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% TAKEN ";
    }
    if (delta.fireDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.fireDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% FIRE DMG ";
    }
    if (delta.coldDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.coldDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% COLD DMG ";
    }
    if (delta.lightningDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.lightningDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% LIGHT DMG ";
    }
    if (delta.poisonDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.poisonDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% POISON DMG ";
    }
    if (delta.fireResistance != 0) {
        summary += (delta.fireResistance > 0 ? "+" : "")
            + std::to_string(delta.fireResistance) + " FIRE RES ";
    }
    if (delta.coldResistance != 0) {
        summary += (delta.coldResistance > 0 ? "+" : "")
            + std::to_string(delta.coldResistance) + " COLD RES ";
    }
    if (delta.lightningResistance != 0) {
        summary += (delta.lightningResistance > 0 ? "+" : "")
            + std::to_string(delta.lightningResistance) + " LIGHT RES ";
    }
    if (delta.poisonResistance != 0) {
        summary += (delta.poisonResistance > 0 ? "+" : "")
            + std::to_string(delta.poisonResistance) + " POISON RES ";
    }
    return summary.empty() ? "No stat change" : summary;
}

sf::Color deltaColor(const Stats& delta) {
    const bool positive = delta.maxHp > 0
        || delta.damageMultiplier > 1.0f
        || delta.attackSpeedMultiplier > 1.0f
        || delta.moveSpeedMultiplier > 1.0f
        || delta.pickupRangeMultiplier > 1.0f
        || delta.projectileDamageMultiplier > 1.0f
        || delta.areaDamageMultiplier > 1.0f
        || delta.areaRadiusMultiplier > 1.0f
        || delta.projectileCountBonus > 0
        || delta.lifeFlaskEffectMultiplier > 1.0f
        || delta.itemQuantityMultiplier > 1.0f
        || delta.incomingDamageMultiplier < 1.0f
        || delta.armor > 0
        || delta.fireDamageMultiplier > 1.0f
        || delta.coldDamageMultiplier > 1.0f
        || delta.lightningDamageMultiplier > 1.0f
        || delta.poisonDamageMultiplier > 1.0f
        || delta.fireResistance > 0
        || delta.coldResistance > 0
        || delta.lightningResistance > 0
        || delta.poisonResistance > 0;
    const bool negative = delta.maxHp < 0
        || delta.damageMultiplier < 1.0f
        || delta.attackSpeedMultiplier < 1.0f
        || delta.moveSpeedMultiplier < 1.0f
        || delta.pickupRangeMultiplier < 1.0f
        || delta.projectileDamageMultiplier < 1.0f
        || delta.areaDamageMultiplier < 1.0f
        || delta.areaRadiusMultiplier < 1.0f
        || delta.projectileCountBonus < 0
        || delta.lifeFlaskEffectMultiplier < 1.0f
        || delta.itemQuantityMultiplier < 1.0f
        || delta.incomingDamageMultiplier > 1.0f
        || delta.armor < 0
        || delta.fireDamageMultiplier < 1.0f
        || delta.coldDamageMultiplier < 1.0f
        || delta.lightningDamageMultiplier < 1.0f
        || delta.poisonDamageMultiplier < 1.0f
        || delta.fireResistance < 0
        || delta.coldResistance < 0
        || delta.lightningResistance < 0
        || delta.poisonResistance < 0;

    if (positive && !negative) {
        return sf::Color(120, 230, 140);
    }
    if (negative && !positive) {
        return sf::Color(240, 120, 120);
    }
    return sf::Color(230, 220, 150);
}

sf::Color passiveBranchColor(PassiveBranch branch) {
    switch (branch) {
        case PassiveBranch::Projectile: return sf::Color(110, 185, 255);
        case PassiveBranch::Area: return sf::Color(255, 150, 85);
        case PassiveBranch::Survival: return sf::Color(120, 235, 145);
        case PassiveBranch::Loot: return sf::Color(245, 215, 90);
        case PassiveBranch::Poison: return sf::Color(105, 220, 105);
    }
    return sf::Color::White;
}

std::string passiveBranchName(PassiveBranch branch) {
    switch (branch) {
        case PassiveBranch::Projectile: return "Projectile";
        case PassiveBranch::Area: return "Area";
        case PassiveBranch::Survival: return "Survival";
        case PassiveBranch::Loot: return "Loot";
        case PassiveBranch::Poison: return "Poison";
    }
    return "Unknown";
}

std::string passiveKeyLabel(std::size_t index) {
    if (index < 9) {
        return std::to_string(index + 1);
    }
    if (index == 9) {
        return "0";
    }
    return "F" + std::to_string(index - 9);
}

std::string skillChoiceLabel(std::size_t index) {
    if (index < 9) {
        return std::to_string(index + 1);
    }
    if (index == 9) {
        return "0";
    }
    return "F" + std::to_string(index - 3);
}

std::string skillSlotName(SkillSlot slot) {
    switch (slot) {
        case SkillSlot::Primary: return "Primary";
        case SkillSlot::Secondary: return "Secondary";
        case SkillSlot::Utility: return "Utility";
        case SkillSlot::Movement: return "Movement";
        case SkillSlot::Count: break;
    }
    return "Unknown";
}

std::string skillCastTypeName(SkillCastType type) {
    switch (type) {
        case SkillCastType::Projectile: return "Projectile";
        case SkillCastType::SelfCenteredArea: return "Self Area";
        case SkillCastType::MouseTargetedArea: return "Mouse Area";
        case SkillCastType::Dash: return "Dash";
    }
    return "Unknown";
}

std::string formatFloat(float value, int precision = 1) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

int effectiveSkillDamage(const SkillDefinition& skill, const Stats& stats, const SupportList& supports) {
    if (skill.baseDamage <= 0) {
        return 0;
    }

    return skillDamage(skill, stats, supports);
}

int effectiveSkillDamage(const SkillDefinition& skill, const Stats& stats, const SupportDefinition* support = nullptr) {
    return effectiveSkillDamage(skill, stats, SupportList{support, nullptr});
}

float effectiveSkillRadius(const SkillDefinition& skill, const Stats& stats, const SupportList& supports) {
    return skillRadius(skill, stats, supports);
}

float effectiveSkillRadius(const SkillDefinition& skill, const Stats& stats, const SupportDefinition* support = nullptr) {
    return effectiveSkillRadius(skill, stats, SupportList{support, nullptr});
}

float effectiveSkillCooldown(const SkillDefinition& skill, const Stats& stats, const SupportList& supports) {
    return skillCooldown(skill, stats, supports);
}

float effectiveSkillCooldown(const SkillDefinition& skill, const Stats& stats, const SupportDefinition* support = nullptr) {
    return effectiveSkillCooldown(skill, stats, SupportList{support, nullptr});
}

std::string ailmentSummary(const AilmentDefinition& ailment) {
    switch (ailment.type) {
        case AilmentType::Ignite:
            return "Ignite " + formatFloat(ailment.duration, 1) + "s"
                + (ailment.ignitePenetration > 0
                    ? " Pen " + std::to_string(ailment.ignitePenetration) + "%"
                    : "");
        case AilmentType::Chill:
            return "Chill " + formatFloat(ailment.duration, 1) + "s "
                + std::to_string(static_cast<int>((1.0f - ailment.speedMultiplier) * 100.0f)) + "% slow"
                + (ailment.chillPenetration > 0
                    ? " Pen " + std::to_string(ailment.chillPenetration) + "%"
                    : "");
        case AilmentType::Shock:
            return "Shock " + formatFloat(ailment.duration, 1) + "s +"
                + std::to_string(static_cast<int>(
                    (ailment.damageTakenMultiplier - 1.0f) * 100.0f
                )) + "% taken"
                + (ailment.shockPenetration > 0
                    ? " Pen " + std::to_string(ailment.shockPenetration) + "%"
                    : "");
        case AilmentType::Poison:
            return "Poison " + formatFloat(ailment.duration, 1) + "s "
                + std::to_string(static_cast<int>(ailment.damageMultiplier * 100.0f)) + "% DoT"
                + (ailment.poisonPenetration > 0
                    ? " Pen " + std::to_string(ailment.poisonPenetration) + "%"
                    : "")
                + (ailment.poisonSpreadRadius > 0.0f
                    ? " Spread " + std::to_string(static_cast<int>(ailment.poisonSpreadRadius))
                        + " @" + std::to_string(static_cast<int>(
                            ailment.poisonSpreadMultiplier * 100.0f
                        )) + "%"
                    : "");
        case AilmentType::None:
        case AilmentType::Count:
            return "";
    }

    return "";
}

std::string skillEffectiveSummary(const SkillDefinition& skill, const Stats& stats, const SupportList& supports) {
    std::string summary = "Base " + std::to_string(skill.baseDamage)
        + "/" + std::to_string(static_cast<int>(skill.radius))
        + "/" + formatFloat(skill.cooldown, 2)
        + "  Actual " + std::to_string(effectiveSkillDamage(skill, stats, supports))
        + "/" + std::to_string(static_cast<int>(effectiveSkillRadius(skill, stats, supports)))
        + "/" + formatFloat(effectiveSkillCooldown(skill, stats, supports), 2)
        + "  Mana " + formatFloat(skill.manaCost, 0)
        + "  " + damageTypeName(skill.damageType);
    if (skill.castType == SkillCastType::Projectile) {
        summary += "  Proj " + std::to_string(skillProjectileCount(skill, supports, stats));
    }
    const int repeatCount = skillRepeatCount(skill, supports);
    if (repeatCount > 1) {
        summary += "  Hits " + std::to_string(repeatCount);
    }
    if (skill.repeatCount > 1 && skill.repeatInterval > 0.0f) {
        summary += "  Pulses " + std::to_string(skill.repeatCount)
            + "/" + formatFloat(skill.repeatInterval, 2) + "s";
    }
    if (skill.delivery == SkillDeliveryType::DelayedArea && skill.castDelay > 0.0f) {
        summary += "  Impact " + formatFloat(skill.castDelay, 2) + "s";
    }
    if (skill.groundHazard.isValid()) {
        summary += "  Leaves " + skill.groundHazard.source
            + " " + formatFloat(skill.groundHazard.duration, 1) + "s";
    }
    return summary;
}

std::string craftingPreview(
    const Item& item,
    std::size_t affixIndex,
    CraftingOperation operation
) {
    if (operation == CraftingOperation::None) {
        return {};
    }
    if (!craftingOperationAllowed(operation, item)) {
        return " [Locked for " + std::string(rarityName(item.rarity)) + "]";
    }
    if (affixIndex >= item.affixes.size() || item.affixes[affixIndex].id.empty()) {
        return " [Fixed]";
    }
    if (operation == CraftingOperation::RerollAffix) {
        return "  => Random legal "
            + std::string(item.affixes[affixIndex].isPrefix ? "prefix" : "suffix");
    }

    Item candidate = item;
    const CraftingResult result = operation == CraftingOperation::ImproveAffix
        ? LootGenerator::improveAffix(candidate, affixIndex)
        : LootGenerator::raiseAffixTier(candidate, affixIndex);
    if (result == CraftingResult::Success) {
        return "  => " + statsDeltaSummary(statsDelta(
            candidate.affixes[affixIndex].stats,
            item.affixes[affixIndex].stats
        ));
    }
    if (result == CraftingResult::AlreadyMaxTier) {
        return " [Already max tier]";
    }
    if (result == CraftingResult::NoImprovement) {
        return " [Already max value]";
    }
    return " [Unavailable]";
}

std::string skillEffectiveSummary(const SkillDefinition& skill, const Stats& stats, const SupportDefinition* support = nullptr) {
    return skillEffectiveSummary(skill, stats, SupportList{support, nullptr});
}

// Combined equipment stats if `candidate` were equipped into its own slot,
// replacing any currently equipped item in that slot (or filling an empty slot).
Stats previewEquipmentStats(const Equipment& equipment, const Item& candidate) {
    const std::array<EquipmentSlot, 4> slots = {
        EquipmentSlot::Weapon,
        EquipmentSlot::Armor,
        EquipmentSlot::Ring,
        EquipmentSlot::Amulet
    };
    Stats result;
    for (const auto slot : slots) {
        if (slot == candidate.slot) {
            result = combineStats(result, candidate.stats);
        } else {
            const auto& equipped = equipment.itemInSlot(slot);
            if (equipped) {
                result = combineStats(result, equipped->stats);
            }
        }
    }
    return result;
}

// Before -> after skill impact lines for the three skill slots.
// Primary shows damage + cooldown; Secondary/Utility show damage + radius.
std::vector<std::string> skillImpactDetailLines(const Stats& before, const Stats& after, const SkillBar& skillBar) {
    std::vector<std::string> lines;
    const auto detail = [&](const char* label, const SkillDefinition& skill, bool showRadius) {
        const auto supports = skillBar.supportDefinitionsFor(skill);
        std::string line = std::string(label) + ": DMG "
            + std::to_string(effectiveSkillDamage(skill, before, supports)) + " -> "
            + std::to_string(effectiveSkillDamage(skill, after, supports));
        if (showRadius) {
            line += "  R "
                + std::to_string(static_cast<int>(effectiveSkillRadius(skill, before, supports))) + " -> "
                + std::to_string(static_cast<int>(effectiveSkillRadius(skill, after, supports)));
        } else {
            line += "  CD "
                + formatFloat(effectiveSkillCooldown(skill, before, supports), 2) + " -> "
                + formatFloat(effectiveSkillCooldown(skill, after, supports), 2);
        }
        return line;
    };
    lines.push_back(detail("Primary", skillBar.definition(SkillSlot::Primary), false));
    lines.push_back(detail("Secondary", skillBar.definition(SkillSlot::Secondary), true));
    lines.push_back(detail("Utility", skillBar.definition(SkillSlot::Utility), true));
    return lines;
}

std::string rewardThemeLabel(const MapRewardDefinition& reward) {
    DamageType theme = DamageType::Physical;
    if (reward.type == MapRewardType::UnlockSkill
        || reward.type == MapRewardType::UpgradeSkill) {
        const auto* skill = SkillLibrary::find(reward.skillName);
        if (skill != nullptr) {
            theme = skill->damageType;
        }
    } else if (reward.type == MapRewardType::UnlockSupport
        || reward.type == MapRewardType::UpgradeSupport) {
        const auto* support = SupportLibrary::find(reward.supportName);
        if (support != nullptr) {
            switch (support->kind) {
                case SupportKind::Combustion: theme = DamageType::Fire; break;
                case SupportKind::DeepChill: theme = DamageType::Cold; break;
                case SupportKind::Conductivity: theme = DamageType::Lightning; break;
                case SupportKind::Toxicity:
                case SupportKind::Contagion: theme = DamageType::Poison; break;
                case SupportKind::Pierce:
                case SupportKind::Amplify:
                case SupportKind::Quickcast:
                case SupportKind::Volley:
                case SupportKind::Trailblazer:
                case SupportKind::Barrage:
                case SupportKind::Concentration:
                case SupportKind::Echo:
                case SupportKind::Pinpoint:
                    break;
            }
        }
    }

    return theme == DamageType::Physical
        ? ""
        : std::string(damageTypeName(theme)) + " build";
}

std::string rewardDetailSummary(const MapRewardDefinition& reward, const GameWorld& world) {
    const std::string theme = rewardThemeLabel(reward);
    const std::string themePrefix = theme.empty() ? "" : theme + " | ";
    if (reward.type == MapRewardType::UnlockSupport
        || reward.type == MapRewardType::UpgradeSupport) {
        return themePrefix + "Support rune: " + reward.description;
    }

    if (reward.type != MapRewardType::UnlockSkill
        && reward.type != MapRewardType::UpgradeSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    const auto& current = world.skillBar().definition(skill->slot);
    const std::string levelText = reward.type == MapRewardType::UpgradeSkill
        ? "Lv" + std::to_string(world.skillLevel(skill->name))
            + " -> Lv" + std::to_string(reward.targetLevel) + "  |  "
        : "";
    return themePrefix + levelText + skillSlotName(skill->slot) + " / " + skillCastTypeName(skill->castType)
        + "  |  Replaces " + current.name;
}

std::string rewardStatPreview(const MapRewardDefinition& reward, const GameWorld& world) {
    if (reward.type == MapRewardType::UnlockSupport
        || reward.type == MapRewardType::UpgradeSupport) {
        const int currentLevel = world.supportLevel(reward.supportName);
        return "Support Lv" + std::to_string(currentLevel) + " -> Lv"
            + std::to_string(reward.targetLevel)
            + "  Configure it in K with F1 / F2 / F3 / F4";
    }

    if (reward.type != MapRewardType::UnlockSkill
        && reward.type != MapRewardType::UpgradeSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    const SkillDefinition effectiveSkill = SkillProgression::skillAtLevel(
        *skill, reward.type == MapRewardType::UpgradeSkill
            ? reward.targetLevel : world.skillLevel(skill->name)
    );
    const auto supports = world.skillBar().supportDefinitionsFor(effectiveSkill);
    return "Lv" + std::to_string(
            reward.type == MapRewardType::UpgradeSkill
                ? reward.targetLevel : world.skillLevel(skill->name)
        ) + "  " + skillEffectiveSummary(
            effectiveSkill, world.player().stats(), supports
        );
}

std::string mapOptionSummary(const MapOption& option) {
    const MapModifier modifier = MapItemLibrary::modifierFor(option);
    std::string summary = std::string(mapRarityName(option.rarity))
        + " Q" + std::to_string(option.quality)
        + " | " + MapItemLibrary::affixSummary(option)
        + " | MHP+" + std::to_string(multiplierPercent(modifier.monsterHpMultiplier))
        + " MD+" + std::to_string(modifier.monsterDamageBonus)
        + " E+" + std::to_string(modifier.eliteWeightBonus)
        + " BHP+" + std::to_string(multiplierPercent(modifier.bossHpMultiplier))
        + " BD+" + std::to_string(multiplierPercent(modifier.bossDamageMultiplier))
        + " IQ+" + std::to_string(multiplierPercent(modifier.itemQuantityMultiplier))
        + " IR+" + std::to_string(multiplierPercent(modifier.itemRarityMultiplier))
        + " IL+" + std::to_string(modifier.itemLevelBonus);
    if (!modifier.elementalChallengeId.empty()) {
        summary += " " + std::string(damageTypeName(modifier.elementalChallengeType))
            + " RES-" + std::to_string(modifier.playerElementalResistancePenalty)
            + " MRES+" + std::to_string(modifier.monsterElementalResistanceBonus);
    }
    return summary;
}

sf::Color mapEventColor(const MapEventInstance& event) {
    if (event.completed) {
        return sf::Color(130, 130, 130);
    }

    switch (event.type) {
        case MapEventType::LootCache: return sf::Color(255, 215, 70);
        case MapEventType::ElitePack: return sf::Color(190, 90, 255);
        case MapEventType::Shrine: return sf::Color(80, 230, 230);
        case MapEventType::Combination:
            switch (event.encounterType) {
                case MapEncounterType::EnhancedCache: return sf::Color(255, 150, 65);
                case MapEncounterType::HazardousElitePack: return sf::Color(215, 80, 210);
                case MapEncounterType::GuardedShrine: return sf::Color(70, 205, 185);
                case MapEncounterType::BountyHunt: return sf::Color(255, 105, 75);
                case MapEncounterType::CursedReliquary: return sf::Color(255, 105, 190);
                case MapEncounterType::WardenCourt: return sf::Color(120, 190, 255);
                case MapEncounterType::FrozenReliquary: return sf::Color(145, 225, 255);
                case MapEncounterType::None: break;
            }
            break;
    }

    return sf::Color::White;
}

std::string enemyDisplayLabel(const GameWorld& world, const Enemy& enemy) {
    if (enemy.isBoss()) {
        return world.bossDefinition().name;
    }

    const auto& definition = EnemyLibrary::forType(enemy.type());
    if (enemy.isRare() && !enemy.displayName().empty()) {
        return "Rare " + enemy.displayName();
    }

    const auto& modifier = EliteModifierLibrary::forModifier(enemy.eliteModifier());
    std::string modifierNames;
    if (!modifier.name.empty()) {
        modifierNames = modifier.name;
    }

    const auto& secondary = EliteModifierLibrary::forModifier(
        enemy.secondaryEliteModifier()
    );
    if (!secondary.name.empty()
        && enemy.secondaryEliteModifier() != enemy.eliteModifier()) {
        if (!modifierNames.empty()) {
            modifierNames += " ";
        }
        modifierNames += secondary.name;
    }

    return modifierNames.empty() ? definition.name : modifierNames + " " + definition.name;
}

std::string eliteModifierDescription(const Enemy& enemy) {
    const auto append = [](std::string& result, EliteModifier modifier) {
        const auto& definition = EliteModifierLibrary::forModifier(modifier);
        if (definition.description.empty()) {
            return;
        }
        if (!result.empty()) {
            result += " | ";
        }
        result += definition.name + ": " + definition.description;
    };

    std::string result;
    append(result, enemy.eliteModifier());
    if (enemy.secondaryEliteModifier() != enemy.eliteModifier()) {
        append(result, enemy.secondaryEliteModifier());
    }
    return result;
}

const Enemy* focusedEliteEnemy(const GameWorld& world) {
    constexpr float FocusRange = 520.0f;
    const float rangeSquared = FocusRange * FocusRange;
    const Vector2 playerPosition = world.player().position();
    const Enemy* focused = nullptr;
    float closestDistanceSquared = 0.0f;

    for (const auto& enemy : world.enemies()) {
        if (!enemy.isElite() || enemy.isDead()) {
            continue;
        }

        const float distanceSquared = (enemy.position() - playerPosition).lengthSquared();
        if (distanceSquared > rangeSquared
            || (focused != nullptr && distanceSquared >= closestDistanceSquared)) {
            continue;
        }

        focused = &enemy;
        closestDistanceSquared = distanceSquared;
    }

    return focused;
}

const Enemy* focusedSummonerEnemy(const GameWorld& world) {
    constexpr float FocusRange = 620.0f;
    const float rangeSquared = FocusRange * FocusRange;
    const Vector2 playerPosition = world.player().position();
    const Enemy* focused = nullptr;
    float closestDistanceSquared = 0.0f;

    for (const auto& enemy : world.enemies()) {
        if (!enemy.isSummoner() || enemy.isDead()) {
            continue;
        }

        const float distanceSquared = (enemy.position() - playerPosition).lengthSquared();
        if (distanceSquared > rangeSquared
            || (focused != nullptr && distanceSquared >= closestDistanceSquared)) {
            continue;
        }

        focused = &enemy;
        closestDistanceSquared = distanceSquared;
    }

    return focused;
}

std::string activeElitePackModifierDescription(const GameWorld& world) {
    if (world.activeEliteEventEnemiesRemaining() <= 0) {
        return "";
    }

    const MapEventInstance* activeEvent = nullptr;
    std::size_t activeEventIndex = 0;
    for (std::size_t index = 0; index < world.map().events().size(); ++index) {
        const auto& event = world.map().events()[index];
        const bool isEliteEncounter = event.type == MapEventType::ElitePack
            || (event.type == MapEventType::Combination
                && (event.encounterType == MapEncounterType::HazardousElitePack
                    || event.encounterType == MapEncounterType::BountyHunt
                    || event.encounterType == MapEncounterType::CursedReliquary
                    || event.encounterType == MapEncounterType::WardenCourt
                    || event.encounterType == MapEncounterType::FrozenReliquary));
        if (isEliteEncounter && event.triggered && !event.completed) {
            activeEvent = &event;
            activeEventIndex = index;
            break;
        }
    }
    if (activeEvent == nullptr) {
        return "";
    }

    const Enemy* eventElite = nullptr;
    float closestDistanceSquared = 0.0f;
    for (const auto& enemy : world.enemies()) {
        if (!enemy.isElite() || enemy.isBoss() || enemy.isDead()) {
            continue;
        }
        if (enemy.mapEventIndex() != static_cast<int>(activeEventIndex)) {
            continue;
        }

        const float distanceSquared = (enemy.position() - activeEvent->position).lengthSquared();
        if (distanceSquared > 600.0f * 600.0f
            || (eventElite != nullptr && distanceSquared >= closestDistanceSquared)) {
            continue;
        }

        eventElite = &enemy;
        closestDistanceSquared = distanceSquared;
    }

    return eventElite == nullptr ? "" : eliteModifierDescription(*eventElite);
}

std::string mapEncounterStatus(const GameWorld& world) {
    for (const auto& event : world.map().events()) {
        if (event.type != MapEventType::Combination) {
            continue;
        }

        const auto& definition = world.map().encounterDefinition();
        if (event.completed) {
            return definition.name + " [Cleared]";
        }
        if (!event.triggered) {
            return definition.name + " [Available]";
        }
        if (event.triggered && world.activeEliteEventEnemiesRemaining() > 0) {
            return definition.name + " [Enemies remaining]";
        }
        return definition.name + " [Ready]";
    }
    return "";
}

std::string activeMapEventLabel(const GameWorld& world) {
    for (const auto& event : world.map().events()) {
        if (event.triggered && !event.completed
            && (event.type == MapEventType::ElitePack
                || event.type == MapEventType::Combination)) {
            return event.type == MapEventType::Combination
                ? world.map().encounterDefinition().name
                : "Elite Pack";
        }
    }
    return "Elite Pack";
}
}

Renderer::Renderer(sf::RenderWindow& window)
    : window_(window)
    , fontLoaded_(false) {
    const char* fontPaths[] = {
        "assets/font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuSans[wdth,wght].ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (const char* path : fontPaths) {
        if (font_.openFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }
}

void Renderer::render(const GameWorld& world) {
    window_.clear(sf::Color::Black);

    drawMap(world);
    drawAmbientHazardWarning(world);
    drawGroundHazards(world);
    drawPendingSkillEffects(world);
    drawNovaEffect(world);
    drawSecondarySkillEffect(world);
    drawDashImpactEffect(world);
    drawBossAoeEffect(world);
    drawRareLeaderEffect(world);
    drawBossDashEffect(world);
    drawVolatileExplosionEffect(world);
    drawPlayer(world);
    drawAimIndicator(world);
    drawProjectiles(world);
    drawBossProjectiles(world);
    drawEnemyProjectiles(world);
    drawEnemies(world);
    drawCombatFeedback(world);
    drawDroppedItems(world);

    drawText("HP " + std::to_string(world.player().hp()) + "/" + std::to_string(world.player().maxHp()),
        {16.0f, 12.0f}, 18, sf::Color::White);
    const bool flaskEmpty = world.lifeFlaskCharges() <= 0;
    drawText("Flask G " + std::to_string(world.lifeFlaskCharges()) + "/"
        + std::to_string(world.lifeFlaskMaxCharges()),
        {180.0f, 12.0f}, 16, flaskEmpty ? sf::Color(255, 100, 100) : sf::Color(170, 235, 190));
    if (!world.lifeFlaskStatusMessage().empty()
        && world.lifeFlaskStatusTimeRemaining() > 0.0f) {
        drawText(world.lifeFlaskStatusMessage(), {310.0f, 12.0f}, 14,
            flaskEmpty ? sf::Color(255, 120, 120) : sf::Color(180, 245, 200));
    }
    drawText("LV " + std::to_string(world.player().level())
        + "  EXP " + std::to_string(world.player().exp()) + "/" + std::to_string(world.player().expToNextLevel())
        + "  SP " + std::to_string(world.player().talentPoints())
        + "  MANA " + std::to_string(static_cast<int>(std::ceil(world.player().mana())))
        + "/" + std::to_string(static_cast<int>(std::ceil(world.player().maxMana()))),
        {16.0f, 36.0f}, 18, sf::Color::White);
    drawText("TIME " + std::to_string(static_cast<int>(world.survivalTime()))
        + "  SCORE " + std::to_string(world.score()),
        {16.0f, 60.0f}, 18, sf::Color::White);
    const std::string mapLine = "MAP " + std::to_string(world.mapLevel()) + " "
        + mapRarityName(world.currentMapOption().rarity)
        + " Q" + std::to_string(world.currentMapOption().quality) + " "
        + world.map().definition().name
        + "  LAYOUT " + std::to_string(world.map().layoutIndex() + 1) + "/"
            + std::to_string(MapLayoutLibrary::VariantCount)
        + "  AREA " + mapAreaName(world.currentMapArea())
        + "  ENEMIES " + std::to_string(world.enemiesRemainingInWave());
    drawText(truncateText(mapLine, 60),
        {16.0f, 108.0f}, 16, sf::Color(210, 220, 255));
    drawText("MODS " + truncateText(world.mapModifier().name, 36)
        + "  |  Threat: "
        + truncateText(world.map().definition().encounter.threatDescription, 28),
        {16.0f, 130.0f}, 14, sf::Color(255, 220, 150));
    const bool bossArenaActive = world.map().bossTriggered()
        && !world.map().bossDefeated();
    const auto& activeThemeEffect = bossArenaActive
        ? world.map().definition().bossArenaEffect
        : world.map().definition().ambientEffect;
    drawText("THEME " + truncateText(world.map().definition().theme, 24)
        + "  |  " + (bossArenaActive ? "ARENA " : "FIELD ")
        + truncateText(activeThemeEffect.name, 24)
        + "  |  LOOT "
        + affixTagName(world.map().definition().signatureLootBias.primaryTag),
        {16.0f, 148.0f}, 14, sf::Color(170, 220, 255));
    const std::string bossLine = world.map().bossDefeated()
        ? "Boss defeated: " + world.bossDefinition().name
        : world.map().bossTriggered()
            ? "Boss active: " + world.bossDefinition().name
            : world.bossDefinition().name + " distance " + std::to_string(static_cast<int>(world.distanceToBoss()));
    drawText(truncateText(bossLine, 68),
        {16.0f, 174.0f}, 14, sf::Color(255, 190, 150));
    drawText(truncateText("Objective: " + world.mapObjective(), 68),
        {16.0f, 196.0f}, 14, sf::Color(210, 255, 210));
    float hudY = 218.0f;
    // Persistent event progress (always visible while exploring the map).
    drawText("Events " + std::to_string(world.mapEventsCompleted())
        + "/" + std::to_string(world.mapEventsTotal()),
        {16.0f, hudY}, 14, sf::Color(210, 255, 210));
    hudY += 18.0f;
    const bool gateUnlocked = world.bossGateUnlocked();
    drawText("Field Progress " + std::to_string(world.fieldPacksCleared())
        + "/" + std::to_string(world.fieldPacksRequired())
        + (gateUnlocked ? " | Boss Gate Unlocked" : " | Boss Gate Locked"),
        {16.0f, hudY}, 14,
        gateUnlocked ? sf::Color(150, 245, 175) : sf::Color(255, 170, 130));
    hudY += 18.0f;
    const std::string encounterStatus = mapEncounterStatus(world);
    if (!encounterStatus.empty()) {
        drawText(truncateText("Encounter: " + encounterStatus, 34),
            {16.0f, hudY}, 14, sf::Color(255, 190, 140));
        hudY += 18.0f;
    }
    if (world.fieldPackEnemiesRemaining() > 0) {
        std::string packLine = "Pack: " + world.fieldPackName()
            + " (" + std::to_string(world.fieldPackEnemiesRemaining()) + " left)";
        if (!world.fieldPackLeaderName().empty()) {
            packLine += " | Rare: " + world.fieldPackLeaderName();
        }
        drawText(truncateText(packLine, 34),
            {16.0f, hudY}, 14, sf::Color(210, 205, 255));
        hudY += 18.0f;
        if (!world.fieldPackLeaderRewardDescription().empty()) {
            drawText(truncateText("Rare loot: "
                    + world.fieldPackLeaderRewardDescription(), 34),
                {16.0f, hudY}, 12, sf::Color(255, 220, 150));
            hudY += 16.0f;
        }
    }
    if (!world.rareLeaderSkillWarning().empty()) {
        drawText(truncateText(world.rareLeaderSkillWarning(), 34),
            {16.0f, hudY}, 14, sf::Color(130, 220, 255));
        hudY += 18.0f;
    }
    if (!world.nearbyEventPrompt().empty()) {
        drawText(truncateText(world.nearbyEventPrompt(), 34), {16.0f, hudY}, 14, sf::Color(255, 235, 150));
        hudY += 18.0f;
    }
    if (!world.eventStatusMessage().empty() && world.eventStatusTimeRemaining() > 0.0f) {
        drawText(truncateText(world.eventStatusMessage(), 34), {16.0f, hudY}, 14, sf::Color(255, 220, 120));
        hudY += 18.0f;
    }
    if (world.activeEliteEventEnemiesRemaining() > 0) {
        std::string elitePackLine = activeMapEventLabel(world) + ": "
            + std::to_string(world.activeEliteEventEnemiesRemaining()) + " enemies left";
        const std::string modifierDescription = activeElitePackModifierDescription(world);
        if (!modifierDescription.empty()) {
            elitePackLine += " | " + modifierDescription;
        }
        drawText(truncateText(elitePackLine, 34), {16.0f, hudY}, 14, sf::Color(200, 140, 255));
        hudY += 18.0f;
    }
    if (const Enemy* focused = focusedEliteEnemy(world)) {
        std::string focusLine = "Focus: " + enemyDisplayLabel(world, *focused)
            + " HP " + std::to_string(std::max(0, focused->hp()))
            + "/" + std::to_string(std::max(0, focused->maxHp()));
        const std::string modifierDescription = eliteModifierDescription(*focused);
        if (!modifierDescription.empty()) {
            focusLine += " | " + modifierDescription;
        }
        drawText(truncateText(focusLine, 34), {16.0f, hudY}, 14, sf::Color(255, 215, 160));
        hudY += 18.0f;
    }
    if (const Enemy* summoner = focusedSummonerEnemy(world)) {
        const int activeMinions = static_cast<int>(std::count_if(
            world.enemies().begin(), world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isSummoned() && !enemy.isDead(); }
        ));
        const std::string priorityLine = "Priority: " + enemyDisplayLabel(world, *summoner)
            + " HP " + std::to_string(std::max(0, summoner->hp()))
            + "/" + std::to_string(std::max(0, summoner->maxHp()))
            + " | Minions " + std::to_string(activeMinions)
            + "/" + std::to_string(Config::MaxSummonerMinions);
        drawText(truncateText(priorityLine, 34), {16.0f, hudY}, 14,
            sf::Color(230, 170, 255));
        hudY += 18.0f;
    }
    const std::string pickupPrompt = world.pickupPrompt();
    if (!pickupPrompt.empty()) {
        const bool full = pickupPrompt.rfind("Inventory full", 0) == 0;
        drawText(truncateText(pickupPrompt, 34), {16.0f, hudY}, 14,
            full ? sf::Color(255, 90, 90) : sf::Color(180, 220, 255));
        hudY += 18.0f;
    } else if (world.inventoryFullPromptTimeRemaining() > 0.0f) {
        drawText("Inventory full", {16.0f, hudY}, 14, sf::Color(255, 90, 90));
        hudY += 18.0f;
    }
    if (world.shrineBuffTimeRemaining() > 0.0f) {
        drawText("Shrine Damage +"
            + std::to_string(Config::ShrineDamageBonusPercent)
            + "%  "
            + std::to_string(static_cast<int>(world.shrineBuffTimeRemaining() + 0.99f)) + "s",
            {16.0f, hudY}, 14, sf::Color(100, 240, 240));
        hudY += 18.0f;
    }
    drawText(truncateText("Build: " + world.passiveBuildSummary()
            + "  |  P Passive Tree  |  K Skills", 70),
        {16.0f, 152.0f}, 14, sf::Color(210, 255, 210));
    const auto& stats = world.player().stats();
    drawText(truncateText("DMG +" + std::to_string(multiplierPercent(stats.damageMultiplier))
        + "%  AS +" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier))
        + "%  MS +" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier))
        + "%  PDMG +" + std::to_string(multiplierPercent(stats.projectileDamageMultiplier))
        + "%  ADMG +" + std::to_string(multiplierPercent(stats.areaDamageMultiplier))
        + "%  AREA +" + std::to_string(multiplierPercent(stats.areaRadiusMultiplier))
        + "%  ARM " + std::to_string(stats.armor), 72),
        {16.0f, 84.0f}, 14, sf::Color(210, 220, 255));
    std::string elementalLine = "ELEM F/C/L/P DMG "
        + std::to_string(multiplierPercent(stats.fireDamageMultiplier)) + "/"
        + std::to_string(multiplierPercent(stats.coldDamageMultiplier)) + "/"
        + std::to_string(multiplierPercent(stats.lightningDamageMultiplier)) + "/"
        + std::to_string(multiplierPercent(stats.poisonDamageMultiplier))
        + "%  RES " + std::to_string(stats.fireResistance) + "/"
        + std::to_string(stats.coldResistance) + "/"
        + std::to_string(stats.lightningResistance) + "/"
        + std::to_string(stats.poisonResistance);
    if (!world.mapModifier().elementalChallengeId.empty()) {
        elementalLine += "  MAP "
            + std::string(damageTypeName(world.mapModifier().elementalChallengeType))
            + " RES -" + std::to_string(world.mapModifier().playerElementalResistancePenalty)
            + " / MRES +" + std::to_string(world.mapModifier().monsterElementalResistanceBonus);
    }
    drawText(truncateText(elementalLine, 64),
        {450.0f, 84.0f}, 12, sf::Color(235, 195, 150));
    std::string ailmentLine;
    if (world.player().isIgnited()) {
        ailmentLine += "Ignite " + formatFloat(world.player().igniteTimeRemaining(), 1) + "s  ";
    }
    if (world.player().isChilled()) {
        ailmentLine += "Chill " + formatFloat(world.player().chillTimeRemaining(), 1)
            + "s  ";
    }
    if (world.player().isShocked()) {
        ailmentLine += "Shock +"
            + std::to_string(multiplierPercent(world.player().damageTakenMultiplier()))
            + "% " + formatFloat(world.player().shockTimeRemaining(), 1) + "s";
    }
    if (world.player().isPoisoned()) {
        ailmentLine += "Poison x" + std::to_string(world.player().poisonStacks())
            + " " + formatFloat(world.player().poisonTimeRemaining(), 1) + "s";
    }
    if (!ailmentLine.empty()) {
        drawText(truncateText("Status " + ailmentLine, 64),
            {450.0f, 102.0f}, 12, sf::Color(255, 180, 210));
    }
    const std::string relicSummary = world.bossRelicEffectSummary();
    if (relicSummary != "None") {
        drawText(truncateText("Relics: " + relicSummary, 38),
            {450.0f, 120.0f}, 12, sf::Color(255, 225, 145));
    }
    drawSkillBar(world);
    drawEquipment(world);
    drawInventory(world);
    drawMinimap(world);
    drawBossHealth(world);
    drawPassiveTree(world);
    drawSkillPanel(world);

    switch (world.state()) {
        case GameState::GameOver:
            drawGameOver(world);
            break;
        case GameState::MapComplete:
            drawMapComplete(world);
            drawMapCompleteBossRelicPreview(world);
            drawMapCompleteInventoryPanel(world);
            drawMapCompleteStashPanel(world);
            drawMapDevicePanel(world);
            drawCraftingPanel(world);
            drawMapCompleteLootDetail(world);
            break;
        case GameState::Playing:
            drawCraftingPanel(world);
            break;
        case GameState::Paused:
            drawPause(world);
            break;
    }

    window_.display();
}

void Renderer::drawPause(const GameWorld& world) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 165));
    window_.draw(overlay);

    drawBox(center, {440.0f, 530.0f}, sf::Color(15, 20, 30));
    drawCenteredText("PAUSED", {center.x, 54.0f}, 28, sf::Color::White);

    float y = 94.0f;
    for (const auto& action : InputBindingLibrary::pauseActions()) {
        drawText(std::string(action.key) + "  " + action.action,
            {center.x - 170.0f, y}, 17, sf::Color(225, 235, 250));
        y += 24.0f;
    }

    drawText("INPUT HELP", {center.x - 170.0f, y + 6.0f}, 14, sf::Color(255, 220, 150));
    y += 28.0f;
    for (const auto& action : InputBindingLibrary::gameplayActions()) {
        drawText(std::string(action.key) + "  " + action.action,
            {center.x - 170.0f, y}, 12, sf::Color(185, 200, 220));
        y += 18.0f;
    }

    drawCenteredText("Simulation frozen", {center.x, height - 34.0f}, 13, sf::Color(170, 190, 210));
}

void Renderer::drawCraftingPanel(const GameWorld& world) {
    if (!world.craftingPanelOpen()) {
        return;
    }

    const int selectedIndex = world.selectedInventoryIndex();
    const auto& items = world.inventory().items();
    if (selectedIndex < 0 || static_cast<std::size_t>(selectedIndex) >= items.size()) {
        return;
    }

    const Item& item = items[static_cast<std::size_t>(selectedIndex)];
    const sf::Vector2f panelPosition{96.0f, 138.0f};
    const sf::Vector2f panelSize{608.0f, 350.0f};
    drawBox({panelPosition.x + panelSize.x / 2.0f, panelPosition.y + panelSize.y / 2.0f},
        panelSize, sf::Color(12, 16, 24));

    const float x = panelPosition.x + 16.0f;
    float y = panelPosition.y + 14.0f;
    drawText("Crafting: " + item.name, {x, y}, 18, rarityColor(item.rarity));
    y += 24.0f;
    drawText("Forge Fragments " + std::to_string(world.forgeFragments())
        + "  Costs I" + std::to_string(Config::ForgeImproveCost)
        + " R" + std::to_string(Config::ForgeRerollCost)
        + " T" + std::to_string(Config::ForgeRaiseTierCost),
        {x, y}, 13, sf::Color(210, 220, 235));
    y += 22.0f;
    drawText("1 Improve affix   2 Reroll affix   3 Raise affix tier",
        {x, y}, 13, sf::Color(255, 225, 160));
    y += 18.0f;
    drawText("Choose an operation, then F1-F3 to apply it to one affix. V / Esc Close",
        {x, y}, 12, sf::Color(160, 175, 195));
    y += 24.0f;

    const int selectedAffix = world.craftingAffixIndex();
    const CraftingOperation operation = world.craftingOperation();
    for (std::size_t index = 0; index < item.affixes.size(); ++index) {
        const auto& affix = item.affixes[index];
        const bool highlighted = static_cast<int>(index) == selectedAffix;
        const bool craftable = !affix.id.empty();
        std::string line = "F" + std::to_string(index + 1) + "  " + affix.name
            + "  T" + std::to_string(affix.tier) + "  " + statsSummary(affix.stats);
        if (!craftable) {
            line += "  [Fixed]";
        } else {
            line += craftingPreview(item, index, operation);
        }
        drawText(truncateText(line, 82), {x, y}, 13,
            highlighted ? sf::Color(255, 215, 90)
                : (craftable ? sf::Color(220, 230, 245) : sf::Color(145, 150, 160)));
        y += 20.0f;
    }

    y += 4.0f;
    const std::string operationText = "Operation: "
        + std::string(craftingOperationName(operation))
        + (operation == CraftingOperation::None
            ? ""
            : " (" + std::to_string(world.craftingCost()) + " Fragments)");
    drawText(operationText, {x, y}, 14,
        world.forgeFragments() >= world.craftingCost()
            ? sf::Color(180, 240, 200)
            : sf::Color(255, 120, 120));
    y += 20.0f;
    if (!world.eventStatusMessage().empty() && world.eventStatusTimeRemaining() > 0.0f) {
        drawText(world.eventStatusMessage(), {x, y}, 13, sf::Color(255, 220, 120));
    } else {
        drawText("Preview is deterministic except Reroll, which keeps prefix/suffix type.",
            {x, y}, 12, sf::Color(155, 170, 190));
    }
}

void Renderer::drawMap(const GameWorld& world) {
    const Vector2 camera = world.cameraTopLeft();
    const auto& map = world.map();
    const auto& palette = map.definition().palette;

    sf::RectangleShape floor({map.size().x, map.size().y});
    floor.setFillColor(mapColor(palette.floor));
    floor.setOutlineColor(sf::Color(80, 90, 96));
    floor.setOutlineThickness(6.0f);
    floor.setPosition({-camera.x, -camera.y});
    window_.draw(floor);

    for (const auto& obstacle : map.obstacles()) {
        sf::RectangleShape shape({obstacle.halfExtents.x * 2.0f, obstacle.halfExtents.y * 2.0f});
        shape.setFillColor(mapColor(palette.obstacle));
        shape.setOutlineColor(sf::Color(120, 130, 132));
        shape.setOutlineThickness(3.0f);
        shape.setOrigin({obstacle.halfExtents.x, obstacle.halfExtents.y});
        shape.setPosition(worldToScreen(world, obstacle.center));
        window_.draw(shape);
    }

    const sf::Vector2f bossCenter = worldToScreen(world, map.bossCenter());
    sf::CircleShape gate(Config::BossGateRadius);
    const sf::Color gateColor = world.bossGateUnlocked()
        ? mapColor(palette.bossGate)
        : sf::Color(170, 75, 75);
    gate.setFillColor(sf::Color(gateColor.r, gateColor.g, gateColor.b, 35));
    gate.setOutlineColor(sf::Color(gateColor.r, gateColor.g, gateColor.b, 150));
    gate.setOutlineThickness(3.0f);
    gate.setOrigin({Config::BossGateRadius, Config::BossGateRadius});
    gate.setPosition(bossCenter);
    window_.draw(gate);

    sf::CircleShape arena(Config::BossArenaRadius);
    arena.setFillColor(mapColor(palette.bossArena, 45));
    arena.setOutlineColor(mapColor(palette.bossArena, 160));
    arena.setOutlineThickness(4.0f);
    arena.setOrigin({Config::BossArenaRadius, Config::BossArenaRadius});
    arena.setPosition(bossCenter);
    window_.draw(arena);

    const sf::Vector2f startCenter = worldToScreen(world, map.playerStart());
    sf::CircleShape start(Config::StartSafeRadius);
    start.setFillColor(mapColor(palette.startArea, 45));
    start.setOutlineColor(mapColor(palette.startArea, 120));
    start.setOutlineThickness(3.0f);
    start.setOrigin({Config::StartSafeRadius, Config::StartSafeRadius});
    start.setPosition(startCenter);
    window_.draw(start);
}

void Renderer::drawAmbientHazardWarning(const GameWorld& world) {
    const float progress = world.ambientHazardWarningProgress();
    if (progress <= 0.0f) {
        return;
    }

    const bool bossArenaActive = world.map().bossTriggered()
        && !world.map().bossDefeated();
    const auto& effect = bossArenaActive
        ? world.map().definition().bossArenaEffect
        : world.map().definition().ambientEffect;
    const float radius = effect.hazard.radius;
    const auto alpha = static_cast<std::uint8_t>(
        85.0f + 130.0f * (1.0f - progress)
    );
    const sf::Color elementColor = damageTypeColor(effect.hazard.damageType);
    sf::Color fillColor = elementColor;
    fillColor.a = 24;
    sf::Color outlineColor = elementColor;
    outlineColor.a = alpha;

    const auto& warningPositions = world.ambientHazardWarningPositions();
    if (warningPositions.empty()) {
        return;
    }

    for (const auto& position : warningPositions) {
        sf::CircleShape warning(radius);
        warning.setFillColor(fillColor);
        warning.setOutlineColor(outlineColor);
        warning.setOutlineThickness(4.0f);
        warning.setOrigin({radius, radius});
        warning.setPosition(worldToScreen(world, position));
        window_.draw(warning);
    }

    drawCenteredText(
        "! " + effect.name,
        worldToScreen(world, warningPositions.front()
            + Vector2(0.0f, -radius - 16.0f)),
        12,
        sf::Color(255, 210, 130, alpha)
    );
}

void Renderer::drawGroundHazards(const GameWorld& world) {
    for (const auto& hazard : world.groundHazards()) {
        const auto& definition = hazard.definition();
        const float tickProgress = definition.tickInterval > 0.0f
            ? 1.0f - std::clamp(
                hazard.tickTimeRemaining() / definition.tickInterval, 0.0f, 1.0f
            )
            : 0.0f;
        const auto outlineAlpha = static_cast<std::uint8_t>(
            150.0f + 90.0f * tickProgress
        );
        const sf::Vector2f screenPosition = worldToScreen(world, hazard.position());
        const sf::Color elementColor = damageTypeColor(definition.damageType);
        sf::Color fillColor = elementColor;
        fillColor.a = 55;
        sf::Color outlineColor = elementColor;
        outlineColor.a = outlineAlpha;

        sf::CircleShape shape(definition.radius);
        shape.setFillColor(fillColor);
        shape.setOutlineColor(outlineColor);
        shape.setOutlineThickness(4.0f + 2.0f * tickProgress);
        shape.setOrigin({definition.radius, definition.radius});
        shape.setPosition(screenPosition);
        window_.draw(shape);

        drawCenteredText(
            definition.source + " "
                + std::to_string(static_cast<int>(std::ceil(hazard.timeRemaining()))) + "s",
            {screenPosition.x, screenPosition.y - 8.0f},
            11,
            sf::Color(255, 205, 120)
        );
    }
}

void Renderer::drawPendingSkillEffects(const GameWorld& world) {
    for (const auto& effect : world.pendingSkillEffects()) {
        const sf::Color elementColor = damageTypeColor(effect.damageType);
        const bool awaitingRepeat = effect.impacted
            && effect.impactsRemaining > 0
            && effect.impactDurationRemaining <= 0.0f;
        const bool awaitingImpact = !effect.impacted || awaitingRepeat;
        const float progress = awaitingImpact
            ? effect.delayDuration > 0.0f
                ? 1.0f - effect.delayRemaining / effect.delayDuration
                : 1.0f
            : 1.0f;
        const float radius = effect.radius * (0.82f + 0.18f * std::clamp(progress, 0.0f, 1.0f));
        const auto alpha = static_cast<std::uint8_t>(
            !awaitingImpact
                ? 190.0f * std::clamp(
                    effect.impactDuration > 0.0f
                        ? effect.impactDurationRemaining / effect.impactDuration
                        : 1.0f,
                    0.0f,
                    1.0f
                )
                : 80.0f + 130.0f * std::clamp(progress, 0.0f, 1.0f)
        );

        sf::CircleShape marker(radius);
        sf::Color fill = elementColor;
        fill.a = awaitingImpact ? 22 : alpha / 5;
        sf::Color outline = elementColor;
        outline.a = alpha;
        marker.setFillColor(fill);
        marker.setOutlineColor(outline);
        marker.setOutlineThickness(awaitingImpact ? 4.0f : 3.0f);
        marker.setOrigin({radius, radius});
        marker.setPosition(worldToScreen(world, effect.position));
        window_.draw(marker);

        if (awaitingImpact) {
            drawCenteredText(
                "! " + effect.source + (awaitingRepeat ? " next" : ""),
                worldToScreen(world, effect.position + Vector2(0.0f, -radius - 16.0f)),
                12,
                sf::Color(255, 220, 150, alpha)
            );
        }
    }
}

void Renderer::drawPlayer(const GameWorld& world) {
    const auto& player = world.player();
    const sf::Vector2f screenPosition = worldToScreen(world, player.position());
    const float hitProgress = world.playerHitEffectProgress();
    if (hitProgress > 0.0f) {
        const float pulseRadius = player.radius() + 8.0f + (1.0f - hitProgress) * 8.0f;
        const auto alpha = static_cast<std::uint8_t>(190.0f * hitProgress);
        sf::CircleShape pulse(pulseRadius);
        pulse.setFillColor(sf::Color(255, 60, 45, alpha / 6));
        pulse.setOutlineColor(sf::Color(255, 105, 75, alpha));
        pulse.setOutlineThickness(3.0f);
        pulse.setOrigin({pulseRadius, pulseRadius});
        pulse.setPosition(screenPosition);
        window_.draw(pulse);
    }

    if (player.isIgnited()) {
        const float radius = player.radius() + 8.0f;
        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(255, 115, 45, 220));
        ring.setOutlineThickness(3.0f);
        ring.setOrigin({radius, radius});
        ring.setPosition(screenPosition);
        window_.draw(ring);
    }
    if (player.isChilled()) {
        const float radius = player.radius() + 12.0f;
        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(105, 225, 255, 220));
        ring.setOutlineThickness(3.0f);
        ring.setOrigin({radius, radius});
        ring.setPosition(screenPosition);
        window_.draw(ring);
    }
    if (player.isShocked()) {
        const float radius = player.radius() + 16.0f;
        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(190, 145, 255, 220));
        ring.setOutlineThickness(3.0f);
        ring.setOrigin({radius, radius});
        ring.setPosition(screenPosition);
        window_.draw(ring);
    }
    if (player.isPoisoned()) {
        const float radius = player.radius() + 20.0f;
        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(105, 220, 105, 220));
        ring.setOutlineThickness(3.0f);
        ring.setOrigin({radius, radius});
        ring.setPosition(screenPosition);
        window_.draw(ring);
    }

    sf::CircleShape shape(player.radius());
    shape.setFillColor(hitProgress > 0.0f ? sf::Color(245, 105, 80) : sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition(screenPosition);
    window_.draw(shape);

}

void Renderer::drawNovaEffect(const GameWorld& world) {
    const float progress = world.novaEffectProgress();
    if (progress <= 0.0f) {
        return;
    }

    const auto& player = world.player();
    const float baseRadius = world.novaEffectRadius();
    const float radius = baseRadius * (1.0f - progress * 0.25f);
    const auto alpha = static_cast<std::uint8_t>(180.0f * progress);
    const sf::Color elementColor = damageTypeColor(
        world.skillBar().definition(SkillSlot::Utility).damageType
    );

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(
        elementColor.r, elementColor.g, elementColor.b, alpha / 4
    ));
    shape.setOutlineColor(sf::Color(
        elementColor.r, elementColor.g, elementColor.b, alpha
    ));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, player.position()));
    window_.draw(shape);
}

void Renderer::drawSecondarySkillEffect(const GameWorld& world) {
    const float progress = world.secondarySkillEffectProgress();
    if (progress <= 0.0f) {
        return;
    }

    const auto& center = world.secondarySkillEffectPosition();
    const float baseRadius = world.secondarySkillEffectRadius();
    const float radius = baseRadius * (1.0f - progress * 0.20f);
    const auto alpha = static_cast<std::uint8_t>(170.0f * progress);

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 180, 80, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 210, 120, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, center));
    window_.draw(shape);
}

void Renderer::drawDashImpactEffect(const GameWorld& world) {
    const float progress = world.dashImpactProgress();
    if (progress <= 0.0f) {
        return;
    }

    const float radius = world.dashImpactRadius() * (1.0f - progress * 0.18f);
    const auto alpha = static_cast<std::uint8_t>(190.0f * progress);
    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 225, 105, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 245, 175, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, world.dashImpactPosition()));
    window_.draw(shape);
}

void Renderer::drawBossAoeEffect(const GameWorld& world) {
    const float telegraphProgress = world.bossAoeTelegraphProgress();
    if (telegraphProgress > 0.0f) {
        const float radius = world.bossAoeRadius();
        const auto alpha = static_cast<std::uint8_t>(70.0f + 120.0f * (1.0f - telegraphProgress));
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color(180, 30, 20, alpha / 4));
        shape.setOutlineColor(sf::Color(255, 90, 60, alpha));
        shape.setOutlineThickness(4.0f);
        shape.setOrigin({radius, radius});
        shape.setPosition(worldToScreen(world, world.bossAoeCenter()));
        window_.draw(shape);
    }

    const float effectProgress = world.bossAoeEffectProgress();
    if (effectProgress <= 0.0f) {
        return;
    }

    const float radius = world.bossAoeRadius() * (1.0f - effectProgress * 0.15f);
    const auto alpha = static_cast<std::uint8_t>(190.0f * effectProgress);
    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 70, 35, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 160, 70, alpha));
    shape.setOutlineThickness(5.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, world.bossAoeCenter()));
    window_.draw(shape);
}

void Renderer::drawRareLeaderEffect(const GameWorld& world) {
    const float progress = world.rareLeaderAoeTelegraphProgress();
    if (progress <= 0.0f) {
        return;
    }

    const float radius = world.rareLeaderAoeRadius();
    const auto alpha = static_cast<std::uint8_t>(
        70.0f + 150.0f * (1.0f - progress)
    );
    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(80, 150, 255, alpha / 4));
    shape.setOutlineColor(sf::Color(120, 220, 255, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, world.rareLeaderAoeCenter()));
    window_.draw(shape);
}

void Renderer::drawBossDashEffect(const GameWorld& world) {
    const float telegraphProgress = world.bossDashTelegraphProgress();
    if (telegraphProgress > 0.0f || world.bossDashMoving()) {
        const sf::Vector2f start = worldToScreen(world, world.bossDashStart());
        const sf::Vector2f target = worldToScreen(world, world.bossDashTarget());
        const auto alpha = static_cast<std::uint8_t>(world.bossDashMoving()
            ? 130.0f
            : 90.0f + 150.0f * (1.0f - telegraphProgress));

        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = start;
        line[0].color = sf::Color(120, 220, 255, alpha);
        line[1].position = target;
        line[1].color = sf::Color(235, 250, 255, alpha);
        window_.draw(line);

        const float radius = world.bossDashRadius();
        sf::CircleShape landing(radius);
        landing.setFillColor(sf::Color(80, 185, 255, alpha / 7));
        landing.setOutlineColor(sf::Color(155, 235, 255, alpha));
        landing.setOutlineThickness(world.bossDashMoving() ? 2.0f : 4.0f);
        landing.setOrigin({radius, radius});
        landing.setPosition(target);
        window_.draw(landing);
    }

    const float effectProgress = world.bossDashEffectProgress();
    if (effectProgress <= 0.0f) {
        return;
    }

    const float radius = world.bossDashRadius() * (1.0f + (1.0f - effectProgress) * 0.20f);
    const auto alpha = static_cast<std::uint8_t>(220.0f * effectProgress);
    sf::CircleShape impact(radius);
    impact.setFillColor(sf::Color(95, 200, 255, alpha / 5));
    impact.setOutlineColor(sf::Color(205, 245, 255, alpha));
    impact.setOutlineThickness(4.0f);
    impact.setOrigin({radius, radius});
    impact.setPosition(worldToScreen(world, world.bossDashEffectPosition()));
    window_.draw(impact);
}

void Renderer::drawVolatileExplosionEffect(const GameWorld& world) {
    const float progress = world.volatileExplosionProgress();
    if (progress <= 0.0f) {
        return;
    }

    const float radius = world.volatileExplosionRadius() * (1.0f + (1.0f - progress) * 0.12f);
    const auto alpha = static_cast<std::uint8_t>(220.0f * progress);
    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 100, 45, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 185, 85, alpha));
    shape.setOutlineThickness(4.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, world.volatileExplosionCenter()));
    window_.draw(shape);
}

void Renderer::drawAimIndicator(const GameWorld& world) {
    const auto& player = world.player();
    const auto& aim = world.aimPosition();

    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = worldToScreen(world, player.position());
    line[0].color = sf::Color(120, 220, 255, 160);
    line[1].position = worldToScreen(world, aim);
    line[1].color = sf::Color(120, 220, 255, 80);
    window_.draw(line);

    constexpr float reticleSize = 8.0f;
    const sf::Vector2f aimScreen = worldToScreen(world, aim);
    sf::VertexArray reticle(sf::PrimitiveType::Lines, 4);
    reticle[0].position = {aimScreen.x - reticleSize, aimScreen.y};
    reticle[1].position = {aimScreen.x + reticleSize, aimScreen.y};
    reticle[2].position = {aimScreen.x, aimScreen.y - reticleSize};
    reticle[3].position = {aimScreen.x, aimScreen.y + reticleSize};

    for (std::size_t i = 0; i < reticle.getVertexCount(); ++i) {
        reticle[i].color = sf::Color(120, 220, 255);
    }

    window_.draw(reticle);
}

void Renderer::drawProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.projectiles()) {
        sf::CircleShape shape(projectile.radius());
        sf::Color color = damageTypeColor(projectile.damageType());
        if (projectile.ailment().type == AilmentType::Ignite) {
            color = sf::Color(255, 125, 45);
        } else if (projectile.ailment().type == AilmentType::Chill) {
            color = sf::Color(105, 225, 255);
        } else if (projectile.ailment().type == AilmentType::Poison) {
            color = sf::Color(105, 220, 105);
        }
        shape.setFillColor(color);
        shape.setOrigin({projectile.radius(), projectile.radius()});
        shape.setPosition(worldToScreen(world, projectile.position()));
        window_.draw(shape);
    }
}

void Renderer::drawBossProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.bossProjectiles()) {
        sf::CircleShape shape(projectile.radius);
        shape.setFillColor(damageTypeColor(projectile.damageType));
        sf::Color outline = damageTypeColor(projectile.damageType);
        outline.a = 220;
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(2.0f);
        shape.setOrigin({projectile.radius, projectile.radius});
        shape.setPosition(worldToScreen(world, projectile.position));
        window_.draw(shape);
    }
}

void Renderer::drawEnemyProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.enemyProjectiles()) {
        sf::CircleShape shape(projectile.radius);
        shape.setFillColor(damageTypeColor(projectile.damageType));
        sf::Color outline = damageTypeColor(projectile.damageType);
        outline.a = 230;
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(1.5f);
        shape.setOrigin({projectile.radius, projectile.radius});
        shape.setPosition(worldToScreen(world, projectile.position));
        window_.draw(shape);
    }
}

void Renderer::drawEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies()) {
        const auto& definition = EnemyLibrary::forType(enemy.type());
        const auto& modifier = EliteModifierLibrary::forModifier(enemy.eliteModifier());
        const sf::Vector2f screenPosition = worldToScreen(world, enemy.position());
        if (enemy.isRare()) {
            const auto drawAura = [&](EliteModifier eliteModifier) {
                const auto& auraDefinition = EliteModifierLibrary::forModifier(eliteModifier);
                if (auraDefinition.auraRadius <= 0.0f) {
                    return;
                }

                const bool damageAura = auraDefinition.allyDamageMultiplier > 1.0f;
                const sf::Color color = damageAura
                    ? sf::Color(255, 100, 190)
                    : sf::Color(105, 235, 150);
                sf::CircleShape aura(auraDefinition.auraRadius);
                aura.setFillColor(sf::Color(color.r, color.g, color.b, 12));
                aura.setOutlineColor(sf::Color(color.r, color.g, color.b, 125));
                aura.setOutlineThickness(2.0f);
                aura.setOrigin({auraDefinition.auraRadius, auraDefinition.auraRadius});
                aura.setPosition(screenPosition);
                window_.draw(aura);
            };

            drawAura(enemy.eliteModifier());
            if (enemy.secondaryEliteModifier() != enemy.eliteModifier()) {
                drawAura(enemy.secondaryEliteModifier());
            }
        }
        if (enemy.isWarden()) {
            const float auraRadius = Config::WardenAuraRadius;
            sf::CircleShape aura(auraRadius);
            aura.setFillColor(sf::Color(70, 190, 235, 18));
            aura.setOutlineColor(sf::Color(115, 225, 255, 145));
            aura.setOutlineThickness(2.0f);
            aura.setOrigin({auraRadius, auraRadius});
            aura.setPosition(screenPosition);
            window_.draw(aura);
        }
        if (enemy.isAttackWindingUp()) {
            if (enemy.isCharger()) {
                const sf::Vector2f chargeTarget = worldToScreen(
                    world,
                    enemy.chargeTargetPosition(world.mapModifier().monsterSpeedMultiplier)
                );
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0].position = screenPosition;
                line[0].color = sf::Color(255, 210, 100, 210);
                line[1].position = chargeTarget;
                line[1].color = sf::Color(255, 125, 55, 120);
                window_.draw(line);

                sf::CircleShape target(12.0f);
                target.setFillColor(sf::Color(255, 125, 55, 28));
                target.setOutlineColor(sf::Color(255, 215, 100, 220));
                target.setOutlineThickness(2.0f);
                target.setOrigin({12.0f, 12.0f});
                target.setPosition(chargeTarget);
                window_.draw(target);
            } else {
            sf::CircleShape warning(enemy.attackRange());
            sf::Color warningColor = enemy.isSummoner()
                ? sf::Color(220, 125, 255, 220)
                : enemy.isRanged()
                    ? sf::Color(255, 220, 75, 210)
                    : sf::Color(255, 110, 75, 210);
            if (enemy.eliteModifier() != EliteModifier::None) {
                warningColor = sf::Color(
                    modifier.outlineColor.r,
                    modifier.outlineColor.g,
                    modifier.outlineColor.b,
                    220
                );
            }
            const float warningRadius = enemy.isSummoner() ? 34.0f : enemy.attackRange();
            warning.setRadius(warningRadius);
            warning.setFillColor(sf::Color(warningColor.r, warningColor.g, warningColor.b, 28));
            warning.setOutlineColor(warningColor);
            warning.setOutlineThickness(enemy.isElite() ? 3.0f : 2.0f);
            warning.setOrigin({warningRadius, warningRadius});
            warning.setPosition(screenPosition);
            window_.draw(warning);
            }
        }

        sf::RectangleShape shape({enemy.radius() * 2, enemy.radius() * 2});
        shape.setFillColor(enemyColor(definition.fillColor));
        if (definition.outlineThickness > 0.0f) {
            shape.setOutlineColor(enemy.isRare()
                ? sf::Color(255, 220, 90)
                : enemy.eliteModifier() == EliteModifier::None
                ? enemyColor(definition.outlineColor)
                : enemyColor(modifier.outlineColor));
            shape.setOutlineThickness(enemy.isRare()
                ? definition.outlineThickness + 1.5f : definition.outlineThickness);
        }

        if ((enemy.isElite() || enemy.isSummoner()) && !enemy.isBoss()) {
            const float barWidth = enemy.radius() * 2.0f;
            const float hpRatio = enemy.maxHp() > 0
                ? static_cast<float>(std::max(0, enemy.hp())) / static_cast<float>(enemy.maxHp())
                : 0.0f;
            sf::RectangleShape background({barWidth, 4.0f});
            background.setFillColor(sf::Color(35, 30, 40, 220));
            background.setOrigin({barWidth * 0.5f, 2.0f});
            background.setPosition({screenPosition.x, screenPosition.y - enemy.radius() - 8.0f});
            window_.draw(background);

            sf::RectangleShape fill({barWidth * hpRatio, 4.0f});
            fill.setFillColor(enemy.isRare()
                ? sf::Color(255, 220, 90)
                : enemy.eliteModifier() == EliteModifier::None
                ? enemyColor(definition.outlineColor)
                : enemyColor(modifier.outlineColor));
            fill.setOrigin({barWidth * 0.5f, 2.0f});
            fill.setPosition({screenPosition.x - barWidth * (1.0f - hpRatio) * 0.5f,
                screenPosition.y - enemy.radius() - 8.0f});
            window_.draw(fill);
        }
        shape.setOrigin({enemy.radius(), enemy.radius()});
        shape.setPosition(screenPosition);
        window_.draw(shape);

        if (enemy.isIgnited()) {
            const float ringRadius = enemy.radius() + 5.0f;
            sf::CircleShape ring(ringRadius);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(255, 120, 45, 230));
            ring.setOutlineThickness(2.5f);
            ring.setOrigin({ringRadius, ringRadius});
            ring.setPosition(screenPosition);
            window_.draw(ring);
        }
        if (enemy.isChilled()) {
            const float ringRadius = enemy.radius() + (enemy.isIgnited() ? 9.0f : 5.0f);
            sf::CircleShape ring(ringRadius);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(105, 225, 255, 230));
            ring.setOutlineThickness(2.0f);
            ring.setOrigin({ringRadius, ringRadius});
            ring.setPosition(screenPosition);
            window_.draw(ring);
        }
        if (enemy.isShocked()) {
            const float ringRadius = enemy.radius()
                + (enemy.isIgnited() ? 13.0f : enemy.isChilled() ? 9.0f : 5.0f);
            sf::CircleShape ring(ringRadius);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(195, 145, 255, 235));
            ring.setOutlineThickness(2.5f);
            ring.setOrigin({ringRadius, ringRadius});
            ring.setPosition(screenPosition);
            window_.draw(ring);
        }
        if (enemy.isPoisoned()) {
            const float ringRadius = enemy.radius()
                + (enemy.isIgnited() ? 17.0f
                    : enemy.isChilled() ? 13.0f
                    : enemy.isShocked() ? 9.0f : 5.0f);
            sf::CircleShape ring(ringRadius);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(105, 220, 105, 235));
            ring.setOutlineThickness(2.5f);
            ring.setOrigin({ringRadius, ringRadius});
            ring.setPosition(screenPosition);
            window_.draw(ring);
        }

        if (definition.outlineThickness > 0.0f) {
            const std::string label = enemyDisplayLabel(world, enemy);
            drawCenteredText(label, {screenPosition.x, screenPosition.y - enemy.radius() - 18.0f},
                11, enemy.isRare()
                    ? sf::Color(255, 220, 90)
                    : enemy.eliteModifier() == EliteModifier::None
                    ? enemyColor(definition.outlineColor)
                    : enemyColor(modifier.outlineColor));
        }
    }
}

void Renderer::drawCombatFeedback(const GameWorld& world) {
    const float duration = Config::CombatFeedbackDuration;
    for (const auto& feedback : world.combatFeedback()) {
        const float progress = duration > 0.0f
            ? 1.0f - feedback.timeRemaining / duration
            : 1.0f;
        const float rise = std::clamp(progress, 0.0f, 1.0f) * 24.0f;
        const auto alpha = static_cast<std::uint8_t>(
            90.0f + (duration > 0.0f
                ? std::clamp(feedback.timeRemaining / duration, 0.0f, 1.0f)
                : 0.0f) * 165.0f
        );
        std::string text;
        sf::Color color;
        switch (feedback.type) {
            case CombatFeedbackType::Damage:
                text = "-" + std::to_string(feedback.damage) + " " + feedback.source;
                color = feedback.source == "Ignite"
                    ? sf::Color(255, 155, 90, alpha)
                    : feedback.source == "Poison"
                        ? sf::Color(120, 225, 120, alpha)
                        : sf::Color(255, 235, 150, alpha);
                break;
            case CombatFeedbackType::PlayerHit:
                text = "HIT -" + std::to_string(feedback.damage) + " " + feedback.source;
                color = sf::Color(255, 105, 90, alpha);
                break;
            case CombatFeedbackType::SkillRejected:
                text = feedback.source;
                color = sf::Color(255, 165, 100, alpha);
                break;
            case CombatFeedbackType::Telegraph:
                text = "! " + feedback.source;
                color = sf::Color(255, 120, 90, alpha);
                break;
            case CombatFeedbackType::Status:
                text = feedback.source;
                color = feedback.source == "Contagion"
                    ? sf::Color(130, 235, 130, alpha)
                    : sf::Color(205, 155, 255, alpha);
                break;
        }
        const Vector2 textPosition(feedback.position.x, feedback.position.y - 24.0f - rise);
        drawCenteredText(
            text,
            worldToScreen(world, textPosition),
            11,
            color
        );
    }
}

void Renderer::drawDroppedItems(const GameWorld& world) {
    const auto& player = world.player();
    const int focusedIndex = world.focusedDroppedItemIndex();

    for (std::size_t i = 0; i < world.droppedItems().size(); ++i) {
        const auto& droppedItem = world.droppedItems()[i];
        const auto& item = droppedItem.item();
        const bool isFocused = static_cast<int>(i) == focusedIndex;

        sf::RectangleShape shape({droppedItem.radius() * 2.0f, droppedItem.radius() * 2.0f});
        shape.setFillColor(rarityColor(item.rarity));
        if (isFocused) {
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(2.0f);
        }
        shape.setOrigin({droppedItem.radius(), droppedItem.radius()});
        shape.setPosition(worldToScreen(world, droppedItem.position()));
        window_.draw(shape);

        std::string prefix;
        if (isFocused) {
            prefix = world.inventory().isFull() ? "FULL " : "F ";
        }
        const std::string label = prefix + item.name + " [" + slotName(item.slot) + "]";
        drawCenteredText(label,
            worldToScreen(world, Vector2(droppedItem.position().x, droppedItem.position().y - 20.0f)),
            12,
            rarityColor(item.rarity));
    }
}

void Renderer::drawSkillBar(const GameWorld& world) {
    const SkillSlot slots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };
    const char* keys[] = {"LMB", "RMB", "Q", "Space"};

    float x = 16.0f;
    const float y = static_cast<float>(Config::WindowHeight) - 34.0f;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = slots[i];
        const auto& skill = world.skillBar().definition(slot);
        const float progress = world.skillBar().cooldownProgress(slot);
        const sf::Color color = progress >= 1.0f ? sf::Color(130, 230, 150) : sf::Color(230, 180, 80);
        drawText(std::string(keys[i]) + " " + skill.name + " "
            + "Lv" + std::to_string(world.skillLevel(skill.name)) + " "
            + std::to_string(static_cast<int>(progress * 100.0f)) + "%"
            + " M" + formatFloat(skill.manaCost, 0),
            {x, y}, 13, color);
        x += 150.0f;
    }
}

void Renderer::drawEquipment(const GameWorld& world) {
    const auto& equipment = world.player().equipment();
    const float x = static_cast<float>(Config::WindowWidth) - 260.0f;
    float y = 12.0f;

    drawText("Equipped", {x, y}, 16, sf::Color::White);
    y += 22.0f;

    const EquipmentSlot slots[] = {
        EquipmentSlot::Weapon,
        EquipmentSlot::Armor,
        EquipmentSlot::Ring,
        EquipmentSlot::Amulet
    };

    for (const auto slot : slots) {
        const auto& item = equipment.itemInSlot(slot);
        const std::string line = std::string(slotName(slot)) + ": "
            + (item ? itemSummary(*item) : "Empty");
        drawText(truncateText(line, 36), {x, y}, 12,
            item
                ? (itemRequirementMet(world, *item)
                    ? rarityColor(item->rarity)
                    : sf::Color(255, 90, 90))
                : sf::Color(150, 150, 150));
        y += 17.0f;
    }
}

void Renderer::drawInventory(const GameWorld& world) {
    // During MapComplete the bag is drawn by drawMapCompleteInventoryPanel() AFTER
    // the settlement overlay, so it stays bright. Skip the dimmed duplicate here.
    if (world.state() == GameState::MapComplete) {
        return;
    }
    const auto& items = world.inventory().items();
    const auto& equipment = world.player().equipment();
    const float x = static_cast<float>(Config::WindowWidth) - 260.0f;
    const float listRight = static_cast<float>(Config::WindowWidth) - 4.0f;

    // Identify the inventory row currently under the mouse (screen space) so we
    // can show a hover detail panel without bloating the compact list.
    const sf::Vector2f mouse = worldToScreen(world, world.aimPosition());
    std::size_t hovered = items.size();
    {
        float bandY = 156.0f;
        const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
        for (std::size_t i = 0; i < visibleCount; ++i) {
            const bool hasCurrent = static_cast<bool>(equipment.itemInSlot(items[i].slot));
            const float rowH = 16.0f + (hasCurrent ? 14.0f : 0.0f);
            if (mouse.x >= x - 8.0f && mouse.x <= listRight
                && mouse.y >= bandY - 2.0f && mouse.y <= bandY + rowH) {
                hovered = i;
                break;
            }
            bandY += rowH;
        }
    }

    const bool inventoryFull = world.inventory().isFull();
    const std::string inventoryTitle = "Inventory "
        + std::to_string(world.inventory().size()) + "/"
        + std::to_string(world.inventory().capacity())
        + "  Forge " + std::to_string(world.forgeFragments());
    float y = 118.0f;
    drawText(inventoryTitle, {x, y}, 16, inventoryFull ? sf::Color(255, 90, 90) : sf::Color::White);
    y += 22.0f;
    if (inventoryFull) {
        drawText("Inventory full - equip or drop an item", {x, y}, 12, sf::Color(255, 90, 90));
        y += 16.0f;
    } else {
        drawText("Tab Select  Del Drop  C Salvage  V Craft", {x, y}, 12, sf::Color(150, 160, 175));
        y += 16.0f;
    }

    const int selectedIndex = world.selectedInventoryIndex();
    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const auto& item = items[i];
        const bool isSelected = !world.stashSelectionActive()
            && static_cast<int>(i) == selectedIndex;
        const bool isHovered = (i == hovered);
        const std::string line = (isSelected ? "> " : "") + std::to_string(i + 1) + ". "
            + item.name + " [" + slotName(item.slot) + "] "
            + itemRequirementSummary(item) + " " + statsSummary(item.stats);
        sf::Color rowColor = rarityColor(item.rarity);
        if (!itemRequirementMet(world, item)) {
            rowColor = sf::Color(255, 90, 90);
        } else if (isSelected) {
            rowColor = sf::Color(255, 215, 90);
        } else if (isHovered) {
            rowColor = sf::Color::White;
        }
        drawText(truncateText(line, 36), {x, y}, 13, rowColor);
        y += 16.0f;

        const auto& current = equipment.itemInSlot(item.slot);
        if (current) {
            const Stats delta = statsDelta(item.stats, current->stats);
            drawText("   Delta: " + statsDeltaSummary(delta), {x, y}, 11, deltaColor(delta));
            y += 14.0f;
        }
    }

    // Detail panel priority:
    //   1. hovered inventory item (player is inspecting the bag)
    //   2. focused ground item (what F would pick up, if no bag hover)
    //   3. selected inventory item (fallback when nothing is under the cursor)
    // Suppressed while Passive Tree / Skill Panel are open (would cover those overlays)
    // and during MapComplete (the settlement screen owns the stage; the focused Boss
    // drop is shown by drawMapCompleteLootDetail() AFTER the overlay instead).
    if (world.state() != GameState::MapComplete) {
        const int focusedGroundIndex = world.focusedDroppedItemIndex();
        const bool showGroundDetail = focusedGroundIndex >= 0
            && static_cast<std::size_t>(focusedGroundIndex) < world.droppedItems().size()
            && !world.passiveTreeOpen()
            && !world.skillPanelOpen();

        const sf::Vector2f inventoryDetailPos{16.0f, 300.0f};
        if (hovered < items.size()) {
            const int key = static_cast<int>(hovered) + 1;
            drawItemDetailPanel(world, inventoryDetailPos, items[hovered], equipment.itemInSlot(items[hovered].slot),
                "Hovered", std::to_string(key) + " Equip  |  Tab Select");
        } else if (showGroundDetail) {
            const auto& groundItem = world.droppedItems()[static_cast<std::size_t>(focusedGroundIndex)].item();
            const std::string groundActionHint = world.inventory().isFull()
                ? "Inventory full - equip or drop an item"
                : "F Pick up";
            drawItemDetailPanel(world, inventoryDetailPos, groundItem, equipment.itemInSlot(groundItem.slot),
                "Pickup Target", groundActionHint);
        } else if (selectedIndex >= 0 && static_cast<std::size_t>(selectedIndex) < items.size()) {
            const int key = selectedIndex + 1;
            drawItemDetailPanel(world, inventoryDetailPos, items[selectedIndex], equipment.itemInSlot(items[selectedIndex].slot),
                "Selected", std::to_string(key) + " Equip  |  Del Drop  |  C Salvage  |  V Craft");
        }
    }
}

void Renderer::drawItemDetailPanel(const GameWorld& world,
    const sf::Vector2f& panelPos,
    const Item& item,
    const std::optional<Item>& current,
    const std::string& statusLabel,
    const std::string& actionHint,
    bool compact) {
    const sf::Vector2f panelSize = compact
        ? sf::Vector2f{300.0f, 240.0f}
        : sf::Vector2f{500.0f, 250.0f};
    drawBox({panelPos.x + panelSize.x / 2.0f, panelPos.y + panelSize.y / 2.0f}, panelSize, sf::Color(18, 22, 30));

    const float x = panelPos.x + 14.0f;
    float y = panelPos.y + 12.0f;

    const std::size_t titleLimit = compact ? 39 : 56;
    const std::size_t bodyLimit = compact ? 39 : 62;
    const std::string title = item.name + "  [" + rarityName(item.rarity) + "]  " + statusLabel;
    drawText(truncateText(title, titleLimit),
        {x, y}, 16, rarityColor(item.rarity));
    y += 20.0f;

    const std::string requirementLine = "Slot: " + std::string(slotName(item.slot))
        + "   iLvl: " + std::to_string(item.itemLevel)
        + "   " + itemRequirementSummary(item);
    drawText(truncateText(requirementLine, bodyLimit), {x, y}, 12,
        itemRequirementMet(world, item) ? sf::Color(200, 210, 225) : sf::Color(255, 90, 90));
    y += 18.0f;

    if (!compact) {
        const auto* base = ItemBaseLibrary::find(item.baseId);
        const std::string baseSummary = base == nullptr
            ? "Base: " + (item.baseName.empty() ? "Unknown" : item.baseName)
                + "  Theme: Unknown"
            : "Base: " + base->name + "  Theme: "
                + itemBuildThemeName(base->buildTheme);
        drawText(truncateText(baseSummary, bodyLimit),
            {x, y}, 12, sf::Color(220, 205, 165));
        y += 16.0f;

        const std::string implicitSummary = statsSummary(item.implicitStats);
        drawText(truncateText("Implicit: " + (implicitSummary.empty() ? "None" : implicitSummary), bodyLimit),
            {x, y}, 11, sf::Color(220, 205, 165));
        y += 16.0f;
    }

    const std::string effectSummary = bossRelicEffectSummary(item);
    if (!effectSummary.empty()) {
        drawText(truncateText("Effect: " + effectSummary, bodyLimit),
            {x, y}, compact ? 10 : 11, sf::Color(255, 180, 80));
        y += compact ? 15.0f : 16.0f;
    }

    // Keep the full panel above the skill bar even for high-affix rare items.
    // The detail summary still reports how many affixes were omitted.
    // A Unique relic uses one line for its effect, so show one fewer affix and
    // keep the panel height stable instead of pushing the skill preview away.
    const std::size_t normalMaxVisibleAffixes = compact ? 2 : 3;
    const std::size_t maxVisibleAffixes = effectSummary.empty()
        ? normalMaxVisibleAffixes
        : normalMaxVisibleAffixes - 1;
    const std::size_t affixCount = std::min(item.affixes.size(), maxVisibleAffixes);
    for (std::size_t index = 0; index < affixCount; ++index) {
        const auto& affix = item.affixes[index];
        const std::string tags = affixTagsSummary(affix.tags);
        drawText(truncateText(
                "- " + affix.name + " T" + std::to_string(affix.tier)
                    + (tags.empty() ? "" : " [" + tags + "]"),
                bodyLimit),
            {x, y}, 11, sf::Color(160, 200, 255));
        y += 15.0f;
    }
    if (item.affixes.size() > affixCount) {
        drawText("+" + std::to_string(item.affixes.size() - affixCount) + " more affixes",
            {x, y}, 10, sf::Color(150, 165, 185));
        y += 14.0f;
    }

    drawText(truncateText("Stats: " + statsSummary(item.stats), bodyLimit),
        {x, y}, 11, sf::Color(210, 220, 235));
    y += 16.0f;

    if (current) {
        const auto* currentBase = ItemBaseLibrary::find(current->baseId);
        const std::string currentTheme = currentBase == nullptr
            ? "Unknown"
            : itemBuildThemeName(currentBase->buildTheme);
        drawText(truncateText("Current: " + current->name + " [" + currentTheme + "]  "
                + statsSummary(current->stats), bodyLimit),
            {x, y}, 11, sf::Color(200, 200, 200));
    } else {
        drawText("Current: Empty", {x, y}, 11, sf::Color(150, 150, 150));
    }
    y += 16.0f;

    if (current) {
        const Stats delta = statsDelta(item.stats, current->stats);
        drawText(truncateText("Delta: " + statsDeltaSummary(delta), bodyLimit),
            {x, y}, 11, deltaColor(delta));
    } else {
        const std::string delta = statsSummary(item.stats);
        drawText(truncateText("Delta: " + (delta.empty() ? "No stat change" : delta), bodyLimit),
            {x, y}, 11, deltaColor(item.stats));
    }
    y += 17.0f;

    drawText("Skill impact if equipped:", {x, y}, 11, sf::Color(190, 200, 215));
    y += 16.0f;

    const Stats before = world.player().stats();
    const Stats after = combineStats(before,
        statsDelta(previewEquipmentStats(world.player().equipment(), item), world.player().equipment().combinedStats()));
    for (const auto& line : skillImpactDetailLines(before, after, world.skillBar())) {
        drawText(truncateText(line, bodyLimit), {x, y}, 11, sf::Color(180, 210, 255));
        y += 15.0f;
    }

    y += 6.0f;
    const bool isPickup = actionHint.rfind("F Pick up", 0) == 0;
    const bool isFull = actionHint.rfind("Inventory full", 0) == 0;
    const sf::Color hintColor = isFull ? sf::Color(255, 90, 90)
        : isPickup ? sf::Color(180, 220, 255)
        : sf::Color(200, 220, 240);
    drawText(truncateText(actionHint, bodyLimit), {x, y}, 12, hintColor);
}

void Renderer::drawPassiveTree(const GameWorld& world) {
    if (!world.passiveTreeOpen()) {
        return;
    }

    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 145));
    window_.draw(overlay);

    drawBox({center.x, center.y}, {760.0f, 520.0f}, sf::Color(22, 28, 36));
    drawCenteredText("Passive Tree", {center.x, center.y - 238.0f}, 24, sf::Color::White);
    drawCenteredText("SP " + std::to_string(world.player().talentPoints())
        + "  |  Left click node  |  1-0/F1-F15 allocate  |  P close",
        {center.x, center.y - 210.0f}, 14, sf::Color(210, 230, 255));

    const auto& nodes = world.player().passiveTree().nodes();

    const auto nodeScreenPosition = [&](const PassiveNode& node) {
        return sf::Vector2f{
            center.x + node.treePosition.x,
            center.y + node.treePosition.y
        };
    };

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const sf::Vector2f from = node.prerequisite >= 0
            ? nodeScreenPosition(nodes[static_cast<std::size_t>(node.prerequisite)])
            : center;
        const sf::Vector2f to = nodeScreenPosition(node);
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        const sf::Color branchColor = passiveBranchColor(node.branch);
        line[0].position = from;
        line[1].position = to;
        line[0].color = node.allocated ? branchColor : sf::Color(90, 100, 112);
        line[1].color = node.allocated ? branchColor : sf::Color(90, 100, 112);
        window_.draw(line);
    }

    sf::CircleShape origin(9.0f);
    origin.setOrigin({9.0f, 9.0f});
    origin.setPosition(center);
    origin.setFillColor(sf::Color(190, 200, 215));
    window_.draw(origin);

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const bool prerequisiteMet = node.prerequisite < 0
            || nodes[static_cast<std::size_t>(node.prerequisite)].allocated;
        const bool available = !node.allocated && prerequisiteMet && world.player().talentPoints() > 0;
        const bool hovered = world.hoveredPassiveNode() == static_cast<int>(i);

        const sf::Color branchColor = passiveBranchColor(node.branch);
        sf::Color fill = sf::Color(54, 60, 70);
        sf::Color outline = sf::Color(120, 130, 145);
        if (node.allocated) {
            fill = branchColor;
            outline = sf::Color::White;
        } else if (available) {
            fill = sf::Color(branchColor.r / 3, branchColor.g / 3, branchColor.b / 3);
            outline = branchColor;
        } else if (prerequisiteMet) {
            outline = sf::Color(180, 180, 180);
        }

        if (hovered) {
            outline = sf::Color::White;
        }

        const float radius = node.size == PassiveNodeSize::Notable ? 16.0f : 12.0f;
        const sf::Vector2f position = nodeScreenPosition(node);
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        shape.setPosition(position);
        shape.setFillColor(fill);
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(hovered ? 3.0f : 2.0f);
        window_.draw(shape);

        drawCenteredText(passiveKeyLabel(i), {position.x, position.y - 6.0f}, 10, sf::Color::White);
    }

    const int hoveredIndex = world.hoveredPassiveNode();
    if (hoveredIndex >= 0 && hoveredIndex < static_cast<int>(nodes.size())) {
        const auto& node = nodes[static_cast<std::size_t>(hoveredIndex)];
        const bool prerequisiteMet = node.prerequisite < 0
            || nodes[static_cast<std::size_t>(node.prerequisite)].allocated;
        std::string status = "Locked";
        sf::Color statusColor = sf::Color(170, 170, 170);
        if (node.allocated) {
            status = "Allocated";
            statusColor = sf::Color(130, 240, 150);
        } else if (prerequisiteMet && world.player().talentPoints() > 0) {
            status = "Available";
            statusColor = sf::Color(220, 240, 255);
        } else if (prerequisiteMet) {
            status = "Need SP";
            statusColor = sf::Color(230, 215, 150);
        }

        drawText(passiveBranchName(node.branch) + " / " + status,
            {center.x - 350.0f, center.y + 218.0f}, 13, statusColor);
        const std::string keystoneLabel = node.keystone == PassiveKeystone::None
            ? ""
            : " [" + std::string(passiveKeystoneName(node.keystone)) + "]";
        drawText(node.name + keystoneLabel + " - " + node.description,
            {center.x - 350.0f, center.y + 238.0f}, 14, passiveBranchColor(node.branch));
    } else {
        drawText("Hover a node to inspect it",
            {center.x - 350.0f, center.y + 232.0f}, 14, sf::Color(190, 200, 215));
    }
}

void Renderer::drawSkillPanel(const GameWorld& world) {
    if (!world.skillPanelOpen()) {
        return;
    }

    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 145));
    window_.draw(overlay);

    drawBox({center.x, center.y}, {760.0f, 560.0f}, sf::Color(24, 30, 40));
    drawCenteredText("Skill Panel", {center.x, center.y - 248.0f}, 24, sf::Color::White);
    drawCenteredText("1-0 / F7-F8 assign unlocked skill  |  F1-F4 cycle  |  F5/F6 link  |  K close",
        {center.x, center.y - 220.0f}, 14, sf::Color(210, 230, 255));

    const SkillSlot slots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };

    const float leftColumn = center.x - 350.0f;
    const float rightColumn = center.x + 20.0f;
    const float equippedY = center.y - 188.0f;
    drawText("Equipped", {leftColumn, equippedY}, 16, sf::Color::White);
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = slots[i];
        const auto& skill = world.skillBar().definition(slot);
        const float x = i % 2 == 0 ? leftColumn : rightColumn;
        const float y = equippedY + 22.0f + static_cast<float>(i / 2) * 18.0f;
        drawText(skillSlotName(slot) + ": " + skill.name
                + " Lv" + std::to_string(world.skillLevel(skill.name)),
            {x, y}, 13, sf::Color(180, 230, 255));
    }

    const float skillsY = center.y - 132.0f;
    drawText("Skills", {leftColumn, skillsY}, 16, sf::Color::White);

    const auto& skills = SkillLibrary::all();
    for (std::size_t i = 0; i < skills.size(); ++i) {
        const auto& baseSkill = skills[i];
        const SkillDefinition skill = SkillProgression::skillAtLevel(
            baseSkill, world.skillLevel(baseSkill.name)
        );
        const bool equipped = world.skillBar().definition(skill.slot).name == skill.name;
        const bool unlocked = world.isSkillUnlocked(skill.name);
        const sf::Color color = !unlocked ? sf::Color(130, 135, 145)
            : equipped ? sf::Color(135, 245, 155)
            : sf::Color(220, 230, 240);
        const std::string state = equipped ? "Equipped" : unlocked ? "Available" : "Locked";
        const std::string marker = equipped ? "> " : "  ";
        const float columnX = i % 2 == 0 ? leftColumn : rightColumn;
        const float rowY = skillsY + 22.0f + static_cast<float>(i / 2) * 36.0f;
        drawText(marker + skillChoiceLabel(i) + ". " + skill.name
                + " Lv" + std::to_string(world.skillLevel(skill.name))
                + " [" + state + "]",
            {columnX, rowY}, 14, color);
        drawText("     " + skillSlotName(skill.slot) + " / " + skillCastTypeName(skill.castType),
            {columnX, rowY + 17.0f}, 11,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
        const auto supports = world.skillBar().supportDefinitionsFor(skill);
        std::string summary = skillEffectiveSummary(skill, world.player().stats(), supports);
        const std::string ailment = ailmentSummary(world.effectiveSkillAilment(skill));
        if (!ailment.empty()) {
            summary += "  " + ailment;
        }
        drawText("     " + truncateText(summary, 48),
            {columnX, rowY + 32.0f}, 10,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
    }

    const SkillSlot supportSlots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };
    const float supportY = center.y + 138.0f;
    drawText("Supports  E equipped A available L locked  |  F1-F4 cycle  |  F5/F6 link",
        {leftColumn, supportY}, 11, sf::Color::White);
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = supportSlots[i];
        const auto supportLabel = [&](std::size_t link) {
            const auto* support = world.skillBar().supportAt(slot, link);
            const bool selected = world.selectedSupportLink() == static_cast<int>(link)
                && link < SkillBar::supportLinkCount(slot);
            return std::string(selected ? "*" : " ") + "L"
                + std::to_string(link + 1) + ": "
                + (support ? support->name : "None");
        };
        const std::string line = skillSlotName(slot) + "  "
            + supportLabel(0)
            + (SkillBar::supportLinkCount(slot) > 1 ? "  |  " + supportLabel(1) : "");
        std::string pool = "Pool ";
        bool firstPoolEntry = true;
        for (const auto& support : SupportLibrary::all()) {
            if (!SupportLibrary::supportsSkill(support, world.skillBar().definition(slot))) {
                continue;
            }

            const bool equipped = (world.skillBar().supportAt(slot, 0) != nullptr
                    && world.skillBar().supportAt(slot, 0)->name == support.name)
                || (world.skillBar().supportAt(slot, 1) != nullptr
                    && world.skillBar().supportAt(slot, 1)->name == support.name);
            const char status = equipped ? 'E' : world.isSupportUnlocked(support.name) ? 'A' : 'L';
            if (!firstPoolEntry) {
                pool += "  ";
            }
            pool += support.name + " Lv" + std::to_string(world.supportLevel(support.name))
                + "(" + status + ")";
            firstPoolEntry = false;
        }
        const float blockX = i % 2 == 0 ? leftColumn : rightColumn;
        const float blockY = supportY + 20.0f + static_cast<float>(i / 2) * 58.0f;
        drawText(line,
            {blockX, blockY}, 11, sf::Color(150, 245, 175));
        drawText(truncateText(pool, 48), {blockX, blockY + 15.0f}, 9, sf::Color(175, 190, 205));
    }
}

void Renderer::drawMinimap(const GameWorld& world) {
    const sf::Vector2f size{150.0f, 112.0f};
    // The top HUD and equipment occupy the upper band; keep the minimap in the
    // lower-right gap above the skill bar so it cannot cover map text or loot UI.
    const sf::Vector2f origin{
        static_cast<float>(Config::WindowWidth) - size.x - 18.0f,
        static_cast<float>(Config::WindowHeight) - size.y - 58.0f
    };
    const auto& map = world.map();
    const auto& palette = map.definition().palette;
    const Vector2 mapSize = map.size();
    const auto& exploration = map.exploration();
    const bool showAll = world.state() == GameState::MapComplete;
    const float scaleX = size.x / mapSize.x;
    const float scaleY = size.y / mapSize.y;

    const auto toMinimap = [&](const Vector2& position) {
        return sf::Vector2f{
            origin.x + position.x * scaleX,
            origin.y + position.y * scaleY
        };
    };

    sf::RectangleShape background(size);
    background.setPosition(origin);
    background.setFillColor(mapColor(palette.floor, 205));
    background.setOutlineColor(sf::Color(180, 190, 200));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

    if (!showAll) {
        for (int row = 0; row < exploration.rows(); ++row) {
            for (int column = 0; column < exploration.columns(); ++column) {
                if (exploration.isCellRevealed(column, row)) {
                    continue;
                }

                sf::RectangleShape hidden({
                    exploration.cellSize() * scaleX + 0.5f,
                    exploration.cellSize() * scaleY + 0.5f
                });
                hidden.setPosition({
                    origin.x + static_cast<float>(column) * exploration.cellSize() * scaleX,
                    origin.y + static_cast<float>(row) * exploration.cellSize() * scaleY
                });
                hidden.setFillColor(sf::Color(8, 10, 14, 235));
                window_.draw(hidden);
            }
        }
    }

    auto drawMapCircle = [&](const Vector2& center, float worldRadius, sf::Color color, float outline = 1.0f) {
        const float radius = std::max(2.0f, worldRadius * std::min(scaleX, scaleY));
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        shape.setPosition(toMinimap(center));
        shape.setFillColor(sf::Color(color.r, color.g, color.b, 45));
        shape.setOutlineColor(color);
        shape.setOutlineThickness(outline);
        window_.draw(shape);
    };

    drawMapCircle(map.playerStart(), Config::StartSafeRadius, sf::Color(80, 210, 120));
    const bool bossAreaRevealed = showAll || exploration.isExplored(map.bossCenter());
    if (bossAreaRevealed) {
        drawMapCircle(map.bossCenter(), Config::BossGateRadius, sf::Color(255, 190, 90));
        drawMapCircle(map.bossCenter(), Config::BossArenaRadius, sf::Color(255, 80, 60), 1.5f);
    } else {
        drawMapCircle(map.bossCenter(), 12.0f, sf::Color(255, 190, 90));
    }

    for (const auto& obstacle : map.obstacles()) {
        if (!showAll && !exploration.isExplored(obstacle.center)) {
            continue;
        }

        sf::RectangleShape shape({obstacle.halfExtents.x * 2.0f * scaleX,
            obstacle.halfExtents.y * 2.0f * scaleY});
        shape.setFillColor(mapColor(palette.obstacle, 220));
        shape.setOutlineColor(sf::Color(135, 145, 150));
        shape.setOutlineThickness(0.75f);
        shape.setOrigin({obstacle.halfExtents.x * scaleX, obstacle.halfExtents.y * scaleY});
        shape.setPosition(toMinimap(obstacle.center));
        window_.draw(shape);
    }

    for (const auto& event : map.events()) {
        if (!showAll && !exploration.isExplored(event.position)) {
            continue;
        }

        sf::CircleShape eventDot(3.0f);
        eventDot.setOrigin({3.0f, 3.0f});
        eventDot.setPosition(toMinimap(event.position));
        eventDot.setFillColor(mapEventColor(event));
        window_.draw(eventDot);
    }

    sf::CircleShape bossDot(3.5f);
    bossDot.setOrigin({3.5f, 3.5f});
    bossDot.setPosition(toMinimap(map.bossCenter()));
    bossDot.setFillColor(!bossAreaRevealed
        ? sf::Color(255, 190, 90)
        : map.bossDefeated() ? sf::Color(120, 120, 120) : sf::Color(255, 80, 60));
    window_.draw(bossDot);

    sf::CircleShape playerDot(3.0f);
    playerDot.setOrigin({3.0f, 3.0f});
    playerDot.setPosition(toMinimap(world.player().position()));
    playerDot.setFillColor(sf::Color(90, 180, 255));
    window_.draw(playerDot);

    const int exploredPercent = exploration.totalCellCount() > 0
        ? static_cast<int>(100.0f * static_cast<float>(exploration.exploredCellCount())
            / static_cast<float>(exploration.totalCellCount()))
        : 0;
    drawText("L" + std::to_string(map.layoutIndex() + 1) + "/"
            + std::to_string(MapLayoutLibrary::VariantCount)
            + "  EXP " + std::to_string(exploredPercent) + "%",
        {origin.x, origin.y + size.y + 5.0f}, 11, sf::Color(210, 220, 230));
}

void Renderer::drawBossHealth(const GameWorld& world) {
    const Enemy* boss = nullptr;
    for (const auto& enemy : world.enemies()) {
        if (enemy.isBoss() && !enemy.isDead()) {
            boss = &enemy;
            break;
        }
    }

    if (!boss || boss->maxHp() <= 0) {
        return;
    }

    // Keep the boss panel out of the left event stack and the right inventory
    // column in the 800px prototype window.
    const sf::Vector2f position{270.0f, 258.0f};
    const sf::Vector2f size{220.0f, 12.0f};
    const float ratio = std::clamp(
        static_cast<float>(std::max(0, boss->hp())) / static_cast<float>(boss->maxHp()),
        0.0f,
        1.0f
    );

    const int bossPhase = world.bossPhase();
    const std::string phaseLabel = bossPhase >= 2
        ? "  FINAL"
        : bossPhase == 1 ? "  ENRAGED" : "";
    const std::string bossLabel = world.bossDefinition().name
        + phaseLabel + "  "
        + std::to_string(std::max(0, boss->hp())) + "/" + std::to_string(boss->maxHp());
    drawText(truncateText(bossLabel, 34),
        {position.x, position.y - 18.0f}, 13,
        bossPhase >= 2 ? sf::Color(255, 95, 65)
            : bossPhase == 1 ? sf::Color(255, 150, 80)
            : sf::Color(255, 210, 160));

    float detailY = position.y + 18.0f;
    drawText(truncateText(world.bossPhaseSummary(), 34), {position.x, detailY}, 12,
        bossPhase >= 2 ? sf::Color(255, 125, 95)
            : bossPhase == 1 ? sf::Color(255, 175, 95)
            : sf::Color(230, 210, 175));
    detailY += 17.0f;

    drawText("Fire Res " + std::to_string(world.bossDefinition().fireResistance)
        + "%  Cold Res " + std::to_string(world.bossDefinition().coldResistance)
        + "%  Light Res " + std::to_string(world.bossDefinition().lightningResistance)
        + "%  Poison Res " + std::to_string(world.bossDefinition().poisonResistance)
        + "%  Shock Res " + std::to_string(world.bossDefinition().shockResistance) + "%",
        {position.x, detailY}, 11, sf::Color(220, 195, 175));
    detailY += 16.0f;

    const std::string castWarning = world.bossSkillWarning();
    if (!castWarning.empty()) {
        drawText(truncateText(castWarning, 34), {position.x, detailY}, 13, sf::Color(255, 130, 90));
    }

    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color(60, 30, 28, 210));
    background.setOutlineColor(sf::Color(255, 210, 160));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

    sf::RectangleShape fill({size.x * ratio, size.y});
    fill.setPosition(position);
    fill.setFillColor(bossPhase >= 2
        ? sf::Color(255, 75, 35)
        : bossPhase == 1 ? sf::Color(255, 125, 35)
        : sf::Color(220, 55, 45));
    window_.draw(fill);
}

void Renderer::drawGameOver(const GameWorld& /*world*/) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window_.draw(overlay);

    drawBox({center.x, center.y - 50.0f}, {300.0f, 100.0f}, sf::Color::Red);
    drawCenteredText("GAME OVER", {center.x, center.y - 58.0f}, 28, sf::Color::White);
    drawBox({center.x, center.y + 50.0f}, {200.0f, 40.0f}, sf::Color::White);
    drawCenteredText("Press R", {center.x, center.y + 43.0f}, 20, sf::Color::Black);
}

void Renderer::drawMapComplete(const GameWorld& world) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const float centerColumnX = 430.0f;
    const float leftColumnX = 16.0f;

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 100, 0, 180));
    window_.draw(overlay);

    const std::string phaseText = !world.mapRewardChosen()
        ? "PHASE: CHOOSE REWARD"
        : world.mapDeviceOpen()
            ? (world.selectedMapItemIndex() >= 0
                ? "PHASE: PRESS E TO ENTER STORED MAP"
                : "PHASE: CHOOSE STORED MAP")
        : !world.nextMapOptionChosen()
            ? "PHASE: CHOOSE NEXT MAP"
            : "PHASE: PRESS E TO ENTER MAP " + std::to_string(world.mapLevel() + 1);
    drawCenteredText(truncateText(phaseText, 28), {centerColumnX, 24.0f}, 14,
        sf::Color(255, 240, 180));

    drawBox({centerColumnX, 105.0f}, {190.0f, 160.0f}, sf::Color::Green);
    drawText(truncateText("BOSS: " + world.bossDefinition().name, 27),
        {centerColumnX - 90.0f, 47.0f}, 14, sf::Color::White);
    drawText("Kills " + std::to_string(world.mapKills())
        + "  XP " + std::to_string(world.mapExperienceGained()),
        {centerColumnX - 90.0f, 68.0f}, 12, sf::Color::White);
    drawText("Drops " + std::to_string(world.mapItemsDropped())
        + "  Boss " + std::to_string(world.mapBossItemsDropped())
        + "  F " + std::to_string(world.mapItemsPickedUp()),
        {centerColumnX - 90.0f, 84.0f}, 11, sf::Color::White);
    drawText("Quality N/M/R/U "
            + std::to_string(world.mapDroppedItemsByRarity(Rarity::Normal)) + "/"
            + std::to_string(world.mapDroppedItemsByRarity(Rarity::Magic)) + "/"
            + std::to_string(world.mapDroppedItemsByRarity(Rarity::Rare)) + "/"
            + std::to_string(world.mapDroppedItemsByRarity(Rarity::Unique)),
        {centerColumnX - 90.0f, 100.0f}, 10, sf::Color(255, 225, 145));
    drawText("Rare " + std::to_string(world.mapRareLeadersDefeated())
        + "  Leader drops " + std::to_string(world.mapRareLeaderItemsDropped()),
        {centerColumnX - 90.0f, 116.0f}, 10, sf::Color(255, 220, 160));
    const auto relicPreview = world.bossRelicPreview();
    const auto* relicBase = relicPreview
        ? ItemBaseLibrary::find(relicPreview->baseId) : nullptr;
    const auto& relicEffect = relicBase != nullptr
        ? BossRelicEffectLibrary::forBase(*relicBase)
        : BossRelicEffectLibrary::forTheme(
            bossRelicTheme(world.bossDefinition().lootTheme));
    drawText(truncateText("Effect: " + relicEffect.name, 27),
        {centerColumnX - 90.0f, 132.0f}, 10, sf::Color(255, 180, 80));
    drawText(truncateText("Relic: " + world.bossDefinition().lootRewardDescription, 27),
        {centerColumnX - 90.0f, 147.0f}, 9, sf::Color(255, 225, 145));
    if (!world.lastRareLeaderName().empty()) {
        drawText(truncateText("Rare: " + world.lastRareLeaderName()
                + " | " + world.lastRareLeaderRewardDescription(), 27),
            {centerColumnX - 90.0f, 161.0f}, 9, sf::Color(255, 205, 140));
    }

    // Left-side pickup guidance (always available during MapComplete):
    // looting runs concurrently with reward / next-map selection, so the player
    // knows F still works and what it will target.
    float pickupY = 320.0f;
    drawText("F Pick up nearby drops", {leftColumnX, pickupY}, 13, sf::Color(180, 220, 255));
    pickupY += 20.0f;
    const int focusedLoot = world.focusedDroppedItemIndex();
    if (focusedLoot >= 0 && static_cast<std::size_t>(focusedLoot) < world.droppedItems().size()) {
        drawText(truncateText("Focused: "
                + world.droppedItems()[static_cast<std::size_t>(focusedLoot)].item().name, 38),
            {leftColumnX, pickupY}, 13, sf::Color(255, 220, 120));
        pickupY += 20.0f;
    }
    if (world.inventory().isFull()) {
        const std::string fullPrompt = world.stashSelectionActive()
            ? "Inventory full - O Take selected Stash item"
            : "Inventory full - Tab select / Del drop an item";
        drawText(truncateText(fullPrompt, 38), {leftColumnX, pickupY}, 13, sf::Color(255, 90, 90));
    }

    const auto& mapOptions = world.nextMapOptions();
    if (!world.mapRewardChosen()) {
        drawText("Choose Reward", {centerColumnX - 90.0f, 192.0f}, 16, sf::Color::White);
        const auto& rewards = world.mapRewardOptions();
        float optionY = 218.0f;
        for (std::size_t i = 0; i < rewards.size(); ++i) {
            const auto& reward = rewards[i];
            drawText(truncateText(std::to_string(i + 1) + ". " + reward.title, 27),
                {centerColumnX - 90.0f, optionY}, 13, sf::Color(220, 245, 255));
            drawText(truncateText("  " + rewardDetailSummary(reward, world), 27),
                {centerColumnX - 90.0f, optionY + 17.0f}, 10, sf::Color(230, 220, 170));
            drawText(truncateText("  " + rewardStatPreview(reward, world), 27),
                {centerColumnX - 90.0f, optionY + 32.0f}, 10, sf::Color(200, 220, 245));
            optionY += 58.0f;
        }
    } else {
        const auto selectedReward = static_cast<std::size_t>(world.selectedMapRewardOption());
        const auto& reward = world.mapRewardOptions()[selectedReward];
        drawText(truncateText("Reward: " + reward.title, 27),
            {centerColumnX - 90.0f, 192.0f}, 13, sf::Color(150, 255, 175));
        drawText("Choose Next Map", {centerColumnX - 90.0f, 212.0f}, 16, sf::Color::White);
        float optionY = 238.0f;
        for (std::size_t i = 0; i < mapOptions.size(); ++i) {
            const bool selected = world.selectedNextMapOption() == static_cast<int>(i);
            const auto& option = mapOptions[i];
            const MapModifier effectiveModifier = MapItemLibrary::modifierFor(option);
            const sf::Color color = selected ? sf::Color(140, 255, 160) : sf::Color(220, 240, 255);
            const std::string marker = selected ? "> " : "  ";
            drawText(truncateText(marker + std::to_string(i + 1) + ". "
                    + effectiveModifier.name + " " + option.recommendedLevel, 27),
                {centerColumnX - 90.0f, optionY}, 12, color);
            drawText(truncateText("  " +
                        MapTemplateLibrary::forIndex(option.templateIndex).name
                            + " | " + effectiveModifier.description, 27),
                {centerColumnX - 90.0f, optionY + 16.0f}, 10, sf::Color(230, 220, 170));
            drawText(truncateText("  " + mapOptionSummary(option), 27),
                {centerColumnX - 90.0f, optionY + 31.0f}, 10, sf::Color(200, 220, 245));
            optionY += 48.0f;
        }
    }

    drawText("M Map Device  Maps " + std::to_string(world.mapItems().size())
            + "/" + std::to_string(world.mapItemCapacity())
            + "  Atlas " + std::to_string(world.completedMapCount()),
        {leftColumnX, 466.0f}, 11, sf::Color(180, 220, 255));

    drawBox({centerColumnX, 410.0f}, {190.0f, 36.0f}, sf::Color::White);
    std::string footer;
    if (!world.mapRewardChosen()) {
        footer = "Pick 1 / 2 / 3 reward";
    } else if (world.mapDeviceOpen()) {
        footer = world.selectedMapItemIndex() >= 0
            ? "E Enter stored map"
            : "Tab / 1-0 select map";
    } else if (world.nextMapOptionChosen()) {
        const auto& selected = mapOptions[static_cast<std::size_t>(world.selectedNextMapOption())];
        footer = "E Enter " + selected.modifier.name;
    } else {
        footer = "Pick 1 / 2 / 3 next map";
    }
    drawCenteredText(truncateText(footer, 25), {centerColumnX, 403.0f}, 12, sf::Color::Black);
}

void Renderer::drawMapCompleteBossRelicPreview(const GameWorld& world) {
    const auto relic = world.bossRelicPreview();
    if (!relic) {
        return;
    }

    const bool stillOnGround = std::any_of(
        world.droppedItems().begin(),
        world.droppedItems().end(),
        [&relic](const DroppedItem& dropped) {
            return dropped.item().rarity == Rarity::Unique
                && dropped.item().baseId == relic->baseId;
        }
    );
    const std::string actionHint = stillOnGround
        ? "F Pick up Unique relic"
        : "Unique relic already collected";
    drawItemDetailPanel(
        world,
        {16.0f, 350.0f},
        *relic,
        world.player().equipment().itemInSlot(relic->slot),
        "Boss Reward",
        actionHint,
        true
    );
}

void Renderer::drawMapCompleteInventoryPanel(const GameWorld& world) {
    const float width = static_cast<float>(Config::WindowWidth);
    const auto& items = world.inventory().items();
    const auto& equipment = world.player().equipment();

    // Right-side panel, same column as the Playing inventory, drawn AFTER the
    // MapComplete overlay so it stays bright. Tab/Del manage the bag here; 1-9 is
    // deliberately NOT shown because number keys belong to reward/next-map choice.
    const float x = width - 260.0f;
    const float panelW = 270.0f;
    const float panelTop = 150.0f;
    const float panelH = 300.0f;
    drawBox({x - 10.0f + panelW / 2.0f, panelTop + panelH / 2.0f}, {panelW, panelH}, sf::Color(18, 22, 30));

    const bool inventoryFull = world.inventory().isFull();
    const std::string inventoryTitle = "Inventory "
        + std::to_string(world.inventory().size()) + "/"
        + std::to_string(world.inventory().capacity())
        + "  Forge " + std::to_string(world.forgeFragments());
    float y = panelTop + 8.0f;
    drawText(inventoryTitle, {x, y}, 16, inventoryFull ? sf::Color(255, 90, 90) : sf::Color::White);
    y += 20.0f;
    if (inventoryFull) {
        drawText("Inventory full - O Take / Del Drop", {x, y}, 12, sf::Color(255, 90, 90));
        y += 16.0f;
        drawText("Tab Cycle  I Store", {x, y}, 12, sf::Color(165, 175, 190));
        y += 16.0f;
    } else {
        drawText("Tab Cycle  I Store  O Take  C Salvage", {x, y}, 12, sf::Color(150, 160, 175));
        y += 16.0f;
    }

    const int selectedIndex = world.selectedInventoryIndex();
    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const auto& item = items[i];
        const bool isSelected = !world.stashSelectionActive()
            && static_cast<int>(i) == selectedIndex;
        const std::string line = (isSelected ? "> " : "") + std::to_string(i + 1) + ". "
            + item.name + " [" + slotName(item.slot) + "] "
            + itemRequirementSummary(item) + " " + statsSummary(item.stats);
        sf::Color rowColor = rarityColor(item.rarity);
        if (!itemRequirementMet(world, item)) {
            rowColor = sf::Color(255, 90, 90);
        } else if (isSelected) {
            rowColor = sf::Color(255, 215, 90);
        }
        drawText(truncateText(line, 36), {x, y}, 13, rowColor);
        y += 16.0f;

        const auto& current = equipment.itemInSlot(item.slot);
        if (current) {
            const Stats delta = statsDelta(item.stats, current->stats);
            drawText("   Delta: " + statsDeltaSummary(delta), {x, y}, 11, deltaColor(delta));
            y += 14.0f;
        }
    }
}

void Renderer::drawMapCompleteStashPanel(const GameWorld& world) {
    const float x = 330.0f;
    const float y = static_cast<float>(Config::WindowHeight) - 166.0f;
    const float panelW = 154.0f;
    const float panelH = 166.0f;
    drawBox({x + panelW / 2.0f, y + panelH / 2.0f}, {panelW, panelH}, sf::Color(18, 22, 30));

    const auto& items = world.stash().items();
    const std::string title = "Stash " + std::to_string(world.stash().size())
        + "/" + std::to_string(world.stash().capacity());
    drawText(title, {x + 8.0f, y + 8.0f}, 16, world.stash().isFull()
        ? sf::Color(255, 90, 90)
        : sf::Color::White);
    drawText("Tab Cycle  I Store  O Take", {x + 8.0f, y + 28.0f}, 11, sf::Color(165, 175, 190));

    const int selectedIndex = world.selectedStashIndex();
    const auto compactName = [](const std::string& name) {
        return name.size() > 17 ? name.substr(0, 14) + "..." : name;
    };
    if (world.stashSelectionActive() && selectedIndex >= 0
        && static_cast<std::size_t>(selectedIndex) < items.size()) {
        drawText("Selected: " + compactName(items[static_cast<std::size_t>(selectedIndex)].name),
            {x + 8.0f, y + 48.0f}, 11, sf::Color(255, 215, 90));
    } else {
        drawText("Selected: Inventory", {x + 8.0f, y + 48.0f}, 11, sf::Color(180, 195, 210));
    }

    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 5);
    float rowY = y + 68.0f;
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const bool selected = world.stashSelectionActive()
            && static_cast<int>(i) == selectedIndex;
        const std::string marker = selected ? "> " : "  ";
        drawText(marker + std::to_string(i + 1) + ". " + compactName(items[i].name),
            {x + 8.0f, rowY}, 11, selected ? sf::Color(255, 215, 90) : rarityColor(items[i].rarity));
        rowY += 16.0f;
    }
    if (items.size() > visibleCount) {
        drawText("... Tab cycles remaining", {x + 8.0f, rowY}, 10, sf::Color(150, 160, 175));
    }
}

void Renderer::drawMapDevicePanel(const GameWorld& world) {
    if (!world.mapDeviceOpen()) {
        return;
    }

    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    drawBox({width / 2.0f, height / 2.0f}, {730.0f, 520.0f}, sf::Color(12, 18, 28));
    drawCenteredText("MAP DEVICE", {width / 2.0f, 48.0f}, 24, sf::Color(255, 235, 170));
    drawCenteredText(
        "Stored maps " + std::to_string(world.mapItems().size()) + "/"
            + std::to_string(world.mapItemCapacity())
            + "   Completed atlas maps " + std::to_string(world.completedMapCount()),
        {width / 2.0f, 78.0f}, 13, sf::Color(185, 220, 245)
    );
    drawCenteredText("1-0 Select  Tab Cycle  E Enter  M / Esc Close",
        {width / 2.0f, 100.0f}, 12, sf::Color(160, 170, 185));

    if (world.mapItems().empty()) {
        drawCenteredText("No stored maps. Close with M and choose a new map.",
            {width / 2.0f, 270.0f}, 14, sf::Color(220, 220, 220));
        return;
    }

    const auto& maps = world.mapItems();
    const int selectedIndex = world.selectedMapItemIndex();
    const std::size_t visibleCount = std::min<std::size_t>(maps.size(), 12);
    float rowY = 126.0f;
    for (std::size_t index = 0; index < visibleCount; ++index) {
        const MapModifier effectiveModifier = MapItemLibrary::modifierFor(maps[index].option);
        const bool selected = static_cast<int>(index) == selectedIndex;
        const sf::Color color = selected
            ? sf::Color(255, 215, 90)
            : sf::Color(220, 235, 250);
        const std::string marker = selected ? "> " : "  ";
        drawText(truncateText(marker + std::to_string(index + 1) + ". "
                + MapItemLibrary::displayName(maps[index])
                + " Q" + std::to_string(maps[index].option.quality)
                + " | " + effectiveModifier.name, 82),
            {72.0f, rowY}, 13, color);
        drawText(truncateText("    " + effectiveModifier.description
                + " | " + effectiveModifier.rewardDescription, 95),
            {72.0f, rowY + 16.0f}, 10, sf::Color(190, 205, 220));
        rowY += 34.0f;
    }
}

void Renderer::drawMapCompleteLootDetail(const GameWorld& world) {
    if (world.mapDeviceOpen()) {
        return;
    }

    const int index = world.focusedDroppedItemIndex();
    if (index < 0) {
        return;
    }
    const auto& droppedItem = world.droppedItems()[static_cast<std::size_t>(index)];
    const Item& item = droppedItem.item();
    const std::string actionHint = world.inventory().isFull()
        ? (world.stashSelectionActive()
            ? "Inventory full - O Take selected Stash item"
            : "Inventory full - Tab select / Del drop an item")
        : "F Pick up";
    // Top-left placement keeps the detail visible in the 800x600 window while
    // leaving the centered reward/map choice area and right-side inventory readable.
    const sf::Vector2f lootDetailPos{16.0f, 64.0f};
    drawItemDetailPanel(world, lootDetailPos, item, world.player().equipment().itemInSlot(item.slot),
        "Boss Drop", actionHint, true);
}

void Renderer::drawBox(const sf::Vector2f& center, const sf::Vector2f& size, const sf::Color& color) {
    sf::RectangleShape box(size);
    box.setFillColor(color);
    box.setOrigin({size.x / 2.0f, size.y / 2.0f});
    box.setPosition(center);
    window_.draw(box);
}

void Renderer::drawText(const std::string& text, const sf::Vector2f& position, unsigned int size, const sf::Color& color) {
    if (!fontLoaded_) {
        return;
    }

    sf::Text drawable(font_, text, size);
    drawable.setFillColor(color);
    drawable.setPosition(position);
    window_.draw(drawable);
}

void Renderer::drawCenteredText(const std::string& text, const sf::Vector2f& center, unsigned int size, const sf::Color& color) {
    if (!fontLoaded_) {
        return;
    }

    sf::Text drawable(font_, text, size);
    drawable.setFillColor(color);
    const auto bounds = drawable.getLocalBounds();
    drawable.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
    drawable.setPosition(center);
    window_.draw(drawable);
}

sf::Vector2f Renderer::worldToScreen(const GameWorld& world, const Vector2& position) const {
    const Vector2 camera = world.cameraTopLeft();
    return {position.x - camera.x, position.y - camera.y};
}
