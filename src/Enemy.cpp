#include "Enemy.hpp"
#include "Config.hpp"

Enemy::Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type)
    : position_(position)
    , radius_(type == EnemyType::Elite ? Config::EnemyRadius * 1.45f : Config::EnemyRadius)
    , hp_(hp)
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
    return hp_ <= 0
        || position_.y - radius_ > Config::WindowHeight
        || position_.y + radius_ < 0
        || position_.x - radius_ > Config::WindowWidth
        || position_.x + radius_ < 0;
}

const Vector2& Enemy::position() const { return position_; }
float Enemy::radius() const { return radius_; }
int Enemy::contactDamage() const { return contactDamage_; }
EnemyType Enemy::type() const { return type_; }
bool Enemy::isElite() const { return type_ == EnemyType::Elite; }
