#include "Enemy.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"

Enemy::Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type)
    : position_(position)
    , radius_(Config::EnemyRadius * EnemyLibrary::forType(type).radiusMultiplier)
    , hp_(hp)
    , maxHp_(hp)
    , contactDamage_(contactDamage)
    , type_(type) {
}

void Enemy::update(float dt, const Vector2& targetPosition) {
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
float Enemy::radius() const { return radius_; }
int Enemy::hp() const { return hp_; }
int Enemy::maxHp() const { return maxHp_; }
int Enemy::contactDamage() const { return contactDamage_; }
EnemyType Enemy::type() const { return type_; }
bool Enemy::isElite() const { return type_ == EnemyType::Elite || type_ == EnemyType::Boss; }
bool Enemy::isBoss() const { return type_ == EnemyType::Boss; }
