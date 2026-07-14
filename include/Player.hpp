#pragma once

#include <array>
#include <optional>

#include "Ailment.hpp"
#include "Vector2.hpp"
#include "PlayerStats.hpp"
#include "Upgrade.hpp"
#include "Equipment.hpp"
#include "Item.hpp"
#include "PassiveTree.hpp"

struct PlayerSaveState {
    Vector2 position;
    int hp = 0;
    float mana = 0.0f;
    int level = 1;
    int exp = 0;
    int expToNextLevel = 1;
    int talentPoints = 0;
    PlayerStats upgradeStats;
    std::array<bool, 20> allocatedPassiveNodes{};
    std::array<std::optional<Item>, EquipmentSlotCount> equipment;
};

class Player {
public:
    Player();

    void update(float dt);

    void moveLeft(float dt);
    void moveRight(float dt);
    void moveUp(float dt);
    void moveDown(float dt);
    void setPosition(const Vector2& position);
    void setBounds(const Vector2& bounds);
    void clearAilments();

    int takeDamage(int damage);
    int heal(int amount);
    bool canSpendMana(float amount) const;
    bool spendMana(float amount);
    bool isDead() const;
    AilmentTickResult updateAilments(float dt);
    void applyIgnite(int damagePerTick, float duration);
    void applyChill(float speedMultiplier, float duration);
    void applyShock(float damageTakenMultiplier, float duration);

    void gainExp(int amount);
    void applyUpgrade(UpgradeType type);
    bool canSpendTalentPoint() const;
    bool spendPassivePoint(std::size_t nodeIndex);
    bool canEquipItem(const Item& item) const;
    std::optional<int> requiredLevelForItem(const Item& item) const;
    std::optional<Item> equipItem(Item item);
    PlayerSaveState saveState() const;
    bool restoreState(const PlayerSaveState& state, const Vector2& bounds);

    const Vector2& position() const;
    float radius() const;
    float moveSpeed() const;
    bool isIgnited() const;
    bool isChilled() const;
    bool isShocked() const;
    float chillTimeRemaining() const;
    float shockTimeRemaining() const;
    float igniteTimeRemaining() const;
    float chillSpeedMultiplier() const;
    float damageTakenMultiplier() const;
    int hp() const;
    int maxHp() const;
    int level() const;
    int exp() const;
    int expToNextLevel() const;
    int talentPoints() const;
    float mana() const;
    float maxMana() const;
    float manaRegenPerSecond() const;
    const PlayerStats& stats() const;
    const Equipment& equipment() const;
    const PassiveTree& passiveTree() const;

private:
    void recalculateStats();

private:
    Vector2 position_;
    Vector2 bounds_;
    float baseSpeed_;
    float radius_;
    int hp_;
    int maxHp_;
    float mana_;
    float maxMana_;
    float manaRegenPerSecond_;

    int level_;
    int exp_;
    int expToNextLevel_;
    int talentPoints_;

    PlayerStats upgradeStats_;
    PlayerStats stats_;
    Equipment equipment_;
    PassiveTree passiveTree_;
    int igniteDamagePerTick_ = 0;
    float igniteTimer_ = 0.0f;
    float igniteTickTimer_ = 0.0f;
    float chillTimer_ = 0.0f;
    float chillSpeedMultiplier_ = 1.0f;
    float shockTimer_ = 0.0f;
    float shockDamageTakenMultiplier_ = 1.0f;
};
