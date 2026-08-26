#include "vulkan/VulkanMemoryConfig.hpp"

#if VK_RENDERER_USE_VMA
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif
