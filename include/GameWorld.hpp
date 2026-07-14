#pragma once

#include <vector>
#include <array>
#include <map>
#include <set>
#include <string>
#include <filesystem>
#include "BossDefinition.hpp"
#include "BossRelicEffect.hpp"
#include "CombatFeedback.hpp"
#include "Crafting.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include "Enemy.hpp"
#include "DroppedItem.hpp"
#include "EnemySpawner.hpp"
#include "Inventory.hpp"
#include "LootGenerator.hpp"
#include "MapInstance.hpp"
#include "MapModifier.hpp"
#include "MapRewardLibrary.hpp"
#include "RandomService.hpp"
#include "SaveData.hpp"
#include "SaveService.hpp"
#include "SkillBar.hpp"
#include "Stash.hpp"
#include "Upgrade.hpp"
#include "Input.hpp"

enum class GameState {
    Playing,
    GameOver,
    MapComplete,
    Paused
};

struct BossProjectile {
    Vector2 position;
    Vector2 velocity;
    float radius = 0.0f;
    int damage = 0;
    std::string source;
    DamageType damageType = DamageType::Physical;
    AilmentDefinition ailment;
    bool alive = true;
};

struct EnemyProjectile {
    Vector2 position;
    Vector2 velocity;
    float radius = 0.0f;
    int damage = 0;
    std::string source;
    DamageType damageType = DamageType::Physical;
    AilmentDefinition ailment;
    bool alive = true;
};

struct RunProgression {
    std::set<std::string> unlockedSkills;
    std::set<std::string> unlockedSupports;
    std::map<std::string, int> skillLevels;
    std::map<std::string, int> supportLevels;
    float itemQuantityRewardMultiplier = 1.0f;
    int forgeFragments = 0;
};

class GameWorld {
public:
    explicit GameWorld(std::uint64_t runSeed = RandomService::DefaultSeed);

    void update(float dt, Input& input);
    void reset();
    void reset(std::uint64_t runSeed);
    bool saveRun(const std::filesystem::path& path) const;
    bool loadRun(const std::filesystem::path& path);
    bool quitRequested() const;

    const Player& player() const;
    const std::vector<Projectile>& projectiles() const;
    const std::vector<BossProjectile>& bossProjectiles() const;
    const std::vector<EnemyProjectile>& enemyProjectiles() const;
    const std::vector<Enemy>& enemies() const;
    const std::vector<CombatFeedback>& combatFeedback() const;
    const std::vector<GroundHazard>& groundHazards() const;
    const std::vector<DroppedItem>& droppedItems() const;
    const Inventory& inventory() const;
    const Stash& stash() const;
    float inventoryFullPromptTimeRemaining() const;
    int selectedInventoryIndex() const;
    int selectedStashIndex() const;
    bool stashSelectionActive() const;
    bool craftingPanelOpen() const;
    CraftingOperation craftingOperation() const;
    int craftingAffixIndex() const;
    int forgeFragments() const;
    int lifeFlaskCharges() const;
    int lifeFlaskMaxCharges() const;
    std::string lifeFlaskStatusMessage() const;
    float lifeFlaskStatusTimeRemaining() const;
    int playerHitDamage() const;
    std::string playerHitSource() const;
    float playerHitEffectProgress() const;
    const Vector2& aimPosition() const;
    float novaEffectProgress() const;
    float novaEffectRadius() const;
    const Vector2& secondarySkillEffectPosition() const;
    float secondarySkillEffectProgress() const;
    float secondarySkillEffectRadius() const;
    const Vector2& dashImpactPosition() const;
    float dashImpactProgress() const;
    float dashImpactRadius() const;
    const Vector2& bossAoeCenter() const;
    float bossAoeRadius() const;
    float bossAoeTelegraphProgress() const;
    float bossAoeEffectProgress() const;
    const Vector2& bossDashStart() const;
    const Vector2& bossDashTarget() const;
    float bossDashTelegraphProgress() const;
    bool bossDashMoving() const;
    const Vector2& bossDashEffectPosition() const;
    float bossDashEffectProgress() const;
    float bossDashRadius() const;
    const Vector2& volatileExplosionCenter() const;
    float volatileExplosionRadius() const;
    float volatileExplosionProgress() const;
    const Vector2& rareLeaderAoeCenter() const;
    float rareLeaderAoeRadius() const;
    float rareLeaderAoeTelegraphProgress() const;
    std::string rareLeaderSkillWarning() const;
    const BossDefinition& bossDefinition() const;
    const SkillBar& skillBar() const;
    AilmentDefinition effectiveSkillAilment(const SkillDefinition& skill) const;
    const MapInstance& map() const;
    MapArea currentMapArea() const;
    float distanceToBoss() const;
    std::string mapObjective() const;
    Vector2 cameraTopLeft() const;
    bool passiveTreeOpen() const;
    bool skillPanelOpen() const;
    int selectedSupportLink() const;
    int hoveredPassiveNode() const;
    std::string passiveBuildSummary() const;
    std::string bossRelicEffectSummary() const;
    bool isSkillUnlocked(const std::string& name) const;
    bool isSupportUnlocked(const std::string& name) const;
    int skillLevel(const std::string& name) const;
    int supportLevel(const std::string& name) const;
    std::string fieldPackName() const;
    std::string fieldPackLeaderName() const;
    std::string fieldPackLeaderDescription() const;
    std::string fieldPackLeaderRewardDescription() const;
    int pendingFieldPackEnemies() const;
    int fieldPackEnemiesRemaining() const;
    int fieldPacksCleared() const;
    int fieldPacksRequired() const;
    bool bossGateUnlocked() const;

