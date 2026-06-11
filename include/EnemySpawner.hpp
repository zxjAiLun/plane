#pragma once

#include <optional>
#include "Enemy.hpp"
#include "Timer.hpp"
#include "Vector2.hpp"

class EnemySpawner {
public:
    EnemySpawner();

    void update(float dt);
    std::optional<Enemy> trySpawn(int hp, int contactDamage, EnemyType type = EnemyType::Normal);
    std::optional<Enemy> trySpawnNear(
        const Vector2& playerPosition,
        const Vector2& worldSize,
        int hp,
        int contactDamage,
        EnemyType type = EnemyType::Normal
    );

    void setSpawnInterval(float interval);
    void reset();

private:
    float defaultInterval_;
    Timer spawnTimer_;
};
