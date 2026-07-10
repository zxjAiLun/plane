#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "EnemyType.hpp"

struct EnemyColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

enum class EnemyAttackStyle {
    Melee,
    Projectile,
    Boss
};

struct EnemyDefinition {
    EnemyType type = EnemyType::Normal;
    std::string name;
    float radiusMultiplier = 1.0f;
    float hpMultiplier = 1.0f;
    int damageBonus = 0;
    int scoreReward = 100;
    int expMultiplier = 1;
    float dropMultiplier = 1.0f;
    EnemyAttackStyle attackStyle = EnemyAttackStyle::Melee;
    float attackRange = 0.0f;
    float attackWindup = 0.0f;
    float attackCooldown = 0.0f;
    float projectileSpeed = 0.0f;
    float projectileRadius = 0.0f;
    EnemyColor fillColor;
    EnemyColor outlineColor;
    float outlineThickness = 0.0f;
};

class EnemyLibrary {
public:
    static const EnemyDefinition& forType(EnemyType type) {
        const auto& definitions = all();
        return definitions[static_cast<std::size_t>(type)];
    }

    static const std::array<EnemyDefinition, 4>& all() {
        static const std::array<EnemyDefinition, 4> definitions = {{
            {
                EnemyType::Normal,
                "Feral",
                1.0f,
                1.0f,
                0,
                100,
                1,
                1.0f,
                EnemyAttackStyle::Melee,
                44.0f,
                0.35f,
                1.20f,
                0.0f,
                0.0f,
                {220, 55, 55},
                {0, 0, 0},
                0.0f
            },
            {
                EnemyType::Ranged,
                "Spitter",
                0.90f,
                0.70f,
                0,
                140,
                1,
                1.20f,
                EnemyAttackStyle::Projectile,
                420.0f,
                0.45f,
                1.50f,
                330.0f,
                7.0f,
                {75, 205, 130},
                {25, 80, 55},
                1.5f
            },
            {
                EnemyType::Elite,
                "Elite",
                1.45f,
                3.0f,
                1,
                500,
                5,
                2.5f,
                EnemyAttackStyle::Melee,
                60.0f,
                0.25f,
                0.90f,
                0.0f,
                0.0f,
                {180, 60, 255},
                {255, 220, 120},
                3.0f
            },
            {
                EnemyType::Boss,
                "Boss",
                2.2f,
                1.0f,
                0,
                1000,
                10,
                1.0f,
                EnemyAttackStyle::Boss,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                {255, 80, 40},
                {255, 220, 120},
                5.0f
            },
        }};

        return definitions;
    }
};
