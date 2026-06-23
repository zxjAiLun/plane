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
};

struct MapOption {
    MapModifier modifier;
    std::string rewardDescription = "Baseline map rewards";
    std::string recommendedLevel = "Recommended level 1";
};

class MapOptionLibrary {
public:
    static MapOption defaultOption() {
        return {
            {"Quiet Coast", "No modifier", 1.0f, 0, 1.0f, 0},
            "Baseline monster density and loot",
            "Recommended level 1"
        };
    }

    static std::array<MapOption, 3> generateOptions(int mapLevel) {
        const float levelBonus = static_cast<float>(std::max(0, mapLevel - 1));
        return {{
            {
                {
                    "Feral Foothills",
                    "Monsters have more life; loot is slightly better",
                    1.15f + levelBonus * 0.05f,
                    mapLevel / 4,
                    1.15f + levelBonus * 0.03f,
                    0
                },
                "+Item quantity, moderate monster life",
                "Recommended level " + std::to_string(mapLevel)
            },
            {
                {
                    "Savage Hollow",
                    "Monsters hit harder; Boss drops more loot",
                    1.05f + levelBonus * 0.04f,
                    1 + mapLevel / 3,
                    1.25f + levelBonus * 0.04f,
                    1
                },
                "+Boss guaranteed drop, higher contact damage",
                "Recommended level " + std::to_string(mapLevel + 1)
            },
            {
                {
                    "Gilded Ruins",
                    "Monsters are tougher; items drop much more often",
                    1.30f + levelBonus * 0.06f,
                    mapLevel / 5,
                    1.50f + levelBonus * 0.05f,
                    mapLevel >= 4 ? 1 : 0
                },
                "High item quantity, high monster life",
                "Recommended level " + std::to_string(mapLevel + 1)
            },
        }};
    }
};
