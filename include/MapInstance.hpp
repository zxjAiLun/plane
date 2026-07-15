#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "EnemyType.hpp"
#include "GroundHazard.hpp"
#include "LootBias.hpp"
#include "MapExploration.hpp"
#include "MapLayout.hpp"
#include "Vector2.hpp"

enum class MapArea {
    Start,
    Field,
    BossGate,
    BossArena,
    BossDefeated
};

enum class MapEventType {
    LootCache,
    ElitePack,
    Shrine,
    Combination
};

enum class MapEncounterType {
    None,
    EnhancedCache,
    HazardousElitePack,
    GuardedShrine,
    BountyHunt,
    CursedReliquary,
    WardenCourt,
    FrozenReliquary
};

struct MapEncounterDefinition {
    MapEncounterType type = MapEncounterType::None;
    std::string id;
    std::string name;
    std::string description;
    float radius = 105.0f;
    int cacheDropCount = 0;
    int eliteCount = 0;
    int normalCount = 0;
    float rewardMultiplier = 1.0f;
    GroundHazardDefinition hazard;
    bool requiresGuardClearance = false;
    int completionDropCount = 0;
    EnemyType primaryEnemyType = EnemyType::Elite;
    EnemyType secondaryEnemyType = EnemyType::Normal;
    LootBias rewardLootBias;
    int forgeFragmentReward = 0;
};

class MapEncounterLibrary {
public:
    static const std::array<MapEncounterDefinition, 7>& all() {
        static const std::array<MapEncounterDefinition, 7> definitions = {{
            {
                MapEncounterType::EnhancedCache,
                "enhanced-cache",
                "Enhanced Cache",
                "A richer cache with additional item quantity",
                105.0f,
                3,
                0,
                0,
                1.15f,
                {},
                false,
                0,
                EnemyType::Elite,
                EnemyType::Normal,
                {},
                1
            },
            {
                MapEncounterType::HazardousElitePack,
                "hazardous-elite-pack",
                "Hazardous Elite Pack",
                "An Elite pack protected by a lingering ground hazard",
                112.0f,
                0,
                1,
                4,
                1.0f,
                {"Ashen Trap", 115.0f, 8.0f, 0.5f, 2},
                false,
                0,
                EnemyType::Elite,
                EnemyType::Normal,
                {},
                2
            },
            {
                MapEncounterType::GuardedShrine,
                "guarded-shrine",
                "Guarded Shrine",
                "Clear the shrine guardians before claiming its blessing",
                108.0f,
                0,
                0,
                4,
                1.0f,
                {},
                true,
                0,
                EnemyType::Elite,
                EnemyType::Normal,
                {},
                2
            },
            {
                MapEncounterType::BountyHunt,
                "bounty-hunt",
                "Bounty Hunt",
                "Hunt a reinforced pack for bonus map loot",
                118.0f,
                0,
                2,
                3,
                1.25f,
                {},
                false,
                2,
                EnemyType::Elite,
                EnemyType::Normal,
                {},
                2
            },
            {
                MapEncounterType::CursedReliquary,
                "cursed-reliquary",
                "Cursed Reliquary",
                "Unseal a cursed cache and defeat its guardians for a larger reward",
                122.0f,
                0,
                1,
                2,
                1.30f,
                {},
                false,
                4,
                EnemyType::Elite,
                EnemyType::Normal,
                {},
                3
            },
            {
                MapEncounterType::WardenCourt,
                "warden-court",
                "Warden Court",
                "Break a defensive formation of Wardens and Hexbinders",
                126.0f,
                0,
                2,
                2,
                1.35f,
                {},
                false,
                3,
                EnemyType::Warden,
                EnemyType::Summoner,
                {},
                3
            },
            {
                MapEncounterType::FrozenReliquary,
                "frozen-reliquary",
                "Frozen Reliquary",
                "Break the ice seal and defeat its wardens for a Cold-biased reward",
                124.0f,
                0,
                1,
                3,
                1.40f,
                {"Rime Sigil", 125.0f, 8.0f, 0.75f, 3,
                    DamageType::Cold, {AilmentType::Chill, 2.5f, 0.0f, 0.55f}},
                false,
                3,
                EnemyType::Elite,
                EnemyType::Warden,
                {AffixTag::Cold, 1.80f, AffixTag::Area, 1.15f},
                4
            }
        }};
        return definitions;
    }

    static const MapEncounterDefinition& forType(MapEncounterType type) {
        for (const auto& definition : all()) {
            if (definition.type == type) {
                return definition;
            }
        }
        static const MapEncounterDefinition none;
        return none;
    }

