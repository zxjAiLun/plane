#include "Weapon.hpp"
#include "Config.hpp"

Weapon::Weapon()
    : cooldown_(Config::WeaponCooldown)
    , range_(Config::WeaponRange)
    , projectileSpeed_(Config::ProjectileSpeed)
    , baseDamage_(Config::ProjectileDamage)
    , cooldownDuration_(Config::WeaponCooldown)
    , damageMultiplier_(1.0f) {
}

void Weapon::update(float dt) {
    cooldown_.update(dt);
}

std::optional<Projectile> Weapon::tryShoot(
    const Vector2& ownerPosition,
    const std::vector<Enemy>& enemies
) {
    if (!cooldown_.isReady()) {
        return std::nullopt;
    }

    const Enemy* nearestEnemy = nullptr;
    float nearestDistSq = range_ * range_;

    for (const auto& enemy : enemies) {
        if (enemy.isDead()) continue;

        Vector2 diff = enemy.position() - ownerPosition;
        float distSq = diff.lengthSquared();

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestEnemy = &enemy;
        }
    }

    if (!nearestEnemy) {
        return std::nullopt;
    }

    cooldown_.reset();

    Vector2 direction = (nearestEnemy->position() - ownerPosition).normalized();
    Vector2 velocity = direction * projectileSpeed_;
    int damage = static_cast<int>(baseDamage_ * damageMultiplier_);

    return Projectile(ownerPosition, velocity, damage);
}

void Weapon::applyStats(const PlayerStats& stats) {
    damageMultiplier_ = stats.damageMultiplier;
    cooldown_.setDuration(cooldownDuration_ / stats.fireRateMultiplier);
}

void Weapon::reset() {
    cooldown_.reset();
    cooldown_.setDuration(cooldownDuration_);
    damageMultiplier_ = 1.0f;
}
