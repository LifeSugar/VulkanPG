#pragma once

#include "asset/AssetFwd.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace VkRenderer
{

inline constexpr uint32_t kInvalidShaderBinding =
    std::numeric_limits<uint32_t>::max();

struct MaterialDescriptorBinding
{
    uint32_t set = 1;
    uint32_t binding = kInvalidShaderBinding;
};

struct MaterialParameterBlockDesc
{
    MaterialDescriptorBinding descriptor{1, 0};
};

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
    MaterialDescriptorBinding imageBinding;
    MaterialDescriptorBinding samplerBinding;
};

/// Defines the parameter and texture interface accepted by one material kind.
class MaterialTemplateAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        std::vector<ShaderAssetHandle> shaders;
        MaterialParameterBlockDesc parameterBlock;
        uint32_t parameterDataSize = 0;
        std::vector<MaterialParameterDesc> parameters;
        std::vector<MaterialTextureSlotDesc> textureSlots;
        /// Filled by AssetManager after validating the referenced shaders.
        uint64_t shaderInterfaceSignature = 0;
    };

    MaterialTemplateAsset() = default;
    explicit MaterialTemplateAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<ShaderAssetHandle>& shaders() const noexcept { return shaders_; }
    [[nodiscard]] const MaterialParameterBlockDesc& parameterBlock() const noexcept
    {
        return parameterBlock_;
    }
    [[nodiscard]] uint32_t parameterDataSize() const noexcept { return parameterDataSize_; }
    [[nodiscard]] const std::vector<MaterialParameterDesc>& parameters() const noexcept { return parameters_; }
    [[nodiscard]] const std::vector<MaterialTextureSlotDesc>& textureSlots() const noexcept { return textureSlots_; }
    [[nodiscard]] uint64_t shaderInterfaceSignature() const noexcept
    {
        return shaderInterfaceSignature_;
    }

    [[nodiscard]] static uint32_t valueSize(MaterialValueType type) noexcept;

private:
    std::string name_;
    std::vector<ShaderAssetHandle> shaders_;
    MaterialParameterBlockDesc parameterBlock_;
    uint32_t parameterDataSize_ = 0;
    std::vector<MaterialParameterDesc> parameters_;
    std::vector<MaterialTextureSlotDesc> textureSlots_;
    uint64_t shaderInterfaceSignature_ = 0;
};

} // namespace VkRenderer
