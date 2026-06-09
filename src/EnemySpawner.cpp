#include "EnemySpawner.hpp"
#include "Config.hpp"
#include <cstdlib>

EnemySpawner::EnemySpawner()
    : defaultInterval_(Config::EnemySpawnInterval)
    , spawnTimer_(Config::EnemySpawnInterval) {
}

void EnemySpawner::update(float dt) {
    spawnTimer_.update(dt);
}

void EnemySpawner::setSpawnInterval(float interval) {
    spawnTimer_.setDuration(interval);
}

std::optional<Enemy> EnemySpawner::trySpawn(int hp, int contactDamage) {
    if (spawnTimer_.isReady()) {
        spawnTimer_.reset();

        float x = 0.0f;
        float y = 0.0f;

        int edge = std::rand() % 4;
        switch (edge) {
            case 0: // top
                x = static_cast<float>(std::rand() % Config::WindowWidth);
                y = -Config::EnemyRadius;
                break;
            case 1: // bottom
                x = static_cast<float>(std::rand() % Config::WindowWidth);
                y = Config::WindowHeight + Config::EnemyRadius;
                break;
            case 2: // left
                x = -Config::EnemyRadius;
                y = static_cast<float>(std::rand() % Config::WindowHeight);
                break;
            case 3: // right
                x = Config::WindowWidth + Config::EnemyRadius;
                y = static_cast<float>(std::rand() % Config::WindowHeight);
                break;
        }

        Vector2 position(x, y);

        return Enemy(position, hp, contactDamage);
    }
    return std::nullopt;
}

void EnemySpawner::reset() {
    spawnTimer_.setDuration(defaultInterval_);
    spawnTimer_.reset();
}
