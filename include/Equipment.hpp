#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

#include "EquipmentSlot.hpp"
#include "Item.hpp"
#include "Stats.hpp"

class Equipment {
public:
    std::optional<Item> equip(Item item) {
        auto& equipped = equipped_[slotIndex(item.slot)];
        std::optional<Item> replaced = std::move(equipped);
        equipped = std::move(item);
        return replaced;
    }

    const std::optional<Item>& itemInSlot(EquipmentSlot slot) const {
        return equipped_[slotIndex(slot)];
    }

    Stats combinedStats() const {
        Stats result;
        for (const auto& item : equipped_) {
            if (item) {
                result = combineStats(result, item->stats);
            }
        }
        return result;
    }

    void reset() {
        for (auto& item : equipped_) {
            item.reset();
        }
    }

private:
    static constexpr std::size_t slotIndex(EquipmentSlot slot) {
        return static_cast<std::size_t>(slot);
    }

private:
    std::array<std::optional<Item>, EquipmentSlotCount> equipped_{};
};
