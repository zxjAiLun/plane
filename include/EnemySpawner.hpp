#pragma once

#include <optional>
#include "Enemy.hpp"
#include "RandomService.hpp"
#include "Timer.hpp"
#include "Vector2.hpp"

class MapInstance;

class EnemySpawner {
public:
    EnemySpawner();

    void update(float dt);
    std::optional<Enemy> trySpawn(int hp, int contactDamage, EnemyType type = EnemyType::Normal,
        EliteModifier eliteModifier = EliteModifier::None, int fieldPackIndex = -1);
    std::optional<Enemy> trySpawn(
        int hp,
        int contactDamage,
        EnemyType type,
        EliteModifier eliteModifier,
        RandomService& random,
        int fieldPackIndex = -1
    );
    std::optional<Enemy> trySpawnNear(
        const Vector2& playerPosition,
        const Vector2& worldSize,
        const MapInstance& map,
        int hp,
        int contactDamage,
        EnemyType type = EnemyType::Normal,
        EliteModifier eliteModifier = EliteModifier::None,
        int fieldPackIndex = -1
    );
    std::optional<Enemy> trySpawnNear(
        const Vector2& playerPosition,
        const Vector2& worldSize,
        const MapInstance& map,
        int hp,
        int contactDamage,
        EnemyType type,
        EliteModifier eliteModifier,
        RandomService& random,
        int fieldPackIndex = -1
    );

    void setSpawnInterval(float interval);
    void reset();

private:
    float defaultInterval_;
    Timer spawnTimer_;
};
