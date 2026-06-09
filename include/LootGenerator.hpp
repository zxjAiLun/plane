#pragma once

#include <cstdlib>
#include <string>
#include <utility>

#include "Item.hpp"

class LootGenerator {
public:
    Item generate(int monsterLevel) const {
        Item item;
        item.itemLevel = monsterLevel;
        item.slot = randomSlot();
        item.rarity = randomRarity(monsterLevel);
        item.name = makeName(item.slot, item.rarity);

        const int affixCount = affixCountFor(item.rarity);
        for (int i = 0; i < affixCount; ++i) {
            item.affixes.push_back(applyRandomAffix(item.stats, monsterLevel));
        }

        if (!item.affixes.empty()) {
            item.name += " " + item.affixes.front();
        }

        return item;
    }

private:
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

    static std::string makeName(EquipmentSlot slot, Rarity rarity) {
        std::string prefix;
        switch (rarity) {
            case Rarity::Normal: prefix = "Plain "; break;
            case Rarity::Magic: prefix = "Gleaming "; break;
            case Rarity::Rare: prefix = "Vicious "; break;
        }
        return prefix + slotName(slot);
    }

    static std::string applyRandomAffix(Stats& stats, int monsterLevel) {
        const int levelBonus = monsterLevel / 3;
        switch (std::rand() % 5) {
            case 0:
                stats.maxHp += 5 + levelBonus;
                return "of Vitality";
            case 1:
                stats.damageMultiplier += 0.10f + monsterLevel * 0.01f;
                return "of Force";
            case 2:
                stats.attackSpeedMultiplier += 0.08f + monsterLevel * 0.005f;
                return "of Swiftness";
            case 3:
                stats.moveSpeedMultiplier += 0.06f + monsterLevel * 0.004f;
                return "of Haste";
            case 4:
                stats.pickupRangeMultiplier += 0.12f + monsterLevel * 0.006f;
                return "of Reach";
        }
        return "of Power";
    }
};
