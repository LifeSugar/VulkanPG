#include "Editor/EditorLayer.h"

#include "Scene/Scene.h"
#include "Vulkan/VulkanRenderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>

#include <cstdint>
#include <stdexcept>

namespace VkRenderer
{

void EditorLayer::attach(const ApplicationGuiContext& context)
{
    registerViewportTextures(context.renderer);
}

void EditorLayer::detach() noexcept
{
    releaseViewportTextures();
}

ApplicationGuiFrameOutput EditorLayer::draw(
    const ApplicationGuiContext& context)
{
    refreshViewportTexturesIfNeeded(context.renderer);
    drawDockSpace();
    ApplicationGuiFrameOutput output{};
    sceneViewportExtent_ = {};

    if (showSceneHierarchy_)
    {
        sceneHierarchyPanel_.draw(
            context.scene,
            context.assets,
            &showSceneHierarchy_);
    }
    if (showInspector_)
    {
        drawInspector(context);
    }
    if (showSceneViewport_)
    {
        output.sceneAspectRatio = drawSceneViewport(context);
    }
    if (showRendererStats_)
    {
        drawRendererStats(context);
    }
    return output;
}

void EditorLayer::drawDockSpace()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiDockNodeFlags dockspaceFlags =
        ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockSpaceHost", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::TextDisabled("Scene persistence is not connected yet");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem(
                "Scene Hierarchy",
                nullptr,
                &showSceneHierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Scene Viewport", nullptr, &showSceneViewport_);
            ImGui::MenuItem("Renderer Stats", nullptr, &showRendererStats_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("EditorMainDockSpace");
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
    {
        ImGui::DockBuilderAddNode(
            dockspaceId,
            dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID center = dockspaceId;
        const ImGuiID left = ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Left,
            0.20f,
            nullptr,
            &center);
        const ImGuiID right = ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Right,
            0.25f,
            nullptr,
            &center);
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Down,
            0.22f,
            nullptr,
            &center);

        ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
        ImGui::DockBuilderDockWindow("Inspector", right);
        ImGui::DockBuilderDockWindow("Renderer Stats", bottom);
        ImGui::DockBuilderDockWindow("Scene Viewport", center);
        ImGui::DockBuilderFinish(dockspaceId);
    }
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    ImGui::End();
}

void EditorLayer::drawInspector(const ApplicationGuiContext& context)
{
    ImGui::Begin("Inspector", &showInspector_);
    static_cast<void>(context);
    ImGui::TextDisabled("Inspector is not connected yet");
    ImGui::End();
}

std::optional<float> EditorLayer::drawSceneViewport(
    const ApplicationGuiContext& context)
{
    const bool visible = ImGui::Begin(
        "Scene Viewport",
        &showSceneViewport_);
    if (!visible)
    {
        ImGui::End();
        return std::nullopt;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (available.x > 0.0f && available.y > 0.0f)
    {
        sceneViewportExtent_ = {
            static_cast<uint32_t>(available.x),
            static_cast<uint32_t>(available.y)
        };
    }
    const uint32_t frameIndex = context.renderer.currentFrameIndex();
    if (available.x > 0.0f && available.y > 0.0f &&
        frameIndex < viewportTextures_.size())
    {
        const ImTextureID textureId = static_cast<ImTextureID>(
            reinterpret_cast<uintptr_t>(viewportTextures_[frameIndex]));
        ImGui::Image(ImTextureRef(textureId), available);
    }
    ImGui::End();
    if (available.x <= 0.0f || available.y <= 0.0f)
    {
        return std::nullopt;
    }
    return available.x / available.y;
}

void EditorLayer::drawRendererStats(
    const ApplicationGuiContext& context)
{
    ImGui::Begin("Renderer Stats", &showRendererStats_);
    static_cast<void>(context);
    const ImGuiIO& io = ImGui::GetIO();
    if (sceneViewportExtent_.width > 0 && sceneViewportExtent_.height > 0)
    {
        ImGui::Text(
            "Scene View: %u x %u",
            sceneViewportExtent_.width,
            sceneViewportExtent_.height);
    }
    else
    {
        ImGui::TextDisabled("Scene View: unavailable");
    }
    ImGui::Text(
        "Frame: %.3f ms (%.1f FPS)",
        io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f,
        io.Framerate);
    ImGui::End();
}

void EditorLayer::registerViewportTextures(
    const VulkanRenderer& renderer)
{
    if (renderer.outputMode() != VulkanRenderer::OutputMode::Editor ||
        renderer.frameCount() == 0)
    {
        throw std::logic_error(
            "EditorLayer requires Renderer Editor output mode");
    }

    releaseViewportTextures();
    viewportTextures_.reserve(renderer.frameCount());
    try
    {
        for (uint32_t frameIndex = 0;
             frameIndex < renderer.frameCount();
             ++frameIndex)
        {
            const VulkanRenderer::EditorViewportOutput output =
                renderer.editorViewportOutput(frameIndex);
            const VkDescriptorSet texture =
                ImGui_ImplVulkan_AddTexture(
                    output.imageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            if (texture == VK_NULL_HANDLE)
            {
                throw std::runtime_error(
                    "failed to register Editor viewport texture with ImGui");
            }
            viewportTextures_.push_back(texture);
            viewportTextureRevision_ = output.revision;
        }
    }
    catch (...)
    {
        releaseViewportTextures();
        throw;
    }
}

void EditorLayer::releaseViewportTextures() noexcept
{
    for (VkDescriptorSet texture : viewportTextures_)
    {
        if (texture != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(texture);
        }
    }
    viewportTextures_.clear();
    viewportTextureRevision_ = 0;
}

void EditorLayer::refreshViewportTexturesIfNeeded(
    const VulkanRenderer& renderer)
{
    if (renderer.frameCount() == 0)
    {
        return;
    }
    const VulkanRenderer::EditorViewportOutput output =
        renderer.editorViewportOutput(0);
    if (viewportTextures_.size() != renderer.frameCount() ||
        viewportTextureRevision_ != output.revision)
    {
        registerViewportTextures(renderer);
    }
}

} // namespace VkRenderer
