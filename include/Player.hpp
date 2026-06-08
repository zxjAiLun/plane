#pragma once

#include "Vector2.hpp"
#include "PlayerStats.hpp"
#include "Upgrade.hpp"

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
    bool isLevelUp() const;
    void applyUpgrade(UpgradeType type);

    const Vector2& position() const;
    float radius() const;
    int hp() const;
    int maxHp() const;
    int level() const;
    int exp() const;
    int expToNextLevel() const;
    const PlayerStats& stats() const;

private:
    Vector2 position_;
    float baseSpeed_;
    float radius_;
    int hp_;
    int maxHp_;

    int level_;
    int exp_;
    int expToNextLevel_;
    int pendingLevelUps_;

    PlayerStats stats_;
};
