#pragma once

#include <optional>
#include <vector>
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "Timer.hpp"

class Weapon {
public:
    Weapon();

    void update(float dt);
    std::optional<Projectile> tryShoot(
        const Vector2& ownerPosition,
        const std::vector<Enemy>& enemies
    );

    void reset();

private:
    Timer cooldown_;
    float range_;
    float projectileSpeed_;
    int damage_;
};