    GameState state() const;
    int score() const;
    float survivalTime() const;
    int mapLevel() const;
    std::uint64_t runSeed() const;
    int currentWave() const;
    int enemiesRemainingInWave() const;
    const MapModifier& mapModifier() const;
    int mapKills() const;
    int mapExperienceGained() const;
    int mapItemsDropped() const;
    int mapBossItemsDropped() const;
    int mapItemsPickedUp() const;
    int mapRareLeadersDefeated() const;
    int mapRareLeaderItemsDropped() const;
    std::string lastRareLeaderName() const;
    std::string lastRareLeaderRewardDescription() const;
    std::string nearbyEventPrompt() const;
    int focusedDroppedItemIndex() const;
    std::string pickupPrompt() const;
    float shrineBuffTimeRemaining() const;
    int mapEventsCompleted() const;
    int mapEventsTotal() const;
    std::string eventStatusMessage() const;
    float eventStatusTimeRemaining() const;
    int activeEliteEventEnemiesRemaining() const;
    std::string bossSkillWarning() const;
    bool bossEnraged() const;
    std::string bossPhaseSummary() const;
    bool nextMapOptionChosen() const;
    bool mapRewardChosen() const;
    const MapOption& currentMapOption() const;
    const std::array<MapOption, 3>& nextMapOptions() const;
    int selectedNextMapOption() const;
    const std::array<MapRewardDefinition, 3>& mapRewardOptions() const;
    int selectedMapRewardOption() const;

private:
    void startNextMap();
    void updatePlaying(float dt, Input& input);
    void updatePaused(Input& input);
    void handleEscape();
    SaveData captureSaveData() const;
    bool restoreFromSaveData(const SaveData& data);
    void movePlayerBy(const Vector2& delta);
    void updateObjects(float dt);
    void updateGroundHazards(float dt);
    void updateBossSkills(float dt);
    void updateBossDash(float dt, Enemy& boss);
    void triggerBossEnrage(Enemy& boss);
    int summonBossAdds(const Enemy& boss, const BossSkillDefinition& skill);
    void updateBossProjectiles(float dt);
    void updateEnemyProjectiles(float dt);
    void updateRareLeaderEffects(float dt);
    void spawnEnemies(float dt);
    void handleCollisions();
    int summonEnemyAdds(Enemy& summoner);
    const Enemy* activeRareLeader() const;
    float enemyDamageMultiplier(const Enemy& enemy) const;
    int enemyAttackDamage(const Enemy& enemy) const;
    void resetRareLeaderEffects();
    int damageToEnemy(
        const Enemy& enemy,
        int rawDamage,
        DamageType damageType = DamageType::Physical
    ) const;
    void handleBossProjectileCollisions();
    void handleEnemyProjectileCollisions();
    void removeDeadObjects();
    void addCombatFeedback(
        const Vector2& position,
        int damage,
        const std::string& source,
        CombatFeedbackType type = CombatFeedbackType::Damage
    );
    void addSkillRejectedFeedback(const std::string& source);
    void updateCombatFeedback(float dt);
    void tryCastMovementSkill(Input& input);
    void tryCastUtilitySkill(Input& input);
    void tryCastSecondarySkill(Input& input);
    void tryCastPrimarySkill(Input& input);
    bool tryStartPlayerSkill(SkillSlot slot);
    void tryUseLifeFlask(Input& input);
    void restoreLifeFlaskCharges(int charges, const std::string& source);
    void dealAreaDamage(
        const Vector2& center,
        float radius,
        int damage,
        const AilmentDefinition* ailment = nullptr,
        const std::string& source = "",
        DamageType damageType = DamageType::Physical
    );
    void applySkillAilment(Enemy& enemy, const AilmentDefinition& ailment, int hitDamage);
    void updateMapEvents(float dt, Input& input);
    void triggerElitePackEvent(std::size_t eventIndex);
    void triggerCombinationEvent(std::size_t eventIndex);
    void spawnMapEventEnemies(
        std::size_t eventIndex,
        int eliteCount,
        int normalCount
    );
    void openLootCacheEvent(MapEventInstance& event);
    void activateShrineEvent(MapEventInstance& event);
    void activateGuardedShrineEvent(MapEventInstance& event);
    int dropItemsAround(
        const Vector2& center,
        int count,
        float eventRewardMultiplier = 1.0f,
        const LootBias& extraBias = {}
    );
    int damageForPlayerSkill(const SkillDefinition& skill) const;
    void applySkillProgression();
    float radiusForPlayerSkill(const SkillDefinition& skill) const;
    int pierceCountForPlayerSkill(const SkillDefinition& skill) const;
    int projectileCountForPlayerSkill(const SkillDefinition& skill) const;
    float spreadAngleForPlayerSkill(const SkillDefinition& skill) const;
    AilmentDefinition ailmentForPlayerSkill(const SkillDefinition& skill) const;
    bool hasBossRelicTheme(ItemBaseTheme theme) const;
    void triggerStormChain(
        const Enemy& source,
        int sourceDamage,
        const AilmentDefinition& ailment
    );
    void noteMapEventEnemyDefeated(const Enemy& enemy);
    void tryPickupDroppedItem(Input& input);
    void trySpendPassivePoint(Input& input);
    void updatePassiveTreeHover(const Input& input);
    void tryAssignSkill(Input& input);
    void tryCycleSkillSupport(Input& input);
    void tryEquipInventoryItem(Input& input);
    void trySelectInventoryItem(Input& input);
    void tryDropSelectedInventoryItem(Input& input);
    void trySalvageSelectedInventoryItem(Input& input);
    void tryMoveSelectedInventoryToStash(Input& input);
    void tryMoveSelectedStashToInventory(Input& input);
    void tryToggleCraftingPanel(Input& input);
    void tryCraftSelectedItem(Input& input);
    void applyCraftingOperation();
    void closeCraftingPanel();
    void updateSelectedInventoryIndex();
    void tryChooseMapReward(Input& input);
    void applyMapReward(const MapRewardDefinition& reward);
    void generateMapRewardOptions();
    void tryChooseNextMapOption(Input& input);
    void generateNextMapOptions();
    void initializeRunProgression();
    void rewardEnemyKill(Enemy& enemy);
    void noteFieldPackEnemyDefeated(const Enemy& enemy);
    void spreadPoisonOnDeath(const Enemy& source);
    void damagePlayer(
        int damage,
        const std::string& source,
        DamageType damageType = DamageType::Physical,
        AilmentDefinition ailment = {}
    );
    void applyPlayerAilment(
        const AilmentDefinition& ailment,
        DamageType damageType,
        int hitDamage,
        const Stats& effectiveStats
    );
    Enemy* activeBoss();
    void resetBossDash();
    float bossSkillInterval() const;
    int bossSkillDamage(int baseDamage) const;
    void advanceWaveIfComplete();
    bool isMapCleared() const;
    int enemiesPerWave() const;
    int enemyHpForMap() const;
    int enemyDamageForMap() const;
    int itemLevelForMap() const;
    EnemyType nextMapEnemyType();
    EliteModifier randomEliteModifier();
    bool shouldSpawnBoss() const;
    void triggerBossIfNeeded();

