#include "Player.hpp"
#include "Config.hpp"

Player::Player()
    : position_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , speed_(Config::PlayerSpeed)
    , radius_(Config::PlayerRadius)
    , hp_(Config::PlayerHp)
    , level_(1)
    , exp_(0)
    , expToNextLevel_(Config::BaseExpToLevel)
    , levelUpPending_(false) {
}

void Player::update(float /*dt*/) {
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
        levelUpPending_ = true;
    }
}

bool Player::isLevelUp() const {
    return levelUpPending_;
}

void Player::confirmLevelUp() {
    levelUpPending_ = false;
}

const Vector2& Player::position() const { return position_; }
float Player::radius() const { return radius_; }
int Player::hp() const { return hp_; }
int Player::level() const { return level_; }
int Player::exp() const { return exp_; }
int Player::expToNextLevel() const { return expToNextLevel_; }
