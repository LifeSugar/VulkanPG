#pragma once

#include <cstdint>

namespace VkRenderer
{

/// Stable render-layer indices shared by scene objects and render views.
enum class RenderLayer : uint8_t
{
    World = 0,
    ViewModel,
    UI,
    Editor
};

/// Strongly typed bit mask selecting one or more RenderLayer values.
class LayerMask final
{
public:
    using Storage = uint32_t;

    /// Creates a mask that selects no layers.
    constexpr LayerMask() noexcept = default;
    /// Creates a mask containing exactly one layer.
    constexpr LayerMask(RenderLayer layer) noexcept
        : bits_(layerBit(layer))
    {
    }

    [[nodiscard]] static constexpr LayerMask none() noexcept
    {
        return LayerMask{};
    }

    [[nodiscard]] static constexpr LayerMask all() noexcept
    {
        return fromBits(~Storage{0});
    }

    /// Restores a mask from serialized or API-facing bits.
    [[nodiscard]] static constexpr LayerMask fromBits(Storage bits) noexcept
    {
        return LayerMask(bits, RawBitsTag{});
    }

    [[nodiscard]] constexpr Storage bits() const noexcept
    {
        return bits_;
    }

    [[nodiscard]] constexpr bool contains(RenderLayer layer) const noexcept
    {
        return (bits_ & layerBit(layer)) != 0;
    }

    [[nodiscard]] constexpr bool intersects(LayerMask other) const noexcept
    {
        return (bits_ & other.bits_) != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return bits_ != 0;
    }

    constexpr LayerMask& operator|=(LayerMask other) noexcept
    {
        bits_ |= other.bits_;
        return *this;
    }

    constexpr LayerMask& operator&=(LayerMask other) noexcept
    {
        bits_ &= other.bits_;
        return *this;
    }

    friend constexpr LayerMask operator|(
        LayerMask left,
        LayerMask right) noexcept
    {
        left |= right;
        return left;
    }

    friend constexpr LayerMask operator&(
        LayerMask left,
        LayerMask right) noexcept
    {
        left &= right;
        return left;
    }

    friend constexpr LayerMask operator~(LayerMask mask) noexcept
    {
        return fromBits(~mask.bits_);
    }

    friend constexpr bool operator==(
        LayerMask left,
        LayerMask right) noexcept
    {
        return left.bits_ == right.bits_;
    }

    friend constexpr bool operator!=(
        LayerMask left,
        LayerMask right) noexcept
    {
        return !(left == right);
    }

private:
    struct RawBitsTag
    {
    };

    constexpr LayerMask(Storage bits, RawBitsTag) noexcept
        : bits_(bits)
    {
    }

    [[nodiscard]] static constexpr Storage layerBit(
        RenderLayer layer) noexcept
    {
        const auto index = static_cast<Storage>(layer);
        return index < sizeof(Storage) * 8u
            ? Storage{1} << index
            : Storage{0};
    }

    Storage bits_ = 0;
};

[[nodiscard]] constexpr LayerMask operator|(
    RenderLayer left,
    RenderLayer right) noexcept
{
    return LayerMask(left) | LayerMask(right);
}

[[nodiscard]] constexpr LayerMask operator|(
    LayerMask left,
    RenderLayer right) noexcept
{
    return left | LayerMask(right);
}

[[nodiscard]] constexpr LayerMask operator|(
    RenderLayer left,
    LayerMask right) noexcept
{
    return LayerMask(left) | right;
}

static_assert(LayerMask(RenderLayer::World).bits() == 1u);
static_assert(
    (RenderLayer::World | RenderLayer::Editor).contains(
        RenderLayer::Editor));
static_assert(
    !(RenderLayer::World | RenderLayer::UI).intersects(
        LayerMask(RenderLayer::Editor)));

} // namespace VkRenderer
