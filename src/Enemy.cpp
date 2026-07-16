#include "Enemy.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"
#include "MapInstance.hpp"

#include <algorithm>
#include <utility>

Enemy::Enemy(
    const Vector2& position,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier
)
    : Enemy(
        position,
        hp,
        contactDamage,
        type,
        eliteModifier,
        -1,
        false,
        -1,
        EliteModifier::None,
        false
    ) {
}

Enemy::Enemy(
    const Vector2& position,
    int hp,
    int contactDamage,
    EnemyType type,
    EliteModifier eliteModifier,
    int mapEventIndex,
    bool summoned,
    int fieldPackIndex,
    EliteModifier secondaryEliteModifier,
    bool rare,
    std::string displayName,
    float rewardDropMultiplier,
    int bonusDropCount,
    int rewardExperienceMultiplier
)
    : position_(position)
    , id_(nextId_++)
    , radius_(Config::EnemyRadius * EnemyLibrary::forType(type).radiusMultiplier)
    , hp_(hp)
    , maxHp_(hp)
    , contactDamage_(contactDamage)
    , type_(type)
    , eliteModifier_(type == EnemyType::Elite ? eliteModifier : EliteModifier::None)
    , secondaryEliteModifier_(type == EnemyType::Elite ? secondaryEliteModifier : EliteModifier::None)
    , mapEventIndex_(mapEventIndex)
    , summoned_(summoned)
    , fieldPackIndex_(fieldPackIndex)
    , rare_(type == EnemyType::Elite && rare)
    , displayName_(std::move(displayName))
    , rewardDropMultiplier_(std::max(1.0f, rewardDropMultiplier))
    , bonusDropCount_(std::max(0, bonusDropCount))
    , rewardExperienceMultiplier_(std::max(1, rewardExperienceMultiplier))
    , attackCooldownTimer_(0.0f)
    , attackWindupTimer_(0.0f)
    , attackReady_(false)
    , chargeDirection_()
    , chargeTimer_(0.0f)
    , chargeHitConsumed_(false)
    , igniteDamagePerTick_(0)
    , igniteTimer_(0.0f)
    , igniteTickTimer_(0.0f)
    , chillTimer_(0.0f)
    , chillSpeedMultiplier_(1.0f)
    , shockTimer_(0.0f)
    , shockDamageTakenMultiplier_(1.0f)
    , poisonDamagePerTick_(0)
    , poisonStacks_(0)
    , poisonTimer_(0.0f)
    , poisonTickTimer_(0.0f)
    , poisonSpreadRadius_(0.0f)
    , poisonSpreadMultiplier_(0.0f)
    , bleedDamagePerTick_(0)
    , bleedStacks_(0)
    , bleedTimer_(0.0f)
    , bleedTickTimer_(0.0f)
    , killRewardClaimed_(false) {
}

void Enemy::update(float dt, const Vector2& targetPosition, const MapInstance& map) {
    update(dt, targetPosition, map, 1.0f);
}

void Enemy::update(
    float dt,
    const Vector2& targetPosition,
    const MapInstance& map,
    float mapSpeedMultiplier
) {
    const float safeMapSpeedMultiplier = std::max(0.0f, mapSpeedMultiplier);
    if (isBoss()) {
        Vector2 direction = (targetPosition - position_).normalized();
        position_ = map.resolveMovement(
            position_, radius_, direction * Config::EnemySpeed * safeMapSpeedMultiplier
                * movementSpeedMultiplier() * dt
        );
        return;
    }

    const auto& definition = EnemyLibrary::forType(type_);
    attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - dt);
    const float speedMultiplier = eliteSpeedMultiplier()
        * safeMapSpeedMultiplier * movementSpeedMultiplier();

    if (chargeTimer_ > 0.0f) {
        const float chargeStep = std::min(dt, chargeTimer_);
        position_ = map.resolveMovement(
            position_, radius_,
            chargeDirection_ * Config::EnemySpeed * definition.chargeSpeedMultiplier * speedMultiplier * chargeStep
        );
        chargeTimer_ = std::max(0.0f, chargeTimer_ - dt);
        if (chargeTimer_ == 0.0f) {
            attackCooldownTimer_ = definition.attackCooldown;
        }
        return;
    }

    if (attackWindupTimer_ > 0.0f) {
        attackWindupTimer_ = std::max(0.0f, attackWindupTimer_ - dt);
        if (attackWindupTimer_ == 0.0f) {
            if (isCharger()) {
                chargeTimer_ = definition.chargeDuration;
                chargeHitConsumed_ = false;
            } else {
                attackReady_ = true;
            }
        }
        return;
    }

    const Vector2 toTarget = targetPosition - position_;
    if (attackCooldownTimer_ <= 0.0f
        && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        if (isCharger()) {
            chargeDirection_ = toTarget.normalized();
            if (chargeDirection_.lengthSquared() <= 0.0f) {
                return;
            }
        }
        attackWindupTimer_ = definition.attackWindup;
        return;
    }

    if ((isRanged() || isSummoner())
        && toTarget.lengthSquared() <= definition.attackRange * definition.attackRange) {
        return;
    }

    Vector2 direction = (targetPosition - position_).normalized();
    position_ = map.resolveMovement(position_, radius_, direction * Config::EnemySpeed * speedMultiplier * dt);
}

