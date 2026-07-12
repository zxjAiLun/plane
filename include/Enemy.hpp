#pragma once

#include "Ailment.hpp"
#include "EnemyType.hpp"
#include "EliteModifier.hpp"
#include "Vector2.hpp"

class MapInstance;

class Enemy {
public:
    Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type = EnemyType::Normal,
        EliteModifier eliteModifier = EliteModifier::None);

    void update(float dt, const Vector2& targetPosition, const MapInstance& map);
    void updateAilments(float dt);

    void takeDamage(int damage);
    void applyIgnite(int damagePerTick, float duration);
    void applyChill(float speedMultiplier, float duration);
    void kill();
    bool isDead() const;

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
    Vector2 chargeTargetPosition() const;
    bool consumeChargeHit();
    bool isElite() const;
    bool isBoss() const;
    EliteModifier eliteModifier() const;

private:
    Vector2 position_;
    int id_;
    float radius_;
    int hp_;
    int maxHp_;
    int contactDamage_;
    EnemyType type_;
    EliteModifier eliteModifier_;
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

    inline static int nextId_ = 1;
};
