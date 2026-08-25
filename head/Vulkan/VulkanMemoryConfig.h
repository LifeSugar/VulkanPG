#pragma once

// Set to 0 before including renderer headers, or configure CMake with
// -DVK_RENDERER_USE_VMA=OFF, to use the original Vulkan memory path.
#ifndef VK_RENDERER_USE_VMA
#define VK_RENDERER_USE_VMA 1
#endif
