#pragma once

#include "EnemyType.hpp"
#include "Vector2.hpp"

class Enemy {
public:
    Enemy(const Vector2& position, int hp, int contactDamage, EnemyType type = EnemyType::Normal);

    void update(float dt, const Vector2& targetPosition);

    void takeDamage(int damage);
    void kill();
    bool isDead() const;

    const Vector2& position() const;
    float radius() const;
    int hp() const;
    int maxHp() const;
    int contactDamage() const;
    float attackRange() const;
    bool isAttackWindingUp() const;
    bool consumeMeleeAttack();
    EnemyType type() const;
    bool isElite() const;
    bool isBoss() const;

private:
    Vector2 position_;
    float radius_;
    int hp_;
    int maxHp_;
    int contactDamage_;
    EnemyType type_;
    float attackCooldownTimer_;
    float attackWindupTimer_;
    bool meleeAttackReady_;
};
