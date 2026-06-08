#pragma once

#include <string>

#include "Equipment.hpp"
#include "Stats.hpp"

enum class Rarity {
    Normal,
    Magic,
    Rare
};

struct Item {
    std::string name;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    Rarity rarity = Rarity::Normal;
    Stats stats;
    int itemLevel = 1;
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
