#pragma once

#include <vector>
#include <array>
#include <set>
#include <string>
#include "BossDefinition.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "DroppedItem.hpp"
#include "EnemySpawner.hpp"
#include "Inventory.hpp"
#include "LootGenerator.hpp"
#include "MapInstance.hpp"
#include "MapModifier.hpp"
#include "SkillBar.hpp"
#include "Upgrade.hpp"
#include "Input.hpp"

enum class GameState {
    Playing,
    GameOver,
    MapComplete
};

struct BossProjectile {
    Vector2 position;
    Vector2 velocity;
    float radius = 0.0f;
    int damage = 0;
    bool alive = true;
};

enum class MapRewardType {
    UnlockSkill,
    Damage,
    MaxHp,
    ItemQuantity
};

struct MapRewardOption {
    MapRewardType type = MapRewardType::Damage;
    std::string title;
    std::string description;
    std::string skillName;
    float itemQuantityMultiplierBonus = 1.0f;
};

class GameWorld {
public:
    GameWorld();

    void update(float dt, Input& input);
    void reset();

    const Player& player() const;
    const std::vector<Projectile>& projectiles() const;
    const std::vector<BossProjectile>& bossProjectiles() const;
    const std::vector<Enemy>& enemies() const;
    const std::vector<DroppedItem>& droppedItems() const;
    const Inventory& inventory() const;
    const Vector2& aimPosition() const;
    float novaEffectProgress() const;
    float novaEffectRadius() const;
    const Vector2& secondarySkillEffectPosition() const;
    float secondarySkillEffectProgress() const;
    float secondarySkillEffectRadius() const;
    const Vector2& bossAoeCenter() const;
    float bossAoeRadius() const;
    float bossAoeTelegraphProgress() const;
    float bossAoeEffectProgress() const;
    const BossDefinition& bossDefinition() const;
    const SkillBar& skillBar() const;
    const MapInstance& map() const;
    MapArea currentMapArea() const;
    float distanceToBoss() const;
    std::string mapObjective() const;
    Vector2 cameraTopLeft() const;
    bool passiveTreeOpen() const;
    bool skillPanelOpen() const;
    int hoveredPassiveNode() const;
    std::string passiveBuildSummary() const;
    bool isSkillUnlocked(const std::string& name) const;

    GameState state() const;
    int score() const;
    float survivalTime() const;
    int mapLevel() const;
    int currentWave() const;
    int enemiesRemainingInWave() const;
    const MapModifier& mapModifier() const;
    int mapKills() const;
    int mapExperienceGained() const;
    int mapItemsDropped() const;
    int mapItemsPickedUp() const;
    std::string nearbyEventPrompt() const;
    float shrineBuffTimeRemaining() const;
    int mapEventsCompleted() const;
    int mapEventsTotal() const;
    bool nextMapOptionChosen() const;
    bool mapRewardChosen() const;
    const MapOption& currentMapOption() const;
    const std::array<MapOption, 3>& nextMapOptions() const;
    int selectedNextMapOption() const;
    const std::array<MapRewardOption, 3>& mapRewardOptions() const;
    int selectedMapRewardOption() const;

private:
    void startNextMap();
    void updatePlaying(float dt, Input& input);
    void updateObjects(float dt);
    void updateBossSkills(float dt);
    void updateBossProjectiles(float dt);
    void spawnEnemies(float dt);
    void handleCollisions();
    void handleBossProjectileCollisions();
    void removeDeadObjects();
    void tryCastMovementSkill(Input& input);
    void tryCastUtilitySkill(Input& input);
    void tryCastSecondarySkill(Input& input);
    void tryCastPrimarySkill(Input& input);
    void dealAreaDamage(const Vector2& center, float radius, int damage);
    void updateMapEvents(float dt, Input& input);
    void triggerElitePackEvent(std::size_t eventIndex);
    void openLootCacheEvent(MapEventInstance& event);
    void activateShrineEvent(MapEventInstance& event);
    void dropItemsAround(const Vector2& center, int count);
    int damageForPlayerSkill(const SkillDefinition& skill) const;
    float radiusForPlayerSkill(const SkillDefinition& skill) const;
    void noteElitePackEnemyDefeated(const Enemy& enemy);
    void tryPickupDroppedItem(Input& input);
    void trySpendPassivePoint(Input& input);
    void updatePassiveTreeHover(const Input& input);
    void tryAssignSkill(Input& input);
    void tryEquipInventoryItem(Input& input);
    void tryChooseMapReward(Input& input);
    void applyMapReward(const MapRewardOption& reward);
    void generateMapRewardOptions();
    void tryChooseNextMapOption(Input& input);
    void generateNextMapOptions();
    void initializeUnlockedSkills();
    void rewardEnemyKill(const Enemy& enemy);
    void damagePlayer(int damage);
    const Enemy* activeBoss() const;
    void advanceWaveIfComplete();
    bool isMapCleared() const;
    int enemiesPerWave() const;
    int enemyHpForMap() const;
    int enemyDamageForMap() const;
    bool shouldSpawnBoss() const;
    void triggerBossIfNeeded();

    float currentSpawnInterval() const;

private:
    Player player_;
    std::vector<Projectile> projectiles_;
    std::vector<BossProjectile> bossProjectiles_;
    std::vector<Enemy> enemies_;
    std::vector<DroppedItem> droppedItems_;
    Inventory inventory_;
    LootGenerator lootGenerator_;
    EnemySpawner spawner_;
    SkillBar skillBar_;
    MapInstance map_;
    std::set<std::string> unlockedSkills_;

    GameState state_;
    int score_;
    float survivalTime_;
    Vector2 aimPosition_;
    float novaEffectTimer_;
    Vector2 secondarySkillEffectPosition_;
    float secondarySkillEffectTimer_;
    Vector2 bossAoeCenter_;
    float bossAoeTelegraphTimer_;
    float bossAoeEffectTimer_;
    BossSkillDefinition bossAoeSkill_;
    float bossSkillTimer_;
    int bossSkillIndex_;
    const BossDefinition* bossDefinition_;
    float playerHitCooldown_;
    int mapLevel_;
    int currentWave_;
    int enemiesSpawnedInWave_;
    MapOption currentMapOption_;
    std::array<MapOption, 3> nextMapOptions_;
    int selectedNextMapOption_;
    std::array<MapRewardOption, 3> mapRewardOptions_;
    int selectedMapRewardOption_;
    MapModifier mapModifier_;
    float itemQuantityRewardMultiplier_;
    int mapKills_;
    int mapExperienceGained_;
    int mapItemsDropped_;
    int mapItemsPickedUp_;
    bool mapRewardChosen_;
    bool nextMapOptionChosen_;
    bool passiveTreeOpen_;
    bool skillPanelOpen_;
    int hoveredPassiveNode_;
    std::string nearbyEventPrompt_;
    float shrineBuffTimer_;
    bool mapEventInteractionConsumed_;
    int activeEliteEventIndex_;
    int eliteEventEnemiesRemaining_;
};
