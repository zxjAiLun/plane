#pragma once

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
