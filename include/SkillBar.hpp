#pragma once

#include <array>

#include "Config.hpp"
#include "PlayerStats.hpp"
#include "Skill.hpp"

class SkillBar {
public:
    SkillBar() {
        setDefaults();
    }

    void update(float dt) {
        for (auto& elapsed : elapsed_) {
            elapsed += dt;
        }
    }

    bool tryCast(SkillSlot slot) {
        const auto idx = slotIndex(slot);
        if (elapsed_[idx] < definitions_[idx].cooldown) {
            return false;
        }

        elapsed_[idx] = 0.0f;
        return true;
    }

    void applyStats(const PlayerStats& stats) {
        definitions_[slotIndex(SkillSlot::Primary)].cooldown =
            Config::PrimarySkillCooldown / stats.attackSpeedMultiplier;
    }

    void reset() {
        setDefaults();
    }

    const SkillDefinition& definition(SkillSlot slot) const {
        return definitions_[slotIndex(slot)];
    }

    float cooldownProgress(SkillSlot slot) const {
        const auto idx = slotIndex(slot);
        if (definitions_[idx].cooldown <= 0.0f) {
            return 1.0f;
        }

        const float progress = elapsed_[idx] / definitions_[idx].cooldown;
        return progress > 1.0f ? 1.0f : progress;
    }

private:
    static constexpr std::size_t slotIndex(SkillSlot slot) {
        return static_cast<std::size_t>(slot);
    }

    void setDefaults() {
        definitions_[slotIndex(SkillSlot::Primary)] = {
            SkillSlot::Primary,
            SkillCastType::Projectile,
            "Bolt",
            Config::PrimarySkillCooldown,
            0.0f,
            Config::ProjectileDamage,
            0.0f
        };
        definitions_[slotIndex(SkillSlot::Secondary)] = {
            SkillSlot::Secondary,
            SkillCastType::MouseTargetedArea,
            "Flare",
            Config::SecondarySkillCooldown,
            Config::SecondarySkillRadius,
            Config::SecondarySkillDamage,
            Config::SecondarySkillEffectDuration
        };
        definitions_[slotIndex(SkillSlot::Utility)] = {
            SkillSlot::Utility,
            SkillCastType::SelfCenteredArea,
            "Nova",
            Config::NovaCooldown,
            Config::NovaRadius,
            Config::NovaDamage,
            Config::NovaEffectDuration
        };
        definitions_[slotIndex(SkillSlot::Movement)] = {
            SkillSlot::Movement,
            SkillCastType::Dash,
            "Dash",
            Config::DashCooldown,
            0.0f,
            0,
            0.0f
        };
        elapsed_.fill(999.0f);
    }

private:
    std::array<SkillDefinition, static_cast<std::size_t>(SkillSlot::Count)> definitions_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> elapsed_{};
};
