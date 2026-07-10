#pragma once

#include <algorithm>
#include <cmath>
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

struct MapObstacle {
    Vector2 center;
    Vector2 halfExtents;
};

class MapInstance {
public:
    MapInstance()
        : size_(Config::MapWidth, Config::MapHeight)
        , playerStart_(220.0f, Config::MapHeight - 220.0f)
        , bossCenter_(Config::MapWidth - 320.0f, 300.0f)
        , bossTriggered_(false)
        , bossDefeated_(false) {
        generateObstacles();
        generateEvents();
    }

    const Vector2& size() const { return size_; }
    const Vector2& playerStart() const { return playerStart_; }
    const Vector2& bossCenter() const { return bossCenter_; }
    bool bossTriggered() const { return bossTriggered_; }
    bool bossDefeated() const { return bossDefeated_; }
    const std::vector<MapEventInstance>& events() const { return events_; }
    std::vector<MapEventInstance>& eventsForMutation() { return events_; }
    const std::vector<MapObstacle>& obstacles() const { return obstacles_; }

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
        // The route stays open, while each obstacle requires a small detour.
        // Start, event locations, Boss Gate and Boss Arena remain unobstructed.
        obstacles_ = {
            {{720.0f, 1330.0f}, {135.0f, 70.0f}},
            {{1040.0f, 1120.0f}, {95.0f, 170.0f}},
            {{1250.0f, 730.0f}, {180.0f, 80.0f}},
            {{1650.0f, 800.0f}, {100.0f, 145.0f}},
        };
    }

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
