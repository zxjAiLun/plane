#include "GameWorld.hpp"
#include "Collision.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace {
constexpr float ShrineBuffDuration = 20.0f;
constexpr float ShrineDamageMultiplier = 1.35f;
}

GameWorld::GameWorld()
    : map_()
    , progression_()
    , state_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f)
    , aimPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , novaEffectTimer_(0.0f)
    , secondarySkillEffectPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , secondarySkillEffectTimer_(0.0f)
    , bossAoeCenter_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , bossAoeTelegraphTimer_(0.0f)
    , bossAoeEffectTimer_(0.0f)
    , bossAoeSkill_()
    , bossSkillTimer_(Config::BossSkillInterval)
    , bossSkillIndex_(0)
    , bossEnraged_(false)
    , bossDefinition_(&BossLibrary::forMapLevel(1))
    , playerHitCooldown_(0.0f)
    , mapLevel_(1)
    , currentWave_(0)
    , enemiesSpawnedInWave_(0)
    , currentMapOption_(MapOptionLibrary::defaultOption())
    , nextMapOptions_(MapOptionLibrary::generateOptions(2))
    , selectedNextMapOption_(-1)
    , mapRewardOptions_()
    , selectedMapRewardOption_(-1)
    , mapModifier_(currentMapOption_.modifier)
    , mapKills_(0)
    , mapExperienceGained_(0)
    , mapItemsDropped_(0)
    , mapBossItemsDropped_(0)
    , mapItemsPickedUp_(0)
    , mapRewardChosen_(false)
    , nextMapOptionChosen_(false)
    , passiveTreeOpen_(false)
    , skillPanelOpen_(false)
    , hoveredPassiveNode_(-1)
    , nearbyEventPrompt_()
    , shrineBuffTimer_(0.0f)
    , lifeFlaskCharges_(Config::LifeFlaskMaxCharges)
    , lifeFlaskStatusMessage_()
    , lifeFlaskStatusTimer_(0.0f)
    , inventoryFullTimer_(0.0f)
    , selectedInventoryIndex_(-1)
    , mapEventInteractionConsumed_(false)
    , activeEliteEventIndex_(-1)
    , eliteEventEnemiesRemaining_(0) {
    initializeRunProgression();
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    skillBar_.applyStats(player_.stats());
}

void GameWorld::update(float dt, Input& input) {
    const Vector2 camera = cameraTopLeft();
    aimPosition_ = Vector2(
        camera.x + static_cast<float>(input.mousePosition().x),
        camera.y + static_cast<float>(input.mousePosition().y)
    );
    inventoryFullTimer_ = std::max(0.0f, inventoryFullTimer_ - dt);

    switch (state_) {
        case GameState::Playing:
            updatePlaying(dt, input);
            break;

        case GameState::GameOver:
            if (input.restart()) {
                reset();
            }
            break;

        case GameState::MapComplete:
            // F still loots Boss drops. Number keys still only drive reward / next-map
            // choice (handled below). Tab/Del let the player free bag space so F can
            // pick up more drops. tryEquipInventoryItem is intentionally NOT called so
            // 1-9 stays mapped to reward/map choices and never equips during settlement.
            tryPickupDroppedItem(input);
            trySelectInventoryItem(input);
            tryDropSelectedInventoryItem(input);
            removeDeadObjects();
            if (!mapRewardChosen_) {
                tryChooseMapReward(input);
            } else {
                tryChooseNextMapOption(input);
            }
            if (mapRewardChosen_ && nextMapOptionChosen_ && input.nextMap()) {
                startNextMap();
            } else if (input.restart()) {
                reset();
            }
            break;
    }

    input.update();
}

void GameWorld::updatePlaying(float dt, Input& input) {
    if (input.passiveTreeToggle()) {
        passiveTreeOpen_ = !passiveTreeOpen_;
        if (passiveTreeOpen_) {
            skillPanelOpen_ = false;
        }
    }

    if (input.skillPanelToggle()) {
        skillPanelOpen_ = !skillPanelOpen_;
        if (skillPanelOpen_) {
            passiveTreeOpen_ = false;
            hoveredPassiveNode_ = -1;
        }
    }

    Vector2 movement;
    if (input.moveLeft()) movement.x -= 1.0f;
    if (input.moveRight()) movement.x += 1.0f;
    if (input.moveUp()) movement.y -= 1.0f;
    if (input.moveDown()) movement.y += 1.0f;
    if (movement.lengthSquared() > 0.0f) {
        movePlayerBy(movement.normalized() * player_.moveSpeed() * dt);
    }

    player_.update(dt);
    skillBar_.update(dt);
    novaEffectTimer_ = std::max(0.0f, novaEffectTimer_ - dt);
    secondarySkillEffectTimer_ = std::max(0.0f, secondarySkillEffectTimer_ - dt);
    bossAoeEffectTimer_ = std::max(0.0f, bossAoeEffectTimer_ - dt);
    playerHitCooldown_ = std::max(0.0f, playerHitCooldown_ - dt);
    shrineBuffTimer_ = std::max(0.0f, shrineBuffTimer_ - dt);
    if (lifeFlaskStatusTimer_ > 0.0f) {
        lifeFlaskStatusTimer_ = std::max(0.0f, lifeFlaskStatusTimer_ - dt);
        if (lifeFlaskStatusTimer_ == 0.0f) {
            lifeFlaskStatusMessage_.clear();
        }
    }
    if (eventStatusTimer_ > 0.0f) {
        eventStatusTimer_ = std::max(0.0f, eventStatusTimer_ - dt);
        if (eventStatusTimer_ == 0.0f) {
            eventStatusMessage_.clear();
        }
    }
    nearbyEventPrompt_.clear();
    mapEventInteractionConsumed_ = false;

    if (passiveTreeOpen_) {
        updatePassiveTreeHover(input);
        trySpendPassivePoint(input);
    } else if (skillPanelOpen_) {
        hoveredPassiveNode_ = -1;
        tryAssignSkill(input);
        tryCycleSkillSupport(input);
    } else {
        hoveredPassiveNode_ = -1;
        tryCastMovementSkill(input);
        tryCastUtilitySkill(input);
        tryCastSecondarySkill(input);
    }

    updateMapEvents(dt, input);
    if (!mapEventInteractionConsumed_) {
        tryPickupDroppedItem(input);
    }

    if (!passiveTreeOpen_ && !skillPanelOpen_) {
        tryUseLifeFlask(input);
        trySelectInventoryItem(input);
        tryDropSelectedInventoryItem(input);
        tryEquipInventoryItem(input);
        tryCastPrimarySkill(input);
    }

    spawnEnemies(dt);
    updateObjects(dt);
    updateBossSkills(dt);
    updateBossProjectiles(dt);
    updateEnemyProjectiles(dt);
    spawner_.setSpawnInterval(currentSpawnInterval());
    handleCollisions();
    handleBossProjectileCollisions();
    handleEnemyProjectileCollisions();
    removeDeadObjects();
    advanceWaveIfComplete();

    survivalTime_ += dt;

    if (player_.isDead()) {
        state_ = GameState::GameOver;
    } else if (isMapCleared()) {
        state_ = GameState::MapComplete;
    }
}

