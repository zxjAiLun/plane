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

    void takeDamage(int damage);
    bool isDead() const;

    void gainExp(int amount);
    void applyUpgrade(UpgradeType type);
    bool canSpendTalentPoint() const;
    bool spendPassivePoint(std::size_t nodeIndex);
    std::optional<Item> equipItem(Item item);

    const Vector2& position() const;
    float radius() const;
    int hp() const;
    int maxHp() const;
    int level() const;
    int exp() const;
    int expToNextLevel() const;
    int talentPoints() const;
    const PlayerStats& stats() const;
    const Equipment& equipment() const;
    const PassiveTree& passiveTree() const;

private:
    void recalculateStats();

private:
    Vector2 position_;
    float baseSpeed_;
    float radius_;
    int hp_;
    int maxHp_;

    int level_;
    int exp_;
    int expToNextLevel_;
    int talentPoints_;

    PlayerStats upgradeStats_;
    PlayerStats stats_;
    Equipment equipment_;
    PassiveTree passiveTree_;
};
