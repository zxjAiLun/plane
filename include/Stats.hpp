#pragma once

struct Stats {
    int maxHp = 0;
    float moveSpeedMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float attackSpeedMultiplier = 1.0f;
    float pickupRangeMultiplier = 1.0f;
    float projectileDamageMultiplier = 1.0f;
    float areaDamageMultiplier = 1.0f;
    float areaRadiusMultiplier = 1.0f;
    int armor = 0;
};

inline Stats combineStats(const Stats& base, const Stats& bonus) {
    return {
        base.maxHp + bonus.maxHp,
        base.moveSpeedMultiplier * bonus.moveSpeedMultiplier,
        base.damageMultiplier * bonus.damageMultiplier,
        base.attackSpeedMultiplier * bonus.attackSpeedMultiplier,
        base.pickupRangeMultiplier * bonus.pickupRangeMultiplier,
        base.projectileDamageMultiplier * bonus.projectileDamageMultiplier,
        base.areaDamageMultiplier * bonus.areaDamageMultiplier,
        base.areaRadiusMultiplier * bonus.areaRadiusMultiplier,
        base.armor + bonus.armor,
    };
}
