#pragma once

enum class DamageType {
    Physical,
    Fire,
    Cold,
    Lightning,
    Poison
};

inline const char* damageTypeName(DamageType type) {
    switch (type) {
        case DamageType::Physical: return "Physical";
        case DamageType::Fire: return "Fire";
        case DamageType::Cold: return "Cold";
        case DamageType::Lightning: return "Lightning";
        case DamageType::Poison: return "Poison";
    }
    return "Unknown";
}
