#include "Projectile.hpp"
#include "Config.hpp"

#include <algorithm>
#include <utility>

Projectile::Projectile(
    const Vector2& position,
    const Vector2& velocity,
    int damage,
    int pierceCount,
    AilmentDefinition ailment,
    std::string source,
    DamageType damageType,
    int physicalPenetration
)
    : position_(position)
    , velocity_(velocity)
    , radius_(Config::ProjectileRadius)
    , damage_(damage)
    , ailment_(ailment)
    , source_(std::move(source))
    , damageType_(damageType)
    , physicalPenetration_(std::max(0, physicalPenetration))
    , remainingPierces_(pierceCount)
    , hitEnemyIds_()
    , alive_(true) {
}

void Projectile::update(float dt, const Vector2& worldSize) {
    position_ += velocity_ * dt;

    if (position_.y + radius_ < 0.0f
        || position_.y - radius_ > worldSize.y
        || position_.x + radius_ < 0.0f
        || position_.x - radius_ > worldSize.x) {
        alive_ = false;
    }
}

const Vector2& Projectile::position() const { return position_; }
float Projectile::radius() const { return radius_; }
int Projectile::damage() const { return damage_; }
const AilmentDefinition& Projectile::ailment() const { return ailment_; }
const std::string& Projectile::source() const { return source_; }
DamageType Projectile::damageType() const { return damageType_; }
int Projectile::physicalPenetration() const { return physicalPenetration_; }
bool Projectile::hasHitEnemy(int enemyId) const {
    return std::find(hitEnemyIds_.begin(), hitEnemyIds_.end(), enemyId) != hitEnemyIds_.end();
}
void Projectile::recordEnemyHit(int enemyId) {
    hitEnemyIds_.push_back(enemyId);
    if (remainingPierces_ <= 0) {
        kill();
    } else {
        --remainingPierces_;
    }
}

bool Projectile::isAlive() const { return alive_; }
void Projectile::kill() { alive_ = false; }
