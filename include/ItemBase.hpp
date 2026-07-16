#pragma once

#include <string>
#include <vector>

#include "EquipmentSlot.hpp"
#include "ItemBuildTheme.hpp"
#include "Stats.hpp"

enum class ItemBaseKind {
    Normal,
    BossRelic
};

enum class ItemBaseTheme {
    None,
    Brimstone,
    Storm,
    Brood,
    Frost,
    Archive,
    Obsidian
};

struct ItemBaseDefinition {
    std::string id;
    std::string name;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    Stats implicitStats;
    ItemBaseKind kind = ItemBaseKind::Normal;
    ItemBaseTheme theme = ItemBaseTheme::None;
    int requiredLevel = 1;
    ItemBuildTheme buildTheme = ItemBuildTheme::General;
    int variant = 0;
};

class ItemBaseLibrary {
public:
    static const std::vector<ItemBaseDefinition>& all() {
        static const std::vector<ItemBaseDefinition> bases = buildBases();
        return bases;
    }

    static const ItemBaseDefinition* find(const std::string& id) {
        for (const auto& base : all()) {
            if (base.id == id) {
                return &base;
            }
        }
        return nullptr;
    }

    static const ItemBaseDefinition& forBossTheme(ItemBaseTheme theme, int variant = 0) {
        const ItemBaseDefinition* fallback = nullptr;
        for (const auto& base : all()) {
            if (base.kind != ItemBaseKind::BossRelic || base.theme != theme) {
                continue;
            }
            if (base.variant == variant) {
                return base;
            }
            if (base.variant == 0) {
                fallback = &base;
            }
        }

        if (fallback != nullptr) {
            return *fallback;
        }
        return all().front();
    }

private:
    static Stats makeStats(
        int maxHp = 0,
        float moveSpeedMultiplier = 1.0f,
        float damageMultiplier = 1.0f,
        float attackSpeedMultiplier = 1.0f,
        float pickupRangeMultiplier = 1.0f,
        float projectileDamageMultiplier = 1.0f,
        float areaDamageMultiplier = 1.0f,
        float areaRadiusMultiplier = 1.0f,
        int armor = 0,
        float itemQuantityMultiplier = 1.0f,
        float poisonDamageMultiplier = 1.0f,
        float coldDamageMultiplier = 1.0f,
        float maxManaMultiplier = 1.0f,
        float manaRegenMultiplier = 1.0f,
        float skillCostMultiplier = 1.0f
    ) {
        Stats stats;
        stats.maxHp = maxHp;
        stats.moveSpeedMultiplier = moveSpeedMultiplier;
        stats.damageMultiplier = damageMultiplier;
        stats.attackSpeedMultiplier = attackSpeedMultiplier;
        stats.pickupRangeMultiplier = pickupRangeMultiplier;
        stats.projectileDamageMultiplier = projectileDamageMultiplier;
        stats.areaDamageMultiplier = areaDamageMultiplier;
        stats.areaRadiusMultiplier = areaRadiusMultiplier;
        stats.armor = armor;
        stats.itemQuantityMultiplier = itemQuantityMultiplier;
        stats.poisonDamageMultiplier = poisonDamageMultiplier;
        stats.coldDamageMultiplier = coldDamageMultiplier;
        stats.maxManaMultiplier = maxManaMultiplier;
        stats.manaRegenMultiplier = manaRegenMultiplier;
        stats.skillCostMultiplier = skillCostMultiplier;
        return stats;
    }

    static std::vector<ItemBaseDefinition> buildBases() {
        return {
            // Weapon bases: choose between general, projectile and area scaling.
            {"weapon.rustbound-blade", "Rustbound Blade", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.05f), ItemBaseKind::Normal, ItemBaseTheme::None, 1,
                    ItemBuildTheme::General},
            {"weapon.hunter-bow", "Hunter's Bow", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 2,
                    ItemBuildTheme::Projectile},
            {"weapon.warhammer", "Warhammer", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 0.96f, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 3,
                    ItemBuildTheme::Area},
            {"weapon.aether-staff", "Aether Staff", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 0.98f, 0.94f, 1.0f, 1.0f, 1.0f, 1.0f,
                    0, 1.0f, 1.0f, 1.0f, 1.10f, 1.08f, 0.96f),
                ItemBaseKind::Normal, ItemBaseTheme::None, 3, ItemBuildTheme::Mana},

            // Armor bases: trade life, armor and movement.
            {"armor.iron-vest", "Iron Vest", EquipmentSlot::Armor,
                makeStats(4, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2), ItemBaseKind::Normal, ItemBaseTheme::None, 1,
                    ItemBuildTheme::Survival},
            {"armor.scaled-mail", "Scaled Mail", EquipmentSlot::Armor,
                makeStats(8), ItemBaseKind::Normal, ItemBaseTheme::None, 2,
                    ItemBuildTheme::Survival},
            {"armor.windweave", "Windweave", EquipmentSlot::Armor,
                makeStats(2, 1.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1, 1.05f),
                    ItemBaseKind::Normal, ItemBaseTheme::None, 2, ItemBuildTheme::Loot},
            {"armor.sageweave", "Sageweave", EquipmentSlot::Armor,
                makeStats(3, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1,
                    1.0f, 1.0f, 1.0f, 1.12f, 1.15f),
                ItemBaseKind::Normal, ItemBaseTheme::None, 3, ItemBuildTheme::Mana},

