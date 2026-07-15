#include "SaveService.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr std::uint32_t MaxStringLength = 1U << 20;
constexpr std::uint32_t MaxVectorLength = 4096U;
constexpr std::uint32_t MaxExplorationCells = 100000U;

class Writer {
public:
    template <typename T>
    void integer(T value) {
        static_assert(std::is_integral_v<T>);
        using Unsigned = std::make_unsigned_t<T>;
        const Unsigned converted = static_cast<Unsigned>(value);
        for (std::size_t index = 0; index < sizeof(T); ++index) {
            bytes_.push_back(static_cast<unsigned char>(converted >> (index * 8)));
        }
    }

    void boolean(bool value) { integer<std::uint8_t>(value ? 1 : 0); }

    void real(float value) {
        std::uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        integer(bits);
    }

    void string(const std::string& value) {
        integer<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    void raw(const std::vector<unsigned char>& bytes) {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    }

    const std::vector<unsigned char>& bytes() const { return bytes_; }

private:
    std::vector<unsigned char> bytes_;
};

class Reader {
public:
    explicit Reader(const std::vector<unsigned char>& bytes)
        : bytes_(bytes) {
    }

    template <typename T>
    bool integer(T& value) {
        static_assert(std::is_integral_v<T>);
        if (remaining() < sizeof(T)) {
            return false;
        }

        using Unsigned = std::make_unsigned_t<T>;
        Unsigned converted = 0;
        for (std::size_t index = 0; index < sizeof(T); ++index) {
            converted |= static_cast<Unsigned>(bytes_[position_ + index]) << (index * 8);
        }
        position_ += sizeof(T);
        value = static_cast<T>(converted);
        return true;
    }

    bool boolean(bool& value) {
        std::uint8_t encoded = 0;
        if (!integer(encoded) || encoded > 1) {
            return false;
        }
        value = encoded != 0;
        return true;
    }

    bool real(float& value) {
        std::uint32_t bits = 0;
        if (!integer(bits)) {
            return false;
        }
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool string(std::string& value) {
        std::uint32_t length = 0;
        if (!integer(length) || length > MaxStringLength || remaining() < length) {
            return false;
        }

        value.assign(
            reinterpret_cast<const char*>(bytes_.data() + position_),
            static_cast<std::size_t>(length)
        );
        position_ += length;
        return true;
    }

    bool raw(std::vector<unsigned char>& bytes, std::size_t length) {
        if (remaining() < length) {
            return false;
        }
        bytes.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
            bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + length));
        position_ += length;
        return true;
    }

    bool atEnd() const { return position_ == bytes_.size(); }
    std::size_t remaining() const { return bytes_.size() - position_; }

private:
    const std::vector<unsigned char>& bytes_;
    std::size_t position_ = 0;
};

void setError(std::string* error, const char* message) {
    if (error != nullptr) {
        *error = message;
    }
}

std::uint32_t crc32(const std::vector<unsigned char>& bytes) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const unsigned char byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320U & (-(crc & 1U)));
        }
    }
    return ~crc;
}

bool validEnumValue(int value, int minimum, int maximum) {
    return value >= minimum && value <= maximum;
}

void writeStats(Writer& writer, const Stats& stats) {
    writer.integer(stats.maxHp);
    writer.real(stats.moveSpeedMultiplier);
    writer.real(stats.damageMultiplier);
    writer.real(stats.attackSpeedMultiplier);
    writer.real(stats.pickupRangeMultiplier);
    writer.real(stats.projectileDamageMultiplier);
    writer.real(stats.areaDamageMultiplier);
    writer.real(stats.areaRadiusMultiplier);
    writer.integer(stats.armor);
    writer.integer(stats.projectileCountBonus);
    writer.real(stats.lifeFlaskEffectMultiplier);
    writer.real(stats.itemQuantityMultiplier);
    writer.real(stats.incomingDamageMultiplier);
    writer.real(stats.fireDamageMultiplier);
    writer.real(stats.coldDamageMultiplier);
    writer.real(stats.lightningDamageMultiplier);
    writer.integer(stats.fireResistance);
    writer.integer(stats.coldResistance);
    writer.integer(stats.lightningResistance);
    writer.real(stats.poisonDamageMultiplier);
    writer.integer(stats.poisonResistance);
}

