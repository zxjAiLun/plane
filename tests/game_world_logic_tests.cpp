#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "Config.hpp"
#include "CombatMath.hpp"
#include "GameWorld.hpp"
#include "Input.hpp"
#include "LootGenerator.hpp"
#include "MapRewardLibrary.hpp"
#include "SaveService.hpp"
#include "SkillLibrary.hpp"

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

void pressKey(GameWorld& world, Input& input, sf::Keyboard::Key key) {
    input.handleKeyPressed(key);
    world.update(0.05f, input);
    input.handleKeyReleased(key);
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

void testMapCompleteLoad() {
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_map_complete_save_test.bin";
    std::filesystem::remove(path);

    GameWorld source(9101);
    expect(source.saveRun(path), "source run creates a settlement save");
    SaveData data;
    std::string error;
    expect(SaveService::load(path, data, &error), "settlement save can be edited for the fixture");
    data.state = SavedRunState::MapComplete;
    expect(SaveService::save(path, data, &error), "settlement fixture is written");

    GameWorld target(9102);
    expect(target.loadRun(path), "GameWorld loads a MapComplete checkpoint");
    expect(target.state() == GameState::MapComplete
            && target.map().bossDefeated()
            && target.mapObjective() == "Choose Reward",
        "MapComplete load restores the boss-defeated settlement phase");
    Input input;
    pressKey(target, input, sf::Keyboard::Key::Escape);
    expect(target.state() == GameState::Paused,
        "MapComplete can be paused without mutating settlement state");
    pressKey(target, input, sf::Keyboard::Key::Escape);
    expect(target.state() == GameState::MapComplete,
        "MapComplete resumes after Pause");
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
                && world.mapEventsTotal() == 3,
            "next map keeps progression and resets transient map state");
    }

    expect(world.mapLevel() == 6, "five transitions reach map level 6");
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

    expect(!world.combatFeedback().empty(),
        "a real player area skill creates combat feedback on hit");
    if (!world.combatFeedback().empty()) {
        const auto& feedback = world.combatFeedback().back();
        expect(feedback.damage > 0 && !feedback.source.empty()
                && feedback.timeRemaining > 0.0f,
            "combat feedback exposes positive damage, source, and lifetime");
    }

    world.update(Config::CombatFeedbackDuration + 0.05f, input);
    expect(world.combatFeedback().empty(),
        "expired combat feedback is removed from the world");
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
    for (const auto& feedback : world.combatFeedback()) {
        if (feedback.source == "Ignite") {
            igniteFeedbackDamage += feedback.damage;
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
    expect(igniteFeedbackDamage == enemyHpLoss,
        "Ignite feedback damage equals the actual Enemy HP loss");
    expect(enemiesKilledByTick > 0
            && world.mapKills() - killsBeforeTick == enemiesKilledByTick,
        "Ignite-killed enemies receive exactly one normal reward claim each");

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
    input.handleMousePressed(sf::Mouse::Button::Left, worldToScreen(boss->position()));
    for (int frame = 0; frame < 30
            && world.combatFeedback().size() == projectileFeedbackStart; ++frame) {
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
    const int expectedAreaDamage = skillDamage(
        secondary, world.player().stats(), secondarySupports
    );
    const float expectedAreaRadius = skillRadius(
        secondary, world.player().stats(), secondarySupports
    );
    const int bossHpBeforeArea = boss->hp();
    const std::size_t areaFeedbackStart = world.combatFeedback().size();
    input.handleMousePressed(sf::Mouse::Button::Right, worldToScreen(boss->position()));
    world.update(0.05f, input);

    int areaFeedbackDamage = 0;
    bool areaFeedbackMatches = true;
    for (std::size_t i = areaFeedbackStart; i < world.combatFeedback().size(); ++i) {
        const auto& feedback = world.combatFeedback()[i];
        if (feedback.source == secondary.name) {
            areaFeedbackDamage += feedback.damage;
            areaFeedbackMatches = areaFeedbackMatches && feedback.damage == expectedAreaDamage;
        }
    }
    boss = findBoss();
    const int bossHpAfterArea = boss == world.enemies().end() ? 0 : boss->hp();
    expect(areaFeedbackDamage > 0 && areaFeedbackMatches,
        "real Area feedback uses the CombatMath damage value");
    expect(bossHpBeforeArea - bossHpAfterArea == areaFeedbackDamage,
        "real Area feedback equals the Boss HP delta");
    expect(std::abs(world.secondarySkillEffectRadius() - expectedAreaRadius) < 0.001f,
        "real Area effect radius matches the CombatMath radius value");

    std::filesystem::remove(path);
}

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
    data.player.upgradeStats.areaDamageMultiplier = 2.0f;
    data.player.mana = data.player.mana > 0.0f ? data.player.mana : Config::PlayerMaxMana;
    data.state = SavedRunState::Playing;
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
    expect(world.state() == GameState::MapComplete && world.map().bossDefeated(),
        "Boss death enters MapComplete through the real reward path");
    expect(world.mapBossItemsDropped() >= 1 && !world.droppedItems().empty(),
        "Boss death creates at least one guaranteed ground drop");

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
    if (world.activeEliteEventEnemiesRemaining() == 5) {
        const Vector2 camera = world.cameraTopLeft();
        const sf::Vector2i screenTarget(
            static_cast<int>(std::lround(eventPosition.x - camera.x)),
            static_cast<int>(std::lround(eventPosition.y - camera.y))
        );
        input.handleMousePressed(sf::Mouse::Button::Right, screenTarget);
        world.update(0.05f, input);
    }

    expect(world.activeEliteEventEnemiesRemaining() == 0,
        "one boosted area cast clears all five ElitePack enemies");
    expect(world.mapEventsCompleted() == 1,
        "ElitePack completes only after all event enemies are defeated");
    expect(world.eventStatusMessage().find("Elite pack cleared") != std::string::npos,
        "ElitePack completion reports nearby loot feedback");

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
    testCorruptLoadDoesNotMutate();
    testMapCompleteLoad();
    testPauseContextsAndFreeze();
    testContinuousMapProgression();
    testInvalidProgressionSaveDoesNotMutate();
    testCombatFeedbackAndDeathClaim();
    testIgniteFeedbackMatchesWorldDamage();
    testBuildMathMatchesWorldHits();
    testBossCombatFlow();
    testElitePackEventFlow();

    std::cout << "Passed: " << (checks - failures)
        << "  Failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
