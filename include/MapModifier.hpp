#pragma once

#include <algorithm>
#include <array>
#include <string>

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
            {"Quiet Coast", "No modifier", 1.0f, 0, 1.0f, 0, 0, 1.0f, 1.0f, 0},
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
                    "More life and elites; better loot",
                    1.15f + levelBonus * 0.05f,
                    mapLevel / 4,
                    1.15f + levelBonus * 0.03f,
                    0,
                    4,
                    1.05f,
                    1.0f,
                    0
                },
                "+Item quantity, moderate elite pressure",
                "Recommended level " + std::to_string(mapLevel),
                0
            },
            {
                {
                    "Savage Hollow",
                    "More damage and elites; richer Boss drops",
                    1.05f + levelBonus * 0.04f,
                    1 + mapLevel / 3,
                    1.25f + levelBonus * 0.04f,
                    1,
                    8,
                    1.10f,
                    1.15f,
                    0
                },
                "+Boss guaranteed drop, dangerous Boss skills",
                "Recommended level " + std::to_string(mapLevel + 1),
                1
            },
            {
                {
                    "Gilded Ruins",
                    "Tougher elites; higher iLvl loot",
                    1.30f + levelBonus * 0.06f,
                    mapLevel / 5,
                    1.50f + levelBonus * 0.05f,
                    mapLevel >= 4 ? 1 : 0,
                    12,
                    1.20f,
                    1.05f,
                    1
                },
                "+Item level, high item quantity, high elite pressure",
                "Recommended level " + std::to_string(mapLevel + 1),
                2
            },
        }};
    }
};
