#pragma once

#include <string>

enum class UpgradeType {
    Damage,
    FireRate,
    MoveSpeed,
    PickupRange,
    MaxHp
};

struct Upgrade {
    UpgradeType type;
    std::string name;
    std::string description;
};
