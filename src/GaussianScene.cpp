#include "GaussianScene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

DirectX::XMFLOAT3 Bounds3::Center() const noexcept
{
    return {
        (minimum.x + maximum.x) * 0.5f,
        (minimum.y + maximum.y) * 0.5f,
        (minimum.z + maximum.z) * 0.5f};
}

DirectX::XMFLOAT3 Bounds3::Extent() const noexcept
{
    return {
        maximum.x - minimum.x,
        maximum.y - minimum.y,
        maximum.z - minimum.z};
}

std::string BuildGaussianSceneReport(
    const GaussianScene& scene,
    std::size_t sampleCount)
{
    const GaussianSceneStatistics& statistics = scene.statistics;
    const DirectX::XMFLOAT3 sourceExtent =
        statistics.sourceBounds.Extent();

    std::ostringstream report;
    report << std::fixed << std::setprecision(9);
    report << "Gaussian scene loaded\n";
    report << "  path: " << scene.sourcePath.string() << '\n';
    report << "  count: " << scene.gaussians.size() << '\n';
    report << "  source bounds min: ("
           << statistics.sourceBounds.minimum.x << ", "
           << statistics.sourceBounds.minimum.y << ", "
           << statistics.sourceBounds.minimum.z << ")\n";
    report << "  source bounds max: ("
           << statistics.sourceBounds.maximum.x << ", "
           << statistics.sourceBounds.maximum.y << ", "
           << statistics.sourceBounds.maximum.z << ")\n";
    report << "  source extent: ("
           << sourceExtent.x << ", "
           << sourceExtent.y << ", "
           << sourceExtent.z << ")\n";
    report << "  recenter offset: ("
           << statistics.sourceCenter.x << ", "
           << statistics.sourceCenter.y << ", "
           << statistics.sourceCenter.z << ")\n";
    report << "  decoded opacity range: ["
           << statistics.minimumOpacity << ", "
           << statistics.maximumOpacity << "]\n";
    report << "  raw quaternion norm range: ["
           << statistics.minimumRawQuaternionNorm << ", "
           << statistics.maximumRawQuaternionNorm << "]\n";

    sampleCount = std::min(sampleCount, scene.gaussians.size());
    for (std::size_t index = 0; index < sampleCount; ++index)
    {
        const Gaussian& gaussian = scene.gaussians[index];
        const DirectX::XMFLOAT3& dc = gaussian.sphericalHarmonics[0];
        report << "  sample[" << index << "]"
               << " position=(" << gaussian.position.x << ", "
               << gaussian.position.y << ", "
               << gaussian.position.z << ")"
               << " scale=(" << gaussian.scale.x << ", "
               << gaussian.scale.y << ", "
               << gaussian.scale.z << ")"
               << " opacity=" << gaussian.opacity
               << " rotation_wxyz=(" << gaussian.rotation.w << ", "
               << gaussian.rotation.x << ", "
               << gaussian.rotation.y << ", "
               << gaussian.rotation.z << ")"
               << " sh_dc=(" << dc.x << ", "
               << dc.y << ", "
               << dc.z << ")\n";
    }

    return report.str();
}
