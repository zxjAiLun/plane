#include "Player.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Player::Player()
    : position_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , bounds_(Config::MapWidth, Config::MapHeight)
    , baseSpeed_(Config::PlayerSpeed)
    , radius_(Config::PlayerRadius)
    , hp_(Config::PlayerHp)
    , maxHp_(Config::PlayerHp)
    , mana_(Config::PlayerMaxMana)
    , maxMana_(Config::PlayerMaxMana)
    , manaRegenPerSecond_(Config::PlayerManaRegenPerSecond)
    , level_(1)
    , exp_(0)
    , expToNextLevel_(Config::BaseExpToLevel)
    , talentPoints_(0)
    , upgradeStats_()
    , stats_() {
    recalculateStats();
}

void Player::update(float dt) {
    if (dt <= 0.0f || mana_ >= maxMana_) {
        return;
    }

    mana_ = std::min(maxMana_, mana_ + manaRegenPerSecond_ * dt);
}

void Player::moveLeft(float dt) {
    position_.x -= baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.x - radius_ < 0.0f) {
        position_.x = radius_;
    }
}

void Player::moveRight(float dt) {
    position_.x += baseSpeed_ * stats_.moveSpeedMultiplier * dt;
    if (position_.x + radius_ > bounds_.x) {
        position_.x = bounds_.x - radius_;
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
    if (position_.y + radius_ > bounds_.y) {
        position_.y = bounds_.y - radius_;
    }
}

void Player::setPosition(const Vector2& position) {
    position_.x = std::clamp(position.x, radius_, bounds_.x - radius_);
    position_.y = std::clamp(position.y, radius_, bounds_.y - radius_);
}

void Player::setBounds(const Vector2& bounds) {
    bounds_ = bounds;
    setPosition(position_);
}

int Player::takeDamage(int damage) {
    const int actualDamage = mitigatedDamage(damage, stats_.armor);
    hp_ -= actualDamage;
    return actualDamage;
}

int Player::heal(int amount) {
    if (amount <= 0 || isDead() || hp_ >= maxHp_) {
        return 0;
    }

    const int hpBefore = hp_;
    hp_ = std::min(maxHp_, hp_ + amount);
    return hp_ - hpBefore;
}

bool Player::canSpendMana(float amount) const {
    return amount <= 0.0f || mana_ >= amount;
}

bool Player::spendMana(float amount) {
    if (amount <= 0.0f) {
        return true;
    }
    if (!canSpendMana(amount)) {
        return false;
    }

    mana_ = std::clamp(mana_ - amount, 0.0f, maxMana_);
    return true;
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
        ++talentPoints_;
    }
}

void Player::applyUpgrade(UpgradeType type) {
    int maxHpBefore = maxHp_;

    switch (type) {
        case UpgradeType::Damage:
            upgradeStats_.damageMultiplier += 0.2f;
            break;
        case UpgradeType::FireRate:
            upgradeStats_.attackSpeedMultiplier += 0.2f;
            break;
        case UpgradeType::MoveSpeed:
            upgradeStats_.moveSpeedMultiplier += 0.1f;
            break;
        case UpgradeType::PickupRange:
            upgradeStats_.pickupRangeMultiplier += 0.25f;
            break;
        case UpgradeType::MaxHp:
            upgradeStats_.maxHp += 1;
            break;
    }

    recalculateStats();
    hp_ += std::max(0, maxHp_ - maxHpBefore);
}

bool Player::canSpendTalentPoint() const {
    return talentPoints_ > 0;
}

bool Player::spendPassivePoint(std::size_t nodeIndex) {
    if (!canSpendTalentPoint()) {
        return false;
    }

    const int maxHpBefore = maxHp_;
    if (!passiveTree_.allocate(nodeIndex)) {
        return false;
    }

    --talentPoints_;
    recalculateStats();
    hp_ += std::max(0, maxHp_ - maxHpBefore);
    return true;
}

bool Player::canEquipItem(const Item& item) const {
    return Equipment::canEquip(item, level_);
}

