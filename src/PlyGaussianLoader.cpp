#include "PlyGaussianLoader.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    enum class PlyScalarType
    {
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Float32,
        Float64
    };

    struct PlyProperty
    {
        std::string name;
        PlyScalarType type = PlyScalarType::Float32;
        std::size_t offset = 0;
        bool isList = false;
    };

    struct PlyElement
    {
        std::string name;
        std::size_t count = 0;
        std::vector<PlyProperty> properties;
    };

    struct PlyHeader
    {
        std::vector<PlyElement> elements;
    };

    struct BoundsAccumulator
    {
        DirectX::XMFLOAT3 minimum{
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity()};
        DirectX::XMFLOAT3 maximum{
            -std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()};

        void Include(const DirectX::XMFLOAT3& point) noexcept
        {
            minimum.x = std::min(minimum.x, point.x);
            minimum.y = std::min(minimum.y, point.y);
            minimum.z = std::min(minimum.z, point.z);
            maximum.x = std::max(maximum.x, point.x);
            maximum.y = std::max(maximum.y, point.y);
            maximum.z = std::max(maximum.z, point.z);
        }

        [[nodiscard]] Bounds3 Finish() const noexcept
        {
            return {minimum, maximum};
        }
    };

    [[noreturn]] void Fail(
        const std::filesystem::path& path,
        std::string_view message)
    {
        throw std::runtime_error(
            "PLY '" + path.string() + "': " + std::string(message));
    }

    std::string RemoveTrailingCarriageReturn(std::string line)
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        return line;
    }

    PlyScalarType ParseScalarType(
        const std::filesystem::path& path,
        std::string_view name)
    {
        if (name == "char" || name == "int8")
        {
            return PlyScalarType::Int8;
        }
        if (name == "uchar" || name == "uint8")
        {
            return PlyScalarType::UInt8;
        }
        if (name == "short" || name == "int16")
        {
            return PlyScalarType::Int16;
        }
        if (name == "ushort" || name == "uint16")
        {
            return PlyScalarType::UInt16;
        }
        if (name == "int" || name == "int32")
        {
            return PlyScalarType::Int32;
        }
        if (name == "uint" || name == "uint32")
        {
            return PlyScalarType::UInt32;
        }
        if (name == "float" || name == "float32")
        {
            return PlyScalarType::Float32;
        }
        if (name == "double" || name == "float64")
        {
            return PlyScalarType::Float64;
        }

        Fail(path, "unsupported scalar type '" + std::string(name) + "'");
    }

    std::size_t ScalarSize(PlyScalarType type) noexcept
    {
        switch (type)
        {
        case PlyScalarType::Int8:
        case PlyScalarType::UInt8:
            return 1;
        case PlyScalarType::Int16:
        case PlyScalarType::UInt16:
            return 2;
        case PlyScalarType::Int32:
        case PlyScalarType::UInt32:
        case PlyScalarType::Float32:
            return 4;
        case PlyScalarType::Float64:
            return 8;
        }
        return 0;
    }

    PlyHeader ReadHeader(
        std::ifstream& stream,
        const std::filesystem::path& path)
    {
        std::string line;
        if (!std::getline(stream, line) ||
            RemoveTrailingCarriageReturn(std::move(line)) != "ply")
        {
            Fail(path, "missing 'ply' magic header");
        }

        PlyHeader header;
        PlyElement* currentElement = nullptr;
        bool formatSeen = false;
        bool endSeen = false;
        std::size_t lineNumber = 1;

        while (std::getline(stream, line))
        {
            ++lineNumber;
            line = RemoveTrailingCarriageReturn(std::move(line));
            if (line.size() > 64 * 1024)
            {
                Fail(path, "header line is unreasonably long");
            }

            std::istringstream tokens(line);
            std::string keyword;
            tokens >> keyword;
            if (keyword.empty() || keyword == "comment" || keyword == "obj_info")
            {
                continue;
            }

            if (keyword == "format")
            {
                std::string encoding;
                std::string version;
                tokens >> encoding >> version;
                if (encoding != "binary_little_endian" || version != "1.0")
                {
                    Fail(
                        path,
                        "expected 'format binary_little_endian 1.0'");
                }
                formatSeen = true;
                continue;
            }

            if (keyword == "element")
            {
                std::string name;
                std::size_t count = 0;
                if (!(tokens >> name >> count) || name.empty())
                {
                    Fail(path, "malformed element declaration on line " +
                        std::to_string(lineNumber));
                }
                header.elements.push_back({name, count, {}});
                currentElement = &header.elements.back();
                continue;
            }

            if (keyword == "property")
            {
                if (currentElement == nullptr)
                {
                    Fail(path, "property declared before an element");
                }

                std::string typeName;
                tokens >> typeName;
                if (typeName == "list")
                {
                    std::string countType;
                    std::string valueType;
                    std::string propertyName;
                    if (!(tokens >> countType >> valueType >> propertyName))
                    {
                        Fail(path, "malformed list property declaration");
                    }
                    currentElement->properties.push_back(
                        {propertyName, PlyScalarType::UInt8, 0, true});
                    continue;
                }

                std::string propertyName;
                if (!(tokens >> propertyName) || propertyName.empty())
                {
                    Fail(path, "malformed scalar property declaration");
                }
                currentElement->properties.push_back(
                    {propertyName, ParseScalarType(path, typeName), 0, false});
                continue;
            }

            if (keyword == "end_header")
            {
                endSeen = true;
                break;
            }

            Fail(path, "unsupported header directive '" + keyword + "'");
        }

        if (!formatSeen)
        {
            Fail(path, "missing format declaration");
        }
        if (!endSeen)
        {
            Fail(path, "missing end_header");
        }
        return header;
    }

    template<typename T>
    T ReadValue(const std::byte* data) noexcept
    {
        T value{};
        std::memcpy(&value, data, sizeof(T));
        return value;
    }

    double ReadScalar(
        const std::vector<std::byte>& record,
        const PlyProperty& property) noexcept
    {
        const std::byte* data = record.data() + property.offset;
        switch (property.type)
        {
        case PlyScalarType::Int8:
            return ReadValue<std::int8_t>(data);
        case PlyScalarType::UInt8:
            return ReadValue<std::uint8_t>(data);
        case PlyScalarType::Int16:
            return ReadValue<std::int16_t>(data);
        case PlyScalarType::UInt16:
            return ReadValue<std::uint16_t>(data);
        case PlyScalarType::Int32:
            return ReadValue<std::int32_t>(data);
        case PlyScalarType::UInt32:
            return ReadValue<std::uint32_t>(data);
        case PlyScalarType::Float32:
            return ReadValue<float>(data);
        case PlyScalarType::Float64:
            return ReadValue<double>(data);
        }
        return 0.0;
    }

    float ReadFiniteFloat(
        const std::vector<std::byte>& record,
        const PlyProperty& property,
        std::size_t recordIndex,
        const std::filesystem::path& path)
    {
        const double value = ReadScalar(record, property);
        if (!std::isfinite(value) ||
            value < -std::numeric_limits<float>::max() ||
            value > std::numeric_limits<float>::max())
        {
            Fail(
                path,
                "non-finite or out-of-range value for property '" +
                    property.name + "' in record " +
                    std::to_string(recordIndex));
        }
        return static_cast<float>(value);
    }

    const PlyProperty& RequireProperty(
        const std::unordered_map<std::string, const PlyProperty*>& properties,
        const std::filesystem::path& path,
        std::string_view name)
    {
        const auto iterator = properties.find(std::string(name));
        if (iterator == properties.end())
        {
            Fail(path, "missing required vertex property '" +
                std::string(name) + "'");
        }
        return *iterator->second;
    }

    float StableSigmoid(float value) noexcept
    {
        if (value >= 0.0f)
        {
            return 1.0f / (1.0f + std::exp(-value));
        }
        const float exponential = std::exp(value);
        return exponential / (1.0f + exponential);
    }
}

