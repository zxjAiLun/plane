#pragma once

#include <optional>
#include "Vector2.hpp"
#include "Bullet.hpp"
#include "Timer.hpp"

class Player {
public:
    Player();

    void update(float dt);

    void moveLeft(float dt);
    void moveRight(float dt);
    void moveUp(float dt);
    void moveDown(float dt);

    std::optional<Bullet> tryShoot();

    void takeDamage(int damage);
    bool isDead() const;

    const Vector2& position() const;
    float radius() const;
    int hp() const;

private:
    Vector2 position_;
    float speed_;
    float radius_;
    int hp_;

    Timer shootCooldown_;
};
