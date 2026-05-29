#include "GameWorld.hpp"
#include "Collision.hpp"

GameWorld::GameWorld()
    : score_(0)
    , gameOver_(false) {
}

void GameWorld::update(float dt, const Input& input) {
    if (gameOver_) {
        if (input.restart()) {
            reset();
        }
        return;
    }

    if (input.moveLeft()) player_.moveLeft(dt);
    if (input.moveRight()) player_.moveRight(dt);
    if (input.moveUp()) player_.moveUp(dt);
    if (input.moveDown()) player_.moveDown(dt);

    player_.update(dt);

    if (input.shoot()) {
        if (auto bullet = player_.tryShoot()) {
            bullets_.push_back(*bullet);
        }
    }

    spawnEnemies(dt);
    updateObjects(dt);
    handleCollisions();
    removeDeadObjects();
}

void GameWorld::reset() {
    player_ = Player();
    bullets_.clear();
    enemies_.clear();
    spawner_.reset();
    score_ = 0;
    gameOver_ = false;
}

void GameWorld::updateObjects(float dt) {
    for (auto& bullet : bullets_) {
        bullet.update(dt);
    }
    for (auto& enemy : enemies_) {
        enemy.update(dt);
    }
}

void GameWorld::spawnEnemies(float dt) {
    spawner_.update(dt);
    if (auto enemy = spawner_.trySpawn()) {
        enemies_.push_back(*enemy);
    }
}

void GameWorld::handleCollisions() {
    for (auto& bullet : bullets_) {
        for (auto& enemy : enemies_) {
            if (!bullet.isAlive() || enemy.isDead()) {
                continue;
            }

            if (Collision::circleCircle(
                    bullet.position(), bullet.radius(),
                    enemy.position(), enemy.radius()
                )) {
                enemy.takeDamage(bullet.damage());
                bullet.kill();

                if (enemy.isDead()) {
                    score_ += 100;
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
            enemy.takeDamage(9999);

            if (player_.isDead()) {
                gameOver_ = true;
            }
        }
    }
}

void GameWorld::removeDeadObjects() {
    auto bulletIt = std::remove_if(bullets_.begin(), bullets_.end(),
        [](const Bullet& b) { return !b.isAlive(); });
    if (bulletIt != bullets_.end()) {
        bullets_.erase(bulletIt, bullets_.end());
    }

    auto enemyIt = std::remove_if(enemies_.begin(), enemies_.end(),
        [](const Enemy& e) { return e.isDead(); });
    if (enemyIt != enemies_.end()) {
        enemies_.erase(enemyIt, enemies_.end());
    }
}

const Player& GameWorld::player() const { return player_; }
const std::vector<Bullet>& GameWorld::bullets() const { return bullets_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
int GameWorld::score() const { return score_; }
bool GameWorld::isGameOver() const { return gameOver_; }
