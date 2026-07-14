#include "GameWorld.hpp"
#include "Collision.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"
#include "MapScaling.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
LootBias bossLootBias(BossLootTheme theme) {
    switch (theme) {
        case BossLootTheme::Brimstone:
            return {AffixTag::Fire, 1.45f, AffixTag::Area, 1.20f};
        case BossLootTheme::Storm:
            return {AffixTag::Lightning, 1.45f, AffixTag::Projectile, 1.20f};
        case BossLootTheme::Brood:
            return {AffixTag::Poison, 1.45f, AffixTag::Area, 1.20f};
    }
    return {};
}

void mergeLootBias(LootBias& target, const LootBias& extra) {
    const auto add = [&](AffixTag tag, float multiplier) {
        if (tag == AffixTag::None || multiplier <= 0.0f) {
            return;
        }
        if (target.primaryTag == tag) {
            target.primaryWeightMultiplier *= multiplier;
        } else if (target.secondaryTag == tag) {
            target.secondaryWeightMultiplier *= multiplier;
        } else if (target.primaryTag == AffixTag::None) {
            target.primaryTag = tag;
            target.primaryWeightMultiplier = multiplier;
        } else if (target.secondaryTag == AffixTag::None) {
            target.secondaryTag = tag;
            target.secondaryWeightMultiplier = multiplier;
        }
    };

    add(extra.primaryTag, extra.primaryWeightMultiplier);
    add(extra.secondaryTag, extra.secondaryWeightMultiplier);
}

const char* ailmentTypeName(AilmentType type) {
    switch (type) {
        case AilmentType::Ignite: return "Ignite";
        case AilmentType::Chill: return "Chill";
        case AilmentType::Shock: return "Shock";
        case AilmentType::Poison: return "Poison";
        case AilmentType::None: break;
        case AilmentType::Count: break;
    }
    return "None";
}

std::string bossSkillWarningText(const BossSkillDefinition& skill) {
    std::string warning = "Boss casting: " + skill.name;
    if (skill.damageType != DamageType::Physical) {
        warning += " [" + std::string(damageTypeName(skill.damageType));
        if (skill.ailment.type != AilmentType::None) {
            warning += "/" + std::string(ailmentTypeName(skill.ailment.type));
        }
        warning += "]";
    }
    return warning;
}

bool positiveFinite(float value) {
    return std::isfinite(value) && value > 0.0f;
}

bool validStatsForRestore(const Stats& stats) {
    const auto validResistance = [](int value) {
        return value >= 0 && value <= 100;
    };
    return positiveFinite(stats.moveSpeedMultiplier)
        && positiveFinite(stats.damageMultiplier)
        && positiveFinite(stats.attackSpeedMultiplier)
        && positiveFinite(stats.pickupRangeMultiplier)
        && positiveFinite(stats.projectileDamageMultiplier)
        && positiveFinite(stats.areaDamageMultiplier)
        && positiveFinite(stats.areaRadiusMultiplier)
        && positiveFinite(stats.lifeFlaskEffectMultiplier)
        && positiveFinite(stats.itemQuantityMultiplier)
        && positiveFinite(stats.incomingDamageMultiplier)
        && positiveFinite(stats.fireDamageMultiplier)
        && positiveFinite(stats.coldDamageMultiplier)
        && positiveFinite(stats.lightningDamageMultiplier)
        && positiveFinite(stats.poisonDamageMultiplier)
        && validResistance(stats.fireResistance)
        && validResistance(stats.coldResistance)
        && validResistance(stats.lightningResistance)
        && validResistance(stats.poisonResistance);
}

bool statsMatchForRestore(const Stats& left, const Stats& right) {
    const auto close = [](float lhs, float rhs) {
        return std::abs(lhs - rhs) <= 0.0001f;
    };
    return left.maxHp == right.maxHp
        && close(left.moveSpeedMultiplier, right.moveSpeedMultiplier)
        && close(left.damageMultiplier, right.damageMultiplier)
        && close(left.attackSpeedMultiplier, right.attackSpeedMultiplier)
        && close(left.pickupRangeMultiplier, right.pickupRangeMultiplier)
        && close(left.projectileDamageMultiplier, right.projectileDamageMultiplier)
        && close(left.areaDamageMultiplier, right.areaDamageMultiplier)
        && close(left.areaRadiusMultiplier, right.areaRadiusMultiplier)
        && left.armor == right.armor
        && left.projectileCountBonus == right.projectileCountBonus
        && close(left.lifeFlaskEffectMultiplier, right.lifeFlaskEffectMultiplier)
        && close(left.itemQuantityMultiplier, right.itemQuantityMultiplier)
        && close(left.incomingDamageMultiplier, right.incomingDamageMultiplier)
        && close(left.fireDamageMultiplier, right.fireDamageMultiplier)
        && close(left.coldDamageMultiplier, right.coldDamageMultiplier)
        && close(left.lightningDamageMultiplier, right.lightningDamageMultiplier)
        && close(left.poisonDamageMultiplier, right.poisonDamageMultiplier)
        && left.fireResistance == right.fireResistance
        && left.coldResistance == right.coldResistance
        && left.lightningResistance == right.lightningResistance
        && left.poisonResistance == right.poisonResistance;
}

int mapElementalResistanceAdjustment(
    const MapModifier& modifier,
    DamageType damageType,
    bool monsterResistance
) {
    if (modifier.elementalChallengeId.empty()
        || modifier.elementalChallengeType != damageType) {
        return 0;
    }

    return monsterResistance
        ? modifier.monsterElementalResistanceBonus
        : -modifier.playerElementalResistancePenalty;
}

bool validItemForRestore(const Item& item) {
    const auto* base = ItemBaseLibrary::find(item.baseId);
    if (static_cast<int>(item.slot) < 0
        || static_cast<int>(item.slot) >= static_cast<int>(EquipmentSlot::Count)
        || static_cast<int>(item.rarity) < 0
        || static_cast<int>(item.rarity) > static_cast<int>(Rarity::Rare)
        || item.itemLevel < 1
        || base == nullptr
        || base->slot != item.slot
        || !statsMatchForRestore(item.implicitStats, base->implicitStats)
        || !validStatsForRestore(item.stats)
        || !validStatsForRestore(item.implicitStats)) {
        return false;
    }

    for (const auto& affix : item.affixes) {
        if (affix.tier < 1
            || static_cast<int>(affix.stat) < 0
            || static_cast<int>(affix.stat) > static_cast<int>(AffixStat::PoisonResistance)
            || !validStatsForRestore(affix.stats)) {
            return false;
        }
        for (const auto tag : affix.tags) {
            if (static_cast<int>(tag) < 0
                || static_cast<int>(tag) > static_cast<int>(AffixTag::Poison)) {
                return false;
            }
        }
    }

    Stats expected = item.implicitStats;
    for (const auto& affix : item.affixes) {
        expected = combineStats(expected, affix.stats);
    }
    return statsMatchForRestore(item.stats, expected);
}

bool validModifierEffectForRestore(const MapModifierEffect& effect) {
    return positiveFinite(effect.monsterHpMultiplier)
        && positiveFinite(effect.monsterSpeedMultiplier)
        && positiveFinite(effect.itemQuantityMultiplier)
        && positiveFinite(effect.bossHpMultiplier)
        && positiveFinite(effect.bossDamageMultiplier)
        && positiveFinite(effect.eventRewardMultiplier)
        && positiveFinite(effect.primaryLootBiasWeightMultiplier)
        && positiveFinite(effect.secondaryLootBiasWeightMultiplier);
}

bool validModifierForRestore(const MapModifier& modifier) {
    if (!positiveFinite(modifier.monsterHpMultiplier)
        || !positiveFinite(modifier.monsterSpeedMultiplier)
        || !positiveFinite(modifier.itemQuantityMultiplier)
        || !positiveFinite(modifier.bossHpMultiplier)
        || !positiveFinite(modifier.bossDamageMultiplier)
        || !positiveFinite(modifier.eventRewardMultiplier)
        || !positiveFinite(modifier.lootBiasWeightMultiplier)
        || !positiveFinite(modifier.secondaryLootBiasWeightMultiplier)
        || modifier.componentCount < 0 || modifier.componentCount > 2
        || modifier.playerElementalResistancePenalty < 0
        || modifier.playerElementalResistancePenalty > 100
        || modifier.monsterElementalResistanceBonus < 0
        || modifier.monsterElementalResistanceBonus > 100) {
        return false;
    }
    if (modifier.elementalChallengeId.empty()) {
        if (modifier.elementalChallengeType != DamageType::Physical
            || modifier.playerElementalResistancePenalty != 0
            || modifier.monsterElementalResistanceBonus != 0) {
            return false;
        }
    } else {
        const auto* challenge = MapModifierLibrary::findElementalChallenge(
            modifier.elementalChallengeId
        );
        if (challenge == nullptr || modifier.elementalChallengeType != challenge->damageType) {
            return false;
        }
    }
    for (int index = 0; index < modifier.componentCount; ++index) {
        if (!validModifierEffectForRestore(
                modifier.components[static_cast<std::size_t>(index)].effect)) {
            return false;
        }
    }
    return true;
}
}

GameWorld::GameWorld(std::uint64_t runSeed)
    : map_()
    , progression_()
    , runSeed_(runSeed)
    , random_(runSeed)
    , state_(GameState::Playing)
    , resumeState_(GameState::Playing)
    , score_(0)
    , survivalTime_(0.0f)
    , aimPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , novaEffectTimer_(0.0f)
    , secondarySkillEffectPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , secondarySkillEffectTimer_(0.0f)
    , dashImpactPosition_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , bossAoeCenter_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
    , bossAoeTelegraphTimer_(0.0f)
    , bossAoeEffectTimer_(0.0f)
    , volatileExplosionCenter_(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f)
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
    , selectedSupportLink_(0)
    , craftingState_()
    , hoveredPassiveNode_(-1)
    , nearbyEventPrompt_()
    , shrineBuffTimer_(0.0f)
    , lifeFlaskCharges_(Config::LifeFlaskMaxCharges)
    , lifeFlaskStatusMessage_()
    , lifeFlaskStatusTimer_(0.0f)
    , inventoryFullTimer_(0.0f)
    , selectedInventoryIndex_(-1)
    , selectedStashIndex_(-1)
    , stashSelectionActive_(false)
    , mapEventInteractionConsumed_(false)
    , activeMapEventIndex_(-1)
    , mapEventEnemiesRemaining_(0) {
    initializeRunProgression();
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    applySkillProgression();
}

