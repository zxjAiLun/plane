#pragma once

#include <algorithm>

#include "Vector2.hpp"

struct BossDashDefinition {
    float distance = 0.0f;
    float speed = 0.0f;

    bool isValid() const {
        return distance > 0.0f && speed > 0.0f;
    }
};

enum class BossDashPhase {
    Idle,
    Telegraph,
    Moving,
    Impact
};

class BossDashState {
public:
    void begin(
        const Vector2& start,
        const Vector2& target,
        float telegraphDuration,
        float speed
    ) {
        reset();
        if ((target - start).lengthSquared() <= 0.0001f || speed <= 0.0f) {
            return;
        }

        start_ = start;
        target_ = target;
        telegraphDuration_ = std::max(0.0f, telegraphDuration);
        telegraphTimer_ = telegraphDuration_;
        speed_ = speed;
        phase_ = telegraphTimer_ > 0.0f
            ? BossDashPhase::Telegraph
            : BossDashPhase::Moving;
    }

    Vector2 update(float dt, const Vector2& currentPosition) {
        if (dt <= 0.0f || phase_ == BossDashPhase::Idle || phase_ == BossDashPhase::Impact) {
            return {};
        }

        if (phase_ == BossDashPhase::Telegraph) {
            telegraphTimer_ = std::max(0.0f, telegraphTimer_ - dt);
            if (telegraphTimer_ <= 0.0f) {
                phase_ = BossDashPhase::Moving;
            }
            return {};
        }

        const Vector2 remaining = target_ - currentPosition;
        const float distance = remaining.length();
        if (distance <= 0.01f) {
            phase_ = BossDashPhase::Impact;
            return {};
        }

        const float stepDistance = std::min(distance, speed_ * dt);
        if (stepDistance <= 0.0f) {
            return {};
        }

        if (stepDistance >= distance - 0.01f) {
            phase_ = BossDashPhase::Impact;
        }
        return remaining.normalized() * stepDistance;
    }

    bool consumeHit() {
        if (hitConsumed_
            || (phase_ != BossDashPhase::Moving && phase_ != BossDashPhase::Impact)) {
            return false;
        }

        hitConsumed_ = true;
        return true;
    }

    bool consumeCompletion() {
        if (phase_ != BossDashPhase::Impact) {
            return false;
        }

        phase_ = BossDashPhase::Idle;
        return true;
    }

    void reset() {
        phase_ = BossDashPhase::Idle;
        start_ = {};
        target_ = {};
        telegraphDuration_ = 0.0f;
        telegraphTimer_ = 0.0f;
        speed_ = 0.0f;
        hitConsumed_ = false;
    }

    bool isActive() const {
        return phase_ == BossDashPhase::Telegraph || phase_ == BossDashPhase::Moving;
    }
    bool isTelegraphing() const { return phase_ == BossDashPhase::Telegraph; }
    bool isMoving() const { return phase_ == BossDashPhase::Moving; }
    BossDashPhase phase() const { return phase_; }
    const Vector2& start() const { return start_; }
    const Vector2& target() const { return target_; }
    float telegraphProgress() const {
        return telegraphDuration_ > 0.0f ? telegraphTimer_ / telegraphDuration_ : 0.0f;
    }

private:
    BossDashPhase phase_ = BossDashPhase::Idle;
    Vector2 start_;
    Vector2 target_;
    float telegraphDuration_ = 0.0f;
    float telegraphTimer_ = 0.0f;
    float speed_ = 0.0f;
    bool hitConsumed_ = false;
};