void GameWorld::movePlayerBy(const Vector2& delta) {
    player_.setPosition(map_.resolveMovement(player_.position(), player_.radius(), delta));
}

void GameWorld::reset() {
    player_ = Player();
    map_ = MapInstance(1, 0);
    bossDefinition_ = &BossLibrary::forMapLevel(1);
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    projectiles_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    droppedItems_.clear();
    inventory_.clear();
    spawner_.reset();
    skillBar_.reset();
    initializeRunProgression();
    skillBar_.applyStats(player_.stats());
    state_ = GameState::Playing;
    score_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectPosition_ = Vector2(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f);
    secondarySkillEffectTimer_ = 0.0f;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    bossSkillTimer_ = bossDefinition_->skillInterval;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    playerHitCooldown_ = 0.0f;
    mapLevel_ = 1;
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    mapKills_ = 0;
    mapExperienceGained_ = 0;
    mapItemsDropped_ = 0;
    mapBossItemsDropped_ = 0;
    mapItemsPickedUp_ = 0;
    nextMapOptionChosen_ = false;
    mapRewardChosen_ = false;
    currentMapOption_ = MapOptionLibrary::defaultOption();
    nextMapOptions_ = MapOptionLibrary::generateOptions(2);
    selectedNextMapOption_ = -1;
    mapRewardOptions_ = {};
    selectedMapRewardOption_ = -1;
    mapModifier_ = currentMapOption_.modifier;
    passiveTreeOpen_ = false;
    skillPanelOpen_ = false;
    hoveredPassiveNode_ = -1;
    nearbyEventPrompt_.clear();
    shrineBuffTimer_ = 0.0f;
    lifeFlaskCharges_ = Config::LifeFlaskMaxCharges;
    lifeFlaskStatusMessage_.clear();
    lifeFlaskStatusTimer_ = 0.0f;
    inventoryFullTimer_ = 0.0f;
    selectedInventoryIndex_ = -1;
    mapEventInteractionConsumed_ = false;
    activeEliteEventIndex_ = -1;
    eliteEventEnemiesRemaining_ = 0;
    eventStatusMessage_.clear();
    eventStatusTimer_ = 0.0f;
}

void GameWorld::startNextMap() {
    if (selectedNextMapOption_ >= 0 && selectedNextMapOption_ < static_cast<int>(nextMapOptions_.size())) {
        currentMapOption_ = nextMapOptions_[static_cast<std::size_t>(selectedNextMapOption_)];
    }

    ++mapLevel_;
    map_ = MapInstance(mapLevel_, currentMapOption_.templateIndex);
    bossDefinition_ = &BossLibrary::forMapLevel(mapLevel_);
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectTimer_ = 0.0f;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    bossSkillTimer_ = bossDefinition_->skillInterval;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    playerHitCooldown_ = 0.0f;

    projectiles_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    droppedItems_.clear();
    spawner_.reset();
    skillBar_.applyStats(player_.stats());
    state_ = GameState::Playing;
    mapKills_ = 0;
    mapExperienceGained_ = 0;
    mapItemsDropped_ = 0;
    mapBossItemsDropped_ = 0;
    mapItemsPickedUp_ = 0;
    mapRewardChosen_ = false;
    nextMapOptionChosen_ = false;
    selectedNextMapOption_ = -1;
    mapRewardOptions_ = {};
    selectedMapRewardOption_ = -1;
    mapModifier_ = currentMapOption_.modifier;
    mapModifier_.itemQuantityMultiplier *= progression_.itemQuantityRewardMultiplier;
    passiveTreeOpen_ = false;
    skillPanelOpen_ = false;
    hoveredPassiveNode_ = -1;
    nearbyEventPrompt_.clear();
    shrineBuffTimer_ = 0.0f;
    lifeFlaskCharges_ = Config::LifeFlaskMaxCharges;
    lifeFlaskStatusMessage_.clear();
    lifeFlaskStatusTimer_ = 0.0f;
    inventoryFullTimer_ = 0.0f;
    selectedInventoryIndex_ = -1;
    mapEventInteractionConsumed_ = false;
    activeEliteEventIndex_ = -1;
    eliteEventEnemiesRemaining_ = 0;
    eventStatusMessage_.clear();
    eventStatusTimer_ = 0.0f;
}

void GameWorld::updateObjects(float dt) {
    for (auto& projectile : projectiles_) {
        const Vector2 previousPosition = projectile.position();
        projectile.update(dt, map_.size());
        if (projectile.isAlive() && map_.pathIntersectsObstacle(
                previousPosition, projectile.position(), projectile.radius()
            )) {
            projectile.kill();
        }
    }
    for (auto& enemy : enemies_) {
        enemy.update(dt, player_.position(), map_);
    }
}

void GameWorld::updateBossSkills(float dt) {
    const Enemy* boss = activeBoss();
    if (!boss) {
        bossAoeTelegraphTimer_ = 0.0f;
        bossSkillTimer_ = bossDefinition_->skillInterval;
        return;
    }

    const float hpRatio = boss->maxHp() > 0
        ? static_cast<float>(std::max(0, boss->hp())) / static_cast<float>(boss->maxHp())
        : 0.0f;
    if (!bossEnraged_ && hpRatio <= bossDefinition_->enrageHealthRatio) {
        bossEnraged_ = true;
        eventStatusMessage_ = "Boss enraged: " + bossDefinition_->name;
        eventStatusTimer_ = 2.0f;
        bossSkillTimer_ = std::min(bossSkillTimer_, bossSkillInterval());
    }

    const float telegraphBefore = bossAoeTelegraphTimer_;
    bossAoeTelegraphTimer_ = std::max(0.0f, bossAoeTelegraphTimer_ - dt);
    if (telegraphBefore > 0.0f && bossAoeTelegraphTimer_ <= 0.0f) {
        if (Collision::circleCircle(
                player_.position(), player_.radius(),
                bossAoeCenter_, bossAoeSkill_.radius
            )) {
            damagePlayer(bossAoeSkill_.damage);
        }
        bossAoeEffectTimer_ = bossAoeSkill_.effectDuration;
    }

    if (bossAoeTelegraphTimer_ > 0.0f) {
        return;
    }

    bossSkillTimer_ = std::max(0.0f, bossSkillTimer_ - dt);
    if (bossSkillTimer_ > 0.0f) {
        return;
    }

    if (bossDefinition_->skills.empty()) {
        bossSkillTimer_ = bossSkillInterval();
        return;
    }

    const BossSkillDefinition& skill = bossDefinition_->skillForCast(
        static_cast<std::size_t>(bossSkillIndex_), bossEnraged_
    );

    if (skill.type == BossSkillType::CircularAoe) {
        bossAoeCenter_ = player_.position();
        bossAoeSkill_ = skill;
        bossAoeSkill_.damage = bossSkillDamage(skill.damage);
        bossAoeTelegraphTimer_ = skill.telegraphDuration;
    } else {
        const Vector2 direction = (player_.position() - boss->position()).normalized();
        if (direction.lengthSquared() > 0.0f) {
            const int projectileCount = std::max(1, skill.projectileCount);
            const float halfSpread = skill.spreadAngle * 0.5f;
            const float step = projectileCount > 1
                ? skill.spreadAngle / static_cast<float>(projectileCount - 1)
                : 0.0f;
            constexpr float degToRad = 3.14159265f / 180.0f;
            for (int i = 0; i < projectileCount; ++i) {
                const float angleDeg = projectileCount > 1
                    ? -halfSpread + step * static_cast<float>(i)
                    : 0.0f;
                const float angleRad = angleDeg * degToRad;
                const float cosA = std::cos(angleRad);
                const float sinA = std::sin(angleRad);
                const Vector2 rotated(
                    direction.x * cosA - direction.y * sinA,
                    direction.x * sinA + direction.y * cosA
                );
                bossProjectiles_.push_back({
                    boss->position(),
                    rotated * skill.projectileSpeed,
                    skill.radius,
                    bossSkillDamage(skill.damage),
                    true
                });
            }
        }
    }

    ++bossSkillIndex_;
    bossSkillTimer_ = bossSkillInterval();
}