bool readStats(Reader& reader, Stats& stats, bool hasPoisonFields) {
    if (!(reader.integer(stats.maxHp)
        && reader.real(stats.moveSpeedMultiplier)
        && reader.real(stats.damageMultiplier)
        && reader.real(stats.attackSpeedMultiplier)
        && reader.real(stats.pickupRangeMultiplier)
        && reader.real(stats.projectileDamageMultiplier)
        && reader.real(stats.areaDamageMultiplier)
        && reader.real(stats.areaRadiusMultiplier)
        && reader.integer(stats.armor)
        && reader.integer(stats.projectileCountBonus)
        && reader.real(stats.lifeFlaskEffectMultiplier)
        && reader.real(stats.itemQuantityMultiplier)
        && reader.real(stats.incomingDamageMultiplier)
        && reader.real(stats.fireDamageMultiplier)
        && reader.real(stats.coldDamageMultiplier)
        && reader.real(stats.lightningDamageMultiplier)
        && reader.integer(stats.fireResistance)
        && reader.integer(stats.coldResistance)
        && reader.integer(stats.lightningResistance))) {
        return false;
    }

    if (!hasPoisonFields) {
        stats.poisonDamageMultiplier = 1.0f;
        stats.poisonResistance = 0;
        return true;
    }

    return reader.real(stats.poisonDamageMultiplier)
        && reader.integer(stats.poisonResistance);
}

void writeVector2(Writer& writer, const Vector2& position) {
    writer.real(position.x);
    writer.real(position.y);
}

bool readVector2(Reader& reader, Vector2& position) {
    return reader.real(position.x) && reader.real(position.y);
}

void writeItem(Writer& writer, const Item& item) {
    writer.string(item.name);
    writer.integer(static_cast<int>(item.slot));
    writer.integer(static_cast<int>(item.rarity));
    writeStats(writer, item.stats);
    writer.integer(item.itemLevel);
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(item.affixes.size()));
    for (const auto& affix : item.affixes) {
        writer.string(affix.name);
        writer.integer(affix.tier);
        writeStats(writer, affix.stats);
        writer.integer<std::uint32_t>(static_cast<std::uint32_t>(affix.tags.size()));
        for (const auto tag : affix.tags) {
            writer.integer(static_cast<int>(tag));
        }
        writer.string(affix.id);
        writer.integer(static_cast<int>(affix.stat));
        writer.boolean(affix.isPrefix);
    }
    writer.string(item.baseId);
    writer.string(item.baseName);
    writeStats(writer, item.implicitStats);
}

bool readItem(Reader& reader, Item& item, bool hasPoisonFields) {
    int slot = 0;
    int rarity = 0;
    std::uint32_t affixCount = 0;
    if (!reader.string(item.name)
        || !reader.integer(slot)
        || !reader.integer(rarity)
        || !validEnumValue(slot, 0, static_cast<int>(EquipmentSlot::Count) - 1)
        || !validEnumValue(rarity, 0, static_cast<int>(Rarity::Unique))
        || !readStats(reader, item.stats, hasPoisonFields)
        || !reader.integer(item.itemLevel)
        || item.itemLevel < 1
        || !reader.integer(affixCount)
        || affixCount > MaxVectorLength) {
        return false;
    }

    item.slot = static_cast<EquipmentSlot>(slot);
    item.rarity = static_cast<Rarity>(rarity);
    item.affixes.clear();
    item.affixes.reserve(affixCount);
    for (std::uint32_t index = 0; index < affixCount; ++index) {
        ItemAffix affix;
        int stat = 0;
        std::uint32_t tagCount = 0;
        if (!reader.string(affix.name)
            || !reader.integer(affix.tier)
            || affix.tier < 1
            || !readStats(reader, affix.stats, hasPoisonFields)
            || !reader.integer(tagCount)
            || tagCount > MaxVectorLength) {
            return false;
        }
        affix.tags.clear();
        affix.tags.reserve(tagCount);
        for (std::uint32_t tagIndex = 0; tagIndex < tagCount; ++tagIndex) {
            int tag = 0;
            if (!reader.integer(tag)
                || !validEnumValue(tag, 0, static_cast<int>(AffixTag::Poison))) {
                return false;
            }
            affix.tags.push_back(static_cast<AffixTag>(tag));
        }
        if (!reader.string(affix.id)
            || !reader.integer(stat)
            || !validEnumValue(stat, 0, static_cast<int>(AffixStat::PoisonResistance))
            || !reader.boolean(affix.isPrefix)) {
            return false;
        }
        affix.stat = static_cast<AffixStat>(stat);
        item.affixes.push_back(std::move(affix));
    }

    return reader.string(item.baseId)
        && reader.string(item.baseName)
        && readStats(reader, item.implicitStats, hasPoisonFields);
}

