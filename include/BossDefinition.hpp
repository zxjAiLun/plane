#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Ailment.hpp"
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
    Brood,
    Frost
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
    DamageType damageType = DamageType::Physical;
    AilmentDefinition ailment;
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
    int lightningResistance = 0;
    int fireResistance = 0;
    int coldResistance = 0;
    int shockResistance = 0;
    int poisonResistance = 0;

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
    static BossDefinition frostBoss() {
        BossSkillDefinition frostNova;
        frostNova.type = BossSkillType::CircularAoe;
        frostNova.name = "Frost Nova";
        frostNova.radius = 145.0f;
        frostNova.damage = 3;
        frostNova.telegraphDuration = 0.60f;
        frostNova.effectDuration = 0.25f;
        frostNova.groundHazard = {
            "Frozen Ground", 125.0f, 5.0f, 0.75f, 4,
            DamageType::Cold, {AilmentType::Chill, 2.5f, 0.0f, 0.55f}
        };
        frostNova.damageType = DamageType::Cold;
        frostNova.ailment = {AilmentType::Chill, 2.5f, 0.0f, 0.55f};

        BossSkillDefinition iceLance;
        iceLance.type = BossSkillType::Projectile;
        iceLance.name = "Ice Lance";
        iceLance.radius = Config::BossProjectileRadius;
        iceLance.damage = 2;
        iceLance.projectileSpeed = 460.0f;
        iceLance.damageType = DamageType::Cold;
        iceLance.ailment = {AilmentType::Chill, 2.0f, 0.0f, 0.60f};

        BossSkillDefinition glacierCall;
        glacierCall.type = BossSkillType::SummonAdds;
        glacierCall.name = "Call Frostbound Wardens";
        glacierCall.radius = 120.0f;
        glacierCall.telegraphDuration = 0.70f;
        glacierCall.effectDuration = 0.25f;
        glacierCall.summonType = EnemyType::Warden;
        glacierCall.summonCount = 2;
        glacierCall.damageType = DamageType::Cold;

        BossDefinition definition;
        definition.name = "Frostbound Warden";
        definition.theme = "Ice and control";
        definition.lootTheme = BossLootTheme::Frost;
        definition.lootRewardDescription = "Unique ring: cold damage and stronger Chill";
        definition.hpMultiplier = 24.0f;
        definition.damageBonus = 1;
        definition.dropMultiplier = 3.1f;
        definition.skillInterval = 1.9f;
        definition.guaranteedDrops = 1;
        definition.enrageHealthRatio = 0.45f;
        definition.enragedSkillIntervalMultiplier = 0.68f;
        definition.enragedDamageMultiplier = 1.15f;
        definition.patternDescription = "Alternates frost novas and ice lances before calling wardens";
        definition.enragedPatternDescription = "Frozen ground spreads while wardens reinforce the arena";
        definition.skills = {frostNova, iceLance, glacierCall};
        definition.normalSkillOrder = {0, 1, 0, 2};
        definition.enragedSkillOrder = {0, 2, 0, 1, 0};
        definition.igniteResistance = 25;
        definition.chillResistance = 45;
        definition.enrageTransitionDescription = "The ice breaks: frozen ground spreads through the arena";
        definition.enrageSummonType = EnemyType::Warden;
        definition.enrageSummonCount = 2;
        definition.enrageHazard = {
            "Shattered Ice", 145.0f, 7.0f, 0.75f, 7,
            DamageType::Cold, {AilmentType::Chill, 2.5f, 0.0f, 0.55f}
        };
        definition.lightningResistance = 25;
        definition.fireResistance = 30;
        definition.coldResistance = 50;
        definition.shockResistance = 35;
        definition.poisonResistance = 30;
        return definition;
    }

    static BossSkillDefinition elementalSkill(
        BossSkillDefinition skill,
        DamageType damageType,
        AilmentDefinition ailment
    ) {
        skill.damageType = damageType;
        skill.ailment = ailment;
        return skill;
    }

    static GroundHazardDefinition elementalHazard(
        GroundHazardDefinition hazard,
        DamageType damageType,
        AilmentDefinition ailment
    ) {
        hazard.damageType = damageType;
        hazard.ailment = ailment;
        return hazard;
    }

    static std::vector<BossDefinition> buildBosses() {
        return {
            {
                "Brimstone Colossus",
                "Lava and stone",
                BossLootTheme::Brimstone,
                "Unique weapon: fire and area damage",
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
                    elementalSkill({
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
                        elementalHazard(
                            {"Magma Pool", 105.0f, 5.0f, 0.75f, 1},
                            DamageType::Fire,
                            {AilmentType::Ignite, 2.5f, 0.20f}
                        )
                    }, DamageType::Fire, {AilmentType::Ignite, 2.5f, 0.20f}),
                    elementalSkill({
                        BossSkillType::Projectile,
                        "Flame Spear",
                        Config::BossProjectileRadius,
                        Config::BossProjectileDamage,
                        0.0f,
                        0.0f,
                        Config::BossProjectileSpeed,
                        1,
                        0.0f
                    }, DamageType::Fire, {AilmentType::Ignite, 2.5f, 0.20f}),
                },
                {0, 1},
                {0, 0, 1},
                35,
                20,
                "Molten core exposed: Ravagers join the burning arena",
                EnemyType::Charger,
                2,
                elementalHazard(
                    {"Enrage Magma", 135.0f, 8.0f, 0.75f, 2, DamageType::Fire},
                    DamageType::Fire,
                    {AilmentType::Ignite, 2.5f, 0.20f}
                ),
                35,
                35,
                20,
                35,
                20
            },
            {
                "Storm Herald",
                "Lightning and speed",
                BossLootTheme::Storm,
                "Unique ring: lightning and projectile damage",
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
                    elementalSkill(
                        {BossSkillType::Projectile, "Lightning Spear", Config::BossProjectileRadius, 2, 0.0f, 0.0f, 520.0f, 1, 0.0f},
                        DamageType::Lightning,
                        {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}
                    ),
                    elementalSkill(
                        {BossSkillType::CircularAoe, "Thundercall", 150.0f, 3, 0.50f, 0.25f, 0.0f, 1, 0.0f},
                        DamageType::Lightning,
                        {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}
                    ),
                    elementalSkill({
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
                    }, DamageType::Lightning,
                        {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}),
                },
                {0, 2, 0, 1},
                {2, 0, 0, 2, 1},
                20,
                35,
                "Storm eye opened: Spitters join the lightning field",
                EnemyType::Ranged,
                2,
                elementalHazard(
                    {"Storm Field", 140.0f, 8.0f, 0.75f, 2, DamageType::Lightning},
                    DamageType::Lightning,
                    {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}
                ),
                45,
                20,
                35,
                45,
                35
            },
            {
                "Brood Matriarch",
                "Acid and brood",
                BossLootTheme::Brood,
                "Unique amulet: poison damage and area radius",
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
                    elementalSkill(
                        {BossSkillType::Projectile, "Acid Spray", Config::BossProjectileRadius, 1, 0.0f, 0.0f, 420.0f, 3, 28.0f},
                        DamageType::Poison,
                        {AilmentType::Poison, 2.5f, 0.35f}
                    ),
                    elementalSkill(
                        {BossSkillType::CircularAoe, "Nest Burst", 115.0f, 2, 0.55f, 0.25f, 0.0f, 1, 0.0f},
                        DamageType::Poison,
                        {AilmentType::Poison, 2.5f, 0.35f}
                    ),
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
                elementalHazard(
                    {"Acid Nest", 120.0f, 8.0f, 0.75f, 2, DamageType::Poison},
                    DamageType::Poison,
                    {AilmentType::Poison, 2.5f, 0.35f}
                ),
                30,
                30,
                30,
                30,
                45
            },
            frostBoss()
        };
    }
};
