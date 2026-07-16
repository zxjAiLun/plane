#pragma once

#include <string>

#include "Ailment.hpp"
#include "EnemyType.hpp"
#include "EliteModifier.hpp"
#include "Vector2.hpp"

class MapInstance;

class Enemy {
public:
    Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type = EnemyType::Normal,
        EliteModifier eliteModifier = EliteModifier::None);
    Enemy(
        const Vector2& position,
        int hp,
        int contactDamage,
        EnemyType type,
        EliteModifier eliteModifier,
        int mapEventIndex,
        bool summoned = false,
        int fieldPackIndex = -1,
        EliteModifier secondaryEliteModifier = EliteModifier::None,
        bool rare = false,
        std::string displayName = {},
        float rewardDropMultiplier = 1.0f,
        int bonusDropCount = 0,
        int rewardExperienceMultiplier = 1
    );

    void update(float dt, const Vector2& targetPosition, const MapInstance& map);
    void update(
        float dt,
        const Vector2& targetPosition,
        const MapInstance& map,
        float mapSpeedMultiplier
    );
    void moveBy(const Vector2& delta, const MapInstance& map);
    AilmentTickResult updateAilments(float dt);

    int takeDamage(int damage);
    int heal(int amount);
    void applyIgnite(int damagePerTick, float duration);
    void applyIgnite(
        int damagePerTick,
        float duration,
        float spreadRadius,
        float spreadMultiplier
    );
    void applyChill(float speedMultiplier, float duration);
    void applyShock(float damageTakenMultiplier, float duration);
    void applyPoison(int damagePerTick, float duration);
    void applyPoison(
        int damagePerTick,
        float duration,
        int maxStacks,
        float spreadRadius,
        float spreadMultiplier
    );
    void applyBleed(int damagePerTick, float duration);
    void applyBleed(int damagePerTick, float duration, int maxStacks);
    void kill();
    bool isDead() const;
    bool claimKillReward();

    const Vector2& position() const;
    int id() const;
    float radius() const;
    int hp() const;
    int maxHp() const;
    int contactDamage() const;
    float attackRange() const;
    bool isAttackWindingUp() const;
    bool consumeAttack();
    EnemyType type() const;
    bool isRanged() const;
    bool isCharger() const;
    bool isWarden() const;
    bool isSummoner() const;
    bool isSummoned() const;
    bool isCharging() const;
    bool isIgnited() const;
    bool isChilled() const;
    bool isShocked() const;
    bool isPoisoned() const;
    bool isBleeding() const;
    int poisonStacks() const;
    int bleedStacks() const;
    int igniteDamagePerTick() const;
    int poisonDamagePerTick() const;
    int bleedDamagePerTick() const;
    float igniteTimeRemaining() const;
    float poisonTimeRemaining() const;
    float bleedTimeRemaining() const;
    float igniteSpreadRadius() const;
    float igniteSpreadMultiplier() const;
    float poisonSpreadRadius() const;
    float poisonSpreadMultiplier() const;
    float damageTakenMultiplier() const;
    float movementSpeedMultiplier() const;
    Vector2 chargeTargetPosition(float mapSpeedMultiplier = 1.0f) const;
    bool consumeChargeHit();
    bool isElite() const;
    bool isBoss() const;
    bool isRare() const;
    EliteModifier eliteModifier() const;
    EliteModifier secondaryEliteModifier() const;
    const std::string& displayName() const;
    int ailmentResistanceBonus() const;
    float rewardDropMultiplier() const;
    int bonusDropCount() const;
    int rewardExperienceMultiplier() const;
    int mapEventIndex() const;
    int fieldPackIndex() const;

private:
    float eliteSpeedMultiplier() const;

    Vector2 position_;
    int id_;
    float radius_;
    int hp_;
    int maxHp_;
    int contactDamage_;
    EnemyType type_;
    EliteModifier eliteModifier_;
    EliteModifier secondaryEliteModifier_;
    int mapEventIndex_;
    bool summoned_;
    int fieldPackIndex_;
    bool rare_;
    std::string displayName_;
    float rewardDropMultiplier_;
    int bonusDropCount_;
    int rewardExperienceMultiplier_;
    float attackCooldownTimer_;
    float attackWindupTimer_;
    bool attackReady_;
    Vector2 chargeDirection_;
    float chargeTimer_;
    bool chargeHitConsumed_;
    int igniteDamagePerTick_;
    float igniteTimer_;
    float igniteTickTimer_;
    float igniteSpreadRadius_;
    float igniteSpreadMultiplier_;
    float chillTimer_;
    float chillSpeedMultiplier_;
    float shockTimer_;
    float shockDamageTakenMultiplier_;
    int poisonDamagePerTick_;
    int poisonStacks_;
    float poisonTimer_;
    float poisonTickTimer_;
    float poisonSpreadRadius_;
    float poisonSpreadMultiplier_;
    int bleedDamagePerTick_;
    int bleedStacks_;
    float bleedTimer_;
    float bleedTickTimer_;
    bool killRewardClaimed_;

    inline static int nextId_ = 1;
};