void writePlayerState(Writer& writer, const PlayerSaveState& state) {
    writeVector2(writer, state.position);
    writer.integer(state.hp);
    writer.real(state.mana);
    writer.integer(state.level);
    writer.integer(state.exp);
    writer.integer(state.expToNextLevel);
    writer.integer(state.talentPoints);
    writeStats(writer, state.upgradeStats);
    for (const bool allocated : state.allocatedPassiveNodes) {
        writer.boolean(allocated);
    }
    for (const auto& item : state.equipment) {
        writer.boolean(item.has_value());
        if (item) {
            writeItem(writer, *item);
        }
    }
}

bool readPlayerState(
    Reader& reader,
    PlayerSaveState& state,
    bool hasPoisonFields,
    std::size_t passiveNodeCount
) {
    if (!readVector2(reader, state.position)
        || !reader.integer(state.hp)
        || !reader.real(state.mana)
        || !reader.integer(state.level)
        || !reader.integer(state.exp)
        || !reader.integer(state.expToNextLevel)
        || !reader.integer(state.talentPoints)
        || !readStats(reader, state.upgradeStats, hasPoisonFields)) {
        return false;
    }
    state.allocatedPassiveNodes.fill(false);
    if (passiveNodeCount > state.allocatedPassiveNodes.size()) {
        return false;
    }
    for (std::size_t index = 0; index < passiveNodeCount; ++index) {
        if (!reader.boolean(state.allocatedPassiveNodes[index])) {
            return false;
        }
    }
    for (auto& item : state.equipment) {
        bool present = false;
        if (!reader.boolean(present)) {
            return false;
        }
        item.reset();
        if (present) {
            item.emplace();
            if (!readItem(reader, *item, hasPoisonFields)) {
                return false;
            }
        }
    }
    return true;
}

void writeSkillBarState(Writer& writer, const SkillBarSaveState& state) {
    for (std::size_t index = 0; index < state.skills.size(); ++index) {
        writer.string(state.skills[index]);
        for (const auto& support : state.supports[index]) {
            writer.string(support);
        }
        writer.real(state.elapsed[index]);
    }
}

bool readSkillBarState(Reader& reader, SkillBarSaveState& state) {
    for (std::size_t index = 0; index < state.skills.size(); ++index) {
        if (!reader.string(state.skills[index])) {
            return false;
        }
        for (auto& support : state.supports[index]) {
            if (!reader.string(support)) {
                return false;
            }
        }
        if (!reader.real(state.elapsed[index])) {
            return false;
        }
    }
    return true;
}

void writeModifierEffect(Writer& writer, const MapModifierEffect& effect) {
    writer.real(effect.monsterHpMultiplier);
    writer.integer(effect.monsterDamageBonus);
    writer.real(effect.monsterSpeedMultiplier);
    writer.real(effect.itemQuantityMultiplier);
    writer.real(effect.itemRarityMultiplier);
    writer.integer(effect.bossDropBonus);
    writer.integer(effect.eliteWeightBonus);
    writer.integer(effect.chargerWeightBonus);
    writer.real(effect.bossHpMultiplier);
    writer.real(effect.bossDamageMultiplier);
    writer.integer(effect.itemLevelBonus);
    writer.real(effect.eventRewardMultiplier);
    writer.integer(effect.ailmentResistanceBonus);
    writer.integer(static_cast<int>(effect.primaryLootBiasTag));
    writer.real(effect.primaryLootBiasWeightMultiplier);
    writer.integer(static_cast<int>(effect.secondaryLootBiasTag));
    writer.real(effect.secondaryLootBiasWeightMultiplier);
}

