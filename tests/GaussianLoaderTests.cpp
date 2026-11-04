#include "GaussianScene.h"
#include "GpuGaussian.h"
#include "PlyGaussianLoader.h"

#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    void Expect(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void ExpectNear(
        float actual,
        float expected,
        float tolerance,
        const std::string& label)
    {
        if (std::abs(actual - expected) > tolerance)
        {
            throw std::runtime_error(
                label + " expected " + std::to_string(expected) +
                ", received " + std::to_string(actual));
        }
    }

    void ExpectLoadFailure(
        const std::filesystem::path& path,
        const std::string& expectedMessageFragment)
    {
        try
        {
            static_cast<void>(PlyGaussianLoader::Load(path));
        }
        catch (const std::exception& exception)
        {
            const std::string message = exception.what();
            Expect(
                message.find(expectedMessageFragment) != std::string::npos,
                "loader failure did not contain: " +
                    expectedMessageFragment + "\nactual: " + message);
            return;
        }

        throw std::runtime_error(
            "loader unexpectedly accepted invalid file: " + path.string());
    }
}

int main(int argumentCount, char** arguments)
{
    try
    {
        if (argumentCount != 2)
        {
            throw std::runtime_error(
                "Usage: GaussianLoaderTests <reference.ply>");
        }

        const GaussianScene scene = PlyGaussianLoader::Load(
            std::filesystem::path(arguments[1]));
        Expect(scene.gaussians.size() == 17'020, "Gaussian count mismatch");

        const GaussianSceneStatistics& statistics = scene.statistics;
        ExpectNear(
            statistics.sourceBounds.minimum.x,
            -0.494141757f,
            1.0e-6f,
            "minimum X");
        ExpectNear(
            statistics.sourceBounds.minimum.y,
            -0.190222681f,
            1.0e-6f,
            "minimum Y");
        ExpectNear(
            statistics.sourceBounds.minimum.z,
            -0.116828270f,
            1.0e-6f,
            "minimum Z");
        ExpectNear(
            statistics.sourceBounds.maximum.x,
            0.493660569f,
            1.0e-6f,
            "maximum X");
        ExpectNear(
            statistics.sourceBounds.maximum.y,
            0.192169592f,
            1.0e-6f,
            "maximum Y");
        ExpectNear(
            statistics.sourceBounds.maximum.z,
            0.096737459f,
            1.0e-6f,
            "maximum Z");

        const Gaussian& first = scene.gaussians.front();
        ExpectNear(first.position.x, -0.011715226f, 1.0e-6f, "first position X");
        ExpectNear(first.position.y, 0.007974744f, 1.0e-6f, "first position Y");
        ExpectNear(first.position.z, 0.043900065f, 1.0e-6f, "first position Z");
        ExpectNear(first.scale.x, 0.02893762f, 1.0e-6f, "first scale X");
        ExpectNear(first.scale.y, 0.02012619f, 1.0e-6f, "first scale Y");
        ExpectNear(first.scale.z, 0.02751104f, 1.0e-6f, "first scale Z");
        ExpectNear(first.opacity, 0.00388111f, 1.0e-6f, "first opacity");

        const float normalizedQuaternionLength = std::sqrt(
            first.rotation.w * first.rotation.w +
            first.rotation.x * first.rotation.x +
            first.rotation.y * first.rotation.y +
            first.rotation.z * first.rotation.z);
        ExpectNear(
            normalizedQuaternionLength,
            1.0f,
            1.0e-5f,
            "normalized quaternion length");

        Expect(
            statistics.minimumOpacity >= 0.0f &&
                statistics.maximumOpacity <= 1.0f,
            "decoded opacity is outside [0, 1]");
        Expect(
            statistics.minimumScale.x > 0.0f &&
                statistics.minimumScale.y > 0.0f &&
                statistics.minimumScale.z > 0.0f,
            "decoded scale is not positive");

        const std::vector<GpuGaussian> packedGaussians =
            PackGaussianScene(scene);
        Expect(
            packedGaussians.size() == scene.gaussians.size(),
            "packed Gaussian count mismatch");
        Expect(
            packedGaussians.size() * sizeof(GpuGaussian) == 4'084'800,
            "packed Gaussian byte count mismatch");

        const GpuGaussian& firstPacked = packedGaussians.front();
        ExpectNear(
            firstPacked.position.x,
            first.position.x,
            0.0f,
            "packed position X");
        ExpectNear(
            firstPacked.opacity,
            first.opacity,
            0.0f,
            "packed opacity");
        ExpectNear(
            firstPacked.scale.z,
            first.scale.z,
            0.0f,
            "packed scale Z");
        ExpectNear(
            firstPacked.rotationWxyz.x,
            first.rotation.w,
            0.0f,
            "packed quaternion W");
        ExpectNear(
            firstPacked.rotationWxyz.w,
            first.rotation.z,
            0.0f,
            "packed quaternion Z");
        ExpectNear(
            firstPacked.sphericalHarmonics[0].x,
            first.sphericalHarmonics[0].x,
            0.0f,
            "packed DC red coefficient");
        ExpectNear(
            firstPacked.sphericalHarmonics[15].z,
            first.sphericalHarmonics[15].z,
            0.0f,
            "packed degree-three blue coefficient");

        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        const std::filesystem::path temporaryDirectory =
            std::filesystem::temp_directory_path();
        const std::filesystem::path asciiPath = temporaryDirectory /
            ("gaussian-loader-ascii-" + std::to_string(uniqueSuffix) + ".ply");
        const std::filesystem::path missingPropertyPath = temporaryDirectory /
            ("gaussian-loader-missing-" + std::to_string(uniqueSuffix) + ".ply");

        {
            std::ofstream file(asciiPath, std::ios::binary);
            file << "ply\n"
                 << "format ascii 1.0\n"
                 << "element vertex 1\n"
                 << "property float x\n"
                 << "end_header\n"
                 << "0\n";
        }
        {
            std::ofstream file(missingPropertyPath, std::ios::binary);
            file << "ply\n"
                 << "format binary_little_endian 1.0\n"
                 << "element vertex 1\n"
                 << "property float x\n"
                 << "property float y\n"
                 << "property float z\n"
                 << "end_header\n";
        }

        ExpectLoadFailure(
            asciiPath,
            "expected 'format binary_little_endian 1.0'");
        ExpectLoadFailure(
            missingPropertyPath,
            "missing required vertex property 'opacity'");
        std::filesystem::remove(asciiPath);
        std::filesystem::remove(missingPropertyPath);

        std::cout << BuildGaussianSceneReport(scene, 1);
        std::cout << "Gaussian loader fixture checks passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Gaussian loader test failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
