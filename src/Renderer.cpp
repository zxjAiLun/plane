#include "Renderer.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"
#include "Equipment.hpp"
#include "Stats.hpp"
#include "SkillBar.hpp"

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
    }
    return sf::Color::White;
}

sf::Color enemyColor(const EnemyColor& color) {
    return sf::Color(color.r, color.g, color.b);
}

int multiplierPercent(float multiplier) {
    return static_cast<int>((multiplier - 1.0f) * 100.0f + 0.5f);
}

std::string statsSummary(const Stats& stats) {
    std::string summary;
    if (stats.maxHp > 0) {
        summary += "+" + std::to_string(stats.maxHp) + " HP ";
    }
    if (stats.damageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.damageMultiplier)) + "% DMG ";
    }
    if (stats.attackSpeedMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier)) + "% AS ";
    }
    if (stats.moveSpeedMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier)) + "% MS ";
    }
    if (stats.pickupRangeMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.pickupRangeMultiplier)) + "% PICKUP ";
    }
    if (stats.projectileDamageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.projectileDamageMultiplier)) + "% PDMG ";
    }
    if (stats.areaDamageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.areaDamageMultiplier)) + "% ADMG ";
    }
    if (stats.areaRadiusMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.areaRadiusMultiplier)) + "% AREA ";
    }
    return summary;
}

std::string itemSummary(const Item& item) {
    return item.name + " " + statsSummary(item.stats);
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
        || delta.areaRadiusMultiplier > 1.0f;
    const bool negative = delta.maxHp < 0
        || delta.damageMultiplier < 1.0f
        || delta.attackSpeedMultiplier < 1.0f
        || delta.moveSpeedMultiplier < 1.0f
        || delta.pickupRangeMultiplier < 1.0f
        || delta.projectileDamageMultiplier < 1.0f
        || delta.areaDamageMultiplier < 1.0f
        || delta.areaRadiusMultiplier < 1.0f;

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
    }
    return sf::Color::White;
}

