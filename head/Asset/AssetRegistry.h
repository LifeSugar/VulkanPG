#pragma once

#include "Asset/AssetHandle.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{

/// Stores one asset type and rejects handles whose slots were recycled.
template <typename Asset>
class AssetRegistry final
{
public:
    using Handle = AssetHandle<Asset>;

    [[nodiscard]] Handle insert(Asset asset)
    {
        uint32_t index = 0;
        if (freeIndices_.empty())
        {
            if (slots_.size() >= kInvalidAssetIndex)
            {
                throw std::overflow_error("asset registry capacity exceeded");
            }
            index = static_cast<uint32_t>(slots_.size());
            slots_.push_back({});
        }
        else
        {
            index = freeIndices_.back();
            freeIndices_.pop_back();
        }

        Slot& slot = slots_[index];
        slot.asset.emplace(std::move(asset));
        ++size_;
        return {index, slot.generation};
    }

    template <typename... Arguments>
    [[nodiscard]] Handle emplace(Arguments&&... arguments)
    {
        return insert(Asset(std::forward<Arguments>(arguments)...));
    }

    [[nodiscard]] bool contains(Handle handle) const noexcept
    {
        return handle.index < slots_.size() &&
            slots_[handle.index].asset.has_value() &&
            slots_[handle.index].generation == handle.generation;
    }

    [[nodiscard]] const Asset& get(Handle handle) const
    {
        if (!contains(handle))
        {
            throw std::out_of_range("asset handle is invalid or stale");
        }
        return *slots_[handle.index].asset;
    }

    bool erase(Handle handle) noexcept
    {
        if (!contains(handle))
        {
            return false;
        }

        Slot& slot = slots_[handle.index];
        slot.asset.reset();
        slot.generation = nextGeneration(slot.generation);
        freeIndices_.push_back(handle.index);
        --size_;
        return true;
    }

    void reset() noexcept
    {
        freeIndices_.clear();
        freeIndices_.reserve(slots_.size());
        for (uint32_t index = 0;
             index < static_cast<uint32_t>(slots_.size());
             ++index)
        {
            Slot& slot = slots_[index];
            slot.asset.reset();
            slot.generation = nextGeneration(slot.generation);
            freeIndices_.push_back(index);
        }
        size_ = 0;
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    struct Slot
    {
        std::optional<Asset> asset;
        uint32_t generation = 1;
    };

    [[nodiscard]] static uint32_t nextGeneration(uint32_t generation) noexcept
    {
        return generation == std::numeric_limits<uint32_t>::max()
            ? 1
            : generation + 1;
    }

    std::vector<Slot> slots_;
    std::vector<uint32_t> freeIndices_;
    std::size_t size_ = 0;
};

} // namespace VkRenderer
