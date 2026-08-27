#pragma once

#include "EditorFwd.hpp"
#include "asset/AssetFwd.hpp"
#include "asset/TextureAsset.hpp"
#include "texture/TextureImportRegistry.hpp"
#include "texture/TextureImportSettings.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace rubia::editor
{

class TextureInspector final
{
public:
    [[nodiscard]] std::optional<importer::texture::TextureReimportRequest> draw(
        const asset::AssetManager& assets,
        render::ApplicationGuiRenderBridge& texturePreviews,
        const importer::texture::TextureImportRegistry* textureImports,
        asset::TextureAssetHandle target);

private:
    struct ImportDraft
    {
        bool initialized = false;
        asset::TextureColorSpace transferFunction = asset::TextureColorSpace::Linear;
        importer::texture::KtxPayloadEncoding payloadEncoding = importer::texture::KtxPayloadEncoding::Uncompressed;
        bool generateMipmaps = false;
        importer::texture::TextureMipFilter mipFilter = importer::texture::TextureMipFilter::Mitchell;
        importer::texture::TextureMipEdgeMode mipEdgeMode = importer::texture::TextureMipEdgeMode::Clamp;
        bool normalMap = false;
        std::array<char, 5> inputSwizzle{};
        uint32_t threadCount =
            importer::texture::defaultTextureImportThreadCount();
        uint32_t etc1sCompressionLevel = 2;
        uint32_t etc1sQualityLevel = 128;
        uint32_t uastcQualityLevel = 2;
        bool uastcRdo = false;
        float uastcRdoQualityScalar = 1.0f;
        uint32_t uastcRdoDictionarySize = 4096;
        uint32_t zstdLevel = 0;
        asset::TextureFormat transcodeFormat = asset::TextureFormat::BC7UNorm;
        bool highQualityTranscode = true;
    };

    [[nodiscard]] ImportDraft& draftFor(
        asset::TextureAssetHandle target,
        const asset::TextureAsset& texture,
        const importer::texture::TextureImportRecord* importRecord);

    std::unordered_map<uint64_t, ImportDraft> importDrafts_;
};

} // namespace rubia::editor
