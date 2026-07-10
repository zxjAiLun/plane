#pragma once

#include <string>
#include <vector>

#include "Skill.hpp"

enum class SupportKind {
    Pierce,
    Amplify,
    Quickcast
};

struct SupportDefinition {
    SupportKind kind = SupportKind::Pierce;
    std::string name;
    std::string description;
    float damageMultiplier = 1.0f;
    float radiusMultiplier = 1.0f;
    float cooldownMultiplier = 1.0f;
    int pierceCount = 0;
};

class SupportLibrary {
public:
    static const std::vector<SupportDefinition>& all() {
        static const std::vector<SupportDefinition> supports = {
            {SupportKind::Pierce, "Pierce", "+1 projectile pierce", 1.0f, 1.0f, 1.0f, 1},
            {SupportKind::Amplify, "Amplify", "+35% area radius, +20% cooldown", 1.0f, 1.35f, 1.20f, 0},
            {SupportKind::Quickcast, "Quickcast", "-30% cooldown, -15% damage", 0.85f, 1.0f, 0.70f, 0},
        };
        return supports;
    }

    static const SupportDefinition* find(const std::string& name) {
        for (const auto& support : all()) {
            if (support.name == name) {
                return &support;
            }
        }
        return nullptr;
    }

    static bool supportsSkill(const SupportDefinition& support, const SkillDefinition& skill) {
        if (skill.slot == SkillSlot::Movement) {
            return false;
        }

        switch (support.kind) {
            case SupportKind::Pierce:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Amplify:
                return skill.castType == SkillCastType::SelfCenteredArea
                    || skill.castType == SkillCastType::MouseTargetedArea;
            case SupportKind::Quickcast:
                return true;
        }

        return false;
    }
};
