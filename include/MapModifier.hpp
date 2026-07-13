#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "LootBias.hpp"

struct MapModifier {
    std::string name = "Quiet Coast";
    std::string description = "No modifier";
    float monsterHpMultiplier = 1.0f;
    int monsterDamageBonus = 0;
    float itemQuantityMultiplier = 1.0f;
    int bossDropBonus = 0;
    int eliteWeightBonus = 0;
    float bossHpMultiplier = 1.0f;
    float bossDamageMultiplier = 1.0f;
    int itemLevelBonus = 0;
    AffixTag lootBiasTag = AffixTag::None;
    float lootBiasWeightMultiplier = 1.0f;

    LootBias lootBias() const {
        return {lootBiasTag, lootBiasWeightMultiplier, AffixTag::None, 1.0f};
    }
};

struct MapOption {
    MapModifier modifier;
    std::string rewardDescription = "Baseline map rewards";
    std::string recommendedLevel = "Recommended level 1";
    int templateIndex = 0;
};

class MapOptionLibrary {
public:
    static MapOption defaultOption() {
        return {
            {"Quiet Coast", "No modifier", 1.0f, 0, 1.0f, 0, 0, 1.0f, 1.0f, 0,
                AffixTag::None, 1.0f},
            "Baseline monster density and loot",
            "Recommended level 1",
            0
        };
    }

    static std::array<MapOption, 3> generateOptions(int mapLevel) {
        const float levelBonus = static_cast<float>(std::max(0, mapLevel - 1));
        return {{
            {
                {
                    "Feral Foothills",
                    "More life and elites; Survival affixes favored",
                    1.15f + levelBonus * 0.05f,
                    mapLevel / 4,
                    1.15f + levelBonus * 0.03f,
                    0,
                    4,
                    1.05f,
                    1.0f,
                    0,
                    AffixTag::Survival,
                    1.35f
                },
                "+Survival affix weight, moderate elite pressure",
                "Recommended level " + std::to_string(mapLevel),
                0
            },
            {
                {
                    "Savage Hollow",
                    "More damage and elites; Damage affixes favored",
                    1.05f + levelBonus * 0.04f,
                    1 + mapLevel / 3,
                    1.25f + levelBonus * 0.04f,
                    1,
                    8,
                    1.10f,
                    1.15f,
                    0,
                    AffixTag::Damage,
                    1.35f
                },
                "+Damage affix weight, richer Boss drops",
                "Recommended level " + std::to_string(mapLevel + 1),
                1
            },
            {
                {
                    "Gilded Ruins",
                    "Tougher elites; Pickup affixes favored",
                    1.30f + levelBonus * 0.06f,
                    mapLevel / 5,
                    1.50f + levelBonus * 0.05f,
                    mapLevel >= 4 ? 1 : 0,
                    12,
                    1.20f,
                    1.05f,
                    1,
                    AffixTag::Pickup,
                    1.35f
                },
                "+Pickup affix weight, high item quantity and elite pressure",
                "Recommended level " + std::to_string(mapLevel + 1),
                2
            },
        }};
    }
};
