#pragma once

#include <string>

#include "ItemBase.hpp"

enum class BossRelicEffectType {
    None,
    MoltenCore,
    StormChain,
    BroodBloom,
    Frostbite,
    ArchiveCurrent,
    ObsidianFurnace,
    AetherReserve,
    SableRot,
    BloodPrice,
    IronheartVerdict,
    StormglassCircuit,
    Permafrost
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
    float chillSpeedMultiplier = 1.0f;
    float chillDurationMultiplier = 1.0f;
    float bleedDamageMultiplier = 1.0f;
    float bleedDurationMultiplier = 1.0f;
    float bleedBurstRadius = 0.0f;
    float bleedBurstDamageMultiplier = 0.0f;
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
        static const BossRelicEffectDefinition frostbite{
            ItemBaseTheme::Frost,
            BossRelicEffectType::Frostbite,
            "Frostbite",
            "Cold skills chill 20% harder and last 25% longer",
            1.0f,
            1.0f,
            0,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.80f,
            1.25f
        };
        static const BossRelicEffectDefinition archiveCurrent{
            ItemBaseTheme::Archive,
            BossRelicEffectType::ArchiveCurrent,
            "Archive Current",
            "Cold skills apply a stronger Chill that lasts 35% longer",
            1.0f,
            1.0f,
            0,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.72f,
            1.35f
        };
        static const BossRelicEffectDefinition obsidianFurnace{
            ItemBaseTheme::Obsidian,
            BossRelicEffectType::ObsidianFurnace,
            "Obsidian Furnace",
            "Fire skills deal 30% more Ignite damage and last 20% longer",
            1.30f,
            1.20f
        };
        static const BossRelicEffectDefinition aetherReserve{
            ItemBaseTheme::Aether,
            BossRelicEffectType::AetherReserve,
            "Aether Reserve",
            "The relic grants maximum Mana, Mana recovery and reduced skill costs"
        };
        static const BossRelicEffectDefinition sableRot{
            ItemBaseTheme::Sable,
            BossRelicEffectType::SableRot,
            "Sable Rot",
            "Poison skills gain damage and area reach from the gravebloom core"
        };
        static const BossRelicEffectDefinition bloodPrice{
            ItemBaseTheme::Bloodletting,
            BossRelicEffectType::BloodPrice,
            "Blood Price",
            "Physical skills deal 25% more Bleed damage; Bleeding enemies burst on death",
            1.0f,
            1.0f,
            0,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            1.25f,
            1.20f,
            110.0f,
            0.45f
        };

