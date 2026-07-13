#pragma once

#include <string>

#include "Vector2.hpp"

struct CombatFeedback {
    Vector2 position;
    int damage = 0;
    std::string source;
    float timeRemaining = 0.0f;
};
