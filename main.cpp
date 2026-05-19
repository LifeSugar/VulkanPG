// ── Standard Library ────────────────────────────────────────────────────────
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// ── GLM ───────────────────────────────────────────────────────────────────────
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ── GLFW ──────────────────────────────────────────────────────────────────────
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// ── Vulkan ────────────────────────────────────────────────────────────────────
#include <vulkan/vulkan.h>

// ─────────────────────────────────────────────────────────────────────────────

static void check_glm()
{
glm::vec3 v(1.0f, 2.0f, 3.0f);
glm::mat4 m = glm::translate(glm::mat4(1.0f), v);
(void)m;
std::cout << "[OK] GLM "
          << GLM_VERSION_MAJOR << '.'
          << GLM_VERSION_MINOR << '.'
          << GLM_VERSION_PATCH << '\n';
}

static void check_glfw()
{
int major, minor, rev;
glfwGetVersion(&major, &minor, &rev);
std::cout << "[OK] GLFW " << major << '.' << minor << '.' << rev << '\n';
}

static void check_vulkan()
{
// Enumerate instance version (Vulkan 1.1+)
uint32_t apiVersion = 0;
if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS) {
std::cerr << "[FAIL] vkEnumerateInstanceVersion() failed\n";
std::exit(EXIT_FAILURE);
}
std::cout << "[OK] Vulkan instance version "
          << VK_VERSION_MAJOR(apiVersion) << '.'
          << VK_VERSION_MINOR(apiVersion) << '.'
          << VK_VERSION_PATCH(apiVersion) << '\n';

// List available instance extensions
uint32_t extCount = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
std::vector<VkExtensionProperties> exts(extCount);
vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());
std::cout << "     " << extCount << " instance extension(s) available\n";

std::vector<const char*> instanceExtensions;
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
if (glfwExtensions != nullptr) {
    instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
}

bool hasPortabilityEnumeration = false;
for (const auto& ext : exts) {
    if (std::strcmp(ext.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
        hasPortabilityEnumeration = true;
        break;
    }
}
if (hasPortabilityEnumeration) {
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
}

// Create a minimal VkInstance to confirm loader + ICD work
VkApplicationInfo appInfo{};
appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName   = "VulkanCheck";
appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
appInfo.pEngineName        = "NoEngine";
appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
appInfo.apiVersion         = VK_API_VERSION_1_0;

VkInstanceCreateInfo createInfo{};
createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo      = &appInfo;
createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
createInfo.ppEnabledExtensionNames = instanceExtensions.empty() ? nullptr : instanceExtensions.data();
if (hasPortabilityEnumeration) {
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}

VkInstance instance = VK_NULL_HANDLE;
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
if (result != VK_SUCCESS) {
    std::cerr << "[FAIL] vkCreateInstance() failed, VkResult " << result << '\n';
    std::exit(EXIT_FAILURE);
}
std::cout << "[OK] VkInstance created successfully\n";

// List physical devices
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
std::cout << "     " << deviceCount << " physical device(s) found\n";
for (const auto& dev : devices) {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(dev, &props);
    std::cout << "       · " << props.deviceName
              << "  (API "
              << VK_VERSION_MAJOR(props.apiVersion) << '.'
              << VK_VERSION_MINOR(props.apiVersion) << '.'
              << VK_VERSION_PATCH(props.apiVersion) << ")\n";
}

vkDestroyInstance(instance, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
std::cout << "=== Dependency check ===\n";
check_glm();
// Initialize GLFW once and keep it alive until all checks are done,
// because check_vulkan() needs glfwGetRequiredInstanceExtensions().
if (!glfwInit()) {
std::cerr << "[FAIL] glfwInit() failed\n";
return EXIT_FAILURE;
}
check_glfw();
check_vulkan();
glfwTerminate();
std::cout << "=== All checks passed ===\n";
return EXIT_SUCCESS;
}
