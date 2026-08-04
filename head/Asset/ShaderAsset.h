#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

/// Source-independent ownership wrapper for SPIR-V shader bytecode.
class ShaderAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        ShaderStage stage = ShaderStage::Vertex;
        std::string entryPoint = "main";
        std::vector<uint32_t> spirv;
    };

    ShaderAsset() = default;
    explicit ShaderAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] ShaderStage stage() const noexcept { return stage_; }
    [[nodiscard]] const std::string& entryPoint() const noexcept { return entryPoint_; }
    [[nodiscard]] const std::vector<uint32_t>& spirv() const noexcept { return spirv_; }
    [[nodiscard]] explicit operator bool() const noexcept { return !spirv_.empty(); }

private:
    std::string name_;
    ShaderStage stage_ = ShaderStage::Vertex;
    std::string entryPoint_ = "main";
    std::vector<uint32_t> spirv_;
};

} // namespace VkRenderer
