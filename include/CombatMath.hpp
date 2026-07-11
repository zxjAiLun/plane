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

inline int refilledFlaskCharges(int currentCharges, int maxCharges, int restoredCharges) {
    return std::clamp(currentCharges + std::max(0, restoredCharges), 0, maxCharges);
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
