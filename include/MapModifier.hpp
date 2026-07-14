#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "DamageType.hpp"
#include "LootBias.hpp"

struct MapModifierEffect {
    float monsterHpMultiplier = 1.0f;
    int monsterDamageBonus = 0;
    float monsterSpeedMultiplier = 1.0f;
    float itemQuantityMultiplier = 1.0f;
    int bossDropBonus = 0;
    int eliteWeightBonus = 0;
    int chargerWeightBonus = 0;
    float bossHpMultiplier = 1.0f;
    float bossDamageMultiplier = 1.0f;
    int itemLevelBonus = 0;
    float eventRewardMultiplier = 1.0f;
    int ailmentResistanceBonus = 0;
    AffixTag primaryLootBiasTag = AffixTag::None;
    float primaryLootBiasWeightMultiplier = 1.0f;
    AffixTag secondaryLootBiasTag = AffixTag::None;
    float secondaryLootBiasWeightMultiplier = 1.0f;
};

struct MapModifierDefinition {
    std::string id;
    std::string name;
    std::string riskDescription;
    std::string rewardDescription;
    MapModifierEffect effect;
};

struct MapElementalChallengeDefinition {
    std::string id;
    std::string name;
    std::string riskDescription;
    std::string rewardDescription;
    DamageType damageType = DamageType::Physical;
    int playerResistancePenalty = 0;
    int monsterResistanceBonus = 0;
};

struct MapModifier {
    std::string name = "Quiet Coast";
    std::string description = "No modifier";
    std::string rewardDescription = "Baseline map rewards";
    float monsterHpMultiplier = 1.0f;
    int monsterDamageBonus = 0;
    float monsterSpeedMultiplier = 1.0f;
    float itemQuantityMultiplier = 1.0f;
    int bossDropBonus = 0;
    int eliteWeightBonus = 0;
    int chargerWeightBonus = 0;
    float bossHpMultiplier = 1.0f;
    float bossDamageMultiplier = 1.0f;
    int itemLevelBonus = 0;
    float eventRewardMultiplier = 1.0f;
    int ailmentResistanceBonus = 0;
    AffixTag lootBiasTag = AffixTag::None;
    float lootBiasWeightMultiplier = 1.0f;
    AffixTag secondaryLootBiasTag = AffixTag::None;
    float secondaryLootBiasWeightMultiplier = 1.0f;
    std::array<MapModifierDefinition, 2> components{};
    int componentCount = 0;
    std::string elementalChallengeId;
    DamageType elementalChallengeType = DamageType::Physical;
    int playerElementalResistancePenalty = 0;
    int monsterElementalResistanceBonus = 0;

    LootBias lootBias() const {
        return {
            lootBiasTag,
            lootBiasWeightMultiplier,
            secondaryLootBiasTag,
            secondaryLootBiasWeightMultiplier
        };
    }

    bool hasModifier(const std::string& id) const {
        if (elementalChallengeId == id) {
            return true;
        }
        for (int index = 0; index < componentCount; ++index) {
            if (components[static_cast<std::size_t>(index)].id == id) {
                return true;
            }
        }
        return false;
    }
};

struct MapOption {
    MapModifier modifier;
    std::string rewardDescription = "Baseline map rewards";
    std::string recommendedLevel = "Recommended level 1";
    int templateIndex = 0;
};

