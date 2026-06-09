#pragma once

#include <cstddef>

enum class EquipmentSlot {
    Weapon,
    Armor,
    Ring,
    Amulet,
    Count
};

constexpr std::size_t EquipmentSlotCount = static_cast<std::size_t>(EquipmentSlot::Count);

