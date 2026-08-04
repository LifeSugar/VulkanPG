#include "Import/WicImageDecoder.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace VkRenderer
{
namespace
{

using Microsoft::WRL::ComPtr;

void requireSucceeded(HRESULT result, const char* operation)
{
    if (FAILED(result))
    {
        throw std::runtime_error(
            std::string("WIC failed to ") + operation);
    }
}

class ComApartment final
{
public:
    ComApartment()
        : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
    {
        if (FAILED(result_) && result_ != RPC_E_CHANGED_MODE)
        {
            throw std::runtime_error("failed to initialize COM for WIC");
        }
    }

    ~ComApartment()
    {
        if (result_ == S_OK || result_ == S_FALSE)
        {
            CoUninitialize();
        }
    }

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

private:
    HRESULT result_ = E_FAIL;
};

ComPtr<IWICImagingFactory> createFactory()
{
    ComPtr<IWICImagingFactory> factory;
    requireSucceeded(
        CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory)),
        "create the imaging factory");
    return factory;
}

TextureAsset::CreateInfo decodeFrame(
    IWICImagingFactory& factory,
    IWICBitmapDecoder& decoder,
    std::string name)
{
    ComPtr<IWICBitmapFrameDecode> frame;
    requireSucceeded(
        decoder.GetFrame(0, &frame),
        "read the first image frame");

    UINT width = 0;
    UINT height = 0;
    requireSucceeded(
        frame->GetSize(&width, &height),
        "read image dimensions");
    if (width == 0 || height == 0 ||
        width > std::numeric_limits<UINT>::max() / 4)
    {
        throw std::runtime_error("WIC image dimensions are invalid");
    }

    const UINT stride = width * 4;
    if (height > std::numeric_limits<UINT>::max() / stride)
    {
        throw std::overflow_error(
            "WIC decoded image exceeds the supported byte range");
    }
    const UINT byteSize = stride * height;

    ComPtr<IWICFormatConverter> converter;
    requireSucceeded(
        factory.CreateFormatConverter(&converter),
        "create an RGBA format converter");
    requireSucceeded(
        converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom),
        "convert image pixels to RGBA8");

    TextureAsset::CreateInfo textureInfo{};
    textureInfo.name = std::move(name);
    textureInfo.width = width;
    textureInfo.height = height;
    textureInfo.format = TextureFormat::RGBA8UNorm;
    textureInfo.pixels.resize(byteSize);
    requireSucceeded(
        converter->CopyPixels(
            nullptr,
            stride,
            byteSize,
            reinterpret_cast<BYTE*>(textureInfo.pixels.data())),
        "copy decoded RGBA8 pixels");
    return textureInfo;
}

} // namespace

TextureAsset::CreateInfo WicImageDecoder::decodeMemory(
    const std::vector<uint8_t>& encodedBytes,
    const std::string& name) const
{
    if (encodedBytes.empty() ||
        encodedBytes.size() > std::numeric_limits<DWORD>::max())
    {
        throw std::invalid_argument(
            "WIC encoded image payload has an invalid size");
    }

    const ComApartment apartment;
    ComPtr<IWICImagingFactory> factory = createFactory();
    ComPtr<IWICStream> stream;
    requireSucceeded(
        factory->CreateStream(&stream),
        "create an image stream");
    requireSucceeded(
        stream->InitializeFromMemory(
            const_cast<BYTE*>(encodedBytes.data()),
            static_cast<DWORD>(encodedBytes.size())),
        "initialize an encoded image stream");

    ComPtr<IWICBitmapDecoder> decoder;
    requireSucceeded(
        factory->CreateDecoderFromStream(
            stream.Get(),
            nullptr,
            WICDecodeMetadataCacheOnLoad,
            &decoder),
        "decode an in-memory image");
    return decodeFrame(*factory.Get(), *decoder.Get(), name);
}

TextureAsset::CreateInfo WicImageDecoder::decodeFile(
    const std::filesystem::path& path,
    const std::string& name) const
{
    if (path.empty())
    {
        throw std::invalid_argument(
            "WIC image decoder requires a file path");
    }

    const ComApartment apartment;
    ComPtr<IWICImagingFactory> factory = createFactory();
    ComPtr<IWICBitmapDecoder> decoder;
    requireSucceeded(
        factory->CreateDecoderFromFilename(
            path.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder),
        "decode an image file");
    return decodeFrame(
        *factory.Get(),
        *decoder.Get(),
        name.empty() ? path.filename().string() : name);
}

} // namespace VkRenderer

#else

#include <stdexcept>

namespace VkRenderer
{

TextureAsset::CreateInfo WicImageDecoder::decodeMemory(
    const std::vector<uint8_t>&,
    const std::string&) const
{
    throw std::runtime_error(
        "WicImageDecoder is only available on Windows");
}

TextureAsset::CreateInfo WicImageDecoder::decodeFile(
    const std::filesystem::path&,
    const std::string&) const
{
    throw std::runtime_error(
        "WicImageDecoder is only available on Windows");
}

} // namespace VkRenderer

#endif