void GameWorld::updateBossProjectiles(float dt) {
    for (auto& projectile : bossProjectiles_) {
        if (!projectile.alive) {
            continue;
        }

        const Vector2 previousPosition = projectile.position;
        projectile.position += projectile.velocity * dt;
        if (projectile.position.y + projectile.radius < 0.0f
            || projectile.position.y - projectile.radius > map_.size().y
            || projectile.position.x + projectile.radius < 0.0f
            || projectile.position.x - projectile.radius > map_.size().x) {
            projectile.alive = false;
        }
        if (projectile.alive && map_.pathIntersectsObstacle(
                previousPosition, projectile.position, projectile.radius
            )) {
            projectile.alive = false;
        }
    }
}

void GameWorld::updateEnemyProjectiles(float dt) {
    for (auto& projectile : enemyProjectiles_) {
        if (!projectile.alive) {
            continue;
        }

        const Vector2 previousPosition = projectile.position;
        projectile.position += projectile.velocity * dt;
        if (projectile.position.y + projectile.radius < 0.0f
            || projectile.position.y - projectile.radius > map_.size().y
            || projectile.position.x + projectile.radius < 0.0f
            || projectile.position.x - projectile.radius > map_.size().x) {
            projectile.alive = false;
        }
        if (projectile.alive && map_.pathIntersectsObstacle(
                previousPosition, projectile.position, projectile.radius
            )) {
            projectile.alive = false;
        }
    }
}

void GameWorld::spawnEnemies(float dt) {
    triggerBossIfNeeded();

    if (map_.bossDefeated() || map_.bossTriggered()) {
        return;
    }

    const MapArea area = map_.areaForPlayer(player_.position());
    if (area == MapArea::Start || area == MapArea::BossGate || area == MapArea::BossArena) {
        return;
    }

    if (static_cast<int>(enemies_.size()) >= Config::MaxActiveEnemies) {
        return;
    }

    spawner_.update(dt);
    const bool spawnElite = (std::rand() % 100) < std::min(20, 6 + mapLevel_ * 2);
    const bool spawnRanged = !spawnElite && (std::rand() % 100) < 25;
    const EnemyType type = spawnElite ? EnemyType::Elite
        : spawnRanged ? EnemyType::Ranged
        : EnemyType::Normal;
    const auto& definition = EnemyLibrary::forType(type);
    const int hp = std::max(1, static_cast<int>(std::ceil(enemyHpForMap() * definition.hpMultiplier)));
    const int damage = enemyDamageForMap() + definition.damageBonus;

    if (auto enemy = spawner_.trySpawnNear(player_.position(), map_.size(), map_, hp, damage, type)) {
        enemies_.push_back(*enemy);
        ++enemiesSpawnedInWave_;
    }
}

void GameWorld::handleCollisions() {
    for (auto& projectile : projectiles_) {
        for (auto& enemy : enemies_) {
            if (!projectile.isAlive() || enemy.isDead()) {
                continue;
            }

            if (Collision::circleCircle(
                    projectile.position(), projectile.radius(),
                    enemy.position(), enemy.radius()
                )) {
                if (projectile.hasHitEnemy(enemy.id())) {
                    continue;
                }

                enemy.takeDamage(projectile.damage());
                projectile.recordEnemyHit(enemy.id());

                if (enemy.isDead()) {
                    rewardEnemyKill(enemy);
                }
            }
        }
    }

    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (enemy.isBoss()) {
            if (Collision::circleCircle(
                    player_.position(), player_.radius(),
                    enemy.position(), enemy.radius()
                )) {
                damagePlayer(enemy.contactDamage());
            }
            continue;
        }

        if (!enemy.consumeAttack()) {
            continue;
        }

        const Vector2 toPlayer = player_.position() - enemy.position();
        if (enemy.isRanged()) {
            const auto& definition = EnemyLibrary::forType(enemy.type());
            const Vector2 direction = toPlayer.normalized();
            if (direction.lengthSquared() > 0.0f) {
                enemyProjectiles_.push_back({
                    enemy.position(),
                    direction * definition.projectileSpeed,
                    definition.projectileRadius,
                    enemy.contactDamage(),
                    true
                });
            }
        } else if (toPlayer.lengthSquared() <= enemy.attackRange() * enemy.attackRange()) {
            damagePlayer(enemy.contactDamage());
        }
    }
}

void GameWorld::handleBossProjectileCollisions() {
    for (auto& projectile : bossProjectiles_) {
        if (!projectile.alive) {
            continue;
        }

        if (Collision::circleCircle(
                player_.position(), player_.radius(),
                projectile.position, projectile.radius
            )) {
            damagePlayer(projectile.damage);
            projectile.alive = false;
        }
    }
}

void GameWorld::handleEnemyProjectileCollisions() {
    for (auto& projectile : enemyProjectiles_) {
        if (!projectile.alive) {
            continue;
        }

        if (Collision::circleCircle(
                player_.position(), player_.radius(),
                projectile.position, projectile.radius
            )) {
            damagePlayer(projectile.damage);
            projectile.alive = false;
        }
    }
}

