#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BossDefinition.hpp"
#include "Affix.hpp"
#include "Crafting.hpp"
#include "Item.hpp"
#include "RandomService.hpp"

struct AffixDefinition {
    std::string name;
    bool isPrefix;
    EquipmentSlot slot;
    AffixStat stat;
    std::array<float, 3> tiers;
    std::vector<AffixTag> tags;
    int weight = 100;
    std::string id;
};

class LootGenerator {
public:
    Item generate(int monsterLevel, const LootBias& bias = {}) const {
        return generate(monsterLevel, RandomService::legacy(), bias);
    }

    Item generate(
        int monsterLevel,
        RandomService& random,
        const LootBias& bias = {}
    ) const {
        return generate(monsterLevel, random, 1.0f, bias);
    }

    Item generate(
        int monsterLevel,
        RandomService& random,
        float rarityMultiplier,
        const LootBias& bias = {}
    ) const {
        Item item;
        item.itemLevel = monsterLevel;
        item.slot = randomSlot(random);
        item.rarity = randomRarity(monsterLevel, random, rarityMultiplier);
        applyBase(item, randomBaseFor(item.slot, random));

        const int affixCount = affixCountFor(item.rarity);
        const int tier = tierForLevel(monsterLevel) + 1;
        std::vector<ItemAffix> prefixes;
        std::vector<ItemAffix> suffixes;
        std::set<std::size_t> usedIndices;
        std::set<AffixStat> usedStats;
        for (int i = 0; i < affixCount; ++i) {
            const AffixDefinition& affix = randomAffixFor(
                item.slot, usedIndices, usedStats, bias, random
            );
            const Stats contribution = affixStatsFor(affix, monsterLevel);
            item.stats = combineStats(item.stats, contribution);
            const ItemAffix itemAffix{
                affix.name, tier, contribution, affix.tags, affix.id, affix.stat, affix.isPrefix
            };
            if (affix.isPrefix) {
                prefixes.push_back(itemAffix);
            } else {
                suffixes.push_back(itemAffix);
            }
        }

        item.name = makeName(item.baseName, prefixes, suffixes);
        item.affixes = std::move(prefixes);
        item.affixes.insert(item.affixes.end(), suffixes.begin(), suffixes.end());

        return item;
    }

