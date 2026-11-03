#pragma once

#include "GaussianScene.h"

#include <filesystem>

class PlyGaussianLoader final
{
public:
    [[nodiscard]] static GaussianScene Load(
        const std::filesystem::path& path);
};
