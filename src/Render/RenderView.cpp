#include "Render/RenderView.h"

#include <atomic>
#include <limits>
#include <stdexcept>

namespace VkRenderer
{

RenderViewId RenderViewId::generate()
{
    static std::atomic<uint64_t> nextId{1};
    uint64_t value = nextId.load(std::memory_order_relaxed);
    while (true)
    {
        if (value == std::numeric_limits<uint64_t>::max())
        {
            throw std::overflow_error("RenderViewId space is exhausted");
        }
        if (nextId.compare_exchange_weak(
                value,
                value + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
        {
            return RenderViewId{value};
        }
    }
}

} // namespace VkRenderer
