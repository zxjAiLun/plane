#pragma once

#include <array>

struct InputBindingDefinition {
    const char* key;
    const char* action;
};

namespace InputBindingLibrary {

inline const std::array<InputBindingDefinition, 5>& pauseActions() {
    static const std::array<InputBindingDefinition, 5> actions = {{
        {"Esc", "Continue"},
        {"F5", "Save Run"},
        {"F9", "Load Run"},
        {"R", "Restart Run"},
        {"Q", "Quit to Desktop"}
    }};
    return actions;
}

inline const std::array<InputBindingDefinition, 9>& gameplayActions() {
    static const std::array<InputBindingDefinition, 9> actions = {{
        {"WASD / Arrows", "Move"},
        {"Mouse 1", "Primary Skill"},
        {"Mouse 2", "Secondary Skill"},
        {"Q / Space", "Utility / Dash"},
        {"E / G", "Next Map / Life Flask"},
        {"P / K", "Passive Tree / Skills"},
        {"1-9 / 0", "Equip / Choose"},
        {"F / Tab / Del", "Events / Inventory"},
        {"V / C / I / O", "Craft / Salvage / Stash"}
    }};
    return actions;
}

} // namespace InputBindingLibrary