std::string passiveBranchName(PassiveBranch branch) {
    switch (branch) {
        case PassiveBranch::Projectile: return "Projectile";
        case PassiveBranch::Area: return "Area";
        case PassiveBranch::Survival: return "Survival";
        case PassiveBranch::Loot: return "Loot";
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

int effectiveSkillDamage(const SkillDefinition& skill, const Stats& stats) {
    if (skill.baseDamage <= 0) {
        return 0;
    }

    float damage = static_cast<float>(skill.baseDamage) * stats.damageMultiplier;
    switch (skill.castType) {
        case SkillCastType::Projectile:
            damage *= stats.projectileDamageMultiplier;
            break;
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            damage *= stats.areaDamageMultiplier;
            break;
        case SkillCastType::Dash:
            break;
    }

    return std::max(1, static_cast<int>(std::ceil(damage)));
}

float effectiveSkillRadius(const SkillDefinition& skill, const Stats& stats) {
    switch (skill.castType) {
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            return skill.radius * stats.areaRadiusMultiplier;
        case SkillCastType::Projectile:
        case SkillCastType::Dash:
            return skill.radius;
    }

    return skill.radius;
}

float effectiveSkillCooldown(const SkillDefinition& skill, const Stats& stats) {
    if (skill.slot == SkillSlot::Primary) {
        return skill.cooldown / stats.attackSpeedMultiplier;
    }

    return skill.cooldown;
}

std::string skillEffectiveSummary(const SkillDefinition& skill, const Stats& stats) {
    return "Base " + std::to_string(skill.baseDamage)
        + "/" + std::to_string(static_cast<int>(skill.radius))
        + "/" + formatFloat(skill.cooldown, 2)
        + "  Actual " + std::to_string(effectiveSkillDamage(skill, stats))
        + "/" + std::to_string(static_cast<int>(effectiveSkillRadius(skill, stats)))
        + "/" + formatFloat(effectiveSkillCooldown(skill, stats), 2);
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
        std::string line = std::string(label) + ": DMG "
            + std::to_string(effectiveSkillDamage(skill, before)) + " -> "
            + std::to_string(effectiveSkillDamage(skill, after));
        if (showRadius) {
            line += "  R "
                + std::to_string(static_cast<int>(effectiveSkillRadius(skill, before))) + " -> "
                + std::to_string(static_cast<int>(effectiveSkillRadius(skill, after)));
        } else {
            line += "  CD "
                + formatFloat(effectiveSkillCooldown(skill, before), 2) + " -> "
                + formatFloat(effectiveSkillCooldown(skill, after), 2);
        }
        return line;
    };
    lines.push_back(detail("Primary", skillBar.definition(SkillSlot::Primary), false));
    lines.push_back(detail("Secondary", skillBar.definition(SkillSlot::Secondary), true));
    lines.push_back(detail("Utility", skillBar.definition(SkillSlot::Utility), true));
    return lines;
}

std::string rewardDetailSummary(const MapRewardDefinition& reward, const GameWorld& world) {
    if (reward.type != MapRewardType::UnlockSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    const auto& current = world.skillBar().definition(skill->slot);
    return skillSlotName(skill->slot) + " / " + skillCastTypeName(skill->castType)
        + "  |  Replaces " + current.name;
}

std::string rewardStatPreview(const MapRewardDefinition& reward, const GameWorld& world) {
    if (reward.type != MapRewardType::UnlockSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    return skillEffectiveSummary(*skill, world.player().stats());
}

std::string mapOptionSummary(const MapOption& option) {
    const auto& modifier = option.modifier;
    return "HP +" + std::to_string(multiplierPercent(modifier.monsterHpMultiplier))
        + "%  DMG +" + std::to_string(modifier.monsterDamageBonus)
        + "  IQ +" + std::to_string(multiplierPercent(modifier.itemQuantityMultiplier))
        + "%  Boss +" + std::to_string(modifier.bossDropBonus);
}

sf::Color mapEventColor(MapEventType type, bool completed) {
    if (completed) {
        return sf::Color(130, 130, 130);
    }

    switch (type) {
        case MapEventType::LootCache: return sf::Color(255, 215, 70);
        case MapEventType::ElitePack: return sf::Color(190, 90, 255);
        case MapEventType::Shrine: return sf::Color(80, 230, 230);
    }

    return sf::Color::White;
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
    drawNovaEffect(world);
    drawSecondarySkillEffect(world);
    drawBossAoeEffect(world);
    drawPlayer(world);
    drawAimIndicator(world);
    drawProjectiles(world);
    drawBossProjectiles(world);
    drawEnemyProjectiles(world);
    drawEnemies(world);
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
        + "  SP " + std::to_string(world.player().talentPoints()),
        {16.0f, 36.0f}, 18, sf::Color::White);
    drawText("TIME " + std::to_string(static_cast<int>(world.survivalTime()))
        + "  SCORE " + std::to_string(world.score()),
        {16.0f, 60.0f}, 18, sf::Color::White);
    drawText("MAP " + std::to_string(world.mapLevel()) + "  " + world.currentMapOption().modifier.name
        + "  AREA " + mapAreaName(world.currentMapArea())
        + "  ENEMIES " + std::to_string(world.enemiesRemainingInWave()),
        {16.0f, 108.0f}, 16, sf::Color(210, 220, 255));
    drawText(world.mapModifier().description + "  |  " + mapOptionSummary(world.currentMapOption()),
        {16.0f, 130.0f}, 14, sf::Color(255, 220, 150));
    const std::string bossLine = world.map().bossDefeated()
        ? "Boss defeated: " + world.bossDefinition().name
        : world.map().bossTriggered()
            ? "Boss active: " + world.bossDefinition().name
            : world.bossDefinition().name + " distance " + std::to_string(static_cast<int>(world.distanceToBoss()));
    drawText(bossLine,
        {16.0f, 174.0f}, 14, sf::Color(255, 190, 150));
    drawText("Objective: " + world.mapObjective(),
        {16.0f, 196.0f}, 14, sf::Color(210, 255, 210));
    float hudY = 218.0f;
    // Persistent event progress (always visible while exploring the map).
    drawText("Events " + std::to_string(world.mapEventsCompleted())
        + "/" + std::to_string(world.mapEventsTotal()),
        {16.0f, hudY}, 14, sf::Color(210, 255, 210));
    hudY += 18.0f;
    if (!world.nearbyEventPrompt().empty()) {
        drawText(world.nearbyEventPrompt(), {16.0f, hudY}, 14, sf::Color(255, 235, 150));
        hudY += 18.0f;
    }
    if (!world.eventStatusMessage().empty() && world.eventStatusTimeRemaining() > 0.0f) {
        drawText(world.eventStatusMessage(), {16.0f, hudY}, 14, sf::Color(255, 220, 120));
        hudY += 18.0f;
    }
    if (world.activeEliteEventEnemiesRemaining() > 0) {
        drawText("Elite pack: " + std::to_string(world.activeEliteEventEnemiesRemaining())
            + " enemies left", {16.0f, hudY}, 14, sf::Color(200, 140, 255));
        hudY += 18.0f;
    }
    const std::string pickupPrompt = world.pickupPrompt();
    if (!pickupPrompt.empty()) {
        const bool full = pickupPrompt.rfind("Inventory full", 0) == 0;
        drawText(pickupPrompt, {16.0f, hudY}, 14,
            full ? sf::Color(255, 90, 90) : sf::Color(180, 220, 255));
        hudY += 18.0f;
    } else if (world.inventoryFullPromptTimeRemaining() > 0.0f) {
        drawText("Inventory full", {16.0f, hudY}, 14, sf::Color(255, 90, 90));
        hudY += 18.0f;
    }
    if (world.shrineBuffTimeRemaining() > 0.0f) {
        drawText("Shrine Damage +35%  "
            + std::to_string(static_cast<int>(world.shrineBuffTimeRemaining() + 0.99f)) + "s",
            {16.0f, hudY}, 14, sf::Color(100, 240, 240));
        hudY += 18.0f;
    }
    drawText("Build: " + world.passiveBuildSummary() + "  |  P Passive Tree  |  K Skills",
        {16.0f, 152.0f}, 14, sf::Color(210, 255, 210));
    const auto& stats = world.player().stats();
    drawText("DMG +" + std::to_string(multiplierPercent(stats.damageMultiplier))
        + "%  AS +" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier))
        + "%  MS +" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier))
        + "%  PDMG +" + std::to_string(multiplierPercent(stats.projectileDamageMultiplier))
        + "%  ADMG +" + std::to_string(multiplierPercent(stats.areaDamageMultiplier))
        + "%  AREA +" + std::to_string(multiplierPercent(stats.areaRadiusMultiplier)) + "%",
        {16.0f, 84.0f}, 14, sf::Color(210, 220, 255));
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
            drawMapCompleteInventoryPanel(world);
            drawMapCompleteLootDetail(world);
            break;
        case GameState::Playing:
            break;
    }

    window_.display();
}

