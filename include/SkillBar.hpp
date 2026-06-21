#pragma once

#include <array>

#include "Config.hpp"
#include "PlayerStats.hpp"
#include "Skill.hpp"
#include "SkillLibrary.hpp"

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
        if (elapsed_[idx] < actualCooldowns_[idx]) {
            return false;
        }

        elapsed_[idx] = 0.0f;
        return true;
    }

    void applyStats(const PlayerStats& stats) {
        for (std::size_t i = 0; i < definitions_.size(); ++i) {
            const auto slot = static_cast<SkillSlot>(i);
            if (slot == SkillSlot::Primary) {
                actualCooldowns_[i] = definitions_[i].cooldown / stats.attackSpeedMultiplier;
            } else {
                actualCooldowns_[i] = definitions_[i].cooldown;
            }
        }
    }

    void reset() {
        setDefaults();
    }

    bool assignSkill(SkillSlot slot, const std::string& name) {
        const auto* skill = SkillLibrary::find(name);
        if (!skill || skill->slot != slot) {
            return false;
        }

        const auto idx = slotIndex(slot);
        definitions_[idx] = *skill;
        actualCooldowns_[idx] = skill->cooldown;
        return true;
    }

    const SkillDefinition& definition(SkillSlot slot) const {
        return definitions_[slotIndex(slot)];
    }

    float cooldownProgress(SkillSlot slot) const {
        const auto idx = slotIndex(slot);
        if (actualCooldowns_[idx] <= 0.0f) {
            return 1.0f;
        }

        const float progress = elapsed_[idx] / actualCooldowns_[idx];
        return progress > 1.0f ? 1.0f : progress;
    }

private:
    static constexpr std::size_t slotIndex(SkillSlot slot) {
        return static_cast<std::size_t>(slot);
    }

    void setDefaults() {
        definitions_[slotIndex(SkillSlot::Primary)] = SkillLibrary::spreadShot();
        definitions_[slotIndex(SkillSlot::Secondary)] = SkillLibrary::meteor();
        definitions_[slotIndex(SkillSlot::Utility)] = SkillLibrary::pulse();
        definitions_[slotIndex(SkillSlot::Movement)] = SkillLibrary::dash();
        for (std::size_t i = 0; i < definitions_.size(); ++i) {
            actualCooldowns_[i] = definitions_[i].cooldown;
        }
        elapsed_.fill(999.0f);
    }

private:
    std::array<SkillDefinition, static_cast<std::size_t>(SkillSlot::Count)> definitions_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> actualCooldowns_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> elapsed_{};
};
