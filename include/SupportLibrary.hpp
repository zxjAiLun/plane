#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "Skill.hpp"

enum class SupportKind {
    Pierce,
    Amplify,
    Quickcast,
    Volley,
    Trailblazer,
    Combustion,
    IgnitionSpread,
    DeepChill,
    Conductivity,
    Barrage,
    Concentration,
    Echo,
    Pinpoint,
    Toxicity,
    Contagion,
    ArcaneEfficiency,
    ElementalFocus,
    Vitality,
    Bloodletting,
    Rupture,
    GlacialLock,
    ShatteringIce,
    MinionMastery
};

struct SupportDefinition {
    SupportKind kind = SupportKind::Pierce;
    std::string name;
    std::string description;
    float damageMultiplier = 1.0f;
    float radiusMultiplier = 1.0f;
    float cooldownMultiplier = 1.0f;
    int pierceCount = 0;
    int extraProjectileCount = 0;
    float extraSpreadAngle = 0.0f;
    int dashBaseDamage = 0;
    float dashRadius = 0.0f;
    float effectDuration = 0.0f;
    float ailmentDamageMultiplier = 1.0f;
    float ailmentDurationMultiplier = 1.0f;
    float chillMagnitudeMultiplier = 1.0f;
    int ignitePenetration = 0;
    int chillPenetration = 0;
    int repeatCountBonus = 0;
    float shockMagnitudeMultiplier = 1.0f;
    int shockPenetration = 0;
    int poisonPenetration = 0;
    float poisonSpreadRadius = 0.0f;
    float poisonSpreadMultiplier = 0.0f;
    float manaCostMultiplier = 1.0f;
    DamageType requiredDamageType = DamageType::Physical;
    float elementalDamageMultiplier = 1.0f;
    int healOnHitBonus = 0;
    int bleedPenetration = 0;
    int physicalPenetration = 0;
    float igniteSpreadRadius = 0.0f;
    float igniteSpreadMultiplier = 0.0f;
    float freezeDuration = 0.0f;
    float shatterRadius = 0.0f;
    float shatterDamageMultiplier = 0.0f;
    int summonCountBonus = 0;
    float summonDurationMultiplier = 1.0f;
    float summonDamageMultiplier = 1.0f;
    float summonAttackIntervalMultiplier = 1.0f;
    float summonHpMultiplier = 1.0f;
};

inline constexpr std::size_t SupportLinkCount = 2;
using SupportNameList = std::array<std::string, SupportLinkCount>;
using SupportList = std::array<const SupportDefinition*, SupportLinkCount>;