GaussianScene PlyGaussianLoader::Load(
    const std::filesystem::path& path)
{
    if constexpr (std::endian::native != std::endian::little)
    {
        Fail(path, "this loader currently requires a little-endian CPU");
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        Fail(path, "could not open file");
    }

    PlyHeader header = ReadHeader(stream, path);
    if (header.elements.empty() || header.elements.front().name != "vertex")
    {
        Fail(path, "the vertex element must be the first binary element");
    }

    PlyElement& vertexElement = header.elements.front();
    if (vertexElement.count == 0)
    {
        Fail(path, "vertex element contains no Gaussians");
    }
    constexpr std::size_t MaximumReasonableGaussianCount = 100'000'000;
    if (vertexElement.count > MaximumReasonableGaussianCount)
    {
        Fail(path, "vertex count exceeds the loader safety limit");
    }

    std::unordered_map<std::string, const PlyProperty*> propertiesByName;
    std::size_t recordSize = 0;
    for (PlyProperty& property : vertexElement.properties)
    {
        if (property.isList)
        {
            Fail(path, "list properties are not supported on the vertex element");
        }

        property.offset = recordSize;
        recordSize += ScalarSize(property.type);
        const auto [iterator, inserted] = propertiesByName.emplace(
            property.name,
            &property);
        if (!inserted)
        {
            Fail(path, "duplicate vertex property '" + property.name + "'");
        }
    }

    if (recordSize == 0)
    {
        Fail(path, "vertex records have zero size");
    }

    const PlyProperty& positionX =
        RequireProperty(propertiesByName, path, "x");
    const PlyProperty& positionY =
        RequireProperty(propertiesByName, path, "y");
    const PlyProperty& positionZ =
        RequireProperty(propertiesByName, path, "z");
    const PlyProperty& opacityProperty =
        RequireProperty(propertiesByName, path, "opacity");

    std::array<const PlyProperty*, 3> dcProperties{};
    std::array<const PlyProperty*, 45> restProperties{};
    std::array<const PlyProperty*, 3> scaleProperties{};
    std::array<const PlyProperty*, 4> rotationProperties{};

    for (std::size_t index = 0; index < dcProperties.size(); ++index)
    {
        dcProperties[index] = &RequireProperty(
            propertiesByName,
            path,
            "f_dc_" + std::to_string(index));
    }
    for (std::size_t index = 0; index < restProperties.size(); ++index)
    {
        restProperties[index] = &RequireProperty(
            propertiesByName,
            path,
            "f_rest_" + std::to_string(index));
    }
    for (std::size_t index = 0; index < scaleProperties.size(); ++index)
    {
        scaleProperties[index] = &RequireProperty(
            propertiesByName,
            path,
            "scale_" + std::to_string(index));
    }
    for (std::size_t index = 0; index < rotationProperties.size(); ++index)
    {
        rotationProperties[index] = &RequireProperty(
            propertiesByName,
            path,
            "rot_" + std::to_string(index));
    }

    GaussianScene scene;
    scene.sourcePath = path;
    scene.gaussians.reserve(vertexElement.count);

    BoundsAccumulator sourceBounds;
    DirectX::XMFLOAT3 minimumScale{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()};
    DirectX::XMFLOAT3 maximumScale{
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};
    float minimumOpacity = std::numeric_limits<float>::infinity();
    float maximumOpacity = -std::numeric_limits<float>::infinity();
    float minimumQuaternionNorm = std::numeric_limits<float>::infinity();
    float maximumQuaternionNorm = -std::numeric_limits<float>::infinity();

    std::vector<std::byte> record(recordSize);
    for (std::size_t recordIndex = 0;
         recordIndex < vertexElement.count;
         ++recordIndex)
    {
        stream.read(
            reinterpret_cast<char*>(record.data()),
            static_cast<std::streamsize>(record.size()));
        if (stream.gcount() != static_cast<std::streamsize>(record.size()))
        {
            Fail(
                path,
                "unexpected end of file while reading record " +
                    std::to_string(recordIndex));
        }

        Gaussian gaussian;
        gaussian.position = {
            ReadFiniteFloat(record, positionX, recordIndex, path),
            ReadFiniteFloat(record, positionY, recordIndex, path),
            ReadFiniteFloat(record, positionZ, recordIndex, path)};
        sourceBounds.Include(gaussian.position);

        const float opacityLogit = ReadFiniteFloat(
            record,
            opacityProperty,
            recordIndex,
            path);
        gaussian.opacity = StableSigmoid(opacityLogit);
        minimumOpacity = std::min(minimumOpacity, gaussian.opacity);
        maximumOpacity = std::max(maximumOpacity, gaussian.opacity);

        std::array<float, 3> decodedScale{};
        for (std::size_t axis = 0; axis < decodedScale.size(); ++axis)
        {
            const float logScale = ReadFiniteFloat(
                record,
                *scaleProperties[axis],
                recordIndex,
                path);
            decodedScale[axis] = std::exp(logScale);
            if (!std::isfinite(decodedScale[axis]) || decodedScale[axis] <= 0.0f)
            {
                Fail(
                    path,
                    "scale decode failed in record " +
                        std::to_string(recordIndex));
            }
        }
        gaussian.scale = {
            decodedScale[0],
            decodedScale[1],
            decodedScale[2]};
        minimumScale.x = std::min(minimumScale.x, gaussian.scale.x);
        minimumScale.y = std::min(minimumScale.y, gaussian.scale.y);
        minimumScale.z = std::min(minimumScale.z, gaussian.scale.z);
        maximumScale.x = std::max(maximumScale.x, gaussian.scale.x);
        maximumScale.y = std::max(maximumScale.y, gaussian.scale.y);
        maximumScale.z = std::max(maximumScale.z, gaussian.scale.z);

        std::array<float, 4> rawRotation{};
        for (std::size_t component = 0;
             component < rawRotation.size();
             ++component)
        {
            rawRotation[component] = ReadFiniteFloat(
                record,
                *rotationProperties[component],
                recordIndex,
                path);
        }

        const float quaternionNorm = std::sqrt(
            rawRotation[0] * rawRotation[0] +
            rawRotation[1] * rawRotation[1] +
            rawRotation[2] * rawRotation[2] +
            rawRotation[3] * rawRotation[3]);
        if (!std::isfinite(quaternionNorm) || quaternionNorm < 1.0e-8f)
        {
            Fail(
                path,
                "zero or invalid quaternion in record " +
                    std::to_string(recordIndex));
        }
        minimumQuaternionNorm = std::min(
            minimumQuaternionNorm,
            quaternionNorm);
        maximumQuaternionNorm = std::max(
            maximumQuaternionNorm,
            quaternionNorm);
        gaussian.rotation = {
            rawRotation[0] / quaternionNorm,
            rawRotation[1] / quaternionNorm,
            rawRotation[2] / quaternionNorm,
            rawRotation[3] / quaternionNorm};

        std::array<float, 3> dc{};
        for (std::size_t channel = 0; channel < dc.size(); ++channel)
        {
            dc[channel] = ReadFiniteFloat(
                record,
                *dcProperties[channel],
                recordIndex,
                path);
        }
        gaussian.sphericalHarmonics[0] = {dc[0], dc[1], dc[2]};

        std::array<float, 45> rest{};
        for (std::size_t index = 0; index < rest.size(); ++index)
        {
            rest[index] = ReadFiniteFloat(
                record,
                *restProperties[index],
                recordIndex,
                path);
        }
        for (std::size_t basis = 0; basis < 15; ++basis)
        {
            gaussian.sphericalHarmonics[basis + 1] = {
                rest[basis],
                rest[15 + basis],
                rest[30 + basis]};
        }

        scene.gaussians.push_back(gaussian);
    }

    scene.statistics.sourceBounds = sourceBounds.Finish();
    scene.statistics.sourceCenter =
        scene.statistics.sourceBounds.Center();
    scene.statistics.minimumScale = minimumScale;
    scene.statistics.maximumScale = maximumScale;
    scene.statistics.minimumOpacity = minimumOpacity;
    scene.statistics.maximumOpacity = maximumOpacity;
    scene.statistics.minimumRawQuaternionNorm = minimumQuaternionNorm;
    scene.statistics.maximumRawQuaternionNorm = maximumQuaternionNorm;

    const DirectX::XMFLOAT3 center = scene.statistics.sourceCenter;
    BoundsAccumulator centeredBounds;
    for (Gaussian& gaussian : scene.gaussians)
    {
        gaussian.position.x -= center.x;
        gaussian.position.y -= center.y;
        gaussian.position.z -= center.z;
        centeredBounds.Include(gaussian.position);
    }
    scene.statistics.centeredBounds = centeredBounds.Finish();

    return scene;
}
