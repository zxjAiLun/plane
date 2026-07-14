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
    int projectileCountBonus = 0;
    float lifeFlaskEffectMultiplier = 1.0f;
    float itemQuantityMultiplier = 1.0f;
    float incomingDamageMultiplier = 1.0f;
    float fireDamageMultiplier = 1.0f;
    float coldDamageMultiplier = 1.0f;
    float lightningDamageMultiplier = 1.0f;
    int fireResistance = 0;
    int coldResistance = 0;
    int lightningResistance = 0;
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
        base.projectileCountBonus + bonus.projectileCountBonus,
        base.lifeFlaskEffectMultiplier * bonus.lifeFlaskEffectMultiplier,
        base.itemQuantityMultiplier * bonus.itemQuantityMultiplier,
        base.incomingDamageMultiplier * bonus.incomingDamageMultiplier,
        base.fireDamageMultiplier * bonus.fireDamageMultiplier,
        base.coldDamageMultiplier * bonus.coldDamageMultiplier,
        base.lightningDamageMultiplier * bonus.lightningDamageMultiplier,
        base.fireResistance + bonus.fireResistance,
        base.coldResistance + bonus.coldResistance,
        base.lightningResistance + bonus.lightningResistance,
    };
}