void GameWorld::update(float dt, Input& input) {
    const Vector2 camera = cameraTopLeft();
    aimPosition_ = Vector2(
        camera.x + static_cast<float>(input.mousePosition().x),
        camera.y + static_cast<float>(input.mousePosition().y)
    );

    if (input.escapePressed()) {
        handleEscape();
        input.update();
        return;
    }

    if (state_ == GameState::Paused) {
        updatePaused(input);
        input.update();
        return;
    }

    const bool saveLoadContext = (state_ == GameState::Playing
        || state_ == GameState::MapComplete)
        && !passiveTreeOpen_
        && !skillPanelOpen_
        && !craftingState_.open;
    if (saveLoadContext && input.loadRun()) {
        loadRun(Config::SaveFileName);
        input.update();
        return;
    }
    if (saveLoadContext && input.saveRun()) {
        const bool saved = saveRun(Config::SaveFileName);
        eventStatusMessage_ = saved ? "Run saved" : "Run save failed";
        eventStatusTimer_ = 2.0f;
    }

    if (state_ != GameState::Paused) {
        inventoryFullTimer_ = std::max(0.0f, inventoryFullTimer_ - dt);
        if (eventStatusTimer_ > 0.0f) {
            eventStatusTimer_ = std::max(0.0f, eventStatusTimer_ - dt);
            if (eventStatusTimer_ == 0.0f) {
                eventStatusMessage_.clear();
            }
        }
    }

    switch (state_) {
        case GameState::Playing:
            updatePlaying(dt, input);
            break;

        case GameState::GameOver:
            if (input.restart()) {
                reset();
            }
            break;

        case GameState::MapComplete: {
            const bool craftingContext = craftingState_.open || input.craftingToggle();
            tryToggleCraftingPanel(input);
            if (craftingContext) {
                tryCraftSelectedItem(input);
                removeDeadObjects();
                if (input.restart()) {
                    reset();
                }
                break;
            }

            // F still loots Boss drops. Number keys still only drive reward / next-map
            // choice (handled below). Tab/Del let the player free bag space so F can
            // pick up more drops. tryEquipInventoryItem is intentionally NOT called so
            // 1-9 stays mapped to reward/map choices and never equips during settlement.
            tryPickupDroppedItem(input);
            trySelectInventoryItem(input);
            tryMoveSelectedInventoryToStash(input);
            tryMoveSelectedStashToInventory(input);
            tryDropSelectedInventoryItem(input);
            trySalvageSelectedInventoryItem(input);
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

        case GameState::Paused:
            // Paused is handled before the simulation switch. Keep this case
            // for exhaustiveness if a caller changes state during an update.
            break;
    }

    input.update();
}

void GameWorld::handleEscape() {
    if (state_ == GameState::GameOver) {
        return;
    }

    if (state_ == GameState::Paused) {
        state_ = resumeState_;
        return;
    }

    if (craftingState_.open) {
        closeCraftingPanel();
        return;
    }

    if (passiveTreeOpen_) {
        passiveTreeOpen_ = false;
        hoveredPassiveNode_ = -1;
        return;
    }

    if (skillPanelOpen_) {
        skillPanelOpen_ = false;
        return;
    }

    resumeState_ = state_;
    state_ = GameState::Paused;
}

void GameWorld::updatePaused(Input& input) {
    if (input.saveRun()) {
        const bool saved = saveRun(Config::SaveFileName);
        eventStatusMessage_ = saved ? "Run saved" : "Run save failed";
        eventStatusTimer_ = 2.0f;
        return;
    }

    if (input.loadRun()) {
        loadRun(Config::SaveFileName);
        return;
    }

    if (input.restart()) {
        reset();
        return;
    }

    if (input.quit()) {
        quitRequested_ = true;
    }
}

bool GameWorld::saveRun(const std::filesystem::path& path) const {
    std::string error;
    return SaveService::save(path, captureSaveData(), &error);
}

bool GameWorld::loadRun(const std::filesystem::path& path) {
    SaveData data;
    std::string error;
    if (!SaveService::load(path, data, &error) || !restoreFromSaveData(data)) {
        eventStatusMessage_ = "Run load failed";
        eventStatusTimer_ = 2.0f;
        return false;
    }

    eventStatusMessage_ = "Run loaded";
    eventStatusTimer_ = 2.0f;
    return true;
}

SaveData GameWorld::captureSaveData() const {
    SaveData data;
    const GameState savedState = state_ == GameState::Paused ? resumeState_ : state_;
    data.state = savedState == GameState::MapComplete
        ? SavedRunState::MapComplete
        : SavedRunState::Playing;
    data.runSeed = runSeed_;
    data.randomEngineState = random_.engineState();
    data.mapLevel = mapLevel_;
    data.mapTemplateIndex = map_.templateIndex();
    data.mapLayoutIndex = map_.layoutIndex();
    data.currentMapOption = currentMapOption_;
    data.nextMapOptions = nextMapOptions_;
    data.mapRewardOptions = mapRewardOptions_;
    data.selectedNextMapOption = selectedNextMapOption_;
    data.selectedMapRewardOption = selectedMapRewardOption_;
    data.nextMapOptionChosen = nextMapOptionChosen_;
    data.mapRewardChosen = mapRewardChosen_;
    data.score = score_;
    data.survivalTime = survivalTime_;
    data.mapKills = mapKills_;
    data.mapExperienceGained = mapExperienceGained_;
    data.mapItemsDropped = mapItemsDropped_;
    data.mapBossItemsDropped = mapBossItemsDropped_;
    data.mapItemsPickedUp = mapItemsPickedUp_;
    data.lifeFlaskCharges = lifeFlaskCharges_;
    data.unlockedSkills = progression_.unlockedSkills;
    data.unlockedSupports = progression_.unlockedSupports;
    data.skillLevels = progression_.skillLevels;
    data.supportLevels = progression_.supportLevels;
    data.itemQuantityRewardMultiplier = progression_.itemQuantityRewardMultiplier;
    data.forgeFragments = progression_.forgeFragments;
    data.player = player_.saveState();
    if (data.player.hp <= 0) {
        data.player.hp = 1;
    }
    data.skillBar = skillBar_.saveState();
    data.inventory = inventory_.items();
    data.stash = stash_.items();

    data.droppedItems.reserve(droppedItems_.size());
    for (const auto& dropped : droppedItems_) {
        if (!dropped.isCollected()) {
            data.droppedItems.push_back({dropped.position(), dropped.item()});
        }
    }
    data.mapEvents.reserve(map_.events().size());
    for (const auto& event : map_.events()) {
        data.mapEvents.push_back({event.type, event.triggered, event.completed});
    }
    data.exploredCells = map_.exploration().revealedCells();
    return data;
}

bool GameWorld::restoreFromSaveData(const SaveData& data) {
    const auto validTemplateIndex = [](int index) {
        return index >= 0 && index < MapLayoutLibrary::TemplateCount;
    };
    const auto validLayoutIndex = [](int index) {
        return index >= 0 && index < MapLayoutLibrary::VariantCount;
    };
    const auto validFloat = [](float value) {
        return std::isfinite(value);
    };
    const auto validChoice = [](int choice) {
        return choice == -1 || (choice >= 0 && choice < 3);
    };

    const auto validLevelMap = [](const auto& levels, const auto& unlocked) {
        for (const auto& [name, level] : levels) {
            if (unlocked.find(name) == unlocked.end()
                || level < 1 || level > Config::SkillGemMaxLevel) {
                return false;
            }
        }
        return true;
    };

    if (data.mapLevel < 1
        || !validTemplateIndex(data.mapTemplateIndex)
        || !validLayoutIndex(data.mapLayoutIndex)
        || !validTemplateIndex(data.currentMapOption.templateIndex)
        || data.currentMapOption.templateIndex != data.mapTemplateIndex
        || data.inventory.size() > static_cast<std::size_t>(Config::InventoryCapacity)
        || data.stash.size() > static_cast<std::size_t>(Config::StashCapacity)
        || data.droppedItems.size() > 4096
        || (data.mapEvents.size() != 3 && data.mapEvents.size() != 4)
        || !validFloat(data.survivalTime)
        || data.survivalTime < 0.0f
        || !validFloat(data.itemQuantityRewardMultiplier)
        || data.itemQuantityRewardMultiplier <= 0.0f
        || !validLevelMap(data.skillLevels, data.unlockedSkills)
        || !validLevelMap(data.supportLevels, data.unlockedSupports)
        || data.lifeFlaskCharges < 0
        || data.lifeFlaskCharges > Config::LifeFlaskMaxCharges) {
        return false;
    }

    if (!validChoice(data.selectedMapRewardOption)
        || !validChoice(data.selectedNextMapOption)
        || (data.mapRewardChosen != (data.selectedMapRewardOption >= 0))
        || (data.nextMapOptionChosen != (data.selectedNextMapOption >= 0))
        || (data.nextMapOptionChosen && !data.mapRewardChosen)
        || (data.state == SavedRunState::Playing
            && (data.mapRewardChosen || data.nextMapOptionChosen))) {
        return false;
    }

    for (const auto& option : data.nextMapOptions) {
        if (!validTemplateIndex(option.templateIndex)
            || !validModifierForRestore(option.modifier)) {
            return false;
        }
    }
    if (!validModifierForRestore(data.currentMapOption.modifier)) {
        return false;
    }
    for (const auto& reward : data.mapRewardOptions) {
        if (!positiveFinite(reward.itemQuantityMultiplierBonus)) {
            return false;
        }
    }
    for (const auto& item : data.inventory) {
        if (!validItemForRestore(item)) {
            return false;
        }
    }
    for (const auto& item : data.stash) {
        if (!validItemForRestore(item)) {
            return false;
        }
    }
    for (const auto& item : data.droppedItems) {
        if (!validItemForRestore(item.item)) {
            return false;
        }
    }
    for (const auto& item : data.player.equipment) {
        if (item && !validItemForRestore(*item)) {
            return false;
        }
    }

    RandomService restoredRandom(data.runSeed);
    if (!restoredRandom.restoreEngineState(data.randomEngineState)) {
        return false;
    }

    PlayerSaveState playerState = data.player;
    playerState.hp = std::max(1, playerState.hp);
    Player restoredPlayer;
    MapInstance restoredMap(
        data.mapLevel,
        data.mapTemplateIndex,
        data.mapLayoutIndex
    );
    if (!restoredPlayer.restoreState(playerState, restoredMap.size())
        || !restoredMap.restoreExploration(data.exploredCells)) {
        return false;
    }

    for (std::size_t index = 0; index < data.mapEvents.size(); ++index) {
        auto& event = restoredMap.eventsForMutation()[index];
        const auto& saved = data.mapEvents[index];
        if (event.type != saved.type) {
            return false;
        }
        event.triggered = saved.triggered;
        event.completed = saved.completed;
        if (event.completed) {
            event.triggered = true;
        } else if (data.state == SavedRunState::Playing) {
            // Do not restore an in-progress ElitePack without its enemies. The
            // encounter is reset and can be triggered again from the safe map.
            event.triggered = false;
        }
    }

    if (data.mapEvents.size() == 3 && data.state == SavedRunState::MapComplete) {
        // Older settlement saves predate the fourth event. Treat the newly
        // introduced encounter as already settled instead of showing a
        // misleading incomplete event in an already-completed map summary.
        auto& migratedEncounter = restoredMap.eventsForMutation().back();
        migratedEncounter.triggered = true;
        migratedEncounter.completed = true;
    }

    if (data.state == SavedRunState::MapComplete) {
        restoredMap.markBossDefeated();
    }
    restoredPlayer.setPosition(restoredMap.playerStart());

    SkillBar restoredSkillBar;
    if (!restoredSkillBar.restoreState(data.skillBar)) {
        return false;
    }
    for (const auto& skillName : data.skillBar.skills) {
        if (data.unlockedSkills.find(skillName) == data.unlockedSkills.end()) {
            return false;
        }
    }
    for (const auto& supportNames : data.skillBar.supports) {
        for (const auto& supportName : supportNames) {
            if (!supportName.empty()
                && data.unlockedSupports.find(supportName) == data.unlockedSupports.end()) {
                return false;
            }
        }
    }

    Inventory restoredInventory;
    for (const auto& item : data.inventory) {
        if (!restoredInventory.add(item)) {
            return false;
        }
    }
    Stash restoredStash;
    for (const auto& item : data.stash) {
        if (!restoredStash.add(item)) {
            return false;
        }
    }
    std::vector<DroppedItem> restoredDroppedItems;
    restoredDroppedItems.reserve(data.droppedItems.size());
    for (const auto& dropped : data.droppedItems) {
        if (!validFloat(dropped.position.x) || !validFloat(dropped.position.y)
            || dropped.position.x < 0.0f || dropped.position.x > restoredMap.size().x
            || dropped.position.y < 0.0f || dropped.position.y > restoredMap.size().y) {
            return false;
        }
        restoredDroppedItems.emplace_back(dropped.position, dropped.item);
    }

    player_ = std::move(restoredPlayer);
    map_ = std::move(restoredMap);
    skillBar_ = std::move(restoredSkillBar);
    inventory_ = std::move(restoredInventory);
    stash_ = std::move(restoredStash);
    progression_.unlockedSkills = data.unlockedSkills;
    progression_.unlockedSupports = data.unlockedSupports;
    progression_.skillLevels = data.skillLevels;
    progression_.supportLevels = data.supportLevels;
    for (const auto& skill : progression_.unlockedSkills) {
        progression_.skillLevels.try_emplace(skill, 1);
    }
    for (const auto& support : progression_.unlockedSupports) {
        progression_.supportLevels.try_emplace(support, 1);
    }
    progression_.itemQuantityRewardMultiplier = data.itemQuantityRewardMultiplier;
    progression_.forgeFragments = data.forgeFragments;
    runSeed_ = data.runSeed;
    random_ = std::move(restoredRandom);
    mapLevel_ = data.mapLevel;
    currentMapOption_ = data.currentMapOption;
    nextMapOptions_ = data.nextMapOptions;
    mapRewardOptions_ = data.mapRewardOptions;
    selectedNextMapOption_ = data.selectedNextMapOption;
    selectedMapRewardOption_ = data.selectedMapRewardOption;
    nextMapOptionChosen_ = data.nextMapOptionChosen;
    mapRewardChosen_ = data.mapRewardChosen;
    mapModifier_ = currentMapOption_.modifier;
    mapModifier_.itemQuantityMultiplier *= progression_.itemQuantityRewardMultiplier;
    bossDefinition_ = &BossLibrary::forMapLevel(mapLevel_);
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    player_.clearAilments();
    applySkillProgression();

    score_ = data.score;
    survivalTime_ = data.survivalTime;
    mapKills_ = data.mapKills;
    mapExperienceGained_ = data.mapExperienceGained;
    mapItemsDropped_ = data.mapItemsDropped;
    mapBossItemsDropped_ = data.mapBossItemsDropped;
    mapItemsPickedUp_ = data.mapItemsPickedUp;
    lifeFlaskCharges_ = data.lifeFlaskCharges;
    state_ = data.state == SavedRunState::MapComplete
        ? GameState::MapComplete
        : GameState::Playing;
    resumeState_ = state_;
    quitRequested_ = false;

    projectiles_.clear();
    combatFeedback_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    groundHazards_.clear();
    droppedItems_ = std::move(restoredDroppedItems);
    spawner_.reset();
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    resetBossDash();
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    volatileExplosionTimer_ = 0.0f;
    volatileExplosionRadius_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    bossSkillTimer_ = bossDefinition_->skillInterval;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    playerHitCooldown_ = 0.0f;
    playerHitEffectTimer_ = 0.0f;
    playerHitDamage_ = 0;
    playerHitSource_.clear();
    skillFailureFeedbackTimer_ = 0.0f;
    lastSkillFailureFeedback_.clear();
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectTimer_ = 0.0f;
    dashImpactPosition_ = player_.position();
    dashImpactTimer_ = 0.0f;
    dashImpactDuration_ = 0.0f;
    dashImpactRadius_ = 0.0f;
    shrineBuffTimer_ = 0.0f;
    passiveTreeOpen_ = false;
    skillPanelOpen_ = false;
    selectedSupportLink_ = 0;
    craftingState_ = CraftingState();
    hoveredPassiveNode_ = -1;
    nearbyEventPrompt_.clear();
    inventoryFullTimer_ = 0.0f;
    selectedInventoryIndex_ = -1;
    selectedStashIndex_ = -1;
    stashSelectionActive_ = false;
    mapEventInteractionConsumed_ = false;
    activeMapEventIndex_ = -1;
    mapEventEnemiesRemaining_ = 0;
    eventStatusMessage_.clear();
    eventStatusTimer_ = 0.0f;
    updateSelectedInventoryIndex();
    return true;
}

void GameWorld::updatePlaying(float dt, Input& input) {
    const bool craftingContext = craftingState_.open || input.craftingToggle();
    tryToggleCraftingPanel(input);

    if (!craftingContext && input.passiveTreeToggle()) {
        passiveTreeOpen_ = !passiveTreeOpen_;
        if (passiveTreeOpen_) {
            skillPanelOpen_ = false;
        }
    }

    if (!craftingContext && input.skillPanelToggle()) {
        skillPanelOpen_ = !skillPanelOpen_;
        if (skillPanelOpen_) {
            passiveTreeOpen_ = false;
            hoveredPassiveNode_ = -1;
        }
        selectedSupportLink_ = 0;
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
    const AilmentTickResult playerAilmentTick = player_.updateAilments(dt);
    const int playerIgniteDamage = playerAilmentTick.damageFor(AilmentType::Ignite);
    if (playerIgniteDamage > 0) {
        addCombatFeedback(
            player_.position(), playerIgniteDamage, "Ignite",
            CombatFeedbackType::PlayerHit
        );
    }
    const int playerPoisonDamage = playerAilmentTick.damageFor(AilmentType::Poison);
    if (playerPoisonDamage > 0) {
        addCombatFeedback(
            player_.position(), playerPoisonDamage, "Poison",
            CombatFeedbackType::PlayerHit
        );
    }
    skillBar_.update(dt);
    updateCombatFeedback(dt);
    novaEffectTimer_ = std::max(0.0f, novaEffectTimer_ - dt);
    secondarySkillEffectTimer_ = std::max(0.0f, secondarySkillEffectTimer_ - dt);
    dashImpactTimer_ = std::max(0.0f, dashImpactTimer_ - dt);
    bossAoeEffectTimer_ = std::max(0.0f, bossAoeEffectTimer_ - dt);
    bossDashEffectTimer_ = std::max(0.0f, bossDashEffectTimer_ - dt);
    volatileExplosionTimer_ = std::max(0.0f, volatileExplosionTimer_ - dt);
    playerHitCooldown_ = std::max(0.0f, playerHitCooldown_ - dt);
    playerHitEffectTimer_ = std::max(0.0f, playerHitEffectTimer_ - dt);
    skillFailureFeedbackTimer_ = std::max(0.0f, skillFailureFeedbackTimer_ - dt);
    shrineBuffTimer_ = std::max(0.0f, shrineBuffTimer_ - dt);
    updateGroundHazards(dt);
    if (lifeFlaskStatusTimer_ > 0.0f) {
        lifeFlaskStatusTimer_ = std::max(0.0f, lifeFlaskStatusTimer_ - dt);
        if (lifeFlaskStatusTimer_ == 0.0f) {
            lifeFlaskStatusMessage_.clear();
        }
    }
    nearbyEventPrompt_.clear();
    mapEventInteractionConsumed_ = false;

    if (craftingState_.open || craftingContext) {
        if (craftingState_.open) {
            tryCraftSelectedItem(input);
        }
    } else if (passiveTreeOpen_) {
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

    if (!craftingState_.open && !craftingContext) {
        updateMapEvents(dt, input);
        if (!mapEventInteractionConsumed_) {
            tryPickupDroppedItem(input);
        }
    }

    if (!passiveTreeOpen_ && !skillPanelOpen_ && !craftingState_.open && !craftingContext) {
        tryUseLifeFlask(input);
        trySelectInventoryItem(input);
        tryDropSelectedInventoryItem(input);
        trySalvageSelectedInventoryItem(input);
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
    map_.revealAround(player_.position());
}

void GameWorld::reset() {
    reset(RandomService::deriveSeed(runSeed_, 1));
}

void GameWorld::reset(std::uint64_t runSeed) {
    runSeed_ = runSeed;
    random_.reseed(runSeed_);
    player_ = Player();
    map_ = MapInstance(1, 0);
    bossDefinition_ = &BossLibrary::forMapLevel(1);
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    projectiles_.clear();
    combatFeedback_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    groundHazards_.clear();
    droppedItems_.clear();
    inventory_.clear();
    stash_.clear();
    spawner_.reset();
    skillBar_.reset();
    initializeRunProgression();
    applySkillProgression();
    state_ = GameState::Playing;
    resumeState_ = GameState::Playing;
    quitRequested_ = false;
    score_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectPosition_ = Vector2(Config::WindowWidth / 2.0f, Config::WindowHeight / 2.0f);
    secondarySkillEffectTimer_ = 0.0f;
    dashImpactPosition_ = player_.position();
    dashImpactTimer_ = 0.0f;
    dashImpactDuration_ = 0.0f;
    dashImpactRadius_ = 0.0f;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    volatileExplosionTimer_ = 0.0f;
    volatileExplosionRadius_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    resetBossDash();
    bossSkillTimer_ = bossDefinition_->skillInterval;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    playerHitCooldown_ = 0.0f;
    playerHitEffectTimer_ = 0.0f;
    playerHitDamage_ = 0;
    playerHitSource_.clear();
    skillFailureFeedbackTimer_ = 0.0f;
    lastSkillFailureFeedback_.clear();
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
    selectedSupportLink_ = 0;
    craftingState_ = CraftingState();
    hoveredPassiveNode_ = -1;
    nearbyEventPrompt_.clear();
    shrineBuffTimer_ = 0.0f;
    lifeFlaskCharges_ = Config::LifeFlaskMaxCharges;
    lifeFlaskStatusMessage_.clear();
    lifeFlaskStatusTimer_ = 0.0f;
    inventoryFullTimer_ = 0.0f;
    selectedInventoryIndex_ = -1;
    selectedStashIndex_ = -1;
    stashSelectionActive_ = false;
    mapEventInteractionConsumed_ = false;
    activeMapEventIndex_ = -1;
    mapEventEnemiesRemaining_ = 0;
    eventStatusMessage_.clear();
    eventStatusTimer_ = 0.0f;
}

void GameWorld::startNextMap() {
    if (selectedNextMapOption_ >= 0 && selectedNextMapOption_ < static_cast<int>(nextMapOptions_.size())) {
        currentMapOption_ = nextMapOptions_[static_cast<std::size_t>(selectedNextMapOption_)];
    }

    ++mapLevel_;
    map_ = MapInstance(
        mapLevel_,
        currentMapOption_.templateIndex,
        MapLayoutLibrary::variantForMapLevel(mapLevel_)
    );
    bossDefinition_ = &BossLibrary::forMapLevel(mapLevel_);
    player_.setBounds(map_.size());
    player_.setPosition(map_.playerStart());
    player_.clearAilments();
    currentWave_ = 0;
    enemiesSpawnedInWave_ = 0;
    survivalTime_ = 0.0f;
    novaEffectTimer_ = 0.0f;
    secondarySkillEffectTimer_ = 0.0f;
    dashImpactPosition_ = player_.position();
    dashImpactTimer_ = 0.0f;
    dashImpactDuration_ = 0.0f;
    dashImpactRadius_ = 0.0f;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    volatileExplosionTimer_ = 0.0f;
    volatileExplosionRadius_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    resetBossDash();
    bossSkillTimer_ = bossDefinition_->skillInterval;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    playerHitCooldown_ = 0.0f;
    playerHitEffectTimer_ = 0.0f;
    playerHitDamage_ = 0;
    playerHitSource_.clear();
    skillFailureFeedbackTimer_ = 0.0f;
    lastSkillFailureFeedback_.clear();

    projectiles_.clear();
    combatFeedback_.clear();
    bossProjectiles_.clear();
    enemyProjectiles_.clear();
    enemies_.clear();
    groundHazards_.clear();
    droppedItems_.clear();
    spawner_.reset();
    applySkillProgression();
    state_ = GameState::Playing;
    resumeState_ = GameState::Playing;
    quitRequested_ = false;
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
    selectedSupportLink_ = 0;
    craftingState_ = CraftingState();
    hoveredPassiveNode_ = -1;
    nearbyEventPrompt_.clear();
    shrineBuffTimer_ = 0.0f;
    lifeFlaskCharges_ = Config::LifeFlaskMaxCharges;
    lifeFlaskStatusMessage_.clear();
    lifeFlaskStatusTimer_ = 0.0f;
    inventoryFullTimer_ = 0.0f;
    selectedInventoryIndex_ = -1;
    selectedStashIndex_ = -1;
    stashSelectionActive_ = false;
    mapEventInteractionConsumed_ = false;
    activeMapEventIndex_ = -1;
    mapEventEnemiesRemaining_ = 0;
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
        const AilmentTickResult ailmentTick = enemy.updateAilments(dt);
        const int igniteDamage = ailmentTick.damageFor(AilmentType::Ignite);
        if (igniteDamage > 0) {
            addCombatFeedback(enemy.position(), igniteDamage, "Ignite");
        }
        const int poisonDamage = ailmentTick.damageFor(AilmentType::Poison);
        if (poisonDamage > 0) {
            addCombatFeedback(enemy.position(), poisonDamage, "Poison");
        }
        if (enemy.isDead()) {
            rewardEnemyKill(enemy);
            continue;
        }
        if (!enemy.isBoss() || !bossDashState_.isActive()) {
            enemy.update(dt, player_.position(), map_, mapModifier_.monsterSpeedMultiplier);
        }
    }
}

void GameWorld::updateGroundHazards(float dt) {
    for (auto& hazard : groundHazards_) {
        const int elapsedTicks = hazard.update(dt);
        if (elapsedTicks <= 0 || !Collision::circleCircle(
                player_.position(), player_.radius(),
                hazard.position(), hazard.definition().radius
            )) {
            continue;
        }

        for (int tick = 0; tick < elapsedTicks; ++tick) {
            damagePlayer(
                hazard.definition().damage,
                hazard.definition().source,
                hazard.definition().damageType,
                hazard.definition().ailment
            );
        }
    }

    groundHazards_.erase(std::remove_if(
        groundHazards_.begin(), groundHazards_.end(),
        [](const GroundHazard& hazard) { return !hazard.isActive(); }
    ), groundHazards_.end());
}

void GameWorld::updateBossSkills(float dt) {
    Enemy* boss = activeBoss();
    if (!boss) {
        bossAoeTelegraphTimer_ = 0.0f;
        bossDashState_.reset();
        bossSkillTimer_ = bossDefinition_->skillInterval;
        return;
    }

    const float hpRatio = boss->maxHp() > 0
        ? static_cast<float>(std::max(0, boss->hp())) / static_cast<float>(boss->maxHp())
        : 0.0f;
    if (!bossEnraged_ && hpRatio <= bossDefinition_->enrageHealthRatio) {
        bossEnraged_ = true;
        triggerBossEnrage(*boss);
        // Enrage may append reinforcements to enemies_, invalidating the
        // pointer returned by activeBoss(). Reacquire it before continuing
        // the boss pattern update.
        boss = activeBoss();
        if (!boss) {
            return;
        }
        bossSkillTimer_ = std::min(bossSkillTimer_, bossSkillInterval());
    }

    if (bossDashState_.isActive()) {
        updateBossDash(dt, *boss);
        return;
    }

    const float telegraphBefore = bossAoeTelegraphTimer_;
    bossAoeTelegraphTimer_ = std::max(0.0f, bossAoeTelegraphTimer_ - dt);
    if (telegraphBefore > 0.0f && bossAoeTelegraphTimer_ <= 0.0f) {
        switch (bossAoeSkill_.type) {
            case BossSkillType::CircularAoe:
                if (Collision::circleCircle(
                        player_.position(), player_.radius(),
                        bossAoeCenter_, bossAoeSkill_.radius
                    )) {
                    damagePlayer(
                        bossAoeSkill_.damage,
                        bossAoeSkill_.name,
                        bossAoeSkill_.damageType,
                        bossAoeSkill_.ailment
                    );
                }
                if (bossAoeSkill_.groundHazard.isValid()) {
                    groundHazards_.emplace_back(
                        bossAoeCenter_, bossAoeSkill_.groundHazard
                    );
                }
                break;
            case BossSkillType::SummonAdds:
                summonBossAdds(*boss, bossAoeSkill_);
                break;
            case BossSkillType::Projectile:
            case BossSkillType::Dash:
                break;
        }
        bossAoeEffectTimer_ = bossAoeSkill_.effectDuration;
        return;
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

    switch (skill.type) {
        case BossSkillType::CircularAoe:
            bossAoeCenter_ = player_.position();
            bossAoeSkill_ = skill;
            bossAoeSkill_.damage = bossSkillDamage(skill.damage);
            if (bossAoeSkill_.groundHazard.isValid()) {
                bossAoeSkill_.groundHazard.damage = bossSkillDamage(
                    skill.groundHazard.damage
                );
            }
            bossAoeTelegraphTimer_ = skill.telegraphDuration;
            addCombatFeedback(
                boss->position(),
                0,
                "Boss casting: " + skill.name,
                CombatFeedbackType::Telegraph
            );
            break;
        case BossSkillType::SummonAdds:
            bossAoeCenter_ = boss->position();
            bossAoeSkill_ = skill;
            bossAoeTelegraphTimer_ = skill.telegraphDuration;
            addCombatFeedback(
                boss->position(),
                0,
                "Boss casting: " + skill.name,
                CombatFeedbackType::Telegraph
            );
            break;
        case BossSkillType::Dash: {
            const Vector2 direction = (player_.position() - boss->position()).normalized();
            if (direction.lengthSquared() <= 0.0f || !skill.dash.isValid()) {
                break;
            }

            const Vector2 target = map_.resolveMovement(
                boss->position(), boss->radius(), direction * skill.dash.distance
            );
            bossDashSkill_ = skill;
            bossDashSkill_.damage = bossSkillDamage(skill.damage);
            bossDashState_.begin(
                boss->position(), target, skill.telegraphDuration, skill.dash.speed
            );
            addCombatFeedback(
                boss->position(),
                0,
                "Boss casting: " + skill.name,
                CombatFeedbackType::Telegraph
            );
            break;
        }
        case BossSkillType::Projectile: {
            const Vector2 direction = (player_.position() - boss->position()).normalized();
            if (direction.lengthSquared() <= 0.0f) {
                break;
            }

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
                    skill.name,
                    skill.damageType,
                    skill.ailment,
                    true
                });
            }
            break;
        }
    }

    ++bossSkillIndex_;
    bossSkillTimer_ = bossSkillInterval();
}

void GameWorld::triggerBossEnrage(Enemy& boss) {
    const Vector2 bossPosition = boss.position();
    int summonedCount = 0;
    if (bossDefinition_->enrageSummonCount > 0) {
        BossSkillDefinition enrageSummon;
        enrageSummon.type = BossSkillType::SummonAdds;
        enrageSummon.name = "Enrage reinforcements";
        enrageSummon.radius = 120.0f;
        enrageSummon.summonType = bossDefinition_->enrageSummonType;
        enrageSummon.summonCount = bossDefinition_->enrageSummonCount;
        summonedCount = summonBossAdds(boss, enrageSummon);
    }

    if (bossDefinition_->enrageHazard.isValid()) {
        GroundHazardDefinition hazard = bossDefinition_->enrageHazard;
        hazard.damage = bossSkillDamage(hazard.damage);
        groundHazards_.emplace_back(bossPosition, std::move(hazard));
    }

    eventStatusMessage_ = "Boss enraged: " + bossDefinition_->name;
    if (!bossDefinition_->enrageTransitionDescription.empty()) {
        eventStatusMessage_ += " - " + bossDefinition_->enrageTransitionDescription;
    }
    if (summonedCount > 0) {
        eventStatusMessage_ += " (" + std::to_string(summonedCount) + " adds)";
    }
    eventStatusTimer_ = 3.0f;
}

void GameWorld::updateBossDash(float dt, Enemy& boss) {
    const bool wasMoving = bossDashState_.isMoving();
    const float movementTime = bossDashState_.isMoving()
        ? dt * boss.movementSpeedMultiplier()
        : dt;
    const Vector2 movement = bossDashState_.update(movementTime, boss.position());
    if (movement.lengthSquared() > 0.0f) {
        boss.moveBy(movement, map_);
    }

    if (wasMoving && Collision::circleCircle(
            player_.position(), player_.radius(),
            boss.position(), bossDashSkill_.radius
        ) && bossDashState_.consumeHit()) {
        damagePlayer(
            bossDashSkill_.damage,
            bossDashSkill_.name,
            bossDashSkill_.damageType,
            bossDashSkill_.ailment
        );
    }

    if (bossDashState_.consumeCompletion()) {
        bossDashEffectPosition_ = boss.position();
        bossDashEffectTimer_ = bossDashSkill_.effectDuration;
        eventStatusMessage_ = bossDashSkill_.name + " impact";
        eventStatusTimer_ = 1.0f;
    }
}

int GameWorld::summonBossAdds(const Enemy& boss, const BossSkillDefinition& skill) {
    const int activeAdds = static_cast<int>(std::count_if(
        enemies_.begin(), enemies_.end(),
        [](const Enemy& enemy) { return !enemy.isBoss() && !enemy.isDead(); }
    ));
    const int summonCount = availableBossSummonCount(
        skill.summonCount, activeAdds, Config::MaxBossSummonedEnemies
    );
    if (summonCount <= 0) {
        eventStatusMessage_ = "Summon limit reached";
        eventStatusTimer_ = 2.0f;
        return 0;
    }

    const auto& definition = EnemyLibrary::forType(skill.summonType);
    const int hp = std::max(1, static_cast<int>(std::ceil(
        enemyHpForMap() * definition.hpMultiplier
    )));
    const int damage = enemyDamageForMap() + definition.damageBonus;
    const float enemyRadius = Config::EnemyRadius * definition.radiusMultiplier;
    const Vector2 bossPosition = boss.position();
    constexpr float twoPi = 6.28318531f;

    for (int i = 0; i < summonCount; ++i) {
        const float angle = twoPi * static_cast<float>(i) / static_cast<float>(summonCount);
        const Vector2 offset(std::cos(angle) * skill.radius, std::sin(angle) * skill.radius);
        const Vector2 position = map_.resolveMovement(bossPosition, enemyRadius, offset);
        enemies_.emplace_back(position, hp, damage, skill.summonType);
    }

    eventStatusMessage_ = "Boss summoned: " + std::to_string(summonCount)
        + " " + definition.name;
    eventStatusTimer_ = 2.0f;
    return summonCount;
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
    const EnemyType type = nextMapEnemyType();
    const auto& definition = EnemyLibrary::forType(type);
    const EliteModifier modifier = type == EnemyType::Elite ? randomEliteModifier() : EliteModifier::None;
    const auto& modifierDefinition = EliteModifierLibrary::forModifier(modifier);
    const int hp = std::max(1, static_cast<int>(std::ceil(
        enemyHpForMap() * definition.hpMultiplier * modifierDefinition.hpMultiplier
    )));
    const int damage = enemyDamageForMap() + definition.damageBonus + modifierDefinition.damageBonus;

    if (auto enemy = spawner_.trySpawnNear(
            player_.position(), map_.size(), map_, hp, damage, type, modifier, random_
        )) {
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

                const int mitigatedDamage = damageToEnemy(
                    enemy, projectile.damage(), projectile.damageType()
                );
                const int dealtDamage = enemy.takeDamage(mitigatedDamage);
                if (dealtDamage > 0) {
                    addCombatFeedback(
                        enemy.position(),
                        dealtDamage,
                        projectile.source().empty()
                            ? skillBar_.definition(SkillSlot::Primary).name
                            : projectile.source()
                    );
                }
                if (projectile.ailment().type != AilmentType::None && dealtDamage > 0) {
                    applySkillAilment(enemy, projectile.ailment(), dealtDamage);
                }
                if (projectile.damageType() == DamageType::Lightning && dealtDamage > 0) {
                    triggerStormChain(enemy, dealtDamage, projectile.ailment());
                }
                projectile.recordEnemyHit(enemy.id());

                if (enemy.isDead()) {
                    rewardEnemyKill(enemy);
                }
            }
        }
    }

    std::vector<int> summonerIdsToProcess;
    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (enemy.isBoss()) {
            if (Collision::circleCircle(
                    player_.position(), player_.radius(),
                    enemy.position(), enemy.radius()
                )) {
                damagePlayer(enemy.contactDamage(), bossDefinition_->name + " contact");
            }
            continue;
        }

        if (enemy.isCharger()) {
            if (enemy.isCharging() && Collision::circleCircle(
                    player_.position(), player_.radius(),
                    enemy.position(), enemy.radius()
                ) && enemy.consumeChargeHit()) {
                const auto& definition = EnemyLibrary::forType(enemy.type());
                damagePlayer(
                    enemy.contactDamage(),
                    definition.name + " charge",
                    definition.contactDamageType,
                    definition.contactAilment
                );
            }
            continue;
        }

        if (enemy.isSummoner()) {
            if (enemy.consumeAttack()) {
                summonerIdsToProcess.push_back(enemy.id());
            }
            continue;
        }

        if (!enemy.consumeAttack()) {
            continue;
        }

        const Vector2 toPlayer = player_.position() - enemy.position();
        const auto& definition = EnemyLibrary::forType(enemy.type());
        if (enemy.isRanged()) {
            const Vector2 direction = toPlayer.normalized();
            if (direction.lengthSquared() > 0.0f) {
                enemyProjectiles_.push_back({
                    enemy.position(),
                    direction * definition.projectileSpeed,
                    definition.projectileRadius,
                    enemy.contactDamage(),
                    definition.name + " shot",
                    definition.projectileDamageType,
                    definition.projectileAilment,
                    true
                });
            }
        } else if (toPlayer.lengthSquared() <= enemy.attackRange() * enemy.attackRange()) {
            damagePlayer(
                enemy.contactDamage(),
                definition.name + " strike",
                definition.contactDamageType,
                definition.contactAilment
            );
        }
    }

    for (const int summonerId : summonerIdsToProcess) {
        const auto summonerIt = std::find_if(
            enemies_.begin(), enemies_.end(),
            [summonerId](const Enemy& enemy) { return enemy.id() == summonerId; }
        );
        if (summonerIt == enemies_.end()) {
            continue;
        }

        const int summonedCount = summonEnemyAdds(*summonerIt);
        if (summonedCount > 0) {
            eventStatusMessage_ = "Hexbinder summoned "
                + std::to_string(summonedCount) + " minions";
            eventStatusTimer_ = 1.5f;
        }
    }
}

