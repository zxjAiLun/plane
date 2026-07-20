#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Config.hpp"
#include "CombatMath.hpp"
#include "EnemyPackLibrary.hpp"
#include "GameWorld.hpp"
#include "Input.hpp"
#include "ItemBase.hpp"
#include "LootGenerator.hpp"
#include "MapRewardLibrary.hpp"
#include "MapScaling.hpp"
#include "SaveService.hpp"
#include "SkillLibrary.hpp"
#include "SupportLibrary.hpp"

namespace {

int failures = 0;
int checks = 0;

void expect(bool condition, const std::string& label) {
    ++checks;
    if (condition) {
        std::cout << "  PASS  " << label << '\n';
    } else {
        ++failures;
        std::cout << "  FAIL  " << label << '\n';
    }
}

std::string enemySignature(const GameWorld& world) {
    std::string signature;
    for (const auto& enemy : world.enemies()) {
        signature += std::to_string(static_cast<int>(enemy.type())) + ":"
            + std::to_string(static_cast<int>(enemy.eliteModifier())) + ":"
            + std::to_string(enemy.hp()) + ":"
            + std::to_string(enemy.position().x) + ":"
            + std::to_string(enemy.position().y) + ";";
    }
    return signature;
}

void advanceIntoTheField(GameWorld& world, Input& input) {
    input.handleKeyPressed(sf::Keyboard::Key::D);
    for (int frame = 0; frame < 60; ++frame) {
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::D);
}

void settleEnemies(GameWorld& world, Input& input) {
    for (int frame = 0; frame < 35; ++frame) {
        world.update(0.05f, input);
    }
}

void resolvePendingSkillEffects(GameWorld& world, Input& input) {
    for (int frame = 0; frame < 24 && !world.pendingSkillEffects().empty(); ++frame) {
        world.update(0.05f, input);
        if (std::any_of(
                world.pendingSkillEffects().begin(),
                world.pendingSkillEffects().end(),
                [](const PendingSkillEffect& effect) { return effect.impacted; })) {
            return;
        }
    }
}

void pressKey(GameWorld& world, Input& input, sf::Keyboard::Key key) {
    input.handleKeyPressed(key);
    world.update(0.05f, input);
    input.handleKeyReleased(key);
}

Item makeBaseItem(const std::string& baseId) {
    Item item;
    const auto* base = ItemBaseLibrary::find(baseId);
    if (base == nullptr) {
        return item;
    }

    item.name = base->name;
    item.slot = base->slot;
    item.rarity = Rarity::Magic;
    item.itemLevel = 1;
    item.baseId = base->id;
    item.baseName = base->name;
    item.implicitStats = base->implicitStats;
    item.stats = base->implicitStats;
    return item;
}

void releaseMovement(Input& input) {
    input.handleKeyReleased(sf::Keyboard::Key::A);
    input.handleKeyReleased(sf::Keyboard::Key::D);
    input.handleKeyReleased(sf::Keyboard::Key::W);
    input.handleKeyReleased(sf::Keyboard::Key::S);
}

bool moveAxisTo(GameWorld& world, Input& input, bool horizontal, float target) {
    constexpr float arrivalDistance = 100.0f;
    constexpr int maxFrames = 240;
    for (int frame = 0; frame < maxFrames && world.state() == GameState::Playing; ++frame) {
        const float current = horizontal
            ? world.player().position().x : world.player().position().y;
        const float delta = target - current;
        if (std::abs(delta) <= arrivalDistance) {
            releaseMovement(input);
            return true;
        }

        const bool positive = delta > 0.0f;
        const auto key = horizontal
            ? (positive ? sf::Keyboard::Key::D : sf::Keyboard::Key::A)
            : (positive ? sf::Keyboard::Key::S : sf::Keyboard::Key::W);
        const auto opposite = horizontal
            ? (positive ? sf::Keyboard::Key::A : sf::Keyboard::Key::D)
            : (positive ? sf::Keyboard::Key::W : sf::Keyboard::Key::S);
        input.handleKeyPressed(key);
        input.handleKeyReleased(opposite);
        if (horizontal) {
            input.handleKeyReleased(sf::Keyboard::Key::W);
            input.handleKeyReleased(sf::Keyboard::Key::S);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::A);
            input.handleKeyReleased(sf::Keyboard::Key::D);
        }
        world.update(0.05f, input);
    }

    releaseMovement(input);
    return world.state() == GameState::Playing
        && std::abs((horizontal ? world.player().position().x : world.player().position().y) - target)
            <= arrivalDistance;
}

bool moveToBoss(GameWorld& world, Input& input) {
    if (!world.bossGateUnlocked()) {
        const auto path = std::filesystem::temp_directory_path()
            / ("plane_fight_boss_gate_fixture_"
                + std::to_string(world.runSeed()) + ".bin");
        std::filesystem::remove(path);
        SaveData data;
        std::string error;
        if (world.saveRun(path) && SaveService::load(path, data, &error)) {
            data.fieldPacksCleared = world.fieldPacksRequired();
            data.state = SavedRunState::Playing;
            data.mapRewardChosen = false;
            data.nextMapOptionChosen = false;
            data.selectedMapRewardOption = -1;
            data.selectedNextMapOption = -1;
            if (SaveService::save(path, data, &error)) {
                world.loadRun(path);
            }
        }
        std::filesystem::remove(path);
    }

    const Vector2 bossCenter = world.map().bossCenter();
    if (!moveAxisTo(world, input, true, bossCenter.x)) {
        return false;
    }
    if (world.map().bossTriggered()) {
        return true;
    }
    return moveAxisTo(world, input, false, bossCenter.y) && world.map().bossTriggered();
}

sf::Vector2i worldToScreen(const GameWorld& world, const Vector2& position) {
    const Vector2 camera = world.cameraTopLeft();
    return {
        static_cast<int>(std::lround(position.x - camera.x)),
        static_cast<int>(std::lround(position.y - camera.y))
    };
}

bool defeatBossWithAreaSkill(GameWorld& world, Input& input) {
    for (int attempt = 0; attempt < 12 && world.state() == GameState::Playing; ++attempt) {
        const auto bossIt = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
        if (bossIt == world.enemies().end()) {
            world.update(0.05f, input);
            continue;
        }

        input.handleMousePressed(sf::Mouse::Button::Right, worldToScreen(world, bossIt->position()));
        world.update(0.05f, input);
        if (world.state() != GameState::Playing) {
            break;
        }
        world.update(1.5f, input);
    }

    return world.state() == GameState::MapComplete && world.map().bossDefeated();
}

bool statsEqual(const Stats& first, const Stats& second) {
    return first.maxHp == second.maxHp
        && std::abs(first.moveSpeedMultiplier - second.moveSpeedMultiplier) < 0.0001f
        && std::abs(first.damageMultiplier - second.damageMultiplier) < 0.0001f
        && std::abs(first.attackSpeedMultiplier - second.attackSpeedMultiplier) < 0.0001f
        && std::abs(first.pickupRangeMultiplier - second.pickupRangeMultiplier) < 0.0001f
        && std::abs(first.projectileDamageMultiplier - second.projectileDamageMultiplier) < 0.0001f
        && std::abs(first.areaDamageMultiplier - second.areaDamageMultiplier) < 0.0001f
        && std::abs(first.areaRadiusMultiplier - second.areaRadiusMultiplier) < 0.0001f
        && std::abs(first.poisonDamageMultiplier - second.poisonDamageMultiplier) < 0.0001f
        && std::abs(first.physicalDamageMultiplier - second.physicalDamageMultiplier) < 0.0001f
        && std::abs(first.bleedDamageMultiplier - second.bleedDamageMultiplier) < 0.0001f
        && std::abs(first.bleedDurationMultiplier - second.bleedDurationMultiplier) < 0.0001f
        && std::abs(first.maxManaMultiplier - second.maxManaMultiplier) < 0.0001f
        && std::abs(first.manaRegenMultiplier - second.manaRegenMultiplier) < 0.0001f
        && std::abs(first.skillCostMultiplier - second.skillCostMultiplier) < 0.0001f
        && first.armor == second.armor
        && first.projectileCountBonus == second.projectileCountBonus
        && std::abs(first.lifeFlaskEffectMultiplier - second.lifeFlaskEffectMultiplier) < 0.0001f
        && std::abs(first.itemQuantityMultiplier - second.itemQuantityMultiplier) < 0.0001f
        && std::abs(first.incomingDamageMultiplier - second.incomingDamageMultiplier) < 0.0001f
        && first.poisonResistance == second.poisonResistance
        && first.bleedPenetration == second.bleedPenetration
        && first.bleedResistance == second.bleedResistance;
}

void testPauseContextsAndFreeze() {
    GameWorld panelWorld(12001);
    Input panelInput;
    pressKey(panelWorld, panelInput, sf::Keyboard::Key::P);
    expect(panelWorld.state() == GameState::Playing && panelWorld.passiveTreeOpen(),
        "opening Passive Tree keeps the world in Playing");
    pressKey(panelWorld, panelInput, sf::Keyboard::Key::Escape);
    expect(panelWorld.state() == GameState::Playing && !panelWorld.passiveTreeOpen(),
        "Escape closes Passive Tree before pausing");
    pressKey(panelWorld, panelInput, sf::Keyboard::Key::Escape);
    expect(panelWorld.state() == GameState::Paused,
        "second Escape opens Pause");
    pressKey(panelWorld, panelInput, sf::Keyboard::Key::Escape);
    expect(panelWorld.state() == GameState::Playing,
        "Escape resumes the state that was paused");

    GameWorld frozenWorld(13001);
    Input frozenInput;
    advanceIntoTheField(frozenWorld, frozenInput);
    frozenInput.handleKeyPressed(sf::Keyboard::Key::Q);
    frozenWorld.update(0.05f, frozenInput);
    const std::string beforePauseEnemies = enemySignature(frozenWorld);
    const float beforePauseMana = frozenWorld.player().mana();
    pressKey(frozenWorld, frozenInput, sf::Keyboard::Key::Escape);
    const std::string pausedEnemies = enemySignature(frozenWorld);
    const float pausedMana = frozenWorld.player().mana();
    const float pausedTime = frozenWorld.survivalTime();
    frozenWorld.update(2.0f, frozenInput);
    expect(frozenWorld.state() == GameState::Paused
            && beforePauseEnemies == pausedEnemies
            && pausedEnemies == enemySignature(frozenWorld),
        "Pause freezes enemy simulation");
    expect(std::abs(frozenWorld.player().mana() - pausedMana) < 0.001f
            && std::abs(frozenWorld.survivalTime() - pausedTime) < 0.001f
            && pausedMana <= beforePauseMana,
        "Pause freezes mana regeneration and survival time");
    pressKey(frozenWorld, frozenInput, sf::Keyboard::Key::Escape);
    expect(frozenWorld.state() == GameState::Playing,
        "paused simulation resumes with Escape");

    const auto savePath = std::filesystem::absolute(Config::SaveFileName);
    std::filesystem::remove(savePath);
    GameWorld menuWorld(14001);
    Input menuInput;
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::Escape);
    const std::uint64_t savedSeed = menuWorld.runSeed();
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::F5);
    expect(std::filesystem::exists(savePath)
            && menuWorld.eventStatusMessage() == "Run saved",
        "Pause Save Run writes the current run");
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::F9);
    expect(menuWorld.state() == GameState::Playing && menuWorld.runSeed() == savedSeed,
        "Pause Load Run restores without entering gameplay input");
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::Escape);
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::R);
    expect(menuWorld.state() == GameState::Playing && menuWorld.runSeed() != savedSeed,
        "Pause Restart Run resets the run");
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::Escape);
    pressKey(menuWorld, menuInput, sf::Keyboard::Key::Q);
    expect(menuWorld.quitRequested(),
        "Pause Quit raises a game-level quit request");
    menuWorld.reset(14002);
    expect(!menuWorld.quitRequested(), "reset clears a pending quit request");
    std::filesystem::remove(savePath);
}

void testBossGateProgression() {
    GameWorld world(12501);
    Input input;
    expect(!world.bossGateUnlocked(),
        "a fresh map starts with its Boss Gate locked");
    expect(world.mapObjective().find("Clear field packs") != std::string::npos,
        "a locked map objective points to field pack progress");

    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_boss_gate_progress_test.bin";
    std::filesystem::remove(path);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Boss Gate progress fixture starts from a valid save");
    data.fieldPacksCleared = world.fieldPacksRequired();
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss Gate progress fixture restores the required field progress");
    expect(world.bossGateUnlocked()
            && world.mapObjective() == "Explore the field",
        "required field progress unlocks the Boss Gate objective path");
    expect(world.fieldPacksCleared() == world.fieldPacksRequired(),
        "Boss Gate field progress is capped at the required threshold");
    expect(world.saveRun(path), "Boss Gate progress can be saved after unlocking");
    SaveData restored;
    expect(SaveService::load(path, restored, &error)
            && restored.fieldPacksCleared == world.fieldPacksRequired(),
        "Boss Gate progress survives a save round-trip");
    std::filesystem::remove(path);
}

void testRareLeaderCombatEffects() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_rare_leader_effects_test.bin";
    std::filesystem::remove(path);

    GameWorld world(12601);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "rare leader fixture starts from a valid run save");
    data.mapTemplateIndex = 1;
    data.mapLayoutIndex = 0;
    data.currentMapOption = MapOptionLibrary::generateOptions(1)[1];
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.state = SavedRunState::Playing;
    data.fieldPacksCleared = 0;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "rare leader fixture restores the ranged field pack map");

    Input input;
    advanceIntoTheField(world, input);
    const bool rareLeaderSpawned = std::any_of(
        world.enemies().begin(), world.enemies().end(),
        [](const Enemy& enemy) {
            return enemy.isRare()
                && enemy.displayName() == "Storm Herald"
                && enemy.secondaryEliteModifier() == EliteModifier::Empowered;
        }
    );
    expect(rareLeaderSpawned,
        "Storm field pack spawns its data-driven rare leader and secondary modifier");
    expect(world.fieldPackLeaderRewardDescription().find("Lightning") != std::string::npos,
        "Storm rare leader exposes its Lightning / Projectile reward tendency");

    bool telegraphObserved = false;
    bool telegraphFeedbackObserved = false;
    for (int frame = 0; frame < 140 && world.state() == GameState::Playing; ++frame) {
        world.update(0.05f, input);
        telegraphObserved = telegraphObserved || !world.rareLeaderSkillWarning().empty();
        telegraphFeedbackObserved = telegraphFeedbackObserved || std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::Telegraph
                    && feedback.source.find("Rare casting: Stormbound") != std::string::npos;
            }
        );
    }
    expect(telegraphObserved && telegraphFeedbackObserved,
        "Stormbound creates a visible warning and typed telegraph feedback");
    expect(world.rareLeaderAoeRadius() > 0.0f,
        "Stormbound exposes its strike radius to the renderer");
    std::filesystem::remove(path);
}

void testRareLeaderRewardProfiles() {
    const auto& packs = EnemyPackLibrary::all();
    expect(packs.size() == EnemyPackLibrary::ThemeCount * EnemyPackLibrary::PacksPerTheme,
        "every field pack has a data-driven rare leader profile");

    const bool allProfilesComplete = std::all_of(
        packs.begin(),
        packs.end(),
        [](const EnemyPackDefinition& pack) {
            return pack.leaderIndex >= 0
                && pack.leaderLootBias.primaryTag != AffixTag::None
                && pack.leaderRewardDescription.find("weighted") != std::string::npos
                && pack.leaderBonusDrops >= 1;
        }
    );
    expect(allProfilesComplete,
        "rare leader profiles define loot bias, reward text, and a guaranteed drop count");

    const auto& storm = EnemyPackLibrary::forMap(1, 1, 0);
    expect(storm.leaderName == "Storm Herald"
            && storm.leaderLootBias.primaryTag == AffixTag::Lightning
            && storm.leaderLootBias.secondaryTag == AffixTag::Projectile
            && storm.leaderBonusDrops == 2,
        "Storm Herald guarantees two Lightning / Projectile-biased drops");

    const auto& lastStand = EnemyPackLibrary::forMap(0, 1, 2);
    expect(lastStand.leaderName == "Last Ember"
            && lastStand.leaderLootBias.primaryTag == AffixTag::Fire
            && lastStand.leaderBonusDrops == 1,
        "Last Ember uses a distinct Fire / Area reward profile");
}

void testSaveLoadRoundTrip() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_world_save_test.bin";
    std::filesystem::remove(path);

    GameWorld original(7101);
    Input input;
    advanceIntoTheField(original, input);
    expect(original.saveRun(path), "GameWorld saves a live run");
    const auto expectedSeed = original.runSeed();
    const auto expectedLevel = original.mapLevel();

    original.reset(9999);
    const bool loaded = original.loadRun(path);
    if (!loaded) {
        std::cout << "    world load status: " << original.eventStatusMessage() << '\n';
    }
    expect(loaded, "GameWorld loads a valid run");
    expect(original.runSeed() == expectedSeed && original.mapLevel() == expectedLevel,
        "load restores run seed and map level");
    expect(original.state() == GameState::Playing && original.enemies().empty(),
        "load clears transient enemies and returns to safe Playing state");
    expect((original.player().position() - original.map().playerStart()).lengthSquared() < 0.01f,
        "load places the player at the map start");

    std::filesystem::remove(path);
}

void testResourceStatsSaveLoad() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_resource_save_test.bin";
    std::filesystem::remove(path);

    GameWorld source(7151);
    SaveData data;
    std::string error;
    expect(source.saveRun(path) && SaveService::load(path, data, &error),
        "resource fixture starts from a valid save");
    data.player.upgradeStats.maxManaMultiplier = 1.50f;
    data.player.upgradeStats.manaRegenMultiplier = 1.25f;
    data.player.upgradeStats.skillCostMultiplier = 0.80f;
    data.player.mana = 75.0f;
    expect(SaveService::save(path, data, &error),
        "resource fixture writes v18 resource stats");

    GameWorld restored(7152);
    expect(restored.loadRun(path), "resource fixture loads into GameWorld");
    expect(std::abs(restored.player().maxMana() - 150.0f) < 0.0001f
            && std::abs(restored.player().manaRegenPerSecond() - 10.0f) < 0.0001f
            && std::abs(restored.player().mana() - 75.0f) < 0.0001f
            && std::abs(restored.player().stats().skillCostMultiplier - 0.80f) < 0.0001f,
        "loaded resource stats affect Mana pool, regeneration, and skill cost");

    std::filesystem::remove(path);
}

void testCorruptLoadDoesNotMutate() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_world_corrupt_test.bin";
    std::filesystem::remove(path);

    GameWorld source(8101);
    expect(source.saveRun(path), "source run creates a save for corruption test");
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << "broken";
    file.close();

    GameWorld target(8102);
    expect(!target.loadRun(path), "corrupt run is rejected");
    expect(target.runSeed() == 8102 && target.mapLevel() == 1,
        "rejected load leaves the current run untouched");
    std::filesystem::remove(path);
}

bool saveAsMapCompleteFixture(
    const std::filesystem::path& path,
    GameWorld& world,
    int sequence,
    bool addGroundDrop
);

void testMapCompleteLoad() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_map_complete_save_test.bin";
    std::filesystem::remove(path);

    GameWorld source(9101);
    expect(source.saveRun(path), "source run creates a settlement save");
    SaveData data;
    std::string error;
    expect(SaveService::load(path, data, &error), "settlement save can be edited for the fixture");
    data.state = SavedRunState::MapComplete;
    data.mapEvents.resize(3);
    for (auto& event : data.mapEvents) {
        event.triggered = true;
        event.completed = true;
    }
    expect(SaveService::save(path, data, &error), "settlement fixture is written");

    GameWorld target(9102);
    expect(target.loadRun(path), "GameWorld loads a MapComplete checkpoint");
    expect(target.state() == GameState::MapComplete && target.map().bossDefeated(),
        "MapComplete load restores the boss-defeated settlement phase");
    expect(target.mapEventsCompleted() == target.mapEventsTotal(),
        "legacy MapComplete migration marks the generated encounter settled");
    expect(target.mapObjective() == "Choose Reward",
        "MapComplete load starts at the reward selection stage");
    Input input;
    pressKey(target, input, sf::Keyboard::Key::Escape);
    expect(target.state() == GameState::Paused,
        "MapComplete can be paused without mutating settlement state");
    expect(target.saveRun(path),
        "paused MapComplete saves its settlement resume context");
    GameWorld pausedLoadTarget(9103);
    expect(pausedLoadTarget.loadRun(path)
            && pausedLoadTarget.state() == GameState::MapComplete
            && pausedLoadTarget.mapObjective() == "Choose Reward",
        "loading a paused MapComplete resumes the settlement phase");
    pressKey(target, input, sf::Keyboard::Key::Escape);
    expect(target.state() == GameState::MapComplete,
        "MapComplete resumes after Pause");

    const auto partialPath = std::filesystem::temp_directory_path()
        / "plane_fight_partial_settlement_save_test.bin";
    std::filesystem::remove(partialPath);
    GameWorld partial(9104);
    Input partialInput;
    expect(saveAsMapCompleteFixture(partialPath, partial, 0, false),
        "partial settlement fixture reaches MapComplete");
    pressKey(partial, partialInput, sf::Keyboard::Key::Num1);
    expect(partial.mapRewardChosen() && !partial.nextMapOptionChosen(),
        "partial settlement records only the reward choice");
    expect(partial.saveRun(partialPath),
        "partial settlement saves between reward and map choice");
    GameWorld partialLoadTarget(9105);
    expect(partialLoadTarget.loadRun(partialPath)
            && partialLoadTarget.mapRewardChosen()
            && !partialLoadTarget.nextMapOptionChosen()
            && partialLoadTarget.mapLevel() == 1,
        "partial settlement load preserves its choice stage");
    const int partialLevel = partialLoadTarget.mapLevel();
    Input partialLoadInput;
    pressKey(partialLoadTarget, partialLoadInput, sf::Keyboard::Key::E);
    expect(partialLoadTarget.state() == GameState::MapComplete
            && partialLoadTarget.mapLevel() == partialLevel,
        "partial settlement still blocks E before map choice");
    pressKey(partialLoadTarget, partialLoadInput, sf::Keyboard::Key::Num1);
    pressKey(partialLoadTarget, partialLoadInput, sf::Keyboard::Key::E);
    expect(partialLoadTarget.state() == GameState::Playing
            && partialLoadTarget.mapLevel() == partialLevel + 1,
        "partial settlement can finish map selection after load");
    std::filesystem::remove(partialPath);
    std::filesystem::remove(path);
}

