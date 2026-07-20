#pragma once

#include <algorithm>
#include <cmath>

#include "Config.hpp"
#include "Skill.hpp"
#include "SupportLibrary.hpp"

namespace SkillProgression {

inline int clampLevel(int level) {
    return std::clamp(level, 1, Config::SkillGemMaxLevel);
}

inline SkillDefinition skillAtLevel(const SkillDefinition& base, int level) {
    SkillDefinition result = base;
    const int clampedLevel = clampLevel(level);
    const int levelDelta = clampedLevel - 1;
    if (levelDelta <= 0) {
        return result;
    }

    result.baseDamage += std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(std::max(1, base.baseDamage))
            * Config::SkillGemDamagePerLevelMultiplier
            * static_cast<float>(levelDelta)
    )));
    if (base.summonDamage > 0) {
        result.summonDamage += std::max(1, static_cast<int>(std::ceil(
            static_cast<float>(base.summonDamage)
                * Config::SkillGemDamagePerLevelMultiplier
                * static_cast<float>(levelDelta)
        )));
    }
    if (base.summonMaxHp > 0) {
        result.summonMaxHp += std::max(1, static_cast<int>(std::ceil(
            static_cast<float>(base.summonMaxHp)
                * Config::SkillGemDamagePerLevelMultiplier
                * static_cast<float>(levelDelta)
        )));
    }
    if (base.radius > 0.0f) {
        result.radius *= 1.0f + Config::SkillGemRadiusPerLevelMultiplier
            * static_cast<float>(levelDelta);
    }
    if (base.wardManaRatio > 0.0f) {
        result.wardManaRatio = std::min(
            0.90f,
            base.wardManaRatio + Config::SkillGemWardManaPerLevel
                * static_cast<float>(levelDelta)
        );
    }

    for (int levelIndex = 0; levelIndex < levelDelta; ++levelIndex) {
        result.cooldown *= Config::SkillGemCooldownPerLevelMultiplier;
    }
    return result;
}

inline float improveMultiplier(float base, int level, float step) {
    const int levelDelta = clampLevel(level) - 1;
    if (levelDelta <= 0 || std::abs(base - 1.0f) < 0.0001f) {
        return base;
    }

    return std::min(3.0f, base + step * static_cast<float>(levelDelta));
}

inline float reduceMultiplier(float base, int level, float step) {
    const int levelDelta = clampLevel(level) - 1;
    if (levelDelta <= 0) {
        return base;
    }

    return std::max(0.05f, base - step * static_cast<float>(levelDelta));
}

inline SupportDefinition supportAtLevel(const SupportDefinition& base, int level) {
    SupportDefinition result = base;
    const int clampedLevel = clampLevel(level);
    const int levelDelta = clampedLevel - 1;
    if (levelDelta <= 0) {
        return result;
    }

    result.damageMultiplier = improveMultiplier(
        result.damageMultiplier, clampedLevel, Config::SupportGemDamageStep
    );
    result.radiusMultiplier = improveMultiplier(
        result.radiusMultiplier, clampedLevel, Config::SupportGemRadiusStep
    );
    result.cooldownMultiplier = reduceMultiplier(
        result.cooldownMultiplier, clampedLevel, Config::SupportGemCooldownStep
    );
    result.manaCostMultiplier = reduceMultiplier(
        result.manaCostMultiplier, clampedLevel, Config::SupportGemManaCostStep
    );
    result.elementalDamageMultiplier = improveMultiplier(
        result.elementalDamageMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.ailmentDamageMultiplier = improveMultiplier(
        result.ailmentDamageMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.ailmentDurationMultiplier = improveMultiplier(
        result.ailmentDurationMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.chillMagnitudeMultiplier = improveMultiplier(
        result.chillMagnitudeMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.shockMagnitudeMultiplier = improveMultiplier(
        result.shockMagnitudeMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.poisonSpreadMultiplier = improveMultiplier(
        result.poisonSpreadMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    result.igniteSpreadMultiplier = improveMultiplier(
        result.igniteSpreadMultiplier, clampedLevel, Config::SupportGemAilmentStep
    );
    if (result.igniteSpreadRadius > 0.0f) {
        result.igniteSpreadRadius *= 1.0f
            + Config::SkillGemRadiusPerLevelMultiplier * static_cast<float>(levelDelta);
    }
    if (result.shatterRadius > 0.0f) {
        result.shatterRadius *= 1.0f
            + Config::SkillGemRadiusPerLevelMultiplier * static_cast<float>(levelDelta);
    }
    result.freezeDuration += levelDelta * 0.05f;
    if (result.shatterDamageMultiplier > 0.0f) {
        result.shatterDamageMultiplier = std::min(
            1.50f,
            result.shatterDamageMultiplier
                + Config::SupportGemAilmentStep * static_cast<float>(levelDelta)
        );
    }
    result.summonCountBonus += levelDelta / 3;
    if (result.summonDurationMultiplier > 1.0f) {
        result.summonDurationMultiplier +=
            Config::SupportGemAilmentStep * static_cast<float>(levelDelta);
    }
    if (result.summonDamageMultiplier > 1.0f) {
        result.summonDamageMultiplier = std::min(
            3.0f,
            result.summonDamageMultiplier
                + Config::SupportGemDamageStep * static_cast<float>(levelDelta)
        );
    }
    if (result.summonAttackIntervalMultiplier > 1.0f) {
        result.summonAttackIntervalMultiplier = std::max(
            1.0f,
            result.summonAttackIntervalMultiplier
                - Config::SupportGemCooldownStep * static_cast<float>(levelDelta)
        );
    }
    if (result.summonHpMultiplier > 1.0f) {
        result.summonHpMultiplier = std::min(
            3.0f,
            result.summonHpMultiplier
                + Config::SupportGemDamageStep * static_cast<float>(levelDelta)
        );
    }
    result.healOnHitBonus += levelDelta * Config::SupportGemHealingStep;
    result.dashBaseDamage += levelDelta;
    if (result.dashRadius > 0.0f) {
        result.dashRadius *= 1.0f + Config::SkillGemRadiusPerLevelMultiplier
            * static_cast<float>(levelDelta);
    }
    result.pierceCount += levelDelta / 2;
    result.extraProjectileCount += levelDelta / 3;
    result.repeatCountBonus += levelDelta / 3;
    result.ignitePenetration += levelDelta / 2;
    result.chillPenetration += levelDelta / 2;
    result.shockPenetration += levelDelta / 2;
    result.poisonPenetration += levelDelta / 2;
    result.bleedPenetration += levelDelta / 2;
    result.physicalPenetration += levelDelta / 2;
    return result;
}

} // namespace SkillProgression
