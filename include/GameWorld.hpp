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

private:
    Player player_;
    std::vector<Projectile> projectiles_;
    std::vector<Enemy> enemies_;
    std::vector<ExperienceOrb> orbs_;
    EnemySpawner spawner_;
    Weapon weapon_;

    GameState state_;
    int score_;
    float survivalTime_;

    std::array<Upgrade, 3> currentUpgrades_;
};