void Renderer::drawMap(const GameWorld& world) {
    const Vector2 camera = world.cameraTopLeft();
    const auto& map = world.map();

    sf::RectangleShape floor({map.size().x, map.size().y});
    floor.setFillColor(sf::Color(24, 28, 30));
    floor.setOutlineColor(sf::Color(80, 90, 96));
    floor.setOutlineThickness(6.0f);
    floor.setPosition({-camera.x, -camera.y});
    window_.draw(floor);

    const sf::Vector2f bossCenter = worldToScreen(world, map.bossCenter());
    sf::CircleShape gate(Config::BossGateRadius);
    gate.setFillColor(sf::Color(120, 70, 40, 35));
    gate.setOutlineColor(sf::Color(210, 150, 90, 130));
    gate.setOutlineThickness(3.0f);
    gate.setOrigin({Config::BossGateRadius, Config::BossGateRadius});
    gate.setPosition(bossCenter);
    window_.draw(gate);

    sf::CircleShape arena(Config::BossArenaRadius);
    arena.setFillColor(sf::Color(120, 35, 35, 45));
    arena.setOutlineColor(sf::Color(240, 90, 80, 160));
    arena.setOutlineThickness(4.0f);
    arena.setOrigin({Config::BossArenaRadius, Config::BossArenaRadius});
    arena.setPosition(bossCenter);
    window_.draw(arena);

    const sf::Vector2f startCenter = worldToScreen(world, map.playerStart());
    sf::CircleShape start(Config::StartSafeRadius);
    start.setFillColor(sf::Color(40, 110, 70, 45));
    start.setOutlineColor(sf::Color(90, 210, 130, 120));
    start.setOutlineThickness(3.0f);
    start.setOrigin({Config::StartSafeRadius, Config::StartSafeRadius});
    start.setPosition(startCenter);
    window_.draw(start);
}