class SupportLibrary {
public:
    static const std::vector<SupportDefinition>& all() {
        static const std::vector<SupportDefinition> supports = {
            {SupportKind::Pierce, "Pierce", "+1 projectile pierce", 1.0f, 1.0f, 1.0f, 1},
            {SupportKind::Amplify, "Amplify", "+35% area radius, +20% cooldown", 1.0f, 1.35f, 1.20f, 0},
            {SupportKind::Quickcast, "Quickcast", "-30% cooldown, -15% damage", 0.85f, 1.0f, 0.70f, 0},
            {SupportKind::Volley, "Volley", "+2 projectiles, wider spread, -20% damage", 0.80f, 1.0f, 1.0f, 0, 2, 22.0f},
            {SupportKind::Trailblazer, "Trailblazer", "Dash impact: area damage at landing", 1.0f, 1.0f, 1.25f, 0, 0, 0.0f, 2, 90.0f, 0.30f},
            {SupportKind::Combustion, "Combustion", "-25% hit damage, +80% Ignite damage, +25% duration", 0.75f, 1.0f, 1.0f, 0, 0, 0.0f, 0, 0.0f, 0.0f, 1.80f, 1.25f},
            [] {
                SupportDefinition support;
                support.kind = SupportKind::IgnitionSpread;
                support.name = "Emberfall";
                support.description = "-10% hit damage, +35% Ignite damage; Ignite spreads on death";
                support.damageMultiplier = 0.90f;
                support.ailmentDamageMultiplier = 1.35f;
                support.ailmentDurationMultiplier = 1.10f;
                support.igniteSpreadRadius = 100.0f;
                support.igniteSpreadMultiplier = 0.50f;
                return support;
            }(),
            {SupportKind::DeepChill, "Deep Chill", "+50% Chill duration, stronger Chill, +20% penetration, +15% cooldown", 1.0f, 1.0f, 1.15f, 0, 0, 0.0f, 0, 0.0f, 0.0f, 1.0f, 1.50f, 1.40f, 0, 20},
            {SupportKind::Conductivity, "Conductivity", "+25% Shock effect, +20% Shock penetration, +20% duration", 1.0f, 1.0f, 1.0f, 0, 0, 0.0f, 0, 0.0f, 0.0f, 1.0f, 1.20f, 1.0f, 0, 0, 0, 1.25f, 20},
            [] {
                SupportDefinition support;
                support.kind = SupportKind::GlacialLock;
                support.name = "Glacial Lock";
                support.description = "-15% hit damage, Chill freezes non-Boss enemies for 0.55s";
                support.damageMultiplier = 0.85f;
                support.chillMagnitudeMultiplier = 1.20f;
                support.freezeDuration = 0.55f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ShatteringIce;
                support.name = "Shattering Ice";
                support.description = "-20% hit damage; frozen enemies shatter for 55% Cold damage in 78 radius";
                support.damageMultiplier = 0.80f;
                support.shatterRadius = 78.0f;
                support.shatterDamageMultiplier = 0.55f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::MinionMastery;
                support.name = "Minion Mastery";
                support.description = "+1 Wisp, +35% minion damage, +25% duration, -20% attack speed";
                support.summonCountBonus = 1;
                support.summonDurationMultiplier = 1.25f;
                support.summonDamageMultiplier = 1.35f;
                support.summonAttackIntervalMultiplier = 1.20f;
                support.summonHpMultiplier = 1.35f;
                return support;
            }(),
            {SupportKind::Barrage, "Barrage", "+1 projectile, wider spread, -12% damage", 0.88f, 1.0f, 1.0f, 0, 1, 12.0f},
            {SupportKind::Concentration, "Concentration", "+22% area damage, -22% radius, +12% cooldown", 1.22f, 0.78f, 1.12f},
            {
                SupportKind::Echo,
                "Echo",
                "Area skills repeat once at 65% damage, +35% cooldown",
                0.65f,
                1.0f,
                1.35f,
                0,
                0,
                0.0f,
                0,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                0,
                0,
                1
            },
            {
                SupportKind::Pinpoint,
                "Pinpoint",
                "+28% projectile damage, -20% spread, +20% cooldown",
                1.28f,
                1.0f,
                1.20f,
                0,
                0,
                -20.0f
            },
            {
                SupportKind::Toxicity,
                "Toxicity",
                "+60% Poison damage, +25% duration, +20% Poison penetration",
                1.0f,
                1.0f,
                1.0f,
                0,
                0,
                0.0f,
                0,
                0.0f,
                0.0f,
                1.60f,
                1.25f,
                1.0f,
                0,
                0,
                0,
                1.0f,
                0,
                20
            },
            {
                SupportKind::Contagion,
                "Contagion",
                "-8% hit damage, Poison spreads on death to nearby enemies",
                0.92f,
                1.0f,
                1.0f,
                0,
                0,
                0.0f,
                0,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                0,
                0,
                0,
                1.0f,
                0,
                0,
                120.0f,
                0.45f
            },
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ArcaneEfficiency;
                support.name = "Arcane Efficiency";
                support.description = "-25% Mana cost, +10% cooldown, -10% damage";
                support.damageMultiplier = 0.90f;
                support.cooldownMultiplier = 1.10f;
                support.manaCostMultiplier = 0.75f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ElementalFocus;
                support.name = "Ember Focus";
                support.description = "+28% Fire damage, -10% Ignite duration";
                support.requiredDamageType = DamageType::Fire;
                support.elementalDamageMultiplier = 1.28f;
                support.ailmentDurationMultiplier = 0.90f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ElementalFocus;
                support.name = "Glacial Focus";
                support.description = "+26% Cold damage, -10% Chill strength";
                support.requiredDamageType = DamageType::Cold;
                support.elementalDamageMultiplier = 1.26f;
                support.chillMagnitudeMultiplier = 0.90f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ElementalFocus;
                support.name = "Storm Focus";
                support.description = "+26% Lightning damage, +10% cooldown";
                support.requiredDamageType = DamageType::Lightning;
                support.elementalDamageMultiplier = 1.26f;
                support.cooldownMultiplier = 1.10f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::ElementalFocus;
                support.name = "Venom Focus";
                support.description = "+28% Poison damage, -12% hit damage";
                support.requiredDamageType = DamageType::Poison;
                support.elementalDamageMultiplier = 1.28f;
                support.damageMultiplier = 0.88f;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::Vitality;
                support.name = "Vitality";
                support.description = "+1 HP per hit for recovery skills, -15% damage";
                support.damageMultiplier = 0.85f;
                support.healOnHitBonus = 1;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::Bloodletting;
                support.name = "Bloodletting";
                support.description = "+70% Bleed damage, +25% duration, +20% Bleed and Physical penetration, -15% hit damage";
                support.damageMultiplier = 0.85f;
                support.ailmentDamageMultiplier = 1.70f;
                support.ailmentDurationMultiplier = 1.25f;
                support.bleedPenetration = 20;
                support.physicalPenetration = 20;
                return support;
            }(),
            [] {
                SupportDefinition support;
                support.kind = SupportKind::Rupture;
                support.name = "Rupture";
                support.description = "-25% hit damage, +35% Bleed damage, +40% duration, +10% Bleed penetration";
                support.damageMultiplier = 0.75f;
                support.ailmentDamageMultiplier = 1.35f;
                support.ailmentDurationMultiplier = 1.40f;
                support.bleedPenetration = 10;
                return support;
            }(),
        };
        return supports;
    }

    static const SupportDefinition* find(const std::string& name) {
        for (const auto& support : all()) {
            if (support.name == name) {
                return &support;
            }
        }
        return nullptr;
    }

    static bool supportsSkill(const SupportDefinition& support, const SkillDefinition& skill) {
        switch (support.kind) {
            case SupportKind::Pierce:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Amplify:
                return skill.castType == SkillCastType::SelfCenteredArea
                    || skill.castType == SkillCastType::MouseTargetedArea;
            case SupportKind::Quickcast:
                return skill.castType != SkillCastType::Dash;
            case SupportKind::Volley:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Trailblazer:
                return skill.castType == SkillCastType::Dash;
            case SupportKind::Combustion:
                return skill.ailment.type == AilmentType::Ignite;
            case SupportKind::IgnitionSpread:
                return skill.ailment.type == AilmentType::Ignite;
            case SupportKind::DeepChill:
                return skill.ailment.type == AilmentType::Chill;
            case SupportKind::Conductivity:
                return skill.ailment.type == AilmentType::Shock;
            case SupportKind::GlacialLock:
                return skill.ailment.type == AilmentType::Chill;
            case SupportKind::ShatteringIce:
                return skill.ailment.type == AilmentType::Chill;
            case SupportKind::MinionMastery:
                return skill.summonCount > 0;
            case SupportKind::Toxicity:
                return skill.ailment.type == AilmentType::Poison;
            case SupportKind::Contagion:
                return skill.ailment.type == AilmentType::Poison;
            case SupportKind::Barrage:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::Concentration:
                return skill.castType == SkillCastType::SelfCenteredArea
                    || skill.castType == SkillCastType::MouseTargetedArea;
            case SupportKind::Echo:
                return skill.castType == SkillCastType::SelfCenteredArea
                    || skill.castType == SkillCastType::MouseTargetedArea;
            case SupportKind::Pinpoint:
                return skill.castType == SkillCastType::Projectile;
            case SupportKind::ArcaneEfficiency:
                return skill.castType != SkillCastType::Dash;
            case SupportKind::ElementalFocus:
                return skill.damageType == support.requiredDamageType;
            case SupportKind::Vitality:
                return skill.healOnHit > 0;
            case SupportKind::Bloodletting:
                return skill.damageType == DamageType::Physical
                    && skill.ailment.type == AilmentType::Bleed;
            case SupportKind::Rupture:
                return skill.damageType == DamageType::Physical
                    && skill.ailment.type == AilmentType::Bleed;
        }

        return false;
    }
};
