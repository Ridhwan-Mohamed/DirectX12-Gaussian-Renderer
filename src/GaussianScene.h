#pragma once

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct Bounds3
{
    DirectX::XMFLOAT3 minimum{};
    DirectX::XMFLOAT3 maximum{};

    [[nodiscard]] DirectX::XMFLOAT3 Center() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 Extent() const noexcept;
};

// The source PLY stores quaternion components as rot_0..rot_3 in W, X, Y, Z
// order. Naming the fields avoids silently treating them as DirectX's common
// X, Y, Z, W layout.
struct QuaternionWxyz
{
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Gaussian
{
    static constexpr std::size_t SphericalHarmonicCoefficientCount = 16;

    // Position is recentered around the source bounds centre during loading.
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 scale{1.0f, 1.0f, 1.0f};
    QuaternionWxyz rotation{};
    float opacity = 1.0f;

    // Basis-major RGB coefficients. Index 0 is the DC term. Indices 1..15
    // are reconstructed from the PLY's channel-major f_rest layout.
    std::array<
        DirectX::XMFLOAT3,
        SphericalHarmonicCoefficientCount>
        sphericalHarmonics{};
};

struct GaussianSceneStatistics
{
    Bounds3 sourceBounds{};
    Bounds3 centeredBounds{};
    DirectX::XMFLOAT3 sourceCenter{};
    DirectX::XMFLOAT3 minimumScale{};
    DirectX::XMFLOAT3 maximumScale{};
    float minimumOpacity = 0.0f;
    float maximumOpacity = 0.0f;
    float minimumRawQuaternionNorm = 0.0f;
    float maximumRawQuaternionNorm = 0.0f;
};

struct GaussianScene
{
    std::filesystem::path sourcePath;
    std::vector<Gaussian> gaussians;
    GaussianSceneStatistics statistics{};
};

[[nodiscard]] std::string BuildGaussianSceneReport(
    const GaussianScene& scene,
    std::size_t sampleCount = 3);
