#pragma once

#include <array>
#include <cstddef>
#include <string>

struct AtlasPassiveEffect {
    float itemQuantityMultiplierBonus = 1.0f;
    float itemRarityMultiplierBonus = 1.0f;
    int eliteWeightBonus = 0;
    int bossDropBonus = 0;
};

enum class AtlasBranch {
    Surveyor,
    Cartographer,
    Huntmaster,
    Conqueror
};

struct AtlasPassiveNode {
    int index = -1;
    const char* name = "";
    const char* description = "";
    AtlasBranch branch = AtlasBranch::Surveyor;
    int prerequisite = -1;
    AtlasPassiveEffect effect;
};

class AtlasPassiveLibrary {
public:
    static constexpr std::size_t NodeCount = 12;

    static const std::array<AtlasPassiveNode, NodeCount>& all() {
        static const std::array<AtlasPassiveNode, NodeCount> nodes = {{
            {0, "Surveyor's Eye", "+3% map item quantity", AtlasBranch::Surveyor, -1,
                {1.03f, 1.0f, 0, 0}},
            {1, "Surveyor's Reach", "+3% map item quantity", AtlasBranch::Surveyor, 0,
                {1.03f, 1.0f, 0, 0}},
            {2, "Surveyor's Fortune", "+3% map item quantity", AtlasBranch::Surveyor, 1,
                {1.03f, 1.0f, 0, 0}},
            {3, "Cartographer's Sense", "+3% map item rarity", AtlasBranch::Cartographer, -1,
                {1.0f, 1.03f, 0, 0}},
            {4, "Cartographer's Detail", "+3% map item rarity", AtlasBranch::Cartographer, 3,
                {1.0f, 1.03f, 0, 0}},
            {5, "Cartographer's Archive", "+3% map item rarity", AtlasBranch::Cartographer, 4,
                {1.0f, 1.03f, 0, 0}},
            {6, "Huntmaster's Trail", "+1 elite encounter weight", AtlasBranch::Huntmaster, -1,
                {1.0f, 1.0f, 1, 0}},
            {7, "Huntmaster's Call", "+1 elite encounter weight", AtlasBranch::Huntmaster, 6,
                {1.0f, 1.0f, 1, 0}},
            {8, "Huntmaster's Menagerie", "+1 elite encounter weight", AtlasBranch::Huntmaster, 7,
                {1.0f, 1.0f, 1, 0}},
            {9, "Conqueror's Mark", "+1 Boss drop", AtlasBranch::Conqueror, -1,
                {1.0f, 1.0f, 0, 1}},
            {10, "Conqueror's Claim", "+1 Boss drop", AtlasBranch::Conqueror, 9,
                {1.0f, 1.0f, 0, 1}},
            {11, "Conqueror's Spoils", "+1 Boss drop", AtlasBranch::Conqueror, 10,
                {1.0f, 1.0f, 0, 1}}
        }};
        return nodes;
    }

    static const AtlasPassiveNode* find(int index) {
        if (index < 0 || index >= static_cast<int>(NodeCount)) {
            return nullptr;
        }
        return &all()[static_cast<std::size_t>(index)];
    }

    static const char* branchName(AtlasBranch branch) {
        switch (branch) {
            case AtlasBranch::Surveyor: return "Surveyor";
            case AtlasBranch::Cartographer: return "Cartographer";
            case AtlasBranch::Huntmaster: return "Huntmaster";
            case AtlasBranch::Conqueror: return "Conqueror";
        }
        return "Atlas";
    }
};
