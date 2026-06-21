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

struct EnemyDefinition {
    EnemyType type = EnemyType::Normal;
    std::string name;
    float radiusMultiplier = 1.0f;
    float hpMultiplier = 1.0f;
    int damageBonus = 0;
    int scoreReward = 100;
    int expMultiplier = 1;
    float dropMultiplier = 1.0f;
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

    static const std::array<EnemyDefinition, 3>& all() {
        static const std::array<EnemyDefinition, 3> definitions = {{
            {
                EnemyType::Normal,
                "Feral",
                1.0f,
                1.0f,
                0,
                100,
                1,
                1.0f,
                {220, 55, 55},
                {0, 0, 0},
                0.0f
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
                {255, 80, 40},
                {255, 220, 120},
                5.0f
            },
        }};

        return definitions;
    }
};
