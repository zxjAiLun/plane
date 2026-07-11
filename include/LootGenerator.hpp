#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BossDefinition.hpp"
#include "Item.hpp"

enum class AffixStat {
    MaxHp,
    DamageMultiplier,
    AttackSpeedMultiplier,
    MoveSpeedMultiplier,
    PickupRangeMultiplier,
    ProjectileDamageMultiplier,
    AreaDamageMultiplier,
    AreaRadiusMultiplier,
    Armor
};

struct AffixDefinition {
    std::string name;
    bool isPrefix;
    EquipmentSlot slot;
    AffixStat stat;
    std::array<float, 3> tiers;
};

class LootGenerator {
public:
    Item generate(int monsterLevel) const {
        Item item;
        item.itemLevel = monsterLevel;
        item.slot = randomSlot();
        item.rarity = randomRarity(monsterLevel);

        const int affixCount = affixCountFor(item.rarity);
        const int tier = tierForLevel(monsterLevel) + 1;
        std::vector<ItemAffix> prefixes;
        std::vector<ItemAffix> suffixes;
        std::set<std::size_t> usedIndices;
        for (int i = 0; i < affixCount; ++i) {
            const AffixDefinition& affix = randomAffixFor(item.slot, usedIndices);
            applyAffix(item.stats, affix, monsterLevel);
            if (affix.isPrefix) {
                prefixes.push_back({affix.name, tier});
            } else {
                suffixes.push_back({affix.name, tier});
            }
        }

        item.name = makeName(item.slot, prefixes, suffixes);
        item.affixes = std::move(prefixes);
        item.affixes.insert(item.affixes.end(), suffixes.begin(), suffixes.end());

        return item;
    }

    Item generateBossReward(int monsterLevel, BossLootTheme theme) const {
        const int tier = tierForLevel(monsterLevel);
        Item item;
        item.itemLevel = monsterLevel;
        item.rarity = Rarity::Rare;
        item.affixes.push_back({"Boss relic", tier + 1});

        switch (theme) {
            case BossLootTheme::Brimstone:
                item.name = "Colossus's Brand";
                item.slot = EquipmentSlot::Weapon;
                item.stats.damageMultiplier += std::array<float, 3>{0.14f, 0.20f, 0.27f}[tier];
                item.stats.areaDamageMultiplier += std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier];
                item.affixes.push_back({"Brimstone might", tier + 1});
                item.affixes.push_back({"Crushing impact", tier + 1});
                break;

            case BossLootTheme::Storm:
                item.name = "Herald's Signet";
                item.slot = EquipmentSlot::Ring;
                item.stats.attackSpeedMultiplier += std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier];
                item.stats.projectileDamageMultiplier += std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier];
                item.affixes.push_back({"Storm cadence", tier + 1});
                item.affixes.push_back({"Charged projectiles", tier + 1});
                break;

            case BossLootTheme::Brood:
                item.name = "Matriarch's Talisman";
                item.slot = EquipmentSlot::Amulet;
                item.stats.areaDamageMultiplier += std::array<float, 3>{0.08f, 0.12f, 0.16f}[tier];
                item.stats.areaRadiusMultiplier += std::array<float, 3>{0.06f, 0.10f, 0.14f}[tier];
                item.affixes.push_back({"Brood surge", tier + 1});
                item.affixes.push_back({"Expanding nests", tier + 1});
                break;
        }

        return item;
    }

    static Rarity rarityForRoll(int monsterLevel, int roll) {
        const int normalizedLevel = std::max(1, monsterLevel);
        const int rareChance = std::min(32, 8 + normalizedLevel * 4);
        const int magicChance = std::min(58, 30 + normalizedLevel * 3);
        if (roll < rareChance) {
            return Rarity::Rare;
        }
        if (roll < rareChance + magicChance) {
            return Rarity::Magic;
        }
        return Rarity::Normal;
    }

private:
    static const std::vector<AffixDefinition>& affixPool() {
        static const std::vector<AffixDefinition> pool = buildAffixPool();
        return pool;
    }

    static std::vector<AffixDefinition> buildAffixPool() {
        return {
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

            // Armor
            {"Sturdy", true, EquipmentSlot::Armor, AffixStat::MaxHp, {4.0f, 8.0f, 12.0f}},
            {"Reinforced", true, EquipmentSlot::Armor, AffixStat::MaxHp, {3.0f, 6.0f, 9.0f}},
            {"Plated", true, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 5.0f, 8.0f}},
            {"of Vitality", false, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"Guarded", true, EquipmentSlot::Armor, AffixStat::Armor, {1.0f, 2.0f, 3.0f}},
            {"of Bulwark", false, EquipmentSlot::Armor, AffixStat::Armor, {1.0f, 1.0f, 2.0f}},
            {"of Haste", false, EquipmentSlot::Armor, AffixStat::MoveSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Reach", false, EquipmentSlot::Armor, AffixStat::PickupRangeMultiplier, {0.08f, 0.14f, 0.20f}},

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
        };
    }

    static EquipmentSlot randomSlot() {
        switch (std::rand() % 4) {
            case 0: return EquipmentSlot::Weapon;
            case 1: return EquipmentSlot::Armor;
            case 2: return EquipmentSlot::Ring;
            default: return EquipmentSlot::Amulet;
        }
    }

    static Rarity randomRarity(int monsterLevel) {
        return rarityForRoll(monsterLevel, std::rand() % 100);
    }

    static int affixCountFor(Rarity rarity) {
        switch (rarity) {
            case Rarity::Normal: return 1;
            case Rarity::Magic: return 2;
            case Rarity::Rare: return 3;
        }
        return 1;
    }

    static const AffixDefinition& randomAffixFor(EquipmentSlot slot, std::set<std::size_t>& usedIndices) {
        const auto& pool = affixPool();
        std::vector<std::size_t> matching;
        for (std::size_t i = 0; i < pool.size(); ++i) {
            if (pool[i].slot == slot && usedIndices.find(i) == usedIndices.end()) {
                matching.push_back(i);
            }
        }

        if (matching.empty()) {
            for (std::size_t i = 0; i < pool.size(); ++i) {
                if (pool[i].slot == slot) {
                    matching.push_back(i);
                }
            }
        }

        const std::size_t index = matching.empty() ? 0 : matching[static_cast<std::size_t>(std::rand()) % matching.size()];
        usedIndices.insert(index);
        return pool[index];
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

    static void applyAffix(Stats& stats, const AffixDefinition& affix, int monsterLevel) {
        const float value = affix.tiers[static_cast<std::size_t>(tierForLevel(monsterLevel))];
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
        }
    }

    static std::string makeName(EquipmentSlot slot,
        const std::vector<ItemAffix>& prefixes, const std::vector<ItemAffix>& suffixes) {
        std::string name;
        if (!prefixes.empty()) {
            name += prefixes.front().name + " ";
        }
        name += slotName(slot);
        if (!suffixes.empty()) {
            name += " " + suffixes.front().name;
        }
        return name;
    }
};
