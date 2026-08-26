#include "Editor/Panels/InspectorPanel.h"

#include "Asset/AssetManager.h"
#include "ApplicationGuiRenderBridge.h"
#include "Scene/Scene.h"

#include <imgui.h>

#include <iterator>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace VkRenderer
{

    namespace
    {

        template <typename... Visitors>
        struct Overloaded : Visitors...
        {
            using Visitors::operator()...;
        };

        template <typename... Visitors>
        Overloaded(Visitors...) -> Overloaded<Visitors...>;

    } // namespace

    std::vector<TextureReimportRequest> InspectorPanel::draw(
        const Scene &scene,
        const AssetManager &assets,
        ApplicationGuiRenderBridge &texturePreviews,
        const TextureImportRegistry* textureImports,
        EditorSelection &selection,
        bool *open)
    {
        const bool visible = ImGui::Begin("Inspector", open);
        if (!visible)
        {
            ImGui::End();
            return {};
        }

        if (selection.canNavigateBack() && ImGui::SmallButton("< Back"))
        {
            selection.navigateBack();
        }

        std::vector<TextureReimportRequest> textureReimports;
        const std::optional<InspectorTarget> navigation = std::visit(
            Overloaded{
                [](std::monostate) -> std::optional<InspectorTarget>
                {
                    ImGui::TextDisabled("Select an object to inspect");
                    return std::nullopt;
                },
                [&](SceneNodeTarget target)
                {
                    return sceneNodeInspector_.draw(scene, assets, target);
                },
                [&](ModelAssetHandle target)
                {
                    return modelInspector_.drawModelAsset(assets, target);
                },
                [&](ModelNodeTarget target)
                {
                    return modelInspector_.drawModelNode(scene, assets, target);
                },
                [&](MeshAssetHandle target)
                {
                    return modelInspector_.drawMeshAsset(assets, target);
                },
                [&](SubmeshTarget target)
                {
                    return modelInspector_.drawSubmesh(assets, target);
                },
                [&](MaterialAssetHandle target)
                {
                    MaterialInspectorOutput output =
                        materialInspector_.drawMaterialAsset(
                        assets,
                        texturePreviews,
                        textureImports,
                        target);
                    textureReimports.insert(
                        textureReimports.end(),
                        std::make_move_iterator(
                            output.textureReimports.begin()),
                        std::make_move_iterator(
                            output.textureReimports.end()));
                    return output.navigation;
                },
                [&](MaterialTemplateAssetHandle target)
                {
                    return materialInspector_.drawMaterialTemplate(
                        assets,
                        target);
                },
                [&](TextureAssetHandle target)
                    -> std::optional<InspectorTarget>
                {
                    std::optional<TextureReimportRequest> request =
                        textureInspector_.draw(
                        assets,
                        texturePreviews,
                        textureImports,
                        target);
                    if (request)
                    {
                        textureReimports.push_back(std::move(*request));
                    }
                    return std::nullopt;
                }},
            selection.target());

        if (navigation)
        {
            selection.navigateTo(std::move(*navigation));
        }
        ImGui::End();
        return textureReimports;
    }

} // namespace VkRenderer