bool saveAsMapCompleteFixture(
    const std::filesystem::path& path,
    GameWorld& world,
    int sequence,
    bool addGroundDrop
) {
    std::string error;
    SaveData data;
    if (!world.saveRun(path) || !SaveService::load(path, data, &error)) {
        return false;
    }

    data.state = SavedRunState::MapComplete;
    RandomService rewardRandom(data.runSeed + static_cast<std::uint64_t>(sequence));
    data.mapRewardOptions = MapRewardLibrary::generateOptions(
        data.unlockedSkills,
        data.unlockedSupports,
        rewardRandom
    );
    data.nextMapOptions = MapOptionLibrary::generateOptions(data.mapLevel + 1);
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;

    if (addGroundDrop) {
        LootGenerator lootGenerator;
        RandomService lootRandom(data.runSeed + 1000U + static_cast<std::uint64_t>(sequence));
        data.droppedItems.push_back({data.player.position, lootGenerator.generate(
            data.mapLevel, lootRandom
        )});
    }

    return SaveService::save(path, data, &error) && world.loadRun(path);
}

void testContinuousMapProgression() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_progression_test.bin";
    std::filesystem::remove(path);

    GameWorld world(15001);
    std::string error;
    expect(world.saveRun(path), "progression fixture saves the initial run");
    SaveData initial;
    expect(SaveService::load(path, initial, &error),
        "progression fixture loads the initial run");
    LootGenerator lootGenerator;
    RandomService lootRandom(15002);
    const Item carriedItem = lootGenerator.generate(1, lootRandom);
    initial.inventory.push_back(carriedItem);
    initial.stash.push_back(carriedItem);
    expect(SaveService::save(path, initial, &error) && world.loadRun(path),
        "progression fixture restores carried Inventory and Stash items");

    const std::size_t inventorySize = world.inventory().size();
    const std::size_t stashSize = world.stash().size();
    const int initialLevel = world.player().level();

    for (int mapIndex = 0; mapIndex < 5; ++mapIndex) {
        const int levelBefore = world.mapLevel();
        expect(saveAsMapCompleteFixture(path, world, mapIndex, true),
            "fixture enters MapComplete for map " + std::to_string(levelBefore));
        expect(world.state() == GameState::MapComplete && world.droppedItems().size() == 1,
            "MapComplete keeps the ground drop before entering the next map");

        Input input;
        pressKey(world, input, sf::Keyboard::Key::E);
        expect(world.state() == GameState::MapComplete && world.mapLevel() == levelBefore,
            "E cannot enter before reward and map choices are complete");

        pressKey(world, input, sf::Keyboard::Key::Num1);
        expect(world.mapRewardChosen(), "reward choice is accepted on MapComplete");
        pressKey(world, input, sf::Keyboard::Key::E);
        expect(world.state() == GameState::MapComplete && !world.nextMapOptionChosen(),
            "E cannot enter before a next map is selected");
        pressKey(world, input, sf::Keyboard::Key::Num1);
        expect(world.nextMapOptionChosen(), "next map choice is accepted on MapComplete");
        pressKey(world, input, sf::Keyboard::Key::E);
        expect(world.state() == GameState::Playing
                && world.mapLevel() == levelBefore + 1,
            "E enters the selected next map");
        expect(world.inventory().size() == inventorySize
                && world.stash().size() == stashSize
                && world.player().level() >= initialLevel
                && world.droppedItems().empty()
                && world.mapEventsCompleted() == 0
                && world.mapEventsTotal() == 4,
            "next map keeps progression and resets transient map state");
    }

    expect(world.mapLevel() == 6, "five transitions reach map level 6");
    std::filesystem::remove(path);
}

void testStoredMapDeviceFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_stored_map_device_test.bin";
    std::filesystem::remove(path);

    GameWorld source(15501);
    expect(source.saveRun(path), "stored map fixture saves a base run");
    SaveData data;
    std::string error;
    expect(SaveService::load(path, data, &error),
        "stored map fixture loads its base save");
    data.state = SavedRunState::MapComplete;
    for (auto& event : data.mapEvents) {
        event.triggered = true;
        event.completed = true;
    }
    RandomService rewardRandom(data.runSeed + 77U);
    data.mapRewardOptions = MapRewardLibrary::generateOptions(
        data.unlockedSkills,
        data.unlockedSupports,
        rewardRandom
    );
    data.selectedMapRewardOption = -1;
    data.mapRewardChosen = false;
    data.selectedNextMapOption = -1;
    data.nextMapOptionChosen = false;
    const auto mapOption = MapOptionLibrary::generateOptions(2)[2];
    data.mapItems.push_back(MapItemLibrary::fromOption(mapOption, 2, 1));
    data.completedMapIds.insert(MapItemLibrary::fromOption(
        data.currentMapOption, data.mapLevel, data.mapLayoutIndex
    ).id);
    data.selectedMapItemIndex = -1;
    expect(SaveService::save(path, data, &error) && source.loadRun(path),
        "stored map fixture restores a settlement with a held map");
    const MapModifier baseModifier = MapItemLibrary::modifierFor(data.currentMapOption);
    expect(source.atlasPoints() == 1
            && source.mapModifier().itemQuantityMultiplier
                > baseModifier.itemQuantityMultiplier
            && source.mapModifier().itemRarityMultiplier
                > baseModifier.itemRarityMultiplier,
        "loaded Atlas completion applies quantity and rarity bonuses to the map");

    Input input;
    pressKey(source, input, sf::Keyboard::Key::T);
    expect(source.atlasPanelOpen()
            && source.atlasAvailablePoints() == 1
            && source.canAllocateAtlasNode(0)
            && !source.canAllocateAtlasNode(1),
        "settlement Atlas panel exposes an unspent point and root node");
    pressKey(source, input, sf::Keyboard::Key::Num1);
    expect(source.isAtlasNodeAllocated(0)
            && source.atlasAvailablePoints() == 0,
        "settlement Atlas panel allocates a node with number input");
    pressKey(source, input, sf::Keyboard::Key::T);
    expect(!source.atlasPanelOpen(),
        "T closes the Atlas panel without consuming map choice input");
    expect(source.saveRun(path), "allocated Atlas state saves from settlement");
    GameWorld persisted(15502);
    expect(persisted.loadRun(path)
            && persisted.isAtlasNodeAllocated(0)
            && persisted.atlasAvailablePoints() == 0,
        "allocated Atlas state survives a world save/load round-trip");
    pressKey(source, input, sf::Keyboard::Key::Num1);
    expect(source.mapRewardChosen(), "stored map flow still chooses reward first");
    pressKey(source, input, sf::Keyboard::Key::M);
    expect(source.mapDeviceOpen()
            && source.mapObjective() == "Choose Stored Map",
        "M opens the stored map device after reward selection");
    pressKey(source, input, sf::Keyboard::Key::Tab);
    expect(source.selectedMapItemIndex() == 0,
        "Tab selects the first held map in the map device");
    pressKey(source, input, sf::Keyboard::Key::E);
    expect(source.state() == GameState::Playing
            && source.mapLevel() == 2
            && source.map().layoutIndex() == 1
            && source.mapItems().empty(),
        "E consumes the selected map item and enters its stored layout");
    expect(source.completedMapCount() == 1 && !source.currentMapCompleted(),
        "atlas progress persists while the newly entered map is incomplete");

    std::filesystem::remove(path);
}

void testItemBaseLevelRequirementWorldFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_item_base_requirement_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(16501);
    SaveData data;
    std::string error;
    const Item starterWeapon = makeBaseItem("weapon.rustbound-blade");
    const Item gatedWeapon = makeBaseItem("weapon.warhammer");
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "item requirement fixture starts from a valid save");
    data.player = world.player().saveState();
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] = starterWeapon;
    data.inventory.clear();
    data.inventory.push_back(gatedWeapon);
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "item requirement fixture restores starter and gated weapons");

    Input input;
    pressKey(world, input, sf::Keyboard::Key::Tab);
    const int selectedBefore = world.selectedInventoryIndex();
    const int hpBefore = world.player().hp();
    const auto& equippedBefore = world.player().equipment().itemInSlot(EquipmentSlot::Weapon);
    expect(selectedBefore == 0 && equippedBefore
            && equippedBefore->baseId == "weapon.rustbound-blade",
        "level requirement fixture selects the gated item without changing equipment");

    pressKey(world, input, sf::Keyboard::Key::Num1);
    const auto& blockedEquipment = world.player().equipment().itemInSlot(EquipmentSlot::Weapon);
    expect(world.inventory().size() == 1
            && world.inventory().items().front().baseId == "weapon.warhammer"
            && world.selectedInventoryIndex() == selectedBefore
            && blockedEquipment && blockedEquipment->baseId == "weapon.rustbound-blade"
            && world.player().hp() == hpBefore,
        "under-level equip preserves the item, selection, equipment, and HP");
    expect(world.eventStatusMessage() == "Requires level 3"
            && world.eventStatusTimeRemaining() > 0.0f,
        "under-level equip reports the required level");

    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "level requirement fixture saves the blocked attempt");
    data.player.level = 3;
    data.player.exp = 0;
    data.player.expToNextLevel = Config::BaseExpToLevel + 4;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "level requirement fixture raises the saved player level");

    pressKey(world, input, sf::Keyboard::Key::Num1);
    const auto& equippedGated = world.player().equipment().itemInSlot(EquipmentSlot::Weapon);
    expect(equippedGated && equippedGated->baseId == "weapon.warhammer"
            && world.inventory().size() == 1
            && world.inventory().items().front().baseId == "weapon.rustbound-blade",
        "at-level equip succeeds and returns the old weapon to Inventory");
    const auto* gatedBase = ItemBaseLibrary::find("weapon.warhammer");
    expect(gatedBase != nullptr && gatedBase->buildTheme == ItemBuildTheme::Area
            && std::abs(world.player().stats().areaDamageMultiplier
                - gatedBase->implicitStats.areaDamageMultiplier) < 0.0001f,
        "equipped Base theme and implicit stats affect the real Player build");

    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "item requirement fixture saves a valid equipped state");
    data.inventory.front().baseId = "missing.base";
    expect(SaveService::save(path, data, &error) && !world.loadRun(path),
        "unknown Item Base is rejected without loading");
    const auto& preservedEquipment = world.player().equipment().itemInSlot(EquipmentSlot::Weapon);
    expect(preservedEquipment && preservedEquipment->baseId == "weapon.warhammer"
            && world.inventory().size() == 1
            && world.inventory().items().front().baseId == "weapon.rustbound-blade",
        "rejected invalid Item Base leaves the current run untouched");

    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "item requirement fixture reloads before implicit validation");
    data.inventory.front().implicitStats.damageMultiplier = 1.99f;
    expect(SaveService::save(path, data, &error) && !world.loadRun(path),
        "implicit stats that disagree with the Base are rejected");
    const auto& preservedAfterImplicitFailure = world.player().equipment().itemInSlot(
        EquipmentSlot::Weapon);
    expect(preservedAfterImplicitFailure
            && preservedAfterImplicitFailure->baseId == "weapon.warhammer"
            && world.inventory().size() == 1
            && world.inventory().items().front().baseId == "weapon.rustbound-blade",
        "rejected implicit stats leave the current run untouched");

    std::filesystem::remove(path);
}

void testFiveMapRealBossProgression() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_five_map_real_progression_test.bin";
    std::filesystem::remove(path);

    GameWorld world(15501);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "real progression fixture starts from a valid run save");

    LootGenerator lootGenerator;
    RandomService lootRandom(15502);
    data.inventory.push_back(lootGenerator.generate(1, lootRandom));
    data.stash.push_back(lootGenerator.generate(1, lootRandom));
    data.unlockedSupports.insert("Pierce");
    data.skillBar.supports[static_cast<std::size_t>(SkillSlot::Primary)][0] = "Pierce";
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.moveSpeedMultiplier = 6.0f;
    data.player.upgradeStats.damageMultiplier = 100.0f;
    data.player.upgradeStats.projectileDamageMultiplier = 100.0f;
    data.player.upgradeStats.areaDamageMultiplier = 1000.0f;
    data.player.upgradeStats.areaRadiusMultiplier = 2.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "real progression fixture restores durable inventory and combat tolerance");

    const std::size_t initialOwnedItems = world.inventory().size() + world.stash().size();
    const int initialLevel = world.player().level();
    const Stats initialEquipmentStats = world.player().equipment().combinedStats();
    const auto initialPassives = world.player().passiveTree().allocatedNodes();
    const auto* initialPrimarySupport = world.skillBar().supportAt(SkillSlot::Primary, 0);
    const std::string initialPrimarySupportName = initialPrimarySupport
        ? initialPrimarySupport->name : "";
    std::vector<bool> unlockedSkills;
    for (const auto& skill : SkillLibrary::all()) {
        unlockedSkills.push_back(world.isSkillUnlocked(skill.name));
    }
    std::vector<bool> unlockedSupports;
    for (const auto& support : SupportLibrary::all()) {
        unlockedSupports.push_back(world.isSupportUnlocked(support.name));
    }

    Input input;
    for (int mapIndex = 0; mapIndex < 5; ++mapIndex) {
        const int expectedMapLevel = mapIndex + 1;
        expect(world.state() == GameState::Playing
                && world.mapLevel() == expectedMapLevel,
            "real progression starts map " + std::to_string(expectedMapLevel));
        expect(world.mapEventsCompleted() == 0 && world.droppedItems().empty(),
            "map " + std::to_string(expectedMapLevel) + " starts without transient map state");

        expect(moveToBoss(world, input),
            "real movement reaches Boss Arena on map " + std::to_string(expectedMapLevel));
        if (world.state() != GameState::Playing || !world.map().bossTriggered()) {
            break;
        }

        expect(defeatBossWithAreaSkill(world, input),
            "real Area skill defeats Boss on map " + std::to_string(expectedMapLevel));
        expect(world.state() == GameState::MapComplete
                && world.map().bossDefeated()
                && !world.droppedItems().empty(),
            "map " + std::to_string(expectedMapLevel)
                + " enters settlement with Boss loot on the ground");
        if (world.state() != GameState::MapComplete) {
            break;
        }

        const int pickedUpBefore = world.mapItemsPickedUp();
        pressKey(world, input, sf::Keyboard::Key::F);
        expect(world.mapItemsPickedUp() == pickedUpBefore + 1,
            "settlement picks up one nearest Boss drop on map "
                + std::to_string(expectedMapLevel));
        const std::size_t ownedAfterPickup = world.inventory().size() + world.stash().size();
        expect(ownedAfterPickup == initialOwnedItems + static_cast<std::size_t>(mapIndex + 1),
            "Boss loot becomes durable inventory ownership on map "
                + std::to_string(expectedMapLevel));

        pressKey(world, input, sf::Keyboard::Key::Tab);
        const std::size_t stashBeforeMove = world.stash().size();
        pressKey(world, input, sf::Keyboard::Key::I);
        expect(world.stash().size() == stashBeforeMove + 1,
            "settlement can move selected Boss loot into Stash on map "
                + std::to_string(expectedMapLevel));

        pressKey(world, input, sf::Keyboard::Key::Num1);
        expect(world.mapRewardChosen(),
            "Boss settlement reward is chosen on map " + std::to_string(expectedMapLevel));
        pressKey(world, input, sf::Keyboard::Key::Num1);
        expect(world.nextMapOptionChosen(),
            "next map option is chosen on map " + std::to_string(expectedMapLevel));
        pressKey(world, input, sf::Keyboard::Key::E);
        expect(world.state() == GameState::Playing
                && world.mapLevel() == expectedMapLevel + 1,
            "E enters map " + std::to_string(expectedMapLevel + 1)
                + " after real Boss settlement");
        expect(world.droppedItems().empty() && world.mapEventsCompleted() == 0
                && world.mapEventsTotal() == 4,
            "map " + std::to_string(expectedMapLevel + 1)
                + " resets ground drops and map events");
        expect(world.inventory().size() + world.stash().size()
                == initialOwnedItems + static_cast<std::size_t>(mapIndex + 1),
            "map transition preserves owned Inventory/Stash items");
        expect(world.player().level() >= initialLevel
                && statsEqual(world.player().equipment().combinedStats(), initialEquipmentStats)
                && world.player().passiveTree().allocatedNodes() == initialPassives,
            "map transition preserves level, equipment, and passive allocation");
        for (std::size_t index = 0; index < unlockedSkills.size(); ++index) {
            if (!unlockedSkills[index]) {
                continue;
            }
            expect(world.isSkillUnlocked(SkillLibrary::all()[index].name),
                "map transition preserves unlocked skills");
        }
        for (std::size_t index = 0; index < unlockedSupports.size(); ++index) {
            if (!unlockedSupports[index]) {
                continue;
            }
            expect(world.isSupportUnlocked(SupportLibrary::all()[index].name),
                "map transition preserves unlocked supports");
        }
        const auto* primarySupport = world.skillBar().supportAt(SkillSlot::Primary, 0);
        expect(primarySupport != nullptr && primarySupport->name == initialPrimarySupportName,
            "map transition preserves equipped support links");
        for (std::size_t index = 0; index < unlockedSkills.size(); ++index) {
            unlockedSkills[index] = world.isSkillUnlocked(SkillLibrary::all()[index].name);
        }
        for (std::size_t index = 0; index < unlockedSupports.size(); ++index) {
            unlockedSupports[index] = world.isSupportUnlocked(SupportLibrary::all()[index].name);
        }
    }

    expect(world.mapLevel() == 6,
        "real Boss progression completes five maps");
    std::filesystem::remove(path);
}

void testBossSpawnUsesMapScaling() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_boss_scaling_path_test.bin";
    std::filesystem::remove(path);

    GameWorld world(15601);
    SaveData baseData;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, baseData, &error),
        "Boss scaling fixture starts from a valid run save");

    int previousBossHp = 0;
    int previousBossDamage = 0;
    for (int mapLevel = 1; mapLevel <= 5; ++mapLevel) {
        SaveData data = baseData;
        const MapOption mapOption = mapLevel == 1
            ? MapOptionLibrary::defaultOption()
            : MapOptionLibrary::generateOptions(mapLevel)[0];
        data.mapLevel = mapLevel;
        data.currentMapOption = mapOption;
        data.mapTemplateIndex = mapOption.templateIndex;
        data.mapLayoutIndex = MapLayoutLibrary::variantForMapLevel(mapLevel);
        data.state = SavedRunState::Playing;
        data.mapRewardChosen = false;
        data.nextMapOptionChosen = false;
        data.selectedMapRewardOption = -1;
        data.selectedNextMapOption = -1;
        data.player.hp = 10000;
        data.player.upgradeStats.maxHp = 10000;
        data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
        data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
        data.player.mana = Config::PlayerMaxMana;
        expect(SaveService::save(path, data, &error) && world.loadRun(path),
            "Boss scaling fixture loads map " + std::to_string(mapLevel));
        const MapInstance expectedMap(
            mapLevel, mapOption.templateIndex, data.mapLayoutIndex
        );
        const int expectedBossIndex = expectedMap.encounterDefinition().bossDefinitionIndex >= 0
            ? expectedMap.encounterDefinition().bossDefinitionIndex
            : mapOption.templateIndex;
        expect(world.bossDefinition().name == BossLibrary::forIndex(expectedBossIndex).name,
            "runtime Boss follows the selected map theme on map "
                + std::to_string(mapLevel));

        Input input;
        expect(moveToBoss(world, input),
            "real movement reaches the Boss on map " + std::to_string(mapLevel));
        const auto bossIt = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
        const int expectedHp = MapScaling::bossHp(
            mapLevel, mapOption.modifier, world.bossDefinition()
        );
        const int expectedDamage = MapScaling::bossContactDamage(
            mapLevel, mapOption.modifier, world.bossDefinition()
        );
        expect(bossIt != world.enemies().end()
                && bossIt->maxHp() == expectedHp
                && bossIt->contactDamage() == expectedDamage,
            "runtime Boss uses the shared map scaling profile on map "
                + std::to_string(mapLevel));
        if (bossIt != world.enemies().end()) {
            expect(bossIt->maxHp() >= previousBossHp,
                "runtime Boss HP does not decrease at map "
                    + std::to_string(mapLevel));
            expect(bossIt->contactDamage() >= previousBossDamage,
                "runtime Boss contact damage does not decrease at map "
                    + std::to_string(mapLevel));
            previousBossHp = bossIt->maxHp();
            previousBossDamage = bossIt->contactDamage();
        }
    }

    std::filesystem::remove(path);
}

