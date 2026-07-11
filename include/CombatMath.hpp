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
