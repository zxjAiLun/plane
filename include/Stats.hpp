#pragma once

struct Stats {
    int maxHp = 0;
    float moveSpeedMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float attackSpeedMultiplier = 1.0f;
    float pickupRangeMultiplier = 1.0f;
};

inline Stats combineStats(const Stats& base, const Stats& bonus) {
    return {
        base.maxHp + bonus.maxHp,
        base.moveSpeedMultiplier * bonus.moveSpeedMultiplier,
        base.damageMultiplier * bonus.damageMultiplier,
        base.attackSpeedMultiplier * bonus.attackSpeedMultiplier,
        base.pickupRangeMultiplier * bonus.pickupRangeMultiplier,
    };
}
