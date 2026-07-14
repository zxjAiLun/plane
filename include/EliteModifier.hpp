#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "EnemyDefinition.hpp"

enum class EliteModifier {
    None,
    Hardened,
    Swift,
    Volatile,
    Empowered,
    Stormbound,
    Rejuvenating
};

struct EliteModifierDefinition {
    EliteModifier modifier = EliteModifier::None;
    std::string name;
    std::string description;
    float hpMultiplier = 1.0f;
    float speedMultiplier = 1.0f;
    int damageBonus = 0;
    float deathBurstRadius = 0.0f;
    int deathBurstDamage = 0;
    EnemyColor outlineColor;
    int ailmentResistanceBonus = 0;
    float allyDamageMultiplier = 1.0f;
    float auraRadius = 0.0f;
    float pulseInterval = 0.0f;
    float pulseTelegraphDuration = 0.0f;
    float pulseRadius = 0.0f;
    int pulseDamage = 0;
    DamageType pulseDamageType = DamageType::Physical;
    AilmentDefinition pulseAilment;
    float healInterval = 0.0f;
    float healRadius = 0.0f;
    float healFraction = 0.0f;
};

class EliteModifierLibrary {
public:
    static const EliteModifierDefinition& forModifier(EliteModifier modifier) {
        return all()[static_cast<std::size_t>(modifier)];
    }

    static const std::array<EliteModifierDefinition, 7>& all() {
        static const std::array<EliteModifierDefinition, 7> definitions = {{
            {EliteModifier::None, "", "", 1.0f, 1.0f, 0, 0.0f, 0, {255, 220, 120}, 0},
            {EliteModifier::Hardened, "Hardened", "+60% maximum life and +20% ailment resistance", 1.60f, 1.0f, 0, 0.0f, 0, {105, 185, 255}, 20},
            {EliteModifier::Swift, "Swift", "+45% movement speed", 1.0f, 1.45f, 0, 0.0f, 0, {255, 235, 95}, 0},
            {EliteModifier::Volatile, "Volatile", "82 radius death burst for 2 damage", 1.0f, 1.0f, 0, 82.0f, 2, {255, 125, 55}, 0},
            {EliteModifier::Empowered, "Empowered", "+25% damage to allies within 190 radius", 1.0f, 1.0f, 0, 0.0f, 0, {255, 100, 190}, 0,
                1.25f, 190.0f},
            {EliteModifier::Stormbound, "Stormbound", "Every 3.5s telegraphs a 120-radius lightning strike", 1.0f, 1.0f, 0, 0.0f, 0, {110, 190, 255}, 0,
                1.0f, 0.0f, 3.5f, 0.8f, 120.0f, 5, DamageType::Lightning,
                {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}},
            {EliteModifier::Rejuvenating, "Rejuvenating", "Heals nearby allies for 8% maximum life every 4s", 1.0f, 1.0f, 0, 0.0f, 0, {105, 235, 150}, 0,
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, DamageType::Physical, {}, 4.0f, 175.0f, 0.08f},
        }};
        return definitions;
    }
};
