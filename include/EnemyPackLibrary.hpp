#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "EnemyType.hpp"
#include "LootBias.hpp"

struct EnemyPackDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::array<EnemyType, 6> enemies{};
    int enemyCount = 0;
    LootBias lootBias;
    int clearRewardDrops = 1;
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
                {AffixTag::Damage, 1.35f, AffixTag::Armor, 1.15f}
            },
            {
                "ashen-warden-court",
                "Ashen Warden Court",
                "A Warden anchors a slow, durable melee formation",
                {EnemyType::Warden, EnemyType::Normal, EnemyType::Normal,
                 EnemyType::Normal, EnemyType::Charger, EnemyType::Elite},
                6,
                {AffixTag::Survival, 1.35f, AffixTag::Armor, 1.20f}
            },
            {
                "ashen-last-stand",
                "Last Stand",
                "Elite pressure backed by a compact melee screen",
                {EnemyType::Elite, EnemyType::Normal, EnemyType::Normal,
                 EnemyType::Charger, EnemyType::Warden, EnemyType::Normal},
                6,
                {AffixTag::Damage, 1.40f, AffixTag::Area, 1.15f}
            },
            {
                "storm-spitter-screen",
                "Spitter Screen",
                "Ranged pressure forces movement through the open field",
                {EnemyType::Ranged, EnemyType::Ranged, EnemyType::Normal,
                 EnemyType::Ranged, EnemyType::Charger, EnemyType::Summoner},
                6,
                {AffixTag::Projectile, 1.40f, AffixTag::AttackSpeed, 1.15f}
            },
            {
                "storm-anchor-patrol",
                "Anchor Patrol",
                "A Hexbinder creates a protected ranged position",
                {EnemyType::Summoner, EnemyType::Ranged, EnemyType::Ranged,
                 EnemyType::Normal, EnemyType::Charger, EnemyType::Normal},
                6,
                {AffixTag::Lightning, 1.35f, AffixTag::Projectile, 1.20f}
            },
            {
                "storm-crossfire",
                "Crossfire Wing",
                "Chargers collapse on a ranged crossfire",
                {EnemyType::Ranged, EnemyType::Charger, EnemyType::Ranged,
                 EnemyType::Normal, EnemyType::Elite, EnemyType::Ranged},
                6,
                {AffixTag::AttackSpeed, 1.35f, AffixTag::Projectile, 1.20f}
            },
            {
                "venom-overgrowth",
                "Overgrowth",
                "Mixed elemental threats with a durable Warden",
                {EnemyType::Warden, EnemyType::Normal, EnemyType::Charger,
                 EnemyType::Normal, EnemyType::Summoner, EnemyType::Normal},
                6,
                {AffixTag::Poison, 1.40f, AffixTag::Area, 1.15f}
            },
            {
                "venom-hunter-nest",
                "Hunter Nest",
                "A Summoner and Chargers punish stationary builds",
                {EnemyType::Summoner, EnemyType::Charger, EnemyType::Charger,
                 EnemyType::Normal, EnemyType::Elite, EnemyType::Normal},
                6,
                {AffixTag::Poison, 1.35f, AffixTag::Damage, 1.20f}
            },
            {
                "venom-bloom-guard",
                "Bloom Guard",
                "Elite and Warden defenses protect a poison-leaning screen",
                {EnemyType::Elite, EnemyType::Warden, EnemyType::Normal,
                 EnemyType::Normal, EnemyType::Charger, EnemyType::Summoner},
                6,
                {AffixTag::Poison, 1.45f, AffixTag::Survival, 1.15f}
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