class MapModifierLibrary {
public:
    static const std::array<MapModifierDefinition, 6>& all() {
        static const std::array<MapModifierDefinition, 6> definitions = {{
            {
                "swift-hunt",
                "Swift Hunt",
                "Monsters move faster",
                "More event cache quantity",
                {
                    1.0f, 0, 1.25f, 1.05f, 0, 0, 4, 1.0f, 1.0f, 0,
                    1.25f, 0, AffixTag::MoveSpeed, 1.30f,
                    AffixTag::None, 1.0f
                }
            },
            {
                "hardened-front",
                "Hardened Front",
                "Monsters have more life",
                "Survival affixes are favored",
                {
                    1.15f, 0, 1.0f, 1.05f, 0, 4, 0, 1.05f, 1.0f, 0,
                    1.0f, 10, AffixTag::Survival, 1.35f,
                    AffixTag::Armor, 1.15f
                }
            },
            {
                "frenzied-march",
                "Frenzied March",
                "Monsters deal more damage",
                "Damage affixes and item quantity increase",
                {
                    1.05f, 1, 1.15f, 1.12f, 0, 8, 0, 1.10f, 1.15f, 0,
                    1.0f, 0, AffixTag::Damage, 1.35f,
                    AffixTag::AttackSpeed, 1.15f
                }
            },
            {
                "blood-tax",
                "Blood Tax",
                "Boss attacks hit harder",
                "Boss drops are more reliable",
                {
                    1.0f, 1, 1.0f, 1.10f, 1, 0, 0, 1.0f, 1.10f, 0,
                    1.0f, 0, AffixTag::Damage, 1.20f,
                    AffixTag::Survival, 1.15f
                }
            },
            {
                "gilded-cache",
                "Gilded Cache",
                "Elite encounters are more common",
                "Item quantity, event rewards and item level increase",
                {
                    1.10f, 0, 1.10f, 1.35f, 0, 6, 4, 1.20f, 1.05f, 1,
                    1.50f, 0, AffixTag::Pickup, 1.35f,
                    AffixTag::Area, 1.15f
                }
            },
            {
                "elite-tide",
                "Elite Tide",
                "Elite and charger packs are more common",
                "Elite drops and area affixes are favored",
                {
                    1.0f, 0, 1.10f, 1.10f, 1, 6, 8, 1.0f, 1.0f, 0,
                    1.0f, 0, AffixTag::Area, 1.35f,
                    AffixTag::Damage, 1.15f
                }
            }
        }};
        return definitions;
    }

    static MapModifier empty() {
        return {};
    }

    static const std::array<MapElementalChallengeDefinition, 4>& elementalChallenges() {
        static const std::array<MapElementalChallengeDefinition, 4> challenges = {{
            {
                "cinder-ward",
                "Cinder Ward",
                "Players have -25% Fire Resistance; monsters gain +15% Fire Resistance",
                "Fire affixes are favored",
                DamageType::Fire,
                25,
                15
            },
            {
                "frostbite",
                "Frostbite",
                "Players have -25% Cold Resistance; monsters gain +15% Cold Resistance",
                "Cold affixes are favored",
                DamageType::Cold,
                25,
                15
            },
            {
                "stormbound",
                "Stormbound",
                "Players have -25% Lightning Resistance; monsters gain +15% Lightning Resistance",
                "Lightning affixes are favored",
                DamageType::Lightning,
                25,
                15
            },
            {
                "venomtide",
                "Venomtide",
                "Players have -25% Poison Resistance; monsters gain +15% Poison Resistance",
                "Poison affixes are favored",
                DamageType::Poison,
                25,
                15
            }
        }};
        return challenges;
    }

    static MapModifier compose(
        int mapLevel,
        const std::array<const char*, 2>& modifierIds,
        const char* elementalChallengeId = nullptr
    ) {
        MapModifier result;
        const int normalizedLevel = std::max(1, mapLevel);
        for (const char* modifierId : modifierIds) {
            if (modifierId == nullptr || modifierId[0] == '\0') {
                continue;
            }

            const auto* definition = find(modifierId);
            if (definition == nullptr || result.componentCount >= 2) {
                continue;
            }

            result.components[static_cast<std::size_t>(result.componentCount)] = *definition;
            ++result.componentCount;
            addDefinition(result, *definition);
        }

        if (result.componentCount == 0) {
            return result;
        }

        applyLevelScaling(result, normalizedLevel);
        addElementalChallenge(result, elementalChallengeId, normalizedLevel);
        return result;
    }

    static const MapModifierDefinition* find(const std::string& id) {
        for (const auto& definition : all()) {
            if (definition.id == id) {
                return &definition;
            }
        }
        return nullptr;
    }

    static const MapElementalChallengeDefinition* findElementalChallenge(
        const std::string& id
    ) {
        for (const auto& definition : elementalChallenges()) {
            if (definition.id == id) {
                return &definition;
            }
        }
        return nullptr;
    }

private:
    static void addLootBias(MapModifier& target, AffixTag tag, float multiplier) {
        if (tag == AffixTag::None || multiplier <= 0.0f) {
            return;
        }

        if (target.lootBiasTag == tag) {
            target.lootBiasWeightMultiplier *= multiplier;
        } else if (target.secondaryLootBiasTag == tag) {
            target.secondaryLootBiasWeightMultiplier *= multiplier;
        } else if (target.lootBiasTag == AffixTag::None) {
            target.lootBiasTag = tag;
            target.lootBiasWeightMultiplier = multiplier;
        } else if (target.secondaryLootBiasTag == AffixTag::None) {
            target.secondaryLootBiasTag = tag;
            target.secondaryLootBiasWeightMultiplier = multiplier;
        }
    }

