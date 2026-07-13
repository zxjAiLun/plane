#pragma once

#include <string>
#include <vector>

#include "EquipmentSlot.hpp"
#include "ItemBase.hpp"
#include "Stats.hpp"

enum class Rarity {
    Normal,
    Magic,
    Rare
};

struct ItemAffix {
    std::string name;
    int tier = 1;
    Stats stats;
};

struct Item {
    std::string name;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    Rarity rarity = Rarity::Normal;
    Stats stats;
    int itemLevel = 1;
    int upgradeLevel = 0;
    std::vector<ItemAffix> affixes;
    std::string baseId;
    std::string baseName;
    Stats implicitStats;
};

inline const char* rarityName(Rarity rarity) {
    switch (rarity) {
        case Rarity::Normal: return "Normal";
        case Rarity::Magic: return "Magic";
        case Rarity::Rare: return "Rare";
    }
    return "Unknown";
}

inline const char* slotName(EquipmentSlot slot) {
    switch (slot) {
        case EquipmentSlot::Weapon: return "Weapon";
        case EquipmentSlot::Armor: return "Armor";
        case EquipmentSlot::Ring: return "Ring";
        case EquipmentSlot::Amulet: return "Amulet";
        case EquipmentSlot::Count: break;
    }
    return "Unknown";
}
