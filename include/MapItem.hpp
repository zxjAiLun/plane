#pragma once

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AtlasPassiveTree.hpp"
#include "MapInstance.hpp"
#include "MapModifier.hpp"
#include "RandomService.hpp"

// A map item is the persistent form of a post-boss map choice.  The option
// keeps the complete composed modifier so entering a stored map cannot drift
// from the preview shown at settlement time.
struct MapItem {
    std::string id;
    int mapLevel = 1;
    int layoutIndex = 0;
    MapOption option;
};

class MapItemLibrary {
public:
    static MapItem fromOption(
        const MapOption& option,
        int mapLevel,
        int layoutIndex
    ) {
        MapItem result;
        result.mapLevel = std::max(1, mapLevel);
        result.layoutIndex = MapLayoutLibrary::normalizeVariantIndex(layoutIndex);
        result.option = option;
        result.id = makeId(result);
        return result;
    }

    static MapItem rollFromOption(
        const MapOption& baseOption,
        int mapLevel,
        int layoutIndex,
        RandomService& random
    ) {
        MapOption option = baseOption;
        const int level = std::max(1, mapLevel);
        const int rareChance = std::min(35, 10 + level * 2);
        const int magicChance = std::min(60, 42 + level);
        const int roll = random.nextInt(0, 99);
        if (roll < rareChance) {
            option.rarity = MapRarity::Rare;
            option.quality = random.nextInt(12, 20);
        } else if (roll < rareChance + magicChance) {
            option.rarity = MapRarity::Magic;
            option.quality = random.nextInt(5, 12);
        } else {
            option.rarity = MapRarity::Normal;
            option.quality = 0;
        }

        option.explicitAffixIds.clear();
        const int affixCount = option.rarity == MapRarity::Rare
            ? random.nextInt(3, 4)
            : option.rarity == MapRarity::Magic ? random.nextInt(1, 2) : 0;
        std::vector<std::string> available;
        available.reserve(MapModifierLibrary::mapItemAffixes().size());
        for (const auto& definition : MapModifierLibrary::mapItemAffixes()) {
            available.push_back(definition.id);
        }
        for (int index = 0; index < affixCount && !available.empty(); ++index) {
            const std::size_t choice = random.nextIndex(available.size());
            option.explicitAffixIds.push_back(available[choice]);
            available.erase(available.begin() + static_cast<std::ptrdiff_t>(choice));
        }

        return fromOption(option, level, layoutIndex);
    }

    static MapModifier modifierFor(const MapOption& option) {
        MapModifier result = option.modifier;
        for (const auto& affixId : option.explicitAffixIds) {
            MapModifierLibrary::applyMapItemAffix(result, affixId);
        }
        if (option.quality > 0) {
            const float quality = static_cast<float>(option.quality) / 100.0f;
            result.itemQuantityMultiplier *= 1.0f + quality;
            result.itemRarityMultiplier *= 1.0f + quality * 0.75f;
            result.eventRewardMultiplier *= 1.0f + quality * 0.50f;
        }
        return result;
    }

    static bool validOptionMetadata(const MapOption& option) {
        if (static_cast<int>(option.rarity) < static_cast<int>(MapRarity::Normal)
            || static_cast<int>(option.rarity) > static_cast<int>(MapRarity::Rare)
            || option.quality < 0 || option.quality > 20
            || (option.rarity == MapRarity::Normal && option.quality != 0)
            || option.explicitAffixIds.size() > 4) {
            return false;
        }
        std::set<std::string> uniqueAffixes;
        for (const auto& affixId : option.explicitAffixIds) {
            if (MapModifierLibrary::findMapItemAffix(affixId) == nullptr
                || !uniqueAffixes.insert(affixId).second) {
                return false;
            }
        }
        if (option.rarity == MapRarity::Normal && !option.explicitAffixIds.empty()) {
            return false;
        }
        if (option.rarity == MapRarity::Magic
            && (option.explicitAffixIds.empty() || option.explicitAffixIds.size() > 2)) {
            return false;
        }
        return option.rarity != MapRarity::Rare
            || (option.explicitAffixIds.size() >= 3
                && option.explicitAffixIds.size() <= 4);
    }

    static std::string affixSummary(const MapOption& option) {
        std::string summary;
        for (const auto& affixId : option.explicitAffixIds) {
            const auto* definition = MapModifierLibrary::findMapItemAffix(affixId);
            if (definition == nullptr) {
                continue;
            }
            if (!summary.empty()) {
                summary += ", ";
            }
            summary += definition->name;
        }
        return summary.empty() ? "No explicit modifiers" : summary;
    }

