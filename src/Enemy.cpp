#include "Enemy.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"

#include <algorithm>

Enemy::Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type)
    : position_(position)
    , id_(nextId_++)
    , radius_(Config::EnemyRadius * EnemyLibrary::forType(type).radiusMultiplier)
    , hp_(hp)
    , maxHp_(hp)
    , contactDamage_(contactDamage)
    , type_(type)
    , attackCooldownTimer_(0.0f)
    , attackWindupTimer_(0.0f)
    , attackReady_(false) {
}

void Enemy::update(float dt, const Vector2& targetPosition) {
    if (isBoss()) {
        Vector2 direction = (targetPosition - position_).normalized();
        position_ += direction * Config::EnemySpeed * dt;
        return;
    }

    const auto& definition = EnemyLibrary::forType(type_);
    attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - dt);

    if (attackWindupTimer_ > 0.0f) {
        attackWindupTimer_ = std::max(0.0f, attackWindupTimer_ - dt);
        if (attackWindupTimer_ == 0.0f) {
            attackReady_ = true;
        }
        return;
    }

    const Vector2 toTarget = targetPosition - position_;
    if (attackCooldownTimer_ <= 0.0f
        && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        attackWindupTimer_ = definition.attackWindup;
        return;
    }

    if (isRanged() && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        return;
    }

    Vector2 direction = (targetPosition - position_).normalized();
    position_ += direction * Config::EnemySpeed * dt;
}

void Enemy::takeDamage(int damage) {
    hp_ -= damage;
}

void Enemy::kill() {
    hp_ = 0;
}

bool Enemy::isDead() const {
    return hp_ <= 0;
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
bool Enemy::isElite() const { return type_ == EnemyType::Elite || type_ == EnemyType::Boss; }
bool Enemy::isBoss() const { return type_ == EnemyType::Boss; }
