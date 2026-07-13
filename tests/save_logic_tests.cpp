#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Item.hpp"
#include "MapModifier.hpp"
#include "RandomService.hpp"
#include "SaveService.hpp"
#include "SkillBar.hpp"

namespace {

int failures = 0;

void expect(bool condition, const std::string& label) {
    if (condition) {
        std::cout << "  PASS  " << label << '\n';
    } else {
        ++failures;
        std::cout << "  FAIL  " << label << '\n';
    }
}

std::vector<unsigned char> readBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void writeBytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
}

SaveData sampleData() {
    SaveData data;
    data.state = SavedRunState::MapComplete;
    data.runSeed = 4401;
    RandomService random(data.runSeed);
    random.nextInt(1, 100);
    data.randomEngineState = random.engineState();
    data.mapLevel = 2;
    data.mapTemplateIndex = 1;
    data.mapLayoutIndex = 1;
    data.currentMapOption = MapOptionLibrary::generateOptions(2)[1];
    data.nextMapOptions = MapOptionLibrary::generateOptions(3);
    data.unlockedSkills.insert("Spread Shot");
    data.unlockedSkills.insert("Arc Bolt");
    data.unlockedSupports.insert("Pierce");
    data.unlockedSkills.insert("Meteor");
    data.unlockedSkills.insert("Pulse");
    data.unlockedSkills.insert("Shockwave");
    data.unlockedSkills.insert("Dash");
    data.unlockedSupports.insert("Barrage");
    data.unlockedSupports.insert("Concentration");
    data.skillBar = SkillBar().saveState();
    data.skillBar.skills[0] = "Arc Bolt";
    data.skillBar.skills[2] = "Shockwave";
    data.skillBar.supports[0][0] = "Pierce";
    data.skillBar.supports[0][1] = "Barrage";
    data.skillBar.supports[2][0] = "Concentration";
    data.unlockedSupports.insert("Volley");
    data.skillBar.elapsed[0] = 0.17f;

    Item weapon;
    weapon.name = "Vicious Weapon of Projectiles";
    weapon.slot = EquipmentSlot::Weapon;
    weapon.rarity = Rarity::Rare;
    weapon.itemLevel = 5;
    weapon.baseId = "weapon.rustbound-blade";
    weapon.baseName = "Rustbound Blade";
    weapon.implicitStats.damageMultiplier = 1.05f;
    weapon.stats.damageMultiplier = 1.13f;
    weapon.affixes.push_back({
        "Piercing",
        2,
        Stats{0, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f},
        {AffixTag::Projectile},
        "projectile_damage_t2",
        AffixStat::ProjectileDamageMultiplier,
        true
    });
    data.player = Player().saveState();
    data.player.equipment[static_cast<std::size_t>(EquipmentSlot::Weapon)] = weapon;
    data.inventory.push_back(weapon);
    data.stash.push_back(weapon);
    data.droppedItems.push_back({{500.0f, 500.0f}, weapon});
    data.itemQuantityRewardMultiplier = 1.15f;
    data.forgeFragments = 7;
    data.mapEvents = {
        {MapEventType::LootCache, true, true},
        {MapEventType::ElitePack, true, true},
        {MapEventType::Shrine, true, true}
    };
    data.exploredCells = {1, 0, 1, 1};
    return data;
}

void testRandomState() {
    RandomService first(901);
    first.nextInt(0, 1000);
    const std::string state = first.engineState();
    const int expected = first.nextInt(0, 1000);

    RandomService restored(901);
    expect(restored.restoreEngineState(state), "RNG state can be restored");
    expect(restored.nextInt(0, 1000) == expected,
        "RNG continues from the exact saved position");
}

void testFileValidation(const std::filesystem::path& path) {
    SaveData data = sampleData();
    std::string error;
    expect(SaveService::save(path, data, &error), "valid save writes atomically");

    SaveData restored;
    const bool loaded = SaveService::load(path, restored, &error);
    if (!loaded) {
        std::cout << "    load error: " << error << '\n';
    }
    expect(loaded, "valid save round-trips");
    expect(restored.runSeed == data.runSeed
            && restored.mapLevel == data.mapLevel
            && restored.currentMapOption.modifier.name
                == data.currentMapOption.modifier.name
            && restored.unlockedSupports == data.unlockedSupports
            && restored.exploredCells == data.exploredCells
            && restored.player.equipment[0]->affixes[0].id
                == "projectile_damage_t2"
            && restored.skillBar.skills[0] == "Arc Bolt"
            && restored.skillBar.skills[2] == "Shockwave"
            && restored.skillBar.supports[0][0] == "Pierce"
            && restored.skillBar.supports[0][1] == "Barrage"
            && restored.skillBar.supports[2][0] == "Concentration"
            && restored.inventory.size() == 1
            && restored.stash.size() == 1
            && restored.droppedItems.size() == 1,
        "round-trip preserves run, map, progression and exploration");

    std::vector<unsigned char> bytes = readBytes(path);
    expect(bytes.size() > 16, "save has a versioned header and payload");

    bytes.back() ^= 0x01;
    writeBytes(path, bytes);
    expect(!SaveService::load(path, restored, &error),
        "CRC rejects a modified payload");

    expect(SaveService::save(path, data, &error), "test save can be rewritten");
    bytes = readBytes(path);
    bytes[4] = 1;
    writeBytes(path, bytes);
    expect(!SaveService::load(path, restored, &error),
        "legacy v1 save is rejected after the multi-link schema change");

    expect(SaveService::save(path, data, &error), "test save can be rewritten after version check");
    bytes = readBytes(path);
    bytes[4] = 99;
    writeBytes(path, bytes);
    expect(!SaveService::load(path, restored, &error),
        "unknown schema version is rejected");

    expect(SaveService::save(path, data, &error), "test save can be restored again");
    bytes = readBytes(path);
    bytes.resize(bytes.size() / 2);
    writeBytes(path, bytes);
    expect(!SaveService::load(path, restored, &error),
        "truncated save is rejected");

    const auto missingPath = path.string() + ".missing";
    expect(!SaveService::load(missingPath, restored, &error),
        "missing save is reported as invalid");

    const auto directoryPath = path.string() + ".directory";
    std::filesystem::create_directories(directoryPath);
    expect(!SaveService::save(directoryPath, data, &error),
        "failed atomic replacement does not report success");
    expect(std::filesystem::is_directory(directoryPath),
        "failed replacement leaves the existing target untouched");
    std::filesystem::remove_all(directoryPath);
}

} // namespace

int main() {
    std::cout << "ARPG save logic tests\n";
    const auto path = std::filesystem::temp_directory_path() / "plane_fight_save_logic_test.bin";
    std::filesystem::remove(path);
    testRandomState();
    testFileValidation(path);
    std::filesystem::remove(path);
    std::cout << "Passed: " << (failures == 0 ? 11 : 11 - failures)
        << "  Failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
