#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "Config.hpp"
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
    Shrine
};

struct MapEventInstance {
    MapEventType type = MapEventType::LootCache;
    Vector2 position;
    float radius = 70.0f;
    bool triggered = false;
    bool completed = false;
};

class MapInstance {
public:
    MapInstance()
        : size_(Config::MapWidth, Config::MapHeight)
        , playerStart_(220.0f, Config::MapHeight - 220.0f)
        , bossCenter_(Config::MapWidth - 320.0f, 300.0f)
        , bossTriggered_(false)
        , bossDefeated_(false) {
        generateEvents();
    }

    const Vector2& size() const { return size_; }
    const Vector2& playerStart() const { return playerStart_; }
    const Vector2& bossCenter() const { return bossCenter_; }
    bool bossTriggered() const { return bossTriggered_; }
    bool bossDefeated() const { return bossDefeated_; }
    const std::vector<MapEventInstance>& events() const { return events_; }
    std::vector<MapEventInstance>& eventsForMutation() { return events_; }

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
    void generateEvents() {
        const Vector2 route = bossCenter_ - playerStart_;
        events_.clear();
        events_.push_back({
            MapEventType::LootCache,
            playerStart_ + route * 0.28f + Vector2(-40.0f, -130.0f),
            78.0f,
            false,
            false
        });
        events_.push_back({
            MapEventType::ElitePack,
            playerStart_ + route * 0.52f + Vector2(130.0f, 35.0f),
            95.0f,
            false,
            false
        });
        events_.push_back({
            MapEventType::Shrine,
            playerStart_ + route * 0.74f + Vector2(-115.0f, -75.0f),
            82.0f,
            false,
            false
        });
    }

    Vector2 size_;
    Vector2 playerStart_;
    Vector2 bossCenter_;
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