int GameWorld::summonEnemyAdds(Enemy& summoner) {
    if (!summoner.isSummoner() || map_.bossTriggered() || map_.bossDefeated()) {
        return 0;
    }

    const auto& summonerDefinition = EnemyLibrary::forType(summoner.type());
    const int activeMinions = static_cast<int>(std::count_if(
        enemies_.begin(), enemies_.end(),
        [](const Enemy& enemy) { return enemy.isSummoned() && !enemy.isDead(); }
    ));
    const int availableSlots = std::max(0, Config::MaxSummonerMinions - activeMinions);
    const int summonCount = std::min(availableSlots, std::max(0, summonerDefinition.summonCount));
    if (summonCount <= 0) {
        return 0;
    }

    const EnemyType summonType = summonerDefinition.summonType;
    const auto& definition = EnemyLibrary::forType(summonType);
    const auto& modifierDefinition = EliteModifierLibrary::forModifier(EliteModifier::None);
    const Vector2 summonerPosition = summoner.position();
    const int mapEventIndex = summoner.mapEventIndex();
    static const Vector2 offsets[] = {
        {-Config::SummonerMinionSpreadRadius, 0.0f},
        {Config::SummonerMinionSpreadRadius, 0.0f},
        {0.0f, -Config::SummonerMinionSpreadRadius},
        {0.0f, Config::SummonerMinionSpreadRadius},
    };

    int spawnedCount = 0;
    const int offsetCount = static_cast<int>(sizeof(offsets) / sizeof(offsets[0]));
    for (int index = 0; index < summonCount; ++index) {
        const Vector2 desiredPosition = summonerPosition
            + offsets[index % offsetCount];
        const Vector2 position = map_.resolveMovement(
            desiredPosition,
            Config::EnemyRadius * definition.radiusMultiplier,
            Vector2()
        );
        if (map_.intersectsObstacle(position, Config::EnemyRadius * definition.radiusMultiplier)) {
            continue;
        }

        const int hp = std::max(1, static_cast<int>(std::ceil(
            enemyHpForMap() * definition.hpMultiplier * modifierDefinition.hpMultiplier
        )));
        const int damage = enemyDamageForMap()
            + definition.damageBonus + modifierDefinition.damageBonus;
        enemies_.emplace_back(
            position,
            hp,
            damage,
            summonType,
            EliteModifier::None,
            mapEventIndex,
            true
        );
        ++spawnedCount;
    }

    return spawnedCount;
}