void Renderer::drawPlayer(const GameWorld& world) {
    const auto& player = world.player();
    sf::CircleShape shape(player.radius());
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition(worldToScreen(world, player.position()));
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

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(80, 180, 255, alpha / 4));
    shape.setOutlineColor(sf::Color(120, 220, 255, alpha));
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
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({projectile.radius(), projectile.radius()});
        shape.setPosition(worldToScreen(world, projectile.position()));
        window_.draw(shape);
    }
}

void Renderer::drawBossProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.bossProjectiles()) {
        sf::CircleShape shape(projectile.radius);
        shape.setFillColor(sf::Color(255, 90, 45));
        shape.setOutlineColor(sf::Color(255, 210, 120));
        shape.setOutlineThickness(2.0f);
        shape.setOrigin({projectile.radius, projectile.radius});
        shape.setPosition(worldToScreen(world, projectile.position));
        window_.draw(shape);
    }
}

void Renderer::drawEnemyProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.enemyProjectiles()) {
        sf::CircleShape shape(projectile.radius);
        shape.setFillColor(sf::Color(80, 235, 145));
        shape.setOutlineColor(sf::Color(210, 255, 190));
        shape.setOutlineThickness(1.5f);
        shape.setOrigin({projectile.radius, projectile.radius});
        shape.setPosition(worldToScreen(world, projectile.position));
        window_.draw(shape);
    }
}

void Renderer::drawEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies()) {
        const auto& definition = EnemyLibrary::forType(enemy.type());
        const sf::Vector2f screenPosition = worldToScreen(world, enemy.position());
        if (enemy.isAttackWindingUp()) {
            sf::CircleShape warning(enemy.attackRange());
            const sf::Color warningColor = enemy.isRanged()
                ? sf::Color(255, 220, 75, 210)
                : sf::Color(255, 110, 75, 210);
            warning.setFillColor(enemy.isRanged()
                ? sf::Color(255, 220, 75, 28)
                : sf::Color(255, 50, 40, 28));
            warning.setOutlineColor(warningColor);
            warning.setOutlineThickness(2.0f);
            warning.setOrigin({enemy.attackRange(), enemy.attackRange()});
            warning.setPosition(screenPosition);
            window_.draw(warning);
        }

        sf::RectangleShape shape({enemy.radius() * 2, enemy.radius() * 2});
        shape.setFillColor(enemyColor(definition.fillColor));
        if (definition.outlineThickness > 0.0f) {
            shape.setOutlineColor(enemyColor(definition.outlineColor));
            shape.setOutlineThickness(definition.outlineThickness);
        }
        shape.setOrigin({enemy.radius(), enemy.radius()});
        shape.setPosition(screenPosition);
        window_.draw(shape);

        if (definition.outlineThickness > 0.0f) {
            const std::string label = enemy.isBoss() ? world.bossDefinition().name : definition.name;
            drawCenteredText(label, {screenPosition.x, screenPosition.y - enemy.radius() - 18.0f},
                11, enemyColor(definition.outlineColor));
        }
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
            + std::to_string(static_cast<int>(progress * 100.0f)) + "%",
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
        drawText(line, {x, y}, 12, item ? rarityColor(item->rarity) : sf::Color(150, 150, 150));
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
        + std::to_string(world.inventory().capacity());
    float y = 118.0f;
    drawText(inventoryTitle, {x, y}, 16, inventoryFull ? sf::Color(255, 90, 90) : sf::Color::White);
    y += 22.0f;
    if (inventoryFull) {
        drawText("Inventory full - equip or drop an item", {x, y}, 12, sf::Color(255, 90, 90));
        y += 16.0f;
    } else {
        drawText("Tab Select  Del Drop", {x, y}, 12, sf::Color(150, 160, 175));
        y += 16.0f;
    }

    const int selectedIndex = world.selectedInventoryIndex();
    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const auto& item = items[i];
        const bool isSelected = (static_cast<int>(i) == selectedIndex);
        const bool isHovered = (i == hovered);
        const std::string line = (isSelected ? "> " : "") + std::to_string(i + 1) + ". "
            + item.name + " [" + slotName(item.slot) + "] " + statsSummary(item.stats);
        sf::Color rowColor = rarityColor(item.rarity);
        if (isSelected) {
            rowColor = sf::Color(255, 215, 90);
        } else if (isHovered) {
            rowColor = sf::Color::White;
        }
        drawText(line, {x, y}, 13, rowColor);
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
                "Selected", std::to_string(key) + " Equip  |  Del Drop");
        }
    }
}

