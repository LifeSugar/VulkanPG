#include "Editor/Inspectors/TextureInspector.h"

#include "Asset/AssetManager.h"
#include "ApplicationGuiRenderBridge.h"
#include "Editor/Inspectors/InspectorWidgets.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>

namespace VkRenderer
{

namespace
{

[[nodiscard]] const char* textureFormatName(TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::Undefined: return "Undefined";
    case TextureFormat::R8UNorm: return "R8 UNorm";
    case TextureFormat::RG8UNorm: return "RG8 UNorm";
    case TextureFormat::RGBA8UNorm: return "RGBA8 UNorm";
    case TextureFormat::RGBA16Float: return "RGBA16 Float";
    case TextureFormat::RGBA32Float: return "RGBA32 Float";
    case TextureFormat::BC1RGBUNorm: return "BC1 RGB UNorm";
    case TextureFormat::BC1RGBAUNorm: return "BC1 RGBA UNorm";
    case TextureFormat::BC2UNorm: return "BC2 UNorm";
    case TextureFormat::BC3UNorm: return "BC3 RGBA UNorm";
    case TextureFormat::BC4UNorm: return "BC4 R UNorm";
    case TextureFormat::BC4SNorm: return "BC4 R SNorm";
    case TextureFormat::BC5UNorm: return "BC5 RG UNorm";
    case TextureFormat::BC5SNorm: return "BC5 RG SNorm";
    case TextureFormat::BC6HUFloat: return "BC6H UFloat";
    case TextureFormat::BC6HSFloat: return "BC6H SFloat";
    case TextureFormat::BC7UNorm: return "BC7 RGBA UNorm";
    }
    return "Unknown";
}

[[nodiscard]] std::string byteSizeName(std::size_t byteSize)
{
    constexpr double kibibyte = 1024.0;
    constexpr double mebibyte = kibibyte * 1024.0;
    char text[64]{};
    if (static_cast<double>(byteSize) >= mebibyte)
    {
        std::snprintf(
            text,
            sizeof(text),
            "%.2f MiB",
            static_cast<double>(byteSize) / mebibyte);
    }
    else if (static_cast<double>(byteSize) >= kibibyte)
    {
        std::snprintf(
            text,
            sizeof(text),
            "%.2f KiB",
            static_cast<double>(byteSize) / kibibyte);
    }
    else
    {
        std::snprintf(text, sizeof(text), "%zu bytes", byteSize);
    }
    return text;
}

template <typename Enum, std::size_t Size>
bool drawEnumCombo(
    const char* label,
    const char* id,
    Enum& value,
    const std::array<std::pair<Enum, const char*>, Size>& options)
{
    const auto current = std::find_if(
        options.begin(),
        options.end(),
        [&](const auto& option) { return option.first == value; });
    const char* preview = current == options.end()
        ? "Unknown"
        : current->second;

    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(-1.0f);
    bool changed = false;
    if (ImGui::BeginCombo(id, preview))
    {
        for (const auto& option : options)
        {
            const bool selected = option.first == value;
            if (ImGui::Selectable(option.second, selected))
            {
                value = option.first;
                changed = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool drawCheckbox(const char* label, const char* id, bool& value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(140.0f);
    return ImGui::Checkbox(id, &value);
}

bool drawUIntSlider(
    const char* label,
    const char* id,
    uint32_t& value,
    int minimum,
    int maximum)
{
    int editable = static_cast<int>(value);
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(-1.0f);
    if (!ImGui::SliderInt(id, &editable, minimum, maximum))
    {
        return false;
    }
    value = static_cast<uint32_t>(editable);
    return true;
}

void drawValidationMessage(const char* message)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.25f, 1.0f));
    ImGui::TextWrapped("%s", message);
    ImGui::PopStyleColor();
}

constexpr std::array transferFunctions{
    std::pair{TextureColorSpace::Linear, "Linear"},
    std::pair{TextureColorSpace::Srgb, "sRGB"}};

constexpr std::array payloadEncodings{
    std::pair{KtxPayloadEncoding::Uncompressed, "Uncompressed"},
    std::pair{KtxPayloadEncoding::Etc1s, "ETC1S"},
    std::pair{KtxPayloadEncoding::Uastc, "UASTC"}};

constexpr std::array mipFilters{
    std::pair{TextureMipFilter::Box, "Box"},
    std::pair{TextureMipFilter::Triangle, "Triangle"},
    std::pair{TextureMipFilter::CubicBSpline, "Cubic B-Spline"},
    std::pair{TextureMipFilter::CatmullRom, "Catmull-Rom"},
    std::pair{TextureMipFilter::Mitchell, "Mitchell"},
    std::pair{TextureMipFilter::Point, "Point"}};

constexpr std::array mipEdgeModes{
    std::pair{TextureMipEdgeMode::Clamp, "Clamp"},
    std::pair{TextureMipEdgeMode::Reflect, "Reflect"},
    std::pair{TextureMipEdgeMode::Wrap, "Wrap"},
    std::pair{TextureMipEdgeMode::Zero, "Zero"}};

constexpr std::array transcodeFormats{
    std::pair{TextureFormat::BC1RGBUNorm, "BC1 RGB"},
    std::pair{TextureFormat::BC3UNorm, "BC3 RGBA"},
    std::pair{TextureFormat::BC4UNorm, "BC4 R"},
    std::pair{TextureFormat::BC5UNorm, "BC5 RG"},
    std::pair{TextureFormat::BC7UNorm, "BC7 RGBA"}};

} // namespace

TextureInspector::ImportDraft& TextureInspector::draftFor(
    TextureAssetHandle target,
    const TextureAsset& texture,
    const TextureImportRecord* importRecord)
{
    const uint64_t key =
        (static_cast<uint64_t>(target.generation) << 32u) |
        static_cast<uint64_t>(target.index);
    ImportDraft& draft = importDrafts_[key];
    if (!draft.initialized)
    {
        draft.initialized = true;
        if (importRecord != nullptr)
        {
            const TextureImportSettings& settings = importRecord->settings;
            draft.transferFunction = settings.colorSpace;
            draft.payloadEncoding = settings.basis.encoding;
            draft.generateMipmaps = settings.generateMipmaps;
            draft.mipFilter = settings.mipFilter;
            draft.mipEdgeMode = settings.mipEdgeMode;
            draft.normalMap = settings.basis.normalMap;
            std::snprintf(
                draft.inputSwizzle.data(),
                draft.inputSwizzle.size(),
                "%s",
                settings.basis.inputSwizzle.c_str());
            draft.threadCount = settings.basis.threadCount;
            draft.etc1sCompressionLevel =
                settings.basis.etc1sCompressionLevel;
            draft.etc1sQualityLevel = settings.basis.etc1sQualityLevel;
            draft.uastcQualityLevel = settings.basis.uastcQualityLevel;
            draft.uastcRdo = settings.basis.uastcRdo;
            draft.uastcRdoQualityScalar =
                settings.basis.uastcRdoQualityScalar;
            draft.uastcRdoDictionarySize =
                settings.basis.uastcRdoDictionarySize;
            draft.zstdLevel = settings.zstdLevel;
            draft.transcodeFormat = settings.transcodeFormat;
            draft.highQualityTranscode = settings.highQualityTranscode;
            return draft;
        }

        draft.transferFunction = texture.colorSpace();
        const auto supportedTarget = std::find_if(
            transcodeFormats.begin(),
            transcodeFormats.end(),
            [&](const auto& option)
            {
                return option.first == texture.format();
            });
        if (supportedTarget != transcodeFormats.end())
        {
            draft.transcodeFormat = texture.format();
        }
    }
    return draft;
}

std::optional<TextureReimportRequest> TextureInspector::draw(
    const AssetManager& assets,
    ApplicationGuiRenderBridge& texturePreviews,
    const TextureImportRegistry* textureImports,
    TextureAssetHandle target)
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled("TextureAsset selection is no longer valid");
        return std::nullopt;
    }

    const TextureAsset& texture = assets.texture(target);
    const TextureImportRecord* importRecord = textureImports == nullptr
        ? nullptr
        : textureImports->find(target);
    ImportDraft& draft = draftFor(target, texture, importRecord);

    ImGui::SeparatorText("Texture Asset");
    drawProperty("Name", displayName(texture.name(), "Unnamed Texture"));

    ImGui::SeparatorText("Preview");
    drawTextureImage(
        texturePreviews,
        target,
        texture,
        ImGui::GetContentRegionAvail().x,
        320.0f);

    ImGui::SeparatorText("Current Result");
    ImGui::TextDisabled("Size");
    ImGui::SameLine(140.0f);
    ImGui::Text("%u x %u", texture.width(), texture.height());
    drawProperty("Format", textureFormatName(texture.format()));
    drawProperty(
        "Transfer Function",
        texture.colorSpace() == TextureColorSpace::Srgb ? "sRGB" : "Linear");
    drawProperty(
        "Mip Levels",
        static_cast<uint32_t>(texture.mipLevels().size()));
    const std::string payloadSize = byteSizeName(texture.payload().size());
    drawProperty("CPU Payload", payloadSize.c_str());

    ImGui::SeparatorText("Import Settings");
    if (importRecord != nullptr)
    {
        drawProperty("Source", importRecord->sourcePath.string().c_str());
        drawProperty("Cooked KTX2", importRecord->cookedPath.string().c_str());
        ImGui::TextDisabled("Revision");
        ImGui::SameLine(140.0f);
        ImGui::Text("%llu", static_cast<unsigned long long>(
            importRecord->revision));
        if (importRecord->reimporting)
        {
            ImGui::TextDisabled(
                "Cooking and importing KTX2 in the background...");
        }
        if (!importRecord->lastError.empty())
        {
            drawValidationMessage(importRecord->lastError.c_str());
        }
    }
    else
    {
        ImGui::TextDisabled(
            "This texture has no source import record and cannot be reimported.");
    }

    drawEnumCombo(
        "Transfer Function",
        "##TextureTransferFunction",
        draft.transferFunction,
        transferFunctions);
    drawEnumCombo(
        "KTX2 Encoding",
        "##TexturePayloadEncoding",
        draft.payloadEncoding,
        payloadEncodings);
    drawCheckbox(
        "Generate Mips",
        "##TextureGenerateMips",
        draft.generateMipmaps);
    if (draft.generateMipmaps)
    {
        drawEnumCombo(
            "Mip Filter",
            "##TextureMipFilter",
            draft.mipFilter,
            mipFilters);
        drawEnumCombo(
            "Mip Edge",
            "##TextureMipEdge",
            draft.mipEdgeMode,
            mipEdgeModes);
    }

    const bool normalMapChanged = drawCheckbox(
        "Normal Map",
        "##TextureNormalMap",
        draft.normalMap);
    if (normalMapChanged && draft.normalMap)
    {
        draft.transferFunction = TextureColorSpace::Linear;
        draft.transcodeFormat = TextureFormat::BC5UNorm;
    }
    ImGui::TextDisabled("Input Swizzle");
    ImGui::SameLine(140.0f);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(
        "##TextureInputSwizzle",
        draft.inputSwizzle.data(),
        draft.inputSwizzle.size());
    drawUIntSlider(
        "Threads",
        "##TextureThreads",
        draft.threadCount,
        1,
        64);

    if (draft.payloadEncoding == KtxPayloadEncoding::Etc1s)
    {
        drawUIntSlider(
            "ETC1S Level",
            "##TextureEtc1sLevel",
            draft.etc1sCompressionLevel,
            0,
            6);
        drawUIntSlider(
            "ETC1S Quality",
            "##TextureEtc1sQuality",
            draft.etc1sQualityLevel,
            1,
            255);
    }
    else if (draft.payloadEncoding == KtxPayloadEncoding::Uastc)
    {
        drawUIntSlider(
            "UASTC Quality",
            "##TextureUastcQuality",
            draft.uastcQualityLevel,
            0,
            4);
        drawCheckbox(
            "UASTC RDO",
            "##TextureUastcRdo",
            draft.uastcRdo);
        if (draft.uastcRdo)
        {
            ImGui::TextDisabled("RDO Scalar");
            ImGui::SameLine(140.0f);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SliderFloat(
                "##TextureUastcRdoScalar",
                &draft.uastcRdoQualityScalar,
                0.001f,
                50.0f,
                "%.3f",
                ImGuiSliderFlags_Logarithmic);
            drawUIntSlider(
                "RDO Dictionary",
                "##TextureUastcRdoDictionary",
                draft.uastcRdoDictionarySize,
                64,
                65536);
        }
    }

    const bool zstdAvailable =
        draft.payloadEncoding != KtxPayloadEncoding::Etc1s;
    ImGui::BeginDisabled(!zstdAvailable);
    drawUIntSlider(
        "Zstd Level",
        "##TextureZstdLevel",
        draft.zstdLevel,
        0,
        22);
    ImGui::EndDisabled();
    if (!zstdAvailable)
    {
        ImGui::TextDisabled("ETC1S uses BasisLZ; Zstd is unavailable.");
    }

    ImGui::SeparatorText("Runtime Transcode");
    const bool requiresTranscode =
        draft.payloadEncoding != KtxPayloadEncoding::Uncompressed;
    ImGui::BeginDisabled(!requiresTranscode);
    drawEnumCombo(
        "Texture Format",
        "##TextureTranscodeFormat",
        draft.transcodeFormat,
        transcodeFormats);
    drawCheckbox(
        "High Quality",
        "##TextureHighQualityTranscode",
        draft.highQualityTranscode);
    ImGui::EndDisabled();
    if (!requiresTranscode)
    {
        ImGui::TextDisabled(
            "Uncompressed KTX2 keeps its stored pixel format.");
    }

    bool invalidSettings = false;
    if (draft.normalMap &&
        draft.transferFunction == TextureColorSpace::Srgb)
    {
        invalidSettings = true;
        drawValidationMessage("Normal-map encoding requires Linear data.");
    }
    if (requiresTranscode &&
        draft.transferFunction == TextureColorSpace::Srgb &&
        (draft.transcodeFormat == TextureFormat::BC4UNorm ||
         draft.transcodeFormat == TextureFormat::BC5UNorm))
    {
        invalidSettings = true;
        drawValidationMessage(
            "BC4 and BC5 have no sRGB representation; choose Linear or another runtime format.");
    }
    if (draft.inputSwizzle[0] != '\0')
    {
        const std::string swizzle(draft.inputSwizzle.data());
        if (swizzle.size() != 4 ||
            swizzle.find_first_not_of("rgba01") != std::string::npos)
        {
            invalidSettings = true;
            drawValidationMessage(
                "Input swizzle must contain exactly four rgba01 components.");
        }
    }

    ImGui::Spacing();
    const bool reimporting =
        importRecord != nullptr && importRecord->reimporting;
    ImGui::BeginDisabled(
        importRecord == nullptr || invalidSettings || reimporting);
    const bool reimport = ImGui::Button(
        reimporting ? "Cooking..." : "Reimport",
        ImVec2(-1.0f, 0.0f));
    ImGui::EndDisabled();
    if (!reimport)
    {
        return std::nullopt;
    }

    TextureImportSettings settings{};
    settings.colorSpace = draft.transferFunction;
    settings.generateMipmaps = draft.generateMipmaps;
    settings.mipFilter = draft.mipFilter;
    settings.mipEdgeMode = draft.mipEdgeMode;
    settings.basis.encoding = draft.payloadEncoding;
    settings.basis.threadCount = draft.threadCount;
    settings.basis.inputSwizzle = draft.inputSwizzle.data();
    settings.basis.normalMap = draft.normalMap;
    settings.basis.etc1sCompressionLevel = draft.etc1sCompressionLevel;
    settings.basis.etc1sQualityLevel = draft.etc1sQualityLevel;
    settings.basis.uastcQualityLevel = draft.uastcQualityLevel;
    settings.basis.uastcRdo = draft.uastcRdo;
    settings.basis.uastcRdoQualityScalar = draft.uastcRdoQualityScalar;
    settings.basis.uastcRdoDictionarySize = draft.uastcRdoDictionarySize;
    settings.zstdLevel = draft.zstdLevel;
    settings.transcodeFormat = draft.transcodeFormat;
    settings.highQualityTranscode = draft.highQualityTranscode;
    return TextureReimportRequest{target, std::move(settings)};
}

} // namespace VkRenderer