    Item generateBossReward(
        int monsterLevel,
        BossLootTheme theme,
        int variant = 0
    ) const {
        const int tier = tierForLevel(monsterLevel);
        const int normalizedVariant = ((variant % 2) + 2) % 2;
        Item item;
        item.itemLevel = monsterLevel;
        item.rarity = Rarity::Unique;
        applyBase(item, ItemBaseLibrary::forBossTheme(toBaseTheme(theme), normalizedVariant));
        item.affixes.push_back({"Boss relic", tier + 1, {}, {}, "", AffixStat::None, false});

        switch (theme) {
            case BossLootTheme::Brimstone:
                if (normalizedVariant == 0) {
                    item.name = "Colossus's Brand";
                    addBossAffix(item, "Brimstone might", tier + 1,
                        damageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.14f, 0.20f, 0.27f}[tier],
                            item.implicitStats.damageMultiplier)), {AffixTag::Damage});
                    addBossAffix(item, "Molten core", tier + 1,
                        fireDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.fireDamageMultiplier)), {AffixTag::Fire, AffixTag::Damage});
                    addBossAffix(item, "Crushing impact", tier + 1,
                        areaDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier],
                            item.implicitStats.areaDamageMultiplier)), {AffixTag::Area});
                } else {
                    item.name = "Ashen Crucible";
                    addBossAffix(item, "Ashen might", tier + 1,
                        damageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.16f, 0.22f}[tier],
                            item.implicitStats.damageMultiplier)), {AffixTag::Damage});
                    addBossAffix(item, "Smoldering core", tier + 1,
                        fireDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.fireDamageMultiplier)), {AffixTag::Fire, AffixTag::Damage});
                    addBossAffix(item, "Erupting reach", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.areaRadiusMultiplier)), {AffixTag::Fire, AffixTag::Area});
                }
                break;

            case BossLootTheme::Storm:
                if (normalizedVariant == 0) {
                    item.name = "Herald's Signet";
                    addBossAffix(item, "Storm cadence", tier + 1,
                        attackSpeedContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.attackSpeedMultiplier)), {AffixTag::AttackSpeed});
                    addBossAffix(item, "Conductive edge", tier + 1,
                        lightningDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.lightningDamageMultiplier)), {AffixTag::Lightning, AffixTag::Damage});
                    addBossAffix(item, "Charged projectiles", tier + 1,
                        projectileDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.projectileDamageMultiplier)), {AffixTag::Projectile, AffixTag::Damage});
                } else {
                    item.name = "Tempest Bow";
                    addBossAffix(item, "Tempest cadence", tier + 1,
                        attackSpeedContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier],
                            item.implicitStats.attackSpeedMultiplier)), {AffixTag::AttackSpeed});
                    addBossAffix(item, "Forked current", tier + 1,
                        lightningDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.lightningDamageMultiplier)), {AffixTag::Lightning, AffixTag::Damage});
                    addBossAffix(item, "Charged volleys", tier + 1,
                        projectileDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.16f, 0.22f}[tier],
                            item.implicitStats.projectileDamageMultiplier)), {AffixTag::Projectile, AffixTag::Damage});
                }
                break;

            case BossLootTheme::Brood:
                if (normalizedVariant == 0) {
                    item.name = "Matriarch's Talisman";
                    addBossAffix(item, "Brood venom", tier + 1,
                        poisonDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.poisonDamageMultiplier)), {AffixTag::Poison, AffixTag::Damage});
                    addBossAffix(item, "Expanding nests", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier],
                            item.implicitStats.areaRadiusMultiplier)), {AffixTag::Area});
                } else {
                    item.name = "Broodscale Band";
                    addBossAffix(item, "Broodscale venom", tier + 1,
                        poisonDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.poisonDamageMultiplier)), {AffixTag::Poison, AffixTag::Damage});
                    addBossAffix(item, "Nested reach", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.areaRadiusMultiplier)), {AffixTag::Poison, AffixTag::Area});
                }
                break;

            case BossLootTheme::Frost:
                if (normalizedVariant == 0) {
                    item.name = "Frostbound Loop";
                    addBossAffix(item, "Winter's bite", tier + 1,
                        coldDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.coldDamageMultiplier)), {AffixTag::Cold, AffixTag::Damage});
                    addBossAffix(item, "Biting chill", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier],
                            item.implicitStats.areaRadiusMultiplier)), {AffixTag::Cold, AffixTag::Area});
                } else {
                    item.name = "Winterheart Pendant";
                    addBossAffix(item, "Winterheart bite", tier + 1,
                        coldDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.coldDamageMultiplier)), {AffixTag::Cold, AffixTag::Damage});
                    addBossAffix(item, "Permafrost reach", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier],
                            item.implicitStats.areaRadiusMultiplier)), {AffixTag::Cold, AffixTag::Area});
                }
                break;

            case BossLootTheme::Archive:
                if (normalizedVariant == 0) {
                    item.name = "Tidebound Ledger";
                    addBossAffix(item, "Tidal memory", tier + 1,
                        coldDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.coldDamageMultiplier)),
                        {AffixTag::Cold, AffixTag::Damage});
                    addBossAffix(item, "Quill doctrine", tier + 1,
                        projectileDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.21f}[tier],
                            item.implicitStats.projectileDamageMultiplier)),
                        {AffixTag::Cold, AffixTag::Projectile});
                } else {
                    item.name = "Drowned Compass";
                    addBossAffix(item, "Compass of currents", tier + 1,
                        coldDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.12f, 0.18f, 0.24f}[tier],
                            item.implicitStats.coldDamageMultiplier)),
                        {AffixTag::Cold, AffixTag::Damage});
                    addBossAffix(item, "Far-shore volleys", tier + 1,
                        projectileDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.12f, 0.18f, 0.24f}[tier],
                            item.implicitStats.projectileDamageMultiplier)),
                        {AffixTag::Projectile, AffixTag::Damage});
                }
                break;

            case BossLootTheme::Obsidian:
                if (normalizedVariant == 0) {
                    item.name = "Obsidian Crown";
                    addBossAffix(item, "Shard authority", tier + 1,
                        areaDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.21f}[tier],
                            item.implicitStats.areaDamageMultiplier)),
                        {AffixTag::Area, AffixTag::Damage});
                    addBossAffix(item, "Furnace edge", tier + 1,
                        fireDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.10f, 0.15f, 0.20f}[tier],
                            item.implicitStats.fireDamageMultiplier)),
                        {AffixTag::Fire, AffixTag::Damage});
                    addBossAffix(item, "Blackglass reach", tier + 1,
                        areaRadiusContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier],
                            item.implicitStats.areaRadiusMultiplier)),
                        {AffixTag::Fire, AffixTag::Area});
                } else {
                    item.name = "Blackglass Heart";
                    addBossAffix(item, "Melted core", tier + 1,
                        fireDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.12f, 0.18f, 0.24f}[tier],
                            item.implicitStats.fireDamageMultiplier)),
                        {AffixTag::Fire, AffixTag::Damage});
                    addBossAffix(item, "Rupturing impact", tier + 1,
                        areaDamageContribution(relativeMultiplier(
                            1.0f + std::array<float, 3>{0.12f, 0.18f, 0.24f}[tier],
                            item.implicitStats.areaDamageMultiplier)),
                        {AffixTag::Area, AffixTag::Damage});
                }
                break;
        }

        return item;
    }

    static int bossRelicVariantForMapLevel(int mapLevel) {
        return (std::max(1, mapLevel) - 1) / 4 % 2;
    }

    static Rarity rarityForRoll(
        int monsterLevel,
        int roll,
        float rarityMultiplier = 1.0f
    ) {
        const int normalizedLevel = std::max(1, monsterLevel);
        const float multiplier = std::max(0.0f, rarityMultiplier);
        const int rareChance = std::min(75, static_cast<int>(std::ceil(
            static_cast<float>(8 + normalizedLevel * 4) * multiplier
        )));
        const int magicChance = std::min(
            90 - rareChance,
            static_cast<int>(std::ceil(
                static_cast<float>(30 + normalizedLevel * 3) * multiplier
            ))
        );
        if (roll < rareChance) {
            return Rarity::Rare;
        }
        if (roll < rareChance + magicChance) {
            return Rarity::Magic;
        }
        return Rarity::Normal;
    }

    static const std::vector<AffixDefinition>& affixDefinitions() {
        return affixPool();
    }

    static const AffixDefinition* definitionFor(const std::string& id) {
        const auto& pool = affixPool();
        const auto it = std::find_if(pool.begin(), pool.end(), [&](const AffixDefinition& affix) {
            return affix.id == id;
        });
        return it == pool.end() ? nullptr : &*it;
    }

    static int maxTierForLevel(int itemLevel) {
        return tierForLevel(itemLevel) + 1;
    }

    static Stats contributionFor(const std::string& id, int itemLevel, int tier) {
        const AffixDefinition* definition = definitionFor(id);
        return definition == nullptr ? Stats() : affixStatsForTier(*definition, itemLevel, tier);
    }

    static void rebuildStats(Item& item) {
        item.stats = item.implicitStats;
        for (const auto& affix : item.affixes) {
            item.stats = combineStats(item.stats, affix.stats);
        }
    }

    static CraftingResult improveAffix(Item& item, std::size_t affixIndex) {
        if (affixIndex >= item.affixes.size()) {
            return CraftingResult::InvalidTarget;
        }

        ItemAffix& itemAffix = item.affixes[affixIndex];
        const AffixDefinition* definition = definitionFor(itemAffix.id);
        if (definition == nullptr) {
            return CraftingResult::InvalidTarget;
        }

        const Stats cap = affixStatsForTier(
            *definition,
            item.itemLevel,
            std::min(3, itemAffix.tier + 1)
        );
        const Stats improved = improvedContribution(itemAffix.stats, cap, definition->stat);
        if (statsEqual(improved, itemAffix.stats)) {
            return CraftingResult::NoImprovement;
        }

        itemAffix.stats = improved;
        rebuildStats(item);
        return CraftingResult::Success;
    }

    static CraftingResult raiseAffixTier(Item& item, std::size_t affixIndex) {
        if (affixIndex >= item.affixes.size()) {
            return CraftingResult::InvalidTarget;
        }

        ItemAffix& itemAffix = item.affixes[affixIndex];
        const AffixDefinition* definition = definitionFor(itemAffix.id);
        if (definition == nullptr) {
            return CraftingResult::InvalidTarget;
        }
        if (itemAffix.tier >= maxTierForLevel(item.itemLevel)) {
            return CraftingResult::AlreadyMaxTier;
        }

        ++itemAffix.tier;
        itemAffix.stats = affixStatsForTier(*definition, item.itemLevel, itemAffix.tier);
        rebuildStats(item);
        return CraftingResult::Success;
    }

    static CraftingResult rerollAffix(
        Item& item,
        std::size_t affixIndex,
        const LootBias& bias = {}
    ) {
        return rerollAffix(item, affixIndex, RandomService::legacy(), bias);
    }

    static CraftingResult rerollAffix(
        Item& item,
        std::size_t affixIndex,
        RandomService& random,
        const LootBias& bias = {}
    ) {
        if (affixIndex >= item.affixes.size()) {
            return CraftingResult::InvalidTarget;
        }

        const ItemAffix& target = item.affixes[affixIndex];
        const AffixDefinition* currentDefinition = definitionFor(target.id);
        if (currentDefinition == nullptr) {
            return CraftingResult::InvalidTarget;
        }

        const auto candidates = rerollCandidateIndices(item, affixIndex);
        if (candidates.empty()) {
            return CraftingResult::NoCandidates;
        }

        std::vector<int> weights;
        weights.reserve(candidates.size());
        for (const std::size_t index : candidates) {
            weights.push_back(weightFor(affixPool()[index], bias));
        }
        const std::size_t selected = candidates[
            weightedChoiceIndex(weights, random)
        ];
        const AffixDefinition& replacement = affixPool()[selected];
        ItemAffix replacementAffix{
            replacement.name,
            target.tier,
            affixStatsForTier(replacement, item.itemLevel, target.tier),
            replacement.tags,
            replacement.id,
            replacement.stat,
            replacement.isPrefix
        };
        item.affixes[affixIndex] = std::move(replacementAffix);
        rebuildStats(item);
        refreshGeneratedName(item);
        return CraftingResult::Success;
    }

    static int weightFor(const AffixDefinition& affix, const LootBias& bias) {
        float weight = static_cast<float>(std::max(1, affix.weight));
        const auto applyBias = [&](AffixTag tag, float multiplier) {
            if (tag == AffixTag::None || multiplier <= 0.0f) {
                return;
            }
            if (std::find(affix.tags.begin(), affix.tags.end(), tag) != affix.tags.end()) {
                weight *= multiplier;
            }
        };
        applyBias(bias.primaryTag, bias.primaryWeightMultiplier);
        applyBias(bias.secondaryTag, bias.secondaryWeightMultiplier);
        return std::max(1, static_cast<int>(std::ceil(weight)));
    }

    static std::size_t weightedChoiceIndex(const std::vector<int>& weights, int roll) {
        if (weights.empty()) {
            return 0;
        }

        long long total = 0;
        for (const int weight : weights) {
            total += std::max(0, weight);
        }
        if (total <= 0) {
            return 0;
        }

        long long normalizedRoll = static_cast<long long>(roll);
        if (normalizedRoll < 0) {
            normalizedRoll = -normalizedRoll;
        }
        long long target = normalizedRoll % total;
        for (std::size_t index = 0; index < weights.size(); ++index) {
            target -= std::max(0, weights[index]);
            if (target < 0) {
                return index;
            }
        }
        return weights.size() - 1;
    }

    static std::size_t weightedChoiceIndex(
        const std::vector<int>& weights,
        RandomService& random
    ) {
        return random.weightedChoiceIndex(weights);
    }

