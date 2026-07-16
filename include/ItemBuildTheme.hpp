#pragma once

enum class ItemBuildTheme {
    General,
    Projectile,
    Area,
    Survival,
    Loot,
    Fire,
    Cold,
    Lightning,
    Poison,
    Mana
};

inline const char* itemBuildThemeName(ItemBuildTheme theme) {
    switch (theme) {
        case ItemBuildTheme::General: return "General";
        case ItemBuildTheme::Projectile: return "Projectile";
        case ItemBuildTheme::Area: return "Area";
        case ItemBuildTheme::Survival: return "Survival";
        case ItemBuildTheme::Loot: return "Loot";
        case ItemBuildTheme::Fire: return "Fire";
        case ItemBuildTheme::Cold: return "Cold";
        case ItemBuildTheme::Lightning: return "Lightning";
        case ItemBuildTheme::Poison: return "Poison";
        case ItemBuildTheme::Mana: return "Mana";
    }
    return "Unknown";
}