bool readModifierEffect(
    Reader& reader,
    MapModifierEffect& effect,
    bool hasItemRarityFields
) {
    int primaryTag = 0;
    int secondaryTag = 0;
    if (!reader.real(effect.monsterHpMultiplier)
        || !reader.integer(effect.monsterDamageBonus)
        || !reader.real(effect.monsterSpeedMultiplier)
        || !reader.real(effect.itemQuantityMultiplier)) {
        return false;
    }
    if (hasItemRarityFields) {
        if (!reader.real(effect.itemRarityMultiplier)) {
            return false;
        }
    } else {
        effect.itemRarityMultiplier = 1.0f;
    }
    if (!reader.integer(effect.bossDropBonus)
        || !reader.integer(effect.eliteWeightBonus)
        || !reader.integer(effect.chargerWeightBonus)
        || !reader.real(effect.bossHpMultiplier)
        || !reader.real(effect.bossDamageMultiplier)
        || !reader.integer(effect.itemLevelBonus)
        || !reader.real(effect.eventRewardMultiplier)
        || !reader.integer(effect.ailmentResistanceBonus)
        || !reader.integer(primaryTag)
        || !reader.real(effect.primaryLootBiasWeightMultiplier)
        || !reader.integer(secondaryTag)
        || !reader.real(effect.secondaryLootBiasWeightMultiplier)) {
        return false;
    }
    if (!validEnumValue(primaryTag, 0, static_cast<int>(AffixTag::Poison))
        || !validEnumValue(secondaryTag, 0, static_cast<int>(AffixTag::Poison))) {
        return false;
    }
    effect.primaryLootBiasTag = static_cast<AffixTag>(primaryTag);
    effect.secondaryLootBiasTag = static_cast<AffixTag>(secondaryTag);
    return true;
}

void writeModifierDefinition(Writer& writer, const MapModifierDefinition& definition) {
    writer.string(definition.id);
    writer.string(definition.name);
    writer.string(definition.riskDescription);
    writer.string(definition.rewardDescription);
    writeModifierEffect(writer, definition.effect);
}

bool readModifierDefinition(
    Reader& reader,
    MapModifierDefinition& definition,
    bool hasItemRarityFields
) {
    return reader.string(definition.id)
        && reader.string(definition.name)
        && reader.string(definition.riskDescription)
        && reader.string(definition.rewardDescription)
        && readModifierEffect(reader, definition.effect, hasItemRarityFields);
}

void writeModifier(Writer& writer, const MapModifier& modifier) {
    writer.string(modifier.name);
    writer.string(modifier.description);
    writer.string(modifier.rewardDescription);
    writer.real(modifier.monsterHpMultiplier);
    writer.integer(modifier.monsterDamageBonus);
    writer.real(modifier.monsterSpeedMultiplier);
    writer.real(modifier.itemQuantityMultiplier);
    writer.real(modifier.itemRarityMultiplier);
    writer.integer(modifier.bossDropBonus);
    writer.integer(modifier.eliteWeightBonus);
    writer.integer(modifier.chargerWeightBonus);
    writer.real(modifier.bossHpMultiplier);
    writer.real(modifier.bossDamageMultiplier);
    writer.integer(modifier.itemLevelBonus);
    writer.real(modifier.eventRewardMultiplier);
    writer.integer(modifier.ailmentResistanceBonus);
    writer.integer(static_cast<int>(modifier.lootBiasTag));
    writer.real(modifier.lootBiasWeightMultiplier);
    writer.integer(static_cast<int>(modifier.secondaryLootBiasTag));
    writer.real(modifier.secondaryLootBiasWeightMultiplier);
    writer.string(modifier.elementalChallengeId);
    writer.integer(static_cast<int>(modifier.elementalChallengeType));
    writer.integer(modifier.playerElementalResistancePenalty);
    writer.integer(modifier.monsterElementalResistanceBonus);
    writer.integer(modifier.componentCount);
    for (int index = 0; index < modifier.componentCount; ++index) {
        writeModifierDefinition(writer, modifier.components[static_cast<std::size_t>(index)]);
    }
}

