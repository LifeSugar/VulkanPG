#pragma once

#include "Asset/AssetFwd.h"
#include "Asset/TextureAsset.h"
#include "Import/TextureImportRegistry.h"
#include "Import/TextureImportSettings.h"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;

class TextureInspector final
{
public:
    [[nodiscard]] std::optional<TextureReimportRequest> draw(
        const AssetManager& assets,
        ApplicationGuiRenderBridge& texturePreviews,
        const TextureImportRegistry* textureImports,
        TextureAssetHandle target);

private:
    struct ImportDraft
    {
        bool initialized = false;
        TextureColorSpace transferFunction = TextureColorSpace::Linear;
        KtxPayloadEncoding payloadEncoding = KtxPayloadEncoding::Uncompressed;
        bool generateMipmaps = false;
        TextureMipFilter mipFilter = TextureMipFilter::Mitchell;
        TextureMipEdgeMode mipEdgeMode = TextureMipEdgeMode::Clamp;
        bool normalMap = false;
        std::array<char, 5> inputSwizzle{};
        uint32_t threadCount = defaultTextureImportThreadCount();
        uint32_t etc1sCompressionLevel = 2;
        uint32_t etc1sQualityLevel = 128;
        uint32_t uastcQualityLevel = 2;
        bool uastcRdo = false;
        float uastcRdoQualityScalar = 1.0f;
        uint32_t uastcRdoDictionarySize = 4096;
        uint32_t zstdLevel = 0;
        TextureFormat transcodeFormat = TextureFormat::BC7UNorm;
        bool highQualityTranscode = true;
    };

    [[nodiscard]] ImportDraft& draftFor(
        TextureAssetHandle target,
        const TextureAsset& texture,
        const TextureImportRecord* importRecord);

    std::unordered_map<uint64_t, ImportDraft> importDrafts_;
};

} // namespace VkRenderer
