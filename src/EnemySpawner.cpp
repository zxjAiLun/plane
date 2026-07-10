#include "EnemySpawner.hpp"
#include "Config.hpp"
#include "MapInstance.hpp"
#include <algorithm>
#include <cmath>
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

std::optional<Enemy> EnemySpawner::trySpawn(int hp, int contactDamage, EnemyType type) {
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

        return Enemy(position, hp, contactDamage, type);
    }
    return std::nullopt;
}

std::optional<Enemy> EnemySpawner::trySpawnNear(
    const Vector2& playerPosition,
    const Vector2& worldSize,
    const MapInstance& map,
    int hp,
    int contactDamage,
    EnemyType type
) {
    if (!spawnTimer_.isReady()) {
        return std::nullopt;
    }

    spawnTimer_.reset();

    constexpr float twoPi = 6.28318530718f;
    for (int attempt = 0; attempt < 8; ++attempt) {
        const float angle = (static_cast<float>(std::rand() % 6283) / 6283.0f) * twoPi;
        const float distance = Config::EnemySpawnMinDistance
            + static_cast<float>(std::rand() % static_cast<int>(Config::EnemySpawnMaxDistance - Config::EnemySpawnMinDistance));
        const Vector2 offset(std::cos(angle) * distance, std::sin(angle) * distance);
        Vector2 position = playerPosition + offset;

        position.x = std::clamp(position.x, Config::EnemyRadius, worldSize.x - Config::EnemyRadius);
        position.y = std::clamp(position.y, Config::EnemyRadius, worldSize.y - Config::EnemyRadius);

        Enemy enemy(position, hp, contactDamage, type);
        if (!map.intersectsObstacle(enemy.position(), enemy.radius())) {
            return enemy;
        }
    }

    return std::nullopt;
}

void EnemySpawner::reset() {
    spawnTimer_.setDuration(defaultInterval_);
    spawnTimer_.reset();
}