void testGameOverRestartBoundary() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_game_over_restart_test.bin";
    std::filesystem::remove(path);

    GameWorld world(16501);
    expect(world.saveRun(path), "GameOver fixture saves an initial run");
    SaveData data;
    std::string error;
    expect(SaveService::load(path, data, &error),
        "GameOver fixture loads its initial save");
    data.player.hp = 1;
    data.player.upgradeStats.incomingDamageMultiplier = 100.0f;
    data.player.upgradeStats.moveSpeedMultiplier = 6.0f;
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "GameOver fixture restores a fragile player");

    Input input;
    input.handleKeyPressed(sf::Keyboard::Key::D);
    for (int frame = 0; frame < 240 && world.state() == GameState::Playing; ++frame) {
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::D);
    expect(world.state() == GameState::GameOver,
        "real combat damage reaches GameOver");

    const std::uint64_t defeatedRunSeed = world.runSeed();
    pressKey(world, input, sf::Keyboard::Key::D);
    expect(world.state() == GameState::GameOver,
        "GameOver ignores gameplay movement input");
    pressKey(world, input, sf::Keyboard::Key::R);
    expect(world.state() == GameState::Playing
            && world.runSeed() != defeatedRunSeed
            && world.mapLevel() == 1
            && world.inventory().size() == 0
            && world.stash().size() == 0
            && world.droppedItems().empty()
            && world.mapEventsCompleted() == 0
            && world.mapEventsTotal() == 4
            && world.player().level() == 1,
        "Restart creates a clean new run after GameOver");
    std::filesystem::remove(path);
}

void testInvalidProgressionSaveDoesNotMutate() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_invalid_progression_test.bin";
    std::filesystem::remove(path);

    GameWorld target(16001);
    Input input;
    expect(saveAsMapCompleteFixture(path, target, 0, false),
        "invalid progression fixture reaches MapComplete");
    const std::uint64_t originalSeed = target.runSeed();
    const int originalLevel = target.mapLevel();

    pressKey(target, input, sf::Keyboard::Key::E);
    expect(target.state() == GameState::MapComplete && target.mapLevel() == originalLevel,
        "MapComplete rejects entering without selections");

    expect(target.saveRun(path), "invalid phase fixture saves a valid current run");
    SaveData invalid;
    std::string error;
    expect(SaveService::load(path, invalid, &error),
        "invalid phase fixture can be edited");
    invalid.state = SavedRunState::MapComplete;
    invalid.mapRewardChosen = false;
    invalid.selectedMapRewardOption = -1;
    invalid.nextMapOptionChosen = true;
    invalid.selectedNextMapOption = 0;
    expect(SaveService::save(path, invalid, &error),
        "invalid phase fixture is written");
    expect(!target.loadRun(path),
        "load rejects a next-map choice without a reward choice");
    expect(target.runSeed() == originalSeed && target.mapLevel() == originalLevel
            && target.state() == GameState::MapComplete,
        "rejected progression save leaves the current run untouched");

    std::filesystem::remove(path);
}

void testCombatFeedbackAndDeathClaim() {
    Enemy enemy({0.0f, 0.0f}, 3, 1);
    expect(enemy.maxHp() == 3, "enemy keeps its configured max HP");
    expect(enemy.takeDamage(8) == 3 && enemy.hp() == 0,
        "enemy damage reports only actual damage and clamps HP at zero");
    expect(enemy.takeDamage(1) == 0,
        "dead enemy cannot receive additional damage");
    expect(enemy.claimKillReward(), "dead enemy grants its kill claim once");
    expect(!enemy.claimKillReward(), "dead enemy cannot grant its kill claim twice");

    GameWorld world(17001);
    Input input;
    advanceIntoTheField(world, input);
    expect(!world.enemies().empty(), "combat feedback fixture reaches a live enemy");
    if (world.enemies().empty()) {
        return;
    }

    const Vector2 target = world.enemies().front().position();
    const Vector2 camera = world.cameraTopLeft();
    const sf::Vector2i screenTarget(
        static_cast<int>(std::lround(target.x - camera.x)),
        static_cast<int>(std::lround(target.y - camera.y))
    );
    input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
    world.update(0.05f, input);
    resolvePendingSkillEffects(world, input);

    expect(!world.combatFeedback().empty(),
        "a real player area skill creates combat feedback on hit");
    const auto feedbackIt = std::find_if(
        world.combatFeedback().rbegin(),
        world.combatFeedback().rend(),
        [](const CombatFeedback& feedback) {
            return feedback.type == CombatFeedbackType::Damage;
        }
    );
    if (feedbackIt != world.combatFeedback().rend()) {
        const auto& feedback = *feedbackIt;
        expect(feedback.damage > 0 && !feedback.source.empty()
                && feedback.timeRemaining > 0.0f
                && feedback.type == CombatFeedbackType::Damage,
            "combat feedback exposes positive damage, source, and lifetime");
    }

    world.update(Config::CombatFeedbackDuration + 0.05f, input);
    expect(world.combatFeedback().empty(),
        "expired combat feedback is removed from the world");
}

void testSkillFailureFeedback() {
    GameWorld cooldownWorld(17501);
    Input cooldownInput;
    cooldownInput.handleMousePressed(sf::Mouse::Button::Left, {700, 300});
    cooldownWorld.update(0.05f, cooldownInput);
    const std::size_t projectileCountAfterCast = cooldownWorld.projectiles().size();
    const float cooldownProgress = cooldownWorld.skillBar().cooldownProgress(SkillSlot::Primary);
    cooldownWorld.update(0.05f, cooldownInput);
    cooldownInput.handleMouseReleased(sf::Mouse::Button::Left, {700, 300});

    const std::size_t cooldownFailureCount = static_cast<std::size_t>(std::count_if(
        cooldownWorld.combatFeedback().begin(),
        cooldownWorld.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.type == CombatFeedbackType::SkillRejected
                && feedback.source.find("Skill cooling down:") == 0;
        }
    ));
    expect(projectileCountAfterCast > 0 && cooldownProgress < 1.0f,
        "first skill cast consumes the Primary cooldown");
    expect(cooldownFailureCount == 1,
        "cooldown failure creates one throttled SkillRejected feedback");
    expect(cooldownWorld.projectiles().size() == projectileCountAfterCast,
        "cooldown failure does not create another projectile");

    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_skill_failure_feedback_test.bin";
    std::filesystem::remove(path);
    GameWorld manaWorld(17502);
    SaveData data;
    std::string error;
    expect(manaWorld.saveRun(path) && SaveService::load(path, data, &error),
        "Mana failure fixture starts from a valid save");
    data.player.mana = 0.0f;
    expect(SaveService::save(path, data, &error) && manaWorld.loadRun(path),
        "Mana failure fixture restores an empty Mana pool");

    Input manaInput;
    const float secondaryCooldownBefore = manaWorld.skillBar().cooldownProgress(
        SkillSlot::Secondary
    );
    manaInput.handleMousePressed(sf::Mouse::Button::Right, {700, 300});
    manaWorld.update(0.05f, manaInput);
    const float secondaryCooldownAfter = manaWorld.skillBar().cooldownProgress(
        SkillSlot::Secondary
    );
    const bool manaRejected = std::any_of(
        manaWorld.combatFeedback().begin(),
        manaWorld.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.type == CombatFeedbackType::SkillRejected
                && feedback.source.find("Not enough Mana:") == 0;
        }
    );
    expect(manaRejected, "Mana failure creates a typed SkillRejected feedback");
    const float expectedManaAfterRegen = Config::PlayerManaRegenPerSecond * 0.05f;
    expect(std::abs(manaWorld.player().mana() - expectedManaAfterRegen) < 0.001f,
        "Mana failure preserves the Mana pool after normal regeneration");
    expect(secondaryCooldownBefore > 0.99f && secondaryCooldownAfter > 0.99f,
        "Mana failure preserves the Secondary cooldown");
    expect(manaWorld.projectiles().empty(),
        "Mana failure creates no projectile side effect");

    std::filesystem::remove(path);
}

void testDelayedSkillEffects() {
    GameWorld world(17601);
    Input input;
    const Vector2 target = world.player().position();
    const Vector2 camera = world.cameraTopLeft();
    const sf::Vector2i screenTarget(
        static_cast<int>(std::lround(target.x - camera.x)),
        static_cast<int>(std::lround(target.y - camera.y))
    );

    const float manaBeforeCast = world.player().mana();
    input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
    world.update(0.05f, input);
    expect(world.pendingSkillEffects().size() == 1
            && !world.pendingSkillEffects().front().impacted
            && world.pendingSkillEffects().front().source == "Meteor"
            && world.pendingSkillEffects().front().groundHazard.source
                == "Meteor Burning Ground",
        "Meteor queues a named pending impact instead of resolving immediately");
    expect(std::abs(manaBeforeCast - world.player().mana()
            - skillManaCost(
                SkillLibrary::meteor(),
                world.player().stats(),
                world.skillBar().supportDefinitionsFor(SkillLibrary::meteor())
            )) < 0.0001f,
        "real Meteor cast consumes the unified effective Mana cost");
    input.handleMouseReleased(sf::Mouse::Button::Right, screenTarget);

    for (int frame = 0; frame < 11; ++frame) {
        world.update(0.05f, input);
    }
    expect(!world.pendingSkillEffects().empty()
            && world.pendingSkillEffects().front().impacted
            && world.pendingSkillEffects().front().impactDurationRemaining > 0.0f,
        "Meteor resolves after its telegraph and exposes the impact effect");
    expect(std::any_of(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Meteor Burning Ground"
                        && hazard.definition().target == GroundHazardTarget::Enemies;
                }
            ),
        "Meteor impact creates its enemy-only burning ground");

    for (int frame = 0; frame < 12; ++frame) {
        world.update(0.05f, input);
    }
    expect(world.pendingSkillEffects().empty(),
        "resolved pending skill effects expire after their impact duration");
}

void testUtilitySkillDelivery() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_utility_skill_delivery_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17602);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Utility delivery fixture starts from a valid run save");

    data.unlockedSkills.insert("Bladestorm");
    data.unlockedSkills.insert("Aftershock");
    data.skillBar.skills[utilityIndex] = "Bladestorm";
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Utility delivery fixture equips Bladestorm");

    Input input;
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    expect(world.pendingSkillEffects().size() == 1
            && world.pendingSkillEffects().front().impactsRemaining
                == Config::BladestormHitCount
            && !world.pendingSkillEffects().front().impacted,
        "Bladestorm queues one multi-hit area sequence");

    bool hitProgressObserved = false;
    for (int frame = 0; frame < 18; ++frame) {
        world.update(0.05f, input);
        if (!world.pendingSkillEffects().empty()
            && world.pendingSkillEffects().front().impactsRemaining
                < Config::BladestormHitCount) {
            hitProgressObserved = true;
        }
    }
    expect(hitProgressObserved,
        "Bladestorm advances through its configured hit cadence");

    for (int frame = 0; frame < 30; ++frame) {
        world.update(0.05f, input);
    }
    expect(world.pendingSkillEffects().empty(),
        "Bladestorm sequence expires after its final hit");

    data.skillBar.skills[utilityIndex] = "Aftershock";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Utility delivery fixture equips Aftershock");
    Input aftershockInput;
    aftershockInput.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, aftershockInput);
    expect(world.pendingSkillEffects().size() == 1
            && world.pendingSkillEffects().front().impactsRemaining == 1
            && !world.pendingSkillEffects().front().impacted
            && world.pendingSkillEffects().front().delayRemaining > 0.0f,
        "Aftershock queues a delayed secondary blast");

    for (int frame = 0; frame < 14; ++frame) {
        world.update(0.05f, aftershockInput);
    }
    expect(!world.pendingSkillEffects().empty()
            && world.pendingSkillEffects().front().impacted,
        "Aftershock resolves after its telegraph window");

    std::filesystem::remove(path);
}

void testPlayerMinionWorldFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_player_minion_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17604);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Player minion fixture starts from a valid run save");

    data.unlockedSkills.insert("Summon Wisp");
    data.unlockedSupports.insert("Minion Mastery");
    data.skillBar.skills[utilityIndex] = "Summon Wisp";
    data.skillBar.supports[utilityIndex] = {"Minion Mastery", ""};
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Player minion fixture restores Summon Wisp and Minion Mastery");

    Input input;
    advanceIntoTheField(world, input);
    auto targetIt = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [](const Enemy& enemy) { return !enemy.isBoss() && !enemy.isDead(); }
    );
    expect(targetIt != world.enemies().end(),
        "Player minion fixture reaches a live field enemy");
    if (targetIt == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    Enemy& target = const_cast<Enemy&>(*targetIt);
    const int targetId = target.id();
    const Vector2 targetPosition = world.player().position() + Vector2(120.0f, 0.0f);
    target.moveBy(targetPosition - target.position(), world.map());
    for (auto& enemy : const_cast<std::vector<Enemy>&>(world.enemies())) {
        if (enemy.id() != targetId && !enemy.isBoss()) {
            enemy.moveBy(world.map().bossCenter() - enemy.position(), world.map());
        }
    }

    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);
    expect(world.playerMinions().size() == 3,
        "Summon Wisp creates the base count plus Minion Mastery");
    expect(std::any_of(
                world.combatFeedback().begin(), world.combatFeedback().end(),
                [](const CombatFeedback& feedback) {
                    return feedback.source == "Summon Wisp x3"
                        && feedback.type == CombatFeedbackType::Status;
                }
            ),
        "Summon Wisp reports its spawned minion count");

    const int targetHpBefore = target.hp();
    bool wispAttackObserved = false;
    for (int frame = 0; frame < 24; ++frame) {
        world.update(0.05f, input);
        wispAttackObserved = wispAttackObserved || std::any_of(
            world.combatFeedback().begin(), world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.source == "Wisp"
                    && feedback.type == CombatFeedbackType::Damage;
            }
        );
    }
    const auto targetAfter = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [targetId](const Enemy& enemy) { return enemy.id() == targetId; }
    );
    expect(wispAttackObserved,
        "live Wisp minions attack through the real GameWorld loop");
    expect(targetAfter == world.enemies().end() || targetAfter->hp() < targetHpBefore,
        "Wisp attacks reduce a live field enemy's HP");

    expect(world.saveRun(path) && world.loadRun(path) && world.playerMinions().empty(),
        "loading a run clears non-persistent player minions");

    std::filesystem::remove(path);
}

void testPulseShockFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_pulse_shock_flow_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17603);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Pulse Shock fixture starts from a valid run save");

    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.skillBar.skills[utilityIndex] = "Pulse";
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Pulse Shock fixture restores a Boss-ready combat setup");

    Input input;
    expect(moveToBoss(world, input),
        "Pulse Shock fixture reaches the Boss Arena through real movement");
    auto findBoss = [&world]() {
        return std::find_if(
            world.enemies().begin(), world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
    };
    expect(world.map().bossTriggered() && findBoss() != world.enemies().end(),
        "Pulse Shock fixture awakens a live Boss through the normal map flow");
    if (!world.map().bossTriggered() || findBoss() == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const auto& pulse = world.skillBar().definition(SkillSlot::Utility);
    const auto supports = world.skillBar().supportDefinitionsFor(pulse);
    const int expectedRawDamage = skillDamage(pulse, world.player().stats(), supports);
    const int expectedFirstDamage = damageAfterResistance(
        expectedRawDamage,
        DamageType::Lightning,
        world.bossDefinition().fireResistance,
        world.bossDefinition().coldResistance,
        world.bossDefinition().lightningResistance,
        world.bossDefinition().poisonResistance
    );
    const std::size_t firstFeedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    int firstPulseDamage = 0;
    bool shockFeedbackObserved = false;
    for (std::size_t index = firstFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.source == pulse.name && feedback.type == CombatFeedbackType::Damage) {
            firstPulseDamage += feedback.damage;
        }
        shockFeedbackObserved = shockFeedbackObserved
            || (feedback.source == "Shock" && feedback.type == CombatFeedbackType::Status);
    }
    auto boss = findBoss();
    expect(firstPulseDamage == expectedFirstDamage,
        "Pulse first hit uses Lightning resistance through the real damage path");
    expect(boss != world.enemies().end() && boss->isShocked() && shockFeedbackObserved,
        "Pulse applies Shock and emits typed status feedback");

    world.update(2.10f, input);
    const std::size_t secondFeedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    int secondPulseDamage = 0;
    for (std::size_t index = secondFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.source == pulse.name && feedback.type == CombatFeedbackType::Damage) {
            secondPulseDamage += feedback.damage;
        }
    }
    expect(secondPulseDamage > firstPulseDamage,
        "Pulse benefits from its active Shock on the next hit");

    std::filesystem::remove(path);
}

void testRendingVolleyBleedFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_rending_volley_bleed_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17605);
    SaveData data;
    std::string error;
    const auto primaryIndex = static_cast<std::size_t>(SkillSlot::Primary);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Rending Volley fixture starts from a valid run save");

    data.unlockedSkills.insert("Rending Volley");
    data.unlockedSupports.insert("Bloodletting");
    data.skillBar.skills[primaryIndex] = "Rending Volley";
    data.skillBar.supports[primaryIndex] = {"Bloodletting", ""};
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Rending Volley fixture restores the active skill and Support");

    Input input;
    expect(moveToBoss(world, input),
        "Rending Volley fixture reaches the Boss through the real map path");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    const auto worldToScreen = [&world](const Vector2& position) {
        const Vector2 camera = world.cameraTopLeft();
        return sf::Vector2i(
            static_cast<int>(std::lround(position.x - camera.x)),
            static_cast<int>(std::lround(position.y - camera.y))
        );
    };
    auto findBoss = [&world]() {
        return std::find_if(
            world.enemies().begin(), world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
    };

    auto boss = findBoss();
    expect(boss != world.enemies().end(),
        "Rending Volley fixture exposes a live Boss");
    if (boss == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const sf::Vector2i bossScreen = worldToScreen(boss->position());
    input.handleMousePressed(sf::Mouse::Button::Left, bossScreen);
    bool bleedObserved = false;
    bool bleedFeedbackObserved = false;
    for (int frame = 0; frame < 80; ++frame) {
        world.update(0.05f, input);
        boss = findBoss();
        bleedObserved = bleedObserved
            || (boss != world.enemies().end() && boss->isBleeding());
        bleedFeedbackObserved = bleedFeedbackObserved || std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.source == "Bleed"
                    && feedback.type == CombatFeedbackType::Status;
            }
        );
        if (bleedObserved && bleedFeedbackObserved) {
            break;
        }
    }
    input.handleMouseReleased(sf::Mouse::Button::Left, bossScreen);
    expect(bleedObserved && bleedFeedbackObserved,
        "Rending Volley applies Bleed and emits typed status feedback");

    boss = findBoss();
    const int hpBeforeBleedTick = boss == world.enemies().end() ? 0 : boss->hp();
    world.update(Config::AilmentTickInterval, input);
    boss = findBoss();
    expect(boss != world.enemies().end() && boss->hp() < hpBeforeBleedTick,
        "Bleed deals damage through the real GameWorld tick path");

    std::filesystem::remove(path);
}

void testCrimsonSweepBleedFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_crimson_sweep_bleed_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17606);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Crimson Sweep fixture starts from a valid run save");

    data.unlockedSkills.insert("Crimson Sweep");
    data.unlockedSupports.insert("Rupture");
    data.skillBar.skills[utilityIndex] = "Crimson Sweep";
    data.skillBar.supports[utilityIndex] = {"Rupture", ""};
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Crimson Sweep fixture restores the active skill and Support");

    Input input;
    expect(moveToBoss(world, input),
        "Crimson Sweep fixture reaches the Boss through the real map path");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    auto findBoss = [&world]() {
        return std::find_if(
            world.enemies().begin(), world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
    };
    auto boss = findBoss();
    expect(boss != world.enemies().end(),
        "Crimson Sweep fixture exposes a live Boss");
    if (boss == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const auto& sweep = world.skillBar().definition(SkillSlot::Utility);
    expect(sweep.name == "Crimson Sweep"
            && sweep.castType == SkillCastType::SelfCenteredArea
            && world.skillBar().support(SkillSlot::Utility) != nullptr
            && world.skillBar().support(SkillSlot::Utility)->name == "Rupture",
        "Crimson Sweep is equipped in Utility with Rupture");

    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    bool bleedObserved = false;
    bool damageObserved = false;
    for (const auto& feedback : world.combatFeedback()) {
        damageObserved = damageObserved
            || (feedback.source == "Crimson Sweep"
                && feedback.type == CombatFeedbackType::Damage);
    }
    boss = findBoss();
    bleedObserved = boss != world.enemies().end() && boss->isBleeding();
    expect(damageObserved && bleedObserved,
        "Crimson Sweep deals area damage and applies Bleed through GameWorld");

    std::filesystem::remove(path);
}

void testSiphonPulseRecovery() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_siphon_pulse_recovery_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17604);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Siphon Pulse fixture starts from a valid run save");

    data.player.hp = 40;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.unlockedSkills.insert("Siphon Pulse");
    data.unlockedSupports.insert("Vitality");
    data.skillLevels["Siphon Pulse"] = 1;
    data.supportLevels["Vitality"] = 1;
    data.skillBar.skills[utilityIndex] = "Siphon Pulse";
    data.skillBar.supports[utilityIndex][0] = "Vitality";
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Siphon Pulse fixture restores the unlocked Utility skill");

    Input input;
    expect(moveToBoss(world, input),
        "Siphon Pulse fixture reaches the Boss Arena through real movement");
    const auto bossIt = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
    );
    expect(bossIt != world.enemies().end(),
        "Siphon Pulse fixture awakens a live Boss target");
    if (bossIt != world.enemies().end()) {
        const int hpBefore = world.player().hp();
        input.handleKeyPressed(sf::Keyboard::Key::Q);
        world.update(0.05f, input);
        input.handleKeyReleased(sf::Keyboard::Key::Q);

        const bool recoveryFeedback = std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::Status
                    && feedback.source.rfind("Siphon Pulse +", 0) == 0;
            }
        );
        expect(world.player().hp() >= hpBefore + 3 && recoveryFeedback,
            "Siphon Pulse and Vitality restore the enhanced hit recovery amount");
    }

    std::filesystem::remove(path);
}

void testGuardingPulseProtection() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_guarding_pulse_protection_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17605);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Guarding Pulse fixture starts from a valid run save");

    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 2.0f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.unlockedSkills.insert("Guarding Pulse");
    data.skillLevels["Guarding Pulse"] = 1;
    data.skillBar.skills[utilityIndex] = "Guarding Pulse";
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Guarding Pulse fixture restores the defensive Utility skill");

    Input input;
    expect(moveToBoss(world, input),
        "Guarding Pulse fixture reaches the Boss Arena through real movement");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    const auto liveBoss = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
    );
    if (liveBoss == world.enemies().end()) {
        expect(false, "Guarding Pulse fixture keeps a live Boss for mitigation");
        std::filesystem::remove(path);
        return;
    }
    const_cast<Player&>(world.player()).setPosition(liveBoss->position());
    for (int frame = 0; frame < 20 && world.playerHitDamage() == 0; ++frame) {
        world.update(0.05f, input);
    }
    const int unguardedDamage = world.playerHitDamage();
    const_cast<Player&>(world.player()).setPosition(world.map().playerStart());
    for (int frame = 0; frame < 30; ++frame) {
        world.update(0.05f, input);
    }

    const auto liveBossAfterWait = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
    );
    if (liveBossAfterWait == world.enemies().end()) {
        expect(false, "Guarding Pulse fixture keeps the Boss alive after cooldown");
        std::filesystem::remove(path);
        return;
    }
    const_cast<Player&>(world.player()).setPosition(liveBossAfterWait->position());
    const std::size_t guardedFeedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);
    const float guardTime = world.guardBuffTimeRemaining();
    const float guardMultiplier = world.guardBuffDamageTakenMultiplier();
    int guardedDamage = 0;
    for (std::size_t index = guardedFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.type == CombatFeedbackType::PlayerHit) {
            guardedDamage = feedback.damage;
            break;
        }
    }
    const bool guardFeedback = std::any_of(
        world.combatFeedback().begin(),
        world.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.type == CombatFeedbackType::Status
                && feedback.source.find("Guarding Pulse:") == 0;
        }
    );
    expect(guardTime > 0.0f && guardMultiplier < 1.0f && guardFeedback,
        "Guarding Pulse activates a timed mitigation state with feedback");
    expect(unguardedDamage > 0 && guardedDamage > 0 && guardedDamage < unguardedDamage,
        "Guarding Pulse reduces real Boss contact damage ("
            + std::to_string(unguardedDamage) + " -> "
            + std::to_string(guardedDamage) + ")");

    std::filesystem::remove(path);
}

void testManaWardProtection() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_mana_ward_protection_test.bin";
    std::filesystem::remove(path);

    GameWorld world(17606);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Mana Ward fixture starts from a valid run save");

    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 2.0f;
    data.player.mana = Config::PlayerMaxMana;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.unlockedSkills.insert("Mana Ward");
    data.skillLevels["Mana Ward"] = 1;
    data.skillBar.skills[utilityIndex] = "Mana Ward";
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Mana Ward fixture restores the resource shield skill");

    Input input;
    expect(moveToBoss(world, input),
        "Mana Ward fixture reaches the Boss Arena through real movement");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    const auto liveBoss = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
    );
    if (liveBoss == world.enemies().end()) {
        expect(false, "Mana Ward fixture keeps a live Boss for mitigation");
        std::filesystem::remove(path);
        return;
    }
    const_cast<Player&>(world.player()).setPosition(liveBoss->position());
    for (int frame = 0; frame < 20 && world.playerHitDamage() == 0; ++frame) {
        world.update(0.05f, input);
    }
    const int unwardedDamage = world.playerHitDamage();
    const_cast<Player&>(world.player()).setPosition(world.map().playerStart());
    for (int frame = 0; frame < 30; ++frame) {
        world.update(0.05f, input);
    }

    const auto bossAfterWait = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
    );
    if (bossAfterWait == world.enemies().end()) {
        expect(false, "Mana Ward fixture keeps the Boss alive after cooldown");
        std::filesystem::remove(path);
        return;
    }
    const_cast<Player&>(world.player()).setPosition(bossAfterWait->position());
    const int hpBeforeWard = world.player().hp();
    const std::size_t feedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    const auto feedbackBegin = world.combatFeedback().begin()
        + static_cast<std::ptrdiff_t>(feedbackStart);
    const bool wardFeedback = std::any_of(
        feedbackBegin,
        world.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.type == CombatFeedbackType::Status
                && feedback.source.find("Mana Ward absorbed ") == 0;
        }
    );
    expect(unwardedDamage > 0 && world.player().hp() == hpBeforeWard
            && world.manaWardCapacity() > world.manaWardAmount()
            && world.manaWardAmount() > 0
            && world.manaWardTimeRemaining() > 0.0f
            && wardFeedback,
        "Mana Ward absorbs a real Boss hit before HP and reports the remaining Ward");

    const_cast<Player&>(world.player()).setPosition(world.map().playerStart());
    world.update(Config::ManaWardEffectDuration + 0.1f, input);
    expect(world.manaWardAmount() == 0
            && world.manaWardCapacity() == 0
            && world.manaWardTimeRemaining() == 0.0f,
        "Mana Ward expires cleanly after its temporary duration");

    std::filesystem::remove(path);
}

void testIgniteFeedbackMatchesWorldDamage() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_ignite_feedback_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(18001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Ignite feedback fixture starts from a valid run save");
    data.mapLevel = 16;
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.damageMultiplier = 0.75f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Ignite feedback fixture restores a high-life target map");

    Input input;
    advanceIntoTheField(world, input);
    expect(!world.enemies().empty(), "Ignite feedback fixture reaches a live enemy");
    const auto targetIt = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const Enemy& enemy) {
            return enemy.type() == EnemyType::Normal
                || enemy.type() == EnemyType::Ranged;
        }
    );
    expect(targetIt != world.enemies().end(),
        "Ignite feedback fixture selects a Normal or Ranged target");
    if (targetIt == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const Vector2 target = targetIt->position();
    const Vector2 camera = world.cameraTopLeft();
    const sf::Vector2i screenTarget(
        static_cast<int>(std::lround(target.x - camera.x)),
        static_cast<int>(std::lround(target.y - camera.y))
    );
    input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
    world.update(0.05f, input);
    resolvePendingSkillEffects(world, input);
    expect(world.player().mana() < Config::PlayerMaxMana,
        "Ignite feedback fixture casts the real Secondary skill");

    std::map<int, int> hpBeforeTick;
    for (const auto& enemy : world.enemies()) {
        hpBeforeTick.emplace(enemy.id(), enemy.hp());
    }
    const int killsBeforeTick = world.mapKills();

    input.update();
    world.update(Config::AilmentTickInterval + 0.05f, input);

    int igniteFeedbackDamage = 0;
    int groundHazardFeedbackDamage = 0;
    bool igniteStatusObserved = false;
    for (const auto& feedback : world.combatFeedback()) {
        if (feedback.source == "Ignite") {
            if (feedback.type == CombatFeedbackType::Status) {
                igniteStatusObserved = true;
            } else {
                igniteFeedbackDamage += feedback.damage;
            }
        } else if (feedback.source == "Meteor Burning Ground") {
            groundHazardFeedbackDamage += feedback.damage;
        }
    }

    int enemyHpLoss = 0;
    int enemiesKilledByTick = 0;
    for (const auto& before : hpBeforeTick) {
        const auto current = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [&before](const Enemy& enemy) { return enemy.id() == before.first; }
        );
        const int after = current == world.enemies().end() ? 0 : current->hp();
        enemyHpLoss += std::max(0, before.second - after);
        if (current == world.enemies().end()) {
            ++enemiesKilledByTick;
        }
    }

    expect(igniteFeedbackDamage > 0,
        "real Ignite tick creates a distinct combat feedback source");
    expect(igniteStatusObserved,
        "real Ignite application creates typed status feedback");
    expect(igniteFeedbackDamage + groundHazardFeedbackDamage == enemyHpLoss,
        "damage-over-time feedback equals the actual Enemy HP loss");
    expect(enemiesKilledByTick > 0
            && world.mapKills() - killsBeforeTick == enemiesKilledByTick,
        "Ignite-killed enemies receive exactly one normal reward claim each");

    std::filesystem::remove(path);
}

void testIgniteDeathSpreadWorldFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_emberfall_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(18003);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Emberfall fixture starts from a valid run save");
    const std::size_t secondaryIndex = static_cast<std::size_t>(SkillSlot::Secondary);
    data.unlockedSkills.insert("Flare");
    data.unlockedSupports.insert("Emberfall");
    data.skillLevels["Flare"] = 1;
    data.supportLevels["Emberfall"] = 1;
    data.skillBar.skills[secondaryIndex] = "Flare";
    data.skillBar.supports[secondaryIndex] = {"Emberfall", ""};
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.mapLevel = 6;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Emberfall fixture restores Flare and its Support link");

    Input input;
    advanceIntoTheField(world, input);
    auto sourceIt = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [](const Enemy& enemy) { return !enemy.isBoss(); }
    );
    auto targetIt = sourceIt == world.enemies().end()
        ? world.enemies().end()
        : std::find_if(
            sourceIt + 1, world.enemies().end(),
            [](const Enemy& enemy) { return !enemy.isBoss(); }
        );
    expect(sourceIt != world.enemies().end() && targetIt != world.enemies().end(),
        "Emberfall fixture reaches two non-Boss enemies");
    if (sourceIt == world.enemies().end() || targetIt == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    Enemy& source = const_cast<Enemy&>(*sourceIt);
    Enemy& target = const_cast<Enemy&>(*targetIt);
    const int targetId = target.id();
    target.moveBy(
        source.position() + Vector2(120.0f, 0.0f) - target.position(),
        world.map()
    );
    const AilmentDefinition effectiveIgnite = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    source.applyIgnite(
        ailmentTickDamage(effectiveIgnite, 6),
        effectiveIgnite.duration,
        effectiveIgnite.igniteSpreadRadius,
        effectiveIgnite.igniteSpreadMultiplier
    );
    expect(effectiveIgnite.igniteSpreadRadius == 100.0f
            && effectiveIgnite.igniteSpreadMultiplier == 0.50f
            && source.isIgnited()
            && source.igniteSpreadRadius() == 100.0f,
        "Emberfall effective Fire snapshot reaches the real Enemy before death");
    source.kill();

    world.update(0.05f, input);
    const auto spreadTarget = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [targetId](const Enemy& enemy) { return enemy.id() == targetId; }
    );
    const bool spreadFeedback = std::any_of(
        world.combatFeedback().begin(), world.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.source == "Emberfall"
                && feedback.type == CombatFeedbackType::Status;
        }
    );
    expect(spreadTarget != world.enemies().end()
            && spreadTarget->isIgnited()
            && spreadTarget->igniteSpreadRadius() == 100.0f
            && spreadFeedback,
        "Ignite death spread applies to a nearby live enemy through GameWorld");

    std::filesystem::remove(path);
}

void testFreezeShatterWorldFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_shattering_ice_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(18004);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Shattering Ice fixture starts from a valid run save");

    const std::size_t primaryIndex = static_cast<std::size_t>(SkillSlot::Primary);
    data.unlockedSkills.insert("Glacial Shard");
    data.unlockedSupports.insert("Glacial Lock");
    data.unlockedSupports.insert("Shattering Ice");
    data.skillLevels["Glacial Shard"] = 1;
    data.supportLevels["Glacial Lock"] = 1;
    data.supportLevels["Shattering Ice"] = 1;
    data.skillBar.skills[primaryIndex] = "Glacial Shard";
    data.skillBar.supports[primaryIndex] = {"Glacial Lock", "Shattering Ice"};
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.mapLevel = 1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Shattering Ice fixture restores Glacial Shard and both Cold Supports");

    Input input;
    advanceIntoTheField(world, input);
    auto sourceIt = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [](const Enemy& enemy) { return !enemy.isBoss(); }
    );
    auto targetIt = sourceIt == world.enemies().end()
        ? world.enemies().end()
        : std::find_if(
            sourceIt + 1, world.enemies().end(),
            [](const Enemy& enemy) { return !enemy.isBoss(); }
        );
    expect(sourceIt != world.enemies().end() && targetIt != world.enemies().end(),
        "Shattering Ice fixture reaches two non-Boss enemies");
    if (sourceIt == world.enemies().end() || targetIt == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    Enemy& source = const_cast<Enemy&>(*sourceIt);
    Enemy& target = const_cast<Enemy&>(*targetIt);
    const int targetId = target.id();
    const Vector2 sourcePosition = world.player().position() + Vector2(150.0f, 0.0f);
    source.moveBy(sourcePosition - source.position(), world.map());
    target.moveBy(sourcePosition + Vector2(55.0f, 0.0f) - target.position(), world.map());
    source.applyFreeze(1.0f);
    const int targetHpBefore = target.hp();
    const AilmentDefinition effectiveChill = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Primary)
    );
    expect(effectiveChill.freezeDuration > 0.0f
            && effectiveChill.shatterRadius > 0.0f
            && effectiveChill.shatterDamageMultiplier > 0.0f,
        "real Glacial Shard snapshot contains Freeze and Shatter payloads");

    input.handleMousePressed(sf::Mouse::Button::Left, worldToScreen(world, source.position()));
    world.update(0.05f, input);
    input.handleMouseReleased(sf::Mouse::Button::Left, worldToScreen(world, source.position()));
    bool shatterObserved = false;
    for (int frame = 0; frame < 30 && !shatterObserved; ++frame) {
        world.update(0.05f, input);
        shatterObserved = std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.source == "Shatter"
                    && feedback.type == CombatFeedbackType::Status;
            }
        );
    }

    const auto targetAfter = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [targetId](const Enemy& enemy) { return enemy.id() == targetId; }
    );
    expect(shatterObserved, "real Cold hit consumes Freeze and emits Shatter feedback");
    expect(targetAfter == world.enemies().end()
            || targetAfter->hp() < targetHpBefore,
        "real Shatter damages a nearby enemy through GameWorld");

    std::filesystem::remove(path);
}

void testBuildMathMatchesWorldHits() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_build_math_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(19001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "build math fixture starts from a valid run save");

    const auto primaryIndex = static_cast<std::size_t>(SkillSlot::Primary);
    const auto secondaryIndex = static_cast<std::size_t>(SkillSlot::Secondary);
    data.unlockedSupports.insert("Pierce");
    data.unlockedSupports.insert("Volley");
    data.unlockedSupports.insert("Amplify");
    data.unlockedSupports.insert("Quickcast");
    data.skillBar.supports[primaryIndex] = {"Pierce", "Volley"};
    data.skillBar.supports[secondaryIndex] = {"Amplify", "Quickcast"};
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.upgradeStats.projectileDamageMultiplier = 1.40f;
    data.player.upgradeStats.areaDamageMultiplier = 1.60f;
    data.player.upgradeStats.areaRadiusMultiplier = 1.25f;
    data.player.upgradeStats.attackSpeedMultiplier = 2.0f;
    data.player.mana = Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "build math fixture restores stats and two Support Links");

    Input input;
    const Vector2 bossCenter = world.map().bossCenter();
    for (int frame = 0; frame < 180 && !world.map().bossTriggered(); ++frame) {
        const Vector2 delta = bossCenter - world.player().position();
        if (delta.x > 25.0f) {
            input.handleKeyPressed(sf::Keyboard::Key::D);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::D);
        }
        if (delta.y < -25.0f) {
            input.handleKeyPressed(sf::Keyboard::Key::W);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::W);
        }
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::D);
    input.handleKeyReleased(sf::Keyboard::Key::W);

    expect(world.map().bossTriggered(),
        "build math fixture reaches a Boss for real skill casts");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    const auto findBoss = [&world]() {
        return std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
    };
    const auto worldToScreen = [&world](const Vector2& position) {
        const Vector2 camera = world.cameraTopLeft();
        return sf::Vector2i(
            static_cast<int>(std::lround(position.x - camera.x)),
            static_cast<int>(std::lround(position.y - camera.y))
        );
    };

    auto boss = findBoss();
    expect(boss != world.enemies().end(),
        "build math fixture exposes the active Boss");
    if (boss == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const auto& primary = world.skillBar().definition(SkillSlot::Primary);
    const auto primarySupports = world.skillBar().supportDefinitionsFor(primary);
    const int expectedProjectileDamage = skillDamage(
        primary, world.player().stats(), primarySupports
    );
    const int bossHpBeforeProjectile = boss->hp();
    const std::size_t projectileFeedbackStart = world.combatFeedback().size();
    const auto hasProjectileFeedback = [&world, &primary]() {
        return std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [&primary](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::Damage
                    && feedback.source == primary.name;
            }
        );
    };
    input.handleMousePressed(sf::Mouse::Button::Left, worldToScreen(boss->position()));
    for (int frame = 0; frame < 30 && !hasProjectileFeedback(); ++frame) {
        world.update(0.05f, input);
    }
    input.handleMouseReleased(sf::Mouse::Button::Left, worldToScreen(bossCenter));

    int projectileFeedbackDamage = 0;
    bool projectileFeedbackMatches = true;
    for (std::size_t i = projectileFeedbackStart; i < world.combatFeedback().size(); ++i) {
        const auto& feedback = world.combatFeedback()[i];
        if (feedback.source == primary.name) {
            projectileFeedbackDamage += feedback.damage;
            projectileFeedbackMatches = projectileFeedbackMatches
                && feedback.damage == expectedProjectileDamage;
        }
    }
    boss = findBoss();
    const int bossHpAfterProjectile = boss == world.enemies().end() ? 0 : boss->hp();
    expect(projectileFeedbackDamage > 0 && projectileFeedbackMatches,
        "real Projectile feedback uses the CombatMath damage value");
    expect(bossHpBeforeProjectile - bossHpAfterProjectile == projectileFeedbackDamage,
        "real Projectile feedback equals the Boss HP delta");

    input.update();
    for (int frame = 0; frame < 120 && !world.projectiles().empty(); ++frame) {
        world.update(0.05f, input);
    }
    expect(world.projectiles().empty(),
        "old Projectile hits are drained before the Area comparison cast");
    boss = findBoss();
    if (boss == world.enemies().end()) {
        expect(false, "Boss remains alive for the real Area comparison cast");
        std::filesystem::remove(path);
        return;
    }

    const auto& secondary = world.skillBar().definition(SkillSlot::Secondary);
    const auto secondarySupports = world.skillBar().supportDefinitionsFor(secondary);
    const int expectedAreaRawDamage = skillDamage(
        secondary, world.player().stats(), secondarySupports
    );
    const int expectedAreaDamage = damageAfterResistance(
        expectedAreaRawDamage,
        secondary.damageType,
        world.bossDefinition().fireResistance,
        world.bossDefinition().coldResistance,
        world.bossDefinition().lightningResistance
    );
    const float expectedAreaRadius = skillRadius(
        secondary, world.player().stats(), secondarySupports
    );
    const int bossHpBeforeArea = boss->hp();
    input.handleMousePressed(sf::Mouse::Button::Right, worldToScreen(boss->position()));
    world.update(0.05f, input);
    resolvePendingSkillEffects(world, input);

    const bool areaFeedbackMatches = std::any_of(
        world.combatFeedback().begin(),
        world.combatFeedback().end(),
        [&secondary, expectedAreaDamage](const CombatFeedback& feedback) {
            return feedback.source == secondary.name
                && feedback.damage == expectedAreaDamage;
        }
    );
    boss = findBoss();
    const int bossHpAfterArea = boss == world.enemies().end() ? 0 : boss->hp();
    const int bossAreaDamage = bossHpBeforeArea - bossHpAfterArea;
    expect(areaFeedbackMatches,
        "real Area feedback uses the CombatMath damage value");
    expect(bossAreaDamage == expectedAreaDamage,
        "real Area feedback equals the Boss HP delta");
    expect(std::abs(world.secondarySkillEffectRadius() - expectedAreaRadius) < 0.001f,
        "real Area effect radius matches the CombatMath radius value");

    std::filesystem::remove(path);
}

