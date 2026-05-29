#pragma once

#include "Vector2.hpp"

class ExperienceOrb {
public:
    ExperienceOrb(const Vector2& position, int value);

    void update(float dt);

    const Vector2& position() const;
    float radius() const;
    int value() const;

    bool isCollected() const;
    void collect();

private:
    Vector2 position_;
    float radius_;
    int value_;
    bool collected_;
};
