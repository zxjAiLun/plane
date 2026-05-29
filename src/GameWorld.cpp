#include "GameWorld.hpp"
#include "Collision.hpp"
#include "Config.hpp"

#include <algorithm>

GameWorld::GameWorld()
    : state_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f) {
}

void GameWorld::update(float dt, const Input& input) {
    switch (state_) {
        case GameState::Playing:
            updatePlaying(dt, input);
            break;

        case GameState::LevelUp:
            if (input.restart()) {
                player_.confirmLevelUp();
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
}

void GameWorld::updatePlaying(float dt, const Input& input) {
    if (input.moveLeft()) player_.moveLeft(dt);
    if (input.moveRight()) player_.moveRight(dt);
    if (input.moveUp()) player_.moveUp(dt);
    if (input.moveDown()) player_.moveDown(dt);

    player_.update(dt);

    weapon_.update(dt);
    if (auto projectile = weapon_.tryShoot(player_.position(), enemies_)) {
        projectiles_.push_back(*projectile);
    }

    spawnEnemies(dt);
    updateObjects(dt);
    handleCollisions();
    removeDeadObjects();

    survivalTime_ += dt;

    if (player_.isLevelUp()) {
        state_ = GameState::LevelUp;
    }

    if (player_.isDead()) {
        state_ = GameState::GameOver;
    }

    if (survivalTime_ >= Config::VictoryTime) {
        state_ = GameState::Victory;
    }
}

void GameWorld::reset() {
    player_ = Player();
    projectiles_.clear();
    enemies_.clear();
    orbs_.clear();
    spawner_.reset();
    weapon_.reset();
    state_ = GameState::Playing;
    score_ = 0;
    survivalTime_ = 0.0f;
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

    for (auto& orb : orbs_) {
        if (orb.isCollected()) {
            continue;
        }

        float pickupRange = Config::PickupRange + player_.radius();
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

const Player& GameWorld::player() const { return player_; }
const std::vector<Projectile>& GameWorld::projectiles() const { return projectiles_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
const std::vector<ExperienceOrb>& GameWorld::orbs() const { return orbs_; }
GameState GameWorld::state() const { return state_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }
