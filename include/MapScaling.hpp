#pragma once

#include <algorithm>
#include <cmath>

#include "BossDefinition.hpp"
#include "Config.hpp"
#include "MapModifier.hpp"

namespace MapScaling {

inline int enemyHp(int mapLevel, const MapModifier& modifier) {
    const float levelMultiplier = 1.0f + (std::max(1, mapLevel) - 1) * 0.25f;
    return std::max(1, static_cast<int>(std::ceil(
        Config::EnemyHp * levelMultiplier * modifier.monsterHpMultiplier
    )));
}

inline int enemyDamage(int mapLevel, const MapModifier& modifier) {
    return Config::EnemyContactDamage
        + (std::max(1, mapLevel) - 1) / 3
        + modifier.monsterDamageBonus;
}

inline int itemLevel(int mapLevel, const MapModifier& modifier) {
    return std::max(1, std::max(1, mapLevel) + modifier.itemLevelBonus);
}

inline int bossHp(
    int mapLevel,
    const MapModifier& modifier,
    const BossDefinition& boss
) {
    return std::max(1, static_cast<int>(std::ceil(
        enemyHp(mapLevel, modifier)
            * boss.hpMultiplier
            * modifier.bossHpMultiplier
    )));
}

inline int bossContactDamage(
    int mapLevel,
    const MapModifier& modifier,
    const BossDefinition& boss
) {
    return std::max(1, static_cast<int>(std::ceil(
        (enemyDamage(mapLevel, modifier) + boss.damageBonus)
            * modifier.bossDamageMultiplier
    )));
}

inline GroundHazardDefinition environmentHazard(
    int mapLevel,
    const MapModifier& modifier,
    const GroundHazardDefinition& baseHazard,
    bool bossArena = false
) {
    const float levelMultiplier = 1.0f
        + static_cast<float>(std::max(1, mapLevel) - 1) * 0.12f;
    const float modifierMultiplier = 1.0f
        + static_cast<float>(std::max(0, modifier.monsterDamageBonus)) * 0.05f;
    const float bossMultiplier = bossArena
        ? std::max(1.0f, modifier.bossDamageMultiplier)
        : 1.0f;

    GroundHazardDefinition scaledHazard = baseHazard;
    scaledHazard.damage = std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(baseHazard.damage)
            * levelMultiplier
            * modifierMultiplier
            * bossMultiplier
    )));
    return scaledHazard;
}

} // namespace MapScaling