int GameWorld::damageToEnemy(
    const Enemy& enemy,
    int rawDamage,
    DamageType damageType
) const {
    if (rawDamage <= 0 || enemy.isDead()) {
        return 0;
    }

    int fireResistance = EnemyLibrary::forType(enemy.type()).fireResistance;
    int coldResistance = EnemyLibrary::forType(enemy.type()).coldResistance;
    int lightningResistance = EnemyLibrary::forType(enemy.type()).lightningResistance;
    int poisonResistance = EnemyLibrary::forType(enemy.type()).poisonResistance;
    if (enemy.isBoss()) {
        fireResistance = bossDefinition_->fireResistance;
        coldResistance = bossDefinition_->coldResistance;
        lightningResistance = bossDefinition_->lightningResistance;
        poisonResistance = bossDefinition_->poisonResistance;
    }
    fireResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Fire, true
    );
    coldResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Cold, true
    );
    lightningResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Lightning, true
    );
    poisonResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Poison, true
    );

    const int resistedDamage = damageAfterResistance(
        rawDamage,
        damageType,
        fireResistance,
        coldResistance,
        lightningResistance,
        poisonResistance
    );
    const int shockedDamage = resistedDamage <= 0
        ? 0
        : static_cast<int>(std::ceil(
            static_cast<float>(resistedDamage) * enemy.damageTakenMultiplier()
        ));
    if (enemy.isWarden()) {
        return shockedDamage;
    }

    const float auraRadiusSquared = Config::WardenAuraRadius * Config::WardenAuraRadius;
    for (const auto& protector : enemies_) {
        if (protector.isDead() || !protector.isWarden()) {
            continue;
        }

        const Vector2 offset = enemy.position() - protector.position();
        if (offset.lengthSquared() <= auraRadiusSquared) {
            return wardenProtectedDamage(
                shockedDamage,
                true,
                Config::WardenDamageTakenMultiplier
            );
        }
    }

    return shockedDamage;
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
            damagePlayer(
                projectile.damage,
                projectile.source,
                projectile.damageType,
                projectile.ailment
            );
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
            damagePlayer(
                projectile.damage,
                projectile.source,
                projectile.damageType,
                projectile.ailment
            );
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
        [this](const Enemy& e) {
            return e.isDead() || (map_.bossDefeated() && !e.isBoss());
        });
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
    if (!input.dash()) {
        return;
    }

    Vector2 direction = (aimPosition_ - player_.position()).normalized();
    if (direction.lengthSquared() == 0.0f) {
        return;
    }
    if (!tryStartPlayerSkill(SkillSlot::Movement)) {
        return;
    }

    const auto* support = skillBar_.support(SkillSlot::Movement);
    movePlayerBy(direction * Config::DashDistance);

    if (support && support->dashBaseDamage > 0 && support->dashRadius > 0.0f) {
        const float shrineMultiplier = shrineBuffTimer_ > 0.0f
            ? Config::ShrineDamageMultiplier : 1.0f;
        dashImpactPosition_ = player_.position();
        dashImpactRadius_ = supportAreaRadius(*support, player_.stats());
        dashImpactDuration_ = support->effectDuration;
        dashImpactTimer_ = support->effectDuration;
        dealAreaDamage(
            dashImpactPosition_,
            dashImpactRadius_,
            supportAreaDamage(*support, player_.stats(), shrineMultiplier),
            nullptr,
            skillBar_.definition(SkillSlot::Movement).name,
            DamageType::Physical
        );
    }
}

void GameWorld::tryCastUtilitySkill(Input& input) {
    if (!input.nova() || !tryStartPlayerSkill(SkillSlot::Utility)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Utility);
    const AilmentDefinition ailment = ailmentForPlayerSkill(skill);
    const int repeatCount = skillRepeatCount(
        skill, skillBar_.supportDefinitionsFor(skill)
    );
    for (int repeat = 0; repeat < repeatCount; ++repeat) {
        dealAreaDamage(
            player_.position(),
            radiusForPlayerSkill(skill),
            damageForPlayerSkill(skill),
            &ailment,
            skill.name,
            skill.damageType
        );
    }
    novaEffectTimer_ = skill.effectDuration;
}

void GameWorld::tryCastSecondarySkill(Input& input) {
    if (!input.secondarySkill() || !tryStartPlayerSkill(SkillSlot::Secondary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Secondary);
    const AilmentDefinition ailment = ailmentForPlayerSkill(skill);
    const int repeatCount = skillRepeatCount(
        skill, skillBar_.supportDefinitionsFor(skill)
    );
    for (int repeat = 0; repeat < repeatCount; ++repeat) {
        dealAreaDamage(
            aimPosition_,
            radiusForPlayerSkill(skill),
            damageForPlayerSkill(skill),
            &ailment,
            skill.name,
            skill.damageType
        );
    }
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

    if (!tryStartPlayerSkill(SkillSlot::Primary)) {
        return;
    }

    const auto& skill = skillBar_.definition(SkillSlot::Primary);
    const int damage = damageForPlayerSkill(skill);
    const AilmentDefinition ailment = ailmentForPlayerSkill(skill);

    const int projectileCount = projectileCountForPlayerSkill(skill);
    const float spreadAngle = spreadAngleForPlayerSkill(skill);
    if (projectileCount <= 1 || spreadAngle <= 0.0f) {
        projectiles_.push_back(Projectile(
            player_.position(), direction * Config::ProjectileSpeed, damage,
            pierceCountForPlayerSkill(skill), ailment, skill.name, skill.damageType
        ));
        return;
    }

    const float degToRad = 3.14159265f / 180.0f;
    const float halfSpread = spreadAngle * 0.5f;
    const float step = spreadAngle / static_cast<float>(projectileCount - 1);
    for (int i = 0; i < projectileCount; ++i) {
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
            pierceCountForPlayerSkill(skill),
            ailment,
            skill.name,
            skill.damageType
        ));
    }
}