void Enemy::moveBy(const Vector2& delta, const MapInstance& map) {
    position_ = map.resolveMovement(position_, radius_, delta);
}

AilmentTickResult Enemy::updateAilments(float dt) {
    AilmentTickResult result;
    const float elapsed = std::max(0.0f, dt);
    if (igniteTimer_ > 0.0f) {
        const float activeTime = std::min(elapsed, igniteTimer_);
        igniteTimer_ = std::max(0.0f, igniteTimer_ - elapsed);
        igniteTickTimer_ -= activeTime;
        while (igniteTickTimer_ <= 0.0f && igniteTimer_ > 0.0f && !isDead()) {
            const int dealtDamage = takeDamage(igniteDamagePerTick_);
            result.record(AilmentType::Ignite, dealtDamage);
            result.killed = isDead();
            igniteTickTimer_ += Config::AilmentTickInterval;
            if (result.killed) {
                break;
            }
        }
        if (igniteTimer_ <= 0.0f) {
            igniteDamagePerTick_ = 0;
            igniteTickTimer_ = 0.0f;
        }
    }

    if (poisonTimer_ > 0.0f) {
        const float activeTime = std::min(elapsed, poisonTimer_);
        poisonTimer_ = std::max(0.0f, poisonTimer_ - elapsed);
        poisonTickTimer_ -= activeTime;
        while (poisonTickTimer_ <= 0.0f && poisonTimer_ > 0.0f && !isDead()) {
            const int dealtDamage = takeDamage(poisonDamagePerTick_);
            result.record(AilmentType::Poison, dealtDamage);
            result.killed = isDead();
            poisonTickTimer_ += Config::AilmentTickInterval;
            if (result.killed) {
                break;
            }
        }
        if (poisonTimer_ <= 0.0f) {
            poisonDamagePerTick_ = 0;
            poisonStacks_ = 0;
            poisonTickTimer_ = 0.0f;
            poisonSpreadRadius_ = 0.0f;
            poisonSpreadMultiplier_ = 0.0f;
        }
    }

    if (bleedTimer_ > 0.0f) {
        const float activeTime = std::min(elapsed, bleedTimer_);
        bleedTimer_ = std::max(0.0f, bleedTimer_ - elapsed);
        bleedTickTimer_ -= activeTime;
        while (bleedTickTimer_ <= 0.0f && bleedTimer_ > 0.0f && !isDead()) {
            const int dealtDamage = takeDamage(bleedDamagePerTick_);
            result.record(AilmentType::Bleed, dealtDamage);
            result.killed = isDead();
            bleedTickTimer_ += Config::AilmentTickInterval;
            if (result.killed) {
                break;
            }
        }
        if (bleedTimer_ <= 0.0f) {
            bleedDamagePerTick_ = 0;
            bleedStacks_ = 0;
            bleedTickTimer_ = 0.0f;
        }
    }

    chillTimer_ = std::max(0.0f, chillTimer_ - elapsed);
    if (chillTimer_ <= 0.0f) {
        chillSpeedMultiplier_ = 1.0f;
    }

    shockTimer_ = std::max(0.0f, shockTimer_ - elapsed);
    if (shockTimer_ <= 0.0f) {
        shockDamageTakenMultiplier_ = 1.0f;
    }

    return result;
}

