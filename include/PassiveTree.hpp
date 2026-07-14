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
    Loot,
    Poison
};

enum class PassiveNodeSize {
    Small,
    Notable
};

enum class PassiveKeystone {
    None,
    VolleyDoctrine,
    ConcentratedImpact,
    SecondWind,
    LoadedDice
};

inline const char* passiveKeystoneName(PassiveKeystone keystone) {
    switch (keystone) {
        case PassiveKeystone::None: return "None";
        case PassiveKeystone::VolleyDoctrine: return "Volley Doctrine";
        case PassiveKeystone::ConcentratedImpact: return "Concentrated Impact";
        case PassiveKeystone::SecondWind: return "Second Wind";
        case PassiveKeystone::LoadedDice: return "Loaded Dice";
    }
    return "Unknown";
}

struct PassiveNode {
    std::string name;
    std::string description;
    Stats stats;
    int prerequisite = -1;
    bool allocated = false;
    Vector2 treePosition;
    PassiveBranch branch = PassiveBranch::Projectile;
    PassiveNodeSize size = PassiveNodeSize::Small;
    PassiveKeystone keystone = PassiveKeystone::None;
};

class PassiveTree {
public:
    static constexpr std::size_t LegacyNodeCount = 20;
    static constexpr std::size_t NodeCount = 25;

    PassiveTree() {
        // Projectile branch (0-4)
        nodes_[0] = {"Sharpened Bolt", "+8% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, -1, false, {70.0f, -42.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[1] = {"Rapid Fire", "+6% attack speed", Stats{0, 1.0f, 1.0f, 1.06f, 1.0f}, 0, false, {135.0f, -78.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[2] = {"Lethal Force", "+10% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 1, false, {200.0f, -115.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[3] = {"Quick Reload", "+8% attack speed", Stats{0, 1.0f, 1.0f, 1.08f, 1.0f}, 2, false, {260.0f, -150.0f}, PassiveBranch::Projectile, PassiveNodeSize::Small};
        nodes_[4] = {"Volley Doctrine", "+2 projectiles, -25% projectile damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 0.75f, 1.0f, 1.0f, 0, 2, 1.0f, 1.0f, 1.0f}, 3, false, {318.0f, -184.0f}, PassiveBranch::Projectile, PassiveNodeSize::Notable, PassiveKeystone::VolleyDoctrine};

        // Area branch (5-9)
        nodes_[5] = {"Inner Blaze", "+8% area damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, -1, false, {-70.0f, -42.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[6] = {"Widening Circle", "+8% area radius", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f}, 5, false, {-135.0f, -78.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[7] = {"Blast Force", "+10% area damage", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 6, false, {-200.0f, -115.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[8] = {"Expanded Impact", "+10% area radius", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f}, 7, false, {-260.0f, -150.0f}, PassiveBranch::Area, PassiveNodeSize::Small};
        nodes_[9] = {"Concentrated Impact", "+35% area damage, -25% area radius", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.35f, 0.75f}, 8, false, {-318.0f, -184.0f}, PassiveBranch::Area, PassiveNodeSize::Notable, PassiveKeystone::ConcentratedImpact};

        // Survival branch (10-14)
        nodes_[10] = {"Vigour", "+5 max HP", Stats{5, 1.0f, 1.0f, 1.0f, 1.0f}, -1, false, {-70.0f, 42.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[11] = {"Stone Skin", "+1 armor", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1}, 10, false, {-135.0f, 78.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[12] = {"Iron Heart", "+6 max HP and +1 armor", Stats{6, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1}, 11, false, {-200.0f, 115.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[13] = {"Wind Runner", "+8% move speed", Stats{0, 1.08f, 1.0f, 1.0f, 1.0f}, 12, false, {-260.0f, 150.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[14] = {"Second Wind", "+50% life flask healing", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0, 1.5f}, 13, false, {-318.0f, 184.0f}, PassiveBranch::Survival, PassiveNodeSize::Notable, PassiveKeystone::SecondWind};

        // Loot branch (15-19)
        nodes_[15] = {"Scavenger", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, -1, false, {70.0f, 42.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[16] = {"Hoarder", "+10% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.10f}, 15, false, {135.0f, 78.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[17] = {"Lucky Step", "+5% move speed", Stats{0, 1.05f, 1.0f, 1.0f, 1.0f}, 16, false, {200.0f, 115.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[18] = {"Far Reach", "+15% pickup range", Stats{0, 1.0f, 1.0f, 1.0f, 1.15f}, 17, false, {260.0f, 150.0f}, PassiveBranch::Loot, PassiveNodeSize::Small};
        nodes_[19] = {"Loaded Dice", "+25% item drops, +20% damage taken", Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0, 1.0f, 1.25f, 1.20f}, 18, false, {318.0f, 184.0f}, PassiveBranch::Loot, PassiveNodeSize::Notable, PassiveKeystone::LoadedDice};

        // Poison branch (20-24)
        nodes_[20] = {"Toxic Skin", "+8% Poison damage", poisonStats(1.08f), -1, false, {70.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Small};
        nodes_[21] = {"Venomous Focus", "+10% Poison damage", poisonStats(1.10f), 20, false, {135.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Small};
        nodes_[22] = {"Antidote Veins", "+12% Poison resistance", poisonStats(1.0f, 12), 21, false, {200.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Small};
        nodes_[23] = {"Lingering Rot", "+12% Poison damage", poisonStats(1.12f), 22, false, {260.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Small};
        nodes_[24] = {"Toxic Bloom", "+30% Poison damage and +10% Poison resistance", poisonStats(1.30f, 10), 23, false, {318.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Notable};
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

    const std::array<PassiveNode, NodeCount>& nodes() const {
        return nodes_;
    }

    std::array<bool, NodeCount> allocatedNodes() const {
        std::array<bool, NodeCount> result{};
        for (std::size_t index = 0; index < nodes_.size(); ++index) {
            result[index] = nodes_[index].allocated;
        }
        return result;
    }

    bool restoreAllocatedNodes(const std::array<bool, NodeCount>& allocated) {
        for (std::size_t index = 0; index < nodes_.size(); ++index) {
            if (!allocated[index]) {
                continue;
            }

            const int prerequisite = nodes_[index].prerequisite;
            if (prerequisite >= 0
                && (static_cast<std::size_t>(prerequisite) >= allocated.size()
                    || !allocated[static_cast<std::size_t>(prerequisite)])) {
                return false;
            }
        }

        for (std::size_t index = 0; index < nodes_.size(); ++index) {
            nodes_[index].allocated = allocated[index];
        }
        return true;
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

    bool hasKeystone(PassiveKeystone keystone) const {
        if (keystone == PassiveKeystone::None) {
            return false;
        }

        for (const auto& node : nodes_) {
            if (node.allocated && node.keystone == keystone) {
                return true;
            }
        }
        return false;
    }

    std::string keystoneSummary() const {
        std::string summary;
        for (const auto& node : nodes_) {
            if (!node.allocated || node.keystone == PassiveKeystone::None) {
                continue;
            }
            if (!summary.empty()) {
                summary += ", ";
            }
            summary += passiveKeystoneName(node.keystone);
        }
        return summary.empty() ? "None" : summary;
    }

private:
    static Stats poisonStats(float poisonDamageMultiplier, int poisonResistance = 0) {
        Stats stats;
        stats.poisonDamageMultiplier = poisonDamageMultiplier;
        stats.poisonResistance = poisonResistance;
        return stats;
    }

    std::array<PassiveNode, NodeCount> nodes_{};
};
