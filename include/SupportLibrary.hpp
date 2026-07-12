#pragma once

#include <string>
#include <vector>

#include "Skill.hpp"

enum class SupportKind {
    Pierce,
    Amplify,
    Quickcast,
    Volley,
    Trailblazer,
    Combustion,
    DeepChill
};

struct SupportDefinition {
    SupportKind kind = SupportKind::Pierce;
    std::string name;
    std::string description;
    float damageMultiplier = 1.0f;
    float radiusMultiplier = 1.0f;
    float cooldownMultiplier = 1.0f;
    int pierceCount = 0;
    int extraProjectileCount = 0;
    float extraSpreadAngle = 0.0f;
    int dashBaseDamage = 0;
    float dashRadius = 0.0f;
    float effectDuration = 0.0f;
    float ailmentDamageMultiplier = 1.0f;
    float ailmentDurationMultiplier = 1.0f;
    float chillMagnitudeMultiplier = 1.0f;
};

class SupportLibrary {
public:
    static const std::vector<SupportDefinition>& all() {
        static const std::vector<SupportDefinition> supports = {
            {SupportKind::Pierce, "Pierce", "+1 projectile pierce", 1.0f, 1.0f, 1.0f, 1},
            {SupportKind::Amplify, "Amplify", "+35% area radius, +20% cooldown", 1.0f, 1.35f, 1.20f, 0},
            {SupportKind::Quickcast, "Quickcast", "-30% cooldown, -15% damage", 0.85f, 1.0f, 0.70f, 0},
            {SupportKind::Volley, "Volley", "+2 projectiles, wider spread, -20% damage", 0.80f, 1.0f, 1.0f, 0, 2, 22.0f},
            {SupportKind::Trailblazer, "Trailblazer", "Dash impact: area damage at landing", 1.0f, 1.0f, 1.25f, 0, 0, 0.0f, 2, 90.0f, 0.30f},
            {SupportKind::Combustion, "Combustion", "-25% hit damage, +80% Ignite damage, +25% duration", 0.75f, 1.0f, 1.0f, 0, 0, 0.0f, 0, 0.0f, 0.0f, 1.80f, 1.25f},
            {SupportKind::DeepChill, "Deep Chill", "+50% Chill duration, stronger Chill, +15% cooldown", 1.0f, 1.0f, 1.15f, 0, 0, 0.0f, 0, 0.0f, 0.0f, 1.0f, 1.50f, 1.40f},
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
        switch (support.kind) {
            case SupportKind::Pierce:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Amplify:
                return skill.castType == SkillCastType::SelfCenteredArea
                    || skill.castType == SkillCastType::MouseTargetedArea;
            case SupportKind::Quickcast:
                return skill.castType != SkillCastType::Dash;
            case SupportKind::Volley:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Trailblazer:
                return skill.castType == SkillCastType::Dash;
            case SupportKind::Combustion:
                return skill.ailment.type == AilmentType::Ignite;
            case SupportKind::DeepChill:
                return skill.ailment.type == AilmentType::Chill;
        }

        return false;
    }
};
