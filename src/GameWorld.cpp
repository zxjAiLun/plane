#include "GameWorld.hpp"
#include "Collision.hpp"
#include "Config.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

GameWorld::GameWorld()
    : dashCooldown_(0.0f)
    , novaCooldown_(0.0f)
    , secondarySkillCooldown_(0.0f)
    , state_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f)
    , aimPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , novaEffectTimer_(0.0f)
    , mapLevel_(1)
    , currentWave_(0)
    , enemiesSpawnedInWave_(0)
    , mapModifier_()
    , currentUpgrades_{} {
    generateUpgrades();
    generateMapModifier();
}

void GameWorld::update(float dt, Input& input) {
    aimPosition_ = Vector2(
        static_cast<float>(input.mousePosition().x),
        static_cast<float>(input.mousePosition().y)
    );

    switch (state_) {
        case GameState::Playing:
            updatePlaying(dt, input);
            break;

        case GameState::LevelUp:
            if (input.upgradeChoice() >= 1 && input.upgradeChoice() <= 3) {
                int idx = input.upgradeChoice() - 1;
                player_.applyUpgrade(currentUpgrades_[idx].type);
                weapon_.applyStats(player_.stats());
                state_ = GameState::Playing;
            }
            break;

        case GameState::GameOver:
            if (input.restart()) {
                reset();
            }
            break;

        case GameState::MapComplete:
            if (input.nextMap()) {
                startNextMap();
            } else if (input.restart()) {
                reset();
            }
            break;
    }

    input.update();
}

void GameWorld::updatePlaying(float dt, Input& input) {
    if (input.moveLeft()) player_.moveLeft(dt);
    if (input.moveRight()) player_.moveRight(dt);
    if (input.moveUp()) player_.moveUp(dt);
    if (input.moveDown()) player_.moveDown(dt);

    player_.update(dt);
    dashCooldown_.update(dt);
    novaCooldown_.update(dt);
    secondarySkillCooldown_.update(dt);
    novaEffectTimer_ = std::max(0.0f, novaEffectTimer_ - dt);
    tryDash(input);
    tryNova(input);
    trySecondarySkill(input);
    tryEquipInventoryItem(input);

    weapon_.update(dt);
    if (input.primaryFireHeld()) {
        if (auto projectile = weapon_.tryShoot(player_.position(), aimPosition_)) {
            projectiles_.push_back(*projectile);
        }
    }

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
    } else if (player_.isLevelUp()) {
        generateUpgrades();
        state_ = GameState::LevelUp;
    }
}

void GameWorld::reset() {
    player_ = Player();
    projectiles_.clear();
    enemies_.clear();
    orbs_.clear();
    droppedItems_.clear();
    inventory_.clear();
    spawner_.reset();
    weapon_.reset();
    dashCooldown_.setDuration(0.0f);
    dashCooldown_.reset();
    novaCooldown_.setDuration(0.0f);
    novaCooldown_.reset();
    secondarySkillCooldown_.setDuration(0.0f);
    secondarySkillCooldown_.reset();
    state_ = GameState::Playing;
    score_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    mapLevel_ = 1;
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    generateUpgrades();
    generateMapModifier();
}

void GameWorld::startNextMap() {
    ++mapLevel_;
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;

    projectiles_.clear();
    enemies_.clear();
    orbs_.clear();
    droppedItems_.clear();
    spawner_.reset();
    dashCooldown_.setDuration(0.0f);
    dashCooldown_.reset();
    novaCooldown_.setDuration(0.0f);
    novaCooldown_.reset();
    secondarySkillCooldown_.setDuration(0.0f);
    secondarySkillCooldown_.reset();
    state_ = GameState::Playing;
    generateMapModifier();
}

void GameWorld::updateObjects(float dt) {
    for (auto& projectile : projectiles_) {
        projectile.update(dt);
    }
    for (auto& enemy : enemies_) {
        enemy.update(dt, player_.position());
    }
}

