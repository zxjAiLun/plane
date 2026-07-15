#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "EnemyType.hpp"
#include "EliteModifier.hpp"
#include "LootBias.hpp"

struct EnemyPackDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::array<EnemyType, 6> enemies{};
    int enemyCount = 0;
    LootBias lootBias;
    int clearRewardDrops = 1;
    int leaderIndex = -1;
    std::string leaderName;
    std::string leaderDescription;
    std::array<EliteModifier, 2> leaderModifiers{
        EliteModifier::None, EliteModifier::None
    };
    float leaderDropMultiplier = 1.0f;
    int leaderBonusDrops = 0;
    int leaderExperienceMultiplier = 1;
    LootBias leaderLootBias;
    std::string leaderRewardDescription;
};

class EnemyPackLibrary {
public:
    static constexpr int ThemeCount = 4;
    static constexpr int PacksPerTheme = 3;

    static const std::array<EnemyPackDefinition, ThemeCount * PacksPerTheme>& all() {
        static const std::array<EnemyPackDefinition, ThemeCount * PacksPerTheme> packs = {{
            {
                "ashen-line-breaker",
                "Line Breakers",
                "Feral front line with a Ravager push",
                {EnemyType::Normal, EnemyType::Normal, EnemyType::Normal,
                 EnemyType::Charger, EnemyType::Elite, EnemyType::Normal},
                6,
                {AffixTag::Damage, 1.35f, AffixTag::Armor, 1.15f},
                1,
                4,
                "Cinderjaw",
                "A hardened empowered line captain that survives focused fire",
                {EliteModifier::Hardened, EliteModifier::Empowered},
                2.0f,
                2,
                2,
                {AffixTag::Damage, 1.70f, AffixTag::Armor, 1.25f},
                "Damage / Armor weighted | 2 guaranteed drops"
            },
            {
                "ashen-warden-court",
                "Ashen Warden Court",
                "A Warden anchors a slow, durable melee formation",
                {EnemyType::Warden, EnemyType::Normal, EnemyType::Normal,
                 EnemyType::Normal, EnemyType::Charger, EnemyType::Elite},
                6,
                {AffixTag::Survival, 1.35f, AffixTag::Armor, 1.20f},
                1,
                5,
                "Ashen Bulwark",
                "A rejuvenating hardened commander guarded by a Warden court",
                {EliteModifier::Hardened, EliteModifier::Rejuvenating},
                2.0f,
                2,
                2,
                {AffixTag::Survival, 1.70f, AffixTag::Armor, 1.25f},
                "Survival / Armor weighted | 2 guaranteed drops"
            },
            {
                "ashen-last-stand",
                "Last Stand",
                "Elite pressure backed by a compact melee screen",
                {EnemyType::Elite, EnemyType::Normal, EnemyType::Normal,
                 EnemyType::Charger, EnemyType::Warden, EnemyType::Normal},
                6,
                {AffixTag::Damage, 1.40f, AffixTag::Area, 1.15f},
                1,
                0,
                "Last Ember",
                "A stormbound captain that accelerates when the formation breaks",
                {EliteModifier::Swift, EliteModifier::Stormbound},
                2.0f,
                1,
                2,
                {AffixTag::Fire, 1.70f, AffixTag::Area, 1.25f},
                "Fire / Area weighted | 1 guaranteed drop"
            },
            {
                "storm-spitter-screen",
                "Spitter Screen",
                "Ranged pressure forces movement through the open field",
                {EnemyType::Ranged, EnemyType::Ranged, EnemyType::Elite,
                 EnemyType::Ranged, EnemyType::Charger, EnemyType::Summoner},
                6,
                {AffixTag::Projectile, 1.40f, AffixTag::AttackSpeed, 1.15f},
                1,
                2,
                "Storm Herald",
                "A stormbound empowered artillery leader with a long-range screen",
                {EliteModifier::Stormbound, EliteModifier::Empowered},
                2.0f,
                2,
                2,
                {AffixTag::Lightning, 1.75f, AffixTag::Projectile, 1.30f},
                "Lightning / Projectile weighted | 2 guaranteed drops"
            },
            {
                "storm-anchor-patrol",
                "Anchor Patrol",
                "A Hexbinder creates a protected ranged position",
                {EnemyType::Summoner, EnemyType::Ranged, EnemyType::Ranged,
                 EnemyType::Elite, EnemyType::Charger, EnemyType::Normal},
                6,
                {AffixTag::Lightning, 1.35f, AffixTag::Projectile, 1.20f},
                1,
                3,
                "Anchor of Glass",
                "A hardened stormbound anchor that protects a crossfire nest",
                {EliteModifier::Hardened, EliteModifier::Stormbound},
                2.0f,
                1,
                2,
                {AffixTag::Lightning, 1.70f, AffixTag::Armor, 1.25f},
                "Lightning / Armor weighted | 1 guaranteed drop"
            },
            {
                "storm-crossfire",
                "Crossfire Wing",
                "Chargers collapse on a ranged crossfire",
                {EnemyType::Ranged, EnemyType::Charger, EnemyType::Ranged,
                 EnemyType::Normal, EnemyType::Elite, EnemyType::Ranged},
                6,
                {AffixTag::AttackSpeed, 1.35f, AffixTag::Projectile, 1.20f},
                1,
                4,
                "Crossfire Prime",
                "A swift stormbound captain that collapses the ranged wing",
                {EliteModifier::Swift, EliteModifier::Stormbound},
                2.0f,
                2,
                2,
                {AffixTag::Projectile, 1.70f, AffixTag::AttackSpeed, 1.25f},
                "Projectile / Attack Speed weighted | 2 guaranteed drops"
            },
            {
                "venom-overgrowth",
                "Overgrowth",
                "Mixed elemental threats with a durable Warden",
                {EnemyType::Warden, EnemyType::Elite, EnemyType::Charger,
                 EnemyType::Normal, EnemyType::Summoner, EnemyType::Normal},
                6,
                {AffixTag::Poison, 1.40f, AffixTag::Area, 1.15f},
                1,
                1,
                "Rotbloom Keeper",
                "A hardened rejuvenating guardian rooted in toxic growth",
                {EliteModifier::Hardened, EliteModifier::Rejuvenating},
                2.0f,
                2,
                2,
                {AffixTag::Poison, 1.75f, AffixTag::Area, 1.25f},
                "Poison / Area weighted | 2 guaranteed drops"
            },
            {
                "venom-hunter-nest",
                "Hunter Nest",
                "A Summoner and Chargers punish stationary builds",
                {EnemyType::Summoner, EnemyType::Charger, EnemyType::Charger,
                 EnemyType::Normal, EnemyType::Elite, EnemyType::Normal},
                6,
                {AffixTag::Poison, 1.35f, AffixTag::Damage, 1.20f},
                1,
                4,
                "Venom Fang",
                "A swift empowered hunter that drives the pack forward",
                {EliteModifier::Swift, EliteModifier::Empowered},
                2.0f,
                1,
                2,
                {AffixTag::Poison, 1.70f, AffixTag::Damage, 1.25f},
                "Poison / Damage weighted | 1 guaranteed drop"
            },
            {
                "venom-bloom-guard",
                "Bloom Guard",
                "Elite and Warden defenses protect a poison-leaning screen",
                {EnemyType::Elite, EnemyType::Warden, EnemyType::Normal,
                 EnemyType::Normal, EnemyType::Charger, EnemyType::Summoner},
                6,
                {AffixTag::Poison, 1.45f, AffixTag::Survival, 1.15f},
                1,
                0,
                "Bloom Matriarch",
                "A rejuvenating volatile leader that protects the final growth",
                {EliteModifier::Rejuvenating, EliteModifier::Volatile},
                2.0f,
                2,
                2,
                {AffixTag::Poison, 1.80f, AffixTag::Survival, 1.30f},
                "Poison / Survival weighted | 2 guaranteed drops"
            },
            {
                "frost-shard-wall",
                "Shard Wall",
                "Wardens hold a cold-forged line while chargers break your escape route",
                {EnemyType::Warden, EnemyType::Charger, EnemyType::Normal,
                 EnemyType::Elite, EnemyType::Charger, EnemyType::Normal},
                6,
                {AffixTag::Cold, 1.45f, AffixTag::Area, 1.20f},
                1,
                3,
                "Rime Bastion",
                "A hardened rejuvenating captain that refuses to yield ground",
                {EliteModifier::Hardened, EliteModifier::Rejuvenating},
                2.0f,
                2,
                2,
                {AffixTag::Cold, 1.75f, AffixTag::Area, 1.30f},
                "Cold / Area weighted | 2 guaranteed drops"
            },
            {
                "frost-whiteout-crossfire",
                "Whiteout Crossfire",
                "Ranged wardens and chargers create a freezing crossfire",
                {EnemyType::Ranged, EnemyType::Warden, EnemyType::Charger,
                 EnemyType::Elite, EnemyType::Ranged, EnemyType::Normal},
                6,
                {AffixTag::Cold, 1.35f, AffixTag::Projectile, 1.20f},
                1,
                3,
                "Whiteout Marshal",
                "An empowered stormbound commander concealed by the blizzard",
                {EliteModifier::Stormbound, EliteModifier::Empowered},
                2.0f,
                2,
                2,
                {AffixTag::Cold, 1.70f, AffixTag::Projectile, 1.30f},
                "Cold / Projectile weighted | 2 guaranteed drops"
            },
            {
                "frost-frozen-procession",
                "Frozen Procession",
                "A Warden procession advances behind a volatile icebreaker",
                {EnemyType::Warden, EnemyType::Summoner, EnemyType::Charger,
                 EnemyType::Elite, EnemyType::Normal, EnemyType::Charger},
                6,
                {AffixTag::Cold, 1.40f, AffixTag::Survival, 1.15f},
                1,
                3,
                "Rime Processioner",
                "A rejuvenating volatile leader that turns every opening into a hazard",
                {EliteModifier::Rejuvenating, EliteModifier::Volatile},
                2.0f,
                2,
                2,
                {AffixTag::Cold, 1.75f, AffixTag::Survival, 1.25f},
                "Cold / Survival weighted | 2 guaranteed drops"
            }
        }};
        return packs;
    }

    static const EnemyPackDefinition& forMap(int templateIndex, int mapLevel, int sequence) {
        const int normalizedTemplate = ((templateIndex % ThemeCount) + ThemeCount) % ThemeCount;
        const int normalizedSequence = std::max(0, sequence) + std::max(1, mapLevel) - 1;
        const int index = normalizedTemplate * PacksPerTheme
            + (normalizedSequence % PacksPerTheme);
        return all()[static_cast<std::size_t>(index)];
    }
};
