#pragma once

#include "asset/ShaderInterface.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace rubia::asset
{

/// Source-independent ownership wrapper for SPIR-V shader bytecode.
class ShaderAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        std::vector<uint32_t> spirv;
        ShaderInterface interface;
    };

    ShaderAsset() = default;
    explicit ShaderAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] ShaderStage stage() const noexcept { return interface_.stage; }
    [[nodiscard]] const std::string& entryPoint() const noexcept
    {
        return interface_.entryPoint;
    }
    [[nodiscard]] const std::vector<uint32_t>& spirv() const noexcept { return spirv_; }
    [[nodiscard]] const ShaderInterface& interface() const noexcept
    {
        return interface_;
    }
    [[nodiscard]] explicit operator bool() const noexcept { return !spirv_.empty(); }

private:
    std::string name_;
    std::vector<uint32_t> spirv_;
    ShaderInterface interface_;
};

} // namespace rubia::asset
