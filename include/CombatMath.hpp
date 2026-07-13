#pragma once

#include <algorithm>
#include <cmath>

#include "Skill.hpp"
#include "Stats.hpp"
#include "SupportLibrary.hpp"

// Pure combat math used by GameWorld skill casting and player mitigation.
// Kept header-only so unit tests can drive the same formulas without SFML.

inline int mitigatedDamage(int rawDamage, int armor) {
    return std::max(1, rawDamage - armor);
}

inline int lifeFlaskHealAmount(int baseAmount, const Stats& stats) {
    return std::max(0, static_cast<int>(std::ceil(
        static_cast<float>(baseAmount) * stats.lifeFlaskEffectMultiplier
    )));
}

inline int incomingDamage(int rawDamage, const Stats& stats) {
    if (rawDamage <= 0) {
        return 0;
    }
    return std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(rawDamage) * stats.incomingDamageMultiplier
    )));
}

inline int itemDropChancePercent(
    int baseChancePercent,
    float mapQuantityMultiplier,
    const Stats& stats
) {
    return std::clamp(static_cast<int>(
        static_cast<float>(baseChancePercent)
            * mapQuantityMultiplier
            * stats.itemQuantityMultiplier
    ), 0, 100);
}

inline int refilledFlaskCharges(int currentCharges, int maxCharges, int restoredCharges) {
    return std::clamp(currentCharges + std::max(0, restoredCharges), 0, maxCharges);
}

inline int availableBossSummonCount(int requested, int active, int maximum) {
    return std::clamp(requested, 0, std::max(0, maximum - std::max(0, active)));
}

inline int skillDamage(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support,
    float shrineMultiplier = 1.0f
) {
    float damage = static_cast<float>(skill.baseDamage) * stats.damageMultiplier;
    switch (skill.castType) {
        case SkillCastType::Projectile:
            damage *= stats.projectileDamageMultiplier;
            break;
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            damage *= stats.areaDamageMultiplier;
            break;
        case SkillCastType::Dash:
            break;
    }

    if (support) {
        damage *= support->damageMultiplier;
    }

    return std::max(1, static_cast<int>(std::ceil(damage * shrineMultiplier)));
}

inline float skillRadius(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support
) {
    const float supportMultiplier = support ? support->radiusMultiplier : 1.0f;
    switch (skill.castType) {
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            return skill.radius * stats.areaRadiusMultiplier * supportMultiplier;
        case SkillCastType::Projectile:
        case SkillCastType::Dash:
            return skill.radius;
    }
    return skill.radius;
}

inline int skillPierceCount(const SupportDefinition* support) {
    return support ? support->pierceCount : 0;
}

inline int skillProjectileCount(const SkillDefinition& skill, const SupportDefinition* support) {
    return std::max(1, skill.projectileCount + (support ? support->extraProjectileCount : 0));
}

inline int skillProjectileCount(
    const SkillDefinition& skill,
    const SupportDefinition* support,
    const Stats& stats
) {
    return std::max(1, skill.projectileCount
        + (support ? support->extraProjectileCount : 0)
        + stats.projectileCountBonus);
}

inline float skillSpreadAngle(const SkillDefinition& skill, const SupportDefinition* support) {
    return std::max(0.0f, skill.spreadAngle + (support ? support->extraSpreadAngle : 0.0f));
}

inline int supportAreaDamage(
    const SupportDefinition& support,
    const Stats& stats,
    float shrineMultiplier = 1.0f
) {
    if (support.dashBaseDamage <= 0) {
        return 0;
    }

    const float damage = static_cast<float>(support.dashBaseDamage)
        * stats.damageMultiplier
        * stats.areaDamageMultiplier
        * support.damageMultiplier;
    return std::max(1, static_cast<int>(std::ceil(damage * shrineMultiplier)));
}

inline float supportAreaRadius(const SupportDefinition& support, const Stats& stats) {
    return support.dashRadius * stats.areaRadiusMultiplier;
}

inline AilmentDefinition skillAilment(
    const SkillDefinition& skill,
    const SupportDefinition* support
) {
    AilmentDefinition ailment = skill.ailment;
    if (ailment.type == AilmentType::None || !support) {
        return ailment;
    }

    ailment.duration *= support->ailmentDurationMultiplier;
    ailment.ignitePenetration = support->ignitePenetration;
    ailment.chillPenetration = support->chillPenetration;
    switch (ailment.type) {
        case AilmentType::Ignite:
            ailment.damageMultiplier *= support->ailmentDamageMultiplier;
            break;
        case AilmentType::Chill:
            ailment.speedMultiplier = std::clamp(
                1.0f - (1.0f - ailment.speedMultiplier) * support->chillMagnitudeMultiplier,
                0.20f,
                1.0f
            );
            break;
        case AilmentType::None:
            break;
    }

    return ailment;
}

inline int ailmentTickDamage(const AilmentDefinition& ailment, int hitDamage) {
    if (ailment.type != AilmentType::Ignite || ailment.damageMultiplier <= 0.0f || hitDamage <= 0) {
        return 0;
    }

    return std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(hitDamage) * ailment.damageMultiplier
    )));
}

inline int effectiveAilmentResistance(int resistance, int penetration) {
    const int clampedResistance = std::clamp(resistance, 0, 100);
    return std::max(0, clampedResistance - std::max(0, penetration));
}

inline int ailmentTickDamageAfterResistance(
    int damagePerTick,
    int resistance,
    int penetration
) {
    if (damagePerTick <= 0) {
        return 0;
    }

    const int effectiveResistance = effectiveAilmentResistance(resistance, penetration);
    const float multiplier = 1.0f - static_cast<float>(effectiveResistance) / 100.0f;
    return std::clamp(static_cast<int>(std::floor(
        static_cast<float>(damagePerTick) * multiplier
    )), 0, damagePerTick);
}

inline float chillSpeedMultiplierAfterResistance(
    float speedMultiplier,
    int resistance,
    int penetration
) {
    const float clampedSpeed = std::clamp(speedMultiplier, 0.20f, 1.0f);
    const int effectiveResistance = effectiveAilmentResistance(resistance, penetration);
    const float remainingSlow = 1.0f - static_cast<float>(effectiveResistance) / 100.0f;
    return std::clamp(
        1.0f - (1.0f - clampedSpeed) * remainingSlow,
        0.20f,
        1.0f
    );
}
