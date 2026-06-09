#pragma once

#include <string>

struct MapModifier {
    std::string description = "No modifier";
    float monsterHpMultiplier = 1.0f;
    int monsterDamageBonus = 0;
    float itemQuantityMultiplier = 1.0f;
};
