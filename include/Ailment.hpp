#pragma once

#include <array>
#include <cstddef>

enum class AilmentType {
    None,
    Ignite,
    Chill,
    Shock,
    Poison,
    Count
};

struct AilmentDefinition {
    AilmentType type = AilmentType::None;
    float duration = 0.0f;
    float damageMultiplier = 0.0f;
    float speedMultiplier = 1.0f;
    int ignitePenetration = 0;
    int chillPenetration = 0;
    float damageTakenMultiplier = 1.0f;
    int shockPenetration = 0;
    int poisonPenetration = 0;
};

struct AilmentTickResult {
    AilmentType type = AilmentType::None;
    int damage = 0;
    int tickCount = 0;
    bool killed = false;
    std::array<int, static_cast<std::size_t>(AilmentType::Count)> damageByType{};

    void record(AilmentType ailment, int dealtDamage) {
        if (ailment == AilmentType::None
            || ailment == AilmentType::Count
            || dealtDamage <= 0) {
            return;
        }

        type = ailment;
        damage += dealtDamage;
        ++tickCount;
        damageByType[static_cast<std::size_t>(ailment)] += dealtDamage;
    }

    int damageFor(AilmentType ailment) const {
        if (ailment == AilmentType::None || ailment == AilmentType::Count) {
            return 0;
        }
        return damageByType[static_cast<std::size_t>(ailment)];
    }
};
