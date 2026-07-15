#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "Ailment.hpp"
#include "DamageType.hpp"
#include "Vector2.hpp"

enum class GroundHazardTarget {
    Player,
    Enemies,
    Both
};

struct GroundHazardDefinition {
    std::string source;
    float radius = 0.0f;
    float duration = 0.0f;
    float tickInterval = 0.0f;
    int damage = 0;
    DamageType damageType = DamageType::Physical;
    AilmentDefinition ailment;
    GroundHazardTarget target = GroundHazardTarget::Player;

    bool isValid() const {
        return !source.empty()
            && radius > 0.0f
            && duration > 0.0f
            && tickInterval > 0.0f
            && damage > 0;
    }
};

class GroundHazard {
public:
    GroundHazard(const Vector2& position, GroundHazardDefinition definition)
        : position_(position)
        , definition_(std::move(definition))
        , timeRemaining_(std::max(0.0f, definition_.duration))
        , tickTimer_(std::max(0.0f, definition_.tickInterval)) {
    }

    int update(float dt) {
        if (!isActive() || dt <= 0.0f) {
            return 0;
        }

        const float activeTime = std::min(dt, timeRemaining_);
        timeRemaining_ = std::max(0.0f, timeRemaining_ - activeTime);
        tickTimer_ -= activeTime;

        int elapsedTicks = 0;
        while (tickTimer_ <= 0.0f) {
            ++elapsedTicks;
            tickTimer_ += definition_.tickInterval;
        }
        return elapsedTicks;
    }

    bool isActive() const {
        return definition_.isValid() && timeRemaining_ > 0.0f;
    }

    const Vector2& position() const { return position_; }
    const GroundHazardDefinition& definition() const { return definition_; }
    float timeRemaining() const { return timeRemaining_; }
    float tickTimeRemaining() const { return std::max(0.0f, tickTimer_); }

private:
    Vector2 position_;
    GroundHazardDefinition definition_;
    float timeRemaining_;
    float tickTimer_;
};
