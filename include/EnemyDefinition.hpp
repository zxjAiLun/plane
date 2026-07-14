#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "Config.hpp"
#include "EnemyType.hpp"

struct EnemyColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

enum class EnemyAttackStyle {
    Melee,
    Projectile,
    Charge,
    Summon,
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
    int flaskChargeChancePercent = 0;
    int flaskChargeAmount = 0;
    EnemyAttackStyle attackStyle = EnemyAttackStyle::Melee;
    float attackRange = 0.0f;
    float attackWindup = 0.0f;
    float attackCooldown = 0.0f;
    float projectileSpeed = 0.0f;
    float projectileRadius = 0.0f;
    EnemyColor fillColor;
    EnemyColor outlineColor;
    float outlineThickness = 0.0f;
    float chargeSpeedMultiplier = 1.0f;
    float chargeDuration = 0.0f;
    int igniteResistance = 0;
    int chillResistance = 0;
    EnemyType summonType = EnemyType::Normal;
    int summonCount = 0;
    int fireResistance = 0;
    int coldResistance = 0;
    int lightningResistance = 0;
};

class EnemyLibrary {
public:
    static const EnemyDefinition& forType(EnemyType type) {
        const auto& definitions = all();
        return definitions[static_cast<std::size_t>(type)];
    }

    static const std::array<EnemyDefinition, 7>& all() {
        static const std::array<EnemyDefinition, 7> definitions = {{
            {
                EnemyType::Normal,
                "Feral",
                1.0f,
                1.0f,
                0,
                100,
                1,
                1.0f,
                20,
                1,
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
                25,
                1,
                EnemyAttackStyle::Projectile,
                420.0f,
                0.45f,
                1.50f,
                330.0f,
                7.0f,
                {75, 205, 130},
                {25, 80, 55},
                1.5f,
                1.0f,
                0.0f,
                10,
                10,
                EnemyType::Normal,
                0,
                10,
                10,
                0
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
                100,
                1,
                EnemyAttackStyle::Melee,
                60.0f,
                0.25f,
                0.90f,
                0.0f,
                0.0f,
                {180, 60, 255},
                {255, 220, 120},
                3.0f,
                1.0f,
                0.0f,
                15,
                15,
                EnemyType::Normal,
                0,
                15,
                15,
                0
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
                100,
                Config::LifeFlaskMaxCharges,
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
            {
                EnemyType::Charger,
                "Ravager",
                1.10f,
                1.25f,
                1,
                180,
                2,
                1.40f,
                30,
                1,
                EnemyAttackStyle::Charge,
                340.0f,
                0.55f,
                2.40f,
                0.0f,
                0.0f,
                {235, 140, 45},
                {255, 215, 100},
                2.0f,
                4.80f,
                0.42f,
                15,
                10,
                EnemyType::Normal,
                0,
                15,
                10,
                0
            },
            {
                EnemyType::Warden,
                "Warden",
                1.25f,
                2.40f,
                1,
                420,
                3,
                1.80f,
                50,
                1,
                EnemyAttackStyle::Melee,
                52.0f,
                0.35f,
                1.30f,
                0.0f,
                0.0f,
                {70, 180, 220},
                {175, 245, 255},
                2.5f,
                1.0f,
                0.0f,
                25,
                20,
                EnemyType::Normal,
                0,
                25,
                20,
                0
            },
            {
                EnemyType::Summoner,
                "Hexbinder",
                1.05f,
                1.80f,
                1,
                320,
                3,
                1.60f,
                50,
                1,
                EnemyAttackStyle::Summon,
                420.0f,
                0.60f,
                3.80f,
                0.0f,
                0.0f,
                {195, 80, 220},
                {245, 170, 255},
                2.5f,
                1.0f,
                0.0f,
                20,
                20,
                EnemyType::Normal,
                2,
                20,
                20,
                0
            },
        }};

        return definitions;
    }
};