        switch (theme) {
            case ItemBaseTheme::Brimstone: return moltenCore;
            case ItemBaseTheme::Storm: return stormChain;
            case ItemBaseTheme::Brood: return broodBloom;
            case ItemBaseTheme::Frost: return frostbite;
            case ItemBaseTheme::Archive: return archiveCurrent;
            case ItemBaseTheme::Obsidian: return obsidianFurnace;
            case ItemBaseTheme::Aether: return aetherReserve;
            case ItemBaseTheme::Sable: return sableRot;
            case ItemBaseTheme::Bloodletting: return bloodPrice;
            case ItemBaseTheme::None: break;
        }
        return none;
    }

    static const BossRelicEffectDefinition& forBase(const ItemBaseDefinition& base) {
        if (base.kind != ItemBaseKind::BossRelic) {
            return forTheme(ItemBaseTheme::None);
        }

        if (base.variant == 1) {
            static const BossRelicEffectDefinition ashenBloom{
                ItemBaseTheme::Brimstone,
                BossRelicEffectType::MoltenCore,
                "Ashen Bloom",
                "Fire skills deal 45% more Ignite damage and last 15% longer",
                1.45f,
                1.15f
            };
            static const BossRelicEffectDefinition tempestChain{
                ItemBaseTheme::Storm,
                BossRelicEffectType::StormChain,
                "Tempest Chain",
                "Lightning hits arc to 3 nearby enemies for 80% damage",
                1.0f,
                1.0f,
                3,
                190.0f,
                0.80f
            };
            static const BossRelicEffectDefinition broodscaleBloom{
                ItemBaseTheme::Brood,
                BossRelicEffectType::BroodBloom,
                "Broodscale Bloom",
                "Poisoned enemies spread 70% Poison across a wider radius on death",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                190.0f,
                0.70f
            };
            static const BossRelicEffectDefinition winterGrasp{
                ItemBaseTheme::Frost,
                BossRelicEffectType::Frostbite,
                "Winter's Grasp",
                "Cold skills chill 30% harder and last 50% longer",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.70f,
                1.50f
            };
            static const BossRelicEffectDefinition drownedCompass{
                ItemBaseTheme::Archive,
                BossRelicEffectType::ArchiveCurrent,
                "Drowned Compass",
                "Cold skills apply a much stronger Chill that lasts 50% longer",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.62f,
                1.50f
            };
            static const BossRelicEffectDefinition blackglassHeart{
                ItemBaseTheme::Obsidian,
                BossRelicEffectType::ObsidianFurnace,
                "Blackglass Heart",
                "Fire skills deal 45% more Ignite damage and last 35% longer",
                1.45f,
                1.35f
            };
            static const BossRelicEffectDefinition nullCrown{
                ItemBaseTheme::Aether,
                BossRelicEffectType::AetherReserve,
                "Null Crown",
                "The crown sharply reduces skill costs and accelerates Mana recovery"
            };
            static const BossRelicEffectDefinition gravebloomHeart{
                ItemBaseTheme::Sable,
                BossRelicEffectType::SableRot,
                "Gravebloom Heart",
                "Poison skills gain stronger damage and wider area reach"
            };
            static const BossRelicEffectDefinition hemorrhageSignet{
                ItemBaseTheme::Bloodletting,
                BossRelicEffectType::BloodPrice,
                "Hemorrhage Signet",
                "Physical skills deal 40% more Bleed damage; Bleeding enemies burst wider on death",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.40f,
                1.30f,
                140.0f,
                0.65f
            };

            switch (base.theme) {
                case ItemBaseTheme::Brimstone: return ashenBloom;
                case ItemBaseTheme::Storm: return tempestChain;
                case ItemBaseTheme::Brood: return broodscaleBloom;
                case ItemBaseTheme::Frost: return winterGrasp;
                case ItemBaseTheme::Archive: return drownedCompass;
                case ItemBaseTheme::Obsidian: return blackglassHeart;
                case ItemBaseTheme::Aether: return nullCrown;
                case ItemBaseTheme::Sable: return gravebloomHeart;
                case ItemBaseTheme::Bloodletting: return hemorrhageSignet;
                case ItemBaseTheme::None: break;
            }
        }

        if (base.variant == 2
            && base.theme == ItemBaseTheme::Bloodletting) {
            static const BossRelicEffectDefinition ironheartVerdict{
                ItemBaseTheme::Bloodletting,
                BossRelicEffectType::IronheartVerdict,
                "Ironheart Verdict",
                "Physical skills deal 35% more Bleed damage; Bleeding enemies burst harder "
                "in a tighter radius",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.35f,
                1.15f,
                90.0f,
                0.75f
            };
            return ironheartVerdict;
        }

        if (base.variant == 2
            && base.theme == ItemBaseTheme::Storm) {
            static const BossRelicEffectDefinition stormglassCircuit{
                ItemBaseTheme::Storm,
                BossRelicEffectType::StormglassCircuit,
                "Stormglass Circuit",
                "Lightning hits arc to 4 nearby enemies for 55% damage",
                1.0f,
                1.0f,
                4,
                220.0f,
                0.55f
            };
            return stormglassCircuit;
        }

        if (base.variant == 2
            && base.theme == ItemBaseTheme::Frost) {
            static const BossRelicEffectDefinition permafrost{
                ItemBaseTheme::Frost,
                BossRelicEffectType::Permafrost,
                "Permafrost",
                "Cold skills Chill 45% harder and last 85% longer",
                1.0f,
                1.0f,
                0,
                0.0f,
                1.0f,
                0.0f,
                0.0f,
                0.55f,
                1.85f
            };
            return permafrost;
        }

        return forTheme(base.theme);
    }
};