            // Ring bases: general damage, attack speed or pickup range.
            {"ring.cinder-band", "Cinder Band", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.05f), ItemBaseKind::Normal, ItemBaseTheme::None, 1,
                    ItemBuildTheme::General},
            {"ring.quickband", "Quickband", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 2,
                    ItemBuildTheme::Projectile},
            {"ring.scavenger-loop", "Scavenger Loop", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.12f), ItemBaseKind::Normal, ItemBaseTheme::None, 3,
                    ItemBuildTheme::Loot},
            {"ring.aether-loop", "Aether Loop", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                    0, 1.0f, 1.0f, 1.0f, 1.15f, 1.10f, 0.95f),
                ItemBaseKind::Normal, ItemBaseTheme::None, 3, ItemBuildTheme::Mana},

            // Amulet bases: life, area damage or area radius.
            {"amulet.ironheart-pendant", "Ironheart Pendant", EquipmentSlot::Amulet,
                makeStats(10), ItemBaseKind::Normal, ItemBaseTheme::None, 1,
                    ItemBuildTheme::Survival},
            {"amulet.sigil-of-focus", "Sigil of Focus", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.07f), ItemBaseKind::Normal, ItemBaseTheme::None, 2,
                    ItemBuildTheme::Area},
            {"amulet.wide-eyed-talisman", "Wide-Eyed Talisman", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f), ItemBaseKind::Normal, ItemBaseTheme::None, 3,
                    ItemBuildTheme::Area},
            {"amulet.sage-codex", "Sage Codex", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                    0, 1.0f, 1.0f, 1.0f, 1.20f, 1.12f, 0.92f),
                ItemBaseKind::Normal, ItemBaseTheme::None, 4, ItemBuildTheme::Mana},

            // Boss relic bases keep a stable theme identity while their affix
            // contributions continue to scale with item level.
            {"boss.brimstone-brand", "Colossus Brand", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.06f, 1.0f, 1.0f, 1.0f, 1.03f), ItemBaseKind::BossRelic, ItemBaseTheme::Brimstone, 1,
                    ItemBuildTheme::Fire},
            {"boss.storm-signet", "Herald's Signet", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.04f, 1.0f, 1.04f), ItemBaseKind::BossRelic, ItemBaseTheme::Storm, 1,
                    ItemBuildTheme::Lightning},
            {"boss.brood-talisman", "Matriarch's Talisman", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.04f, 1.04f, 0, 1.0f, 1.04f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Brood, 1, ItemBuildTheme::Poison},
            {"boss.frostbound-loop", "Frostbound Loop", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.03f,
                    0, 1.0f, 1.0f, 1.06f),
                ItemBaseKind::BossRelic, ItemBaseTheme::Frost, 1, ItemBuildTheme::Cold},
            {"boss.tidebound-ledger", "Tidebound Ledger", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f, 1.0f, 1.0f,
                    0, 1.0f, 1.0f, 1.06f),
                ItemBaseKind::BossRelic, ItemBaseTheme::Archive, 1, ItemBuildTheme::Cold},
            {"boss.obsidian-crown", "Obsidian Crown", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f, 1.05f),
                ItemBaseKind::BossRelic, ItemBaseTheme::Obsidian, 1, ItemBuildTheme::Area},

            // Alternate relic bases keep the same theme identity and passive
            // effect, but give later encounters a different chase target.
            {"boss.ashen-crucible", "Ashen Crucible", EquipmentSlot::Amulet,
                makeStats(8, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 1.04f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Brimstone, 1, ItemBuildTheme::Fire, 1},
            {"boss.tempest-bow", "Tempest Bow", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 1.04f, 1.0f, 1.08f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Storm, 1, ItemBuildTheme::Projectile, 1},
            {"boss.broodscale-band", "Broodscale Band", EquipmentSlot::Ring,
                makeStats(4, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0, 1.0f, 1.06f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Brood, 1, ItemBuildTheme::Poison, 1},
            {"boss.winterheart-pendant", "Winterheart Pendant", EquipmentSlot::Amulet,
                makeStats(6, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.05f,
                    0, 1.0f, 1.0f, 1.08f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Frost, 1, ItemBuildTheme::Cold, 1},
            {"boss.drowned-compass", "Drowned Compass", EquipmentSlot::Amulet,
                makeStats(5, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f, 1.0f, 1.0f,
                    0, 1.0f, 1.0f, 1.08f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Archive, 1,
                    ItemBuildTheme::Projectile, 1},
            {"boss.blackglass-heart", "Blackglass Heart", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f, 1.08f),
                    ItemBaseKind::BossRelic, ItemBaseTheme::Obsidian, 1,
                    ItemBuildTheme::Area, 1},
        };
    }
};