bool GameWorld::tryStartPlayerSkill(SkillSlot slot) {
    const auto& skill = skillBar_.definition(slot);
    if (!skillBar_.canCast(slot)) {
        addSkillRejectedFeedback("Skill cooling down: " + skill.name);
        return false;
    }

    if (!player_.canSpendMana(skill.manaCost)) {
        addSkillRejectedFeedback("Not enough Mana: " + skill.name);
        return false;
    }

    // SkillBar has no Player dependency. Keep resource ownership in Player,
    // but consume both gates here so insufficient Mana cannot start cooldown.
    if (!player_.spendMana(skill.manaCost)) {
        addSkillRejectedFeedback("Not enough Mana: " + skill.name);
        return false;
    }

    skillBar_.consumeCooldown(slot);
    return true;
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

    const int healed = player_.heal(lifeFlaskHealAmount(
        Config::LifeFlaskHealAmount,
        player_.stats()
    ));
    const bool cleansed = player_.hasAilment();
    if (healed <= 0 && !cleansed) {
        return;
    }

    --lifeFlaskCharges_;
    if (cleansed) {
        player_.clearAilments();
    }
    lifeFlaskStatusMessage_ = "Life flask: +" + std::to_string(healed) + " HP"
        + (cleansed ? " | ailments cleansed" : "");
    lifeFlaskStatusTimer_ = 1.5f;
}

void GameWorld::restoreLifeFlaskCharges(int charges, const std::string& source) {
    const int previousCharges = lifeFlaskCharges_;
    lifeFlaskCharges_ = refilledFlaskCharges(
        lifeFlaskCharges_, Config::LifeFlaskMaxCharges, charges
    );

    const int restoredCharges = lifeFlaskCharges_ - previousCharges;
    if (restoredCharges <= 0) {
        return;
    }

    lifeFlaskStatusMessage_ = source + ": Flask +" + std::to_string(restoredCharges);
    lifeFlaskStatusTimer_ = 1.5f;
}

void GameWorld::dealAreaDamage(
    const Vector2& center,
    float radius,
    int damage,
    const AilmentDefinition* ailment,
    const std::string& source,
    DamageType damageType
) {
    for (auto& enemy : enemies_) {
        if (enemy.isDead()) {
            continue;
        }

        if (Collision::circleCircle(
                center, radius,
                enemy.position(), enemy.radius()
            )) {
            const int mitigatedDamage = damageToEnemy(enemy, damage, damageType);
            const int dealtDamage = enemy.takeDamage(mitigatedDamage);
            if (dealtDamage > 0) {
                addCombatFeedback(enemy.position(), dealtDamage, source);
            }
            if (ailment && dealtDamage > 0) {
                applySkillAilment(enemy, *ailment, dealtDamage);
            }

            if (enemy.isDead()) {
                rewardEnemyKill(enemy);
            }
        }
    }
}

void GameWorld::addCombatFeedback(
    const Vector2& position,
    int damage,
    const std::string& source,
    CombatFeedbackType type
) {
    if ((damage <= 0 && type == CombatFeedbackType::Damage)
        || Config::MaxCombatFeedback <= 0) {
        return;
    }

    if (combatFeedback_.size() >= static_cast<std::size_t>(Config::MaxCombatFeedback)) {
        combatFeedback_.erase(combatFeedback_.begin());
    }

    combatFeedback_.push_back({
        position,
        damage,
        source.empty() ? "Skill" : source,
        Config::CombatFeedbackDuration,
        type
    });
}

void GameWorld::addSkillRejectedFeedback(const std::string& source) {
    if (source.empty()) {
        return;
    }

    if (skillFailureFeedbackTimer_ > 0.0f && lastSkillFailureFeedback_ == source) {
        return;
    }

    addCombatFeedback(
        player_.position(),
        0,
        source,
        CombatFeedbackType::SkillRejected
    );
    skillFailureFeedbackTimer_ = Config::CombatFeedbackDuration;
    lastSkillFailureFeedback_ = source;
}

void GameWorld::updateCombatFeedback(float dt) {
    const float elapsed = std::max(0.0f, dt);
    for (auto& feedback : combatFeedback_) {
        feedback.timeRemaining = std::max(0.0f, feedback.timeRemaining - elapsed);
    }

    combatFeedback_.erase(
        std::remove_if(
            combatFeedback_.begin(),
            combatFeedback_.end(),
            [](const CombatFeedback& feedback) { return feedback.timeRemaining <= 0.0f; }
        ),
        combatFeedback_.end()
    );
}

void GameWorld::applySkillAilment(
    Enemy& enemy,
    const AilmentDefinition& ailment,
    int hitDamage
) {
    const auto& enemyDefinition = EnemyLibrary::forType(enemy.type());
    int igniteResistance = enemyDefinition.igniteResistance;
    int chillResistance = enemyDefinition.chillResistance;
    int shockResistance = enemyDefinition.shockResistance;
    int poisonResistance = enemyDefinition.poisonResistance;
    if (enemy.isBoss()) {
        igniteResistance = bossDefinition_->igniteResistance;
        chillResistance = bossDefinition_->chillResistance;
        shockResistance = bossDefinition_->shockResistance;
        poisonResistance = bossDefinition_->poisonResistance;
    }

    switch (ailment.type) {
        case AilmentType::Ignite:
            enemy.applyIgnite(
                ailmentTickDamageAfterResistance(
                    ailmentTickDamage(ailment, hitDamage),
                    std::clamp(igniteResistance + mapModifier_.ailmentResistanceBonus, 0, 100),
                    ailment.ignitePenetration
                ),
                ailment.duration
            );
            break;
        case AilmentType::Chill:
            enemy.applyChill(
                chillSpeedMultiplierAfterResistance(
                    ailment.speedMultiplier,
                    std::clamp(chillResistance + mapModifier_.ailmentResistanceBonus, 0, 100),
                    ailment.chillPenetration
                ),
                ailment.duration
            );
            break;
        case AilmentType::Shock:
            enemy.applyShock(
                damageTakenMultiplierAfterResistance(
                    ailment.damageTakenMultiplier,
                    std::clamp(shockResistance + mapModifier_.ailmentResistanceBonus, 0, 100),
                    ailment.shockPenetration
                ),
                ailment.duration
            );
            addCombatFeedback(
                enemy.position(),
                0,
                "Shock",
                CombatFeedbackType::Status
            );
            break;
        case AilmentType::Poison:
            {
                const int tickDamage = ailmentTickDamageAfterResistance(
                    ailmentTickDamage(ailment, hitDamage),
                    std::clamp(poisonResistance + mapModifier_.ailmentResistanceBonus, 0, 100),
                    ailment.poisonPenetration
                );
                if (tickDamage <= 0) {
                    return;
                }
                enemy.applyPoison(
                    tickDamage,
                    ailment.duration,
                    Config::MaxPoisonStacks,
                    ailment.poisonSpreadRadius,
                    ailment.poisonSpreadMultiplier
                );
            }
            addCombatFeedback(
                enemy.position(),
                0,
                "Poison",
                CombatFeedbackType::Status
            );
            break;
        case AilmentType::None:
        case AilmentType::Count:
            break;
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
                if (!event.triggered
                    && activeMapEventIndex_ >= 0
                    && mapEventEnemiesRemaining_ > 0) {
                    nearbyEventPrompt_ = "Another encounter active";
                    return;
                }
                nearbyEventPrompt_ = event.triggered ? "Elite Pack active" : "Elite Pack ambush";
                if (!event.triggered) {
                    triggerElitePackEvent(i);
                }
                return;

            case MapEventType::Combination: {
                const auto& encounter = map_.encounterDefinition();
                switch (encounter.type) {
                    case MapEncounterType::EnhancedCache:
                        nearbyEventPrompt_ = "F Open " + encounter.name;
                        if (input.pickup()) {
                            triggerCombinationEvent(i);
                            mapEventInteractionConsumed_ = true;
                        }
                        return;

                    case MapEncounterType::CursedReliquary:
                        if (!event.triggered
                            && activeMapEventIndex_ >= 0
                            && mapEventEnemiesRemaining_ > 0) {
                            nearbyEventPrompt_ = "Another encounter active";
                            return;
                        }
                        nearbyEventPrompt_ = event.triggered
                            ? encounter.name + " active"
                            : "F Unseal " + encounter.name;
                        if (!event.triggered && input.pickup()) {
                            triggerCombinationEvent(i);
                            mapEventInteractionConsumed_ = true;
                        }
                        return;

                    case MapEncounterType::HazardousElitePack:
                    case MapEncounterType::BountyHunt:
                    case MapEncounterType::WardenCourt:
                        if (!event.triggered
                            && activeMapEventIndex_ >= 0
                            && mapEventEnemiesRemaining_ > 0) {
                            nearbyEventPrompt_ = "Another encounter active";
                            return;
                        }
                        nearbyEventPrompt_ = event.triggered
                            ? encounter.name + " active"
                            : encounter.name + " ambush";
                        if (!event.triggered) {
                            triggerCombinationEvent(i);
                        }
                        return;

                    case MapEncounterType::GuardedShrine:
                        if (!event.triggered) {
                            if (activeMapEventIndex_ >= 0
                                && mapEventEnemiesRemaining_ > 0) {
                                nearbyEventPrompt_ = "Another encounter active";
                                return;
                            }
                            nearbyEventPrompt_ = "F Awaken " + encounter.name;
                            if (input.pickup()) {
                                triggerCombinationEvent(i);
                                mapEventInteractionConsumed_ = true;
                            }
                        } else if (activeMapEventIndex_ >= 0
                            && mapEventEnemiesRemaining_ > 0) {
                            nearbyEventPrompt_ = encounter.name + ": "
                                + std::to_string(mapEventEnemiesRemaining_)
                                + " guardians left";
                        } else {
                            nearbyEventPrompt_ = "F Activate " + encounter.name;
                            if (input.pickup()) {
                                activateGuardedShrineEvent(event);
                                mapEventInteractionConsumed_ = true;
                            }
                        }
                        return;

                    case MapEncounterType::None:
                        return;
                }
            }
        }
    }
}

void GameWorld::triggerElitePackEvent(std::size_t eventIndex) {
    auto& events = map_.eventsForMutation();
    if (eventIndex >= events.size()) {
        return;
    }

    auto& event = events[eventIndex];
    if (event.triggered || event.completed
        || (activeMapEventIndex_ >= 0 && mapEventEnemiesRemaining_ > 0)) {
        return;
    }

    event.triggered = true;
    activeMapEventIndex_ = static_cast<int>(eventIndex);
    mapEventEnemiesRemaining_ = 5;
    eventStatusMessage_ = "Elite pack awakened";
    eventStatusTimer_ = 2.0f;
    spawnMapEventEnemies(eventIndex, 1, 4);
}

void GameWorld::triggerCombinationEvent(std::size_t eventIndex) {
    auto& events = map_.eventsForMutation();
    if (eventIndex >= events.size()) {
        return;
    }

    auto& event = events[eventIndex];
    const auto& encounter = map_.encounterDefinition();
    if (event.type != MapEventType::Combination
        || event.encounterType != encounter.type
        || event.triggered
        || event.completed
        || (activeMapEventIndex_ >= 0 && mapEventEnemiesRemaining_ > 0)) {
        return;
    }

    event.triggered = true;
    switch (encounter.type) {
        case MapEncounterType::EnhancedCache: {
            const int droppedCount = dropItemsAround(
                event.position,
                encounter.cacheDropCount,
                encounter.rewardMultiplier
            );
            event.completed = true;
            eventStatusMessage_ = encounter.name + ": "
                + std::to_string(droppedCount) + " items dropped";
            eventStatusTimer_ = 2.0f;
            break;
        }

        case MapEncounterType::CursedReliquary:
        case MapEncounterType::HazardousElitePack:
        case MapEncounterType::BountyHunt:
        case MapEncounterType::WardenCourt:
            activeMapEventIndex_ = static_cast<int>(eventIndex);
            mapEventEnemiesRemaining_ = encounter.eliteCount + encounter.normalCount;
            spawnMapEventEnemies(
                eventIndex,
                encounter.eliteCount,
                encounter.normalCount
            );
            if (encounter.hazard.isValid()) {
                groundHazards_.emplace_back(event.position, encounter.hazard);
            }
            eventStatusMessage_ = encounter.name + " awakened";
            eventStatusTimer_ = 2.0f;
            break;

        case MapEncounterType::GuardedShrine:
            activeMapEventIndex_ = static_cast<int>(eventIndex);
            mapEventEnemiesRemaining_ = encounter.eliteCount + encounter.normalCount;
            spawnMapEventEnemies(
                eventIndex,
                encounter.eliteCount,
                encounter.normalCount
            );
            eventStatusMessage_ = encounter.name + " awakened";
            eventStatusTimer_ = 2.0f;
            break;

        case MapEncounterType::None:
            event.triggered = false;
            break;
    }
}

void GameWorld::spawnMapEventEnemies(
    std::size_t eventIndex,
    int eliteCount,
    int normalCount
) {
    if (eventIndex >= map_.events().size()) {
        return;
    }

    const auto& event = map_.events()[eventIndex];
    static const Vector2 offsets[] = {
        {0.0f, 0.0f},
        {-64.0f, -42.0f},
        {62.0f, -34.0f},
        {-48.0f, 58.0f},
        {54.0f, 52.0f},
        {-92.0f, 12.0f},
        {88.0f, 16.0f},
        {0.0f, 94.0f}
    };
    const int totalCount = std::max(0, eliteCount) + std::max(0, normalCount);
    const auto& encounter = map_.encounterDefinition();
    const bool isCombination = event.type == MapEventType::Combination;
    const EnemyType primaryType = isCombination
        ? encounter.primaryEnemyType : EnemyType::Elite;
    const EnemyType secondaryType = isCombination
        ? encounter.secondaryEnemyType : EnemyType::Normal;
    const int offsetCount = static_cast<int>(sizeof(offsets) / sizeof(offsets[0]));
    for (int index = 0; index < totalCount; ++index) {
        const EnemyType type = index < eliteCount ? primaryType : secondaryType;
        const auto& definition = EnemyLibrary::forType(type);
        const EliteModifier modifier = type == EnemyType::Elite
            ? randomEliteModifier()
            : EliteModifier::None;
        const auto& modifierDefinition = EliteModifierLibrary::forModifier(modifier);
        const int hp = std::max(1, static_cast<int>(std::ceil(
            enemyHpForMap() * definition.hpMultiplier * modifierDefinition.hpMultiplier
        )));
        const int damage = enemyDamageForMap()
            + definition.damageBonus + modifierDefinition.damageBonus;
        const Vector2 offset = offsets[static_cast<std::size_t>(
            std::min(index, offsetCount - 1)
        )];
        enemies_.emplace_back(
            event.position + offset,
            hp,
            damage,
            type,
            modifier,
            static_cast<int>(eventIndex)
        );
    }
}

