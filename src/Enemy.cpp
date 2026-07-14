#include "Enemy.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"
#include "MapInstance.hpp"

#include <algorithm>

Enemy::Enemy(
    const Vector2& position,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier
)
    : Enemy(position, hp, contactDamage, type, eliteModifier, -1) {
}

Enemy::Enemy(
    const Vector2& position,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    int mapEventIndex
)
    : position_(position)
    , id_(nextId_++)
    , radius_(Config::EnemyRadius * EnemyLibrary::forType(type).radiusMultiplier)
    , hp_(hp)
    , maxHp_(hp)
    , contactDamage_(contactDamage)
    , type_(type)
    , eliteModifier_(type == EnemyType::Elite ? eliteModifier : EliteModifier::None)
    , mapEventIndex_(mapEventIndex)
    , attackCooldownTimer_(0.0f)
    , attackWindupTimer_(0.0f)
    , attackReady_(false)
    , chargeDirection_()
    , chargeTimer_(0.0f)
    , chargeHitConsumed_(false)
    , igniteDamagePerTick_(0)
    , igniteTimer_(0.0f)
    , igniteTickTimer_(0.0f)
    , chillTimer_(0.0f)
    , chillSpeedMultiplier_(1.0f)
    , killRewardClaimed_(false) {
}

void Enemy::update(float dt, const Vector2& targetPosition, const MapInstance& map) {
    update(dt, targetPosition, map, 1.0f);
}

void Enemy::update(
    float dt,
    const Vector2& targetPosition,
    const MapInstance& map,
    float mapSpeedMultiplier
) {
    const float safeMapSpeedMultiplier = std::max(0.0f, mapSpeedMultiplier);
    if (isBoss()) {
        Vector2 direction = (targetPosition - position_).normalized();
        position_ = map.resolveMovement(
            position_, radius_, direction * Config::EnemySpeed * safeMapSpeedMultiplier
                * movementSpeedMultiplier() * dt
        );
        return;
    }

    const auto& definition = EnemyLibrary::forType(type_);
    attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - dt);
    const float speedMultiplier = EliteModifierLibrary::forModifier(eliteModifier_).speedMultiplier
        * safeMapSpeedMultiplier * movementSpeedMultiplier();

    if (chargeTimer_ > 0.0f) {
        const float chargeStep = std::min(dt, chargeTimer_);
        position_ = map.resolveMovement(
            position_, radius_,
            chargeDirection_ * Config::EnemySpeed * definition.chargeSpeedMultiplier * speedMultiplier * chargeStep
        );
        chargeTimer_ = std::max(0.0f, chargeTimer_ - dt);
        if (chargeTimer_ == 0.0f) {
            attackCooldownTimer_ = definition.attackCooldown;
        }
        return;
    }

    if (attackWindupTimer_ > 0.0f) {
        attackWindupTimer_ = std::max(0.0f, attackWindupTimer_ - dt);
        if (attackWindupTimer_ == 0.0f) {
            if (isCharger()) {
                chargeTimer_ = definition.chargeDuration;
                chargeHitConsumed_ = false;
            } else {
                attackReady_ = true;
            }
        }
        return;
    }

    const Vector2 toTarget = targetPosition - position_;
    if (attackCooldownTimer_ <= 0.0f
        && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        if (isCharger()) {
            chargeDirection_ = toTarget.normalized();
            if (chargeDirection_.lengthSquared() <= 0.0f) {
                return;
            }
        }
        attackWindupTimer_ = definition.attackWindup;
        return;
    }

    if (isRanged() && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        return;
    }

    Vector2 direction = (targetPosition - position_).normalized();
    position_ = map.resolveMovement(position_, radius_, direction * Config::EnemySpeed * speedMultiplier * dt);
}

void Enemy::moveBy(const Vector2& delta, const MapInstance& map) {
    position_ = map.resolveMovement(position_, radius_, delta);
}

AilmentTickResult Enemy::updateAilments(float dt) {
    AilmentTickResult result;
    const float elapsed = std::max(0.0f, dt);
    if (igniteTimer_ > 0.0f) {
        const float activeTime = std::min(elapsed, igniteTimer_);
        igniteTimer_ = std::max(0.0f, igniteTimer_ - elapsed);
        igniteTickTimer_ -= activeTime;
        while (igniteTickTimer_ <= 0.0f && igniteTimer_ > 0.0f && !isDead()) {
            result.type = AilmentType::Ignite;
            ++result.tickCount;
            result.damage += takeDamage(igniteDamagePerTick_);
            result.killed = isDead();
            igniteTickTimer_ += Config::AilmentTickInterval;
            if (result.killed) {
                break;
            }
        }
        if (igniteTimer_ <= 0.0f) {
            igniteDamagePerTick_ = 0;
            igniteTickTimer_ = 0.0f;
        }
    }

    chillTimer_ = std::max(0.0f, chillTimer_ - elapsed);
    if (chillTimer_ <= 0.0f) {
        chillSpeedMultiplier_ = 1.0f;
    }

    return result;
}

