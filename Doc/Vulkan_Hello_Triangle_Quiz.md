# Vulkan Hello Triangle 阶段测试题

总分：100 分  
建议时间：60–90 分钟  
目标：确认你是否真正理解 Vulkan 的初始化链路、资源关系、RenderPass / Pipeline / Swapchain / Shader 等基础概念。

---

## 一、概念判断题 20 分

每题 2 分，判断对错，并说明一句理由。

### 1. VkInstance 可以理解为 Vulkan 程序和 GPU 之间的直接连接。

---

### 2. Vulkan 中的 Surface 通常和窗口系统有关，比如 Win32、XCB、Cocoa、GLFW。

---

### 3. PhysicalDevice 是逻辑设备，LogicalDevice 是物理设备。

---

### 4. QueueFamily 表示 GPU 上某一类队列能力，例如 Graphics、Compute、Transfer、Present。

---

### 5. Swapchain 的 Image 是我们自己用 `vkCreateImage` 创建出来的。

---

### 6. Swapchain ImageView 是对 Swapchain Image 的一种“解释方式”。

---

### 7. RenderPass 只描述“用哪些 attachment、如何加载/存储、布局如何转换”，不直接保存真实图像内存。

---

### 8. Framebuffer 是 RenderPass 和具体 ImageView 的绑定。

---

### 9. VkShaderModule 就是 GPU 已经完全优化好的最终机器码。

---

### 10. Vulkan 的 Pipeline 可以粗略理解为现代图形 API 里的 PSO，也就是一整套固定好的渲染状态组合。

---

## 二、流程排序题 15 分

下面是 Hello Triangle 的核心创建步骤，请按合理顺序排序。

A. 创建 Graphics Pipeline  
B. 创建 Swapchain ImageViews  
C. 创建 VkInstance  
D. 创建 Framebuffers  
E. 创建 Logical Device 和 Queue  
F. 创建 Surface  
G. 选择 Physical Device  
H. 创建 RenderPass  
I. 创建 Swapchain  
J. 创建 ShaderModule  
K. 创建 CommandBuffer  
L. 创建 Sync Objects  
M. 主循环中 acquire image / submit / present

请写出顺序，例如：

```text
C → F → ...
```

---

## 三、简答题 35 分

### 1. 为什么 Vulkan 要先创建 Instance，再创建 Surface，再选择 PhysicalDevice？

5 分

---

### 2. 为什么 Swapchain CreateInfo 里已经有 format，创建 ImageView 时还要再指定 format？

5 分

---

### 3. `VkAttachmentDescription` 中这些参数分别控制什么？

```cpp
colorAttachment.format = swapChainImageFormat;
colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

8 分

---

### 4. RenderPass、Subpass、Framebuffer 三者分别负责什么？

6 分

---

### 5. 为什么 Vulkan 创建 Debug Messenger 时，有时候要用 `vkGetInstanceProcAddr` 获取函数指针？

5 分

---

### 6. `primitiveRestartEnable` 是什么？它和“顶点复用”是一回事吗？

6 分

---

## 四、代码理解题 20 分

### 1. 解释下面 Viewport 参数的含义。

```cpp
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = static_cast<float>(swapChainExtent.width);
viewport.height = static_cast<float>(swapChainExtent.height);
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

5 分

---

### 2. 下面这段代码为什么要检查 `func != nullptr`？

```cpp
auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
);

if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

return VK_ERROR_EXTENSION_NOT_PRESENT;
```

5 分

---

### 3. 解释这几个字段：

```cpp
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 1;
```

5 分

---

### 4. 解释这段代码的意义：

```cpp
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
createInfo.queueFamilyIndexCount = 0;
createInfo.pQueueFamilyIndices = nullptr;
```

5 分

---

## 五、综合设计题 10 分

现在你已经画出了一个三角形。假设下一步你想画一个模型，而不是硬编码三角形。

请回答：

1. 需要新增哪些 GPU 资源？
2. 顶点数据和索引数据分别放在哪里？
3. Pipeline 是否一定要重建？
4. 如果窗口尺寸改变，哪些对象通常需要重建？
5. 如果只是替换模型数据，Swapchain 是否需要重建？