void testExpandedSkillWorldHits() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_expanded_skill_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(20001);
    expect(!world.isSkillUnlocked("Arc Bolt")
            && !world.isSkillUnlocked("Shockwave")
            && !world.isSkillUnlocked("Split Arrow")
            && !world.isSkillUnlocked("Aftershock")
            && !world.isSkillUnlocked("Ember Lance")
            && !world.isSkillUnlocked("Glacial Shard")
            && !world.isSkillUnlocked("Stormfield")
            && !world.isSkillUnlocked("Blight Ring")
            && !world.isSkillUnlocked("Siphon Pulse")
            && !world.isSkillUnlocked("Guarding Pulse")
            && !world.isSkillUnlocked("Mana Ward")
            && !world.isSupportUnlocked("Barrage")
            && !world.isSupportUnlocked("Concentration"),
        "expanded skills and Supports start locked");

    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "expanded skill fixture starts from a valid run save");

    const auto primaryIndex = static_cast<std::size_t>(SkillSlot::Primary);
    const auto secondaryIndex = static_cast<std::size_t>(SkillSlot::Secondary);
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    data.unlockedSkills.insert("Arc Bolt");
    data.unlockedSkills.insert("Toxic Burst");
    data.unlockedSkills.insert("Shockwave");
    data.unlockedSkills.insert("Aftershock");
    data.unlockedSkills.insert("Ember Lance");
    data.unlockedSkills.insert("Glacial Shard");
    data.unlockedSkills.insert("Stormfield");
    data.unlockedSkills.insert("Blight Ring");
    data.unlockedSkills.insert("Siphon Pulse");
    data.unlockedSkills.insert("Guarding Pulse");
    data.unlockedSkills.insert("Mana Ward");
    data.unlockedSupports.insert("Barrage");
    data.unlockedSupports.insert("Concentration");
    data.unlockedSupports.insert("Echo");
    data.skillBar.skills[primaryIndex] = "Arc Bolt";
    data.skillBar.skills[secondaryIndex] = "Toxic Burst";
    data.skillBar.skills[utilityIndex] = "Shockwave";
    data.skillBar.supports[primaryIndex] = {"Barrage", ""};
    data.skillBar.supports[utilityIndex] = {"Concentration", "Echo"};
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.upgradeStats.projectileDamageMultiplier = 2.0f;
    data.player.upgradeStats.areaDamageMultiplier = 2.0f;
    data.player.upgradeStats.areaRadiusMultiplier = 2.0f;
    data.player.mana = Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "expanded skill fixture restores unlocks and links");
    expect(world.skillBar().definition(SkillSlot::Primary).name == "Arc Bolt"
            && world.skillBar().definition(SkillSlot::Secondary).name == "Toxic Burst"
            && world.skillBar().definition(SkillSlot::Utility).name == "Shockwave"
            && world.skillBar().supportAt(SkillSlot::Primary, 0) != nullptr
            && world.skillBar().supportAt(SkillSlot::Primary, 0)->name == "Barrage"
            && world.skillBar().supportAt(SkillSlot::Utility, 0) != nullptr
            && world.skillBar().supportAt(SkillSlot::Utility, 0)->name == "Concentration"
            && world.skillBar().supportAt(SkillSlot::Utility, 1) != nullptr
            && world.skillBar().supportAt(SkillSlot::Utility, 1)->name == "Echo",
        "expanded skill fixture restores the active skills and Support links");

    Input input;
    expect(moveToBoss(world, input),
        "expanded skill fixture reaches the Boss for real casts");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }

    const auto findBoss = [&world]() {
        return std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
    };
    const auto worldToScreen = [&world](const Vector2& position) {
        const Vector2 camera = world.cameraTopLeft();
        return sf::Vector2i(
            static_cast<int>(std::lround(position.x - camera.x)),
            static_cast<int>(std::lround(position.y - camera.y))
        );
    };

    auto boss = findBoss();
    expect(boss != world.enemies().end(),
        "expanded skill fixture exposes the active Boss");
    if (boss == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const auto& projectileSkill = world.skillBar().definition(SkillSlot::Primary);
    const auto projectileSupports = world.skillBar().supportDefinitionsFor(projectileSkill);
    const int expectedProjectileRawDamage = skillDamage(
        projectileSkill, world.player().stats(), projectileSupports
    );
    const int expectedProjectileDamage = damageAfterResistance(
        expectedProjectileRawDamage,
        projectileSkill.damageType,
        world.bossDefinition().fireResistance,
        world.bossDefinition().coldResistance,
        world.bossDefinition().lightningResistance
    );
    const int expectedShockDamage = static_cast<int>(std::ceil(
        static_cast<float>(expectedProjectileDamage)
            * damageTakenMultiplierAfterResistance(
                projectileSkill.ailment.damageTakenMultiplier,
                world.bossDefinition().shockResistance,
                0
            )
    ));
    const int bossHpBeforeProjectile = boss->hp();
    const std::size_t projectileFeedbackStart = world.combatFeedback().size();
    const sf::Vector2i bossScreen = worldToScreen(boss->position());
    input.handleMousePressed(sf::Mouse::Button::Left, bossScreen);
    for (int frame = 0; frame < 40
            && world.combatFeedback().size() == projectileFeedbackStart; ++frame) {
        world.update(0.05f, input);
    }
    input.handleMouseReleased(sf::Mouse::Button::Left, bossScreen);

    int projectileFeedbackDamage = 0;
    int projectileHitCount = 0;
    bool projectileFeedbackMatches = true;
    for (std::size_t index = projectileFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.source == projectileSkill.name) {
            projectileFeedbackDamage += feedback.damage;
            const int expectedHitDamage = projectileHitCount == 0
                ? expectedProjectileDamage : expectedShockDamage;
            projectileFeedbackMatches = projectileFeedbackMatches
                && feedback.damage == expectedHitDamage;
            ++projectileHitCount;
        }
    }
    boss = findBoss();
    const int bossHpAfterProjectile = boss == world.enemies().end() ? 0 : boss->hp();
    expect(projectileHitCount == 2 && projectileFeedbackMatches
            && projectileFeedbackDamage == expectedProjectileDamage + expectedShockDamage,
        "Arc Bolt real feedback includes Shock's increased follow-up hit");
    expect(boss != world.enemies().end() && boss->isShocked(),
        "Arc Bolt applies Shock through the real GameWorld path");
    expect(bossHpBeforeProjectile - bossHpAfterProjectile == projectileFeedbackDamage,
        "Arc Bolt feedback equals the Boss HP delta");

    input.update();
    for (int frame = 0; frame < 120 && !world.projectiles().empty(); ++frame) {
        world.update(0.05f, input);
    }
    boss = findBoss();
    if (boss == world.enemies().end()) {
        expect(false, "Boss remains alive before the Shockwave cast");
        std::filesystem::remove(path);
        return;
    }

    const auto& areaSkill = world.skillBar().definition(SkillSlot::Utility);
    const auto areaSupports = world.skillBar().supportDefinitionsFor(areaSkill);
    const int expectedAreaRawDamage = skillDamage(
        areaSkill, world.player().stats(), areaSupports
    );
    const int expectedAreaResistedDamage = damageAfterResistance(
        expectedAreaRawDamage,
        areaSkill.damageType,
        world.bossDefinition().fireResistance,
        world.bossDefinition().coldResistance,
        world.bossDefinition().lightningResistance
    );
    const int expectedAreaDamage = static_cast<int>(std::ceil(
        static_cast<float>(expectedAreaResistedDamage) * boss->damageTakenMultiplier()
    ));
    const float expectedAreaRadius = skillRadius(
        areaSkill, world.player().stats(), areaSupports
    );
    const int expectedAreaRepeatCount = skillRepeatCount(areaSkill, areaSupports);
    const int bossHpBeforeArea = boss->hp();
    const std::size_t areaFeedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);

    int areaFeedbackDamage = 0;
    bool areaFeedbackMatches = true;
    for (std::size_t index = areaFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.source == areaSkill.name) {
            areaFeedbackDamage += feedback.damage;
            areaFeedbackMatches = areaFeedbackMatches
                && feedback.damage == expectedAreaDamage;
        }
    }
    boss = findBoss();
    const int bossHpAfterArea = boss == world.enemies().end() ? 0 : boss->hp();
    expect(areaFeedbackDamage > 0 && areaFeedbackMatches,
        "Shockwave real feedback uses CombatMath damage");
    expect(areaFeedbackDamage == expectedAreaDamage * expectedAreaRepeatCount,
        "Echo repeats the real Area hit through the GameWorld cast path");
    expect(bossHpBeforeArea - bossHpAfterArea == areaFeedbackDamage,
        "Shockwave feedback equals the Boss HP delta");
    expect(std::abs(world.novaEffectRadius() - expectedAreaRadius) < 0.001f,
        "Shockwave real radius matches CombatMath");
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    // F7 extends the ten number-key entries and is interpreted as a skill
    // assignment only while the Skill Panel owns the input context.
    input.handleKeyPressed(sf::Keyboard::Key::K);
    world.update(0.05f, input);
    input.handleKeyPressed(sf::Keyboard::Key::F7);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Aftershock",
        "Skill Panel F7 assigns the eleventh skill entry");

    input.handleKeyPressed(sf::Keyboard::Key::F9);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Primary).name == "Ember Lance",
        "Skill Panel F9 assigns the first elemental skill entry");
    input.handleKeyPressed(sf::Keyboard::Key::F12);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Blight Ring",
        "Skill Panel F12 assigns the later Poison skill entry");
    input.handleKeyPressed(sf::Keyboard::Key::F14);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Siphon Pulse",
        "Skill Panel F14 assigns the appended recovery skill entry");
    input.handleKeyPressed(sf::Keyboard::Key::F15);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Guarding Pulse",
        "Skill Panel F15 assigns the appended defensive skill entry");
    input.handleMouseMoved({650, 358});
    world.update(0.05f, input);
    expect(world.hoveredSkillIndex() == 20,
        "Skill Panel mouse hover selects the Mana Ward entry");
    const std::size_t projectilesBeforeSkillClick = world.projectiles().size();
    input.handleMousePressed(sf::Mouse::Button::Left, {650, 358});
    world.update(0.05f, input);
    input.handleMouseReleased(sf::Mouse::Button::Left, {650, 358});
    expect(world.projectiles().size() == projectilesBeforeSkillClick,
        "Skill Panel mouse click does not fire the primary skill");
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Mana Ward",
        "Skill Panel mouse click assigns the Mana Ward entry");
    input.handleKeyPressed(sf::Keyboard::Key::F12);
    world.update(0.05f, input);
    expect(world.skillBar().definition(SkillSlot::Utility).name == "Blight Ring",
        "Skill Panel can restore Blight Ring after the F14 assignment");
    input.handleKeyPressed(sf::Keyboard::Key::K);
    world.update(0.05f, input);

    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);
    for (int frame = 0; frame < 8; ++frame) {
        world.update(0.05f, input);
    }
    const bool blightHazardCreated = std::any_of(
        world.groundHazards().begin(), world.groundHazards().end(),
        [](const GroundHazard& hazard) {
            return hazard.definition().source == "Blight Mire";
        }
    );
    expect(blightHazardCreated,
        "Blight Ring creates its Poison ground hazard through the real Utility path");

    std::filesystem::remove(path);
}

void testBossRelicEffectsInWorld() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_boss_relic_effect_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(21001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Boss relic effect fixture starts from a valid run save");

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("boss.brimstone-brand");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Ring)] =
        makeBaseItem("boss.storm-signet");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Amulet)] =
        makeBaseItem("boss.brood-talisman");
    data.unlockedSkills.insert("Flare");
    data.unlockedSkills.insert("Toxic Burst");
    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Flare";
    data.player.mana = Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    const bool relicSaveSucceeded = SaveService::save(path, data, &error);
    SaveData relicRoundTrip;
    const bool relicLoadSucceeded = SaveService::load(path, relicRoundTrip, &error);
    expect(relicSaveSucceeded && relicLoadSucceeded && world.loadRun(path),
        "Boss relic effect fixture restores all three relics");
    expect(world.bossRelicEffectSummary().find("Molten Core") != std::string::npos
            && world.bossRelicEffectSummary().find("Storm Chain") != std::string::npos
            && world.bossRelicEffectSummary().find("Brood Bloom") != std::string::npos,
        "equipped relics expose all three combat effects");

    const auto fireAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(std::abs(fireAilment.damageMultiplier - 0.625f) < 0.0001f
            && std::abs(fireAilment.duration - 3.125f) < 0.0001f,
        "Brimstone relic modifies the effective Fire skill Ignite");

    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Toxic Burst";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture switches to Toxic Burst");
    const auto poisonAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(poisonAilment.poisonSpreadRadius >= 150.0f
            && poisonAilment.poisonSpreadMultiplier >= 0.50f,
        "Brood relic adds Poison spread to the effective skill");

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Ring)] =
        makeBaseItem("boss.frostbound-loop");
    data.unlockedSkills.insert("Frost Bomb");
    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Frost Bomb";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture switches to Frostbound Loop");
    const auto frostAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(world.bossRelicEffectSummary().find("Frostbite") != std::string::npos
            && frostAilment.speedMultiplier < 0.65f
            && frostAilment.duration > 2.0f,
        "Frost relic strengthens the effective Cold skill Chill");

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("weapon.rustbound-blade");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Amulet)] =
        makeBaseItem("boss.ashen-crucible");
    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Flare";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture equips the alternate Brimstone relic");
    const auto alternateFireAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(world.bossRelicEffectSummary().find("Ashen Bloom") != std::string::npos
            && alternateFireAilment.damageMultiplier > fireAilment.damageMultiplier
            && alternateFireAilment.duration < 3.0f,
        "alternate Brimstone relic changes the real Fire ailment effect");

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("weapon.rustbound-blade");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Ring)] =
        makeBaseItem("ring.cinder-band");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Amulet)] =
        makeBaseItem("amulet.ironheart-pendant");
    data.unlockedSkills.insert("Frost Bomb");
    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Frost Bomb";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture resets to a plain Cold skill");
    const auto plainChillAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("boss.tidebound-ledger");
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture equips the Archive relic");
    const auto archiveChillAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(world.bossRelicEffectSummary().find("Archive Current") != std::string::npos
            && archiveChillAilment.speedMultiplier < plainChillAilment.speedMultiplier
            && archiveChillAilment.duration > plainChillAilment.duration,
        "Archive relic changes the real Cold skill Chill");

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("weapon.rustbound-blade");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Ring)] =
        makeBaseItem("ring.cinder-band");
    data.unlockedSkills.insert("Flare");
    data.skillBar.skills[static_cast<std::size_t>(SkillSlot::Secondary)] = "Flare";
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture resets to a plain Fire skill");
    const auto plainIgniteAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );

    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Amulet)] =
        makeBaseItem("boss.obsidian-crown");
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss relic effect fixture equips the Obsidian relic");
    const auto obsidianIgniteAilment = world.effectiveSkillAilment(
        world.skillBar().definition(SkillSlot::Secondary)
    );
    expect(world.bossRelicEffectSummary().find("Obsidian Furnace") != std::string::npos
            && obsidianIgniteAilment.damageMultiplier > plainIgniteAilment.damageMultiplier
            && obsidianIgniteAilment.duration > plainIgniteAilment.duration,
        "Obsidian relic changes the real Fire skill Ignite");

    std::filesystem::remove(path);
}

void prepareCombinationFixture(
    GameWorld& world,
    const std::filesystem::path& path,
    int templateIndex,
    int layoutIndex = 0,
    int mapLevel = 1,
    int manaFlaskCharges = Config::ManaFlaskMaxCharges
);

void testBossCombatFlow() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_boss_combat_test.bin";
    std::filesystem::remove(path);

    GameWorld world(18001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Boss combat fixture starts from a valid run save");

    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 8.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.upgradeStats.projectileDamageMultiplier = 100.0f;
    data.player.upgradeStats.areaDamageMultiplier = 1.0f;
    data.player.mana = data.player.mana > 0.0f ? data.player.mana : Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    data.fieldPacksCleared = Config::BossGateRequiredFieldPacks;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Boss combat fixture restores boosted test-only progression through SaveData");

    Input input;
    const Vector2 bossCenter = world.map().bossCenter();
    for (int frame = 0; frame < 180 && !world.map().bossTriggered(); ++frame) {
        const Vector2 delta = bossCenter - world.player().position();
        if (delta.x > 25.0f) {
            input.handleKeyPressed(sf::Keyboard::Key::D);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::D);
        }
        if (delta.y < -25.0f) {
            input.handleKeyPressed(sf::Keyboard::Key::W);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::W);
        }
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::D);
    input.handleKeyReleased(sf::Keyboard::Key::W);

    expect(world.map().bossTriggered(),
        "real movement reaches the Boss Arena and triggers the Boss");
    if (!world.map().bossTriggered()) {
        std::filesystem::remove(path);
        return;
    }
    bool bossHpReduced = false;
    bool telegraphFeedbackObserved = false;
    bool elementalBossWarningObserved = false;
    bool playerHitFeedbackObserved = false;
    bool enrageObserved = false;
    bool enrageAddsObserved = false;
    bool enrageHazardObserved = false;
    bool arenaHazardObserved = false;
    bool arenaHazardPatternObserved = false;
    bool finalPhaseObserved = false;
    bool finalPhaseAddsObserved = false;
    bool finalPhaseHazardObserved = false;
    for (int frame = 0; frame < 120 && world.state() == GameState::Playing; ++frame) {
        const auto bossIt = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
        if (bossIt == world.enemies().end()) {
            break;
        }

        const int hpBefore = bossIt->hp();
        const Vector2 camera = world.cameraTopLeft();
        const sf::Vector2i screenTarget(
            static_cast<int>(std::lround(bossIt->position().x - camera.x)),
            static_cast<int>(std::lround(bossIt->position().y - camera.y))
        );
        input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
        world.update(0.05f, input);
        resolvePendingSkillEffects(world, input);
        if (world.bossEnraged()) {
            enrageObserved = true;
            enrageAddsObserved = enrageAddsObserved || std::any_of(
                world.enemies().begin(), world.enemies().end(),
                [](const Enemy& enemy) { return !enemy.isBoss() && !enemy.isDead(); }
            );
            enrageHazardObserved = enrageHazardObserved || std::any_of(
                world.groundHazards().begin(), world.groundHazards().end(),
                [&world](const GroundHazard& hazard) {
                    return hazard.definition().source
                        == world.bossDefinition().enrageHazard.source;
                }
            );
        }
        if (world.bossPhase() >= 2) {
            finalPhaseObserved = true;
            finalPhaseAddsObserved = finalPhaseAddsObserved || std::any_of(
                world.enemies().begin(), world.enemies().end(),
                [](const Enemy& enemy) {
                    return !enemy.isBoss() && !enemy.isDead();
                }
            );
            finalPhaseHazardObserved = finalPhaseHazardObserved || std::any_of(
                world.groundHazards().begin(), world.groundHazards().end(),
                [&world](const GroundHazard& hazard) {
                    return hazard.definition().source
                        == world.bossDefinition().finalPhase.hazard.source;
                }
            );
        }
        arenaHazardObserved = arenaHazardObserved || std::any_of(
            world.groundHazards().begin(),
            world.groundHazards().end(),
            [&world](const GroundHazard& hazard) {
                return hazard.definition().source
                    == world.map().definition().bossArenaEffect.hazard.source;
            }
        );
        const int arenaHazardCount = static_cast<int>(std::count_if(
            world.groundHazards().begin(),
            world.groundHazards().end(),
            [&world](const GroundHazard& hazard) {
                return hazard.definition().source
                    == world.map().definition().bossArenaEffect.hazard.source;
            }
        ));
        arenaHazardPatternObserved = arenaHazardPatternObserved || arenaHazardCount >= 4;
        telegraphFeedbackObserved = telegraphFeedbackObserved || std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::Telegraph;
            }
        );
        elementalBossWarningObserved = elementalBossWarningObserved
            || world.bossSkillWarning().find("[Fire/Ignite]") != std::string::npos;
        playerHitFeedbackObserved = playerHitFeedbackObserved || std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::PlayerHit;
            }
        );

        const auto bossAfterHit = std::find_if(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) { return enemy.isBoss() && !enemy.isDead(); }
        );
        bossHpReduced = bossHpReduced
            || (bossAfterHit != world.enemies().end() && bossAfterHit->hp() < hpBefore)
            || world.state() == GameState::MapComplete;
    }

    const bool bossFeedbackObserved = std::any_of(
        world.combatFeedback().begin(),
        world.combatFeedback().end(),
        [](const CombatFeedback& feedback) { return feedback.damage > 0; }
    );
    expect(bossFeedbackObserved, "Boss damage creates the same combat feedback records");
    expect(bossHpReduced, "Boss HP decreases on the same hit that creates feedback");
    expect(telegraphFeedbackObserved,
        "Boss telegraph creates a typed feedback record from the real Boss state");
    expect(elementalBossWarningObserved,
        "Brimstone Boss warning exposes its Fire/Ignite skill identity");
    expect(playerHitFeedbackObserved,
        "player damage creates a typed PlayerHit feedback record");
    expect(enrageObserved, "Boss enters its data-driven enrage phase");
    expect(enrageAddsObserved, "Boss enrage phase adds reinforcements");
    expect(enrageHazardObserved, "Boss enrage phase creates its arena hazard");
    expect(arenaHazardObserved,
        "map theme creates an independent Boss Arena hazard");
    expect(arenaHazardPatternObserved,
        "Brimstone Boss Arena hazard uses its data-driven ring pattern");
    expect(finalPhaseObserved, "Boss enters its data-driven final phase");
    expect(finalPhaseAddsObserved, "Boss final phase adds theme reinforcements");
    expect(finalPhaseHazardObserved, "Boss final phase creates its theme hazard");
    expect(world.state() == GameState::MapComplete && world.map().bossDefeated(),
        "Boss death enters MapComplete through the real reward path");
    expect(world.mapItems().size() == 3 && world.completedMapCount() == 1,
        "Boss completion creates three held maps and records the atlas entry");
    expect(std::all_of(
            world.mapItems().begin(),
            world.mapItems().end(),
            [&world](const MapItem& mapItem) {
                return mapItem.mapLevel == world.mapLevel() + 1
                    && !mapItem.id.empty();
            }),
        "held map items target the next tier with stable identities");
    expect(world.mapBossItemsDropped() >= 1 && !world.droppedItems().empty(),
        "Boss death creates at least one guaranteed ground drop");
    const int qualityDropTotal = world.mapDroppedItemsByRarity(Rarity::Normal)
        + world.mapDroppedItemsByRarity(Rarity::Magic)
        + world.mapDroppedItemsByRarity(Rarity::Rare)
        + world.mapDroppedItemsByRarity(Rarity::Unique);
    expect(qualityDropTotal == world.mapItemsDropped()
            && world.mapDroppedItemsByRarity(Rarity::Unique) >= 1,
        "Boss rewards update the per-rarity drop breakdown");
    expect(std::any_of(
            world.droppedItems().begin(),
            world.droppedItems().end(),
            [](const DroppedItem& dropped) {
                return dropped.item().rarity == Rarity::Unique;
            }),
        "Boss death includes a Unique theme relic on the ground");
    const auto relicPreview = world.bossRelicPreview();
    const auto uniqueDrop = std::find_if(
        world.droppedItems().begin(),
        world.droppedItems().end(),
        [](const DroppedItem& dropped) {
            return dropped.item().rarity == Rarity::Unique;
        }
    );
    expect(relicPreview.has_value()
            && uniqueDrop != world.droppedItems().end()
            && relicPreview->baseId == uniqueDrop->item().baseId
            && relicPreview->name == uniqueDrop->item().name
            && std::abs(relicPreview->stats.damageMultiplier
                - uniqueDrop->item().stats.damageMultiplier) < 0.0001f,
        "MapComplete previews the actual deterministic Boss relic");
    expect(world.mapRewardOptions()[0].skillName == SkillLibrary::flare().name
            || world.mapRewardOptions()[0].supportName == "Combustion",
        "Brimstone Boss reward path leads with a Fire build option");

    std::filesystem::remove(path);
}

