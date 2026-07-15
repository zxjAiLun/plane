#pragma once

#include "Config.hpp"
#include "Item.hpp"

enum class CraftingOperation {
    None,
    ImproveAffix,
    RerollAffix,
    RaiseAffixTier
};

enum class CraftingResult {
    Success,
    InvalidTarget,
    NoCandidates,
    AlreadyMaxTier,
    NoImprovement
};

struct CraftingState {
    bool open = false;
    CraftingOperation operation = CraftingOperation::None;
    int affixIndex = -1;
};

inline const char* craftingOperationName(CraftingOperation operation) {
    switch (operation) {
        case CraftingOperation::None: return "Choose an operation";
        case CraftingOperation::ImproveAffix: return "Improve affix";
        case CraftingOperation::RerollAffix: return "Reroll affix";
        case CraftingOperation::RaiseAffixTier: return "Raise affix tier";
    }
    return "Choose an operation";
}

inline int craftingCostFor(CraftingOperation operation) {
    switch (operation) {
        case CraftingOperation::ImproveAffix: return Config::ForgeImproveCost;
        case CraftingOperation::RerollAffix: return Config::ForgeRerollCost;
        case CraftingOperation::RaiseAffixTier: return Config::ForgeRaiseTierCost;
        case CraftingOperation::None: break;
    }
    return 0;
}

inline bool craftingOperationAllowed(
    CraftingOperation operation,
    const Item& item
) {
    if (item.rarity == Rarity::Unique || item.affixes.empty()) {
        return false;
    }

    switch (operation) {
        case CraftingOperation::ImproveAffix:
            return true;
        case CraftingOperation::RerollAffix:
        case CraftingOperation::RaiseAffixTier:
            return item.rarity == Rarity::Magic || item.rarity == Rarity::Rare;
        case CraftingOperation::None:
            break;
    }
    return false;
}
