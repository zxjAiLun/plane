#pragma once

#include <string>
#include <vector>

#include "Ailment.hpp"
#include "Vector2.hpp"

class Projectile {
public:
    Projectile(
        const Vector2& position,
        const Vector2& velocity,
        int damage,
        int pierceCount = 0,
        AilmentDefinition ailment = {},
        std::string source = {}
    );

    void update(float dt, const Vector2& worldSize);

    const Vector2& position() const;
    float radius() const;
    int damage() const;
    const AilmentDefinition& ailment() const;
    const std::string& source() const;
    bool hasHitEnemy(int enemyId) const;
    void recordEnemyHit(int enemyId);

    bool isAlive() const;
    void kill();

private:
    Vector2 position_;
    Vector2 velocity_;
    float radius_;
    int damage_;
    AilmentDefinition ailment_;
    std::string source_;
    int remainingPierces_;
    std::vector<int> hitEnemyIds_;
    bool alive_;
};