    float currentSpawnInterval() const;

private:
    Player player_;
    std::vector<Projectile> projectiles_;
    std::vector<BossProjectile> bossProjectiles_;
    std::vector<EnemyProjectile> enemyProjectiles_;
    std::vector<Enemy> enemies_;
    std::vector<CombatFeedback> combatFeedback_;
    std::vector<GroundHazard> groundHazards_;
    std::vector<DroppedItem> droppedItems_;
    Inventory inventory_;
    Stash stash_;
    LootGenerator lootGenerator_;
    EnemySpawner spawner_;
    std::vector<EnemyType> pendingFieldPack_;
    int fieldPackSequence_ = 0;
    std::string fieldPackName_;
    bool fieldPackStarted_ = false;
    int activeFieldPackId_ = -1;
    int activeFieldPackSpawnedCount_ = 0;
    int activeFieldPackLeaderIndex_ = -1;
    std::string activeFieldPackLeaderName_;
    std::string activeFieldPackLeaderDescription_;
    std::string activeFieldPackLeaderRewardDescription_;
    EliteModifier activeFieldPackLeaderModifier_ = EliteModifier::None;
    EliteModifier activeFieldPackLeaderSecondaryModifier_ = EliteModifier::None;
    float activeFieldPackLeaderDropMultiplier_ = 1.0f;
    int activeFieldPackLeaderBonusDrops_ = 0;
    int activeFieldPackLeaderExperienceMultiplier_ = 1;
    LootBias activeFieldPackLootBias_;
    LootBias activeFieldPackLeaderLootBias_;
    int activeFieldPackRewardDrops_ = 0;
    SkillBar skillBar_;
    MapInstance map_;
    RunProgression progression_;
    std::uint64_t runSeed_;
    RandomService random_;

