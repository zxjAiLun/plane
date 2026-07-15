#pragma once

#include <string>

#include "Ailment.hpp"
#include "DamageType.hpp"

enum class SkillSlot {
    Primary,
    Secondary,
    Utility,
    Movement,
    Count
};

enum class SkillCastType {
    Projectile,
    SelfCenteredArea,
    MouseTargetedArea,
    Dash
};

enum class SkillDeliveryType {
    Instant,
    DelayedArea
};

struct SkillDefinition {
    SkillSlot slot = SkillSlot::Primary;
    SkillCastType castType = SkillCastType::Projectile;
    std::string name;
    float cooldown = 0.0f;
    float radius = 0.0f;
    int baseDamage = 0;
    float effectDuration = 0.0f;
    int projectileCount = 1;
    float spreadAngle = 0.0f;
    AilmentDefinition ailment;
    float manaCost = 0.0f;
    DamageType damageType = DamageType::Physical;
    SkillDeliveryType delivery = SkillDeliveryType::Instant;
    float castDelay = 0.0f;
};
