#pragma once

#include "ApplicationGui.hpp"

namespace rubia::editor
{

/// Minimal GUI used by the non-editor runtime.
class RuntimeGui final : public ApplicationGui
{
public:
    [[nodiscard]] ApplicationGuiFrameOutput draw(
        const ApplicationGuiContext& context) override;
};

} // namespace rubia::editor