void testThemedBossPhaseHazards() {
    struct HazardCase {
        int mapLevel;
        int templateIndex;
        std::string source;
        DamageType damageType;
        int expectedImpactCount;
        std::string label;
    };

    const std::array<HazardCase, 6> cases{{
        {1, 0, "Cinder Cross", DamageType::Fire, 4, "Brimstone"},
        {2, 1, "Static Lattice", DamageType::Lightning, 4, "Storm"},
        {3, 2, "Acid Bloom", DamageType::Poison, 4, "Brood"},
        {4, 3, "Frostline Cross", DamageType::Cold, 4, "Frost"},
        {5, 4, "Archive Undertow", DamageType::Cold, 1, "Archive"},
        {6, 5, "Molten Ring", DamageType::Fire, 4, "Obsidian"}
    }};

    for (const auto& hazardCase : cases) {
        const auto path = std::filesystem::temp_directory_path()
            / ("plane_fight_" + hazardCase.label + "_boss_phase_hazard_test.bin");
        std::filesystem::remove(path);

        GameWorld world(23000 + static_cast<std::uint64_t>(hazardCase.mapLevel));
        prepareCombinationFixture(
            world, path, hazardCase.templateIndex, 0, hazardCase.mapLevel
        );

        SaveData data;
        std::string error;
        expect(SaveService::load(path, data, &error),
            hazardCase.label + " Boss hazard fixture reloads its map state");
        data.player.upgradeStats.damageMultiplier = 1.0f;
        data.player.upgradeStats.projectileDamageMultiplier = 1.0f;
        data.player.upgradeStats.areaDamageMultiplier = 1.0f;
        data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
        expect(SaveService::save(path, data, &error) && world.loadRun(path),
            hazardCase.label + " Boss hazard fixture applies controlled damage");

        Input input;
        expect(moveToBoss(world, input),
            hazardCase.label + " themed Boss can be reached through the real map");

        bool finalPhaseObserved = false;
        if (world.map().bossTriggered()) {
            for (int cast = 0; cast < 100
                && world.state() == GameState::Playing
                && !finalPhaseObserved; ++cast) {
                const auto bossIt = std::find_if(
                    world.enemies().begin(), world.enemies().end(),
                    [](const Enemy& enemy) {
                        return enemy.isBoss() && !enemy.isDead();
                    }
                );
                if (bossIt == world.enemies().end()) {
                    world.update(0.05f, input);
                    continue;
                }

                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    worldToScreen(world, bossIt->position())
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                world.update(1.5f, input);
                finalPhaseObserved = world.bossPhase() >= 2;
            }
        }

        bool telegraphObserved = false;
        bool warningPatternObserved = false;
        bool impactObserved = false;
        if (finalPhaseObserved) {
            for (int frame = 0; frame < 150
                && world.state() == GameState::Playing; ++frame) {
                world.update(0.05f, input);
                telegraphObserved = telegraphObserved
                    || world.bossPhaseHazardTelegraphProgress() > 0.0f;
                warningPatternObserved = warningPatternObserved
                    || (world.bossPhaseHazardWarningPositions().size()
                        == static_cast<std::size_t>(hazardCase.expectedImpactCount)
                        && world.bossPhaseHazardDamageType()
                            == hazardCase.damageType);
                if (telegraphObserved) {
                    impactObserved = impactObserved || std::any_of(
                        world.groundHazards().begin(),
                        world.groundHazards().end(),
                        [&hazardCase](const GroundHazard& hazard) {
                            return hazard.definition().source == hazardCase.source
                                && hazard.definition().damageType
                                    == hazardCase.damageType;
                        }
                    );
                }
                if (impactObserved) {
                    break;
                }
            }
        }

        expect(finalPhaseObserved,
            hazardCase.label + " Boss enters its final phase");
        expect(telegraphObserved,
            hazardCase.label + " Boss shows the recurring hazard telegraph");
        expect(warningPatternObserved,
            hazardCase.label + " Boss exposes its configured warning pattern");
        expect(impactObserved,
            hazardCase.label + " Boss leaves its scaled elemental ground hazard");

        std::filesystem::remove(path);
    }
}

void testElementalEnemyProjectileFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_elemental_enemy_projectile_test.bin";
    std::filesystem::remove(path);

    GameWorld world(19001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "elemental enemy fixture starts from a valid run save");

    data.mapTemplateIndex = 1;
    data.mapLayoutIndex = 1;
    data.mapLevel = 2;
    data.currentMapOption = MapOptionLibrary::generateOptions(1)[1];
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "elemental enemy fixture restores the ranged map template and challenge");
    expect(world.mapModifier().elementalChallengeType == DamageType::Lightning
            && world.mapModifier().playerElementalResistancePenalty >= 25,
        "Stormbound applies its Lightning resistance challenge to the live map");

    Input input;
    advanceIntoTheField(world, input);
    input.handleKeyPressed(sf::Keyboard::Key::W);
    for (int frame = 0; frame < 20; ++frame) {
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::W);
    bool rangedSpawned = false;
    bool lightningProjectileObserved = false;
    bool shockProjectileObserved = false;
    bool playerHitObserved = false;
    bool shockObserved = false;
    for (int frame = 0; frame < 800 && world.state() == GameState::Playing; ++frame) {
        world.update(0.05f, input);
        rangedSpawned = rangedSpawned || std::any_of(
            world.enemies().begin(),
            world.enemies().end(),
            [](const Enemy& enemy) {
                return enemy.type() == EnemyType::Ranged && !enemy.isDead();
            }
        );
        lightningProjectileObserved = lightningProjectileObserved || std::any_of(
            world.enemyProjectiles().begin(),
            world.enemyProjectiles().end(),
            [](const EnemyProjectile& projectile) {
                return projectile.damageType == DamageType::Lightning;
            }
        );
        shockProjectileObserved = shockProjectileObserved || std::any_of(
            world.enemyProjectiles().begin(),
            world.enemyProjectiles().end(),
            [](const EnemyProjectile& projectile) {
                return projectile.ailment.type == AilmentType::Shock;
            }
        );
        playerHitObserved = playerHitObserved || std::any_of(
            world.combatFeedback().begin(),
            world.combatFeedback().end(),
            [](const CombatFeedback& feedback) {
                return feedback.type == CombatFeedbackType::PlayerHit
                    && feedback.source == "Spitter shot";
            }
        );
        shockObserved = shockObserved || world.player().isShocked();
        if (lightningProjectileObserved && playerHitObserved && shockObserved) {
            break;
        }
    }

    expect(rangedSpawned,
        "Stormscar map spawns a data-driven ranged enemy");
    expect(lightningProjectileObserved,
        "ranged enemy projectile carries its configured Lightning damage type");
    expect(shockProjectileObserved,
        "ranged enemy projectile carries its configured Shock ailment");
    expect(playerHitObserved,
        "ranged enemy projectile reaches the player collision path");
    expect(shockObserved,
        "Lightning projectile applies Shock to the player after resistance");

    bool stormHazardObserved = false;
    for (int frame = 0; frame < 260 && world.state() == GameState::Playing; ++frame) {
        world.update(0.05f, input);
        stormHazardObserved = stormHazardObserved || std::any_of(
            world.groundHazards().begin(),
            world.groundHazards().end(),
            [](const GroundHazard& hazard) {
                return hazard.definition().source == "Arc Flash";
            }
        );
    }
    expect(stormHazardObserved,
        "Stormscar field spawns its data-driven Arc Flash hazard from map tier two");

    const int flaskChargesBefore = world.lifeFlaskCharges();
    input.handleKeyPressed(sf::Keyboard::Key::G);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::G);
    expect(!world.player().hasAilment(),
        "life flask clears the active elemental ailment");
    expect(world.lifeFlaskCharges() == flaskChargesBefore - 1
            && world.lifeFlaskStatusMessage().find("cleansed") != std::string::npos,
        "life flask consumes one charge and reports the cleanse");
    std::filesystem::remove(path);
}

void testThemeEnemyElementalAttacks() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_theme_enemy_attack_test.bin";
    std::filesystem::remove(path);

    GameWorld world(20002);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Venom theme attack fixture starts from a valid run save");

    data.mapLevel = 2;
    data.mapTemplateIndex = 2;
    data.mapLayoutIndex = 0;
    data.currentMapOption = MapOptionLibrary::defaultOption();
    data.currentMapOption.templateIndex = 2;
    data.player.hp = 10000;
    data.player.upgradeStats.maxHp = 10000;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Venom theme attack fixture restores the T2 map");

    Input input;
    advanceIntoTheField(world, input);
    bool poisonObserved = false;
    for (int frame = 0; frame < 240 && world.state() == GameState::Playing; ++frame) {
        world.update(0.05f, input);
        poisonObserved = poisonObserved || world.player().isPoisoned();
    }
    expect(poisonObserved,
        "Venom theme converts physical field attacks into Poison pressure");

    std::filesystem::remove(path);
}

void testManaFlaskFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_mana_flask_test.bin";
    std::filesystem::remove(path);

    GameWorld world(22001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Mana flask fixture starts from a valid save");

    data.player.mana = 5.0f;
    data.manaFlaskCharges = Config::ManaFlaskMaxCharges;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Mana flask fixture restores a low-Mana run");

    const float manaBefore = world.player().mana();
    const int chargesBefore = world.manaFlaskCharges();
    Input input;
    pressKey(world, input, sf::Keyboard::Key::H);
    expect(world.player().mana() > manaBefore
            && world.manaFlaskCharges() == chargesBefore - 1
            && world.manaFlaskStatusMessage().find("Mana flask") != std::string::npos,
        "H consumes one Mana flask charge and restores Mana");

    expect(world.saveRun(path) && SaveService::load(path, data, &error)
            && data.manaFlaskCharges == chargesBefore - 1
            && data.player.mana > manaBefore,
        "Mana flask charges and restored Mana survive a save round trip");

    data.player.mana = Config::PlayerMaxMana;
    data.manaFlaskCharges = 2;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Mana flask fixture restores a full-Mana state");
    pressKey(world, input, sf::Keyboard::Key::H);
    expect(world.manaFlaskCharges() == 2
            && world.manaFlaskStatusMessage() == "Mana already full",
        "full Mana does not consume a Mana flask charge");

    std::filesystem::remove(path);
}

void testElitePackEventFlow() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_elite_pack_event_test.bin";
    std::filesystem::remove(path);

    GameWorld world(20001);
    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "ElitePack fixture starts from a valid run save");

    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 6.0f;
    data.player.upgradeStats.damageMultiplier = 10.0f;
    data.player.upgradeStats.areaDamageMultiplier = 10.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.manaFlaskCharges = 0;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "ElitePack fixture restores a high-tolerance combat setup");

    const auto eventIt = std::find_if(
        world.map().events().begin(),
        world.map().events().end(),
        [](const MapEventInstance& event) { return event.type == MapEventType::ElitePack; }
    );
    expect(eventIt != world.map().events().end(),
        "ElitePack fixture finds the generated ElitePack event");
    if (eventIt == world.map().events().end()) {
        std::filesystem::remove(path);
        return;
    }

    const Vector2 eventPosition = eventIt->position;
    Input input;
    for (int frame = 0; frame < 300 && world.activeEliteEventEnemiesRemaining() == 0;
        ++frame) {
        const float deltaX = eventPosition.x - world.player().position().x;
        if (std::abs(deltaX) > 18.0f) {
            const auto key = deltaX > 0.0f
                ? sf::Keyboard::Key::D : sf::Keyboard::Key::A;
            const auto opposite = deltaX > 0.0f
                ? sf::Keyboard::Key::A : sf::Keyboard::Key::D;
            input.handleKeyPressed(key);
            input.handleKeyReleased(opposite);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::A);
            input.handleKeyReleased(sf::Keyboard::Key::D);
        }
        input.handleKeyReleased(sf::Keyboard::Key::W);
        input.handleKeyReleased(sf::Keyboard::Key::S);
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::A);
    input.handleKeyReleased(sf::Keyboard::Key::D);
    input.handleKeyReleased(sf::Keyboard::Key::W);
    input.handleKeyReleased(sf::Keyboard::Key::S);
    for (int frame = 0; frame < 300 && world.activeEliteEventEnemiesRemaining() == 0;
        ++frame) {
        const float deltaY = eventPosition.y - world.player().position().y;
        if (std::abs(deltaY) > 18.0f) {
            const auto key = deltaY > 0.0f
                ? sf::Keyboard::Key::S : sf::Keyboard::Key::W;
            const auto opposite = deltaY > 0.0f
                ? sf::Keyboard::Key::W : sf::Keyboard::Key::S;
            input.handleKeyPressed(key);
            input.handleKeyReleased(opposite);
        } else {
            input.handleKeyReleased(sf::Keyboard::Key::W);
            input.handleKeyReleased(sf::Keyboard::Key::S);
        }
        input.handleKeyReleased(sf::Keyboard::Key::A);
        input.handleKeyReleased(sf::Keyboard::Key::D);
        world.update(0.05f, input);
    }
    input.handleKeyReleased(sf::Keyboard::Key::A);
    input.handleKeyReleased(sf::Keyboard::Key::D);
    input.handleKeyReleased(sf::Keyboard::Key::W);
    input.handleKeyReleased(sf::Keyboard::Key::S);

    expect(world.activeEliteEventEnemiesRemaining() == 5,
        "entering ElitePack starts exactly five event enemies");
    expect(world.mapEventsCompleted() == 0,
        "an active ElitePack is not counted as completed");
    const int forgeFragmentsBefore = world.forgeFragments();
    if (world.activeEliteEventEnemiesRemaining() == 5) {
        const Vector2 camera = world.cameraTopLeft();
        const sf::Vector2i screenTarget(
            static_cast<int>(std::lround(eventPosition.x - camera.x)),
            static_cast<int>(std::lround(eventPosition.y - camera.y))
        );
        input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
        world.update(0.05f, input);
        resolvePendingSkillEffects(world, input);
    }

    expect(world.activeEliteEventEnemiesRemaining() == 0,
        "one boosted area cast clears all five ElitePack enemies");
    expect(world.mapEventsCompleted() == 1,
        "ElitePack completes only after all event enemies are defeated");
    expect(world.eventStatusMessage().find("Elite pack cleared") != std::string::npos,
        "ElitePack completion reports nearby loot feedback");
    expect(world.forgeFragments() == forgeFragmentsBefore
            + Config::ElitePackForgeFragmentReward
            && world.eventStatusMessage().find("Forge Fragments") != std::string::npos,
        "ElitePack completion awards its configured forge fragments");
    expect(world.manaFlaskCharges() > 0,
        "ElitePack completion keeps Mana flask charges available");

    std::filesystem::remove(path);
}

bool moveToMapEvent(GameWorld& world, Input& input, const Vector2& position) {
    const auto moveAxisPrecisely = [&](bool horizontal, float target) {
        constexpr float arrivalDistance = 20.0f;
        constexpr int maxFrames = 500;
        for (int frame = 0; frame < maxFrames
            && world.state() == GameState::Playing; ++frame) {
            const float current = horizontal
                ? world.player().position().x : world.player().position().y;
            const float delta = target - current;
            if (std::abs(delta) <= arrivalDistance) {
                releaseMovement(input);
                return true;
            }

            const bool positive = delta > 0.0f;
            const auto key = horizontal
                ? (positive ? sf::Keyboard::Key::D : sf::Keyboard::Key::A)
                : (positive ? sf::Keyboard::Key::S : sf::Keyboard::Key::W);
            const auto opposite = horizontal
                ? (positive ? sf::Keyboard::Key::A : sf::Keyboard::Key::D)
                : (positive ? sf::Keyboard::Key::W : sf::Keyboard::Key::S);
            input.handleKeyPressed(key);
            input.handleKeyReleased(opposite);
            if (horizontal) {
                input.handleKeyReleased(sf::Keyboard::Key::W);
                input.handleKeyReleased(sf::Keyboard::Key::S);
            } else {
                input.handleKeyReleased(sf::Keyboard::Key::A);
                input.handleKeyReleased(sf::Keyboard::Key::D);
            }
            world.update(0.05f, input);
        }

        releaseMovement(input);
        return world.state() == GameState::Playing
            && std::abs((horizontal
                ? world.player().position().x : world.player().position().y) - target)
                <= arrivalDistance;
    };

    return moveAxisPrecisely(true, position.x)
        && moveAxisPrecisely(false, position.y);
}

void prepareCombinationFixture(
    GameWorld& world,
    const std::filesystem::path& path,
    int templateIndex,
    int layoutIndex,
    int mapLevel,
    int manaFlaskCharges
) {
    SaveData data;
    std::string error;
    if (!world.saveRun(path) || !SaveService::load(path, data, &error)) {
        return;
    }

    data.mapTemplateIndex = templateIndex;
    data.mapLayoutIndex = layoutIndex;
    data.mapLevel = mapLevel;
    data.currentMapOption.templateIndex = templateIndex;
    data.state = SavedRunState::Playing;
    data.mapRewardChosen = false;
    data.nextMapOptionChosen = false;
    data.selectedMapRewardOption = -1;
    data.selectedNextMapOption = -1;
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 2.0f;
    data.player.upgradeStats.damageMultiplier = 10.0f;
    data.player.upgradeStats.areaDamageMultiplier = 20.0f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.manaFlaskCharges = manaFlaskCharges;
    SaveService::save(path, data, &error);
    world.loadRun(path);
}

