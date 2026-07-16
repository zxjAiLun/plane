#pragma once

#include "ItemBuildTheme.hpp"

enum class AffixTag {
    None,
    Damage,
    AttackSpeed,
    MoveSpeed,
    Pickup,
    Survival,
    Projectile,
    Area,
    Armor,
    Fire,
    Cold,
    Lightning,
    Poison
};

struct LootBias {
    AffixTag primaryTag = AffixTag::None;
    float primaryWeightMultiplier = 1.0f;
    AffixTag secondaryTag = AffixTag::None;
    float secondaryWeightMultiplier = 1.0f;
    ItemBuildTheme baseTheme = ItemBuildTheme::General;
    float baseThemeWeightMultiplier = 1.0f;
};

inline const char* affixTagName(AffixTag tag) {
    switch (tag) {
        case AffixTag::None: return "None";
        case AffixTag::Damage: return "Damage";
        case AffixTag::AttackSpeed: return "Attack Speed";
        case AffixTag::MoveSpeed: return "Move Speed";
        case AffixTag::Pickup: return "Pickup";
        case AffixTag::Survival: return "Survival";
        case AffixTag::Projectile: return "Projectile";
        case AffixTag::Area: return "Area";
        case AffixTag::Armor: return "Armor";
        case AffixTag::Fire: return "Fire";
        case AffixTag::Cold: return "Cold";
        case AffixTag::Lightning: return "Lightning";
        case AffixTag::Poison: return "Poison";
    }
    return "Unknown";
}