void GameWorld::removeDeadObjects() {
    auto projIt = std::remove_if(projectiles_.begin(), projectiles_.end(),
        [](const Projectile& p) { return !p.isAlive(); });
    if (projIt != projectiles_.end()) {
        projectiles_.erase(projIt, projectiles_.end());
    }

    auto enemyIt = std::remove_if(enemies_.begin(), enemies_.end(),
        [](const Enemy& e) { return e.isDead(); });
    if (enemyIt != enemies_.end()) {
        enemies_.erase(enemyIt, enemies_.end());
    }

    auto bossProjectileIt = std::remove_if(bossProjectiles_.begin(), bossProjectiles_.end(),
        [](const BossProjectile& projectile) { return !projectile.alive; });
    if (bossProjectileIt != bossProjectiles_.end()) {
        bossProjectiles_.erase(bossProjectileIt, bossProjectiles_.end());
    }

    auto enemyProjectileIt = std::remove_if(enemyProjectiles_.begin(), enemyProjectiles_.end(),
        [](const EnemyProjectile& projectile) { return !projectile.alive; });
    if (enemyProjectileIt != enemyProjectiles_.end()) {
        enemyProjectiles_.erase(enemyProjectileIt, enemyProjectiles_.end());
    }

    auto itemIt = std::remove_if(droppedItems_.begin(), droppedItems_.end(),
        [](const DroppedItem& item) { return item.isCollected(); });
    if (itemIt != droppedItems_.end()) {
        droppedItems_.erase(itemIt, droppedItems_.end());
    }
}

void GameWorld::tryCastMovementSkill(Input& input) {
    if (!input.dash() || !skillBar_.tryCast(SkillSlot::Movement)) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() == 0.0f) {
        return;
    }

    movePlayerBy(direction * Config::DashDistance);
}

void GameWorld::tryCastUtilitySkill(Input& input) {
    if (!input.nova() || !skillBar_.tryCast(SkillSlot::Utility)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Utility);
    dealAreaDamage(
        player_.position(),
        radiusForPlayerSkill(skill),
        damageForPlayerSkill(skill)
    );
    novaEffectTimer_ = skill.effectDuration;
}

void GameWorld::tryCastSecondarySkill(Input& input) {
    if (!input.secondarySkill() || !skillBar_.tryCast(SkillSlot::Secondary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Secondary);
    dealAreaDamage(
        aimPosition_,
        radiusForPlayerSkill(skill),
        damageForPlayerSkill(skill)
    );
    secondarySkillEffectPosition_ = aimPosition_;
    secondarySkillEffectTimer_ = skill.effectDuration;
}

void GameWorld::tryCastPrimarySkill(Input& input) {
    if (!input.primaryFireHeld()) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() <= 0.0f) {
        return;
    }

    if (!skillBar_.tryCast(SkillSlot::Primary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Primary);
    const int damage = damageForPlayerSkill(skill);

    if (skill.projectileCount <= 1 || skill.spreadAngle <= 0.0f) {
        projectiles_.push_back(Projectile(
            player_.position(), direction * Config::ProjectileSpeed, damage, pierceCountForPlayerSkill(skill)
        ));
        return;
    }

    const float degToRad = 3.14159265f / 180.0f;
    const float halfSpread = skill.spreadAngle * 0.5f;
    const float step = skill.spreadAngle / static_cast<float>(skill.projectileCount - 1);
    for (int i = 0; i < skill.projectileCount; ++i) {
        const float angleDeg = -halfSpread + step * static_cast<float>(i);
        const float angleRad = angleDeg * degToRad;
        const float cosA = std::cos(angleRad);
        const float sinA = std::sin(angleRad);
        const Vector2 rotated(
            direction.x * cosA - direction.y * sinA,
            direction.x * sinA + direction.y * cosA
        );
        projectiles_.push_back(Projectile(
            player_.position(),
            rotated * Config::ProjectileSpeed,
            damage,
            pierceCountForPlayerSkill(skill)
        ));
    }
}

void GameWorld::tryUseLifeFlask(Input& input) {
    if (!input.useLifeFlask()) {
        return;
    }

    if (lifeFlaskCharges_ <= 0) {
        lifeFlaskStatusMessage_ = "Life flask empty";
        lifeFlaskStatusTimer_ = 1.5f;
        return;
    }

    const int healed = player_.heal(Config::LifeFlaskHealAmount);
    if (healed <= 0) {
        return;
    }

    --lifeFlaskCharges_;
    lifeFlaskStatusMessage_ = "Life flask: +" + std::to_string(healed) + " HP";
    lifeFlaskStatusTimer_ = 1.5f;
}

void GameWorld::dealAreaDamage(const Vector2& center, float radius, int damage) {
    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (Collision::circleCircle(
                center, radius,
                enemy.position(), enemy.radius()
            )) {
            enemy.takeDamage(damage);

            if (enemy.isDead()) {
                rewardEnemyKill(enemy);
            }
        }
    }
}

void GameWorld::updateMapEvents(float /*dt*/, Input& input) {
    if (map_.bossTriggered() || map_.bossDefeated()) {
        return;
    }

    auto& events = map_.eventsForMutation();
    for (std::size_t i = 0; i < events.size(); ++i) {
        auto& event = events[i];
        if (event.completed) {
            continue;
        }

        const Vector2 diff = player_.position() - event.position;
        if (diff.lengthSquared() > event.radius * event.radius) {
            continue;
        }

        switch (event.type) {
            case MapEventType::LootCache:
                nearbyEventPrompt_ = "F Open Cache";
                if (input.pickup()) {
                    openLootCacheEvent(event);
                    mapEventInteractionConsumed_ = true;
                }
                return;

            case MapEventType::Shrine:
                nearbyEventPrompt_ = "F Activate Shrine";
                if (input.pickup()) {
                    activateShrineEvent(event);
                    mapEventInteractionConsumed_ = true;
                }
                return;

            case MapEventType::ElitePack:
                nearbyEventPrompt_ = event.triggered ? "Elite Pack active" : "Elite Pack ambush";
                if (!event.triggered) {
                    triggerElitePackEvent(i);
                }
                return;
        }
    }
}

void GameWorld::triggerElitePackEvent(std::size_t eventIndex) {
    auto& events = map_.eventsForMutation();
    if (eventIndex >= events.size()) {
        return;
    }

    auto& event = events[eventIndex];
    if (event.triggered || event.completed) {
        return;
    }

    event.triggered = true;
    activeEliteEventIndex_ = static_cast<int>(eventIndex);
    eliteEventEnemiesRemaining_ = 5;
    eventStatusMessage_ = "Elite pack awakened";
    eventStatusTimer_ = 2.0f;

    const Vector2 offsets[] = {
        {0.0f, 0.0f},
        {-64.0f, -42.0f},
        {62.0f, -34.0f},
        {-48.0f, 58.0f},
        {54.0f, 52.0f}
    };

    const auto spawnEventEnemy = [&](EnemyType type, const Vector2& position) {
        const auto& definition = EnemyLibrary::forType(type);
        const int hp = std::max(1, static_cast<int>(std::ceil(enemyHpForMap() * definition.hpMultiplier)));
        const int damage = enemyDamageForMap() + definition.damageBonus;
        enemies_.push_back(Enemy(position, hp, damage, type));
    };

    spawnEventEnemy(EnemyType::Elite, event.position + offsets[0]);
    for (std::size_t i = 1; i < 5; ++i) {
        spawnEventEnemy(EnemyType::Normal, event.position + offsets[i]);
    }
}