    static void addDefinition(MapModifier& target, const MapModifierDefinition& definition) {
        const auto& effect = definition.effect;
        target.monsterHpMultiplier *= effect.monsterHpMultiplier;
        target.monsterDamageBonus += effect.monsterDamageBonus;
        target.monsterSpeedMultiplier *= effect.monsterSpeedMultiplier;
        target.itemQuantityMultiplier *= effect.itemQuantityMultiplier;
        target.bossDropBonus += effect.bossDropBonus;
        target.eliteWeightBonus += effect.eliteWeightBonus;
        target.chargerWeightBonus += effect.chargerWeightBonus;
        target.bossHpMultiplier *= effect.bossHpMultiplier;
        target.bossDamageMultiplier *= effect.bossDamageMultiplier;
        target.itemLevelBonus += effect.itemLevelBonus;
        target.eventRewardMultiplier *= effect.eventRewardMultiplier;
        target.ailmentResistanceBonus += effect.ailmentResistanceBonus;
        addLootBias(target, effect.primaryLootBiasTag, effect.primaryLootBiasWeightMultiplier);
        addLootBias(target, effect.secondaryLootBiasTag, effect.secondaryLootBiasWeightMultiplier);

        if (target.name == "Quiet Coast") {
            target.name = definition.name;
        } else {
            target.name += " + " + definition.name;
        }
        if (target.description == "No modifier") {
            target.description = definition.riskDescription;
        } else {
            target.description += " + " + definition.riskDescription;
        }
        if (target.rewardDescription == "Baseline map rewards") {
            target.rewardDescription = definition.rewardDescription;
        } else {
            target.rewardDescription += " + " + definition.rewardDescription;
        }
    }

    static void addElementalChallenge(
        MapModifier& target,
        const char* challengeId,
        int mapLevel
    ) {
        if (challengeId == nullptr || challengeId[0] == '\0') {
            return;
        }

        const auto* definition = findElementalChallenge(challengeId);
        if (definition == nullptr) {
            return;
        }

        const int levels = std::max(0, mapLevel - 1);
        target.elementalChallengeId = definition->id;
        target.elementalChallengeType = definition->damageType;
        target.playerElementalResistancePenalty = definition->playerResistancePenalty
            + levels * 2;
        target.monsterElementalResistanceBonus = definition->monsterResistanceBonus
            + levels * 2;
        target.name += " + " + definition->name;
        target.description += " + " + definition->riskDescription;
        target.rewardDescription += " + " + definition->rewardDescription;
    }

    static void applyLevelScaling(MapModifier& modifier, int mapLevel) {
        const int levels = mapLevel - 1;
        if (levels <= 0) {
            return;
        }

        const float level = static_cast<float>(levels);
        modifier.monsterHpMultiplier *= 1.0f + level * 0.03f;
        modifier.itemQuantityMultiplier *= 1.0f + level * 0.02f;
        modifier.bossHpMultiplier *= 1.0f + level * 0.015f;
        modifier.bossDamageMultiplier *= 1.0f + level * 0.02f;
        modifier.monsterDamageBonus += levels / 4;
    }
};

class MapOptionLibrary {
public:
    static MapOption defaultOption() {
        return {
            MapModifierLibrary::empty(),
            "Baseline monster density and loot",
            "Recommended level 1",
            0
        };
    }

    static std::array<MapOption, 3> generateOptions(int mapLevel) {
        return {{
            {
                MapModifierLibrary::compose(
                    mapLevel, {"swift-hunt", "hardened-front"}, "cinder-ward"
                ),
                "Event quantity and Survival affix bias",
                "Recommended level " + std::to_string(mapLevel),
                0
            },
            {
                MapModifierLibrary::compose(
                    mapLevel, {"frenzied-march", "blood-tax"}, "stormbound"
                ),
                "Damage bias, richer drops and Boss pressure",
                "Recommended level " + std::to_string(mapLevel + 1),
                1
            },
            {
                MapModifierLibrary::compose(
                    mapLevel, {"gilded-cache", "elite-tide"},
                    mapLevel >= 3 ? "venomtide" : "frostbite"
                ),
                "Pickup/Area bias, high item quantity and Elite pressure",
                "Recommended level " + std::to_string(mapLevel + 1),
                2
            },
        }};
    }
};
