#pragma once

#include "Asset/AssetFwd.h"
#include "Asset/MaterialState.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace VkRenderer
{

using MaterialValue = std::variant<
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat4,
    int32_t,
    uint32_t,
    bool>;

struct MaterialParameterAssignment
{
    std::string name;
    MaterialValue value;
};

struct MaterialTextureAssignment
{
    std::string name;
    TextureAssetHandle texture;
};

/// Compiled instance of a MaterialTemplateAsset.
class MaterialAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        MaterialTemplateAssetHandle materialTemplate;
        MaterialRenderState renderState;
        std::vector<MaterialParameterAssignment> parameters;
        std::vector<MaterialTextureAssignment> textures;
    };

    MaterialAsset() = default;
    MaterialAsset(const MaterialAsset&) = delete;
    MaterialAsset& operator=(const MaterialAsset&) = delete;
    MaterialAsset(MaterialAsset&&) noexcept = default;
    MaterialAsset& operator=(MaterialAsset&&) noexcept = default;

    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] MaterialTemplateAssetHandle materialTemplate() const noexcept { return materialTemplate_; }
    [[nodiscard]] const MaterialRenderState& renderState() const noexcept { return renderState_; }
    [[nodiscard]] const std::vector<std::byte>& parameterData() const noexcept { return parameterData_; }
    [[nodiscard]] const std::vector<TextureAssetHandle>& textures() const noexcept { return textures_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return static_cast<bool>(materialTemplate_);
    }

private:
    friend class AssetManager;

    struct CompiledCreateInfo
    {
        std::string name;
        MaterialTemplateAssetHandle materialTemplate;
        MaterialRenderState renderState;
        std::vector<std::byte> parameterData;
        std::vector<TextureAssetHandle> textures;
    };

    explicit MaterialAsset(CompiledCreateInfo createInfo);

    std::string name_;
    MaterialTemplateAssetHandle materialTemplate_;
    MaterialRenderState renderState_;
    std::vector<std::byte> parameterData_;
    std::vector<TextureAssetHandle> textures_;
};

} // namespace VkRenderer
