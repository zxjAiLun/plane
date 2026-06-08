#pragma once

#include <optional>
#include "Enemy.hpp"
#include "Timer.hpp"

class EnemySpawner {
public:
    EnemySpawner();

    void update(float dt);
    std::optional<Enemy> trySpawn();

    void setSpawnInterval(float interval);
    void reset();

private:
    float defaultInterval_;
    Timer spawnTimer_;
};