void GameWorld::spawnEnemies(float dt) {
    if (currentWave_ >= Config::MapWaveCount || enemiesSpawnedInWave_ >= enemiesPerWave()) {
        return;
    }

    spawner_.update(dt);
    if (auto enemy = spawner_.trySpawn(enemyHpForMap(), enemyDamageForMap())) {
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
            player_.takeDamage(enemy.contactDamage());
            enemy.kill();
        }
    }

    float pickupRange = (Config::PickupRange + player_.radius()) * player_.stats().pickupRangeMultiplier;
    for (auto& orb : orbs_) {
        if (orb.isCollected()) {
            continue;
        }

        Vector2 diff = player_.position() - orb.position();
        if (diff.lengthSquared() <= pickupRange * pickupRange) {
            orb.collect();
            player_.gainExp(orb.value());
        }
    }

    const float itemPickupRange = Config::ItemPickupRange + player_.radius();
    for (auto& droppedItem : droppedItems_) {
        if (droppedItem.isCollected()) {
            continue;
        }

        Vector2 diff = player_.position() - droppedItem.position();
        if (diff.lengthSquared() <= itemPickupRange * itemPickupRange) {
            inventory_.add(droppedItem.collect());
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

    auto orbIt = std::remove_if(orbs_.begin(), orbs_.end(),
        [](const ExperienceOrb& o) { return o.isCollected(); });
    if (orbIt != orbs_.end()) {
        orbs_.erase(orbIt, orbs_.end());
    }

    auto itemIt = std::remove_if(droppedItems_.begin(), droppedItems_.end(),
        [](const DroppedItem& item) { return item.isCollected(); });
    if (itemIt != droppedItems_.end()) {
        droppedItems_.erase(itemIt, droppedItems_.end());
    }
}

void GameWorld::generateUpgrades() {
    static const std::array<Upgrade, 5> allUpgrades = {{
        {UpgradeType::Damage,    "+20% Damage",    "Increase weapon damage"},
        {UpgradeType::FireRate,  "+20% Fire Rate", "Shoot faster"},
        {UpgradeType::MoveSpeed, "+10% Move Speed", "Move faster"},
        {UpgradeType::PickupRange, "+25% Pickup Range", "Collect exp from further"},
        {UpgradeType::MaxHp,     "+1 Max HP",      "Increase max health"},
    }};

    std::array<bool, 5> used{};
    for (int i = 0; i < 3; ++i) {
        int idx;
        do {
            idx = std::rand() % allUpgrades.size();
        } while (used[idx]);
        used[idx] = true;
        currentUpgrades_[i] = allUpgrades[idx];
    }
}

void GameWorld::tryDash(Input& input) {
    if (!input.dash() || !dashCooldown_.isReady()) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() == 0.0f) {
        return;
    }

    player_.setPosition(player_.position() + direction * Config::DashDistance);
    dashCooldown_.setDuration(Config::DashCooldown);
    dashCooldown_.reset();
}

void GameWorld::tryNova(Input& input) {
    if (!input.nova() || !novaCooldown_.isReady()) {
        return;
    }

    const int damage = static_cast<int>(Config::NovaDamage * player_.stats().damageMultiplier);
    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (Collision::circleCircle(
                player_.position(), Config::NovaRadius,
                enemy.position(), enemy.radius()
            )) {
            enemy.takeDamage(damage);

            if (enemy.isDead()) {
                rewardEnemyKill(enemy);
            }
        }
    }

    novaEffectTimer_ = Config::NovaEffectDuration;
    novaCooldown_.setDuration(Config::NovaCooldown);
    novaCooldown_.reset();
}

void GameWorld::trySecondarySkill(Input& input) {
    if (!input.secondarySkill() || !secondarySkillCooldown_.isReady()) {
        return;
    }

    secondarySkillCooldown_.setDuration(Config::SecondarySkillCooldown);
    secondarySkillCooldown_.reset();
}

void GameWorld::tryEquipInventoryItem(Input& input) {
    if (input.inventoryChoice() <= 0) {
        return;
    }

    const auto index = static_cast<std::size_t>(input.inventoryChoice() - 1);
    if (auto item = inventory_.take(index)) {
        player_.equipItem(*item);
        weapon_.applyStats(player_.stats());
    }
}

void GameWorld::rewardEnemyKill(const Enemy& enemy) {
    score_ += 100;
    orbs_.push_back(ExperienceOrb(enemy.position(), Config::ExpPerKill));

    const int dropChance = std::min(100,
        static_cast<int>(Config::ItemDropChancePercent * mapModifier_.itemQuantityMultiplier));
    if ((std::rand() % 100) < dropChance) {
        droppedItems_.push_back(DroppedItem(enemy.position(), lootGenerator_.generate(mapLevel_)));
    }
}

void GameWorld::advanceWaveIfComplete() {
    if (currentWave_ >= Config::MapWaveCount) {
        return;
    }

    if (enemiesSpawnedInWave_ >= enemiesPerWave() && enemies_.empty()) {
        ++currentWave_;
        enemiesSpawnedInWave_ = 0;
        spawner_.reset();
    }
}

bool GameWorld::isMapCleared() const {
    return currentWave_ >= Config::MapWaveCount;
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
const std::vector<ExperienceOrb>& GameWorld::orbs() const { return orbs_; }
const std::vector<DroppedItem>& GameWorld::droppedItems() const { return droppedItems_; }
const Inventory& GameWorld::inventory() const { return inventory_; }
const std::array<Upgrade, 3>& GameWorld::currentUpgrades() const { return currentUpgrades_; }
const Vector2& GameWorld::aimPosition() const { return aimPosition_; }
float GameWorld::novaEffectProgress() const {
    return novaEffectTimer_ / Config::NovaEffectDuration;
}
GameState GameWorld::state() const { return state_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }
int GameWorld::mapLevel() const { return mapLevel_; }
int GameWorld::currentWave() const {
    return std::min(currentWave_ + 1, Config::MapWaveCount);
}
int GameWorld::enemiesRemainingInWave() const {
    return enemiesPerWave() - enemiesSpawnedInWave_ + static_cast<int>(enemies_.size());
}
const MapModifier& GameWorld::mapModifier() const { return mapModifier_; }

float GameWorld::currentSpawnInterval() const {
    constexpr float startInterval = Config::EnemySpawnInterval;
    constexpr float intervalPerMapLevel = 0.04f;
    constexpr float minimumInterval = 0.25f;

    return std::max(minimumInterval, startInterval - (mapLevel_ - 1) * intervalPerMapLevel);
}