    static const MapEncounterDefinition& forMap(
        int mapLevel,
        int templateIndex,
        int layoutIndex
    ) {
        const int templateCount = MapLayoutLibrary::TemplateCount;
        const int normalizedTemplate = ((templateIndex % templateCount) + templateCount)
            % templateCount;
        if (normalizedTemplate == templateCount - 1) {
            return all().back();
        }

        constexpr int LegacyEncounterCount = 6;
        const int normalizedLevel = std::max(1, mapLevel) - 1;
        const int index = ((normalizedLevel + normalizedTemplate + layoutIndex)
                % LegacyEncounterCount + LegacyEncounterCount) % LegacyEncounterCount;
        return all()[static_cast<std::size_t>(index)];
    }
};

struct MapEventInstance {
    MapEventType type = MapEventType::LootCache;
    Vector2 position;
    float radius = 70.0f;
    bool triggered = false;
    bool completed = false;
    MapEncounterType encounterType = MapEncounterType::None;
    std::string encounterId;
};

struct MapColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

struct MapPalette {
    MapColor floor;
    MapColor obstacle;
    MapColor bossGate;
    MapColor bossArena;
    MapColor startArea;
};

struct MapEncounterProfile {
    int normalWeight = 65;
    int rangedWeight = 25;
    int eliteWeight = 10;
    std::string threatDescription = "Balanced melee packs";
    int chargerWeight = 0;
    int wardenWeight = 0;
    int summonerWeight = 0;
};

struct MapAmbientEffectDefinition {
    std::string name;
    std::string description;
    float interval = 0.0f;
    float telegraphDuration = 0.0f;
    GroundHazardDefinition hazard;

    bool isValid() const {
        return !name.empty()
            && interval > 0.0f
            && telegraphDuration > 0.0f
            && hazard.isValid();
    }
};

struct MapTemplateDefinition {
    std::string name;
    std::string theme;
    MapPalette palette;
    MapEncounterProfile encounter;
    MapAmbientEffectDefinition ambientEffect;
};

class MapTemplateLibrary {
public:
    static const std::vector<MapTemplateDefinition>& all() {
        static const std::vector<MapTemplateDefinition> templates = buildTemplates();
        return templates;
    }

    static const MapTemplateDefinition& forMapLevel(int mapLevel) {
        const auto& templates = all();
        const int normalizedLevel = std::max(1, mapLevel);
        const auto index = static_cast<std::size_t>(
            (normalizedLevel - 1) % static_cast<int>(templates.size())
        );
        return templates[index];
    }

    static const MapTemplateDefinition& forIndex(int templateIndex) {
        const auto& templates = all();
        const int count = static_cast<int>(templates.size());
        const int normalizedIndex = ((templateIndex % count) + count) % count;
        return templates[static_cast<std::size_t>(normalizedIndex)];
    }

private:
    static std::vector<MapTemplateDefinition> buildTemplates() {
        return {
            {
                "Ashen Causeway",
                "Ash and stone",
                {{24, 28, 30}, {65, 70, 72}, {120, 70, 40}, {120, 35, 35}, {40, 110, 70}},
                {50, 20, 15, "Mixed melee, charger and Warden patrols", 10, 5, 0},
                {
                    "Falling Cinders",
                    "Fire sigils periodically mark the field",
                    12.0f,
                    0.65f,
                    {"Cinderfall", 95.0f, 4.0f, 0.75f, 2,
                        DamageType::Fire,
                        {AilmentType::Ignite, 2.5f, 0.20f}}
                }
            },
            {
                "Stormscar Expanse",
                "Rain and shattered glass",
                {{20, 29, 38}, {52, 72, 92}, {75, 115, 145}, {46, 72, 125}, {42, 95, 110}},
                {12, 45, 15, "Ranged pressure, chargers and Summoner anchors", 15, 8, 5},
                {
                    "Storm Strike",
                    "Lightning marks periodically land in the field",
                    10.0f,
                    0.80f,
                    {"Arc Flash", 100.0f, 3.5f, 0.70f, 2,
                        DamageType::Lightning,
                        {AilmentType::Shock, 2.0f, 0.0f, 1.0f, 0, 0, 1.15f}}
                }
            },
            {
                "Venom Hollow",
                "Acid and overgrowth",
                {{23, 38, 31}, {55, 82, 61}, {105, 125, 55}, {92, 68, 35}, {42, 110, 70}},
                {20, 20, 25, "Elite patrols, chargers and Summoner anchors", 15, 10, 10},
                {
                    "Toxic Bloom",
                    "Poison spores linger after a field warning",
                    11.0f,
                    0.70f,
                    {"Toxic Bloom", 110.0f, 5.0f, 0.75f, 2,
                        DamageType::Poison,
                        {AilmentType::Poison, 3.0f}}
                }
            },
            {
                "Frostbound Pass",
                "Snow and fractured ice",
                {{28, 38, 52}, {90, 115, 135}, {110, 180, 225}, {55, 120, 165}, {55, 105, 135}},
                {14, 16, 18, "Warden formations, chargers and cold-forged elites", 20, 20, 12},
                {
                    "Rimefall",
                    "Cold sigils slow the field before they erupt",
                    9.0f,
                    0.70f,
                    {"Rimefall", 105.0f, 4.5f, 0.75f, 2,
                        DamageType::Cold,
                        {AilmentType::Chill, 2.5f, 0.0f, 0.60f}}
                }
            },
        };
    }
};

