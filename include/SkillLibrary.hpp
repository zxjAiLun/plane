#pragma once

#include <string>
#include <vector>

#include "Config.hpp"
#include "Skill.hpp"

class SkillLibrary {
public:
    static const std::vector<SkillDefinition>& all() {
        static const std::vector<SkillDefinition> skills = buildSkills();
        return skills;
    }

    static const SkillDefinition* find(const std::string& name) {
        for (const auto& skill : all()) {
            if (skill.name == name) {
                return &skill;
            }
        }
        return nullptr;
    }

    static SkillDefinition spreadShot() {
        return {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Spread Shot",
            Config::PrimarySkillCooldown,
            0.0f,
            Config::ProjectileDamage,
            0.0f,
            Config::SpreadShotProjectileCount,
            Config::SpreadShotSpreadAngle
        };
    }

    static SkillDefinition flare() {
        return {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Flare",
            Config::SecondarySkillCooldown,
            Config::SecondarySkillRadius,
            Config::SecondarySkillDamage,
            Config::SecondarySkillEffectDuration
        };
    }

    static SkillDefinition meteor() {
        return {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Meteor",
            Config::MeteorCooldown,
            Config::MeteorRadius,
            Config::MeteorDamage,
            Config::MeteorEffectDuration
        };
    }

    static SkillDefinition frostBomb() {
        return {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Frost Bomb",
            1.2f,
            100.0f,
            2,
            0.20f
        };
    }

    static SkillDefinition nova() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Nova",
            Config::NovaCooldown,
            Config::NovaRadius,
            Config::NovaDamage,
            Config::NovaEffectDuration
        };
    }

    static SkillDefinition pulse() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Pulse",
            Config::PulseCooldown,
            Config::PulseRadius,
            Config::PulseDamage,
            Config::PulseEffectDuration
        };
    }

    static SkillDefinition bladestorm() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Bladestorm",
            1.0f,
            70.0f,
            1,
            0.20f
        };
    }

    static SkillDefinition dash() {
        return {
            SkillSlot::Movement,
            SkillCastType::Dash,
            "Dash",
            Config::DashCooldown,
            0.0f,
            0,
            0.0f
        };
    }

private:
    static std::vector<SkillDefinition> buildSkills() {
        return {
            spreadShot(),
            flare(),
            meteor(),
            frostBomb(),
            nova(),
            pulse(),
            bladestorm(),
            dash()
        };
    }
};
