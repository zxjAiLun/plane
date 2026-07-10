#pragma once

#include <vector>

#include "Vector2.hpp"

class Projectile {
public:
    Projectile(const Vector2& position, const Vector2& velocity, int damage, int pierceCount = 0);

    void update(float dt, const Vector2& worldSize);

    const Vector2& position() const;
    float radius() const;
    int damage() const;
    bool hasHitEnemy(int enemyId) const;
    void recordEnemyHit(int enemyId);

    bool isAlive() const;
    void kill();

private:
    Vector2 position_;
    Vector2 velocity_;
    float radius_;
    int damage_;
    int remainingPierces_;
    std::vector<int> hitEnemyIds_;
    bool alive_;
};