    static std::string displayName(const MapItem& map) {
        return std::string(mapRarityName(map.option.rarity)) + " T"
            + std::to_string(map.mapLevel) + " "
            + MapTemplateLibrary::forIndex(map.option.templateIndex).name;
    }

    static std::string summary(const MapItem& map) {
        return displayName(map) + " | " + map.option.modifier.name
            + " | Q" + std::to_string(map.option.quality) + " | "
            + affixSummary(map.option) + " | " + map.option.rewardDescription;
    }

private:
    static std::string makeId(const MapItem& map) {
        std::string id = "map:" + std::to_string(map.mapLevel) + ":"
            + std::to_string(map.option.templateIndex) + ":"
            + std::to_string(map.layoutIndex);
        for (int index = 0; index < map.option.modifier.componentCount; ++index) {
            id += ":" + map.option.modifier.components[
                static_cast<std::size_t>(index)
            ].id;
        }
        id += ":" + map.option.modifier.elementalChallengeId;
        if (map.option.rarity != MapRarity::Normal
            || map.option.quality != 0
            || !map.option.explicitAffixIds.empty()) {
            id += ":" + std::to_string(static_cast<int>(map.option.rarity));
            id += ":" + std::to_string(map.option.quality);
            for (const auto& affixId : map.option.explicitAffixIds) {
                id += ":" + affixId;
            }
        }
        return id;
    }
};

struct AtlasBonuses {
    float itemQuantityMultiplier = 1.0f;
    float itemRarityMultiplier = 1.0f;
    int eliteWeightBonus = 0;
    int bossDropBonus = 0;
    int ironheartBossDropBonus = 0;
};

class MapAtlas {
public:
    void clear() {
        completedMapIds_.clear();
        allocatedNodes_.clear();
    }

    bool record(const MapItem& map) {
        return completedMapIds_.insert(map.id).second;
    }

    bool contains(const std::string& mapId) const {
        return completedMapIds_.find(mapId) != completedMapIds_.end();
    }

    int completedCount() const {
        return static_cast<int>(completedMapIds_.size());
    }

    int atlasPoints() const {
        return completedCount();
    }

    AtlasBonuses bonuses() const {
        const int points = atlasPoints();
        const int quantityPoints = std::min(points, 10);
        const int rarityPoints = std::min(points, 10);
        AtlasBonuses result{
            1.0f + static_cast<float>(quantityPoints) * 0.03f,
            1.0f + static_cast<float>(rarityPoints) * 0.02f,
            points / 2,
            points / 3,
            0
        };
        for (const int nodeIndex : allocatedNodes_) {
            const auto* node = AtlasPassiveLibrary::find(nodeIndex);
            if (node == nullptr) {
                continue;
            }
            result.itemQuantityMultiplier *= node->effect.itemQuantityMultiplierBonus;
            result.itemRarityMultiplier *= node->effect.itemRarityMultiplierBonus;
            result.eliteWeightBonus += node->effect.eliteWeightBonus;
            result.bossDropBonus += node->effect.bossDropBonus;
            result.ironheartBossDropBonus += node->effect.ironheartBossDropBonus;
        }
        return result;
    }

    const std::set<std::string>& completedMapIds() const {
        return completedMapIds_;
    }

    void restore(std::set<std::string> completedMapIds) {
        completedMapIds_ = std::move(completedMapIds);
    }

    int allocatedNodeCount() const {
        return static_cast<int>(allocatedNodes_.size());
    }

    int availablePoints() const {
        return std::max(0, atlasPoints() - allocatedNodeCount());
    }

    bool isNodeAllocated(int nodeIndex) const {
        return allocatedNodes_.find(nodeIndex) != allocatedNodes_.end();
    }

    bool canAllocateNode(int nodeIndex) const {
        const auto* node = AtlasPassiveLibrary::find(nodeIndex);
        if (node == nullptr || isNodeAllocated(nodeIndex) || availablePoints() <= 0) {
            return false;
        }
        return node->prerequisite < 0 || isNodeAllocated(node->prerequisite);
    }

    bool allocateNode(int nodeIndex) {
        if (!canAllocateNode(nodeIndex)) {
            return false;
        }
        allocatedNodes_.insert(nodeIndex);
        return true;
    }

    const std::set<int>& allocatedNodes() const {
        return allocatedNodes_;
    }

    void restoreAllocatedNodes(const std::set<int>& allocatedNodes) {
        allocatedNodes_.clear();
        for (const int nodeIndex : allocatedNodes) {
            if (canAllocateNode(nodeIndex)) {
                allocatedNodes_.insert(nodeIndex);
            }
        }
    }

private:
    std::set<std::string> completedMapIds_;
    std::set<int> allocatedNodes_;
};
