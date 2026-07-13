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
    static std::optional<int> requiredLevelFor(const Item& item) {
        const auto* base = ItemBaseLibrary::find(item.baseId);
        return base == nullptr ? std::nullopt : std::optional<int>(base->requiredLevel);
    }

    static bool canEquip(const Item& item, int playerLevel) {
        const auto* base = ItemBaseLibrary::find(item.baseId);
        return base != nullptr
            && base->slot == item.slot
            && playerLevel >= base->requiredLevel;
    }

    std::optional<Item> equip(Item item) {
        auto& equipped = equipped_[slotIndex(item.slot)];
        std::optional<Item> replaced = std::move(equipped);
        equipped = std::move(item);
        return replaced;
    }

    const std::optional<Item>& itemInSlot(EquipmentSlot slot) const {
        return equipped_[slotIndex(slot)];
    }

    const std::array<std::optional<Item>, EquipmentSlotCount>& items() const {
        return equipped_;
    }

    bool restoreItems(const std::array<std::optional<Item>, EquipmentSlotCount>& items) {
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (items[index] && static_cast<std::size_t>(items[index]->slot) != index) {
                return false;
            }
        }

        equipped_ = items;
        return true;
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
