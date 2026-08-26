#include "Editor/EditorLayer.h"

#include "ApplicationGuiRenderBridge.h"
#include "Scene/Scene.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

namespace VkRenderer
{

ApplicationGuiFrameOutput EditorLayer::draw(
    const ApplicationGuiContext& context)
{
    drawDockSpace();
    ApplicationGuiFrameOutput output{};
    sceneViewportWidth_ = 0;
    sceneViewportHeight_ = 0;

    if (showSceneHierarchy_)
    {
        sceneHierarchyPanel_.draw(
            context.scene,
            context.assets,
            selection_,
            &showSceneHierarchy_);
    }
    if (showInspector_)
    {
        output.textureReimports = inspectorPanel_.draw(
            context.scene,
            context.assets,
            context.render,
            context.textureImports,
            selection_,
            &showInspector_);
    }
    if (showAssets_)
    {
        const ImGuiWindow* inspectorWindow =
            ImGui::FindWindowByName("Inspector");
        if (inspectorWindow != nullptr && inspectorWindow->DockId != 0)
        {
            ImGui::SetNextWindowDockID(
                inspectorWindow->DockId,
                ImGuiCond_FirstUseEver);
        }
        std::vector<TextureReimportRequest> assetReimports =
            assetBrowserPanel_.draw(
                context.assets,
                context.textureImports,
                selection_,
                &showAssets_);
        output.textureReimports.insert(
            output.textureReimports.end(),
            std::make_move_iterator(assetReimports.begin()),
            std::make_move_iterator(assetReimports.end()));
    }
    if (showSceneViewport_)
    {
        output.sceneAspectRatio = drawSceneViewport(context);
    }
    if (showRendererStats_)
    {
        drawRendererStats(context);
    }
    if (showConsole_)
    {
        const ImGuiWindow* statsWindow =
            ImGui::FindWindowByName("Renderer Stats");
        if (statsWindow != nullptr && statsWindow->DockId != 0)
        {
            ImGui::SetNextWindowDockID(
                statsWindow->DockId,
                ImGuiCond_FirstUseEver);
        }
        consolePanel_.draw(&showConsole_);
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
            ImGui::MenuItem("Assets", nullptr, &showAssets_);
            ImGui::MenuItem("Scene Viewport", nullptr, &showSceneViewport_);
            ImGui::MenuItem("Renderer Stats", nullptr, &showRendererStats_);
            ImGui::MenuItem("Console", nullptr, &showConsole_);
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
        ImGui::DockBuilderDockWindow("Assets", right);
        ImGui::DockBuilderDockWindow("Renderer Stats", bottom);
        ImGui::DockBuilderDockWindow("Console", bottom);
        ImGui::DockBuilderDockWindow("Scene Viewport", center);
        ImGui::DockBuilderFinish(dockspaceId);
    }
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
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
    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;
    if (available.x > 0.0f && available.y > 0.0f)
    {
        const ImVec2 framebufferScale =
            ImGui::GetWindowViewport()->FramebufferScale;
        renderWidth = std::max(
            1u,
            static_cast<uint32_t>(
                available.x * framebufferScale.x + 0.5f));
        renderHeight = std::max(
            1u,
            static_cast<uint32_t>(
                available.y * framebufferScale.y + 0.5f));
        sceneViewportWidth_ = renderWidth;
        sceneViewportHeight_ = renderHeight;
        context.render.resizeSceneViewport(renderWidth, renderHeight);
    }
    const ApplicationGuiRenderFrame renderFrame =
        context.render.currentFrame();
    if (available.x > 0.0f && available.y > 0.0f &&
        renderFrame.sceneViewport)
    {
        const ImTextureID textureId = static_cast<ImTextureID>(
            renderFrame.sceneViewport.textureId);
        ImGui::Image(ImTextureRef(textureId), available);
    }
    ImGui::End();
    if (available.x <= 0.0f || available.y <= 0.0f)
    {
        return std::nullopt;
    }
    return static_cast<float>(renderWidth) /
        static_cast<float>(renderHeight);
}

void EditorLayer::drawRendererStats(
    const ApplicationGuiContext& context)
{
    ImGui::Begin("Renderer Stats", &showRendererStats_);
    static_cast<void>(context);
    const ImGuiIO& io = ImGui::GetIO();
    if (sceneViewportWidth_ > 0 && sceneViewportHeight_ > 0)
    {
        ImGui::Text(
            "Scene View: %u x %u",
            sceneViewportWidth_,
            sceneViewportHeight_);
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

} // namespace VkRenderer