int Enemy::takeDamage(int damage) {
    if (damage <= 0 || isDead()) {
        return 0;
    }

    const int previousHp = hp_;
    hp_ = std::max(0, hp_ - damage);
    return previousHp - hp_;
}

void Enemy::applyIgnite(int damagePerTick, float duration) {
    if (damagePerTick <= 0 || duration <= 0.0f) {
        return;
    }

    igniteDamagePerTick_ = std::max(igniteDamagePerTick_, damagePerTick);
    igniteTimer_ = std::max(igniteTimer_, duration);
    igniteTickTimer_ = std::min(igniteTickTimer_, Config::AilmentTickInterval);
    if (igniteTickTimer_ <= 0.0f) {
        igniteTickTimer_ = Config::AilmentTickInterval;
    }
}

void Enemy::applyChill(float speedMultiplier, float duration) {
    if (speedMultiplier <= 0.0f || speedMultiplier >= 1.0f || duration <= 0.0f) {
        return;
    }

    chillSpeedMultiplier_ = std::min(chillSpeedMultiplier_, speedMultiplier);
    chillTimer_ = std::max(chillTimer_, duration);
}

void Enemy::kill() {
    hp_ = 0;
}

bool Enemy::isDead() const {
    return hp_ <= 0;
}

bool Enemy::claimKillReward() {
    if (!isDead() || killRewardClaimed_) {
        return false;
    }

    killRewardClaimed_ = true;
    return true;
}

const Vector2& Enemy::position() const { return position_; }
int Enemy::id() const { return id_; }
float Enemy::radius() const { return radius_; }
int Enemy::hp() const { return hp_; }
int Enemy::maxHp() const { return maxHp_; }
int Enemy::contactDamage() const { return contactDamage_; }
float Enemy::attackRange() const { return EnemyLibrary::forType(type_).attackRange; }
bool Enemy::isAttackWindingUp() const { return !isBoss() && attackWindupTimer_ > 0.0f; }
bool Enemy::consumeAttack() {
    if (!attackReady_) {
        return false;
    }

    attackReady_ = false;
    attackCooldownTimer_ = EnemyLibrary::forType(type_).attackCooldown;
    return true;
}
EnemyType Enemy::type() const { return type_; }
bool Enemy::isRanged() const {
    return EnemyLibrary::forType(type_).attackStyle == EnemyAttackStyle::Projectile;
}
bool Enemy::isCharger() const {
    return EnemyLibrary::forType(type_).attackStyle == EnemyAttackStyle::Charge;
}
bool Enemy::isWarden() const { return type_ == EnemyType::Warden; }
bool Enemy::isCharging() const { return chargeTimer_ > 0.0f; }
bool Enemy::isIgnited() const { return igniteTimer_ > 0.0f; }
bool Enemy::isChilled() const { return chillTimer_ > 0.0f; }
float Enemy::movementSpeedMultiplier() const {
    return isChilled() ? chillSpeedMultiplier_ : 1.0f;
}
Vector2 Enemy::chargeTargetPosition(float mapSpeedMultiplier) const {
    if (!isCharger()) {
        return position_;
    }

    const auto& definition = EnemyLibrary::forType(type_);
    const float duration = chargeTimer_ > 0.0f ? chargeTimer_ : definition.chargeDuration;
    return position_ + chargeDirection_ * Config::EnemySpeed
        * definition.chargeSpeedMultiplier * std::max(0.0f, mapSpeedMultiplier) * duration;
}
bool Enemy::consumeChargeHit() {
    if (!isCharging() || chargeHitConsumed_) {
        return false;
    }

    chargeHitConsumed_ = true;
    return true;
}
bool Enemy::isElite() const {
    return type_ == EnemyType::Elite || type_ == EnemyType::Boss || isWarden();
}
bool Enemy::isBoss() const { return type_ == EnemyType::Boss; }
EliteModifier Enemy::eliteModifier() const { return eliteModifier_; }
int Enemy::mapEventIndex() const { return mapEventIndex_; }
