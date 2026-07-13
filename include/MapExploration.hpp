#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "Vector2.hpp"

class MapExploration {
public:
    static constexpr float DefaultCellSize = 60.0f;
    static constexpr float DefaultRevealRadius = 300.0f;

    MapExploration(
        const Vector2& mapSize,
        float cellSize = DefaultCellSize,
        float revealRadius = DefaultRevealRadius
    )
        : mapSize_(mapSize)
        , cellSize_(std::max(1.0f, cellSize))
        , revealRadius_(std::max(0.0f, revealRadius))
        , columns_(std::max(1, static_cast<int>(std::ceil(mapSize_.x / cellSize_))))
        , rows_(std::max(1, static_cast<int>(std::ceil(mapSize_.y / cellSize_))))
        , revealed_(static_cast<std::size_t>(columns_ * rows_), false) {}

    void reset() {
        std::fill(revealed_.begin(), revealed_.end(), false);
    }

    void revealAround(const Vector2& position) {
        if (position.x < 0.0f || position.y < 0.0f
            || position.x >= mapSize_.x || position.y >= mapSize_.y) {
            return;
        }

        const auto centerCell = cellForPosition(position);
        const int radiusInCells = static_cast<int>(std::ceil(revealRadius_ / cellSize_)) + 1;
        const float revealRadiusSquared = revealRadius_ * revealRadius_;

        for (int y = centerCell.second - radiusInCells;
            y <= centerCell.second + radiusInCells;
            ++y) {
            for (int x = centerCell.first - radiusInCells;
                x <= centerCell.first + radiusInCells;
                ++x) {
                if (!isValidCell(x, y)) {
                    continue;
                }

                const Vector2 center = cellCenter(x, y);
                if ((center - position).lengthSquared() <= revealRadiusSquared) {
                    revealed_[indexForCell(x, y)] = true;
                }
            }
        }
    }

    bool isExplored(const Vector2& position) const {
        if (position.x < 0.0f || position.y < 0.0f
            || position.x >= mapSize_.x || position.y >= mapSize_.y) {
            return false;
        }

        const auto cell = cellForPosition(position);
        return isCellRevealed(cell.first, cell.second);
    }

    bool isCellRevealed(int column, int row) const {
        return isValidCell(column, row)
            && revealed_[indexForCell(column, row)];
    }

    bool isFullyExplored() const {
        return std::all_of(
            revealed_.begin(), revealed_.end(),
            [](bool revealed) { return revealed; }
        );
    }

    int exploredCellCount() const {
        return static_cast<int>(std::count(revealed_.begin(), revealed_.end(), true));
    }

    int totalCellCount() const { return columns_ * rows_; }
    int columns() const { return columns_; }
    int rows() const { return rows_; }
    float cellSize() const { return cellSize_; }
    float revealRadius() const { return revealRadius_; }

    std::vector<unsigned char> revealedCells() const {
        std::vector<unsigned char> result;
        result.reserve(revealed_.size());
        for (const bool revealed : revealed_) {
            result.push_back(revealed ? 1 : 0);
        }
        return result;
    }

    bool restoreRevealedCells(const std::vector<unsigned char>& cells) {
        if (cells.size() != revealed_.size()) {
            return false;
        }

        for (std::size_t index = 0; index < cells.size(); ++index) {
            if (cells[index] > 1) {
                return false;
            }
            revealed_[index] = cells[index] != 0;
        }
        return true;
    }

    Vector2 cellCenter(int column, int row) const {
        return {
            (static_cast<float>(column) + 0.5f) * cellSize_,
            (static_cast<float>(row) + 0.5f) * cellSize_
        };
    }

private:
    std::size_t indexForCell(int column, int row) const {
        return static_cast<std::size_t>(row * columns_ + column);
    }

    bool isValidCell(int column, int row) const {
        return column >= 0 && column < columns_ && row >= 0 && row < rows_;
    }

    std::pair<int, int> cellForPosition(const Vector2& position) const {
        return {
            std::clamp(static_cast<int>(std::floor(position.x / cellSize_)), 0, columns_ - 1),
            std::clamp(static_cast<int>(std::floor(position.y / cellSize_)), 0, rows_ - 1)
        };
    }

    Vector2 mapSize_;
    float cellSize_;
    float revealRadius_;
    int columns_;
    int rows_;
    std::vector<bool> revealed_;
};
