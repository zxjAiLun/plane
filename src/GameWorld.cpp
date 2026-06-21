#include "GameWorld.hpp"
#include "Collision.hpp"
#include "Config.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

GameWorld::GameWorld()
    : state_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f)
    , aimPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , novaEffectTimer_(0.0f)
    , secondarySkillEffectPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , secondarySkillEffectTimer_(0.0f)
    , playerHitCooldown_(0.0f)
    , mapLevel_(1)
    , currentWave_(0)
    , enemiesSpawnedInWave_(0)
    , mapModifier_()
    , map_()
    , mapKills_(0)
    , mapExperienceGained_(0)
    , mapItemsDropped_(0)
    , mapItemsPickedUp_(0)
    , mapRewardChosen_(false)
    , mapRewardItemQuantityBonus_(1.0f)
    , passiveTreeOpen_(false) {
    generateMapModifier();
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    skillBar_.applyStats(player_.stats());
}

void GameWorld::update(float dt, Input& input) {
    const Vector2 camera = cameraTopLeft();
    aimPosition_ = Vector2(
        camera.x + static_cast<float>(input.mousePosition().x),
        camera.y + static_cast<float>(input.mousePosition().y)
    );

    switch (state_) {
        case GameState::Playing:
            updatePlaying(dt, input);
            break;

        case GameState::GameOver:
            if (input.restart()) {
                reset();
            }
            break;

        case GameState::MapComplete:
            tryPickupDroppedItem(input);
            removeDeadObjects();
            tryChooseMapReward(input);
            if (mapRewardChosen_ && input.nextMap()) {
                startNextMap();
            } else if (input.restart()) {
                reset();
            }
            break;
    }

    input.update();
}

void GameWorld::updatePlaying(float dt, Input& input) {
    if (input.passiveTreeToggle()) {
        passiveTreeOpen_ = !passiveTreeOpen_;
    }

    if (input.moveLeft()) player_.moveLeft(dt);
    if (input.moveRight()) player_.moveRight(dt);
    if (input.moveUp()) player_.moveUp(dt);
    if (input.moveDown()) player_.moveDown(dt);

    player_.update(dt);
    skillBar_.update(dt);
    novaEffectTimer_ = std::max(0.0f, novaEffectTimer_ - dt);
    secondarySkillEffectTimer_ = std::max(0.0f, secondarySkillEffectTimer_ - dt);
    playerHitCooldown_ = std::max(0.0f, playerHitCooldown_ - dt);
    tryCastMovementSkill(input);
    tryCastUtilitySkill(input);
    tryCastSecondarySkill(input);
    tryPickupDroppedItem(input);
    trySpendPassivePoint(input);
    if (!passiveTreeOpen_) {
        tryEquipInventoryItem(input);
    }
    tryCastPrimarySkill(input);

    spawnEnemies(dt);
    updateObjects(dt);
    spawner_.setSpawnInterval(currentSpawnInterval());
    handleCollisions();
    removeDeadObjects();
    advanceWaveIfComplete();

    survivalTime_ += dt;

    if (player_.isDead()) {
        state_ = GameState::GameOver;
    } else if (isMapCleared()) {
        state_ = GameState::MapComplete;
    }
}

void GameWorld::reset() {
    player_ = Player();
    map_ = MapInstance();
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    projectiles_.clear();
    enemies_.clear();
    droppedItems_.clear();
    inventory_.clear();
    spawner_.reset();
    skillBar_.reset();
    skillBar_.applyStats(player_.stats());
    state_ = GameState::Playing;
    score_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectPosition_ = Vector2(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f);
    secondarySkillEffectTimer_ = 0.0f;
    playerHitCooldown_ = 0.0f;
    mapLevel_ = 1;
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    mapKills_ = 0;
    mapExperienceGained_ = 0;
    mapItemsDropped_ = 0;
    mapItemsPickedUp_ = 0;
    mapRewardChosen_ = false;
    mapRewardItemQuantityBonus_ = 1.0f;
    passiveTreeOpen_ = false;
    generateMapModifier();
}

void GameWorld::startNextMap() {
    ++mapLevel_;
    map_ = MapInstance();
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectTimer_ = 0.0f;
    playerHitCooldown_ = 0.0f;

    projectiles_.clear();
    enemies_.clear();
    droppedItems_.clear();
    spawner_.reset();
    skillBar_.reset();
    skillBar_.applyStats(player_.stats());
    state_ = GameState::Playing;
    mapKills_ = 0;
    mapExperienceGained_ = 0;
    mapItemsDropped_ = 0;
    mapItemsPickedUp_ = 0;
    mapRewardChosen_ = false;
    passiveTreeOpen_ = false;
    generateMapModifier();
}

