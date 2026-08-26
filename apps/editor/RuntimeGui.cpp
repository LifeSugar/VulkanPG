#include "RuntimeGui.hpp"

#include "render/ApplicationGuiRenderBridge.hpp"

#include <imgui.h>

namespace VkRenderer
{

ApplicationGuiFrameOutput RuntimeGui::draw(
    const ApplicationGuiContext& context)
{
    const ApplicationGuiRenderFrame renderFrame =
        context.render.currentFrame();
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("Renderer");
    ImGui::Text(
        "Resolution: %u x %u",
        renderFrame.width,
        renderFrame.height);
    ImGui::Text(
        "Frame: %.3f ms (%.1f FPS)",
        io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f,
        io.Framerate);
    ImGui::Text("Draws: scene + present + UI overlay");
    ImGui::End();
    return {};
}

} // namespace VkRenderer
