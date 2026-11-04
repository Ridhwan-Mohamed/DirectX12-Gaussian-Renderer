#include "GpuGaussian.h"

std::vector<GpuGaussian> PackGaussianScene(
    const GaussianScene& scene)
{
    std::vector<GpuGaussian> packedGaussians;
    packedGaussians.reserve(scene.gaussians.size());

    for (const Gaussian& source : scene.gaussians)
    {
        GpuGaussian destination{};

        destination.position = source.position;
        destination.opacity = source.opacity;

        destination.scale = source.scale;

        destination.rotationWxyz = {
            source.rotation.w,
            source.rotation.x,
            source.rotation.y,
            source.rotation.z
        };

        destination.sphericalHarmonics =
            source.sphericalHarmonics;

        packedGaussians.push_back(destination);
    }

    return packedGaussians;
}