void GameWorld::updateObjects(float dt) {
    for (auto& projectile : projectiles_) {
        projectile.update(dt, map_.size());
    }
    for (auto& enemy : enemies_) {
        enemy.update(dt, player_.position());
    }
}

void GameWorld::spawnEnemies(float dt) {
    triggerBossIfNeeded();

    if (map_.bossDefeated() || map_.bossTriggered()) {
        return;
    }

    const MapArea area = map_.areaForPlayer(player_.position());
    if (area == MapArea::Start || area == MapArea::BossGate || area == MapArea::BossArena) {
        return;
    }

    if (static_cast<int>(enemies_.size()) >= Config::MaxActiveEnemies) {
        return;
    }

    spawner_.update(dt);
    const bool spawnElite = (std::rand() % 100) < std::min(20, 6 + mapLevel_ * 2);
    const int hp = spawnElite ? enemyHpForMap() * 3 : enemyHpForMap();
    const int damage = spawnElite ? enemyDamageForMap() + 1 : enemyDamageForMap();
    const EnemyType type = spawnElite ? EnemyType::Elite : EnemyType::Normal;

    if (auto enemy = spawner_.trySpawnNear(player_.position(), map_.size(), hp, damage, type)) {
        enemies_.push_back(*enemy);
        ++enemiesSpawnedInWave_;
    }
}

void GameWorld::handleCollisions() {
    for (auto& projectile : projectiles_) {
        for (auto& enemy : enemies_) {
            if (!projectile.isAlive() || enemy.isDead()) {
                continue;
            }

            if (Collision::circleCircle(
                    projectile.position(), projectile.radius(),
                    enemy.position(), enemy.radius()
                )) {
                enemy.takeDamage(projectile.damage());
                projectile.kill();

                if (enemy.isDead()) {
                    rewardEnemyKill(enemy);
                }
            }
        }
    }

    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (Collision::circleCircle(
                player_.position(), player_.radius(),
                enemy.position(), enemy.radius()
            )) {
            if (playerHitCooldown_ <= 0.0f) {
                player_.takeDamage(enemy.contactDamage());
                playerHitCooldown_ = Config::PlayerHitCooldown;
            }
            if (!enemy.isBoss()) {
                enemy.kill();
            }
        }
    }

}

void GameWorld::removeDeadObjects() {
    auto projIt = std::remove_if(projectiles_.begin(), projectiles_.end(),
        [](const Projectile& p) { return !p.isAlive(); });
    if (projIt != projectiles_.end()) {
        projectiles_.erase(projIt, projectiles_.end());
    }

    auto enemyIt = std::remove_if(enemies_.begin(), enemies_.end(),
        [](const Enemy& e) { return e.isDead(); });
    if (enemyIt != enemies_.end()) {
        enemies_.erase(enemyIt, enemies_.end());
    }

    auto itemIt = std::remove_if(droppedItems_.begin(), droppedItems_.end(),
        [](const DroppedItem& item) { return item.isCollected(); });
    if (itemIt != droppedItems_.end()) {
        droppedItems_.erase(itemIt, droppedItems_.end());
    }
}

void GameWorld::tryCastMovementSkill(Input& input) {
    if (!input.dash() || !skillBar_.tryCast(SkillSlot::Movement)) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() == 0.0f) {
        return;
    }

    player_.setPosition(player_.position() + direction * Config::DashDistance);
}

void GameWorld::tryCastUtilitySkill(Input& input) {
    if (!input.nova() || !skillBar_.tryCast(SkillSlot::Utility)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Utility);
    dealAreaDamage(
        player_.position(),
        skill.radius,
        static_cast<int>(skill.baseDamage * player_.stats().damageMultiplier)
    );
    novaEffectTimer_ = skill.effectDuration;
}

