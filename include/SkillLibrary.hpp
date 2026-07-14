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
            Config::SpreadShotSpreadAngle,
            {},
            Config::PrimarySkillManaCost
        };
    }

    static SkillDefinition arcBolt() {
        SkillDefinition skill = {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Arc Bolt",
            0.65f,
            0.0f,
            3,
            0.0f,
            1,
            0.0f,
            {},
            2.0f
        };
        skill.damageType = DamageType::Lightning;
        return skill;
    }

    static SkillDefinition splitArrow() {
        return {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Split Arrow",
            Config::SplitArrowCooldown,
            0.0f,
            Config::SplitArrowDamage,
            0.0f,
            Config::SplitArrowProjectileCount,
            Config::SplitArrowSpreadAngle,
            {},
            Config::SplitArrowManaCost
        };
    }

    static SkillDefinition flare() {
        SkillDefinition skill = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Flare",
            Config::SecondarySkillCooldown,
            Config::SecondarySkillRadius,
            Config::SecondarySkillDamage,
            Config::SecondarySkillEffectDuration,
            1,
            0.0f,
            {AilmentType::Ignite, 2.5f, 0.50f},
            Config::FlareManaCost
        };
        skill.damageType = DamageType::Fire;
        return skill;
    }

    static SkillDefinition meteor() {
        SkillDefinition skill = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Meteor",
            Config::MeteorCooldown,
            Config::MeteorRadius,
            Config::MeteorDamage,
            Config::MeteorEffectDuration,
            1,
            0.0f,
            {AilmentType::Ignite, 3.5f, 0.50f},
            Config::MeteorManaCost
        };
        skill.damageType = DamageType::Fire;
        return skill;
    }

    static SkillDefinition frostBomb() {
        SkillDefinition skill = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Frost Bomb",
            1.2f,
            100.0f,
            2,
            0.20f,
            1,
            0.0f,
            {AilmentType::Chill, 2.5f, 0.0f, 0.55f},
            Config::FrostBombManaCost
        };
        skill.damageType = DamageType::Cold;
        return skill;
    }

    static SkillDefinition nova() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Nova",
            Config::NovaCooldown,
            Config::NovaRadius,
            Config::NovaDamage,
            Config::NovaEffectDuration,
            1,
            0.0f,
            {},
            Config::NovaManaCost
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
            Config::PulseEffectDuration,
            1,
            0.0f,
            {},
            Config::PulseManaCost
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
            0.20f,
            1,
            0.0f,
            {},
            Config::BladestormManaCost
        };
    }

    static SkillDefinition shockwave() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Shockwave",
            1.50f,
            120.0f,
            3,
            0.20f,
            1,
            0.0f,
            {},
            6.0f
        };
    }

    static SkillDefinition aftershock() {
        return {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Aftershock",
            Config::AftershockCooldown,
            Config::AftershockRadius,
            Config::AftershockDamage,
            Config::AftershockEffectDuration,
            1,
            0.0f,
            {},
            Config::AftershockManaCost
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
            0.0f,
            1,
            0.0f,
            {},
            Config::DashManaCost
        };
    }

private:
    static std::vector<SkillDefinition> buildSkills() {
        return {
            spreadShot(),
            arcBolt(),
            splitArrow(),
            flare(),
            meteor(),
            frostBomb(),
            nova(),
            pulse(),
            bladestorm(),
            shockwave(),
            aftershock(),
            dash()
        };
    }
};
