#include "Window.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace VkRenderer
{
namespace
{

std::mutex gGlfwRuntimeMutex;
uint32_t gGlfwRuntimeUsers = 0;

void acquireGlfwRuntime()
{
    std::lock_guard<std::mutex> lock(gGlfwRuntimeMutex);
    if (gGlfwRuntimeUsers == 0 && glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("failed to initialize GLFW");
    }
    ++gGlfwRuntimeUsers;
}

void releaseGlfwRuntime() noexcept
{
    std::lock_guard<std::mutex> lock(gGlfwRuntimeMutex);
    if (gGlfwRuntimeUsers == 0)
    {
        return;
    }

    --gGlfwRuntimeUsers;
    if (gGlfwRuntimeUsers == 0)
    {
        glfwTerminate();
    }
}

} // namespace

Window::Window(const CreateInfo& createInfo)
{
    create(createInfo);
}

Window::~Window()
{
    reset();
}

void Window::create(const CreateInfo& createInfo)
{
    if (createInfo.width == 0 || createInfo.height == 0)
    {
        throw std::invalid_argument("window dimensions must be greater than zero");
    }

    acquireGlfwRuntime();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* newHandle = glfwCreateWindow(
        static_cast<int>(createInfo.width),
        static_cast<int>(createInfo.height),
        createInfo.title.c_str(),
        nullptr,
        nullptr);
    if (newHandle == nullptr)
    {
        releaseGlfwRuntime();
        throw std::runtime_error("failed to create GLFW window");
    }

    // Acquire the replacement before releasing the old window so replacing the
    // only window cannot terminate GLFW between the two operations.
    reset();
    handle_ = newHandle;
    ownsGlfwRuntime_ = true;
    framebufferResized_ = false;
    glfwSetWindowUserPointer(handle_, this);
    glfwSetFramebufferSizeCallback(handle_, framebufferResizeCallback);
}

void Window::reset() noexcept
{
    if (handle_ != nullptr)
    {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
    }

    if (ownsGlfwRuntime_)
    {
        releaseGlfwRuntime();
        ownsGlfwRuntime_ = false;
    }
    framebufferResized_ = false;
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

void Window::waitEvents() const
{
    glfwWaitEvents();
}

void Window::waitEventsTimeout(double timeoutSeconds) const
{
    glfwWaitEventsTimeout(timeoutSeconds);
}

bool Window::shouldClose() const
{
    if (handle_ == nullptr)
    {
        return true;
    }
    return glfwWindowShouldClose(handle_) == GLFW_TRUE;
}

VkExtent2D Window::framebufferExtent() const
{
    if (handle_ == nullptr)
    {
        return {};
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(handle_, &width, &height);
    return {
        static_cast<uint32_t>(std::max(width, 0)),
        static_cast<uint32_t>(std::max(height, 0))
    };
}

bool Window::consumeFramebufferResize() noexcept
{
    const bool resized = framebufferResized_;
    framebufferResized_ = false;
    return resized;
}

VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const
{
    if (handle_ == nullptr)
    {
        throw std::logic_error("cannot create a Vulkan surface without a window");
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, handle_, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

std::vector<const char*> Window::requiredVulkanInstanceExtensions()
{
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (extensions == nullptr || extensionCount == 0)
    {
        throw std::runtime_error("GLFW did not provide Vulkan instance extensions");
    }
    return {extensions, extensions + extensionCount};
}

double Window::time() noexcept
{
    return glfwGetTime();
}

Window::operator bool() const noexcept
{
    return handle_ != nullptr;
}

void Window::framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr)
    {
        self->framebufferResized_ = true;
    }
}

} // namespace VkRenderer