void GameWorld::tryCastSecondarySkill(Input& input) {
    if (!input.secondarySkill() || !skillBar_.tryCast(SkillSlot::Secondary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Secondary);
    dealAreaDamage(
        aimPosition_,
        skill.radius,
        static_cast<int>(skill.baseDamage * player_.stats().damageMultiplier)
    );
    secondarySkillEffectPosition_ = aimPosition_;
    secondarySkillEffectTimer_ = skill.effectDuration;
}

void GameWorld::tryCastPrimarySkill(Input& input) {
    if (!input.primaryFireHeld()) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() <= 0.0f) {
        return;
    }

    if (!skillBar_.tryCast(SkillSlot::Primary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Primary);
    const int damage = static_cast<int>(skill.baseDamage * player_.stats().damageMultiplier);

    if (skill.projectileCount <= 1 || skill.spreadAngle <= 0.0f) {
        projectiles_.push_back(Projectile(player_.position(), direction * Config::ProjectileSpeed, damage));
        return;
    }

    const float degToRad = 3.14159265f / 180.0f;
    const float halfSpread = skill.spreadAngle * 0.5f;
    const float step = skill.spreadAngle / static_cast<float>(skill.projectileCount - 1);
    for (int i = 0; i < skill.projectileCount; ++i) {
        const float angleDeg = -halfSpread + step * static_cast<float>(i);
        const float angleRad = angleDeg * degToRad;
        const float cosA = std::cos(angleRad);
        const float sinA = std::sin(angleRad);
        const Vector2 rotated(
            direction.x * cosA - direction.y * sinA,
            direction.x * sinA + direction.y * cosA
        );
        projectiles_.push_back(Projectile(
            player_.position(),
            rotated * Config::ProjectileSpeed,
            damage
        ));
    }
}

void GameWorld::dealAreaDamage(const Vector2& center, float radius, int damage) {
    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (Collision::circleCircle(
                center, radius,
                enemy.position(), enemy.radius()
            )) {
            enemy.takeDamage(damage);

            if (enemy.isDead()) {
                rewardEnemyKill(enemy);
            }
        }
    }
}

void GameWorld::tryPickupDroppedItem(Input& input) {
    if (!input.pickup()) {
        return;
    }

    const float itemPickupRange = (Config::ItemPickupRange + player_.radius())
        * player_.stats().pickupRangeMultiplier;
    for (auto& droppedItem : droppedItems_) {
        if (droppedItem.isCollected()) {
            continue;
        }

        Vector2 diff = player_.position() - droppedItem.position();
        if (diff.lengthSquared() <= itemPickupRange * itemPickupRange) {
            inventory_.add(droppedItem.collect());
            ++mapItemsPickedUp_;
            return;
        }
    }
}

void GameWorld::trySpendPassivePoint(Input& input) {
    if (!passiveTreeOpen_ || input.passiveChoice() <= 0) {
        return;
    }

    const auto nodeIndex = static_cast<std::size_t>(input.passiveChoice() - 1);
    if (player_.spendPassivePoint(nodeIndex)) {
        skillBar_.applyStats(player_.stats());
    }
}

void GameWorld::tryEquipInventoryItem(Input& input) {
    if (input.inventoryChoice() <= 0) {
        return;
    }

    const auto index = static_cast<std::size_t>(input.inventoryChoice() - 1);
    if (auto item = inventory_.take(index)) {
        if (auto replaced = player_.equipItem(std::move(*item))) {
            inventory_.add(std::move(*replaced));
        }
        skillBar_.applyStats(player_.stats());
    }
}

void GameWorld::tryChooseMapReward(Input& input) {
    if (mapRewardChosen_ || input.rewardChoice() <= 0) {
        return;
    }

    applyMapReward(input.rewardChoice());
}

void GameWorld::applyMapReward(int rewardChoice) {
    switch (rewardChoice) {
        case 1:
            player_.applyUpgrade(UpgradeType::Damage);
            skillBar_.applyStats(player_.stats());
            mapRewardChosen_ = true;
            break;
        case 2:
            player_.applyUpgrade(UpgradeType::MaxHp);
            mapRewardChosen_ = true;
            break;
        case 3:
            mapRewardItemQuantityBonus_ *= 1.25f;
            mapRewardChosen_ = true;
            break;
        default:
            break;
    }
}

void GameWorld::rewardEnemyKill(const Enemy& enemy) {
    if (enemy.isBoss()) {
        map_.markBossDefeated();
    }

    ++mapKills_;
    score_ += 100;
    if (enemy.isBoss()) {
        score_ += 900;
    } else if (enemy.isElite()) {
        score_ += 400;
    }

    const int exp = enemy.isBoss() ? Config::ExpPerKill * 10
        : enemy.isElite() ? Config::ExpPerKill * 5
        : Config::ExpPerKill;
    player_.gainExp(exp);
    mapExperienceGained_ += exp;

    const float eliteDropMultiplier = enemy.isBoss() ? 4.0f
        : enemy.isElite() ? 2.5f
        : 1.0f;
    const int dropChance = std::min(100, static_cast<int>(
        Config::ItemDropChancePercent * mapModifier_.itemQuantityMultiplier
        * mapRewardItemQuantityBonus_ * eliteDropMultiplier));
    if ((std::rand() % 100) < dropChance) {
        droppedItems_.push_back(DroppedItem(enemy.position(), lootGenerator_.generate(mapLevel_)));
        ++mapItemsDropped_;
    }
}

void GameWorld::advanceWaveIfComplete() {
}

bool GameWorld::isMapCleared() const {
    return map_.bossDefeated();
}

int GameWorld::enemiesPerWave() const {
    return Config::BaseEnemiesPerWave + (mapLevel_ - 1) * Config::EnemiesPerMapLevel;
}

