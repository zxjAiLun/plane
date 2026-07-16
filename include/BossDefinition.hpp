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
    Frost,
    Archive,
    Obsidian,
    Aether,
    Sable
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

enum class BossPhaseHazardPattern {
    Target,
    Ring,
    Cross
};

struct BossPhaseHazardDefinition {
    float interval = 0.0f;
    float telegraphDuration = 0.0f;
    GroundHazardDefinition hazard;
    BossPhaseHazardPattern pattern = BossPhaseHazardPattern::Target;
    float patternRadius = 0.0f;

    bool isValid() const {
        const bool validPattern = pattern == BossPhaseHazardPattern::Target
            || patternRadius > 0.0f;
        return interval > 0.0f
            && telegraphDuration > 0.0f
            && hazard.isValid()
            && validPattern;
    }
};

struct BossPhaseDefinition {
    float healthRatio = 0.0f;
    float skillIntervalMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    std::string patternDescription;
    std::string transitionDescription;
    std::vector<std::size_t> skillOrder;
    EnemyType summonType = EnemyType::Normal;
    int summonCount = 0;
    GroundHazardDefinition hazard;
    BossPhaseHazardDefinition recurringHazard;

    bool isValid() const {
        return healthRatio > 0.0f
            && healthRatio < 1.0f
            && skillIntervalMultiplier > 0.0f
            && damageMultiplier > 0.0f;
    }
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
    BossPhaseDefinition finalPhase;

    const BossSkillDefinition& skillForCast(std::size_t castIndex, bool enraged) const {
        return skillForCast(castIndex, enraged ? 1 : 0);
    }

