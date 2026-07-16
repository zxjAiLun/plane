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

inline int resistanceForDamageType(
    DamageType type,
    int fireResistance,
    int coldResistance,
    int lightningResistance,
    int poisonResistance = 0
) {
    switch (type) {
        case DamageType::Fire: return fireResistance;
        case DamageType::Cold: return coldResistance;
        case DamageType::Lightning: return lightningResistance;
        case DamageType::Poison: return poisonResistance;
        case DamageType::Physical: break;
    }
    return 0;
}

inline int damageAfterResistance(
    int rawDamage,
    DamageType type,
    int fireResistance,
    int coldResistance,
    int lightningResistance,
    int poisonResistance = 0
) {
    if (rawDamage <= 0) {
        return 0;
    }

    // Negative resistance is intentional: elemental map challenges can reduce
    // the player's resistance below zero and should increase incoming damage.
    const int resistance = std::clamp(
        resistanceForDamageType(
            type, fireResistance, coldResistance, lightningResistance, poisonResistance
        ),
        -100,
        100
    );
    if (resistance >= 100) {
        return 0;
    }
    const long long scaledDamage = static_cast<long long>(rawDamage) * (100 - resistance);
    return std::max(1, static_cast<int>((scaledDamage + 99) / 100));
}

inline int lifeFlaskHealAmount(int baseAmount, const Stats& stats) {
    return std::max(0, static_cast<int>(std::ceil(
        static_cast<float>(baseAmount) * stats.lifeFlaskEffectMultiplier
    )));
}

inline int incomingDamage(
    int rawDamage,
    const Stats& stats,
    DamageType type = DamageType::Physical
) {
    if (rawDamage <= 0) {
        return 0;
    }
    const int scaledDamage = std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(rawDamage) * stats.incomingDamageMultiplier
    )));
    return damageAfterResistance(
        scaledDamage,
        type,
        stats.fireResistance,
        stats.coldResistance,
        stats.lightningResistance,
        stats.poisonResistance
    );
}

inline int wardenProtectedDamage(int rawDamage, bool protectedByWarden, float damageMultiplier) {
    if (rawDamage <= 0) {
        return 0;
    }
    if (!protectedByWarden) {
        return rawDamage;
    }

    const float safeMultiplier = std::clamp(damageMultiplier, 0.0f, 1.0f);
    return std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(rawDamage) * safeMultiplier
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
    const SupportList& supports,
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

    switch (skill.damageType) {
        case DamageType::Fire:
            damage *= stats.fireDamageMultiplier;
            break;
        case DamageType::Cold:
            damage *= stats.coldDamageMultiplier;
            break;
        case DamageType::Lightning:
            damage *= stats.lightningDamageMultiplier;
            break;
        case DamageType::Poison:
            damage *= stats.poisonDamageMultiplier;
            break;
        case DamageType::Physical:
            break;
    }

    for (const auto* support : supports) {
        if (support != nullptr) {
            damage *= support->damageMultiplier;
        }
    }

    return std::max(1, static_cast<int>(std::ceil(damage * shrineMultiplier)));
}

inline int skillDamage(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support,
    float shrineMultiplier = 1.0f
) {
    return skillDamage(skill, stats, SupportList{support, nullptr}, shrineMultiplier);
}