bool readModifier(
    Reader& reader,
    MapModifier& modifier,
    bool hasElementalChallengeFields,
    bool hasItemRarityFields
) {
    int lootTag = 0;
    int secondaryTag = 0;
    int elementalChallengeType = 0;
    if (!reader.string(modifier.name)
        || !reader.string(modifier.description)
        || !reader.string(modifier.rewardDescription)
        || !reader.real(modifier.monsterHpMultiplier)
        || !reader.integer(modifier.monsterDamageBonus)
        || !reader.real(modifier.monsterSpeedMultiplier)
        || !reader.real(modifier.itemQuantityMultiplier)) {
        return false;
    }
    if (hasItemRarityFields) {
        if (!reader.real(modifier.itemRarityMultiplier)) {
            return false;
        }
    } else {
        modifier.itemRarityMultiplier = 1.0f;
    }
    if (!reader.integer(modifier.bossDropBonus)
        || !reader.integer(modifier.eliteWeightBonus)
        || !reader.integer(modifier.chargerWeightBonus)
        || !reader.real(modifier.bossHpMultiplier)
        || !reader.real(modifier.bossDamageMultiplier)
        || !reader.integer(modifier.itemLevelBonus)
        || !reader.real(modifier.eventRewardMultiplier)
        || !reader.integer(modifier.ailmentResistanceBonus)
        || !reader.integer(lootTag)
        || !reader.real(modifier.lootBiasWeightMultiplier)
        || !reader.integer(secondaryTag)
        || !reader.real(modifier.secondaryLootBiasWeightMultiplier)) {
        return false;
    }
    if (!validEnumValue(lootTag, 0, static_cast<int>(AffixTag::Poison))
        || !validEnumValue(secondaryTag, 0, static_cast<int>(AffixTag::Poison))) {
        return false;
    }

    if (hasElementalChallengeFields) {
        if (!reader.string(modifier.elementalChallengeId)
            || !reader.integer(elementalChallengeType)
            || !reader.integer(modifier.playerElementalResistancePenalty)
            || !reader.integer(modifier.monsterElementalResistanceBonus)
            || !validEnumValue(
                elementalChallengeType,
                static_cast<int>(DamageType::Physical),
                static_cast<int>(DamageType::Poison)
            )) {
            return false;
        }
        modifier.elementalChallengeType = static_cast<DamageType>(elementalChallengeType);
    } else {
        modifier.elementalChallengeId.clear();
        modifier.elementalChallengeType = DamageType::Physical;
        modifier.playerElementalResistancePenalty = 0;
        modifier.monsterElementalResistanceBonus = 0;
    }

    if (!reader.integer(modifier.componentCount)
        || modifier.componentCount < 0
        || modifier.componentCount > 2) {
        return false;
    }

    modifier.lootBiasTag = static_cast<AffixTag>(lootTag);
    modifier.secondaryLootBiasTag = static_cast<AffixTag>(secondaryTag);
    modifier.components = {};
    for (int index = 0; index < modifier.componentCount; ++index) {
        if (!readModifierDefinition(
                reader,
                modifier.components[static_cast<std::size_t>(index)],
                hasItemRarityFields
            )) {
            return false;
        }
    }
    return true;
}

void writeMapOption(Writer& writer, const MapOption& option) {
    writeModifier(writer, option.modifier);
    writer.string(option.rewardDescription);
    writer.string(option.recommendedLevel);
    writer.integer(option.templateIndex);
}

bool readMapOption(
    Reader& reader,
    MapOption& option,
    bool hasElementalChallengeFields,
    bool hasItemRarityFields
) {
    return readModifier(
            reader,
            option.modifier,
            hasElementalChallengeFields,
            hasItemRarityFields
        )
        && reader.string(option.rewardDescription)
        && reader.string(option.recommendedLevel)
        && reader.integer(option.templateIndex);
}

void writeMapReward(Writer& writer, const MapRewardDefinition& reward) {
    writer.integer(static_cast<int>(reward.type));
    writer.string(reward.title);
    writer.string(reward.description);
    writer.string(reward.skillName);
    writer.string(reward.supportName);
    writer.real(reward.itemQuantityMultiplierBonus);
    writer.integer(reward.targetLevel);
}