class MapInstance {
public:
    explicit MapInstance(int mapLevel = 1, int templateIndex = -1, int layoutIndex = -1)
        : size_(Config::MapWidth, Config::MapHeight)
        , playerStart_(220.0f, Config::MapHeight - 220.0f)
        , bossCenter_(Config::MapWidth - 320.0f, 300.0f)
        , exploration_(size_)
        , mapLevel_(std::max(1, mapLevel))
        , templateIndex_(templateIndex >= 0
            ? MapLayoutLibrary::normalizeTemplateIndex(templateIndex)
            : MapLayoutLibrary::normalizeTemplateIndex(std::max(1, mapLevel) - 1))
        , layoutIndex_(layoutIndex >= 0
            ? MapLayoutLibrary::normalizeVariantIndex(layoutIndex)
            : MapLayoutLibrary::variantForMapLevel(mapLevel))
        , templateDefinition_(&MapTemplateLibrary::forIndex(templateIndex_))
        , layoutDefinition_(&MapLayoutLibrary::forTemplate(templateIndex_, layoutIndex_))
        , encounterDefinition_(&MapEncounterLibrary::forMap(
            mapLevel_, templateIndex_, layoutIndex_
        ))
        , bossTriggered_(false)
        , bossDefeated_(false) {
        generateObstacles();
        generateEvents();
        exploration_.revealAround(playerStart_);
    }

    const Vector2& size() const { return size_; }
    const Vector2& playerStart() const { return playerStart_; }
    const Vector2& bossCenter() const { return bossCenter_; }
    int mapLevel() const { return mapLevel_; }
    const MapTemplateDefinition& definition() const { return *templateDefinition_; }
    const MapLayoutDefinition& layoutDefinition() const { return *layoutDefinition_; }
    const MapEncounterDefinition& encounterDefinition() const { return *encounterDefinition_; }
    int templateIndex() const { return templateIndex_; }
    int layoutIndex() const { return layoutIndex_; }
    const std::string& layoutId() const { return layoutDefinition_->id; }
    bool bossTriggered() const { return bossTriggered_; }
    bool bossDefeated() const { return bossDefeated_; }
    const std::vector<MapEventInstance>& events() const { return events_; }
    std::vector<MapEventInstance>& eventsForMutation() { return events_; }
    const std::vector<MapObstacle>& obstacles() const { return obstacles_; }
    const MapExploration& exploration() const { return exploration_; }
    void revealAround(const Vector2& position) { exploration_.revealAround(position); }
    void resetExploration() {
        exploration_.reset();
        exploration_.revealAround(playerStart_);
    }

    bool restoreExploration(const std::vector<unsigned char>& cells) {
        return exploration_.restoreRevealedCells(cells);
    }