const MapEventInstance* combinationEvent(const GameWorld& world) {
    const auto it = std::find_if(
        world.map().events().begin(),
        world.map().events().end(),
        [](const MapEventInstance& event) {
            return event.type == MapEventType::Combination;
        }
    );
    return it == world.map().events().end() ? nullptr : &*it;
}

void testCombinationMapEvents() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_combination_event_test.bin";
    std::filesystem::remove(path);

    {
        GameWorld world(21001);
        prepareCombinationFixture(world, path, 0, 0, 1, 0);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::EnhancedCache,
            "map 1 combination fixture uses the data-driven Enhanced Cache");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Enhanced Cache encounter");
            pressKey(world, input, sf::Keyboard::Key::F);
            const MapEventInstance* afterOpen = combinationEvent(world);
            expect(afterOpen != nullptr && afterOpen->triggered && afterOpen->completed,
                "Enhanced Cache completes exactly on its F interaction");
            expect(world.mapEventsCompleted() == 1 && world.mapItemsDropped() >= 3,
                "Enhanced Cache uses its configured drop quantity");
            expect(world.forgeFragments() == Config::LootCacheForgeFragmentReward
                    && world.eventStatusMessage().find("Forge Fragments")
                        != std::string::npos,
                "Enhanced Cache awards its configured forge fragments");
            expect(world.manaFlaskCharges() == 1,
                "Enhanced Cache restores one Mana flask charge");
            const int dropsAfterOpen = world.mapItemsDropped();
            pressKey(world, input, sf::Keyboard::Key::F);
            expect(world.mapItemsDropped() == dropsAfterOpen,
                "completed Enhanced Cache cannot be opened twice");

            expect(world.saveRun(path) && world.loadRun(path),
                "completed combination event can be saved and loaded");
            const MapEventInstance* afterLoad = combinationEvent(world);
            expect(afterLoad != nullptr && afterLoad->completed
                    && world.mapEventsCompleted() == 1,
                "saved combination completion is restored without duplication");
            expect(world.forgeFragments() == Config::LootCacheForgeFragmentReward,
                "saved combination completion preserves its forge fragments");
        }
    }

    {
        GameWorld world(21002);
        prepareCombinationFixture(world, path, 1);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::HazardousElitePack,
            "map 1 alternate layout uses the Hazardous Elite Pack definition");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            const Vector2 verticalWaypoint(world.player().position().x, position.y);
            expect(moveToMapEvent(world, input, verticalWaypoint)
                    && moveToMapEvent(world, input, position),
                "player can reach the Hazardous Elite Pack encounter");
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && world.groundHazards().size() == 1,
                "Hazardous Elite Pack spawns five owned enemies and one hazard");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            const MapEventInstance* afterClear = combinationEvent(world);
            const int remainingAfterClear = world.activeEliteEventEnemiesRemaining();
            expect(afterClear != nullptr && afterClear->completed
                    && remainingAfterClear == 0,
                "Hazardous Elite Pack completes after its owned enemies die");
            const std::size_t enemyCount = world.enemies().size();
            for (int frame = 0; frame < 20; ++frame) {
                world.update(0.05f, input);
            }
            expect(world.mapEventsCompleted() == 1
                    && world.enemies().size() <= enemyCount + 1,
                "completed Hazardous Elite Pack does not respawn its encounter enemies");
        }
    }

    {
        GameWorld world(21004);
        prepareCombinationFixture(world, path, 1);
        const MapEventInstance* event = combinationEvent(world);
        const auto eliteIt = std::find_if(
            world.map().events().begin(),
            world.map().events().end(),
            [](const MapEventInstance& candidate) {
                return candidate.type == MapEventType::ElitePack;
            }
        );
        expect(event != nullptr && eliteIt != world.map().events().end(),
            "map event lock fixture exposes a basic Elite Pack and combination event");
        if (event != nullptr && eliteIt != world.map().events().end()) {
            Input input;
            const Vector2 aroundObstacleWaypoint(1500.0f, 820.0f);
            expect(moveToMapEvent(world, input, aroundObstacleWaypoint)
                    && moveToMapEvent(world, input, eliteIt->position),
                "player can reach the first Elite Pack in the event lock fixture");
            expect(world.activeEliteEventEnemiesRemaining() == 5,
                "first event owns its five spawned enemies");
            const Vector2 combinationPosition = event->position;
            expect(moveToMapEvent(world, input, combinationPosition),
                "player can reach the second event while the first is active");
            const MapEventInstance* afterMove = combinationEvent(world);
            expect(afterMove != nullptr && !afterMove->triggered
                    && world.activeEliteEventEnemiesRemaining() == 5,
                "an active event blocks a second enemy encounter from overwriting ownership");
        }
    }

    {
        GameWorld world(21003);
        prepareCombinationFixture(world, path, 2);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::GuardedShrine,
            "map 1 third layout uses the Guarded Shrine definition");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Guarded Shrine encounter");
            pressKey(world, input, sf::Keyboard::Key::F);
            expect(world.activeEliteEventEnemiesRemaining() == 4
                    && world.mapEventsCompleted() == 0,
                "Guarded Shrine requires clearing four guardians first");
            pressKey(world, input, sf::Keyboard::Key::F);
            expect(world.activeEliteEventEnemiesRemaining() == 4
                    && world.mapEventsCompleted() == 0,
                "Guarded Shrine cannot be activated while guardians remain");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 0,
                "Guarded Shrine remains incomplete after guardians are defeated");
            const int forgeFragmentsBefore = world.forgeFragments();
            pressKey(world, input, sf::Keyboard::Key::F);
            const MapEventInstance* afterActivate = combinationEvent(world);
            expect(afterActivate != nullptr && afterActivate->completed
                    && world.shrineBuffTimeRemaining() > 0.0f,
                "Guarded Shrine activates once after its guards are cleared");
            expect(world.forgeFragments() == forgeFragmentsBefore
                    + world.map().encounterDefinition().forgeFragmentReward,
                "Guarded Shrine awards its data-driven forge fragment reward");
        }
    }

    {
        GameWorld world(21004);
        prepareCombinationFixture(world, path, 0, 2, 3, 0);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::BloodlettingPit,
            "map level three Ashen variant uses the Bloodletting Pit encounter");
        expect(world.bossDefinition().name == "Gorebound Executioner"
                && world.bossDefinition().physicalResistance == 35,
            "Bloodletting Pit routes the map Boss to the Gorebound Executioner");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Bloodletting Pit encounter");
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && std::any_of(
                        world.groundHazards().begin(),
                        world.groundHazards().end(),
                        [](const GroundHazard& hazard) {
                            return hazard.definition().source == "Hemorrhage Pool"
                                && hazard.definition().damageType == DamageType::Physical
                                && hazard.definition().ailment.type == AilmentType::Bleed;
                        }
                    ),
                "Bloodletting Pit spawns its Bleed hazard and five owned enemies");

            bool bleedObserved = false;
            for (int frame = 0; frame < 120; ++frame) {
                world.update(0.05f, input);
                bleedObserved = bleedObserved || world.player().isBleeding();
            }
            expect(bleedObserved && world.player().bleedStacks() > 0,
                "Bloodletting Pit applies real Bleed pressure to the player");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            const MapEventInstance* afterClear = combinationEvent(world);
            expect(afterClear != nullptr && afterClear->completed
                    && world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapItemsDropped() >= 4,
                "Bloodletting Pit clears and drops its Physical/Bleed reward");
            expect(world.map().encounterDefinition().bossDropBonus == 2,
                "Bloodletting Pit increases the completed-map Boss reward");
        }
    }

    {
        GameWorld world(21008);
        prepareCombinationFixture(world, path, 6, 0, 7, 0);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::AetherConvergence,
            "map level seven uses the data-driven Aether Convergence encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Aether Convergence encounter");
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && std::any_of(
                        world.groundHazards().begin(),
                        world.groundHazards().end(),
                        [](const GroundHazard& hazard) {
                            return hazard.definition().damageType == DamageType::Lightning;
                        }
                    ),
                "Aether Convergence spawns its Lightning hazard and owned enemies");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            const MapEventInstance* afterClear = combinationEvent(world);
            expect(afterClear != nullptr && afterClear->completed
                    && world.activeEliteEventEnemiesRemaining() == 0,
                "Aether Convergence now triggers and clears through the shared event path");
        }
    }

    {
        GameWorld world(21005);
        prepareCombinationFixture(world, path, 1, 2);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::BountyHunt,
            "alternate layout uses the data-driven Bounty Hunt encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            const Vector2 verticalWaypoint(world.player().position().x, position.y);
            expect(moveToMapEvent(world, input, verticalWaypoint)
                    && moveToMapEvent(world, input, position),
                "player can reach the Bounty Hunt encounter");
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && world.groundHazards().empty(),
                "Bounty Hunt spawns two Elite and three Normal enemies without a hazard");

            const int dropsBeforeClear = world.mapItemsDropped();
            const int forgeFragmentsBefore = world.forgeFragments();
            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            const MapEventInstance* afterClear = combinationEvent(world);
            expect(afterClear != nullptr && afterClear->completed
                    && world.activeEliteEventEnemiesRemaining() == 0,
                "Bounty Hunt completes after its owned enemies die");
            expect(world.mapItemsDropped() >= dropsBeforeClear + 2
                    && world.eventStatusMessage().find("bonus items dropped")
                        != std::string::npos,
                "Bounty Hunt drops its configured completion reward once");
            expect(world.forgeFragments() == forgeFragmentsBefore
                    + world.map().encounterDefinition().forgeFragmentReward,
                "Bounty Hunt awards its data-driven forge fragment reward");
        }
    }

    {
        GameWorld world(21006);
        prepareCombinationFixture(world, path, 2, 2);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::CursedReliquary,
            "third template layout uses the data-driven Cursed Reliquary encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, {800.0f, 500.0f})
                    && moveToMapEvent(world, input, {1350.0f, 500.0f})
                    && moveToMapEvent(world, input, position),
                "player can reach the Cursed Reliquary encounter");
            pressKey(world, input, sf::Keyboard::Key::F);
            expect(world.activeEliteEventEnemiesRemaining() == 3
                    && world.mapEventsCompleted() == 0,
                "Cursed Reliquary requires clearing its three guardians");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1
                    && world.mapItemsDropped() >= 4,
                "Cursed Reliquary clears and drops its larger completion reward");
        }
    }

    {
        GameWorld world(21007);
        prepareCombinationFixture(world, path, 2, 2, 2);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::WardenCourt,
            "map level two selects the data-driven Warden Court encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, {800.0f, 500.0f})
                    && moveToMapEvent(world, input, {1350.0f, 500.0f})
                    && moveToMapEvent(world, input, position),
                "player can reach the Warden Court encounter");
            const auto eventEnemyCount = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(), world.enemies().end(),
                    [type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == 3 && enemy.type() == type;
                    }
                );
            };
            expect(world.activeEliteEventEnemiesRemaining() == 4
                    && eventEnemyCount(EnemyType::Warden) == 2
                    && eventEnemyCount(EnemyType::Summoner) == 2,
                "Warden Court spawns two Wardens and two Hexbinders");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Warden Court clears and completes its encounter");
        }
    }

    {
        GameWorld world(21008);
        prepareCombinationFixture(world, path, 3, 0, 4);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::FrozenReliquary,
            "map level four selects the Frost Frozen Reliquary encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, {840.0f, 700.0f})
                    && moveToMapEvent(world, input, position),
                "player can reach the Frost Frozen Reliquary encounter");
            const auto frozenHazard = std::find_if(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Rime Sigil";
                }
            );
            expect(world.activeEliteEventEnemiesRemaining() == 4
                    && frozenHazard != world.groundHazards().end(),
                "Frozen Reliquary spawns one Elite, three Wardens and one Cold hazard");
            expect(world.map().encounterDefinition().rewardLootBias.primaryTag == AffixTag::Cold
                    && world.map().encounterDefinition().rewardLootBias.secondaryTag == AffixTag::Area,
                "Frozen Reliquary configures Cold-biased completion rewards");
            const auto& hazard = frozenHazard->definition();
            expect(hazard.damageType == DamageType::Cold
                    && hazard.ailment.type == AilmentType::Chill,
                "Frozen Reliquary hazard applies Cold and Chill feedback");
            expect(hazard.damage > world.map().encounterDefinition().hazard.damage,
                "Frozen Reliquary hazard damage scales with its map level");
            for (int frame = 0; frame < 210; ++frame) {
                world.update(0.05f, input);
            }
            const bool ambientHazardSpawned = std::any_of(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& candidate) {
                    return candidate.definition().source == "Rimefall";
                }
            );
            expect(ambientHazardSpawned,
                "Frost field periodically spawns its telegraphed Rimefall hazard");
            const auto ambientHazard = std::find_if(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& candidate) {
                    return candidate.definition().source == "Rimefall";
                }
            );
            expect(ambientHazard != world.groundHazards().end()
                    && ambientHazard->definition().damage
                        > MapTemplateLibrary::forIndex(3)
                            .ambientEffect.hazard.damage,
                "Frost field hazard damage scales with its map level");
        }
    }

    {
        GameWorld world(21009);
        prepareCombinationFixture(world, path, 4, 0, 5);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::ArchivePurge,
            "map level five selects the Drowned Archive Archive Purge encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, {850.0f, world.player().position().y})
                    && moveToMapEvent(world, input, {850.0f, position.y})
                    && moveToMapEvent(world, input, position),
                "player can reach the Archive Purge encounter");
            const auto archiveHazard = std::find_if(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Inkfreeze Seal";
                }
            );
            const auto countEventEnemies = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == 3 && enemy.type() == type;
                    }
                );
            };
            expect(world.activeEliteEventEnemiesRemaining() == 5,
                "Archive Purge starts with five encounter enemies");
            expect(countEventEnemies(EnemyType::Warden) == 1
                    && countEventEnemies(EnemyType::Ranged) == 4,
                "Archive Purge spawns one Warden and four Ranged enemies");
            expect(archiveHazard != world.groundHazards().end(),
                "Archive Purge creates its Cold hazard");
            expect(world.map().encounterDefinition().rewardLootBias.primaryTag == AffixTag::Cold
                    && world.map().encounterDefinition().rewardLootBias.secondaryTag
                        == AffixTag::Projectile,
                "Archive Purge uses Cold and Projectile completion rewards");
            bool archiveProjectileSeen = false;
            for (int frame = 0; frame < 11; ++frame) {
                world.update(0.05f, input);
                archiveProjectileSeen = archiveProjectileSeen
                    || std::any_of(
                        world.enemyProjectiles().begin(),
                        world.enemyProjectiles().end(),
                        [](const EnemyProjectile& projectile) {
                            return projectile.damageType == DamageType::Cold
                                && projectile.ailment.type == AilmentType::Chill;
                        }
                    );
            }
            expect(archiveProjectileSeen,
                "Archive Purge Ranged enemies fire Cold projectiles that Chill");
            bool archiveSkillTelegraphSeen = false;
            bool archiveSkillHazardSeen = false;
            for (int frame = 0; frame < 120; ++frame) {
                world.update(0.05f, input);
                archiveSkillTelegraphSeen = archiveSkillTelegraphSeen
                    || !world.mapEventSkillWarning().empty();
                archiveSkillHazardSeen = archiveSkillHazardSeen
                    || std::any_of(
                        world.groundHazards().begin(),
                        world.groundHazards().end(),
                        [](const GroundHazard& hazard) {
                            return hazard.definition().source == "Frozen Ink";
                        }
                    );
            }
            expect(archiveSkillTelegraphSeen,
                "Archive Purge Warden telegraphs its Inkfreeze Pulse");
            expect(archiveSkillHazardSeen,
                "Archive Purge Inkfreeze Pulse leaves a Cold ground hazard");

            const Vector2 camera = world.cameraTopLeft();
            input.handleMousePressed(
                sf::Mouse::Button::Right,
                {static_cast<int>(std::lround(position.x - camera.x)),
                 static_cast<int>(std::lround(position.y - camera.y))}
            );
            world.update(0.05f, input);
            resolvePendingSkillEffects(world, input);
            expect(world.activeEliteEventEnemiesRemaining() == 0,
                "Archive Purge encounter enemies can be cleared");
            expect(world.mapEventsCompleted() == 1
                    && world.mapItemsDropped() >= 4,
                "Archive Purge drops its configured completion reward");
            expect(world.bossRewardSummary().find("+1 Boss Drop") != std::string::npos
                    && world.bossRewardSummary().find("Cold") != std::string::npos
                    && world.bossRewardSummary().find("Projectile") != std::string::npos,
                "Archive Purge adds Cold / Projectile bias to the Boss reward");

            SaveData rewardData;
            std::string rewardError;
            expect(world.saveRun(path)
                    && SaveService::load(path, rewardData, &rewardError),
                "Archive Purge reward fixture saves its completed encounter");
            rewardData.fieldPacksCleared = world.fieldPacksRequired();
            rewardData.state = SavedRunState::Playing;
            expect(SaveService::save(path, rewardData, &rewardError)
                    && world.loadRun(path),
                "Archive Purge reward fixture restores the unlocked Boss path");
            Input bossInput;
            expect(moveToBoss(world, bossInput),
                "Archive Purge reward fixture reaches its themed Boss");
            expect(defeatBossWithAreaSkill(world, bossInput),
                "Archive Purge reward fixture defeats its themed Boss");
            expect(world.mapBossItemsDropped()
                    >= world.bossDefinition().guaranteedDrops + 1,
                "Archive Purge guarantees an extra Boss drop after event completion");
        }
    }

    {
        GameWorld world(21010);
        prepareCombinationFixture(world, path, 5, 0, 6);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::ForgeCollapse,
            "map level six selects the Obsidian Reliquary Forge Collapse encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, {850.0f, world.player().position().y})
                    && moveToMapEvent(world, input, {850.0f, position.y})
                    && moveToMapEvent(world, input, position),
                "player can reach the Forge Collapse encounter");
            const auto forgeHazard = std::find_if(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Forge Collapse";
                }
            );
            const auto countEventEnemies = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == 3 && enemy.type() == type;
                    }
                );
            };
            expect(world.activeEliteEventEnemiesRemaining() == 5,
                "Forge Collapse starts with five encounter enemies");
            expect(countEventEnemies(EnemyType::Charger) == 1
                    && countEventEnemies(EnemyType::Summoner) == 4,
                "Forge Collapse spawns one Charger and four Summoners");
            expect(forgeHazard != world.groundHazards().end(),
                "Forge Collapse creates its Fire hazard");
            expect(world.map().encounterDefinition().rewardLootBias.primaryTag == AffixTag::Fire
                    && world.map().encounterDefinition().rewardLootBias.secondaryTag
                        == AffixTag::Area,
                "Forge Collapse uses Fire and Area completion rewards");
            expect(moveToMapEvent(world, input, {position.x + 160.0f, position.y}),
                "Forge Collapse fixture reaches the Charger attack path");
            for (int frame = 0; frame < 30 && !world.player().isIgnited(); ++frame) {
                world.update(0.05f, input);
            }
            expect(world.player().isIgnited(),
                "Forge Collapse Charger attacks use Fire and Ignite");
            bool forgeSkillTelegraphSeen = false;
            bool forgeSkillHazardSeen = false;
            for (int frame = 0; frame < 120; ++frame) {
                world.update(0.05f, input);
                forgeSkillTelegraphSeen = forgeSkillTelegraphSeen
                    || !world.mapEventSkillWarning().empty();
                forgeSkillHazardSeen = forgeSkillHazardSeen
                    || std::any_of(
                        world.groundHazards().begin(),
                        world.groundHazards().end(),
                        [](const GroundHazard& hazard) {
                            return hazard.definition().source == "Magma Brand";
                        }
                    );
            }
            expect(forgeSkillTelegraphSeen,
                "Forge Collapse Charger telegraphs its Magma Collapse");
            expect(forgeSkillHazardSeen,
                "Forge Collapse Magma Collapse leaves a Fire ground hazard");

            for (int cast = 0; cast < 8
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0,
                "Forge Collapse encounter enemies can be cleared");
            expect(world.mapEventsCompleted() == 1
                    && world.mapItemsDropped() >= 4,
                "Forge Collapse drops its configured completion reward");
            expect(world.bossRewardSummary().find("+1 Boss Drop") != std::string::npos
                    && world.bossRewardSummary().find("Fire") != std::string::npos
                    && world.bossRewardSummary().find("Area") != std::string::npos,
                "Forge Collapse adds Fire / Area bias to the Boss reward");
        }
    }

    {
        GameWorld world(21011);
        prepareCombinationFixture(world, path, 7, 0, 8, 0);
        const MapEventInstance* event = combinationEvent(world);
        expect(event != nullptr
                && event->encounterType == MapEncounterType::NecroticOssuary,
            "map level eight selects the Sable Necropolis Necrotic Ossuary encounter");
        if (event != nullptr) {
            const Vector2 position = event->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Necrotic Ossuary encounter");
            const auto marrowHazard = std::find_if(
                world.groundHazards().begin(),
                world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Marrow Bloom";
                }
            );
            const auto countEventEnemies = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(),
                    world.enemies().end(),
                    [type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == 3 && enemy.type() == type;
                    }
                );
            };
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && countEventEnemies(EnemyType::Warden) == 1
                    && countEventEnemies(EnemyType::Summoner) == 4,
                "Necrotic Ossuary spawns its Warden and Summoner composition");
            expect(marrowHazard != world.groundHazards().end()
                    && marrowHazard->definition().damage
                        > world.map().encounterDefinition().hazard.damage,
                "Necrotic Ossuary hazard damage scales with its map level");

            for (int cast = 0; cast < 8
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Necrotic Ossuary completes after its Poison encounter enemies die");
            Input bossInput;
            expect(moveToBoss(world, bossInput)
                    && world.bossDefinition().name == "Gravebloom Sovereign",
                "Sable Necropolis reaches its themed Boss after the encounter");
            expect(defeatBossWithAreaSkill(world, bossInput)
                    && world.state() == GameState::MapComplete
                    && world.mapBossItemsDropped()
                        >= world.bossDefinition().guaranteedDrops + 2,
                "Gravebloom Sovereign completes the map with its bonus Boss drops");
        }
    }

    {
        GameWorld world(21012);
        prepareCombinationFixture(world, path, 0, 2, 9, 0);
        SaveData atlasData;
        std::string atlasError;
        expect(world.saveRun(path) && SaveService::load(path, atlasData, &atlasError),
            "Ironheart Atlas fixture starts from a valid high-tier save");
        for (int index = 0; index < 12; ++index) {
            atlasData.completedMapIds.insert(
                "ironheart-atlas-fixture-" + std::to_string(index)
            );
        }
        atlasData.allocatedAtlasNodes = {9, 10, 11, 12};
        expect(SaveService::save(path, atlasData, &atlasError)
                && world.loadRun(path)
                && world.atlasBonuses().ironheartBossDropBonus == 1,
            "Ironheart Atlas fixture restores the dedicated drop node");
        const auto eventIt = std::find_if(
            world.map().events().begin(), world.map().events().end(),
            [](const MapEventInstance& event) {
                return event.type == MapEventType::Combination
                    && event.encounterType == MapEncounterType::IronheartTrial;
            }
        );
        expect(eventIt != world.map().events().end(),
            "map level nine exposes the Ironheart Trial encounter");
        if (eventIt != world.map().events().end()) {
            const std::size_t eventIndex = static_cast<std::size_t>(
                std::distance(world.map().events().begin(), eventIt)
            );
            const Vector2 position = eventIt->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Ironheart Trial encounter");

            const auto ownedEnemyCount = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(), world.enemies().end(),
                    [eventIndex, type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == static_cast<int>(eventIndex)
                            && enemy.type() == type;
                    }
                );
            };
            const auto ironbloodIt = std::find_if(
                world.groundHazards().begin(), world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Ironblood Seal";
                }
            );
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && ownedEnemyCount(EnemyType::Elite) == 2
                    && ownedEnemyCount(EnemyType::Charger) == 3
                    && ironbloodIt != world.groundHazards().end(),
                "Ironheart Trial spawns its Physical/Bleed pack and seal hazard");

            for (int cast = 0; cast < 10
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Ironheart Trial completes after its owned enemies die");

            Input bossInput;
            expect(moveToBoss(world, bossInput)
                    && world.bossDefinition().name == "Ironheart Warden"
                    && world.bossDefinition().guaranteedDrops == 3
                    && world.bossDefinition().relicVariant == 2,
                "Ironheart Trial routes to the Ironheart Warden Boss");
            const bool bossDefeated = defeatBossWithAreaSkill(world, bossInput);
            const bool dedicatedRelicDropped = std::any_of(
                world.droppedItems().begin(), world.droppedItems().end(),
                [](const DroppedItem& dropped) {
                    return dropped.item().baseId == "boss.ironheart-bastion";
                }
            );
            expect(bossDefeated && dedicatedRelicDropped,
                "Ironheart Warden drops the dedicated Ironheart Bastion relic");
            expect(world.mapBossItemsDropped() >= 14,
                "Ironheart Atlas node increases the real Ironheart Boss drop count");
        }
    }

    {
        GameWorld world(21013);
        prepareCombinationFixture(world, path, 1, 2, 9, 0);
        const auto eventIt = std::find_if(
            world.map().events().begin(), world.map().events().end(),
            [](const MapEventInstance& event) {
                return event.type == MapEventType::Combination
                    && event.encounterType == MapEncounterType::StormglassGauntlet;
            }
        );
        expect(eventIt != world.map().events().end(),
            "map level nine exposes the Stormglass Gauntlet encounter");
        if (eventIt != world.map().events().end()) {
            const std::size_t eventIndex = static_cast<std::size_t>(
                std::distance(world.map().events().begin(), eventIt)
            );
            const Vector2 position = eventIt->position;
            Input input;
            const bool reachedEvent = moveToMapEvent(world, input, {800.0f, 850.0f})
                && moveToMapEvent(world, input, position);
            expect(reachedEvent,
                "player can reach the Stormglass Gauntlet encounter");
            const auto ownedEnemyCount = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(), world.enemies().end(),
                    [eventIndex, type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == static_cast<int>(eventIndex)
                            && enemy.type() == type;
                    }
                );
            };
            const auto stormglassHazard = std::find_if(
                world.groundHazards().begin(), world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Stormglass Field";
                }
            );
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && ownedEnemyCount(EnemyType::Ranged) == 2
                    && ownedEnemyCount(EnemyType::Warden) == 3
                    && stormglassHazard != world.groundHazards().end(),
                "Stormglass Gauntlet spawns its Lightning pack and field hazard");

            for (int cast = 0; cast < 10
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Stormglass Gauntlet completes after its owned enemies die");

            Input bossInput;
            // Route around the central prism obstacle in Stormscar variant 2.
            const bool reachedBoss = moveToMapEvent(world, bossInput, {1100.0f, 700.0f})
                && moveToBoss(world, bossInput);
            expect(reachedBoss,
                "Stormglass Gauntlet reaches its Boss arena");
            expect(world.bossDefinition().name == "Stormglass Herald"
                    && world.bossDefinition().relicVariant == 2,
                "Stormglass Gauntlet routes to the Stormglass Herald Boss");
            const bool bossDefeated = defeatBossWithAreaSkill(world, bossInput);
            const bool dedicatedRelicDropped = std::any_of(
                world.droppedItems().begin(), world.droppedItems().end(),
                [](const DroppedItem& dropped) {
                    return dropped.item().baseId == "boss.stormglass-lens";
                }
            );
            expect(bossDefeated && dedicatedRelicDropped,
                "Stormglass Herald drops the dedicated Stormglass Lens relic");
        }
    }

    {
        GameWorld world(21014);
        prepareCombinationFixture(world, path, 3, 2, 9, 0);
        const auto eventIt = std::find_if(
            world.map().events().begin(), world.map().events().end(),
            [](const MapEventInstance& event) {
                return event.type == MapEventType::Combination
                    && event.encounterType == MapEncounterType::FrostveilCitadel;
            }
        );
        expect(eventIt != world.map().events().end(),
            "map level nine exposes the Frostveil Citadel encounter");
        if (eventIt != world.map().events().end()) {
            const std::size_t eventIndex = static_cast<std::size_t>(
                std::distance(world.map().events().begin(), eventIt)
            );
            const Vector2 position = eventIt->position;
            Input input;
            expect(moveToMapEvent(world, input, position),
                "player can reach the Frostveil Citadel encounter");
            const auto ownedEnemyCount = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(), world.enemies().end(),
                    [eventIndex, type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == static_cast<int>(eventIndex)
                            && enemy.type() == type;
                    }
                );
            };
            const auto frostveilHazard = std::find_if(
                world.groundHazards().begin(), world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Frostveil Ward";
                }
            );
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && ownedEnemyCount(EnemyType::Warden) == 2
                    && ownedEnemyCount(EnemyType::Charger) == 3
                    && frostveilHazard != world.groundHazards().end(),
                "Frostveil Citadel spawns its Cold pack and frozen ward");

            for (int cast = 0; cast < 10
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Frostveil Citadel completes after its owned enemies die");

            Input bossInput;
            const bool reachedBoss = moveToBoss(world, bossInput);
            expect(reachedBoss
                    && world.bossDefinition().name == "Frostveil Regent"
                    && world.bossDefinition().relicVariant == 2,
                "Frostveil Citadel routes to the Frostveil Regent Boss");
            const bool bossDefeated = defeatBossWithAreaSkill(world, bossInput);
            const bool dedicatedRelicDropped = std::any_of(
                world.droppedItems().begin(), world.droppedItems().end(),
                [](const DroppedItem& dropped) {
                    return dropped.item().baseId == "boss.permafrost-diadem";
                }
            );
            expect(bossDefeated && dedicatedRelicDropped,
                "Frostveil Regent drops the dedicated Permafrost Diadem relic");
        }
    }

    {
        GameWorld world(21015);
        prepareCombinationFixture(world, path, 5, 1, 12, 0);
        const auto eventIt = std::find_if(
            world.map().events().begin(), world.map().events().end(),
            [](const MapEventInstance& event) {
                return event.type == MapEventType::Combination
                    && event.encounterType == MapEncounterType::CinderwakeCrucible;
            }
        );
        expect(eventIt != world.map().events().end(),
            "map level twelve exposes the Cinderwake Crucible encounter");
        if (eventIt != world.map().events().end()) {
            const std::size_t eventIndex = static_cast<std::size_t>(
                std::distance(world.map().events().begin(), eventIt)
            );
            const Vector2 position = eventIt->position;
            Input input;
            expect(moveToMapEvent(world, input, {650.0f, 700.0f})
                    && moveToMapEvent(world, input, position),
                "player can reach the Cinderwake Crucible encounter");
            const auto ownedEnemyCount = [&](EnemyType type) {
                return std::count_if(
                    world.enemies().begin(), world.enemies().end(),
                    [eventIndex, type](const Enemy& enemy) {
                        return enemy.mapEventIndex() == static_cast<int>(eventIndex)
                            && enemy.type() == type;
                    }
                );
            };
            const auto cinderwakeHazard = std::find_if(
                world.groundHazards().begin(), world.groundHazards().end(),
                [](const GroundHazard& hazard) {
                    return hazard.definition().source == "Cinderwake Ward";
                }
            );
            expect(world.activeEliteEventEnemiesRemaining() == 5
                    && ownedEnemyCount(EnemyType::Charger) == 2
                    && ownedEnemyCount(EnemyType::Summoner) == 3
                    && cinderwakeHazard != world.groundHazards().end(),
                "Cinderwake Crucible spawns its Fire pack and burning ward");

            for (int cast = 0; cast < 10
                && world.activeEliteEventEnemiesRemaining() > 0; ++cast) {
                const Vector2 camera = world.cameraTopLeft();
                input.handleMousePressed(
                    sf::Mouse::Button::Right,
                    {static_cast<int>(std::lround(position.x - camera.x)),
                     static_cast<int>(std::lround(position.y - camera.y))}
                );
                world.update(0.05f, input);
                resolvePendingSkillEffects(world, input);
                for (int frame = 0; frame < 35; ++frame) {
                    world.update(0.05f, input);
                }
            }
            expect(world.activeEliteEventEnemiesRemaining() == 0
                    && world.mapEventsCompleted() == 1,
                "Cinderwake Crucible completes after its owned enemies die");

            Input bossInput;
            const bool reachedBoss = moveToMapEvent(world, bossInput, {930.0f, 450.0f})
                && moveToBoss(world, bossInput);
            expect(reachedBoss
                    && world.bossDefinition().name == "Cinderwake Sovereign"
                    && world.bossDefinition().relicVariant == 2,
                "Cinderwake Crucible routes to the Cinderwake Sovereign Boss");
            const bool bossDefeated = defeatBossWithAreaSkill(world, bossInput);
            const bool dedicatedRelicDropped = std::any_of(
                world.droppedItems().begin(), world.droppedItems().end(),
                [](const DroppedItem& dropped) {
                    return dropped.item().baseId == "boss.cinderheart-core";
                }
            );
            expect(bossDefeated && dedicatedRelicDropped,
                "Cinderwake Sovereign drops the dedicated Cinderheart Core relic");
        }
    }

    std::filesystem::remove(path);
}

