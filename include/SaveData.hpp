#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "MapInstance.hpp"
#include "MapModifier.hpp"
#include "MapRewardLibrary.hpp"
#include "Player.hpp"
#include "SkillBar.hpp"

enum class SavedRunState {
    Playing,
    MapComplete
};

struct SavedDroppedItem {
    Vector2 position;
    Item item;
};

struct SavedMapEvent {
    MapEventType type = MapEventType::LootCache;
    bool triggered = false;
    bool completed = false;
};

struct SaveData {
    static constexpr std::uint32_t Magic = 0x4D415247U;
    static constexpr std::uint32_t Version = 12U;

    SavedRunState state = SavedRunState::Playing;
    std::uint64_t runSeed = 0;
    std::string randomEngineState;

    int mapLevel = 1;
    int mapTemplateIndex = 0;
    int mapLayoutIndex = 0;
    MapOption currentMapOption;
    std::array<MapOption, 3> nextMapOptions{};
    std::array<MapRewardDefinition, 3> mapRewardOptions{};
    int selectedNextMapOption = -1;
    int selectedMapRewardOption = -1;
    bool nextMapOptionChosen = false;
    bool mapRewardChosen = false;

    int score = 0;
    float survivalTime = 0.0f;
    int mapKills = 0;
    int mapExperienceGained = 0;
    int mapItemsDropped = 0;
    int mapBossItemsDropped = 0;
    int mapItemsPickedUp = 0;
    std::array<int, 4> mapDroppedItemsByRarity{};
    int mapRareLeadersDefeated = 0;
    int mapRareLeaderItemsDropped = 0;
    std::string lastRareLeaderName;
    std::string lastRareLeaderRewardDescription;
    int fieldPacksCleared = 0;
    int lifeFlaskCharges = 0;

    std::set<std::string> unlockedSkills;
    std::set<std::string> unlockedSupports;
    std::map<std::string, int> skillLevels;
    std::map<std::string, int> supportLevels;
    float itemQuantityRewardMultiplier = 1.0f;
    int forgeFragments = 0;

    PlayerSaveState player;
    SkillBarSaveState skillBar;
    std::vector<Item> inventory;
    std::vector<Item> stash;
    std::vector<SavedDroppedItem> droppedItems;
    std::vector<SavedMapEvent> mapEvents;
    std::vector<unsigned char> exploredCells;
};
