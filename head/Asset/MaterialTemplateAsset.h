#pragma once

#include "Asset/AssetFwd.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

enum class MaterialValueType
{
    Float,
    Float2,
    Float3,
    Float4,
    Matrix4,
    Int,
    UInt,
    Bool
};

struct MaterialParameterDesc
{
    std::string name;
    MaterialValueType type = MaterialValueType::Float;
    uint32_t byteOffset = 0;
    bool required = false;
};

struct MaterialTextureSlotDesc
{
    std::string name;
    uint32_t slot = 0;
    bool required = false;
};

/// Defines the parameter and texture interface accepted by one material kind.
class MaterialTemplateAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        std::vector<ShaderAssetHandle> shaders;
        uint32_t parameterDataSize = 0;
        std::vector<MaterialParameterDesc> parameters;
        std::vector<MaterialTextureSlotDesc> textureSlots;
    };

    MaterialTemplateAsset() = default;
    explicit MaterialTemplateAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<ShaderAssetHandle>& shaders() const noexcept { return shaders_; }
    [[nodiscard]] uint32_t parameterDataSize() const noexcept { return parameterDataSize_; }
    [[nodiscard]] const std::vector<MaterialParameterDesc>& parameters() const noexcept { return parameters_; }
    [[nodiscard]] const std::vector<MaterialTextureSlotDesc>& textureSlots() const noexcept { return textureSlots_; }

    [[nodiscard]] static uint32_t valueSize(MaterialValueType type) noexcept;

private:
    std::string name_;
    std::vector<ShaderAssetHandle> shaders_;
    uint32_t parameterDataSize_ = 0;
    std::vector<MaterialParameterDesc> parameters_;
    std::vector<MaterialTextureSlotDesc> textureSlots_;
};

} // namespace VkRenderer
