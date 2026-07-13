#pragma once

#include <filesystem>
#include <string>

#include "SaveData.hpp"

class SaveService {
public:
    static bool save(
        const std::filesystem::path& path,
        const SaveData& data,
        std::string* error = nullptr
    );

    static bool load(
        const std::filesystem::path& path,
        SaveData& data,
        std::string* error = nullptr
    );
};