bool readMapReward(Reader& reader, MapRewardDefinition& reward, bool hasTargetLevel) {
    int type = 0;
    if (!reader.integer(type)
        || !validEnumValue(type, 0, static_cast<int>(MapRewardType::UpgradeSupport))
        || !reader.string(reward.title)
        || !reader.string(reward.description)
        || !reader.string(reward.skillName)
        || !reader.string(reward.supportName)
        || !reader.real(reward.itemQuantityMultiplierBonus)) {
        return false;
    }
    reward.type = static_cast<MapRewardType>(type);
    reward.targetLevel = 1;
    return !hasTargetLevel || reader.integer(reward.targetLevel);
}

void writeSet(Writer& writer, const std::set<std::string>& values) {
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(values.size()));
    for (const auto& value : values) {
        writer.string(value);
    }
}

bool readSet(Reader& reader, std::set<std::string>& values) {
    std::uint32_t count = 0;
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }
    values.clear();
    for (std::uint32_t index = 0; index < count; ++index) {
        std::string value;
        if (!reader.string(value)) {
            return false;
        }
        values.insert(std::move(value));
    }
    return true;
}

void writeLevelMap(Writer& writer, const std::map<std::string, int>& values) {
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(values.size()));
    for (const auto& [name, level] : values) {
        writer.string(name);
        writer.integer(level);
    }
}

bool readLevelMap(Reader& reader, std::map<std::string, int>& values) {
    std::uint32_t count = 0;
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }

    values.clear();
    for (std::uint32_t index = 0; index < count; ++index) {
        std::string name;
        int level = 0;
        if (!reader.string(name) || !reader.integer(level)) {
            return false;
        }
        values[name] = level;
    }
    return true;
}

void writeSaveData(Writer& writer, const SaveData& data) {
    writer.integer(static_cast<int>(data.state));
    writer.integer(data.runSeed);
    writer.string(data.randomEngineState);
    writer.integer(data.mapLevel);
    writer.integer(data.mapTemplateIndex);
    writer.integer(data.mapLayoutIndex);
    writeMapOption(writer, data.currentMapOption);
    for (const auto& option : data.nextMapOptions) {
        writeMapOption(writer, option);
    }
    for (const auto& reward : data.mapRewardOptions) {
        writeMapReward(writer, reward);
    }
    writer.integer(data.selectedNextMapOption);
    writer.integer(data.selectedMapRewardOption);
    writer.boolean(data.nextMapOptionChosen);
    writer.boolean(data.mapRewardChosen);
    writer.integer(data.score);
    writer.real(data.survivalTime);
    writer.integer(data.mapKills);
    writer.integer(data.mapExperienceGained);
    writer.integer(data.mapItemsDropped);
    writer.integer(data.mapBossItemsDropped);
    writer.integer(data.mapItemsPickedUp);
    for (const int count : data.mapDroppedItemsByRarity) {
        writer.integer(count);
    }
    writer.integer(data.mapRareLeadersDefeated);
    writer.integer(data.mapRareLeaderItemsDropped);
    writer.string(data.lastRareLeaderName);
    writer.string(data.lastRareLeaderRewardDescription);
    writer.integer(data.fieldPacksCleared);
    writer.integer(data.lifeFlaskCharges);
    writeSet(writer, data.unlockedSkills);
    writeSet(writer, data.unlockedSupports);
    writeLevelMap(writer, data.skillLevels);
    writeLevelMap(writer, data.supportLevels);
    writer.real(data.itemQuantityRewardMultiplier);
    writer.integer(data.forgeFragments);
    writePlayerState(writer, data.player);
    writeSkillBarState(writer, data.skillBar);

    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(data.inventory.size()));
    for (const auto& item : data.inventory) {
        writeItem(writer, item);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(data.stash.size()));
    for (const auto& item : data.stash) {
        writeItem(writer, item);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(data.droppedItems.size()));
    for (const auto& dropped : data.droppedItems) {
        writeVector2(writer, dropped.position);
        writeItem(writer, dropped.item);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(data.mapEvents.size()));
    for (const auto& event : data.mapEvents) {
        writer.integer(static_cast<int>(event.type));
        writer.boolean(event.triggered);
        writer.boolean(event.completed);
    }
    writer.integer<std::uint32_t>(static_cast<std::uint32_t>(data.exploredCells.size()));
    writer.raw(data.exploredCells);
}

