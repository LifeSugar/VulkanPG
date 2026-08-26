#include "panels/AssetBrowserPanel.hpp"

#include "asset/AssetManager.hpp"
#include "inspectors/InspectorWidgets.hpp"

#include <imgui.h>

#include <string>
#include <vector>

namespace VkRenderer
{
namespace
{

struct GlobalTextureReimportBatch
{
    std::vector<TextureReimportRequest> requests;
    bool busy = false;
};

[[nodiscard]] GlobalTextureReimportBatch collectTextureReimports(
    const AssetManager& assets,
    const TextureImportRegistry* textureImports)
{
    GlobalTextureReimportBatch result{};
    if (textureImports == nullptr)
    {
        return result;
    }

    for (TextureAssetHandle texture : assets.textureHandles())
    {
        const TextureImportRecord* record = textureImports->find(texture);
        if (record == nullptr)
        {
            continue;
        }
        result.busy = result.busy || record->reimporting;
        result.requests.push_back({texture, record->settings});
    }
    return result;
}

template <typename Handle>
[[nodiscard]] bool isSelected(
    const EditorSelection& selection,
    Handle handle) noexcept
{
    const Handle* selected = std::get_if<Handle>(&selection.target());
    return selected != nullptr && *selected == handle;
}

template <typename Handle>
void pushHandleId(Handle handle)
{
    ImGui::PushID(static_cast<int>(handle.index));
    ImGui::PushID(static_cast<int>(handle.generation));
}

void popHandleId()
{
    ImGui::PopID();
    ImGui::PopID();
}

} // namespace

std::vector<TextureReimportRequest> AssetBrowserPanel::draw(
    const AssetManager& assets,
    const TextureImportRegistry* textureImports,
    EditorSelection& selection,
    bool* open) const
{
    std::vector<TextureReimportRequest> reimports;
    const bool visible = ImGui::Begin("Assets", open);
    if (!visible)
    {
        ImGui::End();
        return reimports;
    }

    const GlobalTextureReimportBatch batch =
        collectTextureReimports(assets, textureImports);
    const bool reimportDisabled = batch.requests.empty() || batch.busy;
    ImGui::BeginDisabled(reimportDisabled);
    const std::string buttonLabel =
        "Reimport All (" + std::to_string(batch.requests.size()) + ")";
    if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1.0f, 0.0f)))
    {
        reimports = batch.requests;
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if (batch.requests.empty())
        {
            ImGui::SetTooltip("No source-backed textures are registered");
        }
        else if (batch.busy)
        {
            ImGui::SetTooltip("Texture reimport is already running");
        }
        else
        {
            ImGui::SetTooltip(
                "Cook and replace every registered texture sequentially");
        }
    }

    const std::vector<ModelAssetHandle> models = assets.modelHandles();
    const std::string modelHeader =
        "Models (" + std::to_string(models.size()) + ")";
    if (ImGui::CollapsingHeader(
            modelHeader.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (ModelAssetHandle handle : models)
        {
            pushHandleId(handle);
            const ModelAsset& model = assets.model(handle);
            if (ImGui::Selectable(
                    InspectorWidgets::displayName(
                        model.name(),
                        "Unnamed Model"),
                    isSelected(selection, handle)))
            {
                selection.select(InspectorTarget{handle});
            }
            popHandleId();
        }
    }

    const std::vector<MaterialAssetHandle> materials =
        assets.materialHandles();
    const std::string materialHeader =
        "Materials (" + std::to_string(materials.size()) + ")";
    if (ImGui::CollapsingHeader(
            materialHeader.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (MaterialAssetHandle handle : materials)
        {
            pushHandleId(handle);
            const MaterialAsset& material = assets.material(handle);
            if (ImGui::Selectable(
                    InspectorWidgets::displayName(
                        material.name(),
                        "Unnamed Material"),
                    isSelected(selection, handle)))
            {
                selection.select(InspectorTarget{handle});
            }
            popHandleId();
        }
    }

    const std::vector<TextureAssetHandle> textures =
        assets.textureHandles();
    const std::string textureHeader =
        "Textures (" + std::to_string(textures.size()) + ")";
    if (ImGui::CollapsingHeader(
            textureHeader.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (TextureAssetHandle handle : textures)
        {
            pushHandleId(handle);
            const TextureAsset& texture = assets.texture(handle);
            if (ImGui::Selectable(
                    InspectorWidgets::displayName(
                        texture.name(),
                        "Unnamed Texture"),
                    isSelected(selection, handle)))
            {
                selection.select(InspectorTarget{handle});
            }
            if (textureImports != nullptr)
            {
                const TextureImportRecord* record =
                    textureImports->find(handle);
                if (record != nullptr && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(
                        "%s\n%s",
                        record->sourcePath.string().c_str(),
                        record->reimporting ? "Reimporting" : "Source-backed");
                }
            }
            popHandleId();
        }
    }

    ImGui::End();
    return reimports;
}

} // namespace VkRenderer
