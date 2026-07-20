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
            {AilmentType::Shock, 3.0f, 0.0f, 1.0f, 0, 0, 1.20f, 0},
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
        skill.delivery = SkillDeliveryType::DelayedArea;
        skill.castDelay = 0.55f;
        skill.groundHazard = {
            "Meteor Burning Ground",
            82.0f,
            Config::MeteorGroundHazardDuration,
            Config::MeteorGroundHazardTickInterval,
            Config::MeteorGroundHazardDamage,
            DamageType::Fire,
            {AilmentType::Ignite, 2.0f, 0.35f},
            GroundHazardTarget::Enemies
        };
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
        skill.delivery = SkillDeliveryType::DelayedArea;
        skill.castDelay = 0.20f;
        skill.groundHazard = {
            "Frost Bomb Chillfield",
            88.0f,
            Config::FrostBombGroundHazardDuration,
            Config::FrostBombGroundHazardTickInterval,
            Config::FrostBombGroundHazardDamage,
            DamageType::Cold,
            {AilmentType::Chill, 2.0f, 0.0f, 0.55f},
            GroundHazardTarget::Enemies
        };
        return skill;
    }

    static SkillDefinition toxicBurst() {
        SkillDefinition skill = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Toxic Burst",
            Config::ToxicBurstCooldown,
            Config::ToxicBurstRadius,
            Config::ToxicBurstDamage,
            Config::ToxicBurstEffectDuration,
            1,
            0.0f,
            {AilmentType::Poison, 3.5f, 0.35f},
            Config::ToxicBurstManaCost
        };
        skill.damageType = DamageType::Poison;
        skill.groundHazard = {
            "Toxic Mire",
            78.0f,
            Config::ToxicBurstGroundHazardDuration,
            Config::ToxicBurstGroundHazardTickInterval,
            Config::ToxicBurstGroundHazardDamage,
            DamageType::Poison,
            {AilmentType::Poison, 2.5f, 0.35f},
            GroundHazardTarget::Enemies
        };
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
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Pulse",
            Config::PulseCooldown,
            Config::PulseRadius,
            Config::PulseDamage,
            Config::PulseEffectDuration,
            1,
            0.0f,
            {AilmentType::Shock, 2.4f, 0.0f, 1.0f, 0, 0, 1.20f},
            Config::PulseManaCost
        };
        skill.damageType = DamageType::Lightning;
        return skill;
    }

    static SkillDefinition bladestorm() {
        SkillDefinition skill = {
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
        skill.delivery = SkillDeliveryType::RepeatingArea;
        skill.repeatCount = Config::BladestormHitCount;
        skill.repeatInterval = Config::BladestormHitInterval;
        return skill;
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

    static SkillDefinition summonWisp() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Summon Wisp",
            9.0f,
            0.0f,
            3,
            18.0f,
            1,
            0.0f,
            {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.12f},
            10.0f
        };
        skill.damageType = DamageType::Lightning;
        skill.summonCount = 2;
        skill.summonDuration = 18.0f;
        skill.summonDamage = 3;
        skill.summonMaxHp = 28;
        skill.summonAttackInterval = 0.95f;
        skill.summonAttackRange = 220.0f;
        return skill;
    }

    static SkillDefinition aftershock() {
        SkillDefinition skill = {
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
        skill.delivery = SkillDeliveryType::DelayedArea;
        skill.castDelay = Config::AftershockCastDelay;
        return skill;
    }

    static SkillDefinition emberLance() {
        SkillDefinition skill = {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Ember Lance",
            Config::EmberLanceCooldown,
            0.0f,
            Config::EmberLanceDamage,
            0.0f,
            1,
            0.0f,
            {AilmentType::Ignite, 3.0f, 0.45f},
            Config::EmberLanceManaCost
        };
        skill.damageType = DamageType::Fire;
        return skill;
    }

    static SkillDefinition glacialShard() {
        SkillDefinition skill = {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Glacial Shard",
            Config::GlacialShardCooldown,
            0.0f,
            Config::GlacialShardDamage,
            0.0f,
            Config::GlacialShardProjectileCount,
            Config::GlacialShardSpreadAngle,
            {AilmentType::Chill, 3.0f, 0.0f, 0.55f},
            Config::GlacialShardManaCost
        };
        skill.damageType = DamageType::Cold;
        return skill;
    }

    static SkillDefinition stormfield() {
        SkillDefinition skill = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Stormfield",
            Config::StormfieldCooldown,
            Config::StormfieldRadius,
            Config::StormfieldDamage,
            Config::StormfieldEffectDuration,
            1,
            0.0f,
            {AilmentType::Shock, 2.6f, 0.0f, 1.0f, 0, 0, 1.20f},
            Config::StormfieldManaCost
        };
        skill.damageType = DamageType::Lightning;
        skill.delivery = SkillDeliveryType::DelayedArea;
        skill.castDelay = Config::StormfieldCastDelay;
        skill.groundHazard = {
            "Stormfield Residue",
            86.0f,
            Config::StormfieldGroundHazardDuration,
            Config::StormfieldGroundHazardTickInterval,
            Config::StormfieldGroundHazardDamage,
            DamageType::Lightning,
            {AilmentType::Shock, 2.0f, 0.0f, 1.15f},
            GroundHazardTarget::Enemies
        };
        return skill;
    }

    static SkillDefinition blightRing() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Blight Ring",
            Config::BlightRingCooldown,
            Config::BlightRingRadius,
            Config::BlightRingDamage,
            Config::BlightRingEffectDuration,
            1,
            0.0f,
            {AilmentType::Poison, 3.2f, 0.35f},
            Config::BlightRingManaCost
        };
        skill.damageType = DamageType::Poison;
        skill.delivery = SkillDeliveryType::DelayedArea;
        skill.castDelay = Config::BlightRingCastDelay;
        skill.groundHazard = {
            "Blight Mire",
            92.0f,
            Config::BlightRingGroundHazardDuration,
            Config::BlightRingGroundHazardTickInterval,
            Config::BlightRingGroundHazardDamage,
            DamageType::Poison,
            {AilmentType::Poison, 2.5f, 0.35f},
            GroundHazardTarget::Enemies
        };
        return skill;
    }

    static SkillDefinition siphonPulse() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Siphon Pulse",
            Config::SiphonPulseCooldown,
            Config::SiphonPulseRadius,
            Config::SiphonPulseDamage,
            Config::SiphonPulseEffectDuration,
            1,
            0.0f,
            {AilmentType::Poison, 2.8f, 0.25f},
            Config::SiphonPulseManaCost
        };
        skill.damageType = DamageType::Poison;
        skill.healOnHit = Config::SiphonPulseHealOnHit;
        return skill;
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

    static SkillDefinition guardingPulse() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Guarding Pulse",
            Config::GuardingPulseCooldown,
            Config::GuardingPulseRadius,
            0,
            Config::GuardingPulseEffectDuration,
            1,
            0.0f,
            {},
            Config::GuardingPulseManaCost
        };
        skill.selfDamageTakenMultiplier = Config::GuardingPulseDamageTakenMultiplier;
        return skill;
    }

    static SkillDefinition manaWard() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Mana Ward",
            Config::ManaWardCooldown,
            Config::ManaWardRadius,
            0,
            Config::ManaWardEffectDuration,
            1,
            0.0f,
            {},
            Config::ManaWardManaCost
        };
        skill.wardManaRatio = Config::ManaWardManaRatio;
        return skill;
    }

    static SkillDefinition rendingVolley() {
        SkillDefinition skill = {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Rending Volley",
            Config::RendingVolleyCooldown,
            0.0f,
            Config::RendingVolleyDamage,
            0.0f,
            Config::RendingVolleyProjectileCount,
            Config::RendingVolleySpreadAngle,
            {AilmentType::Bleed, Config::RendingVolleyBleedDuration,
                Config::RendingVolleyBleedDamageMultiplier},
            Config::RendingVolleyManaCost
        };
        skill.damageType = DamageType::Physical;
        return skill;
    }

    static SkillDefinition crimsonSweep() {
        SkillDefinition skill = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Crimson Sweep",
            Config::CrimsonSweepCooldown,
            Config::CrimsonSweepRadius,
            Config::CrimsonSweepDamage,
            Config::CrimsonSweepEffectDuration,
            1,
            0.0f,
            {AilmentType::Bleed, Config::CrimsonSweepBleedDuration,
                Config::CrimsonSweepBleedDamageMultiplier},
            Config::CrimsonSweepManaCost
        };
        skill.damageType = DamageType::Physical;
        return skill;
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
            summonWisp(),
            aftershock(),
            toxicBurst(),
            emberLance(),
            glacialShard(),
            stormfield(),
            blightRing(),
            dash(),
            siphonPulse(),
            guardingPulse(),
            manaWard(),
            rendingVolley(),
            crimsonSweep()
        };
    }
};