void GameWorld::openLootCacheEvent(MapEventInstance& event) {
    event.triggered = true;
    event.completed = true;
    dropItemsAround(event.position, 2);
    eventStatusMessage_ = "Cache opened: 2 items dropped";
    eventStatusTimer_ = 2.0f;
}

void GameWorld::activateShrineEvent(MapEventInstance& event) {
    event.triggered = true;
    event.completed = true;
    shrineBuffTimer_ = ShrineBuffDuration;
    eventStatusMessage_ = "Shrine activated: +35% damage";
    eventStatusTimer_ = 2.0f;
}

void GameWorld::dropItemsAround(const Vector2& center, int count) {
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) * 2.39996323f;
        const float radius = i == 0 ? 0.0f : 24.0f + static_cast<float>(i) * 5.0f;
        const Vector2 offset(std::cos(angle) * radius, std::sin(angle) * radius);
        droppedItems_.push_back(DroppedItem(center + offset, lootGenerator_.generate(mapLevel_)));
        ++mapItemsDropped_;
    }
}

int GameWorld::damageForPlayerSkill(const SkillDefinition& skill) const {
    float damage = static_cast<float>(skill.baseDamage) * player_.stats().damageMultiplier;
    switch (skill.castType) {
        case SkillCastType::Projectile:
            damage *= player_.stats().projectileDamageMultiplier;
            break;
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            damage *= player_.stats().areaDamageMultiplier;
            break;
        case SkillCastType::Dash:
            break;
    }

    if (const auto* support = skillBar_.support(skill.slot)) {
        damage *= support->damageMultiplier;
    }

    const float multiplier = shrineBuffTimer_ > 0.0f ? ShrineDamageMultiplier : 1.0f;
    return std::max(1, static_cast<int>(std::ceil(damage * multiplier)));
}

float GameWorld::radiusForPlayerSkill(const SkillDefinition& skill) const {
    const float supportMultiplier = [&]() {
        const auto* support = skillBar_.support(skill.slot);
        return support ? support->radiusMultiplier : 1.0f;
    }();

    switch (skill.castType) {
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            return skill.radius * player_.stats().areaRadiusMultiplier * supportMultiplier;
        case SkillCastType::Projectile:
        case SkillCastType::Dash:
            return skill.radius;
    }

    return skill.radius;
}

int GameWorld::pierceCountForPlayerSkill(const SkillDefinition& skill) const {
    const auto* support = skillBar_.support(skill.slot);
    return support ? support->pierceCount : 0;
}

void GameWorld::noteElitePackEnemyDefeated(const Enemy& enemy) {
    if (enemy.isBoss() || activeEliteEventIndex_ < 0 || eliteEventEnemiesRemaining_ <= 0) {
        return;
    }

    auto& events = map_.eventsForMutation();
    const auto eventIndex = static_cast<std::size_t>(activeEliteEventIndex_);
    if (eventIndex >= events.size()) {
        return;
    }

    auto& event = events[eventIndex];
    const float completionRadius = event.radius + 240.0f;
    if ((enemy.position() - event.position).lengthSquared() > completionRadius * completionRadius) {
        return;
    }

    --eliteEventEnemiesRemaining_;
    if (eliteEventEnemiesRemaining_ <= 0) {
        event.completed = true;
        activeEliteEventIndex_ = -1;
        eventStatusMessage_ = "Elite pack cleared";
        eventStatusTimer_ = 2.0f;
    }
}

int GameWorld::focusedDroppedItemIndex() const {
    const float itemPickupRange = (Config::ItemPickupRange + player_.radius())
        * player_.stats().pickupRangeMultiplier;
    const float rangeSq = itemPickupRange * itemPickupRange;

    int bestIndex = -1;
    // Strictly above any in-range distance so that, on ties, the earlier
    // item (already recorded with a smaller index) is kept -> stable behavior.
    float bestDistSq = rangeSq + 1.0f;
    for (std::size_t i = 0; i < droppedItems_.size(); ++i) {
        const auto& droppedItem = droppedItems_[i];
        if (droppedItem.isCollected()) {
            continue;
        }

        const Vector2 diff = player_.position() - droppedItem.position();
        const float distSq = diff.lengthSquared();
        if (distSq <= rangeSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIndex = static_cast<int>(i);
        }
    }
    return bestIndex;
}

void GameWorld::tryPickupDroppedItem(Input& input) {
    if (!input.pickup()) {
        return;
    }

    const int index = focusedDroppedItemIndex();
    if (index < 0) {
        return;
    }

    if (inventory_.isFull()) {
        // Inventory is full: keep the focused item on the ground and notify the player.
        inventoryFullTimer_ = 1.5f;
        return;
    }

    inventory_.add(droppedItems_[static_cast<std::size_t>(index)].collect());
    droppedItems_.erase(droppedItems_.begin() + static_cast<std::ptrdiff_t>(index));
    ++mapItemsPickedUp_;
}

void GameWorld::trySpendPassivePoint(Input& input) {
    if (!passiveTreeOpen_) {
        return;
    }

    if (input.leftMousePressed() && hoveredPassiveNode_ >= 0) {
        if (player_.spendPassivePoint(static_cast<std::size_t>(hoveredPassiveNode_))) {
            skillBar_.applyStats(player_.stats());
        }
        return;
    }

    const int choice = input.functionChoice() > 0
        ? input.functionChoice() + 10
        : input.numberChoice();
    if (choice <= 0) {
        return;
    }

    const auto nodeIndex = static_cast<std::size_t>(choice - 1);
    if (player_.spendPassivePoint(nodeIndex)) {
        skillBar_.applyStats(player_.stats());
    }
}

void GameWorld::updatePassiveTreeHover(const Input& input) {
    const Vector2 treePosition(
        static_cast<float>(input.mousePosition().x) - static_cast<float>(Config::WindowWidth) / 2.0f,
        static_cast<float>(input.mousePosition().y) - static_cast<float>(Config::WindowHeight) / 2.0f
    );
    hoveredPassiveNode_ = player_.passiveTree().nodeAtPosition(treePosition, 19.0f);
}

void GameWorld::tryAssignSkill(Input& input) {
    if (!skillPanelOpen_ || input.numberChoice() <= 0) {
        return;
    }

    const auto& skills = SkillLibrary::all();
    const auto index = static_cast<std::size_t>(input.numberChoice() - 1);
    if (index >= skills.size()) {
        return;
    }

    const auto& skill = skills[index];
    if (!isSkillUnlocked(skill.name)) {
        return;
    }

    if (skillBar_.assignSkill(skill.slot, skill.name)) {
        skillBar_.applyStats(player_.stats());
    }
}