void GameWorld::openLootCacheEvent(MapEventInstance& event) {
    if (event.triggered || event.completed) {
        return;
    }

    event.triggered = true;
    event.completed = true;
    const int droppedCount = dropItemsAround(
        event.position,
        2,
        mapModifier_.eventRewardMultiplier
    );
    eventStatusMessage_ = "Cache opened: " + std::to_string(droppedCount) + " items dropped";
    eventStatusTimer_ = 2.0f;
}

void GameWorld::activateShrineEvent(MapEventInstance& event) {
    if (event.triggered || event.completed) {
        return;
    }

    event.triggered = true;
    event.completed = true;
    shrineBuffTimer_ = Config::ShrineBuffDuration;
    eventStatusMessage_ = "Shrine activated: +"
        + std::to_string(Config::ShrineDamageBonusPercent)
        + "% damage";
    eventStatusTimer_ = 2.0f;
}

void GameWorld::activateGuardedShrineEvent(MapEventInstance& event) {
    const auto& encounter = map_.encounterDefinition();
    if (event.type != MapEventType::Combination
        || encounter.type != MapEncounterType::GuardedShrine
        || !event.triggered
        || event.completed
        || mapEventEnemiesRemaining_ > 0) {
        return;
    }

    event.completed = true;
    shrineBuffTimer_ = Config::ShrineBuffDuration;
    eventStatusMessage_ = encounter.name + " activated: +"
        + std::to_string(Config::ShrineDamageBonusPercent) + "% damage";
    eventStatusTimer_ = 2.0f;
}

int GameWorld::dropItemsAround(
    const Vector2& center,
    int count,
    float eventRewardMultiplier
) {
    const float quantityMultiplier = std::max(0.0f, eventRewardMultiplier)
        * std::max(0.0f, mapModifier_.itemQuantityMultiplier);
    const int scaledCount = std::max(count, static_cast<int>(std::ceil(
        static_cast<float>(count) * player_.stats().itemQuantityMultiplier * quantityMultiplier
    )));
    for (int i = 0; i < scaledCount; ++i) {
        const float angle = static_cast<float>(i) * 2.39996323f;
        const float radius = i == 0 ? 0.0f : 24.0f + static_cast<float>(i) * 5.0f;
        const Vector2 offset(std::cos(angle) * radius, std::sin(angle) * radius);
        droppedItems_.push_back(DroppedItem(
            center + offset,
            lootGenerator_.generate(itemLevelForMap(), random_, mapModifier_.lootBias())
        ));
        ++mapItemsDropped_;
    }

    return scaledCount;
}

int GameWorld::damageForPlayerSkill(const SkillDefinition& skill) const {
    const float shrineMultiplier = shrineBuffTimer_ > 0.0f
        ? Config::ShrineDamageMultiplier : 1.0f;
    return skillDamage(
        skill, player_.stats(), skillBar_.supportDefinitionsFor(skill), shrineMultiplier
    );
}

void GameWorld::applySkillProgression() {
    skillBar_.applyProgression(progression_.skillLevels, progression_.supportLevels);
    skillBar_.applyStats(player_.stats());
}

float GameWorld::radiusForPlayerSkill(const SkillDefinition& skill) const {
    return skillRadius(skill, player_.stats(), skillBar_.supportDefinitionsFor(skill));
}

int GameWorld::pierceCountForPlayerSkill(const SkillDefinition& skill) const {
    return skillPierceCount(skillBar_.supportDefinitionsFor(skill));
}

int GameWorld::projectileCountForPlayerSkill(const SkillDefinition& skill) const {
    return skillProjectileCount(
        skill, skillBar_.supportDefinitionsFor(skill), player_.stats()
    );
}

float GameWorld::spreadAngleForPlayerSkill(const SkillDefinition& skill) const {
    return skillSpreadAngle(skill, skillBar_.supportDefinitionsFor(skill));
}

AilmentDefinition GameWorld::ailmentForPlayerSkill(const SkillDefinition& skill) const {
    AilmentDefinition ailment = skillAilment(skill, skillBar_.supportDefinitionsFor(skill));
    if (skill.damageType == DamageType::Fire
        && ailment.type == AilmentType::Ignite
        && hasBossRelicTheme(ItemBaseTheme::Brimstone)) {
        const auto& effect = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Brimstone);
        ailment.damageMultiplier *= effect.igniteDamageMultiplier;
        ailment.duration *= effect.igniteDurationMultiplier;
    }
    if (skill.damageType == DamageType::Poison
        && ailment.type == AilmentType::Poison
        && hasBossRelicTheme(ItemBaseTheme::Brood)) {
        const auto& effect = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Brood);
        ailment.poisonSpreadRadius = std::max(
            ailment.poisonSpreadRadius, effect.poisonSpreadRadius
        );
        ailment.poisonSpreadMultiplier = std::max(
            ailment.poisonSpreadMultiplier, effect.poisonSpreadMultiplier
        );
    }
    return ailment;
}

AilmentDefinition GameWorld::effectiveSkillAilment(const SkillDefinition& skill) const {
    return ailmentForPlayerSkill(skill);
}

bool GameWorld::hasBossRelicTheme(ItemBaseTheme theme) const {
    for (const auto& item : player_.equipment().items()) {
        if (!item) {
            continue;
        }

        const auto* base = ItemBaseLibrary::find(item->baseId);
        if (base != nullptr && base->theme == theme) {
            return true;
        }
    }
    return false;
}

void GameWorld::triggerStormChain(
    const Enemy& source,
    int sourceDamage,
    const AilmentDefinition& ailment
) {
    if (sourceDamage <= 0 || !hasBossRelicTheme(ItemBaseTheme::Storm)) {
        return;
    }

    const auto& effect = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Storm);
    if (effect.lightningChainCount <= 0 || effect.lightningChainRadius <= 0.0f) {
        return;
    }

    std::vector<int> hitIds{source.id()};
    Vector2 chainOrigin = source.position();
    for (int jump = 0; jump < effect.lightningChainCount; ++jump) {
        Enemy* target = nullptr;
        float closestDistanceSquared = effect.lightningChainRadius * effect.lightningChainRadius;
        for (auto& enemy : enemies_) {
            if (enemy.isDead()
                || std::find(hitIds.begin(), hitIds.end(), enemy.id()) != hitIds.end()) {
                continue;
            }

            const float distanceSquared = (enemy.position() - chainOrigin).lengthSquared();
            if (distanceSquared > closestDistanceSquared) {
                continue;
            }

            target = &enemy;
            closestDistanceSquared = distanceSquared;
        }

        if (target == nullptr) {
            break;
        }

        const int chainDamage = std::max(1, static_cast<int>(std::ceil(
            static_cast<float>(sourceDamage) * effect.lightningChainDamageMultiplier
        )));
        const int dealtDamage = target->takeDamage(
            damageToEnemy(*target, chainDamage, DamageType::Lightning)
        );
        if (dealtDamage > 0) {
            addCombatFeedback(target->position(), dealtDamage, effect.name);
            if (ailment.type != AilmentType::None) {
                applySkillAilment(*target, ailment, dealtDamage);
            }
        }

        hitIds.push_back(target->id());
        chainOrigin = target->position();
        if (target->isDead()) {
            rewardEnemyKill(*target);
        }
    }
}

void GameWorld::noteMapEventEnemyDefeated(const Enemy& enemy) {
    if (enemy.isBoss()
        || enemy.mapEventIndex() < 0
        || activeMapEventIndex_ < 0
        || mapEventEnemiesRemaining_ <= 0
        || enemy.mapEventIndex() != activeMapEventIndex_) {
        return;
    }

    auto& events = map_.eventsForMutation();
    const auto eventIndex = static_cast<std::size_t>(activeMapEventIndex_);
    if (eventIndex >= events.size()) {
        return;
    }

    auto& event = events[eventIndex];
    if (event.completed || !event.triggered) {
        return;
    }

    --mapEventEnemiesRemaining_;
    if (mapEventEnemiesRemaining_ <= 0) {
        activeMapEventIndex_ = -1;
        const bool guardedShrine = event.type == MapEventType::Combination
            && event.encounterType == MapEncounterType::GuardedShrine;
        if (guardedShrine) {
            eventStatusMessage_ = "Guardians defeated - activate shrine";
        } else {
            event.completed = true;
            if (event.type == MapEventType::Combination) {
                const auto& encounter = map_.encounterDefinition();
                const int droppedCount = dropItemsAround(
                    event.position,
                    encounter.completionDropCount,
                    encounter.rewardMultiplier
                );
                eventStatusMessage_ = encounter.completionDropCount > 0
                    ? encounter.name + " cleared: "
                        + std::to_string(droppedCount) + " bonus items dropped"
                    : encounter.name + " cleared";
            } else {
                eventStatusMessage_ = "Elite pack cleared - check nearby loot";
            }
        }
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
            applySkillProgression();
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
        applySkillProgression();
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
    if (!skillPanelOpen_) {
        return;
    }

    const auto& skills = SkillLibrary::all();
    int skillIndex = input.numberChoice() - 1;
    const int functionChoice = input.functionChoice();
    if (skillIndex < 0 && functionChoice >= 7 && functionChoice <= 8) {
        // Number keys cover the first ten entries. F7/F8 extend the panel to
        // the two entries that cannot fit in the 1-0 key range.
        skillIndex = functionChoice + 3;
    }
    if (skillIndex < 0) {
        return;
    }

    const auto index = static_cast<std::size_t>(skillIndex);
    if (index >= skills.size()) {
        return;
    }

    const auto& skill = skills[index];
    if (!isSkillUnlocked(skill.name)) {
        return;
    }

    if (skillBar_.assignSkill(skill.slot, skill.name)) {
        applySkillProgression();
    }
}

void GameWorld::tryCycleSkillSupport(Input& input) {
    if (!skillPanelOpen_) {
        return;
    }

    const int choice = input.functionChoice();
    if (choice == 5 || choice == 6) {
        selectedSupportLink_ = choice - 5;
        return;
    }
    if (choice <= 0 || choice > 4) {
        return;
    }

    const SkillSlot slot = static_cast<SkillSlot>(choice - 1);
    const std::size_t linkCount = SkillBar::supportLinkCount(slot);
    const std::size_t linkIndex = std::min(
        static_cast<std::size_t>(std::max(0, selectedSupportLink_)),
        linkCount - 1
    );
    selectedSupportLink_ = static_cast<int>(linkIndex);
    const auto& skill = skillBar_.definition(slot);
    std::vector<std::string> options = {""};
    for (const auto& support : SupportLibrary::all()) {
        if (!isSupportUnlocked(support.name) || !SupportLibrary::supportsSkill(support, skill)) {
            continue;
        }

        bool assignedToOtherLink = false;
        for (std::size_t other = 0; other < linkCount; ++other) {
            if (other != linkIndex && skillBar_.supportAt(slot, other) != nullptr
                && skillBar_.supportAt(slot, other)->name == support.name) {
                assignedToOtherLink = true;
                break;
            }
        }
        if (!assignedToOtherLink) {
            options.push_back(support.name);
        }
    }

    const auto* current = skillBar_.supportAt(slot, linkIndex);
    const std::string currentName = current ? current->name : "";
    auto currentIt = std::find(options.begin(), options.end(), currentName);
    const std::size_t currentIndex = currentIt == options.end()
        ? 0
        : static_cast<std::size_t>(currentIt - options.begin());
    const std::string& next = options[(currentIndex + 1) % options.size()];
    if (skillBar_.assignSupport(slot, next, linkIndex)) {
        applySkillProgression();
    }
}

void GameWorld::tryEquipInventoryItem(Input& input) {
    if (input.numberChoice() <= 0) {
        return;
    }

    const auto index = static_cast<std::size_t>(input.numberChoice() - 1);
    const Item* candidate = inventory_.itemAt(index);
    if (candidate == nullptr) {
        return;
    }

    if (!player_.canEquipItem(*candidate)) {
        const auto requiredLevel = player_.requiredLevelForItem(*candidate);
        if (requiredLevel && player_.level() < *requiredLevel) {
            eventStatusMessage_ = "Requires level " + std::to_string(*requiredLevel);
        } else {
            eventStatusMessage_ = "Cannot equip item";
        }
        eventStatusTimer_ = 2.0f;
        return;
    }

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
        applySkillProgression();
    }
    updateSelectedInventoryIndex();
}

void GameWorld::trySelectInventoryItem(Input& input) {
    if (!input.inventorySelectNext()) {
        return;
    }

    if (state_ != GameState::MapComplete) {
        stashSelectionActive_ = false;
        const std::size_t size = inventory_.size();
        if (size == 0) {
            selectedInventoryIndex_ = -1;
            return;
        }
        selectedInventoryIndex_ = (selectedInventoryIndex_ + 1) % static_cast<int>(size);
        return;
    }

    const std::size_t inventorySize = inventory_.size();
    const std::size_t stashSize = stash_.size();
    if (inventorySize == 0 && stashSize == 0) {
        selectedInventoryIndex_ = -1;
        selectedStashIndex_ = -1;
        stashSelectionActive_ = false;
        return;
    }

    if (!stashSelectionActive_) {
        if (selectedInventoryIndex_ + 1 < static_cast<int>(inventorySize)) {
            ++selectedInventoryIndex_;
        } else if (stashSize > 0) {
            stashSelectionActive_ = true;
            selectedStashIndex_ = 0;
        } else {
            selectedInventoryIndex_ = 0;
        }
        return;
    }

    if (selectedStashIndex_ + 1 < static_cast<int>(stashSize)) {
        ++selectedStashIndex_;
    } else if (inventorySize > 0) {
        stashSelectionActive_ = false;
        selectedInventoryIndex_ = 0;
    } else {
        selectedStashIndex_ = 0;
    }
}

