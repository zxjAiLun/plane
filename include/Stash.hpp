#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "Item.hpp"

class Stash {
public:
    bool add(const Item& item) {
        if (isFull()) {
            return false;
        }
        items_.push_back(item);
        return true;
    }

    bool add(Item&& item) {
        if (isFull()) {
            return false;
        }
        items_.push_back(std::move(item));
        return true;
    }

    bool insert(std::size_t index, const Item& item) {
        if (isFull() || index > items_.size()) {
            return false;
        }
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), item);
        return true;
    }

    bool insert(std::size_t index, Item&& item) {
        if (isFull() || index > items_.size()) {
            return false;
        }
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), std::move(item));
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
        return static_cast<std::size_t>(Config::StashCapacity);
    }

    bool isFull() const {
        return items_.size() >= capacity();
    }

private:
    std::vector<Item> items_;
};