void GameWorld::tryCycleSkillSupport(Input& input) {
    if (!skillPanelOpen_ || input.functionChoice() <= 0 || input.functionChoice() > 3) {
        return;
    }

    const SkillSlot slot = static_cast<SkillSlot>(input.functionChoice() - 1);
    const auto& skill = skillBar_.definition(slot);
    std::vector<std::string> options = {""};
    for (const auto& support : SupportLibrary::all()) {
        if (isSupportUnlocked(support.name) && SupportLibrary::supportsSkill(support, skill)) {
            options.push_back(support.name);
        }
    }

    const auto* current = skillBar_.support(slot);
    const std::string currentName = current ? current->name : "";
    auto currentIt = std::find(options.begin(), options.end(), currentName);
    const std::size_t currentIndex = currentIt == options.end()
        ? 0
        : static_cast<std::size_t>(currentIt - options.begin());
    const std::string& next = options[(currentIndex + 1) % options.size()];
    if (skillBar_.assignSupport(slot, next)) {
        skillBar_.applyStats(player_.stats());
    }
}

void GameWorld::tryEquipInventoryItem(Input& input) {
    if (input.numberChoice() <= 0) {
        return;
    }

    const auto index = static_cast<std::size_t>(input.numberChoice() - 1);
    if (auto item = inventory_.take(index)) {
        if (auto replaced = player_.equipItem(std::move(*item))) {
            // The candidate was removed first, so there is normally room for the
            // replaced item. In the extreme case it does not fit, drop it at the
            // player's feet rather than losing it.
            Item oldItem = std::move(*replaced);
            if (!inventory_.isFull()) {
                inventory_.add(std::move(oldItem));
            } else {
                droppedItems_.push_back(DroppedItem(player_.position(), std::move(oldItem)));
            }
        }
        skillBar_.applyStats(player_.stats());
    }
    updateSelectedInventoryIndex();
}

void GameWorld::trySelectInventoryItem(Input& input) {
    if (!input.inventorySelectNext()) {
        return;
    }

    const std::size_t size = inventory_.size();
    if (size == 0) {
        selectedInventoryIndex_ = -1;
        return;
    }
    selectedInventoryIndex_ = (selectedInventoryIndex_ + 1) % static_cast<int>(size);
}

void GameWorld::tryDropSelectedInventoryItem(Input& input) {
    if (!input.inventoryDropSelected()) {
        return;
    }

    if (selectedInventoryIndex_ < 0
        || static_cast<std::size_t>(selectedInventoryIndex_) >= inventory_.size()) {
        return;
    }

    const std::size_t index = static_cast<std::size_t>(selectedInventoryIndex_);
    if (auto item = inventory_.take(index)) {
        const float pickupRange = (Config::ItemPickupRange + player_.radius())
            * player_.stats().pickupRangeMultiplier;
        const float dropDistance = pickupRange + Config::ItemDropRadius + 12.0f;
        const Vector2 offsets[] = {
            Vector2(dropDistance, -dropDistance * 0.35f),
            Vector2(-dropDistance, -dropDistance * 0.35f),
            Vector2(dropDistance, dropDistance * 0.35f),
            Vector2(-dropDistance, dropDistance * 0.35f),
        };

        Vector2 dropPos = player_.position() + offsets[0];
        float bestDistSq = -1.0f;
        for (const auto& offset : offsets) {
            Vector2 candidate = player_.position() + offset;
            candidate.x = std::clamp(candidate.x, Config::ItemDropRadius, map_.size().x - Config::ItemDropRadius);
            candidate.y = std::clamp(candidate.y, Config::ItemDropRadius, map_.size().y - Config::ItemDropRadius);
            const float distSq = (candidate - player_.position()).lengthSquared();
            if (distSq > bestDistSq) {
                bestDistSq = distSq;
                dropPos = candidate;
            }
        }
        droppedItems_.push_back(DroppedItem(dropPos, std::move(*item)));
    }
    updateSelectedInventoryIndex();
}

void GameWorld::updateSelectedInventoryIndex() {
    const std::size_t size = inventory_.size();
    if (size == 0) {
        selectedInventoryIndex_ = -1;
        return;
    }
    if (selectedInventoryIndex_ >= static_cast<int>(size)) {
        selectedInventoryIndex_ = static_cast<int>(size) - 1;
    }
}

void GameWorld::tryChooseNextMapOption(Input& input) {
    if (nextMapOptionChosen_ || input.numberChoice() <= 0) {
        return;
    }

    const int optionIndex = input.numberChoice() - 1;
    if (optionIndex < 0 || optionIndex >= static_cast<int>(nextMapOptions_.size())) {
        return;
    }

    selectedNextMapOption_ = optionIndex;
    nextMapOptionChosen_ = true;
}

void GameWorld::tryChooseMapReward(Input& input) {
    if (mapRewardChosen_ || input.numberChoice() <= 0) {
        return;
    }

    const int optionIndex = input.numberChoice() - 1;
    if (optionIndex < 0 || optionIndex >= static_cast<int>(mapRewardOptions_.size())) {
        return;
    }

    selectedMapRewardOption_ = optionIndex;
    applyMapReward(mapRewardOptions_[static_cast<std::size_t>(optionIndex)]);
    mapRewardChosen_ = true;
}

void GameWorld::applyMapReward(const MapRewardDefinition& reward) {
    switch (reward.type) {
        case MapRewardType::UnlockSkill:
            if (!reward.skillName.empty()) {
                progression_.unlockedSkills.insert(reward.skillName);
            }
            break;
        case MapRewardType::UnlockSupport:
            if (!reward.supportName.empty()) {
                progression_.unlockedSupports.insert(reward.supportName);
            }
            break;
        case MapRewardType::Damage:
            player_.applyUpgrade(UpgradeType::Damage);
            break;
        case MapRewardType::MaxHp:
            player_.applyUpgrade(UpgradeType::MaxHp);
            break;
        case MapRewardType::ItemQuantity:
            progression_.itemQuantityRewardMultiplier *= reward.itemQuantityMultiplierBonus;
            mapModifier_.itemQuantityMultiplier *= reward.itemQuantityMultiplierBonus;
            break;
    }

    skillBar_.applyStats(player_.stats());
}

void GameWorld::generateMapRewardOptions() {
    mapRewardOptions_ = MapRewardLibrary::generateOptions(
        progression_.unlockedSkills, progression_.unlockedSupports
    );
    selectedMapRewardOption_ = -1;
    mapRewardChosen_ = false;
}

void GameWorld::generateNextMapOptions() {
    nextMapOptions_ = MapOptionLibrary::generateOptions(mapLevel_ + 1);
    selectedNextMapOption_ = -1;
    nextMapOptionChosen_ = false;
}

