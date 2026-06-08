#pragma once

#include <vector>
#include <array>
#include "Player.hpp"
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "ExperienceOrb.hpp"
#include "EnemySpawner.hpp"
#include "Weapon.hpp"
#include "Upgrade.hpp"
#include "Input.hpp"
#include "Timer.hpp"

enum class GameState {
    Playing,
    LevelUp,
    GameOver,
    Victory
};

class GameWorld {
public:
    GameWorld();

    void update(float dt, Input& input);
    void reset();

    const Player& player() const;
    const std::vector<Projectile>& projectiles() const;
    const std::vector<Enemy>& enemies() const;
    const std::vector<ExperienceOrb>& orbs() const;
    const std::array<Upgrade, 3>& currentUpgrades() const;
    const Vector2& aimPosition() const;

    GameState state() const;
    int score() const;
    float survivalTime() const;

private:
    void updatePlaying(float dt, Input& input);
    void updateObjects(float dt);
    void spawnEnemies(float dt);
    void handleCollisions();
    void removeDeadObjects();
    void generateUpgrades();
    void tryDash(Input& input);

    float currentSpawnInterval() const;

private:
    Player player_;
    std::vector<Projectile> projectiles_;
    std::vector<Enemy> enemies_;
    std::vector<ExperienceOrb> orbs_;
    EnemySpawner spawner_;
    Weapon weapon_;
    Timer dashCooldown_;

    GameState state_;
    int score_;
    float survivalTime_;
    Vector2 aimPosition_;

    std::array<Upgrade, 3> currentUpgrades_;
};