void testStormRelicAreaChain() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_storm_relic_area_chain_test.bin";
    std::filesystem::remove(path);

    GameWorld world(21009);
    SaveData data;
    std::string error;
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    const auto ringIndex = static_cast<std::size_t>(EquipmentSlot::Ring);
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "Storm Relic area fixture starts from a valid run save");

    data.player.equipment[ringIndex] = makeBaseItem("boss.storm-signet");
    data.skillBar.skills[utilityIndex] = "Pulse";
    data.player.hp = 1000;
    data.player.upgradeStats.maxHp = 1000;
    data.player.upgradeStats.moveSpeedMultiplier = 6.0f;
    data.player.upgradeStats.areaDamageMultiplier = 0.5f;
    data.player.upgradeStats.incomingDamageMultiplier = 0.01f;
    data.player.mana = Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Storm Relic area fixture equips the Lightning relic and Pulse");
    expect(world.bossRelicEffectSummary().find("Storm Chain") != std::string::npos,
        "Storm Relic area fixture exposes the active Storm Chain effect");

    const auto eventIt = std::find_if(
        world.map().events().begin(), world.map().events().end(),
        [](const MapEventInstance& event) { return event.type == MapEventType::ElitePack; }
    );
    expect(eventIt != world.map().events().end(),
        "Storm Relic area fixture finds an ElitePack encounter");
    if (eventIt == world.map().events().end()) {
        std::filesystem::remove(path);
        return;
    }

    Input input;
    expect(moveToMapEvent(world, input, eventIt->position),
        "Storm Relic area fixture reaches the ElitePack encounter");
    expect(world.activeEliteEventEnemiesRemaining() > 0,
        "Storm Relic area fixture starts owned encounter enemies");

    const std::size_t feedbackStart = world.combatFeedback().size();
    input.handleKeyPressed(sf::Keyboard::Key::Q);
    world.update(0.05f, input);
    input.handleKeyReleased(sf::Keyboard::Key::Q);

    const bool chainObserved = std::any_of(
        world.combatFeedback().begin() + static_cast<std::ptrdiff_t>(feedbackStart),
        world.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.source == "Storm Chain"
                && feedback.type == CombatFeedbackType::Damage;
        }
    );
    expect(chainObserved,
        "Lightning Pulse triggers Storm Chain from the real area-damage path");

    std::filesystem::remove(path);
}

void testBloodPriceBleedBurst() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_blood_price_burst_test.bin";
    std::filesystem::remove(path);

    GameWorld world(21010);
    prepareCombinationFixture(world, path, 0, 2, 3, 0);
    SaveData data;
    std::string error;
    expect(SaveService::load(path, data, &error),
        "Blood Price fixture starts from a valid map encounter save");
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] =
        makeBaseItem("boss.gorebound-cleaver");
    expect(SaveService::save(path, data, &error) && world.loadRun(path),
        "Blood Price fixture equips the Bloodletting relic");
    expect(world.bossRelicEffectSummary().find("Blood Price") != std::string::npos,
        "Blood Price appears in the active relic summary");

    const auto eventIt = std::find_if(
        world.map().events().begin(), world.map().events().end(),
        [](const MapEventInstance& event) {
            return event.type == MapEventType::Combination
                && event.encounterType == MapEncounterType::BloodlettingPit;
        }
    );
    expect(eventIt != world.map().events().end(),
        "Blood Price fixture finds the Bloodletting Pit encounter");
    if (eventIt == world.map().events().end()) {
        std::filesystem::remove(path);
        return;
    }

    const std::size_t eventIndex = static_cast<std::size_t>(
        std::distance(world.map().events().begin(), eventIt)
    );
    Input input;
    expect(moveToMapEvent(world, input, eventIt->position),
        "Blood Price fixture reaches the Bloodletting Pit encounter");

    auto sourceIt = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [eventIndex](const Enemy& enemy) {
            return !enemy.isDead()
                && enemy.mapEventIndex() == static_cast<int>(eventIndex);
        }
    );
    auto targetIt = sourceIt == world.enemies().end()
        ? world.enemies().end()
        : std::find_if(
            sourceIt + 1, world.enemies().end(),
            [eventIndex](const Enemy& enemy) {
                return !enemy.isDead()
                    && enemy.mapEventIndex() == static_cast<int>(eventIndex);
            }
        );
    expect(sourceIt != world.enemies().end()
            && targetIt != world.enemies().end(),
        "Blood Price fixture exposes two live encounter enemies");
    if (sourceIt == world.enemies().end() || targetIt == world.enemies().end()) {
        std::filesystem::remove(path);
        return;
    }

    const int targetId = targetIt->id();
    const int targetHpBefore = targetIt->hp();
    const_cast<Enemy&>(*sourceIt).applyBleed(1, 3.0f);
    const_cast<Enemy&>(*sourceIt).kill();
    world.update(0.05f, input);

    const auto updatedTarget = std::find_if(
        world.enemies().begin(), world.enemies().end(),
        [targetId](const Enemy& enemy) { return enemy.id() == targetId; }
    );
    const bool burstFeedback = std::any_of(
        world.combatFeedback().begin(), world.combatFeedback().end(),
        [](const CombatFeedback& feedback) {
            return feedback.source == "Blood Price"
                && feedback.type == CombatFeedbackType::Damage;
        }
    );
    expect(updatedTarget != world.enemies().end()
            && updatedTarget->hp() < targetHpBefore
            && burstFeedback,
        "Blood Price bursts Bleeding enemies onto nearby targets");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    std::cout << "ARPG GameWorld RNG tests\n";

    GameWorld first(4401);
    GameWorld second(4401);
    GameWorld different(4402);
    Input firstInput;
    Input secondInput;
    Input differentInput;

    advanceIntoTheField(first, firstInput);
    advanceIntoTheField(second, secondInput);
    advanceIntoTheField(different, differentInput);
    settleEnemies(first, firstInput);
    settleEnemies(second, secondInput);
    settleEnemies(different, differentInput);

    expect(first.state() == GameState::Playing
            && second.state() == GameState::Playing
            && different.state() == GameState::Playing,
        "identical short runs remain in the Playing state");
    expect(!first.enemies().empty() && !second.enemies().empty(),
        "field simulation produces enemies through the real spawn path");
    expect(first.runSeed() == 4401 && second.runSeed() == 4401
            && different.runSeed() == 4402,
        "GameWorld retains the configured run seed");
    expect(enemySignature(first) == enemySignature(second),
        "same GameWorld seed reproduces enemy encounter state");
    expect(enemySignature(first) != enemySignature(different),
        "different GameWorld seeds change enemy encounter state");

    const std::uint64_t originalSeed = first.runSeed();
    first.reset();
    expect(first.runSeed() != originalSeed, "reset derives a new run seed");
    first.reset(4401);
    expect(first.runSeed() == 4401, "explicit reset restores a requested run seed");

    testSaveLoadRoundTrip();
    testResourceStatsSaveLoad();
    testCorruptLoadDoesNotMutate();
    testMapCompleteLoad();
    testPauseContextsAndFreeze();
    testBossGateProgression();
    testRareLeaderCombatEffects();
    testRareLeaderRewardProfiles();
    testContinuousMapProgression();
    testStoredMapDeviceFlow();
    testItemBaseLevelRequirementWorldFlow();
    testFiveMapRealBossProgression();
    testBossSpawnUsesMapScaling();
    testGameOverRestartBoundary();
    testInvalidProgressionSaveDoesNotMutate();
    testCombatFeedbackAndDeathClaim();
    testSkillFailureFeedback();
    testDelayedSkillEffects();
    testUtilitySkillDelivery();
    testPlayerMinionWorldFlow();
    testPulseShockFlow();
    testRendingVolleyBleedFlow();
    testCrimsonSweepBleedFlow();
    testSiphonPulseRecovery();
    testGuardingPulseProtection();
    testManaWardProtection();
    testIgniteFeedbackMatchesWorldDamage();
    testIgniteDeathSpreadWorldFlow();
    testBuildMathMatchesWorldHits();
    testExpandedSkillWorldHits();
    testBossRelicEffectsInWorld();
    testBossCombatFlow();
    testThemedBossPhaseHazards();
    testElementalEnemyProjectileFlow();
    testThemeEnemyElementalAttacks();
    testManaFlaskFlow();
    testElitePackEventFlow();
    testCombinationMapEvents();
    testFreezeShatterWorldFlow();
    testStormRelicAreaChain();
    testBloodPriceBleedBurst();

    std::cout << "Passed: " << (checks - failures)
        << "  Failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