void Renderer::drawItemDetailPanel(const GameWorld& world, const sf::Vector2f& panelPos, const Item& item, const std::optional<Item>& current, const std::string& statusLabel, const std::string& actionHint) {
    const sf::Vector2f panelSize{500.0f, 220.0f};
    drawBox({panelPos.x + panelSize.x / 2.0f, panelPos.y + panelSize.y / 2.0f}, panelSize, sf::Color(18, 22, 30));

    const float x = panelPos.x + 14.0f;
    float y = panelPos.y + 12.0f;

    drawText(item.name + "  [" + rarityName(item.rarity) + "]  " + statusLabel, {x, y}, 16, rarityColor(item.rarity));
    y += 20.0f;

    drawText("Slot: " + std::string(slotName(item.slot)) + "   iLvl: " + std::to_string(item.itemLevel),
        {x, y}, 12, sf::Color(200, 210, 225));
    y += 18.0f;

    for (const auto& affix : item.affixes) {
        drawText("- " + affix, {x, y}, 11, sf::Color(160, 200, 255));
        y += 15.0f;
    }

    drawText("Stats: " + statsSummary(item.stats), {x, y}, 11, sf::Color(210, 220, 235));
    y += 16.0f;

    if (current) {
        drawText("Current: " + current->name + "  " + statsSummary(current->stats), {x, y}, 11, sf::Color(200, 200, 200));
    } else {
        drawText("Current: Empty", {x, y}, 11, sf::Color(150, 150, 150));
    }
    y += 16.0f;

    if (current) {
        const Stats delta = statsDelta(item.stats, current->stats);
        drawText("Delta: " + statsDeltaSummary(delta), {x, y}, 11, deltaColor(delta));
    } else {
        const std::string delta = statsSummary(item.stats);
        drawText("Delta: " + (delta.empty() ? "No stat change" : delta),
            {x, y}, 11, deltaColor(item.stats));
    }
    y += 17.0f;

    drawText("Skill impact if equipped:", {x, y}, 11, sf::Color(190, 200, 215));
    y += 16.0f;

    const Stats before = world.player().stats();
    const Stats after = combineStats(before,
        statsDelta(previewEquipmentStats(world.player().equipment(), item), world.player().equipment().combinedStats()));
    for (const auto& line : skillImpactDetailLines(before, after, world.skillBar())) {
        drawText(line, {x, y}, 11, sf::Color(180, 210, 255));
        y += 15.0f;
    }

    y += 6.0f;
    const bool isPickup = actionHint.rfind("F Pick up", 0) == 0;
    const bool isFull = actionHint.rfind("Inventory full", 0) == 0;
    const sf::Color hintColor = isFull ? sf::Color(255, 90, 90)
        : isPickup ? sf::Color(180, 220, 255)
        : sf::Color(200, 220, 240);
    drawText(actionHint, {x, y}, 12, hintColor);
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
        + "  |  Left click node  |  1-0/F1-F10 allocate  |  P close",
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
        drawText(node.name + " - " + node.description,
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

    drawBox({center.x, center.y}, {760.0f, 500.0f}, sf::Color(24, 30, 40));
    drawCenteredText("Skill Panel", {center.x, center.y - 228.0f}, 24, sf::Color::White);
    drawCenteredText("1-8 assign unlocked skill  |  K close",
        {center.x, center.y - 200.0f}, 14, sf::Color(210, 230, 255));

    const SkillSlot slots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };

    float y = center.y - 164.0f;
    drawText("Equipped", {center.x - 350.0f, y}, 16, sf::Color::White);
    y += 22.0f;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = slots[i];
        const auto& skill = world.skillBar().definition(slot);
        const float x = center.x - 350.0f + static_cast<float>(i % 2) * 370.0f;
        if (i == 2) {
            y += 20.0f;
        }
        drawText(skillSlotName(slot) + ": " + skill.name,
            {x, y}, 13, sf::Color(180, 230, 255));
    }

    y += 40.0f;
    drawText("Skills", {center.x - 350.0f, y}, 16, sf::Color::White);
    y += 24.0f;

    const auto& skills = SkillLibrary::all();
    for (std::size_t i = 0; i < skills.size(); ++i) {
        const auto& skill = skills[i];
        const bool equipped = world.skillBar().definition(skill.slot).name == skill.name;
        const bool unlocked = world.isSkillUnlocked(skill.name);
        const sf::Color color = !unlocked ? sf::Color(130, 135, 145)
            : equipped ? sf::Color(135, 245, 155)
            : sf::Color(220, 230, 240);
        const std::string state = equipped ? "Equipped" : unlocked ? "Available" : "Locked";
        const std::string marker = equipped ? "> " : "  ";
        const float columnX = center.x - 350.0f + static_cast<float>(i % 2) * 370.0f;
        const float rowY = y + static_cast<float>(i / 2) * 54.0f;
        drawText(marker + std::to_string(i + 1) + ". " + skill.name + " [" + state + "]",
            {columnX, rowY}, 14, color);
        drawText("     " + skillSlotName(skill.slot) + " / " + skillCastTypeName(skill.castType),
            {columnX, rowY + 17.0f}, 11,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
        drawText("     " + skillEffectiveSummary(skill, world.player().stats()),
            {columnX, rowY + 32.0f}, 10,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
    }
}

void Renderer::drawMinimap(const GameWorld& world) {
    const sf::Vector2f size{150.0f, 112.0f};
    const sf::Vector2f origin{
        static_cast<float>(Config::WindowWidth) - size.x - 18.0f,
        18.0f
    };
    const auto& map = world.map();
    const Vector2 mapSize = map.size();
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
    background.setFillColor(sf::Color(10, 14, 18, 205));
    background.setOutlineColor(sf::Color(180, 190, 200));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

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
    drawMapCircle(map.bossCenter(), Config::BossGateRadius, sf::Color(255, 190, 90));
    drawMapCircle(map.bossCenter(), Config::BossArenaRadius, sf::Color(255, 80, 60), 1.5f);

    for (const auto& event : map.events()) {
        sf::CircleShape eventDot(3.0f);
        eventDot.setOrigin({3.0f, 3.0f});
        eventDot.setPosition(toMinimap(event.position));
        eventDot.setFillColor(mapEventColor(event.type, event.completed));
        window_.draw(eventDot);
    }

    sf::CircleShape bossDot(3.5f);
    bossDot.setOrigin({3.5f, 3.5f});
    bossDot.setPosition(toMinimap(map.bossCenter()));
    bossDot.setFillColor(map.bossDefeated() ? sf::Color(120, 120, 120) : sf::Color(255, 80, 60));
    window_.draw(bossDot);

    sf::CircleShape playerDot(3.0f);
    playerDot.setOrigin({3.0f, 3.0f});
    playerDot.setPosition(toMinimap(world.player().position()));
    playerDot.setFillColor(sf::Color(90, 180, 255));
    window_.draw(playerDot);

    drawText("Map " + std::to_string(world.mapLevel())
        + "  " + std::to_string(static_cast<int>(map.progressToBoss(world.player().position()) * 100.0f)) + "%",
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

    const sf::Vector2f position{16.0f, 258.0f};
    const sf::Vector2f size{260.0f, 12.0f};
    const float ratio = std::clamp(
        static_cast<float>(std::max(0, boss->hp())) / static_cast<float>(boss->maxHp()),
        0.0f,
        1.0f
    );

    drawText(world.bossDefinition().name + "  "
        + std::to_string(std::max(0, boss->hp())) + "/" + std::to_string(boss->maxHp()),
        {position.x, position.y - 18.0f}, 13, sf::Color(255, 210, 160));

    const std::string castWarning = world.bossSkillWarning();
    if (!castWarning.empty()) {
        drawText(castWarning, {position.x, position.y + 18.0f}, 13, sf::Color(255, 130, 90));
    }

    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color(60, 30, 28, 210));
    background.setOutlineColor(sf::Color(255, 210, 160));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

    sf::RectangleShape fill({size.x * ratio, size.y});
    fill.setPosition(position);
    fill.setFillColor(sf::Color(220, 55, 45));
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
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 100, 0, 180));
    window_.draw(overlay);

    const std::string phaseText = !world.mapRewardChosen()
        ? "PHASE: CHOOSE REWARD"
        : !world.nextMapOptionChosen()
            ? "PHASE: CHOOSE NEXT MAP"
            : "PHASE: PRESS E TO ENTER MAP " + std::to_string(world.mapLevel() + 1);
    drawCenteredText(phaseText, {center.x, center.y - 160.0f}, 20, sf::Color(255, 240, 180));

    drawBox({center.x, center.y - 84.0f}, {360.0f, 90.0f}, sf::Color::Green);
    drawCenteredText("BOSS DEFEATED: " + world.bossDefinition().name,
        {center.x, center.y - 110.0f}, 22, sf::Color::White);
    drawCenteredText("Kills " + std::to_string(world.mapKills())
        + "  XP " + std::to_string(world.mapExperienceGained()),
        {center.x, center.y - 76.0f}, 16, sf::Color::White);
    drawCenteredText("Drops " + std::to_string(world.mapItemsDropped())
        + "  Boss Drops " + std::to_string(world.mapBossItemsDropped())
        + "  Picked " + std::to_string(world.mapItemsPickedUp()),
        {center.x, center.y - 52.0f}, 16, sf::Color::White);
    drawCenteredText("Events " + std::to_string(world.mapEventsCompleted())
        + "/" + std::to_string(world.mapEventsTotal()),
        {center.x, center.y - 30.0f}, 16, sf::Color::White);

    // Left-side pickup guidance (always available during MapComplete):
    // looting runs concurrently with reward / next-map selection, so the player
    // knows F still works and what it will target.
    float pickupY = 360.0f;
    drawText("F Pick up nearby drops", {20.0f, pickupY}, 14, sf::Color(180, 220, 255));
    pickupY += 20.0f;
    const int focusedLoot = world.focusedDroppedItemIndex();
    if (focusedLoot >= 0) {
        drawText("Focused: " + world.droppedItems()[static_cast<std::size_t>(focusedLoot)].item().name,
            {20.0f, pickupY}, 14, sf::Color(255, 220, 120));
        pickupY += 20.0f;
    }
    if (world.inventory().isFull()) {
        drawText("Inventory full - Tab select / Del drop an item", {20.0f, pickupY}, 14, sf::Color(255, 90, 90));
    }

    const auto& mapOptions = world.nextMapOptions();
    if (!world.mapRewardChosen()) {
        drawCenteredText("Choose Reward", {center.x, center.y - 2.0f}, 18, sf::Color::White);
        const auto& rewards = world.mapRewardOptions();
        float optionY = center.y + 26.0f;
        for (std::size_t i = 0; i < rewards.size(); ++i) {
            const auto& reward = rewards[i];
            drawText(std::to_string(i + 1) + ". " + reward.title,
                {center.x - 235.0f, optionY}, 15, sf::Color(220, 245, 255));
            drawText("     " + rewardDetailSummary(reward, world),
                {center.x - 235.0f, optionY + 19.0f}, 12, sf::Color(230, 220, 170));
            drawText("     " + rewardStatPreview(reward, world),
                {center.x - 235.0f, optionY + 34.0f}, 11, sf::Color(200, 220, 245));
            optionY += 58.0f;
        }
    } else {
        const auto selectedReward = static_cast<std::size_t>(world.selectedMapRewardOption());
        const auto& reward = world.mapRewardOptions()[selectedReward];
        drawCenteredText("Reward: " + reward.title,
            {center.x, center.y - 4.0f}, 16, sf::Color(150, 255, 175));
        drawCenteredText("Choose Next Map", {center.x, center.y + 22.0f}, 18, sf::Color::White);
        float optionY = center.y + 48.0f;
        for (std::size_t i = 0; i < mapOptions.size(); ++i) {
            const bool selected = world.selectedNextMapOption() == static_cast<int>(i);
            const auto& option = mapOptions[i];
            const sf::Color color = selected ? sf::Color(140, 255, 160) : sf::Color(220, 240, 255);
            const std::string marker = selected ? "> " : "  ";
            drawText(marker + std::to_string(i + 1) + ". " + option.modifier.name + " - " + option.recommendedLevel,
                {center.x - 235.0f, optionY}, 14, color);
            drawText("     " + option.modifier.description,
                {center.x - 235.0f, optionY + 17.0f}, 12, sf::Color(230, 220, 170));
            drawText("     " + mapOptionSummary(option) + "  |  " + option.rewardDescription,
                {center.x - 235.0f, optionY + 32.0f}, 12, sf::Color(200, 220, 245));
            optionY += 50.0f;
        }
    }

    drawBox({center.x, center.y + 198.0f}, {360.0f, 40.0f}, sf::Color::White);
    if (!world.mapRewardChosen()) {
        drawCenteredText("Pick 1 / 2 / 3 reward first",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
    } else if (world.nextMapOptionChosen()) {
        const auto& selected = mapOptions[static_cast<std::size_t>(world.selectedNextMapOption())];
        drawCenteredText("E Enter " + selected.modifier.name + " / R Restart",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
    } else {
        drawCenteredText("Pick 1 / 2 / 3 next map first",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
    }
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
        + std::to_string(world.inventory().capacity());
    float y = panelTop + 8.0f;
    drawText(inventoryTitle, {x, y}, 16, inventoryFull ? sf::Color(255, 90, 90) : sf::Color::White);
    y += 20.0f;
    if (inventoryFull) {
        drawText("Inventory full - drop an item to loot", {x, y}, 12, sf::Color(255, 90, 90));
        y += 16.0f;
    } else {
        drawText("Tab Select  Del Drop", {x, y}, 12, sf::Color(150, 160, 175));
        y += 16.0f;
    }

    const int selectedIndex = world.selectedInventoryIndex();
    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const auto& item = items[i];
        const bool isSelected = (static_cast<int>(i) == selectedIndex);
        const std::string line = (isSelected ? "> " : "") + std::to_string(i + 1) + ". "
            + item.name + " [" + slotName(item.slot) + "] " + statsSummary(item.stats);
        sf::Color rowColor = rarityColor(item.rarity);
        if (isSelected) {
            rowColor = sf::Color(255, 215, 90);
        }
        drawText(line, {x, y}, 13, rowColor);
        y += 16.0f;

        const auto& current = equipment.itemInSlot(item.slot);
        if (current) {
            const Stats delta = statsDelta(item.stats, current->stats);
            drawText("   Delta: " + statsDeltaSummary(delta), {x, y}, 11, deltaColor(delta));
            y += 14.0f;
        }
    }
}

void Renderer::drawMapCompleteLootDetail(const GameWorld& world) {
    const int index = world.focusedDroppedItemIndex();
    if (index < 0) {
        return;
    }
    const auto& droppedItem = world.droppedItems()[static_cast<std::size_t>(index)];
    const Item& item = droppedItem.item();
    const std::string actionHint = world.inventory().isFull()
        ? "Inventory full - Tab select / Del drop an item"
        : "F Pick up";
    // Top-left placement keeps the detail visible in the 800x600 window while
    // leaving the centered reward/map choice area and right-side inventory readable.
    const sf::Vector2f lootDetailPos{16.0f, 64.0f};
    drawItemDetailPanel(world, lootDetailPos, item, world.player().equipment().itemInSlot(item.slot),
        "Boss Drop", actionHint);
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
