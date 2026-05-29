#include "Bullet.hpp"
#include "Config.hpp"

Bullet::Bullet(const Vector2& position, const Vector2& velocity, int damage)
    : position_(position)
    , velocity_(velocity)
    , radius_(Config::BulletRadius)
    , damage_(damage)
    , alive_(true) {
}

void Bullet::update(float dt) {
    position_ += velocity_ * dt;

    if (position_.y + radius_ < 0.0f) {
        alive_ = false;
    }
}

const Vector2& Bullet::position() const { return position_; }
float Bullet::radius() const { return radius_; }
int Bullet::damage() const { return damage_; }

bool Bullet::isAlive() const { return alive_; }
void Bullet::kill() { alive_ = false; }
