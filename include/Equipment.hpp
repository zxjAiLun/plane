#pragma once

#include <array>
#include <cstddef>

#include "Stats.hpp"

enum class EquipmentSlot {
    Weapon,
    Armor,
    Ring,
    Amulet,
    Count
};

class Equipment {
public:
    void setSlotStats(EquipmentSlot slot, const Stats& stats) {
        slotStats_[slotIndex(slot)] = stats;
    }

    const Stats& slotStats(EquipmentSlot slot) const {
        return slotStats_[slotIndex(slot)];
    }

    Stats combinedStats() const {
        Stats result;
        for (const auto& stats : slotStats_) {
            result = combineStats(result, stats);
        }
        return result;
    }

    void reset() {
        slotStats_.fill(Stats{});
    }

private:
    static constexpr std::size_t slotIndex(EquipmentSlot slot) {
        return static_cast<std::size_t>(slot);
    }

private:
    std::array<Stats, slotIndex(EquipmentSlot::Count)> slotStats_{};
};