int GameWorld::enemyHpForMap() const {
    const float levelMultiplier = 1.0f + (mapLevel_ - 1) * 0.25f;
    return std::max(1, static_cast<int>(std::ceil(
        Config::EnemyHp * levelMultiplier * mapModifier_.monsterHpMultiplier
    )));
}

int GameWorld::enemyDamageForMap() const {
    return Config::EnemyContactDamage + (mapLevel_ - 1) / 3 + mapModifier_.monsterDamageBonus;
}

bool GameWorld::shouldSpawnBoss() const {
    return !map_.bossTriggered()
        && map_.areaForPlayer(player_.position()) == MapArea::BossArena;
}

void GameWorld::triggerBossIfNeeded() {
    if (!shouldSpawnBoss()) {
        return;
    }

    map_.triggerBoss();
    enemies_.clear();
    projectiles_.clear();

    const int hp = enemyHpForMap() * 16;
    const int damage = enemyDamageForMap() + 2;
    enemies_.push_back(Enemy(map_.bossCenter(), hp, damage, EnemyType::Boss));
}

void GameWorld::generateMapModifier() {
    switch (std::rand() % 3) {
        case 0:
            mapModifier_ = {"Monsters have +50% life", 1.5f, 0, 1.0f};
            break;
        case 1:
            mapModifier_ = {"Monsters deal +1 damage", 1.0f, 1, 1.0f};
            break;
        case 2:
            mapModifier_ = {"Items drop 50% more often", 1.0f, 0, 1.5f};
            break;
    }
}

const Player& GameWorld::player() const { return player_; }
const std::vector<Projectile>& GameWorld::projectiles() const { return projectiles_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
const std::vector<DroppedItem>& GameWorld::droppedItems() const { return droppedItems_; }
const Inventory& GameWorld::inventory() const { return inventory_; }
const Vector2& GameWorld::aimPosition() const { return aimPosition_; }
float GameWorld::novaEffectProgress() const {
    const float duration = skillBar_.definition(SkillSlot::Utility).effectDuration;
    return duration > 0.0f ? novaEffectTimer_ / duration : 0.0f;
}
float GameWorld::novaEffectRadius() const {
    return skillBar_.definition(SkillSlot::Utility).radius;
}
const Vector2& GameWorld::secondarySkillEffectPosition() const {
    return secondarySkillEffectPosition_;
}
float GameWorld::secondarySkillEffectProgress() const {
    const float duration = skillBar_.definition(SkillSlot::Secondary).effectDuration;
    return duration > 0.0f ? secondarySkillEffectTimer_ / duration : 0.0f;
}
float GameWorld::secondarySkillEffectRadius() const {
    return skillBar_.definition(SkillSlot::Secondary).radius;
}
const SkillBar& GameWorld::skillBar() const { return skillBar_; }
const MapInstance& GameWorld::map() const { return map_; }
MapArea GameWorld::currentMapArea() const { return map_.areaForPlayer(player_.position()); }
float GameWorld::distanceToBoss() const { return map_.distanceToBoss(player_.position()); }
Vector2 GameWorld::cameraTopLeft() const {
    const float viewportWidth = static_cast<float>(Config::WindowWidth);
    const float viewportHeight = static_cast<float>(Config::WindowHeight);
    return {
        std::clamp(player_.position().x - viewportWidth / 2.0f, 0.0f, map_.size().x - viewportWidth),
        std::clamp(player_.position().y - viewportHeight / 2.0f, 0.0f, map_.size().y - viewportHeight)
    };
}
bool GameWorld::passiveTreeOpen() const { return passiveTreeOpen_; }
GameState GameWorld::state() const { return state_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }
int GameWorld::mapLevel() const { return mapLevel_; }
int GameWorld::currentWave() const {
    return 0;
}
int GameWorld::enemiesRemainingInWave() const {
    return static_cast<int>(enemies_.size());
}
const MapModifier& GameWorld::mapModifier() const { return mapModifier_; }
int GameWorld::mapKills() const { return mapKills_; }
int GameWorld::mapExperienceGained() const { return mapExperienceGained_; }
int GameWorld::mapItemsDropped() const { return mapItemsDropped_; }
int GameWorld::mapItemsPickedUp() const { return mapItemsPickedUp_; }
bool GameWorld::mapRewardChosen() const { return mapRewardChosen_; }
float GameWorld::mapRewardItemQuantityBonus() const { return mapRewardItemQuantityBonus_; }

float GameWorld::currentSpawnInterval() const {
    constexpr float startInterval = Config::EnemySpawnInterval;
    constexpr float intervalPerMapLevel = 0.04f;
    constexpr float minimumInterval = 0.25f;

    return std::max(minimumInterval, startInterval - (mapLevel_ - 1) * intervalPerMapLevel);
}
