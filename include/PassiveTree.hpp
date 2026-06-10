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
        nodes_[0] = {"Path of Force", "+10% damage", Stats{0, 1.0f, 1.10f, 1.0f, 1.0f}, -1, false};
        nodes_[1] = {"Quick Hands", "+8% attack speed", Stats{0, 1.0f, 1.0f, 1.08f, 1.0f}, 0, false};
        nodes_[2] = {"Fleet Step", "+8% move speed", Stats{0, 1.08f, 1.0f, 1.0f, 1.0f}, 0, false};
        nodes_[3] = {"Iron Skin", "+3 max HP", Stats{3, 1.0f, 1.0f, 1.0f, 1.0f}, 1, false};
        nodes_[4] = {"Long Reach", "+20% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.20f}, 2, false};
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

    const std::array<PassiveNode, 5>& nodes() const {
        return nodes_;
    }

private:
    std::array<PassiveNode, 5> nodes_{};
};

