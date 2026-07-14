#pragma once

#include <string>

#include "Vector2.hpp"

enum class CombatFeedbackType {
    Damage,
    PlayerHit,
    SkillRejected,
    Telegraph,
    Status
};

struct CombatFeedback {
    Vector2 position;
    int damage = 0;
    std::string source;
    float timeRemaining = 0.0f;
    CombatFeedbackType type = CombatFeedbackType::Damage;
};
