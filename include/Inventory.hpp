#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Item.hpp"

class Inventory {
public:
    // Adds the item. Returns false (and leaves the inventory unchanged) when full.
    bool add(Item item) {
        if (isFull()) {
            return false;
        }
        items_.push_back(std::move(item));
        return true;
    }

    std::optional<Item> take(std::size_t index) {
        if (index >= items_.size()) {
            return std::nullopt;
        }

        Item item = std::move(items_[index]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
        return item;
    }

    const std::vector<Item>& items() const {
        return items_;
    }

    void clear() {
        items_.clear();
    }

    std::size_t size() const {
        return items_.size();
    }

    std::size_t capacity() const {
        return static_cast<std::size_t>(Config::InventoryCapacity);
    }

    bool isFull() const {
        return items_.size() >= capacity();
    }

    std::size_t remainingSlots() const {
        return isFull() ? 0 : capacity() - items_.size();
    }

private:
    std::vector<Item> items_;
};
