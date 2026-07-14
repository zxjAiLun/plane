#pragma once

#include <string>

#include "ItemBase.hpp"

enum class BossRelicEffectType {
    None,
    MoltenCore,
    StormChain,
    BroodBloom
};

struct BossRelicEffectDefinition {
    ItemBaseTheme theme = ItemBaseTheme::None;
    BossRelicEffectType type = BossRelicEffectType::None;
    std::string name;
    std::string description;
    float igniteDamageMultiplier = 1.0f;
    float igniteDurationMultiplier = 1.0f;
    int lightningChainCount = 0;
    float lightningChainRadius = 0.0f;
    float lightningChainDamageMultiplier = 1.0f;
    float poisonSpreadRadius = 0.0f;
    float poisonSpreadMultiplier = 0.0f;
};

class BossRelicEffectLibrary {
public:
    static const BossRelicEffectDefinition& forTheme(ItemBaseTheme theme) {
        static const BossRelicEffectDefinition none;
        static const BossRelicEffectDefinition moltenCore{
            ItemBaseTheme::Brimstone,
            BossRelicEffectType::MoltenCore,
            "Molten Core",
            "Fire skills deal 25% more Ignite damage and last 25% longer",
            1.25f,
            1.25f
        };
        static const BossRelicEffectDefinition stormChain{
            ItemBaseTheme::Storm,
            BossRelicEffectType::StormChain,
            "Storm Chain",
            "Lightning hits arc to 2 nearby enemies for 65% damage",
            1.0f,
            1.0f,
            2,
            150.0f,
            0.65f
        };
        static const BossRelicEffectDefinition broodBloom{
            ItemBaseTheme::Brood,
            BossRelicEffectType::BroodBloom,
            "Brood Bloom",
            "Poisoned enemies spread 50% Poison to nearby enemies on death",
            1.0f,
            1.0f,
            0,
            0.0f,
            1.0f,
            150.0f,
            0.50f
        };

        switch (theme) {
            case ItemBaseTheme::Brimstone: return moltenCore;
            case ItemBaseTheme::Storm: return stormChain;
            case ItemBaseTheme::Brood: return broodBloom;
            case ItemBaseTheme::None: break;
        }
        return none;
    }
};
