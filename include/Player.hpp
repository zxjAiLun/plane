#pragma once

#include <optional>

#include "Vector2.hpp"
#include "PlayerStats.hpp"
#include "Upgrade.hpp"
#include "Equipment.hpp"
#include "Item.hpp"
#include "PassiveTree.hpp"

class Player {
public:
    Player();

    void update(float dt);

    void moveLeft(float dt);
    void moveRight(float dt);
    void moveUp(float dt);
    void moveDown(float dt);
    void setPosition(const Vector2& position);
    void setBounds(const Vector2& bounds);

    int takeDamage(int damage);
    int heal(int amount);
    bool canSpendMana(float amount) const;
    bool spendMana(float amount);
    bool isDead() const;

    void gainExp(int amount);
    void applyUpgrade(UpgradeType type);
    bool canSpendTalentPoint() const;
    bool spendPassivePoint(std::size_t nodeIndex);
    std::optional<Item> equipItem(Item item);

    const Vector2& position() const;
    float radius() const;
    float moveSpeed() const;
    int hp() const;
    int maxHp() const;
    int level() const;
    int exp() const;
    int expToNextLevel() const;
    int talentPoints() const;
    float mana() const;
    float maxMana() const;
    float manaRegenPerSecond() const;
    const PlayerStats& stats() const;
    const Equipment& equipment() const;
    const PassiveTree& passiveTree() const;

private:
    void recalculateStats();

private:
    Vector2 position_;
    Vector2 bounds_;
    float baseSpeed_;
    float radius_;
    int hp_;
    int maxHp_;
    float mana_;
    float maxMana_;
    float manaRegenPerSecond_;

    int level_;
    int exp_;
    int expToNextLevel_;
    int talentPoints_;

    PlayerStats upgradeStats_;
    PlayerStats stats_;
    Equipment equipment_;
    PassiveTree passiveTree_;
};