    bool geometryIsValid(float playerRadius = Config::PlayerRadius) const {
        for (const auto& obstacle : obstacles_) {
            const float minX = obstacle.center.x - obstacle.halfExtents.x;
            const float maxX = obstacle.center.x + obstacle.halfExtents.x;
            const float minY = obstacle.center.y - obstacle.halfExtents.y;
            const float maxY = obstacle.center.y + obstacle.halfExtents.y;
            if (minX < 0.0f || maxX > size_.x || minY < 0.0f || maxY > size_.y) {
                return false;
            }
        }

        if (intersectsObstacle(playerStart_, playerRadius)
            || intersectsObstacle(bossCenter_, Config::BossArenaRadius)) {
            return false;
        }

        for (const auto& event : events_) {
            const float startSafeDistance = Config::StartSafeRadius + event.radius;
            const float bossArenaDistance = Config::BossArenaRadius + event.radius;
            if (event.position.x < event.radius
                || event.position.x > size_.x - event.radius
                || event.position.y < event.radius
                || event.position.y > size_.y - event.radius
                || (event.position - playerStart_).lengthSquared()
                    <= startSafeDistance * startSafeDistance
                || (event.position - bossCenter_).lengthSquared()
                    <= bossArenaDistance * bossArenaDistance
                || intersectsObstacle(event.position, event.radius)) {
                return false;
            }
        }

        for (std::size_t first = 0; first < events_.size(); ++first) {
            for (std::size_t second = first + 1; second < events_.size(); ++second) {
                const float minimumDistance = events_[first].radius + events_[second].radius;
                if ((events_[first].position - events_[second].position).lengthSquared()
                        < minimumDistance * minimumDistance) {
                    return false;
                }
            }
        }

        return hasReachableBossPath(playerRadius);
    }

    bool hasReachableBossPath(float playerRadius = Config::PlayerRadius, float cellSize = 40.0f) const {
        if (cellSize <= 0.0f) {
            return false;
        }

        const int columns = std::max(1, static_cast<int>(std::ceil(size_.x / cellSize)));
        const int rows = std::max(1, static_cast<int>(std::ceil(size_.y / cellSize)));
        const auto cellIndex = [columns](int x, int y) { return y * columns + x; };
        const auto cellPosition = [cellSize](int x, int y) {
            return Vector2((static_cast<float>(x) + 0.5f) * cellSize,
                (static_cast<float>(y) + 0.5f) * cellSize);
        };
        const auto clampCell = [columns, rows, cellSize](const Vector2& position) {
            return std::pair<int, int>(
                std::clamp(static_cast<int>(position.x / cellSize), 0, columns - 1),
                std::clamp(static_cast<int>(position.y / cellSize), 0, rows - 1)
            );
        };

        // The default grid spacing is fixed at 40px so the result is stable
        // across all normal callers and map levels.
        const auto startCell = clampCell(playerStart_);
        const auto goalPosition = bossCenter_;
        const int startIndex = cellIndex(startCell.first, startCell.second);
        std::vector<bool> visited(static_cast<std::size_t>(columns * rows), false);
        std::queue<std::pair<int, int>> pending;
        if (intersectsObstacle(cellPosition(startCell.first, startCell.second), playerRadius)) {
            return false;
        }
        pending.push(startCell);
        visited[static_cast<std::size_t>(startIndex)] = true;

        constexpr int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!pending.empty()) {
            const auto current = pending.front();
            pending.pop();
            const Vector2 position = cellPosition(current.first, current.second);
            if ((position - goalPosition).lengthSquared()
                    <= Config::BossArenaRadius * Config::BossArenaRadius) {
                return true;
            }

            for (const auto& direction : directions) {
                const int nextX = current.first + direction[0];
                const int nextY = current.second + direction[1];
                if (nextX < 0 || nextX >= columns || nextY < 0 || nextY >= rows) {
                    continue;
                }

                const int nextIndex = cellIndex(nextX, nextY);
                if (visited[static_cast<std::size_t>(nextIndex)]
                    || intersectsObstacle(cellPosition(nextX, nextY), playerRadius)) {
                    continue;
                }
                visited[static_cast<std::size_t>(nextIndex)] = true;
                pending.emplace(nextX, nextY);
            }
        }

