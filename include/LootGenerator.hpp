#pragma once

#include <array>
#include <cstdlib>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Item.hpp"

enum class AffixStat {
    MaxHp,
    DamageMultiplier,
    AttackSpeedMultiplier,
    MoveSpeedMultiplier,
    PickupRangeMultiplier
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
        std::vector<std::string> prefixes;
        std::vector<std::string> suffixes;
        std::set<std::size_t> usedIndices;
        for (int i = 0; i < affixCount; ++i) {
            const AffixDefinition& affix = randomAffixFor(item.slot, usedIndices);
            applyAffix(item.stats, affix, monsterLevel);
            if (affix.isPrefix) {
                prefixes.push_back(affix.name);
            } else {
                suffixes.push_back(affix.name);
            }
        }

        item.name = makeName(item.slot, item.rarity, prefixes, suffixes);
        item.affixes = suffixes;
        if (!prefixes.empty()) {
            item.affixes.insert(item.affixes.begin(), prefixes.begin(), prefixes.end());
        }

        return item;
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

            // Armor
            {"Sturdy", true, EquipmentSlot::Armor, AffixStat::MaxHp, {4.0f, 8.0f, 12.0f}},
            {"Reinforced", true, EquipmentSlot::Armor, AffixStat::MaxHp, {3.0f, 6.0f, 9.0f}},
            {"Plated", true, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 5.0f, 8.0f}},
            {"of Vitality", false, EquipmentSlot::Armor, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"of Haste", false, EquipmentSlot::Armor, AffixStat::MoveSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"of Reach", false, EquipmentSlot::Armor, AffixStat::PickupRangeMultiplier, {0.08f, 0.14f, 0.20f}},

            // Ring
            {"Glinting", true, EquipmentSlot::Ring, AffixStat::DamageMultiplier, {0.05f, 0.09f, 0.13f}},
            {"Agile", true, EquipmentSlot::Ring, AffixStat::AttackSpeedMultiplier, {0.04f, 0.07f, 0.10f}},
            {"Runner's", true, EquipmentSlot::Ring, AffixStat::MoveSpeedMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Vitality", false, EquipmentSlot::Ring, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"of Swiftness", false, EquipmentSlot::Ring, AffixStat::AttackSpeedMultiplier, {0.03f, 0.06f, 0.09f}},
            {"of Haste", false, EquipmentSlot::Ring, AffixStat::MoveSpeedMultiplier, {0.03f, 0.06f, 0.09f}},

            // Amulet
            {"Blessed", true, EquipmentSlot::Amulet, AffixStat::MaxHp, {3.0f, 6.0f, 9.0f}},
            {"Radiant", true, EquipmentSlot::Amulet, AffixStat::DamageMultiplier, {0.06f, 0.10f, 0.14f}},
            {"Gilded", true, EquipmentSlot::Amulet, AffixStat::PickupRangeMultiplier, {0.10f, 0.16f, 0.22f}},
            {"of Vitality", false, EquipmentSlot::Amulet, AffixStat::MaxHp, {2.0f, 4.0f, 6.0f}},
            {"of Haste", false, EquipmentSlot::Amulet, AffixStat::MoveSpeedMultiplier, {0.05f, 0.08f, 0.11f}},
            {"of Reach", false, EquipmentSlot::Amulet, AffixStat::PickupRangeMultiplier, {0.08f, 0.12f, 0.16f}},
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
        const int roll = std::rand() % 100;
        const int rareChance = 10 + monsterLevel / 3;
        const int magicChance = 35 + monsterLevel / 4;
        if (roll < rareChance) {
            return Rarity::Rare;
        }
        if (roll < rareChance + magicChance) {
            return Rarity::Magic;
        }
        return Rarity::Normal;
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
        if (monsterLevel <= 1) {
            return 0;
        }
        if (monsterLevel <= 3) {
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
        }
    }

    static std::string makeName(EquipmentSlot slot, Rarity rarity,
        const std::vector<std::string>& prefixes, const std::vector<std::string>& suffixes) {
        std::string name;
        if (!prefixes.empty()) {
            name += prefixes.front() + " ";
        }
        name += slotName(slot);
        if (!suffixes.empty()) {
            name += " " + suffixes.front();
        }
        return name;
    }
};
