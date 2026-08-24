#include "RuntimeGui.h"

#include "Vulkan/VulkanRenderer.h"

#include <imgui.h>

namespace VkRenderer
{

ApplicationGuiFrameOutput RuntimeGui::draw(
    const ApplicationGuiContext& context)
{
    const VkExtent2D renderExtent = context.renderer.extent();
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("Renderer");
    ImGui::Text(
        "Resolution: %u x %u",
        renderExtent.width,
        renderExtent.height);
    ImGui::Text(
        "Frame: %.3f ms (%.1f FPS)",
        io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f,
        io.Framerate);
    ImGui::Text("Draws: scene + present + UI overlay");
    ImGui::End();
    return {};
}

} // namespace VkRenderer