void GameWorld::initializeRunProgression() {
    progression_ = RunProgression();
    progression_.unlockedSkills.insert(SkillLibrary::spreadShot().name);
    progression_.unlockedSkills.insert(SkillLibrary::meteor().name);
    progression_.unlockedSkills.insert(SkillLibrary::pulse().name);
    progression_.unlockedSkills.insert(SkillLibrary::dash().name);
}

void GameWorld::rewardEnemyKill(const Enemy& enemy) {
    const auto& definition = EnemyLibrary::forType(enemy.type());

    if (enemy.isBoss()) {
        map_.markBossDefeated();
        bossProjectiles_.clear();
        bossAoeTelegraphTimer_ = 0.0f;
        bossAoeEffectTimer_ = 0.0f;
        bossAoeSkill_ = BossSkillDefinition();
        bossEnraged_ = false;
        eventStatusMessage_ = "Boss defeated: " + bossDefinition_->name;
        eventStatusTimer_ = 2.0f;
        generateMapRewardOptions();
        generateNextMapOptions();
    }

    ++mapKills_;
    score_ += definition.scoreReward;

    const int exp = Config::ExpPerKill * definition.expMultiplier;
    player_.gainExp(exp);
    mapExperienceGained_ += exp;

    const float eliteDropMultiplier = enemy.isBoss() ? bossDefinition_->dropMultiplier
        : definition.dropMultiplier;
    const int dropChance = std::min(100, static_cast<int>(
        Config::ItemDropChancePercent * mapModifier_.itemQuantityMultiplier
        * eliteDropMultiplier));

    int dropsToCreate = (std::rand() % 100) < dropChance ? 1 : 0;
    if (enemy.isBoss()) {
        dropsToCreate = std::max(dropsToCreate, bossDefinition_->guaranteedDrops + mapModifier_.bossDropBonus);
    }

    for (int i = 0; i < dropsToCreate; ++i) {
        const float angle = static_cast<float>(i) * 2.39996323f;
        const float radius = i == 0 ? 0.0f : 18.0f + static_cast<float>(i) * 4.0f;
        const Vector2 offset(std::cos(angle) * radius, std::sin(angle) * radius);
        droppedItems_.push_back(DroppedItem(enemy.position() + offset, lootGenerator_.generate(mapLevel_)));
        ++mapItemsDropped_;
        if (enemy.isBoss()) {
            ++mapBossItemsDropped_;
        }
    }

    noteElitePackEnemyDefeated(enemy);
}

void GameWorld::damagePlayer(int damage) {
    if (playerHitCooldown_ > 0.0f) {
        return;
    }

    player_.takeDamage(damage);
    playerHitCooldown_ = Config::PlayerHitCooldown;
}

const Enemy* GameWorld::activeBoss() const {
    for (const auto& enemy : enemies_) {
        if (enemy.isBoss() && !enemy.isDead()) {
            return &enemy;
        }
    }

    return nullptr;
}

float GameWorld::bossSkillInterval() const {
    return bossDefinition_->skillInterval * (bossEnraged_
        ? bossDefinition_->enragedSkillIntervalMultiplier
        : 1.0f);
}

int GameWorld::bossSkillDamage(int baseDamage) const {
    const float multiplier = bossEnraged_
        ? bossDefinition_->enragedDamageMultiplier
        : 1.0f;
    return std::max(1, static_cast<int>(std::ceil(baseDamage * multiplier)));
}

void GameWorld::advanceWaveIfComplete() {
}

bool GameWorld::isMapCleared() const {
    return map_.bossDefeated();
}

int GameWorld::enemiesPerWave() const {
    return Config::BaseEnemiesPerWave + (mapLevel_ - 1) * Config::EnemiesPerMapLevel;
}

int GameWorld::enemyHpForMap() const {
    const float levelMultiplier = 1.0f + (mapLevel_ - 1) * 0.25f;
    return std::max(1, static_cast<int>(std::ceil(
        Config::EnemyHp * levelMultiplier * mapModifier_.monsterHpMultiplier
    )));
}

int GameWorld::enemyDamageForMap() const {
    return Config::EnemyContactDamage + (mapLevel_ - 1) / 3 + mapModifier_.monsterDamageBonus;
}

bool GameWorld::shouldSpawnBoss() const {
    return !map_.bossTriggered()
        && map_.areaForPlayer(player_.position()) == MapArea::BossArena;
}

void GameWorld::triggerBossIfNeeded() {
    if (!shouldSpawnBoss()) {
        return;
    }

    map_.triggerBoss();
    enemies_.clear();
    projectiles_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    activeEliteEventIndex_ = -1;
    eliteEventEnemiesRemaining_ = 0;
    nearbyEventPrompt_.clear();
    mapEventInteractionConsumed_ = false;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    bossSkillTimer_ = bossDefinition_->skillInterval * 0.5f;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    eventStatusMessage_ = "Boss awakened: " + bossDefinition_->name;
    eventStatusTimer_ = 2.0f;

    const int hp = std::max(1, static_cast<int>(std::ceil(enemyHpForMap() * bossDefinition_->hpMultiplier)));
    const int damage = enemyDamageForMap() + bossDefinition_->damageBonus;
    enemies_.push_back(Enemy(map_.bossCenter(), hp, damage, EnemyType::Boss));
}

