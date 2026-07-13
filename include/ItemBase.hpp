#pragma once

#include <string>
#include <vector>

#include "EquipmentSlot.hpp"
#include "Stats.hpp"

enum class ItemBaseKind {
    Normal,
    BossRelic
};

enum class ItemBaseTheme {
    None,
    Brimstone,
    Storm,
    Brood
};

struct ItemBaseDefinition {
    std::string id;
    std::string name;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    Stats implicitStats;
    ItemBaseKind kind = ItemBaseKind::Normal;
    ItemBaseTheme theme = ItemBaseTheme::None;
    int requiredLevel = 1;
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

    static const ItemBaseDefinition& forBossTheme(ItemBaseTheme theme) {
        for (const auto& base : all()) {
            if (base.kind == ItemBaseKind::BossRelic && base.theme == theme) {
                return base;
            }
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
        int armor = 0
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
        return stats;
    }

    static std::vector<ItemBaseDefinition> buildBases() {
        return {
            // Weapon bases: choose between general, projectile and area scaling.
            {"weapon.rustbound-blade", "Rustbound Blade", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.05f), ItemBaseKind::Normal, ItemBaseTheme::None, 1},
            {"weapon.hunter-bow", "Hunter's Bow", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 2},
            {"weapon.warhammer", "Warhammer", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.0f, 0.96f, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 3},

            // Armor bases: trade life, armor and movement.
            {"armor.iron-vest", "Iron Vest", EquipmentSlot::Armor,
                makeStats(4, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2), ItemBaseKind::Normal, ItemBaseTheme::None, 1},
            {"armor.scaled-mail", "Scaled Mail", EquipmentSlot::Armor,
                makeStats(8), ItemBaseKind::Normal, ItemBaseTheme::None, 2},
            {"armor.windweave", "Windweave", EquipmentSlot::Armor,
                makeStats(2, 1.05f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1), ItemBaseKind::Normal, ItemBaseTheme::None, 2},

            // Ring bases: general damage, attack speed or pickup range.
            {"ring.cinder-band", "Cinder Band", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.05f), ItemBaseKind::Normal, ItemBaseTheme::None, 1},
            {"ring.quickband", "Quickband", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.06f), ItemBaseKind::Normal, ItemBaseTheme::None, 2},
            {"ring.scavenger-loop", "Scavenger Loop", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.12f), ItemBaseKind::Normal, ItemBaseTheme::None, 3},

            // Amulet bases: life, area damage or area radius.
            {"amulet.ironheart-pendant", "Ironheart Pendant", EquipmentSlot::Amulet,
                makeStats(10), ItemBaseKind::Normal, ItemBaseTheme::None, 1},
            {"amulet.sigil-of-focus", "Sigil of Focus", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.07f), ItemBaseKind::Normal, ItemBaseTheme::None, 2},
            {"amulet.wide-eyed-talisman", "Wide-Eyed Talisman", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.08f), ItemBaseKind::Normal, ItemBaseTheme::None, 3},

            // Boss relic bases keep a stable theme identity while their affix
            // contributions continue to scale with item level.
            {"boss.brimstone-brand", "Colossus Brand", EquipmentSlot::Weapon,
                makeStats(0, 1.0f, 1.06f, 1.0f, 1.0f, 1.0f, 1.03f), ItemBaseKind::BossRelic, ItemBaseTheme::Brimstone, 1},
            {"boss.storm-signet", "Herald's Signet", EquipmentSlot::Ring,
                makeStats(0, 1.0f, 1.0f, 1.04f, 1.0f, 1.04f), ItemBaseKind::BossRelic, ItemBaseTheme::Storm, 1},
            {"boss.brood-talisman", "Matriarch's Talisman", EquipmentSlot::Amulet,
                makeStats(0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.04f, 1.04f), ItemBaseKind::BossRelic, ItemBaseTheme::Brood, 1},
        };
    }
};
