#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "Ailment.hpp"
#include "DamageType.hpp"
#include "Vector2.hpp"

class PlayerMinion {
public:
    PlayerMinion(
        const Vector2& position,
        int maxHp,
        int damage,
        float lifetime,
        float attackInterval,
        float attackRange,
        DamageType damageType,
        AilmentDefinition ailment,
        int physicalPenetration,
        std::string name
    )
        : position_(position)
        , hp_(std::max(1, maxHp))
        , maxHp_(std::max(1, maxHp))
        , damage_(std::max(1, damage))
        , lifetime_(std::max(0.0f, lifetime))
        , attackInterval_(std::max(0.05f, attackInterval))
        , attackTimer_(std::max(0.05f, attackInterval))
        , attackRange_(std::max(1.0f, attackRange))
        , damageType_(damageType)
        , ailment_(std::move(ailment))
        , physicalPenetration_(std::max(0, physicalPenetration))
        , name_(std::move(name)) {
    }

    void update(float dt) {
        const float elapsed = std::max(0.0f, dt);
        lifetime_ = std::max(0.0f, lifetime_ - elapsed);
        attackTimer_ += elapsed;
    }

    void moveBy(const Vector2& delta) {
        position_ += delta;
    }

    bool canAttack() const {
        return !isDead() && lifetime_ > 0.0f && attackTimer_ >= attackInterval_;
    }

    void consumeAttack() {
        attackTimer_ = 0.0f;
    }

    int takeDamage(int damage) {
        if (damage <= 0 || isDead()) {
            return 0;
        }

        const int previousHp = hp_;
        hp_ = std::max(0, hp_ - damage);
        return previousHp - hp_;
    }

    bool isDead() const { return hp_ <= 0; }
    bool isExpired() const { return lifetime_ <= 0.0f; }
    bool isAlive() const { return !isDead() && !isExpired(); }
    void kill() { hp_ = 0; }

    const Vector2& position() const { return position_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int damage() const { return damage_; }
    float radius() const { return radius_; }
    float attackRange() const { return attackRange_; }
    float lifetime() const { return lifetime_; }
    DamageType damageType() const { return damageType_; }
    const AilmentDefinition& ailment() const { return ailment_; }
    int physicalPenetration() const { return physicalPenetration_; }
    const std::string& name() const { return name_; }

private:
    Vector2 position_;
    int hp_;
    int maxHp_;
    int damage_;
    float lifetime_;
    float attackInterval_;
    float attackTimer_;
    float attackRange_;
    DamageType damageType_;
    AilmentDefinition ailment_;
    int physicalPenetration_;
    std::string name_;
    float radius_ = 14.0f;
};
