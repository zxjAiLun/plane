#include "EnemySpawner.hpp"
#include "Config.hpp"
#include "MapInstance.hpp"
#include <algorithm>
#include <cmath>

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

std::optional<Enemy> EnemySpawner::trySpawn(
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    int fieldPackIndex,
    EliteModifier secondaryEliteModifier,
    bool rare,
    const std::string& displayName,
    float rewardDropMultiplier,
    int bonusDropCount,
    int rewardExperienceMultiplier
) {
    return trySpawn(
        hp, contactDamage, type, eliteModifier, RandomService::legacy(), fieldPackIndex,
        secondaryEliteModifier, rare, displayName, rewardDropMultiplier,
        bonusDropCount, rewardExperienceMultiplier
    );
}

std::optional<Enemy> EnemySpawner::trySpawn(
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    RandomService& random,
    int fieldPackIndex,
    EliteModifier secondaryEliteModifier,
    bool rare,
    const std::string& displayName,
    float rewardDropMultiplier,
    int bonusDropCount,
    int rewardExperienceMultiplier
) {
    if (spawnTimer_.isReady()) {
        spawnTimer_.reset();

        float x = 0.0f;
        float y = 0.0f;

        const int edge = random.nextInt(0, 3);
        switch (edge) {
            case 0: // top
                x = static_cast<float>(random.nextInt(0, Config::WindowWidth - 1));
                y = -Config::EnemyRadius;
                break;
            case 1: // bottom
                x = static_cast<float>(random.nextInt(0, Config::WindowWidth - 1));
                y = Config::WindowHeight + Config::EnemyRadius;
                break;
            case 2: // left
                x = -Config::EnemyRadius;
                y = static_cast<float>(random.nextInt(0, Config::WindowHeight - 1));
                break;
            case 3: // right
                x = Config::WindowWidth + Config::EnemyRadius;
                y = static_cast<float>(random.nextInt(0, Config::WindowHeight - 1));
                break;
        }

        Vector2 position(x, y);

        return Enemy(
            position,
            hp,
            contactDamage,
            type,
            eliteModifier,
            -1,
            false,
            fieldPackIndex,
            secondaryEliteModifier,
            rare,
            displayName,
            rewardDropMultiplier,
            bonusDropCount,
            rewardExperienceMultiplier
        );
    }
    return std::nullopt;
}

std::optional<Enemy> EnemySpawner::trySpawnNear(
    const Vector2& playerPosition,
    const Vector2& worldSize,
    const MapInstance& map,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    int fieldPackIndex,
    EliteModifier secondaryEliteModifier,
    bool rare,
    const std::string& displayName,
    float rewardDropMultiplier,
    int bonusDropCount,
    int rewardExperienceMultiplier
) {
    return trySpawnNear(
        playerPosition,
        worldSize,
        map,
        hp,
        contactDamage,
        type,
        eliteModifier,
        RandomService::legacy(),
        fieldPackIndex,
        secondaryEliteModifier,
        rare,
        displayName,
        rewardDropMultiplier,
        bonusDropCount,
        rewardExperienceMultiplier
    );
}

std::optional<Enemy> EnemySpawner::trySpawnNear(
    const Vector2& playerPosition,
    const Vector2& worldSize,
    const MapInstance& map,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    RandomService& random,
    int fieldPackIndex,
    EliteModifier secondaryEliteModifier,
    bool rare,
    const std::string& displayName,
    float rewardDropMultiplier,
    int bonusDropCount,
    int rewardExperienceMultiplier
) {
    if (!spawnTimer_.isReady()) {
        return std::nullopt;
    }

    spawnTimer_.reset();

    constexpr float twoPi = 6.28318530718f;
    for (int attempt = 0; attempt < 8; ++attempt) {
        const float angle = random.nextFloat01() * twoPi;
        const int distanceRange = static_cast<int>(
            Config::EnemySpawnMaxDistance - Config::EnemySpawnMinDistance
        );
        const float distance = Config::EnemySpawnMinDistance
            + static_cast<float>(random.nextInt(0, std::max(0, distanceRange - 1)));
        const Vector2 offset(std::cos(angle) * distance, std::sin(angle) * distance);
        Vector2 position = playerPosition + offset;

        position.x = std::clamp(position.x, Config::EnemyRadius, worldSize.x - Config::EnemyRadius);
        position.y = std::clamp(position.y, Config::EnemyRadius, worldSize.y - Config::EnemyRadius);

        Enemy enemy(
            position,
            hp,
            contactDamage,
            type,
            eliteModifier,
            -1,
            false,
            fieldPackIndex,
            secondaryEliteModifier,
            rare,
            displayName,
            rewardDropMultiplier,
            bonusDropCount,
            rewardExperienceMultiplier
        );
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
