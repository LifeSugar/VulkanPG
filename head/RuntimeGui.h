#pragma once

#include "ApplicationGui.h"

namespace VkRenderer
{

/// Minimal GUI used by the non-editor runtime.
class RuntimeGui final : public ApplicationGui
{
public:
    [[nodiscard]] ApplicationGuiFrameOutput draw(
        const ApplicationGuiContext& context) override;
};

} // namespace VkRenderer
