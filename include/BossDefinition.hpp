#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "BossDash.hpp"
#include "Config.hpp"
#include "EnemyType.hpp"
#include "GroundHazard.hpp"

enum class BossSkillType {
    CircularAoe,
    Projectile,
    SummonAdds,
    Dash
};

enum class BossLootTheme {
    Brimstone,
    Storm,
    Brood
};

struct BossSkillDefinition {
    BossSkillType type = BossSkillType::CircularAoe;
    std::string name;
    float radius = 0.0f;
    int damage = 0;
    float telegraphDuration = 0.0f;
    float effectDuration = 0.0f;
    float projectileSpeed = 0.0f;
    int projectileCount = 1;
    float spreadAngle = 0.0f;
    EnemyType summonType = EnemyType::Normal;
    int summonCount = 0;
    GroundHazardDefinition groundHazard;
    BossDashDefinition dash;
};

struct BossDefinition {
    std::string name;
    std::string theme;
    BossLootTheme lootTheme = BossLootTheme::Brimstone;
    std::string lootRewardDescription;
    float hpMultiplier = 1.0f;
    int damageBonus = 0;
    float dropMultiplier = 1.0f;
    float skillInterval = Config::BossSkillInterval;
    int guaranteedDrops = 1;
    float enrageHealthRatio = 0.5f;
    float enragedSkillIntervalMultiplier = 0.7f;
    float enragedDamageMultiplier = 1.15f;
    std::string patternDescription;
    std::string enragedPatternDescription;
    std::vector<BossSkillDefinition> skills;
    std::vector<std::size_t> normalSkillOrder;
    std::vector<std::size_t> enragedSkillOrder;
    int igniteResistance = 0;
    int chillResistance = 0;
    std::string enrageTransitionDescription;
    EnemyType enrageSummonType = EnemyType::Normal;
    int enrageSummonCount = 0;
    GroundHazardDefinition enrageHazard;

    const BossSkillDefinition& skillForCast(std::size_t castIndex, bool enraged) const {
        static const BossSkillDefinition fallback;
        if (skills.empty()) {
            return fallback;
        }

        const auto& order = enraged && !enragedSkillOrder.empty()
            ? enragedSkillOrder
            : normalSkillOrder;
        if (order.empty()) {
            return skills[castIndex % skills.size()];
        }

        const std::size_t skillIndex = order[castIndex % order.size()];
        return skills[skillIndex < skills.size() ? skillIndex : 0];
    }
};

class BossLibrary {
public:
    static const std::vector<BossDefinition>& all() {
        static const std::vector<BossDefinition> bosses = buildBosses();
        return bosses;
    }

    static const BossDefinition& forMapLevel(int mapLevel) {
        const auto& bosses = all();
        const auto index = static_cast<std::size_t>((mapLevel - 1) % static_cast<int>(bosses.size()));
        return bosses[index];
    }

private:
    static std::vector<BossDefinition> buildBosses() {
        return {
            {
                "Brimstone Colossus",
                "Lava and stone",
                BossLootTheme::Brimstone,
                "Rare weapon: damage and area damage",
                24.0f,
                2,
                3.5f,
                2.2f,
                2,
                0.50f,
                0.70f,
                1.15f,
                "Alternates magma slams and flame spears",
                "Repeated magma slams",
                {
                    {
                        BossSkillType::CircularAoe,
                        "Magma Slam",
                        Config::BossAoeRadius,
                        Config::BossAoeDamage,
                        Config::BossAoeTelegraphDuration,
                        Config::BossAoeEffectDuration,
                        0.0f,
                        1,
                        0.0f,
                        EnemyType::Normal,
                        0,
                        {"Magma Pool", 105.0f, 5.0f, 0.75f, 1}
                    },
                    {
                        BossSkillType::Projectile,
                        "Flame Spear",
                        Config::BossProjectileRadius,
                        Config::BossProjectileDamage,
                        0.0f,
                        0.0f,
                        Config::BossProjectileSpeed,
                        1,
                        0.0f
                    },
                },
                {0, 1},
                {0, 0, 1},
                35,
                20,
                "Molten core exposed: Ravagers join the burning arena",
                EnemyType::Charger,
                2,
                {"Enrage Magma", 135.0f, 8.0f, 0.75f, 2}
            },
            {
                "Storm Herald",
                "Lightning and speed",
                BossLootTheme::Storm,
                "Rare ring: attack speed and projectile damage",
                24.0f,
                1,
                3.0f,
                1.8f,
                1,
                0.45f,
                0.65f,
                1.10f,
                "Lightning spears and a telegraphed tempest rush",
                "Rapid spear pressure with repeated tempest rushes",
                {
                    {BossSkillType::Projectile, "Lightning Spear", Config::BossProjectileRadius, 2, 0.0f, 0.0f, 520.0f, 1, 0.0f},
                    {BossSkillType::CircularAoe, "Thundercall", 150.0f, 3, 0.50f, 0.25f, 0.0f, 1, 0.0f},
                    {
                        BossSkillType::Dash,
                        "Tempest Rush",
                        52.0f,
                        2,
                        0.60f,
                        0.30f,
                        0.0f,
                        1,
                        0.0f,
                        EnemyType::Normal,
                        0,
                        {},
                        {340.0f, 680.0f}
                    },
                },
                {0, 2, 0, 1},
                {2, 0, 0, 2, 1},
                20,
                35,
                "Storm eye opened: Spitters join the lightning field",
                EnemyType::Ranged,
                2,
                {"Storm Field", 140.0f, 8.0f, 0.75f, 2}
            },
            {
                "Brood Matriarch",
                "Acid and brood",
                BossLootTheme::Brood,
                "Rare amulet: area damage and radius",
                24.0f,
                1,
                3.2f,
                2.0f,
                2,
                0.50f,
                0.72f,
                1.20f,
                "Acid spray, nest bursts, and brooding hatchlings",
                "Rapid acid pressure with ranged broodlings",
                {
                    {BossSkillType::Projectile, "Acid Spray", Config::BossProjectileRadius, 1, 0.0f, 0.0f, 420.0f, 3, 28.0f},
                    {BossSkillType::CircularAoe, "Nest Burst", 115.0f, 2, 0.55f, 0.25f, 0.0f, 1, 0.0f},
                    {
                        BossSkillType::SummonAdds,
                        "Hatch Broodlings",
                        105.0f,
                        0,
                        0.65f,
                        0.25f,
                        0.0f,
                        1,
                        0.0f,
                        EnemyType::Normal,
                        4
                    },
                    {
                        BossSkillType::SummonAdds,
                        "Hatch Spitters",
                        125.0f,
                        0,
                        0.75f,
                        0.25f,
                        0.0f,
                        1,
                        0.0f,
                        EnemyType::Ranged,
                        2
                    },
                },
                {0, 2, 1, 0},
                {0, 3, 0, 2, 1},
                30,
                30,
                "Brood unleashed: Acid pools spread beneath the nest",
                EnemyType::Ranged,
                2,
                {"Acid Nest", 120.0f, 8.0f, 0.75f, 2}
            },
        };
    }
};
