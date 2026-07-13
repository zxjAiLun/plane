#pragma once

#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "Config.hpp"
#include "PlayerStats.hpp"
#include "Skill.hpp"
#include "SkillLibrary.hpp"
#include "SupportLibrary.hpp"

struct SkillBarSaveState {
    std::array<std::string, static_cast<std::size_t>(SkillSlot::Count)> skills;
    std::array<SupportNameList, static_cast<std::size_t>(SkillSlot::Count)> supports;
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> elapsed{};
};

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
            for (const auto* support : supportDefinitions(slot)) {
                if (support == nullptr) {
                    continue;
                }
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
        for (std::size_t link = 0; link < SupportLinkCount; ++link) {
            if (supportNames_[idx][link].empty()) {
                continue;
            }

            const auto* support = SupportLibrary::find(supportNames_[idx][link]);
            bool duplicate = false;
            for (std::size_t other = 0; other < link; ++other) {
                duplicate = supportNames_[idx][other] == supportNames_[idx][link];
                if (duplicate) {
                    break;
                }
            }
            if (!support || !SupportLibrary::supportsSkill(*support, *skill)
                || duplicate || link >= supportLinkCount(slot)) {
                supportNames_[idx][link].clear();
            }
        }
        actualCooldowns_[idx] = skill->cooldown;
        return true;
    }

    bool assignSupport(SkillSlot slot, const std::string& name, std::size_t linkIndex = 0) {
        const auto idx = slotIndex(slot);
        if (linkIndex >= supportLinkCount(slot)) {
            return false;
        }
        if (name.empty()) {
            supportNames_[idx][linkIndex].clear();
            return true;
        }

        const auto* support = SupportLibrary::find(name);
        if (!support || !SupportLibrary::supportsSkill(*support, definitions_[idx])) {
            return false;
        }

        for (std::size_t other = 0; other < supportLinkCount(slot); ++other) {
            if (other != linkIndex && supportNames_[idx][other] == name) {
                return false;
            }
        }

        supportNames_[idx][linkIndex] = name;
        return true;
    }

    SkillBarSaveState saveState() const {
        SkillBarSaveState state;
        for (std::size_t index = 0; index < definitions_.size(); ++index) {
            state.skills[index] = definitions_[index].name;
            for (std::size_t link = 0; link < SupportLinkCount; ++link) {
                state.supports[index][link] = supportNames_[index][link];
            }
            state.elapsed[index] = elapsed_[index];
        }
        return state;
    }

    bool restoreState(const SkillBarSaveState& state) {
        SkillBar restored = *this;
        for (std::size_t index = 0; index < definitions_.size(); ++index) {
            const auto* skill = SkillLibrary::find(state.skills[index]);
            if (!skill || static_cast<std::size_t>(skill->slot) != index
                || !std::isfinite(state.elapsed[index])
                || state.elapsed[index] < 0.0f) {
                return false;
            }

            restored.definitions_[index] = *skill;
            restored.supportNames_[index].fill("");
            const auto slot = static_cast<SkillSlot>(index);
            for (std::size_t link = 0; link < SupportLinkCount; ++link) {
                const std::string& supportName = state.supports[index][link];
                if (link >= supportLinkCount(slot) && !supportName.empty()) {
                    return false;
                }
                if (supportName.empty()) {
                    continue;
                }

                const auto* support = SupportLibrary::find(supportName);
                if (!support || !SupportLibrary::supportsSkill(*support, *skill)
                    || !restored.assignSupport(slot, supportName, link)) {
                    return false;
                }
            }
            restored.elapsed_[index] = state.elapsed[index];
        }

        *this = std::move(restored);
        return true;
    }

    const SkillDefinition& definition(SkillSlot slot) const {
        return definitions_[slotIndex(slot)];
    }

    const SupportDefinition* support(SkillSlot slot) const {
        return supportForSlot(slot);
    }

    const SupportDefinition* supportAt(SkillSlot slot, std::size_t linkIndex) const {
        if (linkIndex >= supportLinkCount(slot)) {
            return nullptr;
        }

        const auto& name = supportNames_[slotIndex(slot)][linkIndex];
        return name.empty() ? nullptr : SupportLibrary::find(name);
    }

    SupportList supportDefinitions(SkillSlot slot) const {
        SupportList result{};
        for (std::size_t link = 0; link < supportLinkCount(slot); ++link) {
            result[link] = supportAt(slot, link);
        }
        return result;
    }

    static constexpr std::size_t supportLinkCount(SkillSlot slot) {
        if (slot == SkillSlot::Count) {
            return 0;
        }
        return slot == SkillSlot::Movement ? 1 : SupportLinkCount;
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
        for (auto& names : supportNames_) {
            names.fill("");
        }
        elapsed_.fill(999.0f);
    }

    const SupportDefinition* supportForSlot(SkillSlot slot) const {
        return supportAt(slot, 0);
    }

private:
    std::array<SkillDefinition, static_cast<std::size_t>(SkillSlot::Count)> definitions_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> actualCooldowns_{};
    std::array<float, static_cast<std::size_t>(SkillSlot::Count)> elapsed_{};
    std::array<SupportNameList, static_cast<std::size_t>(SkillSlot::Count)> supportNames_{};
};
