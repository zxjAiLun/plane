#include "GameWorld.hpp"
#include "Collision.hpp"
#include "Config.hpp"

#include <algorithm>
#include <cstdlib>

GameWorld::GameWorld()
    : dashCooldown_(0.0f)
    , state_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f)
    , aimPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , currentUpgrades_{} {
    generateUpgrades();
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

        case GameState::Victory:
            if (input.restart()) {
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
    tryDash(input);

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

    survivalTime_ += dt;

    if (player_.isDead()) {
        state_ = GameState::GameOver;
    } else if (survivalTime_ >= Config::VictoryTime) {
        state_ = GameState::Victory;
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
    spawner_.reset();
    weapon_.reset();
    dashCooldown_.setDuration(0.0f);
    dashCooldown_.reset();
    state_ = GameState::Playing;
    score_ = 0;
    survivalTime_ = 0.0f;
    generateUpgrades();
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
    spawner_.update(dt);
    if (auto enemy = spawner_.trySpawn()) {
        enemies_.push_back(*enemy);
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
                    score_ += 100;
                    orbs_.push_back(ExperienceOrb(enemy.position(), Config::ExpPerKill));
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

const Player& GameWorld::player() const { return player_; }
const std::vector<Projectile>& GameWorld::projectiles() const { return projectiles_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
const std::vector<ExperienceOrb>& GameWorld::orbs() const { return orbs_; }
const std::array<Upgrade, 3>& GameWorld::currentUpgrades() const { return currentUpgrades_; }
const Vector2& GameWorld::aimPosition() const { return aimPosition_; }
GameState GameWorld::state() const { return state_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }

float GameWorld::currentSpawnInterval() const {
    constexpr float startInterval = Config::EnemySpawnInterval;
    constexpr float endInterval = 0.15f;

    const float progress = std::min(survivalTime_ / Config::VictoryTime, 1.0f);
    return startInterval + (endInterval - startInterval) * progress;
}
