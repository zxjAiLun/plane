#include "Enemy.hpp"
#include "Config.hpp"

Enemy::Enemy(const Vector2& position, const Vector2& velocity, int hp)
    : position_(position)
    , velocity_(velocity)
    , radius_(Config::EnemyRadius)
    , hp_(hp)
    , contactDamage_(Config::EnemyContactDamage) {
}

void Enemy::update(float dt) {
    position_ += velocity_ * dt;
}

void Enemy::takeDamage(int damage) {
    hp_ -= damage;
}

bool Enemy::isDead() const {
    return hp_ <= 0 || position_.y - radius_ > Config::WindowHeight;
}

const Vector2& Enemy::position() const { return position_; }
float Enemy::radius() const { return radius_; }
int Enemy::contactDamage() const { return contactDamage_; }