const Player& GameWorld::player() const { return player_; }
const std::vector<Projectile>& GameWorld::projectiles() const { return projectiles_; }
const std::vector<BossProjectile>& GameWorld::bossProjectiles() const { return bossProjectiles_; }
const std::vector<EnemyProjectile>& GameWorld::enemyProjectiles() const { return enemyProjectiles_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
const std::vector<DroppedItem>& GameWorld::droppedItems() const { return droppedItems_; }
const Inventory& GameWorld::inventory() const { return inventory_; }
int GameWorld::lifeFlaskCharges() const { return lifeFlaskCharges_; }
int GameWorld::lifeFlaskMaxCharges() const { return Config::LifeFlaskMaxCharges; }
std::string GameWorld::lifeFlaskStatusMessage() const { return lifeFlaskStatusMessage_; }
float GameWorld::lifeFlaskStatusTimeRemaining() const { return lifeFlaskStatusTimer_; }
const Vector2& GameWorld::aimPosition() const { return aimPosition_; }
float GameWorld::novaEffectProgress() const {
    const float duration = skillBar_.definition(SkillSlot::Utility).effectDuration;
    return duration > 0.0f ? novaEffectTimer_ / duration : 0.0f;
}
float GameWorld::novaEffectRadius() const {
    return radiusForPlayerSkill(skillBar_.definition(SkillSlot::Utility));
}
const Vector2& GameWorld::secondarySkillEffectPosition() const {
    return secondarySkillEffectPosition_;
}
float GameWorld::secondarySkillEffectProgress() const {
    const float duration = skillBar_.definition(SkillSlot::Secondary).effectDuration;
    return duration > 0.0f ? secondarySkillEffectTimer_ / duration : 0.0f;
}
float GameWorld::secondarySkillEffectRadius() const {
    return radiusForPlayerSkill(skillBar_.definition(SkillSlot::Secondary));
}
const Vector2& GameWorld::bossAoeCenter() const { return bossAoeCenter_; }
float GameWorld::bossAoeRadius() const { return bossAoeSkill_.radius; }
float GameWorld::bossAoeTelegraphProgress() const {
    return bossAoeSkill_.telegraphDuration > 0.0f
        ? bossAoeTelegraphTimer_ / bossAoeSkill_.telegraphDuration
        : 0.0f;
}
float GameWorld::bossAoeEffectProgress() const {
    return bossAoeSkill_.effectDuration > 0.0f
        ? bossAoeEffectTimer_ / bossAoeSkill_.effectDuration
        : 0.0f;
}
const BossDefinition& GameWorld::bossDefinition() const { return *bossDefinition_; }
const SkillBar& GameWorld::skillBar() const { return skillBar_; }
const MapInstance& GameWorld::map() const { return map_; }
MapArea GameWorld::currentMapArea() const { return map_.areaForPlayer(player_.position()); }
float GameWorld::distanceToBoss() const { return map_.distanceToBoss(player_.position()); }
std::string GameWorld::mapObjective() const {
    if (state_ == GameState::MapComplete || map_.bossDefeated()) {
        return mapRewardChosen_ ? "Choose Next Map" : "Choose Reward";
    }

    if (map_.bossTriggered()) {
        return "Defeat Boss";
    }

    switch (currentMapArea()) {
        case MapArea::Start:
            return "Explore the field";
        case MapArea::BossGate:
            return "Enter Boss Arena";
        case MapArea::Field:
            return "Reach Boss Gate";
        case MapArea::BossArena:
            return "Defeat Boss";
        case MapArea::BossDefeated:
            return "Choose Next Map";
    }

    return "Explore the field";
}
Vector2 GameWorld::cameraTopLeft() const {
    const float viewportWidth = static_cast<float>(Config::WindowWidth);
    const float viewportHeight = static_cast<float>(Config::WindowHeight);
    return {
        std::clamp(player_.position().x - viewportWidth / 2.0f, 0.0f, map_.size().x - viewportWidth),
        std::clamp(player_.position().y - viewportHeight / 2.0f, 0.0f, map_.size().y - viewportHeight)
    };
}
bool GameWorld::passiveTreeOpen() const { return passiveTreeOpen_; }
bool GameWorld::skillPanelOpen() const { return skillPanelOpen_; }
int GameWorld::hoveredPassiveNode() const { return hoveredPassiveNode_; }
std::string GameWorld::passiveBuildSummary() const {
    const auto& tree = player_.passiveTree();
    return "Projectile " + std::to_string(tree.allocatedCount(PassiveBranch::Projectile))
        + " / Area " + std::to_string(tree.allocatedCount(PassiveBranch::Area))
        + " / Survival " + std::to_string(tree.allocatedCount(PassiveBranch::Survival))
        + " / Loot " + std::to_string(tree.allocatedCount(PassiveBranch::Loot));
}
bool GameWorld::isSkillUnlocked(const std::string& name) const {
    return progression_.unlockedSkills.find(name) != progression_.unlockedSkills.end();
}
bool GameWorld::isSupportUnlocked(const std::string& name) const {
    return progression_.unlockedSupports.find(name) != progression_.unlockedSupports.end();
}
GameState GameWorld::state() const { return state_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }
int GameWorld::mapLevel() const { return mapLevel_; }
int GameWorld::currentWave() const {
    return 0;
}
int GameWorld::enemiesRemainingInWave() const {
    return static_cast<int>(enemies_.size());
}
const MapModifier& GameWorld::mapModifier() const { return mapModifier_; }
int GameWorld::mapKills() const { return mapKills_; }
int GameWorld::mapExperienceGained() const { return mapExperienceGained_; }
int GameWorld::mapItemsDropped() const { return mapItemsDropped_; }
int GameWorld::mapBossItemsDropped() const { return mapBossItemsDropped_; }
int GameWorld::mapItemsPickedUp() const { return mapItemsPickedUp_; }
std::string GameWorld::nearbyEventPrompt() const { return nearbyEventPrompt_; }
float GameWorld::shrineBuffTimeRemaining() const { return shrineBuffTimer_; }
float GameWorld::inventoryFullPromptTimeRemaining() const { return inventoryFullTimer_; }
std::string GameWorld::eventStatusMessage() const { return eventStatusMessage_; }
float GameWorld::eventStatusTimeRemaining() const { return eventStatusTimer_; }
int GameWorld::activeEliteEventEnemiesRemaining() const { return eliteEventEnemiesRemaining_; }
std::string GameWorld::bossSkillWarning() const {
    if (bossAoeTelegraphTimer_ <= 0.0f || bossAoeSkill_.name.empty()) {
        return "";
    }

    return "Boss casting: " + bossAoeSkill_.name;
}
bool GameWorld::bossEnraged() const { return bossEnraged_; }
std::string GameWorld::bossPhaseSummary() const {
    return bossEnraged_
        ? "Enraged: " + bossDefinition_->enragedPatternDescription
        : "Pattern: " + bossDefinition_->patternDescription;
}

std::string GameWorld::pickupPrompt() const {
    const int index = focusedDroppedItemIndex();
    if (index < 0) {
        return "";
    }

    const std::string& name = droppedItems_[static_cast<std::size_t>(index)].item().name;
    if (inventory_.isFull()) {
        return "Inventory full - " + name + " remains on ground";
    }
    return "F Pick up " + name;
}

int GameWorld::selectedInventoryIndex() const { return selectedInventoryIndex_; }
int GameWorld::mapEventsCompleted() const {
    return static_cast<int>(std::count_if(map_.events().begin(), map_.events().end(),
        [](const MapEventInstance& event) { return event.completed; }));
}
int GameWorld::mapEventsTotal() const { return static_cast<int>(map_.events().size()); }
bool GameWorld::nextMapOptionChosen() const { return nextMapOptionChosen_; }
bool GameWorld::mapRewardChosen() const { return mapRewardChosen_; }
const MapOption& GameWorld::currentMapOption() const { return currentMapOption_; }
const std::array<MapOption, 3>& GameWorld::nextMapOptions() const { return nextMapOptions_; }
int GameWorld::selectedNextMapOption() const { return selectedNextMapOption_; }
const std::array<MapRewardDefinition, 3>& GameWorld::mapRewardOptions() const { return mapRewardOptions_; }
int GameWorld::selectedMapRewardOption() const { return selectedMapRewardOption_; }

float GameWorld::currentSpawnInterval() const {
    constexpr float startInterval = Config::EnemySpawnInterval;
    constexpr float intervalPerMapLevel = 0.04f;
    constexpr float minimumInterval = 0.25f;

    return std::max(minimumInterval, startInterval - (mapLevel_ - 1) * intervalPerMapLevel);
}
