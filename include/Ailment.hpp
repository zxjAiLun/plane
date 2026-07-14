#pragma once

enum class AilmentType {
    None,
    Ignite,
    Chill,
    Shock
};

struct AilmentDefinition {
    AilmentType type = AilmentType::None;
    float duration = 0.0f;
    float damageMultiplier = 0.0f;
    float speedMultiplier = 1.0f;
    int ignitePenetration = 0;
    int chillPenetration = 0;
    float damageTakenMultiplier = 1.0f;
    int shockPenetration = 0;
};

struct AilmentTickResult {
    AilmentType type = AilmentType::None;
    int damage = 0;
    int tickCount = 0;
    bool killed = false;
};
