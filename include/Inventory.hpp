#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Item.hpp"

class Inventory {
public:
    void add(Item item) {
        items_.push_back(std::move(item));
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

private:
    std::vector<Item> items_;
};
