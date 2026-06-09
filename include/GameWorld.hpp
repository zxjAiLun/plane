#pragma once

#include <vector>
#include <array>
#include "Player.hpp"
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "ExperienceOrb.hpp"
#include "DroppedItem.hpp"
#include "EnemySpawner.hpp"
#include "Inventory.hpp"
#include "LootGenerator.hpp"
#include "Weapon.hpp"
#include "Upgrade.hpp"
#include "Input.hpp"
#include "Timer.hpp"

enum class GameState {
    Playing,
    LevelUp,
    GameOver,
    MapComplete
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
    const std::vector<DroppedItem>& droppedItems() const;
    const Inventory& inventory() const;
    const std::array<Upgrade, 3>& currentUpgrades() const;
    const Vector2& aimPosition() const;
    float novaEffectProgress() const;

    GameState state() const;
    int score() const;
    float survivalTime() const;
    int mapLevel() const;
    int currentWave() const;
    int enemiesRemainingInWave() const;

private:
    void updatePlaying(float dt, Input& input);
    void updateObjects(float dt);
    void spawnEnemies(float dt);
    void handleCollisions();
    void removeDeadObjects();
    void generateUpgrades();
    void tryDash(Input& input);
    void tryNova(Input& input);
    void trySecondarySkill(Input& input);
    void tryEquipInventoryItem(Input& input);
    void rewardEnemyKill(const Enemy& enemy);
    void advanceWaveIfComplete();
    bool isMapCleared() const;
    int enemiesPerWave() const;

    float currentSpawnInterval() const;

private:
    Player player_;
    std::vector<Projectile> projectiles_;
    std::vector<Enemy> enemies_;
    std::vector<ExperienceOrb> orbs_;
    std::vector<DroppedItem> droppedItems_;
    Inventory inventory_;
    LootGenerator lootGenerator_;
    EnemySpawner spawner_;
    Weapon weapon_;
    Timer dashCooldown_;
    Timer novaCooldown_;
    Timer secondarySkillCooldown_;

    GameState state_;
    int score_;
    float survivalTime_;
    Vector2 aimPosition_;
    float novaEffectTimer_;
    int mapLevel_;
    int currentWave_;
    int enemiesSpawnedInWave_;

    std::array<Upgrade, 3> currentUpgrades_;
};
