#include "Player.hpp"
#include "Config.hpp"

Player::Player()
    : position_(Config::WindowWidth / 2.0f, Config::WindowHeight - 80.0f)
    , speed_(Config::PlayerSpeed)
    , radius_(Config::PlayerRadius)
    , hp_(Config::PlayerHp)
    , shootCooldown_(Config::ShootCooldown) {
}

void Player::update(float dt) {
    shootCooldown_.update(dt);
}

void Player::moveLeft(float dt) {
    position_.x -= speed_ * dt;
    if (position_.x - radius_ < 0.0f) {
        position_.x = radius_;
    }
}

void Player::moveRight(float dt) {
    position_.x += speed_ * dt;
    if (position_.x + radius_ > Config::WindowWidth) {
        position_.x = Config::WindowWidth - radius_;
    }
}

void Player::moveUp(float dt) {
    position_.y -= speed_ * dt;
    if (position_.y - radius_ < 0.0f) {
        position_.y = radius_;
    }
}

void Player::moveDown(float dt) {
    position_.y += speed_ * dt;
    if (position_.y + radius_ > Config::WindowHeight) {
        position_.y = Config::WindowHeight - radius_;
    }
}

std::optional<Bullet> Player::tryShoot() {
    if (shootCooldown_.isReady()) {
        shootCooldown_.reset();
        Vector2 velocity(0.0f, -Config::BulletSpeed);
        return Bullet(position_, velocity, Config::BulletDamage);
    }
    return std::nullopt;
}

void Player::takeDamage(int damage) {
    hp_ -= damage;
}

bool Player::isDead() const {
    return hp_ <= 0;
}

const Vector2& Player::position() const { return position_; }
float Player::radius() const { return radius_; }
int Player::hp() const { return hp_; }
