#pragma once

#include <optional>
#include "Projectile.hpp"
#include "Timer.hpp"
#include "PlayerStats.hpp"

class Weapon {
public:
    Weapon();

    void update(float dt);
    std::optional<Projectile> tryShoot(
        const Vector2& ownerPosition,
        const Vector2& targetPosition
    );

    void applyStats(const PlayerStats& stats);
    void reset();

private:
    Timer cooldown_;
    float projectileSpeed_;
    int baseDamage_;
    float cooldownDuration_;
    float damageMultiplier_;
};