    GameState state_;
    GameState resumeState_;
    bool quitRequested_ = false;
    int score_;
    float survivalTime_;
    Vector2 aimPosition_;
    float novaEffectTimer_;
    Vector2 secondarySkillEffectPosition_;
    float secondarySkillEffectTimer_;
    Vector2 dashImpactPosition_;
    float dashImpactTimer_ = 0.0f;
    float dashImpactDuration_ = 0.0f;
    float dashImpactRadius_ = 0.0f;
    Vector2 bossAoeCenter_;
    float bossAoeTelegraphTimer_;
    float bossAoeEffectTimer_;
    Vector2 volatileExplosionCenter_;
    float volatileExplosionTimer_ = 0.0f;
    float volatileExplosionRadius_ = 0.0f;
    int rareLeaderId_ = -1;
    float rareLeaderPulseTimer_ = 0.0f;
    float rareLeaderRegenTimer_ = 0.0f;
    Vector2 rareLeaderAoeCenter_;
    float rareLeaderAoeTelegraphTimer_ = 0.0f;
    float rareLeaderAoeTelegraphDuration_ = 0.0f;
    float rareLeaderAoeRadius_ = 0.0f;
    int rareLeaderAoeDamage_ = 0;
    DamageType rareLeaderAoeDamageType_ = DamageType::Physical;
    AilmentDefinition rareLeaderAoeAilment_;
    std::string rareLeaderAoeName_;
    BossSkillDefinition bossAoeSkill_;
    BossDashState bossDashState_;
    BossSkillDefinition bossDashSkill_;
    Vector2 bossDashEffectPosition_;
    float bossDashEffectTimer_ = 0.0f;
    float bossSkillTimer_;
    int bossSkillIndex_;
    bool bossEnraged_ = false;
    const BossDefinition* bossDefinition_;
    float playerHitCooldown_;
    float playerHitEffectTimer_ = 0.0f;
    int playerHitDamage_ = 0;
    std::string playerHitSource_;
    float skillFailureFeedbackTimer_ = 0.0f;
    std::string lastSkillFailureFeedback_;
    int mapLevel_;
    int currentWave_;
    int enemiesSpawnedInWave_;
    MapOption currentMapOption_;
    std::array<MapOption, 3> nextMapOptions_;
    int selectedNextMapOption_;
    std::array<MapRewardDefinition, 3> mapRewardOptions_;
    int selectedMapRewardOption_;
    MapModifier mapModifier_;
    int mapKills_;
    int mapExperienceGained_;
    int mapItemsDropped_;
    int mapBossItemsDropped_;
    int mapItemsPickedUp_;
    int mapRareLeadersDefeated_;
    int mapRareLeaderItemsDropped_;
    std::string lastRareLeaderName_;
    std::string lastRareLeaderRewardDescription_;
    int fieldPacksCleared_;
    bool mapRewardChosen_;
    bool nextMapOptionChosen_;
    bool passiveTreeOpen_;
    bool skillPanelOpen_;
    int selectedSupportLink_;
    CraftingState craftingState_;
    int hoveredPassiveNode_;
    std::string nearbyEventPrompt_;
    float shrineBuffTimer_;
    int lifeFlaskCharges_;
    std::string lifeFlaskStatusMessage_;
    float lifeFlaskStatusTimer_;
    float inventoryFullTimer_ = 0.0f;
    int selectedInventoryIndex_ = -1;
    int selectedStashIndex_ = -1;
    bool stashSelectionActive_ = false;
    bool mapEventInteractionConsumed_;
    int activeMapEventIndex_;
    int mapEventEnemiesRemaining_;
    std::string eventStatusMessage_;
    float eventStatusTimer_ = 0.0f;
};
