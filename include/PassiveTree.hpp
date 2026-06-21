#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "Stats.hpp"

struct PassiveNode {
    std::string name;
    std::string description;
    Stats stats;
    int prerequisite = -1;
    bool allocated = false;
};

class PassiveTree {
public:
    PassiveTree() {
        // Projectile branch (0-4)
        nodes_[0] = {"Sharpened Bolt", "+8% damage", Stats{0, 1.0f, 1.08f, 1.0f, 1.0f}, -1, false};
        nodes_[1] = {"Rapid Fire", "+6% attack speed", Stats{0, 1.0f, 1.0f, 1.06f, 1.0f}, 0, false};
        nodes_[2] = {"Lethal Force", "+10% damage", Stats{0, 1.0f, 1.10f, 1.0f, 1.0f}, 1, false};
        nodes_[3] = {"Quick Reload", "+8% attack speed", Stats{0, 1.0f, 1.0f, 1.08f, 1.0f}, 2, false};
        nodes_[4] = {"Annihilation", "+12% damage", Stats{0, 1.0f, 1.12f, 1.0f, 1.0f}, 3, false};

        // Area branch (5-9)
        nodes_[5] = {"Inner Blaze", "+8% damage", Stats{0, 1.0f, 1.08f, 1.0f, 1.0f}, -1, false};
        nodes_[6] = {"Thick Skin", "+4 max HP", Stats{4, 1.0f, 1.0f, 1.0f, 1.0f}, 5, false};
        nodes_[7] = {"Blast Radius", "+10% damage", Stats{0, 1.0f, 1.10f, 1.0f, 1.0f}, 6, false};
        nodes_[8] = {"Sturdy Frame", "+5 max HP", Stats{5, 1.0f, 1.0f, 1.0f, 1.0f}, 7, false};
        nodes_[9] = {"Cataclysm", "+12% damage", Stats{0, 1.0f, 1.12f, 1.0f, 1.0f}, 8, false};

        // Survival branch (10-14)
        nodes_[10] = {"Vigour", "+5 max HP", Stats{5, 1.0f, 1.0f, 1.0f, 1.0f}, -1, false};
        nodes_[11] = {"Swift Foot", "+6% move speed", Stats{0, 1.06f, 1.0f, 1.0f, 1.0f}, 10, false};
        nodes_[12] = {"Iron Heart", "+6 max HP", Stats{6, 1.0f, 1.0f, 1.0f, 1.0f}, 11, false};
        nodes_[13] = {"Wind Runner", "+8% move speed", Stats{0, 1.08f, 1.0f, 1.0f, 1.0f}, 12, false};
        nodes_[14] = {"Unyielding", "+8 max HP", Stats{8, 1.0f, 1.0f, 1.0f, 1.0f}, 13, false};

        // Loot branch (15-19)
        nodes_[15] = {"Scavenger", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, -1, false};
        nodes_[16] = {"Hoarder", "+10% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.10f}, 15, false};
        nodes_[17] = {"Lucky Step", "+5% move speed", Stats{0, 1.05f, 1.0f, 1.0f, 1.0f}, 16, false};
        nodes_[18] = {"Far Reach", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, 17, false};
        nodes_[19] = {"Magnetism", "+20% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.20f}, 18, false};
    }

    bool allocate(std::size_t index) {
        if (index >= nodes_.size() || nodes_[index].allocated) {
            return false;
        }

        const int prerequisite = nodes_[index].prerequisite;
        if (prerequisite >= 0 && !nodes_[static_cast<std::size_t>(prerequisite)].allocated) {
            return false;
        }

        nodes_[index].allocated = true;
        return true;
    }

    Stats combinedStats() const {
        Stats result;
        for (const auto& node : nodes_) {
            if (node.allocated) {
                result = combineStats(result, node.stats);
            }
        }
        return result;
    }

    const std::array<PassiveNode, 20>& nodes() const {
        return nodes_;
    }

private:
    std::array<PassiveNode, 20> nodes_{};
};

