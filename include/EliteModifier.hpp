#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "EnemyDefinition.hpp"

enum class EliteModifier {
    None,
    Hardened,
    Swift,
    Volatile
};

struct EliteModifierDefinition {
    EliteModifier modifier = EliteModifier::None;
    std::string name;
    float hpMultiplier = 1.0f;
    float speedMultiplier = 1.0f;
    int damageBonus = 0;
    float deathBurstRadius = 0.0f;
    int deathBurstDamage = 0;
    EnemyColor outlineColor;
};

class EliteModifierLibrary {
public:
    static const EliteModifierDefinition& forModifier(EliteModifier modifier) {
        return all()[static_cast<std::size_t>(modifier)];
    }

    static const std::array<EliteModifierDefinition, 4>& all() {
        static const std::array<EliteModifierDefinition, 4> definitions = {{
            {EliteModifier::None, "", 1.0f, 1.0f, 0, 0.0f, 0, {255, 220, 120}},
            {EliteModifier::Hardened, "Hardened", 1.60f, 1.0f, 0, 0.0f, 0, {105, 185, 255}},
            {EliteModifier::Swift, "Swift", 1.0f, 1.45f, 0, 0.0f, 0, {255, 235, 95}},
            {EliteModifier::Volatile, "Volatile", 1.0f, 1.0f, 0, 82.0f, 2, {255, 125, 55}},
        }};
        return definitions;
    }
};
