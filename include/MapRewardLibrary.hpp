#pragma once

#include <array>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include "SkillLibrary.hpp"

enum class MapRewardType {
    UnlockSkill,
    Damage,
    MaxHp,
    ItemQuantity
};

struct MapRewardDefinition {
    MapRewardType type = MapRewardType::Damage;
    std::string title;
    std::string description;
    std::string skillName;
    float itemQuantityMultiplierBonus = 1.0f;
};

class MapRewardLibrary {
public:
    static MapRewardDefinition skillUnlockReward(const SkillDefinition& skill) {
        return {
            MapRewardType::UnlockSkill,
            "Unlock " + skill.name,
            skillSlotLabel(skill.slot) + " / " + skillTypeLabel(skill.castType)
                + "  DMG " + std::to_string(skill.baseDamage)
                + "  R " + std::to_string(static_cast<int>(skill.radius))
                + "  CD " + std::to_string(static_cast<int>(skill.cooldown * 1000.0f)) + "ms",
            skill.name,
            1.0f
        };
    }

    static const std::array<MapRewardDefinition, 3>& fallbackRewards() {
        static const std::array<MapRewardDefinition, 3> rewards = {{
            {
                MapRewardType::Damage,
                "+20% Global Damage",
                "Permanent character damage bonus",
                "",
                1.0f
            },
            {
                MapRewardType::MaxHp,
                "+1 Max HP",
                "Permanent maximum health bonus",
                "",
                1.0f
            },
            {
                MapRewardType::ItemQuantity,
                "+15% Future Item Quantity",
                "Permanent item quantity bonus for future drops",
                "",
                1.15f
            }
        }};
        return rewards;
    }

    static std::array<MapRewardDefinition, 3> generateOptions(const std::set<std::string>& unlockedSkills) {
        std::vector<const SkillDefinition*> lockedSkills;
        for (const auto& skill : SkillLibrary::all()) {
            if (unlockedSkills.find(skill.name) == unlockedSkills.end()) {
                lockedSkills.push_back(&skill);
            }
        }

        std::array<MapRewardDefinition, 3> rewards{};
        std::size_t rewardIndex = 0;
        while (rewardIndex < rewards.size() && !lockedSkills.empty()) {
            const auto randomIndex = static_cast<std::size_t>(std::rand()) % lockedSkills.size();
            rewards[rewardIndex] = skillUnlockReward(*lockedSkills[randomIndex]);
            lockedSkills.erase(lockedSkills.begin() + randomIndex);
            ++rewardIndex;
        }

        const auto& fallbacks = fallbackRewards();
        std::size_t fallbackIndex = 0;
        while (rewardIndex < rewards.size()) {
            rewards[rewardIndex] = fallbacks[fallbackIndex % fallbacks.size()];
            ++rewardIndex;
            ++fallbackIndex;
        }

        return rewards;
    }

private:
    static std::string skillSlotLabel(SkillSlot slot) {
        switch (slot) {
            case SkillSlot::Primary: return "Primary";
            case SkillSlot::Secondary: return "Secondary";
            case SkillSlot::Utility: return "Utility";
            case SkillSlot::Movement: return "Movement";
            case SkillSlot::Count: break;
        }

        return "Unknown";
    }

    static std::string skillTypeLabel(SkillCastType type) {
        switch (type) {
            case SkillCastType::Projectile: return "Projectile";
            case SkillCastType::SelfCenteredArea: return "Self-centered area";
            case SkillCastType::MouseTargetedArea: return "Mouse-targeted area";
            case SkillCastType::Dash: return "Movement";
        }

        return "Unknown";
    }
};
