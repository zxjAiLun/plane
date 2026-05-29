#pragma once

#include "Vector2.hpp"

class Projectile {
public:
    Projectile(const Vector2& position, const Vector2& velocity, int damage);

    void update(float dt);

    const Vector2& position() const;
    float radius() const;
    int damage() const;

    bool isAlive() const;
    void kill();

private:
    Vector2 position_;
    Vector2 velocity_;
    float radius_;
    int damage_;
    bool alive_;
};