std::optional<int> Player::requiredLevelForItem(const Item& item) const {
    return Equipment::requiredLevelFor(item);
}

std::optional<Item> Player::equipItem(Item item) {
    const int maxHpBefore = maxHp_;
    std::optional<Item> replaced = equipment_.equip(std::move(item));
    recalculateStats();

    const int maxHpDelta = maxHp_ - maxHpBefore;
    if (maxHpDelta > 0) {
        hp_ += maxHpDelta;
    } else if (hp_ > maxHp_) {
        hp_ = maxHp_;
    }

    return replaced;
}

PlayerSaveState Player::saveState() const {
    return {
        position_,
        hp_,
        mana_,
        level_,
        exp_,
        expToNextLevel_,
        talentPoints_,
        upgradeStats_,
        passiveTree_.allocatedNodes(),
        equipment_.items()
    };
}

bool Player::restoreState(const PlayerSaveState& state, const Vector2& bounds) {
    const auto validPositiveMultiplier = [](float value) {
        return std::isfinite(value) && value > 0.0f;
    };
    const auto validResistance = [](int value) {
        return value >= 0 && value <= 100;
    };
    if (!std::isfinite(state.position.x) || !std::isfinite(state.position.y)
        || state.level < 1 || state.exp < 0 || state.expToNextLevel < 1
        || state.talentPoints < 0 || state.hp < 0
        || !std::isfinite(state.mana) || state.mana < 0.0f || state.mana > maxMana_
        || !validPositiveMultiplier(state.upgradeStats.moveSpeedMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.damageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.attackSpeedMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.pickupRangeMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.projectileDamageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.areaDamageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.areaRadiusMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.lifeFlaskEffectMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.itemQuantityMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.incomingDamageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.fireDamageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.coldDamageMultiplier)
        || !validPositiveMultiplier(state.upgradeStats.lightningDamageMultiplier)
        || !validResistance(state.upgradeStats.fireResistance)
        || !validResistance(state.upgradeStats.coldResistance)
        || !validResistance(state.upgradeStats.lightningResistance)) {
        return false;
    }

    Player restored = *this;
    restored.bounds_ = bounds;
    if (!restored.passiveTree_.restoreAllocatedNodes(state.allocatedPassiveNodes)
        || !restored.equipment_.restoreItems(state.equipment)) {
        return false;
    }

    restored.position_ = state.position;
    restored.upgradeStats_ = state.upgradeStats;
    restored.level_ = state.level;
    restored.exp_ = state.exp;
    restored.expToNextLevel_ = state.expToNextLevel;
    restored.talentPoints_ = state.talentPoints;
    restored.mana_ = std::clamp(state.mana, 0.0f, restored.maxMana_);
    restored.recalculateStats();
    if (state.hp > restored.maxHp_) {
        return false;
    }
    restored.hp_ = state.hp;
    restored.setPosition(state.position);
    *this = std::move(restored);
    return true;
}

void Player::recalculateStats() {
    stats_ = combineStats(upgradeStats_, passiveTree_.combinedStats());
    stats_ = combineStats(stats_, equipment_.combinedStats());
    maxHp_ = Config::PlayerHp + stats_.maxHp;
    if (hp_ > maxHp_) {
        hp_ = maxHp_;
    }
}

const Vector2& Player::position() const { return position_; }
float Player::radius() const { return radius_; }
float Player::moveSpeed() const { return baseSpeed_ * stats_.moveSpeedMultiplier; }
int Player::hp() const { return hp_; }
int Player::maxHp() const { return maxHp_; }
int Player::level() const { return level_; }
int Player::exp() const { return exp_; }
int Player::expToNextLevel() const { return expToNextLevel_; }
int Player::talentPoints() const { return talentPoints_; }
float Player::mana() const { return mana_; }
float Player::maxMana() const { return maxMana_; }
float Player::manaRegenPerSecond() const { return manaRegenPerSecond_; }
const PlayerStats& Player::stats() const { return stats_; }
const Equipment& Player::equipment() const { return equipment_; }
const PassiveTree& Player::passiveTree() const { return passiveTree_; }