bool readSaveData(
    Reader& reader,
    SaveData& data,
    bool hasElementalChallengeFields,
    bool hasItemRarityFields,
    bool hasPoisonFields,
    std::size_t passiveNodeCount,
    bool hasGemProgression,
    bool hasFieldPackProgress,
    bool hasRareLeaderProgress,
    bool hasDropRarityStats
) {
    int state = 0;
    if (!reader.integer(state)
        || !validEnumValue(state, 0, static_cast<int>(SavedRunState::MapComplete))
        || !reader.integer(data.runSeed)
        || !reader.string(data.randomEngineState)
        || !reader.integer(data.mapLevel)
        || data.mapLevel < 1
        || !reader.integer(data.mapTemplateIndex)
        || !reader.integer(data.mapLayoutIndex)
        || !readMapOption(
            reader,
            data.currentMapOption,
            hasElementalChallengeFields,
            hasItemRarityFields
        )) {
        return false;
    }
    data.state = static_cast<SavedRunState>(state);
    for (auto& option : data.nextMapOptions) {
        if (!readMapOption(
                reader,
                option,
                hasElementalChallengeFields,
                hasItemRarityFields
            )) {
            return false;
        }
    }
    for (auto& reward : data.mapRewardOptions) {
        if (!readMapReward(reader, reward, hasGemProgression)) {
            return false;
        }
    }
    data.fieldPacksCleared = 0;
    data.mapDroppedItemsByRarity = {};
    data.mapRareLeadersDefeated = 0;
    data.mapRareLeaderItemsDropped = 0;
    data.lastRareLeaderName.clear();
    data.lastRareLeaderRewardDescription.clear();
    if (!reader.integer(data.selectedNextMapOption)
        || !reader.integer(data.selectedMapRewardOption)
        || !reader.boolean(data.nextMapOptionChosen)
        || !reader.boolean(data.mapRewardChosen)
        || !reader.integer(data.score)
        || !reader.real(data.survivalTime)
        || !reader.integer(data.mapKills)
        || !reader.integer(data.mapExperienceGained)
        || !reader.integer(data.mapItemsDropped)
        || !reader.integer(data.mapBossItemsDropped)
        || !reader.integer(data.mapItemsPickedUp)
        || (hasDropRarityStats
            && (!reader.integer(data.mapDroppedItemsByRarity[0])
                || !reader.integer(data.mapDroppedItemsByRarity[1])
                || !reader.integer(data.mapDroppedItemsByRarity[2])
                || !reader.integer(data.mapDroppedItemsByRarity[3])))
        || (hasRareLeaderProgress
            && (!reader.integer(data.mapRareLeadersDefeated)
                || !reader.integer(data.mapRareLeaderItemsDropped)
                || !reader.string(data.lastRareLeaderName)
                || !reader.string(data.lastRareLeaderRewardDescription)))
        || (hasFieldPackProgress && !reader.integer(data.fieldPacksCleared))
        || !reader.integer(data.lifeFlaskCharges)
        || !readSet(reader, data.unlockedSkills)
        || !readSet(reader, data.unlockedSupports)
        || (hasGemProgression
            && (!readLevelMap(reader, data.skillLevels)
                || !readLevelMap(reader, data.supportLevels)))
        || !reader.real(data.itemQuantityRewardMultiplier)
        || !reader.integer(data.forgeFragments)
        || !readPlayerState(reader, data.player, hasPoisonFields, passiveNodeCount)
        || !readSkillBarState(reader, data.skillBar)) {
        return false;
    }

    if (!hasGemProgression) {
        data.skillLevels.clear();
        data.supportLevels.clear();
    }

    std::uint32_t count = 0;
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }
    data.inventory.clear();
    data.inventory.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        Item item;
        if (!readItem(reader, item, hasPoisonFields)) {
            return false;
        }
        data.inventory.push_back(std::move(item));
    }
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }
    data.stash.clear();
    data.stash.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        Item item;
        if (!readItem(reader, item, hasPoisonFields)) {
            return false;
        }
        data.stash.push_back(std::move(item));
    }
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }
    data.droppedItems.clear();
    data.droppedItems.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        SavedDroppedItem dropped;
        if (!readVector2(reader, dropped.position)
            || !readItem(reader, dropped.item, hasPoisonFields)) {
            return false;
        }
        data.droppedItems.push_back(std::move(dropped));
    }
    if (!reader.integer(count) || count > MaxVectorLength) {
        return false;
    }
    data.mapEvents.clear();
    data.mapEvents.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        SavedMapEvent event;
        int type = 0;
        if (!reader.integer(type)
            || !validEnumValue(type, 0, static_cast<int>(MapEventType::Combination))
            || !reader.boolean(event.triggered)
            || !reader.boolean(event.completed)) {
            return false;
        }
        event.type = static_cast<MapEventType>(type);
        data.mapEvents.push_back(event);
    }
    if (!reader.integer(count) || count > MaxExplorationCells
        || !reader.raw(data.exploredCells, count)) {
        return false;
    }
    return reader.atEnd();
}

