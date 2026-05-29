#include "Projectile.hpp"
#include "Config.hpp"

Projectile::Projectile(const Vector2& position, const Vector2& velocity, int damage)
    : position_(position)
    , velocity_(velocity)
    , radius_(Config::ProjectileRadius)
    , damage_(damage)
    , alive_(true) {
}

void Projectile::update(float dt) {
    position_ += velocity_ * dt;

    if (position_.y + radius_ < 0.0f
        || position_.y - radius_ > Config::WindowHeight
        || position_.x + radius_ < 0.0f
        || position_.x - radius_ > Config::WindowWidth) {
        alive_ = false;
    }
}

const Vector2& Projectile::position() const { return position_; }
float Projectile::radius() const { return radius_; }
int Projectile::damage() const { return damage_; }

bool Projectile::isAlive() const { return alive_; }
void Projectile::kill() { alive_ = false; }
