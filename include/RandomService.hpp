#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

class RandomService {
public:
    static constexpr std::uint64_t DefaultSeed = 0x4D494E4941525047ULL;

    explicit RandomService(std::uint64_t seed = DefaultSeed)
        : seed_(seed)
        , engine_(seed) {
    }

    std::uint64_t seed() const { return seed_; }

    std::string engineState() const {
        std::ostringstream stream;
        stream << engine_;
        return stream.str();
    }

    bool restoreEngineState(const std::string& serializedState) {
        std::istringstream stream(serializedState);
        std::mt19937_64 restored;
        if (!(stream >> restored)) {
            return false;
        }

        engine_ = restored;
        return true;
    }

    void reseed(std::uint64_t seed) {
        seed_ = seed;
        engine_.seed(seed);
    }

    int nextInt(int minimum, int maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }
        std::uniform_int_distribution<int> distribution(minimum, maximum);
        return distribution(engine_);
    }

    std::uint64_t nextUInt64(std::uint64_t minimum, std::uint64_t maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }
        std::uniform_int_distribution<std::uint64_t> distribution(minimum, maximum);
        return distribution(engine_);
    }

    float nextFloat01() {
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(engine_);
    }

    bool chance(int percentage) {
        if (percentage <= 0) {
            return false;
        }
        if (percentage >= 100) {
            return true;
        }
        return nextInt(0, 99) < percentage;
    }

    std::size_t nextIndex(std::size_t size) {
        if (size == 0) {
            return 0;
        }
        return static_cast<std::size_t>(nextUInt64(0, static_cast<std::uint64_t>(size - 1)));
    }

    std::size_t weightedChoiceIndex(const std::vector<int>& weights) {
        std::uint64_t total = 0;
        for (const int weight : weights) {
            if (weight <= 0) {
                continue;
            }
            const auto positiveWeight = static_cast<std::uint64_t>(weight);
            if (total > std::numeric_limits<std::uint64_t>::max() - positiveWeight) {
                total = std::numeric_limits<std::uint64_t>::max();
                break;
            }
            total += positiveWeight;
        }

        if (total == 0) {
            return 0;
        }

        const std::uint64_t target = nextUInt64(0, total - 1);
        std::uint64_t remaining = target;
        for (std::size_t index = 0; index < weights.size(); ++index) {
            const int weight = std::max(0, weights[index]);
            const auto positiveWeight = static_cast<std::uint64_t>(weight);
            if (remaining < positiveWeight) {
                return index;
            }
            remaining -= positiveWeight;
        }
        return weights.empty() ? 0 : weights.size() - 1;
    }

    static std::uint64_t deriveSeed(std::uint64_t seed, std::uint64_t stream) {
        std::uint64_t value = seed + 0x9E3779B97F4A7C15ULL * (stream + 1);
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    // Compatibility path for old call sites. New gameplay code must inject its
    // run-owned service so a complete run remains reproducible.
    static RandomService& legacy() {
        static RandomService service(DefaultSeed);
        return service;
    }

private:
    std::uint64_t seed_;
    std::mt19937_64 engine_;
};