inline float skillRadius(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportList& supports
) {
    float supportMultiplier = 1.0f;
    for (const auto* support : supports) {
        if (support != nullptr) {
            supportMultiplier *= support->radiusMultiplier;
        }
    }
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

inline float skillRadius(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support
) {
    return skillRadius(skill, stats, SupportList{support, nullptr});
}

inline float skillCooldown(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportList& supports
) {
    float cooldown = skill.cooldown;
    for (const auto* support : supports) {
        if (support != nullptr) {
            cooldown *= support->cooldownMultiplier;
        }
    }

    if (skill.slot == SkillSlot::Primary) {
        return cooldown / std::max(0.0001f, stats.attackSpeedMultiplier);
    }

    return cooldown;
}

inline float skillManaCost(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportList& supports
) {
    float cost = skill.manaCost * stats.skillCostMultiplier;
    for (const auto* support : supports) {
        if (support != nullptr) {
            cost *= support->manaCostMultiplier;
        }
    }
    return std::max(0.0f, cost);
}

inline float skillManaCost(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support
) {
    return skillManaCost(skill, stats, SupportList{support, nullptr});
}

inline float skillCooldown(
    const SkillDefinition& skill,
    const Stats& stats,
    const SupportDefinition* support
) {
    return skillCooldown(skill, stats, SupportList{support, nullptr});
}

inline int skillPierceCount(const SupportList& supports) {
    int pierceCount = 0;
    for (const auto* support : supports) {
        if (support != nullptr) {
            pierceCount += support->pierceCount;
        }
    }
    return pierceCount;
}

inline int skillPierceCount(const SupportDefinition* support) {
    return skillPierceCount(SupportList{support, nullptr});
}

inline int skillProjectileCount(const SkillDefinition& skill, const SupportList& supports) {
    int extraProjectiles = 0;
    for (const auto* support : supports) {
        if (support != nullptr) {
            extraProjectiles += support->extraProjectileCount;
        }
    }
    return std::max(1, skill.projectileCount + extraProjectiles);
}

inline int skillProjectileCount(
    const SkillDefinition& skill,
    const SupportDefinition* support
) {
    return skillProjectileCount(skill, SupportList{support, nullptr});
}

inline int skillProjectileCount(
    const SkillDefinition& skill,
    const SupportList& supports,
    const Stats& stats
) {
    return std::max(1, skillProjectileCount(skill, supports) + stats.projectileCountBonus);
}

inline int skillProjectileCount(
    const SkillDefinition& skill,
    const SupportDefinition* support,
    const Stats& stats
) {
    return skillProjectileCount(skill, SupportList{support, nullptr}, stats);
}

inline float skillSpreadAngle(const SkillDefinition& skill, const SupportList& supports) {
    float extraSpread = 0.0f;
    for (const auto* support : supports) {
        if (support != nullptr) {
            extraSpread += support->extraSpreadAngle;
        }
    }
    return std::max(0.0f, skill.spreadAngle + extraSpread);
}

inline float skillSpreadAngle(const SkillDefinition& skill, const SupportDefinition* support) {
    return skillSpreadAngle(skill, SupportList{support, nullptr});
}

inline int skillRepeatCount(const SkillDefinition& skill, const SupportList& supports) {
    if (skill.castType != SkillCastType::SelfCenteredArea
        && skill.castType != SkillCastType::MouseTargetedArea) {
        return 1;
    }

    int repeatCount = 1;
    for (const auto* support : supports) {
        if (support != nullptr) {
            repeatCount += support->repeatCountBonus;
        }
    }
    return std::max(1, repeatCount);
}

inline int skillRepeatCount(const SkillDefinition& skill, const SupportDefinition* support) {
    return skillRepeatCount(skill, SupportList{support, nullptr});
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
    const SupportList& supports
) {
    AilmentDefinition ailment = skill.ailment;
    if (ailment.type == AilmentType::None) {
        return ailment;
    }

    for (const auto* support : supports) {
        if (support == nullptr) {
            continue;
        }

        ailment.duration *= support->ailmentDurationMultiplier;
        ailment.ignitePenetration += support->ignitePenetration;
        ailment.chillPenetration += support->chillPenetration;
        ailment.shockPenetration += support->shockPenetration;
        ailment.poisonPenetration += support->poisonPenetration;
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
            case AilmentType::Shock:
                ailment.damageTakenMultiplier = std::clamp(
                    1.0f + (ailment.damageTakenMultiplier - 1.0f)
                        * support->shockMagnitudeMultiplier,
                    1.0f,
                    2.0f
                );
                break;
            case AilmentType::Poison:
                ailment.damageMultiplier *= support->ailmentDamageMultiplier;
                ailment.poisonSpreadRadius = std::max(
                    ailment.poisonSpreadRadius, support->poisonSpreadRadius
                );
                ailment.poisonSpreadMultiplier = std::max(
                    ailment.poisonSpreadMultiplier, support->poisonSpreadMultiplier
                );
                break;
            case AilmentType::None:
            case AilmentType::Count:
                break;
        }
    }

    return ailment;
}

inline AilmentDefinition skillAilment(
    const SkillDefinition& skill,
    const SupportDefinition* support
) {
    return skillAilment(skill, SupportList{support, nullptr});
}

inline int ailmentTickDamage(const AilmentDefinition& ailment, int hitDamage) {
    if ((ailment.type != AilmentType::Ignite && ailment.type != AilmentType::Poison)
        || ailment.damageMultiplier <= 0.0f || hitDamage <= 0) {
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

inline float damageTakenMultiplierAfterResistance(
    float damageTakenMultiplier,
    int resistance,
    int penetration
) {
    const float clampedMultiplier = std::clamp(damageTakenMultiplier, 1.0f, 2.0f);
    const int effectiveResistance = effectiveAilmentResistance(resistance, penetration);
    return std::clamp(
        1.0f + (clampedMultiplier - 1.0f)
            * (1.0f - static_cast<float>(effectiveResistance) / 100.0f),
        1.0f,
        clampedMultiplier
    );
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
