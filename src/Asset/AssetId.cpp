#include "Asset/AssetId.h"

#include <array>
#include <random>

namespace VkRenderer
{
namespace
{

constexpr std::size_t kAssetIdTextLength = 36;
constexpr char kHexDigits[] = "0123456789abcdef";

[[nodiscard]] int hexValue(char character) noexcept
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

[[nodiscard]] bool isHyphenPosition(std::size_t index) noexcept
{
    return index == 8 || index == 13 || index == 18 || index == 23;
}

[[nodiscard]] std::mt19937_64 makeGenerator()
{
    std::random_device entropy;
    std::array<uint32_t, 8> seedData{};
    for (uint32_t& seed : seedData)
    {
        seed = entropy();
    }
    std::seed_seq seed(seedData.begin(), seedData.end());
    return std::mt19937_64(seed);
}

} // namespace

AssetId AssetId::generate()
{
    thread_local std::mt19937_64 generator = makeGenerator();

    AssetId id{generator(), generator()};

    // RFC 4122 version 4: xxxx4xxx in the time-high field.
    id.high =
        (id.high & UINT64_C(0xffffffffffff0fff)) |
        UINT64_C(0x0000000000004000);
    // RFC 4122 variant: the most significant two bits are 10.
    id.low =
        (id.low & UINT64_C(0x3fffffffffffffff)) |
        UINT64_C(0x8000000000000000);
    return id;
}

std::optional<AssetId> AssetId::parse(std::string_view text) noexcept
{
    if (text.size() != kAssetIdTextLength)
    {
        return std::nullopt;
    }

    AssetId id{};
    std::size_t hexIndex = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if (isHyphenPosition(index))
        {
            if (text[index] != '-')
            {
                return std::nullopt;
            }
            continue;
        }

        const int value = hexValue(text[index]);
        if (value < 0)
        {
            return std::nullopt;
        }

        uint64_t& half = hexIndex < 16 ? id.high : id.low;
        half = (half << 4) | static_cast<uint64_t>(value);
        ++hexIndex;
    }

    return id;
}

std::string AssetId::toString() const
{
    std::array<char, kAssetIdTextLength> text{};
    std::size_t hexIndex = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if (isHyphenPosition(index))
        {
            text[index] = '-';
            continue;
        }

        const uint64_t half = hexIndex < 16 ? high : low;
        const std::size_t halfHexIndex = hexIndex % 16;
        const std::size_t shift = (15 - halfHexIndex) * 4;
        text[index] = kHexDigits[(half >> shift) & UINT64_C(0xf)];
        ++hexIndex;
    }
    return std::string(text.begin(), text.end());
}

std::size_t AssetIdHash::operator()(AssetId id) const noexcept
{
    const std::size_t highHash = std::hash<uint64_t>{}(id.high);
    const std::size_t lowHash = std::hash<uint64_t>{}(id.low);
    return highHash ^
        (lowHash + static_cast<std::size_t>(0x9e3779b9U) +
         (highHash << 6) + (highHash >> 2));
}

} // namespace VkRenderer
