#pragma once

#include "Vector2.hpp"

class Enemy {
public:
    Enemy(const Vector2& position, const Vector2& velocity, int hp);

    void update(float dt);

    void takeDamage(int damage);
    bool isDead() const;

    const Vector2& position() const;
    float radius() const;
    int contactDamage() const;

private:
    Vector2 position_;
    Vector2 velocity_;
    float radius_;
    int hp_;
    int contactDamage_;
};