void GameWorld::tryDropSelectedInventoryItem(Input& input) {
    if (!input.inventoryDropSelected()) {
        return;
    }

    if (state_ == GameState::MapComplete && stashSelectionActive_) {
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

void GameWorld::trySalvageSelectedInventoryItem(Input& input) {
    if (!input.inventorySalvageSelected()
        || (state_ == GameState::MapComplete && stashSelectionActive_)
        || selectedInventoryIndex_ < 0
        || static_cast<std::size_t>(selectedInventoryIndex_) >= inventory_.size()) {
        return;
    }

    const std::size_t index = static_cast<std::size_t>(selectedInventoryIndex_);
    if (auto item = inventory_.take(index)) {
        int value = 1;
        switch (item->rarity) {
            case Rarity::Magic: value = 2; break;
            case Rarity::Rare: value = 4; break;
            case Rarity::Normal: break;
        }
        progression_.forgeFragments += value;
    }
    updateSelectedInventoryIndex();
}

void GameWorld::tryMoveSelectedInventoryToStash(Input& input) {
    if (!input.stashStoreSelected()
        || state_ != GameState::MapComplete
        || craftingState_.open
        || stashSelectionActive_) {
        return;
    }

    if (selectedInventoryIndex_ < 0
        || static_cast<std::size_t>(selectedInventoryIndex_) >= inventory_.size()) {
        eventStatusMessage_ = "Select an inventory item first";
        eventStatusTimer_ = 2.0f;
        return;
    }
    if (stash_.isFull()) {
        eventStatusMessage_ = "Stash full";
        eventStatusTimer_ = 2.0f;
        return;
    }

    const std::size_t index = static_cast<std::size_t>(selectedInventoryIndex_);
    auto item = inventory_.take(index);
    if (!item) {
        updateSelectedInventoryIndex();
        return;
    }

    // Check capacity before taking and keep a defensive rollback for future
    // changes to the container implementation.
    if (!stash_.add(*item)) {
        inventory_.insert(index, std::move(*item));
        eventStatusMessage_ = "Stash full";
        eventStatusTimer_ = 2.0f;
        return;
    }

    eventStatusMessage_ = "Moved item to Stash";
    eventStatusTimer_ = 2.0f;
    updateSelectedInventoryIndex();
}

void GameWorld::tryMoveSelectedStashToInventory(Input& input) {
    if (!input.stashWithdrawSelected()
        || state_ != GameState::MapComplete
        || craftingState_.open
        || !stashSelectionActive_) {
        return;
    }

    if (selectedStashIndex_ < 0
        || static_cast<std::size_t>(selectedStashIndex_) >= stash_.size()) {
        updateSelectedInventoryIndex();
        return;
    }
    if (inventory_.isFull()) {
        eventStatusMessage_ = "Inventory full";
        eventStatusTimer_ = 2.0f;
        return;
    }

    const std::size_t index = static_cast<std::size_t>(selectedStashIndex_);
    auto item = stash_.take(index);
    if (!item) {
        updateSelectedInventoryIndex();
        return;
    }

    if (!inventory_.add(*item)) {
        stash_.insert(index, std::move(*item));
        eventStatusMessage_ = "Inventory full";
        eventStatusTimer_ = 2.0f;
        return;
    }

    eventStatusMessage_ = "Moved item to Inventory";
    eventStatusTimer_ = 2.0f;
    updateSelectedInventoryIndex();
}

void GameWorld::tryToggleCraftingPanel(Input& input) {
    if (!input.craftingToggle()) {
        return;
    }

    if (craftingState_.open) {
        closeCraftingPanel();
        return;
    }

    if (stashSelectionActive_
        || selectedInventoryIndex_ < 0
        || static_cast<std::size_t>(selectedInventoryIndex_) >= inventory_.size()) {
        eventStatusMessage_ = "Select an inventory item first";
        eventStatusTimer_ = 2.0f;
        return;
    }

    passiveTreeOpen_ = false;
    skillPanelOpen_ = false;
    hoveredPassiveNode_ = -1;
    craftingState_ = CraftingState();
    craftingState_.open = true;
}

void GameWorld::tryCraftSelectedItem(Input& input) {
    if (!craftingState_.open) {
        return;
    }

    if (input.cancel()) {
        closeCraftingPanel();
        return;
    }

    const int operationChoice = input.numberChoice();
    if (operationChoice >= 1 && operationChoice <= 3) {
        craftingState_.operation = static_cast<CraftingOperation>(operationChoice);
        craftingState_.affixIndex = -1;
        return;
    }

    const int affixChoice = input.functionChoice();
    if (affixChoice >= 1 && affixChoice <= 3
        && craftingState_.operation != CraftingOperation::None) {
        craftingState_.affixIndex = affixChoice - 1;
        applyCraftingOperation();
        craftingState_.affixIndex = -1;
    }
}

void GameWorld::applyCraftingOperation() {
    if (selectedInventoryIndex_ < 0
        || static_cast<std::size_t>(selectedInventoryIndex_) >= inventory_.size()
        || craftingState_.affixIndex < 0) {
        eventStatusMessage_ = "Select a valid affix";
        eventStatusTimer_ = 2.0f;
        return;
    }
    if (progression_.forgeFragments < Config::ForgeUpgradeCost) {
        eventStatusMessage_ = "Need " + std::to_string(Config::ForgeUpgradeCost)
            + " Forge Fragments";
        eventStatusTimer_ = 2.0f;
        return;
    }

    const std::size_t itemIndex = static_cast<std::size_t>(selectedInventoryIndex_);
    Item* item = inventory_.itemAt(itemIndex);
    if (item == nullptr) {
        eventStatusMessage_ = "Select a valid item";
        eventStatusTimer_ = 2.0f;
        return;
    }

    Item candidate = *item;
    CraftingResult result = CraftingResult::InvalidTarget;
    switch (craftingState_.operation) {
        case CraftingOperation::ImproveAffix:
            result = LootGenerator::improveAffix(candidate,
                static_cast<std::size_t>(craftingState_.affixIndex));
            break;
        case CraftingOperation::RerollAffix:
            result = LootGenerator::rerollAffix(candidate,
                static_cast<std::size_t>(craftingState_.affixIndex), random_, mapModifier_.lootBias());
            break;
        case CraftingOperation::RaiseAffixTier:
            result = LootGenerator::raiseAffixTier(candidate,
                static_cast<std::size_t>(craftingState_.affixIndex));
            break;
        case CraftingOperation::None:
            return;
    }

    if (result != CraftingResult::Success) {
        switch (result) {
            case CraftingResult::NoCandidates:
                eventStatusMessage_ = "No legal reroll candidates";
                break;
            case CraftingResult::AlreadyMaxTier:
                eventStatusMessage_ = "Affix already at max tier";
                break;
            case CraftingResult::NoImprovement:
                eventStatusMessage_ = "Affix cannot improve further";
                break;
            case CraftingResult::InvalidTarget:
                eventStatusMessage_ = "Affix is not craftable";
                break;
            case CraftingResult::Success:
                break;
        }
        eventStatusTimer_ = 2.0f;
        return;
    }

    *item = std::move(candidate);
    progression_.forgeFragments -= Config::ForgeUpgradeCost;
    switch (craftingState_.operation) {
        case CraftingOperation::ImproveAffix:
            eventStatusMessage_ = "Affix improved";
            break;
        case CraftingOperation::RerollAffix:
            eventStatusMessage_ = "Affix rerolled";
            break;
        case CraftingOperation::RaiseAffixTier:
            eventStatusMessage_ = "Affix tier raised";
            break;
        case CraftingOperation::None:
            break;
    }
    eventStatusTimer_ = 2.0f;
}

void GameWorld::closeCraftingPanel() {
    craftingState_ = CraftingState();
}

void GameWorld::updateSelectedInventoryIndex() {
    const std::size_t size = inventory_.size();
    if (size == 0) {
        selectedInventoryIndex_ = -1;
    } else if (selectedInventoryIndex_ >= static_cast<int>(size)) {
        selectedInventoryIndex_ = static_cast<int>(size) - 1;
    }

    const std::size_t stashSize = stash_.size();
    if (stashSize == 0) {
        selectedStashIndex_ = -1;
    } else if (selectedStashIndex_ >= static_cast<int>(stashSize)) {
        selectedStashIndex_ = static_cast<int>(stashSize) - 1;
    }

    if (state_ != GameState::MapComplete) {
        stashSelectionActive_ = false;
        selectedStashIndex_ = -1;
        return;
    }

    if (stashSelectionActive_ && stashSize == 0 && size > 0) {
        stashSelectionActive_ = false;
    } else if (!stashSelectionActive_ && size == 0 && stashSize > 0) {
        stashSelectionActive_ = true;
        if (selectedStashIndex_ < 0) {
            selectedStashIndex_ = 0;
        }
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
                progression_.skillLevels.try_emplace(reward.skillName, 1);
            }
            break;
        case MapRewardType::UnlockSupport:
            if (!reward.supportName.empty()) {
                progression_.unlockedSupports.insert(reward.supportName);
                progression_.supportLevels.try_emplace(reward.supportName, 1);
            }
            break;
        case MapRewardType::UpgradeSkill:
            if (!reward.skillName.empty()
                && progression_.unlockedSkills.find(reward.skillName)
                    != progression_.unlockedSkills.end()) {
                progression_.skillLevels[reward.skillName] = std::clamp(
                    reward.targetLevel, 1, Config::SkillGemMaxLevel
                );
            }
            break;
        case MapRewardType::UpgradeSupport:
            if (!reward.supportName.empty()
                && progression_.unlockedSupports.find(reward.supportName)
                    != progression_.unlockedSupports.end()) {
                progression_.supportLevels[reward.supportName] = std::clamp(
                    reward.targetLevel, 1, Config::SkillGemMaxLevel
                );
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

    applySkillProgression();
}

void GameWorld::generateMapRewardOptions() {
    mapRewardOptions_ = MapRewardLibrary::generateOptions(
        progression_.unlockedSkills,
        progression_.unlockedSupports,
        progression_.skillLevels,
        progression_.supportLevels,
        mapLevel_,
        random_
    );
    selectedMapRewardOption_ = -1;
    mapRewardChosen_ = false;
}

void GameWorld::generateNextMapOptions() {
    nextMapOptions_ = MapOptionLibrary::generateOptions(mapLevel_ + 1);
    selectedNextMapOption_ = -1;
    nextMapOptionChosen_ = false;
}

void GameWorld::spreadPoisonOnDeath(const Enemy& source) {
    if (source.isBoss()
        || source.poisonSpreadRadius() <= 0.0f
        || source.poisonSpreadMultiplier() <= 0.0f
        || source.poisonDamagePerTick() <= 0) {
        return;
    }

    const int spreadDamage = std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(source.poisonDamagePerTick())
            * source.poisonSpreadMultiplier()
    )));
    const float spreadDuration = std::max(0.5f, source.poisonTimeRemaining());
    for (auto& target : enemies_) {
        if (target.isDead() || target.id() == source.id() || target.isBoss()) {
            continue;
        }

        const float distanceLimit = source.poisonSpreadRadius()
            + source.radius() + target.radius();
        if ((target.position() - source.position()).lengthSquared()
                > distanceLimit * distanceLimit) {
            continue;
        }

        target.applyPoison(
            spreadDamage,
            spreadDuration,
            Config::MaxPoisonStacks,
            source.poisonSpreadRadius(),
            source.poisonSpreadMultiplier()
        );
        addCombatFeedback(target.position(), 0, "Contagion", CombatFeedbackType::Status);
    }
}

void GameWorld::initializeRunProgression() {
    progression_ = RunProgression();
    progression_.unlockedSkills.insert(SkillLibrary::spreadShot().name);
    progression_.unlockedSkills.insert(SkillLibrary::meteor().name);
    progression_.unlockedSkills.insert(SkillLibrary::pulse().name);
    progression_.unlockedSkills.insert(SkillLibrary::dash().name);
    for (const auto& skill : progression_.unlockedSkills) {
        progression_.skillLevels[skill] = 1;
    }
}

void GameWorld::rewardEnemyKill(Enemy& enemy) {
    if (!enemy.claimKillReward()) {
        return;
    }

    spreadPoisonOnDeath(enemy);

    const auto& definition = EnemyLibrary::forType(enemy.type());

    const auto& eliteModifier = EliteModifierLibrary::forModifier(enemy.eliteModifier());
    if (eliteModifier.deathBurstRadius > 0.0f) {
        volatileExplosionCenter_ = enemy.position();
        volatileExplosionRadius_ = eliteModifier.deathBurstRadius;
        volatileExplosionTimer_ = Config::VolatileExplosionEffectDuration;
        if (Collision::circleCircle(
                player_.position(), player_.radius(),
                volatileExplosionCenter_, volatileExplosionRadius_
            )) {
                damagePlayer(eliteModifier.deathBurstDamage, eliteModifier.name + " explosion");
        }
    }

    if (enemy.isBoss()) {
        map_.markBossDefeated();
        bossProjectiles_.clear();
        enemyProjectiles_.clear();
        groundHazards_.clear();
        bossAoeTelegraphTimer_ = 0.0f;
        bossAoeEffectTimer_ = 0.0f;
        bossAoeSkill_ = BossSkillDefinition();
        resetBossDash();
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

    const bool restoresFlask = definition.flaskChargeAmount > 0
        && random_.chance(definition.flaskChargeChancePercent);
    if (restoresFlask) {
        restoreLifeFlaskCharges(
            definition.flaskChargeAmount,
            enemy.isBoss() ? "Boss kill" : definition.name + " kill"
        );
    }

    const float eliteDropMultiplier = enemy.isBoss() ? bossDefinition_->dropMultiplier
        : definition.dropMultiplier;
    const int dropChance = itemDropChancePercent(
        Config::ItemDropChancePercent,
        mapModifier_.itemQuantityMultiplier * eliteDropMultiplier,
        player_.stats()
    );

    int dropsToCreate = random_.chance(dropChance) ? 1 : 0;
    if (enemy.isBoss()) {
        const int guaranteedDrops = bossDefinition_->guaranteedDrops + mapModifier_.bossDropBonus;
        const int scaledGuaranteedDrops = std::max(guaranteedDrops, static_cast<int>(std::ceil(
            static_cast<float>(guaranteedDrops) * player_.stats().itemQuantityMultiplier
        )));
        dropsToCreate = std::max(dropsToCreate, scaledGuaranteedDrops);
    }

    for (int i = 0; i < dropsToCreate; ++i) {
        const float angle = static_cast<float>(i) * 2.39996323f;
        const float radius = i == 0 ? 0.0f : 18.0f + static_cast<float>(i) * 4.0f;
        const Vector2 offset(std::cos(angle) * radius, std::sin(angle) * radius);
        LootBias dropBias = mapModifier_.lootBias();
        if (enemy.isBoss()) {
            mergeLootBias(dropBias, bossLootBias(bossDefinition_->lootTheme));
        }
        Item item = enemy.isBoss() && i == 0
            ? lootGenerator_.generateBossReward(itemLevelForMap(), bossDefinition_->lootTheme)
            : lootGenerator_.generate(itemLevelForMap(), random_, dropBias);
        droppedItems_.push_back(DroppedItem(enemy.position() + offset, std::move(item)));
        ++mapItemsDropped_;
        if (enemy.isBoss()) {
            ++mapBossItemsDropped_;
        }
    }

    noteMapEventEnemyDefeated(enemy);
}

void GameWorld::damagePlayer(
    int damage,
    const std::string& source,
    DamageType damageType,
    AilmentDefinition ailment
) {
    if (playerHitCooldown_ > 0.0f) {
        return;
    }

    Stats effectiveStats = player_.stats();
    effectiveStats.fireResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Fire, false
    );
    effectiveStats.coldResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Cold, false
    );
    effectiveStats.lightningResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Lightning, false
    );
    effectiveStats.poisonResistance += mapElementalResistanceAdjustment(
        mapModifier_, DamageType::Poison, false
    );
    playerHitDamage_ = player_.takeDamage(incomingDamage(damage, effectiveStats, damageType));
    if (playerHitDamage_ <= 0) {
        return;
    }

    applyPlayerAilment(ailment, damageType, playerHitDamage_, effectiveStats);

    playerHitSource_ = source;
    playerHitEffectTimer_ = Config::PlayerHitEffectDuration;
    playerHitCooldown_ = Config::PlayerHitCooldown;
    addCombatFeedback(
        player_.position(),
        playerHitDamage_,
        source,
        CombatFeedbackType::PlayerHit
    );
}

