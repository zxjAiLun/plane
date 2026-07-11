#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "Stats.hpp"
#include "Vector2.hpp"

enum class PassiveBranch {
    Projectile,
    Area,
    Survival,
    Loot
};

enum class PassiveNodeSize {
    Small,
    Notable
};

struct PassiveNode {
    std::string name;
    std::string description;
    Stats stats;
    int prerequisite = -1;
    bool allocated = false;
    Vector2 treePosition;
    PassiveBranch branch = PassiveBranch::Projectile;
    PassiveNodeSize size = PassiveNodeSize::Small;
};

class PassiveTree {
public:
    PassiveTree() {
        // Projectile branch (0-4)
        nodes_[0] = {"Sharpened Bolt", "+8% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, -1, false, {70.0f, -42.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[1] = {"Rapid Fire", "+6% attack speed", Stats{0, 1.0f, 1.0f, 1.06f, 1.0f}, 0, false, {135.0f, -78.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[2] = {"Lethal Force", "+10% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 1, false, {200.0f, -115.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[3] = {"Quick Reload", "+8% attack speed", Stats{0, 1.0f, 1.0f, 1.08f, 1.0f}, 2, false, {260.0f, -150.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[4] = {"Annihilation", "+14% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.14f}, 3, false, {318.0f, -184.0f}, PassiveBranch::Projectile, PassiveNodeSize::Notable};

        // Area branch (5-9)
        nodes_[5] = {"Inner Blaze", "+8% area damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, -1, false, {-70.0f, -42.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[6] = {"Widening Circle", "+8% area radius", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, 5, false, {-135.0f, -78.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[7] = {"Blast Force", "+10% area damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 6, false, {-200.0f, -115.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[8] = {"Expanded Impact", "+10% area radius", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 7, false, {-260.0f, -150.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[9] = {"Cataclysm", "+14% area damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.14f}, 8, false, {-318.0f, -184.0f}, PassiveBranch::Area, PassiveNodeSize::Notable};

        // Survival branch (10-14)
        nodes_[10] = {"Vigour", "+5 max HP", Stats{5, 1.0f, 1.0f, 1.0f, 1.0f}, -1, false, {-70.0f, 42.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[11] = {"Stone Skin", "+1 armor", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1}, 10, false, {-135.0f, 78.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[12] = {"Iron Heart", "+6 max HP and +1 armor", Stats{6, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1}, 11, false, {-200.0f, 115.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[13] = {"Wind Runner", "+8% move speed", Stats{0, 1.08f, 1.0f, 1.0f, 1.0f}, 12, false, {-260.0f, 150.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[14] = {"Unyielding", "+8 max HP and +2 armor", Stats{8, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2}, 13, false, {-318.0f, 184.0f}, PassiveBranch::Survival, PassiveNodeSize::Notable};

        // Loot branch (15-19)
        nodes_[15] = {"Scavenger", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, -1, false, {70.0f, 42.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[16] = {"Hoarder", "+10% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.10f}, 15, false, {135.0f, 78.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[17] = {"Lucky Step", "+5% move speed", Stats{0, 1.05f, 1.0f, 1.0f, 1.0f}, 16, false, {200.0f, 115.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[18] = {"Far Reach", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, 17, false, {260.0f, 150.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[19] = {"Magnetism", "+20% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.20f}, 18, false, {318.0f, 184.0f}, PassiveBranch::Loot, PassiveNodeSize::Notable};
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

    int nodeAtPosition(const Vector2& treePosition, float radius) const {
        int bestIndex = -1;
        float bestDistance = 1000000000.0f;
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            const float nodeRadius = nodes_[i].size == PassiveNodeSize::Notable ? radius * 1.25f : radius;
            const float allowedDistance = nodeRadius * nodeRadius;
            const float distance = (treePosition - nodes_[i].treePosition).lengthSquared();
            if (distance <= allowedDistance && distance < bestDistance) {
                bestDistance = distance;
                bestIndex = static_cast<int>(i);
            }
        }
        return bestIndex;
    }

    int allocatedCount(PassiveBranch branch) const {
        int count = 0;
        for (const auto& node : nodes_) {
            if (node.allocated && node.branch == branch) {
                ++count;
            }
        }
        return count;
    }

private:
    std::array<PassiveNode, 20> nodes_{};
};