int Enemy::takeDamage(int damage) {
    if (damage <= 0 || isDead()) {
        return 0;
    }

    const int previousHp = hp_;
    hp_ = std::max(0, hp_ - damage);
    return previousHp - hp_;
}

int Enemy::heal(int amount) {
    if (amount <= 0 || isDead() || hp_ >= maxHp_) {
        return 0;
    }

    const int previousHp = hp_;
    hp_ = std::min(maxHp_, hp_ + amount);
    return hp_ - previousHp;
}

void Enemy::applyIgnite(int damagePerTick, float duration) {
    if (damagePerTick <= 0 || duration <= 0.0f) {
        return;
    }

    igniteDamagePerTick_ = std::max(igniteDamagePerTick_, damagePerTick);
    igniteTimer_ = std::max(igniteTimer_, duration);
    igniteTickTimer_ = std::min(igniteTickTimer_, Config::AilmentTickInterval);
    if (igniteTickTimer_ <= 0.0f) {
        igniteTickTimer_ = Config::AilmentTickInterval;
    }
}

void Enemy::applyChill(float speedMultiplier, float duration) {
    if (speedMultiplier <= 0.0f || speedMultiplier >= 1.0f || duration <= 0.0f) {
        return;
    }

    chillSpeedMultiplier_ = std::min(chillSpeedMultiplier_, speedMultiplier);
    chillTimer_ = std::max(chillTimer_, duration);
}

void Enemy::applyShock(float damageTakenMultiplier, float duration) {
    if (damageTakenMultiplier <= 1.0f || duration <= 0.0f) {
        return;
    }

    shockDamageTakenMultiplier_ = std::max(
        shockDamageTakenMultiplier_, damageTakenMultiplier
    );
    shockTimer_ = std::max(shockTimer_, duration);
}

void Enemy::applyPoison(int damagePerTick, float duration) {
    applyPoison(damagePerTick, duration, Config::MaxPoisonStacks, 0.0f, 0.0f);
}

void Enemy::applyPoison(
    int damagePerTick,
    float duration,
    int maxStacks,
    float spreadRadius,
    float spreadMultiplier
) {
    if (damagePerTick <= 0 || duration <= 0.0f) {
        return;
    }

    if (poisonStacks_ < std::max(1, maxStacks)) {
        ++poisonStacks_;
        poisonDamagePerTick_ += damagePerTick;
    }
    poisonTimer_ = std::max(poisonTimer_, duration);
    poisonSpreadRadius_ = std::max(poisonSpreadRadius_, std::max(0.0f, spreadRadius));
    poisonSpreadMultiplier_ = std::max(
        poisonSpreadMultiplier_, std::max(0.0f, spreadMultiplier)
    );
    poisonTickTimer_ = std::min(poisonTickTimer_, Config::AilmentTickInterval);
    if (poisonTickTimer_ <= 0.0f) {
        poisonTickTimer_ = Config::AilmentTickInterval;
    }
}

void Enemy::applyBleed(int damagePerTick, float duration) {
    applyBleed(damagePerTick, duration, Config::MaxBleedStacks);
}

void Enemy::applyBleed(int damagePerTick, float duration, int maxStacks) {
    if (damagePerTick <= 0 || duration <= 0.0f) {
        return;
    }

    if (bleedStacks_ < std::max(1, maxStacks)) {
        ++bleedStacks_;
        bleedDamagePerTick_ += damagePerTick;
    }
    bleedTimer_ = std::max(bleedTimer_, duration);
    bleedTickTimer_ = std::min(bleedTickTimer_, Config::AilmentTickInterval);
    if (bleedTickTimer_ <= 0.0f) {
        bleedTickTimer_ = Config::AilmentTickInterval;
    }
}

void Enemy::kill() {
    hp_ = 0;
}

bool Enemy::isDead() const {
    return hp_ <= 0;
}

bool Enemy::claimKillReward() {
    if (!isDead() || killRewardClaimed_) {
        return false;
    }

    killRewardClaimed_ = true;
    return true;
}

