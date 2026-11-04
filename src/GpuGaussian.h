#pragma once

#include "GaussianScene.h"

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

struct alignas(16) GpuGaussian
{
    DirectX::XMFLOAT3 position{};
    float opacity = 0.0f;

    DirectX::XMFLOAT3 scale{};
    float padding = 0.0f;

    // Stored explicitly as W, X, Y, Z.
    DirectX::XMFLOAT4 rotationWxyz{};

    std::array<
        DirectX::XMFLOAT3,
        Gaussian::SphericalHarmonicCoefficientCount>
        sphericalHarmonics{};
};

static_assert(std::is_trivially_copyable_v<GpuGaussian>);
static_assert(sizeof(GpuGaussian) == 240);

static_assert(offsetof(GpuGaussian, position) == 0);
static_assert(offsetof(GpuGaussian, opacity) == 12);
static_assert(offsetof(GpuGaussian, scale) == 16);
static_assert(offsetof(GpuGaussian, padding) == 28);
static_assert(offsetof(GpuGaussian, rotationWxyz) == 32);
static_assert(offsetof(GpuGaussian, sphericalHarmonics) == 48);

[[nodiscard]] std::vector<GpuGaussian> PackGaussianScene(
    const GaussianScene& scene);
