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
    if (base.radius > 0.0f) {
        result.radius *= 1.0f + Config::SkillGemRadiusPerLevelMultiplier
            * static_cast<float>(levelDelta);
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
    return result;
}

} // namespace SkillProgression
