#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

class Window final
{
public:
    struct CreateInfo
    {
        uint32_t width = 1280;
        uint32_t height = 720;
        std::string title = "Vulkan";
    };

    Window() = default;
    explicit Window(const CreateInfo& createInfo);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void create(const CreateInfo& createInfo);
    void reset() noexcept;

    void pollEvents() const;
    void waitEvents() const;
    void waitEventsTimeout(double timeoutSeconds) const;

    [[nodiscard]] bool shouldClose() const;
    [[nodiscard]] VkExtent2D framebufferExtent() const;
    [[nodiscard]] bool consumeFramebufferResize() noexcept;

    [[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance instance) const;
    [[nodiscard]] static std::vector<const char*> requiredVulkanInstanceExtensions();
    [[nodiscard]] static double time() noexcept;

    [[nodiscard]] explicit operator bool() const noexcept;

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* handle_ = nullptr;
    bool ownsGlfwRuntime_ = false;
    bool framebufferResized_ = false;
};

} // namespace VkRenderer
