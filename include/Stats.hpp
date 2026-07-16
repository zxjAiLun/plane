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
    // Keep new fields at the end so existing aggregate initializers remain valid.
    float poisonDamageMultiplier = 1.0f;
    int poisonResistance = 0;
    float maxManaMultiplier = 1.0f;
    float manaRegenMultiplier = 1.0f;
    float skillCostMultiplier = 1.0f;
    float igniteDamageMultiplier = 1.0f;
    float igniteDurationMultiplier = 1.0f;
    float chillMagnitudeMultiplier = 1.0f;
    float chillDurationMultiplier = 1.0f;
    float shockMagnitudeMultiplier = 1.0f;
    float shockDurationMultiplier = 1.0f;
    float poisonDurationMultiplier = 1.0f;
    float physicalDamageMultiplier = 1.0f;
    float bleedDamageMultiplier = 1.0f;
    float bleedDurationMultiplier = 1.0f;
    int bleedPenetration = 0;
    int bleedResistance = 0;
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
        base.poisonDamageMultiplier * bonus.poisonDamageMultiplier,
        base.poisonResistance + bonus.poisonResistance,
        base.maxManaMultiplier * bonus.maxManaMultiplier,
        base.manaRegenMultiplier * bonus.manaRegenMultiplier,
        base.skillCostMultiplier * bonus.skillCostMultiplier,
        base.igniteDamageMultiplier * bonus.igniteDamageMultiplier,
        base.igniteDurationMultiplier * bonus.igniteDurationMultiplier,
        base.chillMagnitudeMultiplier * bonus.chillMagnitudeMultiplier,
        base.chillDurationMultiplier * bonus.chillDurationMultiplier,
        base.shockMagnitudeMultiplier * bonus.shockMagnitudeMultiplier,
        base.shockDurationMultiplier * bonus.shockDurationMultiplier,
        base.poisonDurationMultiplier * bonus.poisonDurationMultiplier,
        base.physicalDamageMultiplier * bonus.physicalDamageMultiplier,
        base.bleedDamageMultiplier * bonus.bleedDamageMultiplier,
        base.bleedDurationMultiplier * bonus.bleedDurationMultiplier,
        base.bleedPenetration + bonus.bleedPenetration,
        base.bleedResistance + bonus.bleedResistance,
    };
}
