#pragma once

#include <utility>

#include "Config.hpp"
#include "Item.hpp"
#include "Vector2.hpp"

class DroppedItem {
public:
    DroppedItem(const Vector2& position, Item item)
        : position_(position)
        , item_(std::move(item))
        , radius_(Config::ItemDropRadius)
        , collected_(false) {
    }

    const Vector2& position() const { return position_; }
    float radius() const { return radius_; }
    const Item& item() const { return item_; }

    bool isCollected() const { return collected_; }

    Item collect() {
        collected_ = true;
        return std::move(item_);
    }

private:
    Vector2 position_;
    Item item_;
    float radius_;
    bool collected_;
};
