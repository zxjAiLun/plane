#pragma once

#include "Ailment.hpp"
#include "EnemyType.hpp"
#include "EliteModifier.hpp"
#include "Vector2.hpp"

class MapInstance;

struct AilmentTickResult {
    AilmentType type = AilmentType::None;
    int damage = 0;
    int tickCount = 0;
    bool killed = false;
};

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
        int mapEventIndex
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
    void applyIgnite(int damagePerTick, float duration);
    void applyChill(float speedMultiplier, float duration);
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
    bool isCharging() const;
    bool isIgnited() const;
    bool isChilled() const;
    float movementSpeedMultiplier() const;
    Vector2 chargeTargetPosition(float mapSpeedMultiplier = 1.0f) const;
    bool consumeChargeHit();
    bool isElite() const;
    bool isBoss() const;
    EliteModifier eliteModifier() const;
    int mapEventIndex() const;

private:
    Vector2 position_;
    int id_;
    float radius_;
    int hp_;
    int maxHp_;
    int contactDamage_;
    EnemyType type_;
    EliteModifier eliteModifier_;
    int mapEventIndex_;
    float attackCooldownTimer_;
    float attackWindupTimer_;
    bool attackReady_;
    Vector2 chargeDirection_;
    float chargeTimer_;
    bool chargeHitConsumed_;
    int igniteDamagePerTick_;
    float igniteTimer_;
    float igniteTickTimer_;
    float chillTimer_;
    float chillSpeedMultiplier_;
    bool killRewardClaimed_;

    inline static int nextId_ = 1;
};