const Vector2& Enemy::position() const { return position_; }
int Enemy::id() const { return id_; }
float Enemy::radius() const { return radius_; }
int Enemy::hp() const { return hp_; }
int Enemy::maxHp() const { return maxHp_; }
int Enemy::contactDamage() const { return contactDamage_; }
float Enemy::attackRange() const { return EnemyLibrary::forType(type_).attackRange; }
bool Enemy::isAttackWindingUp() const { return !isBoss() && attackWindupTimer_ > 0.0f; }
bool Enemy::consumeAttack() {
    if (!attackReady_) {
        return false;
    }

    attackReady_ = false;
    attackCooldownTimer_ = EnemyLibrary::forType(type_).attackCooldown;
    return true;
}
EnemyType Enemy::type() const { return type_; }
bool Enemy::isRanged() const {
    return EnemyLibrary::forType(type_).attackStyle == EnemyAttackStyle::Projectile;
}
bool Enemy::isCharger() const {
    return EnemyLibrary::forType(type_).attackStyle == EnemyAttackStyle::Charge;
}
bool Enemy::isWarden() const { return type_ == EnemyType::Warden; }
bool Enemy::isSummoner() const {
    return EnemyLibrary::forType(type_).attackStyle == EnemyAttackStyle::Summon;
}
bool Enemy::isSummoned() const { return summoned_; }
bool Enemy::isCharging() const { return chargeTimer_ > 0.0f; }
bool Enemy::isIgnited() const { return igniteTimer_ > 0.0f; }
bool Enemy::isChilled() const { return chillTimer_ > 0.0f; }
bool Enemy::isShocked() const { return shockTimer_ > 0.0f; }
bool Enemy::isPoisoned() const { return poisonTimer_ > 0.0f; }
bool Enemy::isBleeding() const { return bleedTimer_ > 0.0f; }
int Enemy::poisonStacks() const { return poisonStacks_; }
int Enemy::bleedStacks() const { return bleedStacks_; }
int Enemy::poisonDamagePerTick() const { return poisonDamagePerTick_; }
int Enemy::bleedDamagePerTick() const { return bleedDamagePerTick_; }
float Enemy::poisonTimeRemaining() const { return poisonTimer_; }
float Enemy::bleedTimeRemaining() const { return bleedTimer_; }
float Enemy::poisonSpreadRadius() const { return poisonSpreadRadius_; }
float Enemy::poisonSpreadMultiplier() const { return poisonSpreadMultiplier_; }
float Enemy::damageTakenMultiplier() const {
    return isShocked() ? shockDamageTakenMultiplier_ : 1.0f;
}
float Enemy::movementSpeedMultiplier() const {
    return isChilled() ? chillSpeedMultiplier_ : 1.0f;
}
Vector2 Enemy::chargeTargetPosition(float mapSpeedMultiplier) const {
    if (!isCharger()) {
        return position_;
    }

    const auto& definition = EnemyLibrary::forType(type_);
    const float duration = chargeTimer_ > 0.0f ? chargeTimer_ : definition.chargeDuration;
    return position_ + chargeDirection_ * Config::EnemySpeed
        * definition.chargeSpeedMultiplier * std::max(0.0f, mapSpeedMultiplier) * duration;
}
bool Enemy::consumeChargeHit() {
    if (!isCharging() || chargeHitConsumed_) {
        return false;
    }

    chargeHitConsumed_ = true;
    return true;
}
bool Enemy::isElite() const {
    return type_ == EnemyType::Elite || type_ == EnemyType::Boss || isWarden();
}
bool Enemy::isBoss() const { return type_ == EnemyType::Boss; }
bool Enemy::isRare() const { return rare_; }
EliteModifier Enemy::eliteModifier() const { return eliteModifier_; }
EliteModifier Enemy::secondaryEliteModifier() const { return secondaryEliteModifier_; }
const std::string& Enemy::displayName() const { return displayName_; }
int Enemy::ailmentResistanceBonus() const {
    return EliteModifierLibrary::forModifier(eliteModifier_).ailmentResistanceBonus
        + EliteModifierLibrary::forModifier(secondaryEliteModifier_).ailmentResistanceBonus;
}
float Enemy::rewardDropMultiplier() const { return rewardDropMultiplier_; }
int Enemy::bonusDropCount() const { return bonusDropCount_; }
int Enemy::rewardExperienceMultiplier() const { return rewardExperienceMultiplier_; }
int Enemy::mapEventIndex() const { return mapEventIndex_; }
int Enemy::fieldPackIndex() const { return fieldPackIndex_; }

float Enemy::eliteSpeedMultiplier() const {
    return EliteModifierLibrary::forModifier(eliteModifier_).speedMultiplier
        * EliteModifierLibrary::forModifier(secondaryEliteModifier_).speedMultiplier;
}
