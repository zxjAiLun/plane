#pragma once

#include <vector>
#include "Player.hpp"
#include "Bullet.hpp"
#include "Enemy.hpp"
#include "EnemySpawner.hpp"
#include "Input.hpp"

class GameWorld {
public:
    GameWorld();

    void update(float dt, const Input& input);
    void reset();

    const Player& player() const;
    const std::vector<Bullet>& bullets() const;
    const std::vector<Enemy>& enemies() const;

    int score() const;
    bool isGameOver() const;

private:
    void updateObjects(float dt);
    void spawnEnemies(float dt);
    void handleCollisions();
    void removeDeadObjects();

private:
    Player player_;
    std::vector<Bullet> bullets_;
    std::vector<Enemy> enemies_;
    EnemySpawner spawner_;

    int score_;
    bool gameOver_;
};
