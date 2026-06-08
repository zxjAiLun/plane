#include "Weapon.hpp"
#include "Config.hpp"

Weapon::Weapon()
    : cooldown_(Config::WeaponCooldown)
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
    const Vector2& targetPosition
) {
    if (!cooldown_.isReady()) {
        return std::nullopt;
    }

    Vector2 direction = (targetPosition - ownerPosition).normalized();
    if (direction.lengthSquared() <= 0.0f) {
        return std::nullopt;
    }

    cooldown_.reset();

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
