#pragma once

#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "MapInstance.hpp"
#include "MapModifier.hpp"

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

    static std::string displayName(const MapItem& map) {
        return "T" + std::to_string(map.mapLevel) + " "
            + MapTemplateLibrary::forIndex(map.option.templateIndex).name;
    }

    static std::string summary(const MapItem& map) {
        return displayName(map) + " | " + map.option.modifier.name
            + " | " + map.option.rewardDescription;
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
        return id;
    }
};

class MapAtlas {
public:
    void clear() {
        completedMapIds_.clear();
    }

    void record(const MapItem& map) {
        completedMapIds_.insert(map.id);
    }

    bool contains(const std::string& mapId) const {
        return completedMapIds_.find(mapId) != completedMapIds_.end();
    }

    int completedCount() const {
        return static_cast<int>(completedMapIds_.size());
    }

    const std::set<std::string>& completedMapIds() const {
        return completedMapIds_;
    }

    void restore(std::set<std::string> completedMapIds) {
        completedMapIds_ = std::move(completedMapIds);
    }

private:
    std::set<std::string> completedMapIds_;
};
