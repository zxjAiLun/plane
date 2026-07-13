#pragma once

#include <array>
#include <string>

#include "Config.hpp"
#include "PlayerStats.hpp"
#include "Skill.hpp"
#include "SkillLibrary.hpp"
#include "SupportLibrary.hpp"

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

    bool canCast(SkillSlot slot) const {
        const auto idx = slotIndex(slot);
        return elapsed_[idx] >= actualCooldowns_[idx];
    }

    void consumeCooldown(SkillSlot slot) {
        const auto idx = slotIndex(slot);
        elapsed_[idx] = 0.0f;
    }

    bool tryCast(SkillSlot slot) {
        if (!canCast(slot)) {
            return false;
        }

        consumeCooldown(slot);
        return true;
    }

    void applyStats(const PlayerStats& stats) {
        for (std::size_t i = 0; i < definitions_.size(); ++i) {
            const auto slot = static_cast<SkillSlot>(i);
            float cooldown = definitions_[i].cooldown;
            if (const auto* support = supportForSlot(slot)) {
                cooldown *= support->cooldownMultiplier;
            }
            if (slot == SkillSlot::Primary) {
                actualCooldowns_[i] = cooldown / stats.attackSpeedMultiplier;
            } else {
                actualCooldowns_[i] = cooldown;
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
        if (!supportNames_[idx].empty()) {
            const auto* support = SupportLibrary::find(supportNames_[idx]);
            if (!support || !SupportLibrary::supportsSkill(*support, *skill)) {
                supportNames_[idx].clear();
            }
        }
        actualCooldowns_[idx] = skill->cooldown;
        return true;
    }

    bool assignSupport(SkillSlot slot, const std::string& name) {
        const auto idx = slotIndex(slot);
        if (name.empty()) {
            supportNames_[idx].clear();
            return true;
        }

        const auto* support = SupportLibrary::find(name);
        if (!support || !SupportLibrary::supportsSkill(*support, definitions_[idx])) {
            return false;
        }

        supportNames_[idx] = name;
        return true;
    }

    const SkillDefinition& definition(SkillSlot slot) const {
        return definitions_[slotIndex(slot)];
    }

    const SupportDefinition* support(SkillSlot slot) const {
        return supportForSlot(slot);
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
        supportNames_.fill("");
        elapsed_.fill(999.0f);
    }

    const SupportDefinition* supportForSlot(SkillSlot slot) const {
        const auto& name = supportNames_[slotIndex(slot)];
        return name.empty() ? nullptr : SupportLibrary::find(name);
    }

private:
    std::array<SkillDefinition, static_cast<std::size_t>(SkillSlot::Count)> definitions_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> actualCooldowns_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> elapsed_{};
    std::array<std::string, static_cast<std::size_t>(SkillSlot::Count)> supportNames_{};
};