bool writeAtomically(const std::filesystem::path& path,
    const std::vector<unsigned char>& bytes, std::string* error) {
    const std::filesystem::path temporary = path.string() + ".tmp";
    std::error_code filesystemError;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), filesystemError);
        if (filesystemError) {
            setError(error, "cannot create save directory");
            return false;
        }
    }

    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(error, "cannot open temporary save file");
        return false;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    file.flush();
    if (!file) {
        file.close();
        std::filesystem::remove(temporary, filesystemError);
        setError(error, "cannot write temporary save file");
        return false;
    }
    file.close();

#ifdef _WIN32
    if (!MoveFileExW(temporary.wstring().c_str(), path.wstring().c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temporary, filesystemError);
        setError(error, "cannot replace save file");
        return false;
    }
    return true;
#else
    std::filesystem::rename(temporary, path, filesystemError);
    if (filesystemError) {
        std::filesystem::remove(temporary, filesystemError);
        setError(error, "cannot replace save file");
        return false;
    }
    return true;
#endif
}

bool readFile(const std::filesystem::path& path,
    std::vector<unsigned char>& bytes, std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(error, "save file does not exist");
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (file.bad()) {
        setError(error, "cannot read save file");
        return false;
    }
    return true;
}

} // namespace

bool SaveService::save(const std::filesystem::path& path,
    const SaveData& data, std::string* error) {
    Writer payload;
    writeSaveData(payload, data);

    Writer file;
    file.integer(SaveData::Magic);
    file.integer(SaveData::Version);
    file.integer<std::uint32_t>(static_cast<std::uint32_t>(payload.bytes().size()));
    file.integer(crc32(payload.bytes()));
    file.raw(payload.bytes());
    return writeAtomically(path, file.bytes(), error);
}

bool SaveService::load(const std::filesystem::path& path,
    SaveData& data, std::string* error) {
    std::vector<unsigned char> bytes;
    if (!readFile(path, bytes, error)) {
        return false;
    }

    Reader file(bytes);
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t payloadLength = 0;
    std::uint32_t expectedCrc = 0;
    if (!file.integer(magic) || !file.integer(version)
        || !file.integer(payloadLength) || !file.integer(expectedCrc)
        || magic != SaveData::Magic
        || (version != 3U && version != 4U && version != 5U
            && version != 6U && version != 7U && version != 8U
            && version != 9U && version != 10U && version != 11U
            && version != SaveData::Version)
        || payloadLength != file.remaining()) {
        setError(error, "invalid save header");
        return false;
    }

    std::vector<unsigned char> payload;
    if (!file.raw(payload, payloadLength) || crc32(payload) != expectedCrc) {
        setError(error, "corrupt save payload");
        return false;
    }

    Reader payloadReader(payload);
    SaveData restored;
    if (!readSaveData(
            payloadReader,
            restored,
            version >= 4U,
            version >= 11U,
            version >= 5U,
            version >= 6U ? PassiveTree::NodeCount : PassiveTree::LegacyNodeCount,
            version >= 7U,
            version >= 8U,
            version >= 9U,
            version >= 12U
        )) {
        setError(error, "invalid save payload");
        return false;
    }
    data = std::move(restored);
    return true;
}
