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
};

class EnemyPackLibrary {
public:
    static constexpr int ThemeCount = 3;
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
                1,
                2
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
                1,
                2
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
                2
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
                1,
                2
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
                2
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
                1,
                2
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
                1,
                2
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
                2
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
                1,
                2
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
