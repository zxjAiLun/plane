#pragma once

enum class AilmentType {
    None,
    Ignite,
    Chill
};

struct AilmentDefinition {
    AilmentType type = AilmentType::None;
    float duration = 0.0f;
    float damageMultiplier = 0.0f;
    float speedMultiplier = 1.0f;
    int ignitePenetration = 0;
    int chillPenetration = 0;
};
