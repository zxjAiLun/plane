#pragma once

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <map>
#include <vector>

#include "Config.hpp"
#include "RandomService.hpp"
#include "SkillLibrary.hpp"
#include "SupportLibrary.hpp"

enum class MapRewardType {
    UnlockSkill,
    UnlockSupport,
    Damage,
    MaxHp,
    ItemQuantity,
    UpgradeSkill,
    UpgradeSupport
};

struct MapRewardDefinition {
    MapRewardType type = MapRewardType::Damage;
    std::string title;
    std::string description;
    std::string skillName;
    std::string supportName;
    float itemQuantityMultiplierBonus = 1.0f;
    int targetLevel = 1;
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
            "",
            1.0f,
            1
        };
    }

    static MapRewardDefinition supportUnlockReward(const SupportDefinition& support) {
        return {
            MapRewardType::UnlockSupport,
            "Unlock " + support.name,
            support.description,
            "",
            support.name,
            1.0f,
            1
        };
    }

    static MapRewardDefinition skillUpgradeReward(
        const SkillDefinition& skill,
        int targetLevel
    ) {
        return {
            MapRewardType::UpgradeSkill,
            "Empower " + skill.name + " to Lv" + std::to_string(targetLevel),
            skillSlotLabel(skill.slot) + " / " + skillTypeLabel(skill.castType)
                + "  Skill gem level upgrade",
            skill.name,
            "",
            1.0f,
            targetLevel
        };
    }

    static MapRewardDefinition supportUpgradeReward(
        const SupportDefinition& support,
        int targetLevel
    ) {
        return {
            MapRewardType::UpgradeSupport,
            "Empower " + support.name + " to Lv" + std::to_string(targetLevel),
            support.description + "  Support gem level upgrade",
            "",
            support.name,
            1.0f,
            targetLevel
        };
    }

    static const std::array<MapRewardDefinition, 3>& fallbackRewards() {
        static const std::array<MapRewardDefinition, 3> rewards = {{
            {
                MapRewardType::Damage,
                "+20% Global Damage",
                "Permanent character damage bonus",
                "",
                "",
                1.0f,
                1
            },
            {
                MapRewardType::MaxHp,
                "+1 Max HP",
                "Permanent maximum health bonus",
                "",
                "",
                1.0f,
                1
            },
            {
                MapRewardType::ItemQuantity,
                "+15% Future Item Quantity",
                "Permanent item quantity bonus for future drops",
                "",
                "",
                1.15f,
                1
            }
        }};
        return rewards;
    }

    static std::array<MapRewardDefinition, 3> generateOptions(
        const std::set<std::string>& unlockedSkills,
        const std::set<std::string>& unlockedSupports,
        RandomService& random
    ) {
        return generateOptionsImpl(
            unlockedSkills,
            unlockedSupports,
            {},
            {},
            1,
            random,
            false
        );
    }

    static std::array<MapRewardDefinition, 3> generateOptions(
        const std::set<std::string>& unlockedSkills,
        const std::set<std::string>& unlockedSupports,
        const std::map<std::string, int>& skillLevels,
        const std::map<std::string, int>& supportLevels,
        int mapLevel,
        RandomService& random
    ) {
        return generateOptionsImpl(
            unlockedSkills,
            unlockedSupports,
            skillLevels,
            supportLevels,
            mapLevel,
            random,
            true
        );
    }

    static std::array<MapRewardDefinition, 3> generateOptionsImpl(
        const std::set<std::string>& unlockedSkills,
        const std::set<std::string>& unlockedSupports,
        const std::map<std::string, int>& skillLevels,
        const std::map<std::string, int>& supportLevels,
        int mapLevel,
        RandomService& random,
        bool includeUpgrades
    ) {
        std::vector<const SkillDefinition*> lockedSkills;
        for (const auto& skill : SkillLibrary::all()) {
            if (unlockedSkills.find(skill.name) == unlockedSkills.end()) {
                lockedSkills.push_back(&skill);
            }
        }
        std::array<MapRewardDefinition, 3> rewards{};
        std::size_t rewardIndex = 0;
        std::vector<MapRewardDefinition> upgrades;
        if (includeUpgrades && mapLevel >= 2) {
            for (const auto& skill : SkillLibrary::all()) {
                const auto levelIt = skillLevels.find(skill.name);
                const int level = levelIt == skillLevels.end() ? 1 : levelIt->second;
                if (unlockedSkills.find(skill.name) != unlockedSkills.end()
                    && level < Config::SkillGemMaxLevel) {
                    upgrades.push_back(skillUpgradeReward(skill, level + 1));
                }
            }
            for (const auto& support : SupportLibrary::all()) {
                const auto levelIt = supportLevels.find(support.name);
                const int level = levelIt == supportLevels.end() ? 1 : levelIt->second;
                if (unlockedSupports.find(support.name) == unlockedSupports.end()
                    || level >= Config::SkillGemMaxLevel) {
                    continue;
                }
                const bool compatible = std::any_of(
                    SkillLibrary::all().begin(),
                    SkillLibrary::all().end(),
                    [&](const SkillDefinition& skill) {
                        return unlockedSkills.find(skill.name) != unlockedSkills.end()
                            && SupportLibrary::supportsSkill(support, skill);
                    }
                );
                if (compatible) {
                    upgrades.push_back(supportUpgradeReward(support, level + 1));
                }
            }
        }

        const std::size_t contentLimit = upgrades.empty()
            ? rewards.size()
            : rewards.size() - 1;
        while (rewardIndex < contentLimit && !lockedSkills.empty()) {
            const auto randomIndex = random.nextIndex(lockedSkills.size());
            rewards[rewardIndex] = skillUnlockReward(*lockedSkills[randomIndex]);
            lockedSkills.erase(lockedSkills.begin() + randomIndex);
            ++rewardIndex;
        }

        // Once the remaining skill pool no longer fills all three choices,
        // offer supports that can already modify an unlocked skill. This lets
        // a build start specializing before every skill in the library is found.
        std::vector<const SupportDefinition*> lockedSupports;
        for (const auto& support : SupportLibrary::all()) {
            if (unlockedSupports.find(support.name) != unlockedSupports.end()) {
                continue;
            }

            const bool matchesUnlockedSkill = std::any_of(
                SkillLibrary::all().begin(),
                SkillLibrary::all().end(),
                [&](const SkillDefinition& skill) {
                    return unlockedSkills.find(skill.name) != unlockedSkills.end()
                        && SupportLibrary::supportsSkill(support, skill);
                }
            );
            if (matchesUnlockedSkill) {
                lockedSupports.push_back(&support);
            }
        }

        while (rewardIndex < contentLimit && !lockedSupports.empty()) {
            const auto randomIndex = random.nextIndex(lockedSupports.size());
            rewards[rewardIndex] = supportUnlockReward(*lockedSupports[randomIndex]);
            lockedSupports.erase(lockedSupports.begin() + randomIndex);
            ++rewardIndex;
        }

        if (rewardIndex < rewards.size() && !upgrades.empty()) {
            const auto randomIndex = random.nextIndex(upgrades.size());
            rewards[rewardIndex] = upgrades[randomIndex];
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

    static std::array<MapRewardDefinition, 3> generateOptions(
        const std::set<std::string>& unlockedSkills,
        const std::set<std::string>& unlockedSupports
    ) {
        return generateOptions(unlockedSkills, unlockedSupports, RandomService::legacy());
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
