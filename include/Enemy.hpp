#pragma once

#include "Vector2.hpp"

class Enemy {
public:
    Enemy(const Vector2& position, int hp);

    void update(float dt, const Vector2& targetPosition);

    void takeDamage(int damage);
    void kill();
    bool isDead() const;

    const Vector2& position() const;
    float radius() const;
    int contactDamage() const;

private:
    Vector2 position_;
    float radius_;
    int hp_;
    int contactDamage_;
};
