#include "EnemySpawner.hpp"
#include "Config.hpp"
#include <cstdlib>

EnemySpawner::EnemySpawner()
    : spawnTimer_(Config::EnemySpawnInterval) {
}

void EnemySpawner::update(float dt) {
    spawnTimer_.update(dt);
}

std::optional<Enemy> EnemySpawner::trySpawn() {
    if (spawnTimer_.isReady()) {
        spawnTimer_.reset();

        float x = static_cast<float>(std::rand() % Config::WindowWidth);
        Vector2 position(x, -Config::EnemyRadius);
        Vector2 velocity(0.0f, Config::EnemySpeed);

        return Enemy(position, velocity, Config::EnemyHp);
    }
    return std::nullopt;
}

void EnemySpawner::reset() {
    spawnTimer_.reset();
}