        return false;
    }

    bool intersectsObstacle(const Vector2& position, float radius) const {
        for (const auto& obstacle : obstacles_) {
            const float minX = obstacle.center.x - obstacle.halfExtents.x;
            const float maxX = obstacle.center.x + obstacle.halfExtents.x;
            const float minY = obstacle.center.y - obstacle.halfExtents.y;
            const float maxY = obstacle.center.y + obstacle.halfExtents.y;
            const float closestX = std::clamp(position.x, minX, maxX);
            const float closestY = std::clamp(position.y, minY, maxY);
            const Vector2 offset(position.x - closestX, position.y - closestY);
            if (offset.lengthSquared() < radius * radius) {
                return true;
            }
        }
        return false;
    }

    Vector2 resolveMovement(const Vector2& position, float radius, const Vector2& delta) const {
        Vector2 result = position;
        const float stepLength = std::max(4.0f, radius * 0.5f);
        const int steps = std::max(1, static_cast<int>(std::ceil(delta.length() / stepLength)));
        const Vector2 step = delta * (1.0f / static_cast<float>(steps));

        const auto clampToBounds = [&](Vector2 candidate) {
            candidate.x = std::clamp(candidate.x, radius, size_.x - radius);
            candidate.y = std::clamp(candidate.y, radius, size_.y - radius);
            return candidate;
        };

        for (int i = 0; i < steps; ++i) {
            Vector2 xCandidate = clampToBounds({result.x + step.x, result.y});
            if (!intersectsObstacle(xCandidate, radius)) {
                result.x = xCandidate.x;
            }

            Vector2 yCandidate = clampToBounds({result.x, result.y + step.y});
            if (!intersectsObstacle(yCandidate, radius)) {
                result.y = yCandidate.y;
            }
        }

        return result;
    }

    bool pathIntersectsObstacle(const Vector2& start, const Vector2& end, float radius) const {
        const Vector2 delta = end - start;
        const float stepLength = std::max(2.0f, radius * 0.5f);
        const int steps = std::max(1, static_cast<int>(std::ceil(delta.length() / stepLength)));
        for (int i = 1; i <= steps; ++i) {
            const float progress = static_cast<float>(i) / static_cast<float>(steps);
            if (intersectsObstacle(start + delta * progress, radius)) {
                return true;
            }
        }
        return false;
    }

    void triggerBoss() { bossTriggered_ = true; }
    void markBossDefeated() {
        bossTriggered_ = true;
        bossDefeated_ = true;
    }

    MapArea areaForPlayer(const Vector2& playerPosition) const {
        if (bossDefeated_) {
            return MapArea::BossDefeated;
        }

        if ((playerPosition - bossCenter_).lengthSquared() <= Config::BossArenaRadius * Config::BossArenaRadius) {
            return MapArea::BossArena;
        }

        if ((playerPosition - bossCenter_).lengthSquared() <= Config::BossGateRadius * Config::BossGateRadius) {
            return MapArea::BossGate;
        }

        if ((playerPosition - playerStart_).lengthSquared() <= Config::StartSafeRadius * Config::StartSafeRadius) {
            return MapArea::Start;
        }

        return MapArea::Field;
    }

    float distanceToBoss(const Vector2& playerPosition) const {
        return (bossCenter_ - playerPosition).length();
    }

    float progressToBoss(const Vector2& playerPosition) const {
        const float totalDistance = (bossCenter_ - playerStart_).length();
        if (totalDistance <= 0.0f) {
            return 1.0f;
        }

        return std::clamp(1.0f - distanceToBoss(playerPosition) / totalDistance, 0.0f, 1.0f);
    }

private:
    void generateObstacles() {
        obstacles_ = layoutDefinition_->obstacles;
    }

    void generateEvents() {
        const auto& eventPositions = layoutDefinition_->eventPositions;
        events_.clear();
        if (eventPositions.size() < 3) {
            return;
        }
        events_.push_back({
            MapEventType::LootCache,
            eventPositions[0],
            78.0f,
            false,
            false
        });
        events_.push_back({
            MapEventType::ElitePack,
            eventPositions[1],
            95.0f,
            false,
            false
        });
        events_.push_back({
            MapEventType::Shrine,
            eventPositions[2],
            82.0f,
            false,
            false
        });
        const auto& encounter = *encounterDefinition_;
        events_.push_back({
            MapEventType::Combination,
            layoutDefinition_->encounterPosition,
            encounter.radius,
            false,
            false,
            encounter.type,
            encounter.id
        });
    }

    Vector2 size_;
    Vector2 playerStart_;
    Vector2 bossCenter_;
    MapExploration exploration_;
    int mapLevel_;
    int templateIndex_;
    int layoutIndex_;
    const MapTemplateDefinition* templateDefinition_;
    const MapLayoutDefinition* layoutDefinition_;
    const MapEncounterDefinition* encounterDefinition_;
    std::vector<MapObstacle> obstacles_;
    std::vector<MapEventInstance> events_;
    bool bossTriggered_;
    bool bossDefeated_;
};

inline const char* mapAreaName(MapArea area) {
    switch (area) {
        case MapArea::Start: return "Start";
        case MapArea::Field: return "Field";
        case MapArea::BossGate: return "Boss Gate";
        case MapArea::BossArena: return "Boss Arena";
        case MapArea::BossDefeated: return "Boss Defeated";
    }
    return "Unknown";
}