private:
    static ItemBaseTheme toBaseTheme(BossLootTheme theme) {
        switch (theme) {
            case BossLootTheme::Brimstone: return ItemBaseTheme::Brimstone;
            case BossLootTheme::Storm: return ItemBaseTheme::Storm;
            case BossLootTheme::Brood: return ItemBaseTheme::Brood;
            case BossLootTheme::Frost: return ItemBaseTheme::Frost;
            case BossLootTheme::Archive: return ItemBaseTheme::Archive;
            case BossLootTheme::Obsidian: return ItemBaseTheme::Obsidian;
        }
        return ItemBaseTheme::None;
    }

    static void applyBase(Item& item, const ItemBaseDefinition& base) {
        item.baseId = base.id;
        item.baseName = base.name;
        item.slot = base.slot;
        item.implicitStats = base.implicitStats;
        item.stats = item.implicitStats;
    }

    static const ItemBaseDefinition& randomBaseFor(EquipmentSlot slot, RandomService& random) {
        std::vector<const ItemBaseDefinition*> matching;
        for (const auto& base : ItemBaseLibrary::all()) {
            if (base.kind == ItemBaseKind::Normal && base.slot == slot) {
                matching.push_back(&base);
            }
        }

        if (matching.empty()) {
            return ItemBaseLibrary::all().front();
        }
        return *matching[random.nextIndex(matching.size())];
    }

    static float relativeMultiplier(float target, float base) {
        return base <= 0.0f ? target : target / base;
    }

    static Stats damageContribution(float multiplier) {
        Stats stats;
        stats.damageMultiplier = multiplier;
        return stats;
    }

    static Stats attackSpeedContribution(float multiplier) {
        Stats stats;
        stats.attackSpeedMultiplier = multiplier;
        return stats;
    }

    static Stats projectileDamageContribution(float multiplier) {
        Stats stats;
        stats.projectileDamageMultiplier = multiplier;
        return stats;
    }

    static Stats areaDamageContribution(float multiplier) {
        Stats stats;
        stats.areaDamageMultiplier = multiplier;
        return stats;
    }

    static Stats areaRadiusContribution(float multiplier) {
        Stats stats;
        stats.areaRadiusMultiplier = multiplier;
        return stats;
    }

    static Stats fireDamageContribution(float multiplier) {
        Stats stats;
        stats.fireDamageMultiplier = multiplier;
        return stats;
    }

    static Stats coldDamageContribution(float multiplier) {
        Stats stats;
        stats.coldDamageMultiplier = multiplier;
        return stats;
    }

    static Stats lightningDamageContribution(float multiplier) {
        Stats stats;
        stats.lightningDamageMultiplier = multiplier;
        return stats;
    }

    static Stats poisonDamageContribution(float multiplier) {
        Stats stats;
        stats.poisonDamageMultiplier = multiplier;
        return stats;
    }

    static void addBossAffix(
        Item& item,
        const std::string& name,
        int tier,
        const Stats& contribution,
        std::vector<AffixTag> tags
    ) {
        item.stats = combineStats(item.stats, contribution);
        item.affixes.push_back({name, tier, contribution, std::move(tags), "", AffixStat::None, false});
    }

    static const std::vector<AffixDefinition>& affixPool() {
        static const std::vector<AffixDefinition> pool = buildAffixPool();
        return pool;
    }

    static std::vector<AffixDefinition> buildAffixPool() {
        auto pool = std::vector<AffixDefinition>{
            // Weapon
            {"Vicious", true, EquipmentSlot::Weapon, AffixStat::DamageMultiplier, {0.08f, 0.14f, 0.20f}},
            {"Serrated", true, EquipmentSlot::Weapon, AffixStat::DamageMultiplier, {0.06f, 0.10f, 0.14f}},
            {"Swift", true, EquipmentSlot::Weapon, AffixStat::AttackSpeedMultiplier, {0.05f, 0.09f, 0.13f}},
            {"of Force", false, EquipmentSlot::Weapon, AffixStat::DamageMultiplier, {0.04f, 0.08f, 0.12f}},
            {"of Swiftness", false, EquipmentSlot::Weapon, AffixStat::AttackSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Piercing", false, EquipmentSlot::Weapon, AffixStat::DamageMultiplier, {0.03f, 0.06f, 0.09f}},
            // Weapon build-specific affixes (Projectile / Area)
            {"Piercing", true, EquipmentSlot::Weapon, AffixStat::ProjectileDamageMultiplier, {0.05f, 0.09f, 0.13f}},
            {"of Projectiles", false, EquipmentSlot::Weapon, AffixStat::ProjectileDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Shattering", true, EquipmentSlot::Weapon, AffixStat::AreaDamageMultiplier, {0.05f, 0.09f, 0.13f}},
            {"of Blasting", false, EquipmentSlot::Weapon, AffixStat::AreaDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Wide", true, EquipmentSlot::Weapon, AffixStat::AreaRadiusMultiplier, {0.05f, 0.09f, 0.13f}},
            {"of Expansion", false, EquipmentSlot::Weapon, AffixStat::AreaRadiusMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Emberforged", true, EquipmentSlot::Weapon, AffixStat::FireDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Frostbite", false, EquipmentSlot::Weapon, AffixStat::ColdDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Stormcharged", true, EquipmentSlot::Weapon, AffixStat::LightningDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Venomforged", true, EquipmentSlot::Weapon, AffixStat::PoisonDamageMultiplier, {0.04f, 0.07f, 0.10f}},

            // Armor
            {"Sturdy", true, EquipmentSlot::Armor, AffixStat::MaxHp, {4.0f, 8.0f, 12.0f}},
            {"Reinforced", true, EquipmentSlot::Armor, AffixStat::MaxHp, {3.0f, 6.0f, 9.0f}},
            {"Plated", true, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 5.0f, 8.0f}},
            {"of Vitality", false, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"Guarded", true, EquipmentSlot::Armor, AffixStat::Armor, {1.0f, 2.0f, 3.0f}},
            {"of Bulwark", false, EquipmentSlot::Armor, AffixStat::Armor, {1.0f, 1.0f, 2.0f}},
            {"of Haste", false, EquipmentSlot::Armor, AffixStat::MoveSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Reach", false, EquipmentSlot::Armor, AffixStat::PickupRangeMultiplier, {0.08f, 0.14f, 0.20f}},
            {"Flameguard", true, EquipmentSlot::Armor, AffixStat::FireResistance, {4.0f, 8.0f, 12.0f}},
            {"Frostguard", true, EquipmentSlot::Armor, AffixStat::ColdResistance, {4.0f, 8.0f, 12.0f}},
            {"Stormguard", true, EquipmentSlot::Armor, AffixStat::LightningResistance, {4.0f, 8.0f, 12.0f}},
            {"Venomguard", true, EquipmentSlot::Armor, AffixStat::PoisonResistance, {4.0f, 8.0f, 12.0f}},

            // Ring
            {"Glinting", true, EquipmentSlot::Ring, AffixStat::DamageMultiplier, {0.05f, 0.09f, 0.13f}},
            {"Agile", true, EquipmentSlot::Ring, AffixStat::AttackSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Runner's", true, EquipmentSlot::Ring, AffixStat::MoveSpeedMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Vitality", false, EquipmentSlot::Ring, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"of Swiftness", false, EquipmentSlot::Ring, AffixStat::AttackSpeedMultiplier, {0.03f, 0.06f, 0.09f}},
            {"of Haste", false, EquipmentSlot::Ring, AffixStat::MoveSpeedMultiplier, {0.03f, 0.06f, 0.09f}},
            // Ring build-specific affixes (Projectile / Area)
            {"Piercing", true, EquipmentSlot::Ring, AffixStat::ProjectileDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Projectiles", false, EquipmentSlot::Ring, AffixStat::ProjectileDamageMultiplier, {0.03f, 0.05f, 0.08f}},
            {"Shattering", true, EquipmentSlot::Ring, AffixStat::AreaDamageMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Blasting", false, EquipmentSlot::Ring, AffixStat::AreaDamageMultiplier, {0.03f, 0.05f, 0.08f}},
            {"Wide", true, EquipmentSlot::Ring, AffixStat::AreaRadiusMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Expansion", false, EquipmentSlot::Ring, AffixStat::AreaRadiusMultiplier, {0.03f, 0.05f, 0.08f}},

            // Amulet
            {"Blessed", true, EquipmentSlot::Amulet, AffixStat::MaxHp, {3.0f, 6.0f, 9.0f}},
            {"Radiant", true, EquipmentSlot::Amulet, AffixStat::DamageMultiplier, {0.06f, 0.10f, 0.14f}},
            {"Gilded", true, EquipmentSlot::Amulet, AffixStat::PickupRangeMultiplier, {0.10f, 0.16f, 0.22f}},
            {"of Vitality", false, EquipmentSlot::Amulet, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"of Haste", false, EquipmentSlot::Amulet, AffixStat::MoveSpeedMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Reach", false, EquipmentSlot::Amulet, AffixStat::PickupRangeMultiplier, {0.08f, 0.12f, 0.16f}},
            // Amulet build-specific affixes (Projectile / Area)
            {"Piercing", true, EquipmentSlot::Amulet, AffixStat::ProjectileDamageMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Projectiles", false, EquipmentSlot::Amulet, AffixStat::ProjectileDamageMultiplier, {0.03f, 0.06f, 0.09f}},
            {"Shattering", true, EquipmentSlot::Amulet, AffixStat::AreaDamageMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Blasting", false, EquipmentSlot::Amulet, AffixStat::AreaDamageMultiplier, {0.03f, 0.06f, 0.09f}},
            {"Wide", true, EquipmentSlot::Amulet, AffixStat::AreaRadiusMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Expansion", false, EquipmentSlot::Amulet, AffixStat::AreaRadiusMultiplier, {0.03f, 0.06f, 0.09f}},
            {"Toxic", true, EquipmentSlot::Amulet, AffixStat::PoisonDamageMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Antidotes", false, EquipmentSlot::Amulet, AffixStat::PoisonResistance, {4.0f, 8.0f, 12.0f}},
            {"Arcane", true, EquipmentSlot::Ring, AffixStat::MaxManaMultiplier, {0.08f, 0.14f, 0.20f}},
            {"of Meditation", false, EquipmentSlot::Ring, AffixStat::ManaRegenMultiplier, {0.10f, 0.18f, 0.26f}},
            {"Arcane", true, EquipmentSlot::Amulet, AffixStat::MaxManaMultiplier, {0.10f, 0.16f, 0.22f}},
            {"of Meditation", false, EquipmentSlot::Amulet, AffixStat::ManaRegenMultiplier, {0.12f, 0.20f, 0.28f}},
        };

        for (auto& affix : pool) {
            affix.tags = tagsForStat(affix.stat);
            affix.weight = defaultWeightFor(affix);
        }
        for (std::size_t index = 0; index < pool.size(); ++index) {
            // Pool position is a stable data identity; display names are not IDs.
            pool[index].id = "affix_" + std::to_string(index);
        }
        return pool;
    }

    static std::vector<AffixTag> tagsForStat(AffixStat stat) {
        switch (stat) {
            case AffixStat::MaxHp:
                return {AffixTag::Survival};
            case AffixStat::MaxManaMultiplier:
            case AffixStat::ManaRegenMultiplier:
                return {AffixTag::Survival};
            case AffixStat::DamageMultiplier:
                return {AffixTag::Damage};
            case AffixStat::AttackSpeedMultiplier:
                return {AffixTag::AttackSpeed};
            case AffixStat::MoveSpeedMultiplier:
                return {AffixTag::MoveSpeed};
            case AffixStat::PickupRangeMultiplier:
                return {AffixTag::Pickup};
            case AffixStat::ProjectileDamageMultiplier:
                return {AffixTag::Projectile, AffixTag::Damage};
            case AffixStat::AreaDamageMultiplier:
            case AffixStat::AreaRadiusMultiplier:
                return {AffixTag::Area};
            case AffixStat::Armor:
                return {AffixTag::Armor, AffixTag::Survival};
            case AffixStat::FireDamageMultiplier:
                return {AffixTag::Fire, AffixTag::Damage};
            case AffixStat::ColdDamageMultiplier:
                return {AffixTag::Cold, AffixTag::Damage};
            case AffixStat::LightningDamageMultiplier:
                return {AffixTag::Lightning, AffixTag::Damage};
            case AffixStat::PoisonDamageMultiplier:
                return {AffixTag::Poison, AffixTag::Damage};
            case AffixStat::FireResistance:
            case AffixStat::ColdResistance:
            case AffixStat::LightningResistance:
            case AffixStat::PoisonResistance:
                return {AffixTag::Survival};
        }
        return {AffixTag::None};
    }

    static int defaultWeightFor(const AffixDefinition& affix) {
        int weight = 100;
        switch (affix.stat) {
            case AffixStat::MaxHp: weight = 110; break;
            case AffixStat::MaxManaMultiplier:
            case AffixStat::ManaRegenMultiplier: weight = 90; break;
            case AffixStat::DamageMultiplier: weight = 100; break;
            case AffixStat::AttackSpeedMultiplier: weight = 95; break;
            case AffixStat::MoveSpeedMultiplier: weight = 90; break;
            case AffixStat::PickupRangeMultiplier: weight = 85; break;
            case AffixStat::ProjectileDamageMultiplier: weight = 90; break;
            case AffixStat::AreaDamageMultiplier:
            case AffixStat::AreaRadiusMultiplier: weight = 90; break;
            case AffixStat::Armor: weight = 95; break;
            case AffixStat::FireDamageMultiplier:
            case AffixStat::ColdDamageMultiplier:
            case AffixStat::LightningDamageMultiplier:
            case AffixStat::PoisonDamageMultiplier: weight = 85; break;
            case AffixStat::FireResistance:
            case AffixStat::ColdResistance:
            case AffixStat::LightningResistance:
            case AffixStat::PoisonResistance: weight = 100; break;
        }
        return affix.isPrefix ? weight + 10 : weight;
    }

    static EquipmentSlot randomSlot(RandomService& random) {
        switch (random.nextInt(0, 3)) {
            case 0: return EquipmentSlot::Weapon;
            case 1: return EquipmentSlot::Armor;
            case 2: return EquipmentSlot::Ring;
            default: return EquipmentSlot::Amulet;
        }
    }

    static Rarity randomRarity(
        int monsterLevel,
        RandomService& random,
        float rarityMultiplier
    ) {
        return rarityForRoll(monsterLevel, random.nextInt(0, 99), rarityMultiplier);
    }

    static int affixCountFor(Rarity rarity) {
        switch (rarity) {
            case Rarity::Normal: return 1;
            case Rarity::Magic: return 2;
            case Rarity::Rare: return 3;
            case Rarity::Unique: return 3;
        }
        return 1;
    }

    static const AffixDefinition& randomAffixFor(
        EquipmentSlot slot,
        std::set<std::size_t>& usedIndices,
        std::set<AffixStat>& usedStats,
        const LootBias& bias,
        RandomService& random
    ) {
        const auto& pool = affixPool();
        std::vector<std::size_t> matching;
        for (std::size_t i = 0; i < pool.size(); ++i) {
            if (pool[i].slot == slot
                && usedIndices.find(i) == usedIndices.end()
                && usedStats.find(pool[i].stat) == usedStats.end()) {
                matching.push_back(i);
            }
        }

        if (matching.empty()) {
            for (std::size_t i = 0; i < pool.size(); ++i) {
                if (pool[i].slot == slot && usedIndices.find(i) == usedIndices.end()) {
                    matching.push_back(i);
                }
            }
        }

        if (matching.empty()) {
            for (std::size_t i = 0; i < pool.size(); ++i) {
                if (pool[i].slot == slot) {
                    matching.push_back(i);
                }
            }
        }

        std::vector<int> weights;
        weights.reserve(matching.size());
        for (const std::size_t index : matching) {
            weights.push_back(weightFor(pool[index], bias));
        }

        const std::size_t matchingIndex = weightedChoiceIndex(weights, random);
        const std::size_t index = matching.empty() ? 0 : matching[matchingIndex];
        usedIndices.insert(index);
        if (index < pool.size()) {
            usedStats.insert(pool[index].stat);
        }
        return pool[index];
    }

    static std::vector<std::size_t> rerollCandidateIndices(
        const Item& item,
        std::size_t targetIndex
    ) {
        if (targetIndex >= item.affixes.size()) {
            return {};
        }

        const ItemAffix& target = item.affixes[targetIndex];
        const AffixDefinition* targetDefinition = definitionFor(target.id);
        if (targetDefinition == nullptr) {
            return {};
        }

        std::set<AffixStat> usedStats;
        std::set<std::string> usedIds;
        for (std::size_t index = 0; index < item.affixes.size(); ++index) {
            if (index == targetIndex) {
                continue;
            }
            if (item.affixes[index].stat != AffixStat::None) {
                usedStats.insert(item.affixes[index].stat);
            }
            if (!item.affixes[index].id.empty()) {
                usedIds.insert(item.affixes[index].id);
            }
        }

        std::vector<std::size_t> candidates;
        const auto& pool = affixPool();
        for (std::size_t index = 0; index < pool.size(); ++index) {
            const auto& candidate = pool[index];
            if (candidate.slot != item.slot
                || candidate.isPrefix != targetDefinition->isPrefix
                || candidate.id == target.id
                || usedStats.find(candidate.stat) != usedStats.end()
                || usedIds.find(candidate.id) != usedIds.end()) {
                continue;
            }
            candidates.push_back(index);
        }
        return candidates;
    }

    static int tierForLevel(int monsterLevel) {
        if (monsterLevel <= 2) {
            return 0;
        }
        if (monsterLevel <= 4) {
            return 1;
        }
        return 2;
    }

    static Stats affixStatsFor(const AffixDefinition& affix, int monsterLevel) {
        return affixStatsForTier(affix, monsterLevel, tierForLevel(monsterLevel) + 1);
    }

    static Stats affixStatsForTier(const AffixDefinition& affix, int monsterLevel, int tier) {
        Stats stats;
        const int clampedTier = std::clamp(tier, 1, static_cast<int>(affix.tiers.size()));
        const float value = affix.tiers[static_cast<std::size_t>(clampedTier - 1)];
        switch (affix.stat) {
            case AffixStat::MaxHp:
                stats.maxHp += static_cast<int>(value);
                break;
            case AffixStat::DamageMultiplier:
                stats.damageMultiplier += value;
                break;
            case AffixStat::AttackSpeedMultiplier:
                stats.attackSpeedMultiplier += value;
                break;
            case AffixStat::MoveSpeedMultiplier:
                stats.moveSpeedMultiplier += value;
                break;
            case AffixStat::PickupRangeMultiplier:
                stats.pickupRangeMultiplier += value;
                break;
            case AffixStat::ProjectileDamageMultiplier:
                stats.projectileDamageMultiplier += value;
                break;
            case AffixStat::AreaDamageMultiplier:
                stats.areaDamageMultiplier += value;
                break;
            case AffixStat::AreaRadiusMultiplier:
                stats.areaRadiusMultiplier += value;
                break;
            case AffixStat::Armor:
                stats.armor += static_cast<int>(value);
                break;
            case AffixStat::FireDamageMultiplier:
                stats.fireDamageMultiplier += value;
                break;
            case AffixStat::ColdDamageMultiplier:
                stats.coldDamageMultiplier += value;
                break;
            case AffixStat::LightningDamageMultiplier:
                stats.lightningDamageMultiplier += value;
                break;
            case AffixStat::PoisonDamageMultiplier:
                stats.poisonDamageMultiplier += value;
                break;
            case AffixStat::FireResistance:
                stats.fireResistance += static_cast<int>(value);
                break;
            case AffixStat::ColdResistance:
                stats.coldResistance += static_cast<int>(value);
                break;
            case AffixStat::LightningResistance:
                stats.lightningResistance += static_cast<int>(value);
                break;
            case AffixStat::PoisonResistance:
                stats.poisonResistance += static_cast<int>(value);
                break;
            case AffixStat::MaxManaMultiplier:
                stats.maxManaMultiplier += value;
                break;
            case AffixStat::ManaRegenMultiplier:
                stats.manaRegenMultiplier += value;
                break;
            case AffixStat::SkillCostMultiplier:
                stats.skillCostMultiplier += value;
                break;
        }
        return stats;
    }

    static Stats improvedContribution(const Stats& current, const Stats& cap, AffixStat stat) {
        constexpr float improvementMultiplier = 1.15f;
        Stats improved = current;
        switch (stat) {
            case AffixStat::MaxHp:
                improved.maxHp = std::min(cap.maxHp,
                    std::max(current.maxHp + 1,
                        static_cast<int>(std::ceil(current.maxHp * improvementMultiplier))));
                break;
            case AffixStat::DamageMultiplier:
                improved.damageMultiplier = std::min(cap.damageMultiplier,
                    1.0f + (current.damageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::AttackSpeedMultiplier:
                improved.attackSpeedMultiplier = std::min(cap.attackSpeedMultiplier,
                    1.0f + (current.attackSpeedMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::MoveSpeedMultiplier:
                improved.moveSpeedMultiplier = std::min(cap.moveSpeedMultiplier,
                    1.0f + (current.moveSpeedMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::PickupRangeMultiplier:
                improved.pickupRangeMultiplier = std::min(cap.pickupRangeMultiplier,
                    1.0f + (current.pickupRangeMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::ProjectileDamageMultiplier:
                improved.projectileDamageMultiplier = std::min(cap.projectileDamageMultiplier,
                    1.0f + (current.projectileDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::AreaDamageMultiplier:
                improved.areaDamageMultiplier = std::min(cap.areaDamageMultiplier,
                    1.0f + (current.areaDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::AreaRadiusMultiplier:
                improved.areaRadiusMultiplier = std::min(cap.areaRadiusMultiplier,
                    1.0f + (current.areaRadiusMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::Armor:
                improved.armor = std::min(cap.armor,
                    std::max(current.armor + 1,
                        static_cast<int>(std::ceil(current.armor * improvementMultiplier))));
                break;
            case AffixStat::FireDamageMultiplier:
                improved.fireDamageMultiplier = std::min(cap.fireDamageMultiplier,
                    1.0f + (current.fireDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::ColdDamageMultiplier:
                improved.coldDamageMultiplier = std::min(cap.coldDamageMultiplier,
                    1.0f + (current.coldDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::LightningDamageMultiplier:
                improved.lightningDamageMultiplier = std::min(cap.lightningDamageMultiplier,
                    1.0f + (current.lightningDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::FireResistance:
                improved.fireResistance = std::min(cap.fireResistance,
                    std::max(current.fireResistance + 1,
                        static_cast<int>(std::ceil(current.fireResistance * improvementMultiplier))));
                break;
            case AffixStat::ColdResistance:
                improved.coldResistance = std::min(cap.coldResistance,
                    std::max(current.coldResistance + 1,
                        static_cast<int>(std::ceil(current.coldResistance * improvementMultiplier))));
                break;
            case AffixStat::LightningResistance:
                improved.lightningResistance = std::min(cap.lightningResistance,
                    std::max(current.lightningResistance + 1,
                        static_cast<int>(std::ceil(current.lightningResistance * improvementMultiplier))));
                break;
            case AffixStat::PoisonDamageMultiplier:
                improved.poisonDamageMultiplier = std::min(cap.poisonDamageMultiplier,
                    1.0f + (current.poisonDamageMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::PoisonResistance:
                improved.poisonResistance = std::min(cap.poisonResistance,
                    std::max(current.poisonResistance + 1,
                        static_cast<int>(std::ceil(current.poisonResistance * improvementMultiplier))));
                break;
            case AffixStat::MaxManaMultiplier:
                improved.maxManaMultiplier = std::min(cap.maxManaMultiplier,
                    1.0f + (current.maxManaMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::ManaRegenMultiplier:
                improved.manaRegenMultiplier = std::min(cap.manaRegenMultiplier,
                    1.0f + (current.manaRegenMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::SkillCostMultiplier:
                improved.skillCostMultiplier = std::max(cap.skillCostMultiplier,
                    1.0f + (current.skillCostMultiplier - 1.0f) * improvementMultiplier);
                break;
            case AffixStat::None:
                break;
        }
        return improved;
    }

    static bool statsEqual(const Stats& left, const Stats& right) {
        return left.maxHp == right.maxHp
            && left.moveSpeedMultiplier == right.moveSpeedMultiplier
            && left.damageMultiplier == right.damageMultiplier
            && left.attackSpeedMultiplier == right.attackSpeedMultiplier
            && left.pickupRangeMultiplier == right.pickupRangeMultiplier
            && left.projectileDamageMultiplier == right.projectileDamageMultiplier
            && left.areaDamageMultiplier == right.areaDamageMultiplier
            && left.areaRadiusMultiplier == right.areaRadiusMultiplier
            && left.armor == right.armor
            && left.projectileCountBonus == right.projectileCountBonus
            && left.lifeFlaskEffectMultiplier == right.lifeFlaskEffectMultiplier
            && left.itemQuantityMultiplier == right.itemQuantityMultiplier
            && left.incomingDamageMultiplier == right.incomingDamageMultiplier
            && left.fireDamageMultiplier == right.fireDamageMultiplier
            && left.coldDamageMultiplier == right.coldDamageMultiplier
            && left.lightningDamageMultiplier == right.lightningDamageMultiplier
            && left.fireResistance == right.fireResistance
            && left.coldResistance == right.coldResistance
            && left.lightningResistance == right.lightningResistance
            && std::abs(left.poisonDamageMultiplier - right.poisonDamageMultiplier) < 0.0001f
            && left.poisonResistance == right.poisonResistance
            && std::abs(left.maxManaMultiplier - right.maxManaMultiplier) < 0.0001f
            && std::abs(left.manaRegenMultiplier - right.manaRegenMultiplier) < 0.0001f
            && std::abs(left.skillCostMultiplier - right.skillCostMultiplier) < 0.0001f;
    }

    static void refreshGeneratedName(Item& item) {
        if (item.baseName.empty()) {
            return;
        }
        std::string prefix;
        std::string suffix;
        for (const auto& affix : item.affixes) {
            if (affix.id.empty()) {
                return;
            }
            if (affix.isPrefix && prefix.empty()) {
                prefix = affix.name;
            } else if (!affix.isPrefix && suffix.empty()) {
                suffix = affix.name;
            }
        }
        item.name = prefix.empty() ? item.baseName : prefix + " " + item.baseName;
        if (!suffix.empty()) {
            item.name += " " + suffix;
        }
    }

    static std::string makeName(const std::string& baseName,
        const std::vector<ItemAffix>& prefixes, const std::vector<ItemAffix>& suffixes) {
        std::string name;
        if (!prefixes.empty()) {
            name += prefixes.front().name + " ";
        }
        name += baseName;
        if (!suffixes.empty()) {
            name += " " + suffixes.front().name;
        }
        return name;
    }
};