void GameWorld::applyPlayerAilment(
    const AilmentDefinition& ailment,
    DamageType damageType,
    int hitDamage,
    const Stats& effectiveStats
) {
    if (ailment.type == AilmentType::None || hitDamage <= 0) {
        return;
    }

    const int resistance = resistanceForDamageType(
        damageType,
        effectiveStats.fireResistance,
        effectiveStats.coldResistance,
        effectiveStats.lightningResistance,
        effectiveStats.poisonResistance
    );
    bool applied = false;
    switch (ailment.type) {
        case AilmentType::Ignite: {
            const int tickDamage = ailmentTickDamageAfterResistance(
                ailmentTickDamage(ailment, hitDamage),
                resistance,
                ailment.ignitePenetration
            );
            if (tickDamage <= 0) {
                return;
            }
            player_.applyIgnite(
                tickDamage,
                ailment.duration
            );
            applied = true;
            break;
        }
        case AilmentType::Chill: {
            const float speedMultiplier = chillSpeedMultiplierAfterResistance(
                ailment.speedMultiplier,
                resistance,
                ailment.chillPenetration
            );
            if (speedMultiplier >= 1.0f) {
                return;
            }
            player_.applyChill(
                speedMultiplier,
                ailment.duration
            );
            applied = true;
            break;
        }
        case AilmentType::Shock: {
            const float damageTakenMultiplier = damageTakenMultiplierAfterResistance(
                ailment.damageTakenMultiplier,
                resistance,
                ailment.shockPenetration
            );
            if (damageTakenMultiplier <= 1.0f) {
                return;
            }
            player_.applyShock(
                damageTakenMultiplier,
                ailment.duration
            );
            applied = true;
            break;
        }
        case AilmentType::Poison: {
            const int tickDamage = ailmentTickDamageAfterResistance(
                ailmentTickDamage(ailment, hitDamage),
                resistance,
                ailment.poisonPenetration
            );
            if (tickDamage <= 0) {
                return;
            }
            player_.applyPoison(tickDamage, ailment.duration);
            applied = true;
            break;
        }
        case AilmentType::None:
        case AilmentType::Count:
            return;
    }

    if (!applied) {
        return;
    }

    const char* statusName = ailmentTypeName(ailment.type);
    addCombatFeedback(
        player_.position(),
        0,
        statusName,
        CombatFeedbackType::Status
    );
}

Enemy* GameWorld::activeBoss() {
    for (auto& enemy : enemies_) {
        if (enemy.isBoss() && !enemy.isDead()) {
            return &enemy;
        }
    }

    return nullptr;
}

void GameWorld::resetBossDash() {
    bossDashState_.reset();
    bossDashSkill_ = BossSkillDefinition();
    bossDashEffectPosition_ = {};
    bossDashEffectTimer_ = 0.0f;
}

float GameWorld::bossSkillInterval() const {
    return bossDefinition_->skillInterval * (bossEnraged_
        ? bossDefinition_->enragedSkillIntervalMultiplier
        : 1.0f);
}

int GameWorld::bossSkillDamage(int baseDamage) const {
    const float enrageMultiplier = bossEnraged_
        ? bossDefinition_->enragedDamageMultiplier
        : 1.0f;
    const float mapDamage = static_cast<float>(baseDamage + mapModifier_.monsterDamageBonus)
        * mapModifier_.bossDamageMultiplier;
    return std::max(1, static_cast<int>(std::ceil(mapDamage * enrageMultiplier)));
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
    return MapScaling::enemyHp(mapLevel_, mapModifier_);
}

int GameWorld::enemyDamageForMap() const {
    return MapScaling::enemyDamage(mapLevel_, mapModifier_);
}

int GameWorld::itemLevelForMap() const {
    return MapScaling::itemLevel(mapLevel_, mapModifier_);
}

EliteModifier GameWorld::randomEliteModifier() {
    const int modifierCount = static_cast<int>(EliteModifierLibrary::all().size()) - 1;
    if (modifierCount <= 0) {
        return EliteModifier::None;
    }
    return static_cast<EliteModifier>(random_.nextInt(1, modifierCount));
}

EnemyType GameWorld::nextMapEnemyType() {
    const auto& encounter = map_.definition().encounter;
    const int eliteWeight = std::min(
        45, encounter.eliteWeight + mapLevel_ * 2 + mapModifier_.eliteWeightBonus
    );
    const int normalWeight = std::max(1, encounter.normalWeight - (eliteWeight - encounter.eliteWeight));
    const int rangedWeight = std::max(0, encounter.rangedWeight);
    const int chargerWeight = std::max(
        0,
        encounter.chargerWeight + mapModifier_.chargerWeightBonus
    );
    const int wardenWeight = std::max(0, encounter.wardenWeight + mapLevel_ / 2);
    const int summonerWeight = std::max(0, encounter.summonerWeight + mapLevel_ / 3);
    const int totalWeight = normalWeight + rangedWeight + chargerWeight
        + eliteWeight + wardenWeight + summonerWeight;
    const int roll = random_.nextInt(0, totalWeight - 1);

    if (roll < eliteWeight) {
        return EnemyType::Elite;
    }
    if (roll < eliteWeight + rangedWeight) {
        return EnemyType::Ranged;
    }
    if (roll < eliteWeight + rangedWeight + chargerWeight) {
        return EnemyType::Charger;
    }
    if (roll < eliteWeight + rangedWeight + chargerWeight + wardenWeight) {
        return EnemyType::Warden;
    }
    if (roll < eliteWeight + rangedWeight + chargerWeight + wardenWeight + summonerWeight) {
        return EnemyType::Summoner;
    }
    return EnemyType::Normal;
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
    activeMapEventIndex_ = -1;
    mapEventEnemiesRemaining_ = 0;
    nearbyEventPrompt_.clear();
    mapEventInteractionConsumed_ = false;
    bossAoeCenter_ = map_.bossCenter();
    bossAoeTelegraphTimer_ = 0.0f;
    bossAoeEffectTimer_ = 0.0f;
    volatileExplosionTimer_ = 0.0f;
    volatileExplosionRadius_ = 0.0f;
    bossAoeSkill_ = BossSkillDefinition();
    resetBossDash();
    bossSkillTimer_ = bossDefinition_->skillInterval * 0.5f;
    bossSkillIndex_ = 0;
    bossEnraged_ = false;
    eventStatusMessage_ = "Boss awakened: " + bossDefinition_->name;
    eventStatusTimer_ = 2.0f;

    const int hp = MapScaling::bossHp(mapLevel_, mapModifier_, *bossDefinition_);
    const int damage = MapScaling::bossContactDamage(
        mapLevel_, mapModifier_, *bossDefinition_
    );
    enemies_.push_back(Enemy(map_.bossCenter(), hp, damage, EnemyType::Boss));
}

const Player& GameWorld::player() const { return player_; }
const std::vector<Projectile>& GameWorld::projectiles() const { return projectiles_; }
const std::vector<BossProjectile>& GameWorld::bossProjectiles() const { return bossProjectiles_; }
const std::vector<EnemyProjectile>& GameWorld::enemyProjectiles() const { return enemyProjectiles_; }
const std::vector<Enemy>& GameWorld::enemies() const { return enemies_; }
const std::vector<CombatFeedback>& GameWorld::combatFeedback() const { return combatFeedback_; }
const std::vector<GroundHazard>& GameWorld::groundHazards() const { return groundHazards_; }
const std::vector<DroppedItem>& GameWorld::droppedItems() const { return droppedItems_; }
const Inventory& GameWorld::inventory() const { return inventory_; }
const Stash& GameWorld::stash() const { return stash_; }
int GameWorld::lifeFlaskCharges() const { return lifeFlaskCharges_; }
int GameWorld::lifeFlaskMaxCharges() const { return Config::LifeFlaskMaxCharges; }
std::string GameWorld::lifeFlaskStatusMessage() const { return lifeFlaskStatusMessage_; }
float GameWorld::lifeFlaskStatusTimeRemaining() const { return lifeFlaskStatusTimer_; }
int GameWorld::playerHitDamage() const { return playerHitDamage_; }
std::string GameWorld::playerHitSource() const { return playerHitSource_; }
float GameWorld::playerHitEffectProgress() const {
    return Config::PlayerHitEffectDuration > 0.0f
        ? playerHitEffectTimer_ / Config::PlayerHitEffectDuration
        : 0.0f;
}
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
const Vector2& GameWorld::dashImpactPosition() const { return dashImpactPosition_; }
float GameWorld::dashImpactProgress() const {
    return dashImpactDuration_ > 0.0f ? dashImpactTimer_ / dashImpactDuration_ : 0.0f;
}
float GameWorld::dashImpactRadius() const { return dashImpactRadius_; }
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
const Vector2& GameWorld::bossDashStart() const { return bossDashState_.start(); }
const Vector2& GameWorld::bossDashTarget() const { return bossDashState_.target(); }
float GameWorld::bossDashTelegraphProgress() const { return bossDashState_.telegraphProgress(); }
bool GameWorld::bossDashMoving() const { return bossDashState_.isMoving(); }
const Vector2& GameWorld::bossDashEffectPosition() const { return bossDashEffectPosition_; }
float GameWorld::bossDashEffectProgress() const {
    return bossDashSkill_.effectDuration > 0.0f
        ? bossDashEffectTimer_ / bossDashSkill_.effectDuration
        : 0.0f;
}
float GameWorld::bossDashRadius() const { return bossDashSkill_.radius; }
const Vector2& GameWorld::volatileExplosionCenter() const { return volatileExplosionCenter_; }
float GameWorld::volatileExplosionRadius() const { return volatileExplosionRadius_; }
float GameWorld::volatileExplosionProgress() const {
    return Config::VolatileExplosionEffectDuration > 0.0f
        ? volatileExplosionTimer_ / Config::VolatileExplosionEffectDuration
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
int GameWorld::selectedSupportLink() const { return selectedSupportLink_; }
int GameWorld::hoveredPassiveNode() const { return hoveredPassiveNode_; }
std::string GameWorld::passiveBuildSummary() const {
    const auto& tree = player_.passiveTree();
    return "Projectile " + std::to_string(tree.allocatedCount(PassiveBranch::Projectile))
        + " / Area " + std::to_string(tree.allocatedCount(PassiveBranch::Area))
        + " / Survival " + std::to_string(tree.allocatedCount(PassiveBranch::Survival))
        + " / Loot " + std::to_string(tree.allocatedCount(PassiveBranch::Loot))
        + " / Poison " + std::to_string(tree.allocatedCount(PassiveBranch::Poison))
        + " / Keystone " + tree.keystoneSummary();
}
std::string GameWorld::bossRelicEffectSummary() const {
    const ItemBaseTheme themes[] = {
        ItemBaseTheme::Brimstone,
        ItemBaseTheme::Storm,
        ItemBaseTheme::Brood
    };
    std::string summary;
    for (const auto theme : themes) {
        if (!hasBossRelicTheme(theme)) {
            continue;
        }

        if (!summary.empty()) {
            summary += " | ";
        }
        summary += BossRelicEffectLibrary::forTheme(theme).name;
    }
    return summary.empty() ? "None" : summary;
}
bool GameWorld::isSkillUnlocked(const std::string& name) const {
    return progression_.unlockedSkills.find(name) != progression_.unlockedSkills.end();
}
bool GameWorld::isSupportUnlocked(const std::string& name) const {
    return progression_.unlockedSupports.find(name) != progression_.unlockedSupports.end();
}
int GameWorld::skillLevel(const std::string& name) const {
    const auto it = progression_.skillLevels.find(name);
    return it == progression_.skillLevels.end() ? 1 : it->second;
}
int GameWorld::supportLevel(const std::string& name) const {
    const auto it = progression_.supportLevels.find(name);
    return it == progression_.supportLevels.end() ? 1 : it->second;
}
GameState GameWorld::state() const { return state_; }
bool GameWorld::quitRequested() const { return quitRequested_; }
int GameWorld::score() const { return score_; }
float GameWorld::survivalTime() const { return survivalTime_; }
int GameWorld::mapLevel() const { return mapLevel_; }
std::uint64_t GameWorld::runSeed() const { return runSeed_; }
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
int GameWorld::activeEliteEventEnemiesRemaining() const { return mapEventEnemiesRemaining_; }
std::string GameWorld::bossSkillWarning() const {
    if (bossDashState_.isTelegraphing() && !bossDashSkill_.name.empty()) {
        return bossSkillWarningText(bossDashSkill_);
    }
    if (bossAoeTelegraphTimer_ <= 0.0f || bossAoeSkill_.name.empty()) {
        return "";
    }

    return bossSkillWarningText(bossAoeSkill_);
}
bool GameWorld::bossEnraged() const { return bossEnraged_; }
std::string GameWorld::bossPhaseSummary() const {
    if (!bossEnraged_) {
        return "Pattern: " + bossDefinition_->patternDescription;
    }

    std::string summary = "Enraged: " + bossDefinition_->enragedPatternDescription;
    if (!bossDefinition_->enrageTransitionDescription.empty()) {
        summary += " | " + bossDefinition_->enrageTransitionDescription;
    }
    return summary;
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
int GameWorld::selectedStashIndex() const { return selectedStashIndex_; }
bool GameWorld::stashSelectionActive() const { return stashSelectionActive_; }
bool GameWorld::craftingPanelOpen() const { return craftingState_.open; }
CraftingOperation GameWorld::craftingOperation() const { return craftingState_.operation; }
int GameWorld::craftingAffixIndex() const { return craftingState_.affixIndex; }
int GameWorld::forgeFragments() const { return progression_.forgeFragments; }
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
