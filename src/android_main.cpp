// Android entry point for the Vulkan renderer.
// Uses android_native_app_glue for lifecycle management.

#include "App.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>

#define LOG_TAG "VulkanApp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{

VkSurfaceKHR createAndroidSurface(VkInstance instance, ANativeWindow* window)
{
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (vkCreateAndroidSurfaceKHR(
            instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
    {
        LOGE("Failed to create Android Vulkan surface");
        return VK_NULL_HANDLE;
    }
    return surface;
}

struct AndroidAppState
{
    VkRenderer::App app;
    bool vulkanInitialized = false;
};

void handleAppCommand(struct android_app* state, int32_t cmd)
{
    auto* appState = static_cast<AndroidAppState*>(state->userData);

    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        if (state->window != nullptr)
        {
            const int32_t width = ANativeWindow_getWidth(state->window);
            const int32_t height = ANativeWindow_getHeight(state->window);
            LOGI("Init window: %dx%d", width, height);

            try
            {
                // Two-phase init: create instance, then surface, then device.
                VkRenderer::VulkanContext::CreateInfo ctxCI{};
                ctxCI.enableValidationLayers = false;
                appState->app.vulkanContext().initInstance(ctxCI);

                VkSurfaceKHR surface = createAndroidSurface(
                    appState->app.vulkanContext().instance(),
                    state->window);

                appState->app.initVulkan(
                    surface,
                    static_cast<uint32_t>(width > 0 ? width : 1280),
                    static_cast<uint32_t>(height > 0 ? height : 720));
                appState->vulkanInitialized = true;
            }
            catch (const std::exception& e)
            {
                LOGE("Vulkan init failed: %s", e.what());
            }
        }
        break;

    case APP_CMD_TERM_WINDOW:
        LOGI("Term window");
        // Surface will be destroyed by Android; stop using it.
        // A full cleanup on Android is handled in APP_CMD_DESTROY.
        break;

    case APP_CMD_WINDOW_RESIZED:
        if (state->window != nullptr)
        {
            const int32_t width = ANativeWindow_getWidth(state->window);
            const int32_t height = ANativeWindow_getHeight(state->window);
            LOGI("Window resized: %dx%d", width, height);
            if (appState->vulkanInitialized && width > 0 && height > 0)
            {
                appState->app.resizeSwapchain(
                    {static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height)});
            }
        }
        break;

    case APP_CMD_DESTROY:
        LOGI("Destroy");
        appState->app.cleanup();
        appState->vulkanInitialized = false;
        break;

    default:
        break;
    }
}

} // namespace

void android_main(struct android_app* state)
{
    AndroidAppState appState;

    // Register lifecycle callbacks before the event loop starts.
    state->userData = &appState;
    state->onAppCmd = handleAppCommand;

    LOGI("Android Vulkan app started");

    while (true)
    {
        // Process Android events (APP_CMD_* callbacks).
        int events;
        struct android_poll_source* source;
        const int timeoutMillis = appState.vulkanInitialized ? 0 : -1;
        ALooper_pollOnce(timeoutMillis, nullptr, &events,
                         reinterpret_cast<void**>(&source));
        if (source != nullptr)
        {
            source->process(state, source);
        }
        if (state->destroyRequested != 0)
        {
            return;
        }

        // Render a frame if Vulkan is initialized and we have a window.
        if (appState.vulkanInitialized && state->window != nullptr)
        {
            try
            {
                appState.app.renderFrame();
            }
            catch (const std::exception& e)
            {
                LOGE("Render error: %s", e.what());
            }
        }
    }
}
