#pragma once

#include <optional>
#include <vector>
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "Timer.hpp"
#include "PlayerStats.hpp"

class Weapon {
public:
    Weapon();

    void update(float dt);
    std::optional<Projectile> tryShoot(
        const Vector2& ownerPosition,
        const std::vector<Enemy>& enemies
    );

    void applyStats(const PlayerStats& stats);
    void reset();

private:
    Timer cooldown_;
    float range_;
    float projectileSpeed_;
    int baseDamage_;
    float cooldownDuration_;
    float damageMultiplier_;
};
