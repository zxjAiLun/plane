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
    Poison,
    Fire,
    Cold,
    Lightning,
    PhysicalBleed
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
    static constexpr std::size_t PreviousNodeCount = 25;
    static constexpr std::size_t PrePhysicalBleedNodeCount = 28;
    static constexpr std::size_t NodeCount = 33;

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

        // Survival branch (10-14): life, mitigation, and a deliberate Mana sustain route.
        nodes_[10] = {"Vigour", "+5 max HP", survivalStats(5), -1, false, {-70.0f, 42.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[11] = {"Arcane Guard", "+1 armor, +8% max Mana", survivalStats(0, 1, 1.0f, 1.08f), 10, false, {-135.0f, 78.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[12] = {"Iron Heart", "+6 max HP, +1 armor, +5% max Mana", survivalStats(6, 1, 1.0f, 1.05f), 11, false, {-200.0f, 115.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[13] = {"Meditative Stride", "+8% move speed, +12% Mana regen", survivalStats(0, 0, 1.08f, 1.0f, 1.12f), 12, false, {-260.0f, 150.0f}, PassiveBranch::Survival, PassiveNodeSize::Small};
        nodes_[14] = {"Second Wind", "+50% life flask healing, +20% Mana regen, -8% skill costs", survivalStats(0, 0, 1.0f, 1.0f, 1.20f, 1.50f, 0.92f), 13, false, {-318.0f, 184.0f}, PassiveBranch::Survival, PassiveNodeSize::Notable, PassiveKeystone::SecondWind};

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
        nodes_[24] = {"Toxic Bloom", "+30% Poison damage, +20% Poison duration, +10% Poison resistance", poisonStats(1.30f, 10, 1.20f), 23, false, {318.0f, 0.0f}, PassiveBranch::Poison, PassiveNodeSize::Notable};

        // Keep the existing Poison route stable and add independent elemental roots.
        nodes_[25] = {"Ember Attunement", "+18% Fire damage, +15% Ignite damage, +5 Fire resistance", fireStats(1.18f, 5, 1.15f, 1.10f), -1, false, {0.0f, -196.0f}, PassiveBranch::Fire, PassiveNodeSize::Notable};
        nodes_[26] = {"Glacial Attunement", "+18% Cold damage, +15% Chill effect/duration, +5 Cold resistance", coldStats(1.18f, 5, 1.15f, 1.15f), -1, false, {0.0f, 196.0f}, PassiveBranch::Cold, PassiveNodeSize::Notable};
        nodes_[27] = {"Storm Attunement", "+18% Lightning damage, +15% Shock effect/duration, +5 Lightning resistance", lightningStats(1.18f, 5, 1.15f, 1.15f), -1, false, {-70.0f, 0.0f}, PassiveBranch::Lightning, PassiveNodeSize::Notable};

        // Physical / Bleed branch: a separate damage-over-time route for
        // builds that use Hemorrhage skills and physical hit scaling.
        nodes_[28] = {"Bloodied Edge", "+8% Physical damage", physicalBleedStats(1.08f), -1, false, {-135.0f, 0.0f}, PassiveBranch::PhysicalBleed, PassiveNodeSize::Small};
        nodes_[29] = {"Open Veins", "+10% Bleed damage", physicalBleedStats(1.0f, 1.10f), 28, false, {-200.0f, 0.0f}, PassiveBranch::PhysicalBleed, PassiveNodeSize::Small};
        nodes_[30] = {"Deep Cuts", "+12% Bleed duration, +4% Bleed penetration", physicalBleedStats(1.0f, 1.0f, 1.12f, 4), 29, false, {-260.0f, 0.0f}, PassiveBranch::PhysicalBleed, PassiveNodeSize::Small};
        nodes_[31] = {"Sundering Wounds", "+6% Physical damage, +4% Bleed penetration", physicalBleedStats(1.06f, 1.0f, 1.0f, 4), 30, false, {-318.0f, 0.0f}, PassiveBranch::PhysicalBleed, PassiveNodeSize::Small};
        nodes_[32] = {"Hemorrhagic Momentum", "+20% Physical damage, +20% Bleed damage, +15% Bleed duration, +8% Bleed penetration", physicalBleedStats(1.20f, 1.20f, 1.15f, 8), 31, false, {-350.0f, -42.0f}, PassiveBranch::PhysicalBleed, PassiveNodeSize::Notable};
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
    static Stats survivalStats(
        int maxHp = 0,
        int armor = 0,
        float moveSpeedMultiplier = 1.0f,
        float maxManaMultiplier = 1.0f,
        float manaRegenMultiplier = 1.0f,
        float lifeFlaskEffectMultiplier = 1.0f,
        float skillCostMultiplier = 1.0f
    ) {
        Stats stats;
        stats.maxHp = maxHp;
        stats.armor = armor;
        stats.moveSpeedMultiplier = moveSpeedMultiplier;
        stats.maxManaMultiplier = maxManaMultiplier;
        stats.manaRegenMultiplier = manaRegenMultiplier;
        stats.lifeFlaskEffectMultiplier = lifeFlaskEffectMultiplier;
        stats.skillCostMultiplier = skillCostMultiplier;
        return stats;
    }

    static Stats poisonStats(
        float poisonDamageMultiplier,
        int poisonResistance = 0,
        float poisonDurationMultiplier = 1.0f
    ) {
        Stats stats;
        stats.poisonDamageMultiplier = poisonDamageMultiplier;
        stats.poisonResistance = poisonResistance;
        stats.poisonDurationMultiplier = poisonDurationMultiplier;
        return stats;
    }

    static Stats fireStats(
        float damageMultiplier,
        int resistance,
        float igniteDamageMultiplier = 1.0f,
        float igniteDurationMultiplier = 1.0f
    ) {
        Stats stats;
        stats.fireDamageMultiplier = damageMultiplier;
        stats.fireResistance = resistance;
        stats.igniteDamageMultiplier = igniteDamageMultiplier;
        stats.igniteDurationMultiplier = igniteDurationMultiplier;
        return stats;
    }

    static Stats coldStats(
        float damageMultiplier,
        int resistance,
        float chillMagnitudeMultiplier = 1.0f,
        float chillDurationMultiplier = 1.0f
    ) {
        Stats stats;
        stats.coldDamageMultiplier = damageMultiplier;
        stats.coldResistance = resistance;
        stats.chillMagnitudeMultiplier = chillMagnitudeMultiplier;
        stats.chillDurationMultiplier = chillDurationMultiplier;
        return stats;
    }

    static Stats lightningStats(
        float damageMultiplier,
        int resistance,
        float shockMagnitudeMultiplier = 1.0f,
        float shockDurationMultiplier = 1.0f
    ) {
        Stats stats;
        stats.lightningDamageMultiplier = damageMultiplier;
        stats.lightningResistance = resistance;
        stats.shockMagnitudeMultiplier = shockMagnitudeMultiplier;
        stats.shockDurationMultiplier = shockDurationMultiplier;
        return stats;
    }

    static Stats physicalBleedStats(
        float physicalDamageMultiplier = 1.0f,
        float bleedDamageMultiplier = 1.0f,
        float bleedDurationMultiplier = 1.0f,
        int bleedPenetration = 0
    ) {
        Stats stats;
        stats.physicalDamageMultiplier = physicalDamageMultiplier;
        stats.bleedDamageMultiplier = bleedDamageMultiplier;
        stats.bleedDurationMultiplier = bleedDurationMultiplier;
        stats.bleedPenetration = bleedPenetration;
        return stats;
    }

    std::array<PassiveNode, NodeCount> nodes_{};
};
