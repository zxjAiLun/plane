#include <algorithm>
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
#include "GameWorld.hpp"
#include "Input.hpp"
#include "ItemBase.hpp"
#include "LootGenerator.hpp"
#include "MapRewardLibrary.hpp"
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
        && first.armor == second.armor
        && first.projectileCountBonus == second.projectileCountBonus
        && std::abs(first.lifeFlaskEffectMultiplier - second.lifeFlaskEffectMultiplier) < 0.0001f
        && std::abs(first.itemQuantityMultiplier - second.itemQuantityMultiplier) < 0.0001f
        && std::abs(first.incomingDamageMultiplier - second.incomingDamageMultiplier) < 0.0001f;
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
                && world.mapEventsTotal() == 3,
            "next map keeps progression and resets transient map state");
    }

    expect(world.mapLevel() == 6, "five transitions reach map level 6");
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
                && world.mapEventsTotal() == 3,
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
            && world.mapEventsTotal() == 3
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

void testExpandedSkillWorldHits() {
    const auto path = std::filesystem::temp_directory_path()
        / "plane_fight_expanded_skill_world_test.bin";
    std::filesystem::remove(path);

    GameWorld world(20001);
    expect(!world.isSkillUnlocked("Arc Bolt")
            && !world.isSkillUnlocked("Shockwave")
            && !world.isSupportUnlocked("Barrage")
            && !world.isSupportUnlocked("Concentration"),
        "expanded skills and Supports start locked");

    SaveData data;
    std::string error;
    expect(world.saveRun(path) && SaveService::load(path, data, &error),
        "expanded skill fixture starts from a valid run save");

    const auto primaryIndex = static_cast<std::size_t>(SkillSlot::Primary);
    const auto utilityIndex = static_cast<std::size_t>(SkillSlot::Utility);
    data.unlockedSkills.insert("Arc Bolt");
    data.unlockedSkills.insert("Shockwave");
    data.unlockedSupports.insert("Barrage");
    data.unlockedSupports.insert("Concentration");
    data.skillBar.skills[primaryIndex] = "Arc Bolt";
    data.skillBar.skills[utilityIndex] = "Shockwave";
    data.skillBar.supports[primaryIndex] = {"Barrage", ""};
    data.skillBar.supports[utilityIndex] = {"Concentration", ""};
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
            && world.skillBar().definition(SkillSlot::Utility).name == "Shockwave"
            && world.skillBar().supportAt(SkillSlot::Primary, 0) != nullptr
            && world.skillBar().supportAt(SkillSlot::Primary, 0)->name == "Barrage"
            && world.skillBar().supportAt(SkillSlot::Utility, 0) != nullptr
            && world.skillBar().supportAt(SkillSlot::Utility, 0)->name == "Concentration",
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
    const int expectedProjectileDamage = skillDamage(
        projectileSkill, world.player().stats(), projectileSupports
    );
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
    bool projectileFeedbackMatches = true;
    for (std::size_t index = projectileFeedbackStart;
        index < world.combatFeedback().size(); ++index) {
        const auto& feedback = world.combatFeedback()[index];
        if (feedback.source == projectileSkill.name) {
            projectileFeedbackDamage += feedback.damage;
            projectileFeedbackMatches = projectileFeedbackMatches
                && feedback.damage == expectedProjectileDamage;
        }
    }
    boss = findBoss();
    const int bossHpAfterProjectile = boss == world.enemies().end() ? 0 : boss->hp();
    expect(projectileFeedbackDamage > 0 && projectileFeedbackMatches,
        "Arc Bolt real feedback uses CombatMath damage");
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
    const int expectedAreaDamage = skillDamage(
        areaSkill, world.player().stats(), areaSupports
    );
    const float expectedAreaRadius = skillRadius(
        areaSkill, world.player().stats(), areaSupports
    );
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
    expect(bossHpBeforeArea - bossHpAfterArea == areaFeedbackDamage,
        "Shockwave feedback equals the Boss HP delta");
    expect(std::abs(world.novaEffectRadius() - expectedAreaRadius) < 0.001f,
        "Shockwave real radius matches CombatMath");
    input.handleKeyReleased(sf::Keyboard::Key::Q);

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
    testItemBaseLevelRequirementWorldFlow();
    testFiveMapRealBossProgression();
    testGameOverRestartBoundary();
    testInvalidProgressionSaveDoesNotMutate();
    testCombatFeedbackAndDeathClaim();
    testIgniteFeedbackMatchesWorldDamage();
    testBuildMathMatchesWorldHits();
    testExpandedSkillWorldHits();
    testBossCombatFlow();
    testElitePackEventFlow();

    std::cout << "Passed: " << (checks - failures)
        << "  Failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
