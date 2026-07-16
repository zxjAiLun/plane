#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <set>
#include <string>
#include <map>
#include <vector>

#include "Config.hpp"
#include "DamageType.hpp"
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
        MapRewardDefinition reward = {
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
        if (skill.healOnHit > 0) {
            reward.description += "  Heal " + std::to_string(skill.healOnHit)
                + " HP per enemy hit";
        }
        if (skill.selfDamageTakenMultiplier < 1.0f) {
            reward.description += "  Take "
                + std::to_string(static_cast<int>(skill.selfDamageTakenMultiplier * 100.0f))
                + "% damage for " + std::to_string(static_cast<int>(skill.effectDuration))
                + "s";
        }
        return reward;
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
            DamageType::Physical,
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
            DamageType::Physical,
            random,
            true
        );
    }

    static std::array<MapRewardDefinition, 3> generateOptions(
        const std::set<std::string>& unlockedSkills,
        const std::set<std::string>& unlockedSupports,
        const std::map<std::string, int>& skillLevels,
        const std::map<std::string, int>& supportLevels,
        int mapLevel,
        DamageType rewardTheme,
        RandomService& random
    ) {
        return generateOptionsImpl(
            unlockedSkills,
            unlockedSupports,
            skillLevels,
            supportLevels,
            mapLevel,
            rewardTheme,
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
        DamageType rewardTheme,
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

        // Boss themes should teach a build direction without removing the
        // normal random reward pool. Prefer one matching unlock first; when
        // the library is already unlocked, the upgrade pass below can provide
        // the matching gem instead.
        if (rewardTheme != DamageType::Physical && rewardIndex < contentLimit) {
            std::vector<std::size_t> themedSkillIndices;
            for (std::size_t index = 0; index < lockedSkills.size(); ++index) {
                if (lockedSkills[index]->damageType == rewardTheme) {
                    themedSkillIndices.push_back(index);
                }
            }
            if (!themedSkillIndices.empty()) {
                const std::size_t choice = random.nextIndex(themedSkillIndices.size());
                const std::size_t skillIndex = themedSkillIndices[choice];
                rewards[rewardIndex] = skillUnlockReward(*lockedSkills[skillIndex]);
                lockedSkills.erase(lockedSkills.begin()
                    + static_cast<std::ptrdiff_t>(skillIndex));
                ++rewardIndex;
            } else {
                std::vector<std::size_t> themedSupportIndices;
                for (std::size_t index = 0; index < lockedSupports.size(); ++index) {
                    if (supportMatchesTheme(*lockedSupports[index], rewardTheme)) {
                        themedSupportIndices.push_back(index);
                    }
                }
                if (!themedSupportIndices.empty()) {
                    const std::size_t choice = random.nextIndex(themedSupportIndices.size());
                    const std::size_t supportIndex = themedSupportIndices[choice];
                    rewards[rewardIndex] = supportUnlockReward(
                        *lockedSupports[supportIndex]);
                    lockedSupports.erase(lockedSupports.begin()
                        + static_cast<std::ptrdiff_t>(supportIndex));
                    ++rewardIndex;
                }
            }
        }

        // If the themed skill/support is already unlocked, use a matching
        // level upgrade before filling the remaining choices with unrelated
        // unlocks. This keeps later maps on the same build path.
        if (rewardTheme != DamageType::Physical && rewardIndex == 0
            && !upgrades.empty()) {
            std::vector<std::size_t> themedUpgradeIndices;
            for (std::size_t index = 0; index < upgrades.size(); ++index) {
                if (rewardMatchesTheme(upgrades[index], rewardTheme)) {
                    themedUpgradeIndices.push_back(index);
                }
            }
            if (!themedUpgradeIndices.empty()) {
                const std::size_t choice = random.nextIndex(themedUpgradeIndices.size());
                const std::size_t upgradeIndex = themedUpgradeIndices[choice];
                rewards[rewardIndex] = upgrades[upgradeIndex];
                upgrades.erase(upgrades.begin() + static_cast<std::ptrdiff_t>(upgradeIndex));
                ++rewardIndex;
            }
        }

        // Start offering one compatible Support as soon as the run reaches
        // map level two. A build should be able to combine a newly unlocked
        // skill with a meaningful link before the entire skill library is
        // exhausted.
        if (includeUpgrades && rewardIndex < contentLimit && !lockedSupports.empty()) {
            const std::size_t supportIndex = random.nextIndex(lockedSupports.size());
            rewards[rewardIndex] = supportUnlockReward(*lockedSupports[supportIndex]);
            lockedSupports.erase(
                lockedSupports.begin() + static_cast<std::ptrdiff_t>(supportIndex)
            );
            ++rewardIndex;
        }

        while (rewardIndex < contentLimit && !lockedSkills.empty()) {
            const auto randomIndex = random.nextIndex(lockedSkills.size());
            rewards[rewardIndex] = skillUnlockReward(*lockedSkills[randomIndex]);
            lockedSkills.erase(lockedSkills.begin() + randomIndex);
            ++rewardIndex;
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

    static bool supportMatchesTheme(const SupportDefinition& support, DamageType theme) {
        if (support.kind == SupportKind::ElementalFocus) {
            return support.requiredDamageType == theme;
        }

        switch (theme) {
            case DamageType::Fire:
                return support.kind == SupportKind::Combustion;
            case DamageType::Cold:
                return support.kind == SupportKind::DeepChill;
            case DamageType::Lightning:
                return support.kind == SupportKind::Conductivity;
            case DamageType::Poison:
                return support.kind == SupportKind::Toxicity
                    || support.kind == SupportKind::Contagion;
            case DamageType::Physical:
                return false;
        }
        return false;
    }

    static bool rewardMatchesTheme(const MapRewardDefinition& reward, DamageType theme) {
        if (reward.type == MapRewardType::UnlockSkill
            || reward.type == MapRewardType::UpgradeSkill) {
            const auto* skill = SkillLibrary::find(reward.skillName);
            return skill != nullptr && skill->damageType == theme;
        }

        if (reward.type == MapRewardType::UnlockSupport
            || reward.type == MapRewardType::UpgradeSupport) {
            const auto* support = SupportLibrary::find(reward.supportName);
            return support != nullptr && supportMatchesTheme(*support, theme);
        }

        return false;
    }
};
