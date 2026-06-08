#include "Player.hpp"
#include "Config.hpp"

#include <algorithm>

Player::Player()
    : position_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , baseSpeed_(Config::PlayerSpeed)
    , radius_(Config::PlayerRadius)
    , hp_(Config::PlayerHp)
    , maxHp_(Config::PlayerHp)
    , level_(1)
    , exp_(0)
    , expToNextLevel_(Config::BaseExpToLevel)
    , pendingLevelUps_(0)
    , stats_() {
}

void Player::update(float /*dt*/) {
}

void Player::moveLeft(float dt) {
    position_.x -= baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.x - radius_ < 0.0f) {
        position_.x = radius_;
    }
}

void Player::moveRight(float dt) {
    position_.x += baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.x + radius_ > Config::WindowWidth) {
        position_.x = Config::WindowWidth - radius_;
    }
}

void Player::moveUp(float dt) {
    position_.y -= baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.y - radius_ < 0.0f) {
        position_.y = radius_;
    }
}

void Player::moveDown(float dt) {
    position_.y += baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.y + radius_ > Config::WindowHeight) {
        position_.y = Config::WindowHeight - radius_;
    }
}

void Player::setPosition(const Vector2& position) {
    position_.x = std::clamp(position.x, radius_, Config::WindowWidth - radius_);
    position_.y = std::clamp(position.y, radius_, Config::WindowHeight - radius_);
}

void Player::takeDamage(int damage) {
    hp_ -= damage;
}

bool Player::isDead() const {
    return hp_ <= 0;
}

void Player::gainExp(int amount) {
    exp_ += amount;
    while (exp_ >= expToNextLevel_) {
        exp_ -= expToNextLevel_;
        level_++;
        expToNextLevel_ = Config::BaseExpToLevel + (level_ - 1) * 2;
        ++pendingLevelUps_;
    }
}

bool Player::isLevelUp() const {
    return pendingLevelUps_ > 0;
}

void Player::applyUpgrade(UpgradeType type) {
    switch (type) {
        case UpgradeType::Damage:
            stats_.damageMultiplier += 0.2f;
            break;
        case UpgradeType::FireRate:
            stats_.attackSpeedMultiplier += 0.2f;
            break;
        case UpgradeType::MoveSpeed:
            stats_.moveSpeedMultiplier += 0.1f;
            break;
        case UpgradeType::PickupRange:
            stats_.pickupRangeMultiplier += 0.25f;
            break;
        case UpgradeType::MaxHp:
            maxHp_ += 1;
            hp_ += 1;
            break;
    }
    if (pendingLevelUps_ > 0) {
        --pendingLevelUps_;
    }
}

const Vector2& Player::position() const { return position_; }
float Player::radius() const { return radius_; }
int Player::hp() const { return hp_; }
int Player::maxHp() const { return maxHp_; }
int Player::level() const { return level_; }
int Player::exp() const { return exp_; }
int Player::expToNextLevel() const { return expToNextLevel_; }
const PlayerStats& Player::stats() const { return stats_; }