    const BossSkillDefinition& skillForCast(
        std::size_t castIndex,
        int phase
    ) const {
        static const BossSkillDefinition fallback;
        if (skills.empty()) {
            return fallback;
        }

        const auto& order = phase >= 2 && !finalPhase.skillOrder.empty()
            ? finalPhase.skillOrder
            : phase >= 1 && !enragedSkillOrder.empty()
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

    static const BossDefinition& forIndex(int bossIndex) {
        const auto& bosses = all();
        const int count = static_cast<int>(bosses.size());
        const int normalizedIndex = ((bossIndex % count) + count) % count;
        return bosses[static_cast<std::size_t>(normalizedIndex)];
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
        definition.lootRewardDescription = "Unique relic: cold damage and stronger Chill";
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
        definition.finalPhase = {
            0.20f,
            0.55f,
            1.30f,
            "Absolute zero: wardens and expanding frost zones control the arena",
            "The pass freezes solid: the Warden enters its final cycle",
            {0, 2, 0, 1, 2},
            EnemyType::Warden,
            3,
            {"Absolute Zero", 165.0f, 7.0f, 0.65f, 8,
                DamageType::Cold, {AilmentType::Chill, 2.5f, 0.0f, 0.50f}}
        };
        definition.finalPhase.recurringHazard = {
            3.8f,
            0.60f,
            {"Frostline Cross", 78.0f, 4.0f, 0.60f, 4,
                DamageType::Cold, {AilmentType::Chill, 2.5f, 0.0f, 0.50f}},
            BossPhaseHazardPattern::Cross,
            165.0f
        };
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

    static BossDefinition drownedBoss() {
        const AilmentDefinition chill{
            AilmentType::Chill, 2.5f, 0.0f, 0.55f
        };

        BossSkillDefinition tidalQuill;
        tidalQuill.type = BossSkillType::Projectile;
        tidalQuill.name = "Tidal Quill";
        tidalQuill.radius = Config::BossProjectileRadius;
        tidalQuill.damage = 2;
        tidalQuill.projectileSpeed = 440.0f;
        tidalQuill.projectileCount = 5;
        tidalQuill.spreadAngle = 42.0f;
        tidalQuill.damageType = DamageType::Cold;
        tidalQuill.ailment = chill;

        BossSkillDefinition undertowSeal;
        undertowSeal.type = BossSkillType::CircularAoe;
        undertowSeal.name = "Undertow Seal";
        undertowSeal.radius = 130.0f;
        undertowSeal.damage = 3;
        undertowSeal.telegraphDuration = 0.55f;
        undertowSeal.effectDuration = 0.25f;
        undertowSeal.groundHazard = elementalHazard(
            {"Archive Undertow", 120.0f, 5.0f, 0.75f, 3},
            DamageType::Cold,
            chill
        );
        undertowSeal.damageType = DamageType::Cold;
        undertowSeal.ailment = chill;

        BossSkillDefinition drownedWardens;
        drownedWardens.type = BossSkillType::SummonAdds;
        drownedWardens.name = "Call Drowned Wardens";
        drownedWardens.radius = 110.0f;
        drownedWardens.telegraphDuration = 0.70f;
        drownedWardens.effectDuration = 0.25f;
        drownedWardens.summonType = EnemyType::Warden;
        drownedWardens.summonCount = 2;
        drownedWardens.damageType = DamageType::Cold;

        BossSkillDefinition inkCurrent;
        inkCurrent.type = BossSkillType::Dash;
        inkCurrent.name = "Ink Current";
        inkCurrent.radius = 52.0f;
        inkCurrent.damage = 2;
        inkCurrent.telegraphDuration = 0.55f;
        inkCurrent.effectDuration = 0.30f;
        inkCurrent.dash = {320.0f, 620.0f};
        inkCurrent.damageType = DamageType::Cold;
        inkCurrent.ailment = chill;

        BossDefinition definition;
        definition.name = "Tidebound Archivist";
        definition.theme = "Flooded archives and cold currents";
        definition.lootTheme = BossLootTheme::Archive;
        definition.lootRewardDescription = "Unique relic: projectile damage and Chill control";
        definition.hpMultiplier = 25.0f;
        definition.damageBonus = 2;
        definition.dropMultiplier = 3.6f;
        definition.skillInterval = 1.85f;
        definition.guaranteedDrops = 2;
        definition.enrageHealthRatio = 0.48f;
        definition.enragedSkillIntervalMultiplier = 0.65f;
        definition.enragedDamageMultiplier = 1.18f;
        definition.patternDescription = "Cold seals and fan-shaped quills control the archive halls";
        definition.enragedPatternDescription = "Drowned wardens advance while the undertow seals overlap";
        definition.skills = {tidalQuill, undertowSeal, drownedWardens, inkCurrent};
        definition.normalSkillOrder = {0, 1, 0, 2, 3};
        definition.enragedSkillOrder = {1, 0, 3, 2, 0};
        definition.igniteResistance = 30;
        definition.chillResistance = 55;
        definition.enrageTransitionDescription = "The archive floods: drowned wardens rise from the dark water";
        definition.enrageSummonType = EnemyType::Warden;
        definition.enrageSummonCount = 2;
        definition.enrageHazard = elementalHazard(
            {"Blackwater Surge", 145.0f, 7.0f, 0.70f, 6},
            DamageType::Cold,
            chill
        );
        definition.lightningResistance = 25;
        definition.fireResistance = 35;
        definition.coldResistance = 55;
        definition.shockResistance = 35;
        definition.poisonResistance = 30;
        definition.finalPhase = {
            0.20f,
            0.52f,
            1.32f,
            "The drowned archive closes: seals, quills and wardens fill the arena",
            "The final ledger opens: the Archivist enters its last cycle",
            {1, 0, 3, 2, 1},
            EnemyType::Warden,
            3,
            {"Deep Undertow", 165.0f, 7.0f, 0.65f, 8,
                DamageType::Cold, chill}
        };
        definition.finalPhase.recurringHazard = {
            4.2f,
            0.70f,
            {"Archive Undertow", 118.0f, 3.2f, 0.65f, 4,
                DamageType::Cold, chill},
            BossPhaseHazardPattern::Target,
            0.0f
        };
        return definition;
    }

    static BossDefinition obsidianBoss() {
        const AilmentDefinition ignite{
            AilmentType::Ignite, 2.5f, 0.20f
        };

        BossSkillDefinition shardfall;
        shardfall.type = BossSkillType::CircularAoe;
        shardfall.name = "Shardfall";
        shardfall.radius = 120.0f;
        shardfall.damage = 4;
        shardfall.telegraphDuration = 0.45f;
        shardfall.effectDuration = 0.25f;
        shardfall.groundHazard = elementalHazard(
            {"Obsidian Shards", 105.0f, 5.0f, 0.70f, 4},
            DamageType::Fire,
            ignite
        );
        shardfall.damageType = DamageType::Fire;
        shardfall.ailment = ignite;

        BossSkillDefinition glassVolley;
        glassVolley.type = BossSkillType::Projectile;
        glassVolley.name = "Glass Volley";
        glassVolley.radius = Config::BossProjectileRadius;
        glassVolley.damage = 2;
        glassVolley.projectileSpeed = 480.0f;
        glassVolley.projectileCount = 3;
        glassVolley.spreadAngle = 30.0f;
        glassVolley.damageType = DamageType::Fire;
        glassVolley.ailment = ignite;

        BossSkillDefinition forgeRavagers;
        forgeRavagers.type = BossSkillType::SummonAdds;
        forgeRavagers.name = "Forge Ravagers";
        forgeRavagers.radius = 115.0f;
        forgeRavagers.telegraphDuration = 0.60f;
        forgeRavagers.effectDuration = 0.25f;
        forgeRavagers.summonType = EnemyType::Charger;
        forgeRavagers.summonCount = 2;
        forgeRavagers.damageType = DamageType::Fire;

        BossSkillDefinition blackglassCharge;
        blackglassCharge.type = BossSkillType::Dash;
        blackglassCharge.name = "Blackglass Charge";
        blackglassCharge.radius = 56.0f;
        blackglassCharge.damage = 3;
        blackglassCharge.telegraphDuration = 0.50f;
        blackglassCharge.effectDuration = 0.30f;
        blackglassCharge.dash = {350.0f, 650.0f};
        blackglassCharge.damageType = DamageType::Fire;
        blackglassCharge.ailment = ignite;

        BossDefinition definition;
        definition.name = "Obsidian Tyrant";
        definition.theme = "Black glass and ember dust";
        definition.lootTheme = BossLootTheme::Obsidian;
        definition.lootRewardDescription = "Unique relic: fire damage and area devastation";
        definition.hpMultiplier = 26.0f;
        definition.damageBonus = 2;
        definition.dropMultiplier = 3.8f;
        definition.skillInterval = 1.75f;
        definition.guaranteedDrops = 2;
        definition.enrageHealthRatio = 0.46f;
        definition.enragedSkillIntervalMultiplier = 0.63f;
        definition.enragedDamageMultiplier = 1.20f;
        definition.patternDescription = "Shardfall zones and glass volleys punish stationary movement";
        definition.enragedPatternDescription = "Ravagers charge through expanding fields of burning glass";
        definition.skills = {shardfall, glassVolley, forgeRavagers, blackglassCharge};
        definition.normalSkillOrder = {0, 1, 0, 2, 3};
        definition.enragedSkillOrder = {3, 0, 2, 1, 0};
        definition.igniteResistance = 55;
        definition.chillResistance = 25;
        definition.enrageTransitionDescription = "The reliquary cracks: forge ravagers spill into the arena";
        definition.enrageSummonType = EnemyType::Charger;
        definition.enrageSummonCount = 2;
        definition.enrageHazard = elementalHazard(
            {"Tyrant's Furnace", 150.0f, 7.0f, 0.65f, 8},
            DamageType::Fire,
            ignite
        );
        definition.lightningResistance = 25;
        definition.fireResistance = 60;
        definition.coldResistance = 25;
        definition.shockResistance = 30;
        definition.poisonResistance = 30;
        definition.finalPhase = {
            0.18f,
            0.50f,
            1.35f,
            "The reliquary collapses: shards, volleys and charges overlap",
            "The black core ignites: the Tyrant enters its last cycle",
            {0, 3, 1, 2, 0},
            EnemyType::Charger,
            3,
            {"Melted Reliquary", 170.0f, 8.0f, 0.60f, 9,
                DamageType::Fire, ignite}
        };
        definition.finalPhase.recurringHazard = {
            3.6f,
            0.55f,
            {"Molten Ring", 82.0f, 3.0f, 0.60f, 5,
                DamageType::Fire, ignite},
            BossPhaseHazardPattern::Ring,
            155.0f
        };
        return definition;
    }

    static BossDefinition aetherBoss() {
        const AilmentDefinition shock{
            AilmentType::Shock, 2.0f, 0.0f, 1.15f
        };

        BossSkillDefinition prismPulse;
        prismPulse.type = BossSkillType::CircularAoe;
        prismPulse.name = "Prism Pulse";
        prismPulse.radius = 142.0f;
        prismPulse.damage = 3;
        prismPulse.telegraphDuration = 0.50f;
        prismPulse.effectDuration = 0.25f;
        prismPulse.groundHazard = elementalHazard(
            {"Prism Residue", 118.0f, 5.0f, 0.70f, 3},
            DamageType::Lightning,
            shock
        );
        prismPulse.damageType = DamageType::Lightning;
        prismPulse.ailment = shock;

        BossSkillDefinition lensVolley;
        lensVolley.type = BossSkillType::Projectile;
        lensVolley.name = "Lens Volley";
        lensVolley.radius = Config::BossProjectileRadius;
        lensVolley.damage = 2;
        lensVolley.projectileSpeed = 500.0f;
        lensVolley.projectileCount = 5;
        lensVolley.spreadAngle = 46.0f;
        lensVolley.damageType = DamageType::Lightning;
        lensVolley.ailment = shock;

        BossSkillDefinition summonCasters;
        summonCasters.type = BossSkillType::SummonAdds;
        summonCasters.name = "Summon Lens Casters";
        summonCasters.radius = 120.0f;
        summonCasters.telegraphDuration = 0.70f;
        summonCasters.effectDuration = 0.25f;
        summonCasters.summonType = EnemyType::Ranged;
        summonCasters.summonCount = 2;
        summonCasters.damageType = DamageType::Lightning;

        BossSkillDefinition phaseDash;
        phaseDash.type = BossSkillType::Dash;
        phaseDash.name = "Phase Shift";
        phaseDash.radius = 54.0f;
        phaseDash.damage = 2;
        phaseDash.telegraphDuration = 0.55f;
        phaseDash.effectDuration = 0.30f;
        phaseDash.dash = {360.0f, 700.0f};
        phaseDash.damageType = DamageType::Lightning;
        phaseDash.ailment = shock;

        BossDefinition definition;
        definition.name = "Astral Nullifier";
        definition.theme = "Aether lenses and charged void";
        definition.lootTheme = BossLootTheme::Aether;
        definition.lootRewardDescription = "Unique relic: maximum Mana, Mana recovery and reduced skill costs";
        definition.hpMultiplier = 27.0f;
        definition.damageBonus = 2;
        definition.dropMultiplier = 4.0f;
        definition.skillInterval = 1.75f;
        definition.guaranteedDrops = 2;
        definition.enrageHealthRatio = 0.46f;
        definition.enragedSkillIntervalMultiplier = 0.62f;
        definition.enragedDamageMultiplier = 1.20f;
        definition.patternDescription = "Prism pulses, lens volleys and phase shifts protect the observatory core";
        definition.enragedPatternDescription = "Charged casters reinforce the arena while pulses overlap";
        definition.skills = {prismPulse, lensVolley, summonCasters, phaseDash};
        definition.normalSkillOrder = {0, 1, 0, 2, 3};
        definition.enragedSkillOrder = {3, 0, 2, 1, 0};
        definition.igniteResistance = 25;
        definition.chillResistance = 25;
        definition.lightningResistance = 60;
        definition.fireResistance = 25;
        definition.coldResistance = 30;
        definition.shockResistance = 55;
        definition.poisonResistance = 35;
        definition.enrageTransitionDescription = "The lens fractures: charged casters join the null field";
        definition.enrageSummonType = EnemyType::Ranged;
        definition.enrageSummonCount = 2;
        definition.enrageHazard = elementalHazard(
            {"Null Field", 148.0f, 7.0f, 0.65f, 7},
            DamageType::Lightning,
            shock
        );
        definition.finalPhase = {
            0.18f,
            0.50f,
            1.35f,
            "The observatory collapses: pulses, volleys and phase shifts fill the core",
            "The null lens opens: the Nullifier enters its final cycle",
            {0, 3, 2, 1, 0},
            EnemyType::Ranged,
            3,
            {"Astral Collapse", 168.0f, 8.0f, 0.60f, 9,
                DamageType::Lightning, shock}
        };
        definition.finalPhase.recurringHazard = {
            3.5f,
            0.55f,
            {"Rotating Lens", 84.0f, 3.0f, 0.60f, 5,
                DamageType::Lightning, shock},
            BossPhaseHazardPattern::Ring,
            158.0f
        };
        return definition;
    }

    static BossDefinition sableBoss() {
        const AilmentDefinition poison{
            AilmentType::Poison, 3.0f, 0.35f
        };

        BossSkillDefinition marrowBurst = elementalSkill({
            BossSkillType::CircularAoe,
            "Marrow Burst",
            148.0f,
            4,
            0.55f,
            0.25f,
            0.0f,
            1,
            0.0f,
            EnemyType::Normal,
            0,
            elementalHazard(
                {"Marrow Pool", 122.0f, 6.0f, 0.70f, 4},
                DamageType::Poison,
                poison
            )
        }, DamageType::Poison, poison);

        BossSkillDefinition venomVolley = elementalSkill({
            BossSkillType::Projectile,
            "Venom Volley",
            Config::BossProjectileRadius,
            2,
            0.0f,
            0.0f,
            450.0f,
            5,
            44.0f
        }, DamageType::Poison, poison);

        BossSkillDefinition gravebloomCall = elementalSkill({
            BossSkillType::SummonAdds,
            "Call Gravebloom",
            118.0f,
            0,
            0.70f,
            0.25f,
            0.0f,
            1,
            0.0f,
            EnemyType::Summoner,
            2
        }, DamageType::Poison, poison);

        BossSkillDefinition ossuaryDash = elementalSkill({
            BossSkillType::Dash,
            "Ossuary Lunge",
            56.0f,
            3,
            0.55f,
            0.30f,
            0.0f,
            1,
            0.0f,
            EnemyType::Normal,
            0,
            {},
            {360.0f, 670.0f}
        }, DamageType::Poison, poison);

        BossDefinition definition;
        definition.name = "Gravebloom Sovereign";
        definition.theme = "Dust, bone and venom";
        definition.lootTheme = BossLootTheme::Sable;
        definition.lootRewardDescription = "Unique relic: Poison damage and gravebloom reach";
        definition.hpMultiplier = 28.0f;
        definition.damageBonus = 2;
        definition.dropMultiplier = 4.2f;
        definition.skillInterval = 1.70f;
        definition.guaranteedDrops = 2;
        definition.enrageHealthRatio = 0.46f;
        definition.enragedSkillIntervalMultiplier = 0.62f;
        definition.enragedDamageMultiplier = 1.22f;
        definition.patternDescription = "Marrow bursts and venom volleys surround the ossuary core";
        definition.enragedPatternDescription = "Gravebloom summons spread poisonous zones through the arena";
        definition.skills = {marrowBurst, venomVolley, gravebloomCall, ossuaryDash};
        definition.normalSkillOrder = {0, 1, 0, 2, 3};
        definition.enragedSkillOrder = {3, 0, 2, 1, 0};
        definition.igniteResistance = 25;
        definition.chillResistance = 25;
        definition.lightningResistance = 25;
        definition.fireResistance = 25;
        definition.coldResistance = 25;
        definition.shockResistance = 25;
        definition.poisonResistance = 60;
        definition.enrageTransitionDescription = "The grave opens: summoned blooms flood the arena";
        definition.enrageSummonType = EnemyType::Summoner;
        definition.enrageSummonCount = 2;
        definition.enrageHazard = elementalHazard(
            {"Sovereign's Rot", 152.0f, 8.0f, 0.65f, 8},
            DamageType::Poison,
            poison
        );
        definition.finalPhase = {
            0.18f,
            0.48f,
            1.38f,
            "The ossuary blooms: bursts, volleys and lunges overlap",
            "The Sovereign tears open the marrow seal for its final cycle",
            {0, 3, 2, 1, 0},
            EnemyType::Summoner,
            3,
            {"Marrow Apocalypse", 174.0f, 9.0f, 0.60f, 10,
                DamageType::Poison, poison}
        };
        definition.finalPhase.recurringHazard = {
            3.4f,
            0.55f,
            {"Gravebloom Ring", 86.0f, 4.0f, 0.60f, 6,
                DamageType::Poison, poison},
            BossPhaseHazardPattern::Ring,
            158.0f
        };
        return definition;
    }

    static std::vector<BossDefinition> buildBosses() {
        auto bosses = std::vector<BossDefinition>{
            {
                "Brimstone Colossus",
                "Lava and stone",
                BossLootTheme::Brimstone,
                "Unique relic: fire and area damage",
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
                "Unique relic: lightning and projectile damage",
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
                "Unique relic: poison damage and area radius",
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
            frostBoss(),
            drownedBoss(),
            obsidianBoss(),
            aetherBoss(),
            sableBoss()
        };

        bosses[0].finalPhase = {
            0.25f,
            0.55f,
            1.35f,
            "The arena burns: magma slams chain while Ravagers close in",
            "The core ruptures: the Colossus enters its final eruption",
            {0, 0, 1, 0, 1},
            EnemyType::Charger,
            2,
            {"Final Inferno", 170.0f, 6.5f, 0.60f, 3,
                DamageType::Fire, {AilmentType::Ignite, 2.5f, 0.20f}}
        };
        bosses[0].finalPhase.recurringHazard = {
            3.7f,
            0.58f,
            {"Cinder Cross", 88.0f, 4.5f, 0.70f, 4,
                DamageType::Fire, {AilmentType::Ignite, 2.5f, 0.20f}},
            BossPhaseHazardPattern::Cross,
            145.0f
        };
        bosses[1].finalPhase = {
            0.22f,
            0.50f,
            1.30f,
            "The storm collapses inward: dashes and lightning strikes overlap",
            "The storm eye opens: the Herald unleashes its last cycle",
            {2, 1, 0, 2, 2},
            EnemyType::Ranged,
            3,
            {"Eye of the Storm", 155.0f, 6.0f, 0.60f, 3,
                DamageType::Lightning,
                {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.20f}}
        };
        bosses[1].finalPhase.recurringHazard = {
            3.2f,
            0.50f,
            {"Static Lattice", 72.0f, 3.5f, 0.60f, 3,
                DamageType::Lightning,
                {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.20f}},
            BossPhaseHazardPattern::Cross,
            175.0f
        };
        bosses[2].finalPhase = {
            0.25f,
            0.58f,
            1.35f,
            "The nest ruptures: acid bursts and ranged brood overlap",
            "The brood awakens: the Matriarch fights through its last clutch",
            {1, 3, 0, 2, 1},
            EnemyType::Ranged,
            3,
            {"Brood Acid", 150.0f, 7.0f, 0.70f, 3,
                DamageType::Poison, {AilmentType::Poison, 2.5f, 0.35f}}
        };
        bosses[2].finalPhase.recurringHazard = {
            4.0f,
            0.65f,
            {"Acid Bloom", 96.0f, 3.5f, 0.80f, 4,
                DamageType::Poison, {AilmentType::Poison, 2.5f, 0.35f}},
            BossPhaseHazardPattern::Ring,
            135.0f
        };
        return bosses;
    }
};
