# Vulkan Hello Triangle 阶段测试题参考答案

这份答案用于对照复盘。建议先完成试题卷，再打开本文件查缺补漏。

---

## 一、判断题答案

### 1. 错

`VkInstance` 更像是 Vulkan 应用层入口，负责连接 Vulkan Loader、扩展、验证层等。真正代表 GPU 能力的是 `VkPhysicalDevice`，真正用于发命令的是 `VkDevice` 和 Queue。

---

### 2. 对

`Surface` 是 Vulkan 和平台窗口系统之间的桥。比如 Windows 是 Win32 Surface，Linux 可能是 XCB / Wayland Surface，macOS 通过 MoltenVK 通常走 Metal Layer。

---

### 3. 错

反了。

`VkPhysicalDevice` 是物理 GPU。  
`VkDevice` 是从物理 GPU 创建出来的逻辑设备。

---

### 4. 对

QueueFamily 表示一组具有相同能力的队列。例如某个 QueueFamily 支持 Graphics，另一个可能只支持 Transfer。

---

### 5. 错

Swapchain Image 由 Swapchain 创建和管理。我们通过 `vkGetSwapchainImagesKHR` 拿到它们的句柄，而不是自己 `vkCreateImage`。

---

### 6. 对

Image 是真实图像资源。ImageView 描述“如何看待这张图”，例如格式、维度、mip、array layer、color/depth/stencil aspect。

---

### 7. 对

RenderPass 是渲染流程描述，不是真实图像。它描述 attachment 的格式、load/store、layout transition、subpass 如何使用它们。

---

### 8. 对

Framebuffer 把 RenderPass 里抽象的 attachment 绑定到具体的 ImageView 上。

---

### 9. 错

`VkShaderModule` 是 SPIR-V 字节码创建出的 shader 模块。它还不是最终针对某个 GPU 和 Pipeline 状态完全优化后的机器码。真正的驱动编译/优化通常发生在 Pipeline 创建阶段。

---

### 10. 对

Pipeline 是一整套渲染状态组合，包括 shader、vertex input、input assembly、viewport、rasterization、multisample、depth/stencil、color blend 等。

---

## 二、流程排序题答案

一个典型顺序是：

```text
C → F → G → E → I → B → H → J → A → D → K → L → M
```

也就是：

```text
创建 Instance
→ 创建 Surface
→ 选择 PhysicalDevice
→ 创建 LogicalDevice 和 Queue
→ 创建 Swapchain
→ 创建 Swapchain ImageViews
→ 创建 RenderPass
→ 创建 ShaderModule
→ 创建 Graphics Pipeline
→ 创建 Framebuffers
→ 创建 CommandBuffers
→ 创建 Sync Objects
→ 主循环 acquire / submit / present
```

注意：有些教程会把 ShaderModule 和 RenderPass 的创建顺序稍微调整，只要依赖关系正确即可。

---

## 三、简答题参考答案

### 1. 为什么先 Instance，再 Surface，再 PhysicalDevice？

因为 Vulkan 初始化是逐层收窄的。

`Instance` 是 Vulkan 程序入口。  
`Surface` 描述你要把图像呈现到哪个窗口系统。  
选择 `PhysicalDevice` 时，不仅要看 GPU 是否支持 graphics queue，还要看它是否支持把图像 present 到这个 Surface。

所以选择 GPU 时经常要检查：

```text
是否支持 graphics queue
是否支持 present queue
是否支持 swapchain extension
是否支持当前 surface 的 format / present mode / extent
```

如果没有 Surface，就无法判断这个 GPU 能不能把结果显示到当前窗口。

---

### 2. 为什么 ImageView 还要指定 format？

因为 Swapchain 的 format 是 Image 的存储格式，而 ImageView 的 format 是“这次如何解释这个 Image”。

在最简单的 Swapchain 场景里，二者通常相同。

但是 Vulkan 的设计允许同一张 Image 被不同 View 解释，例如：

```text
同一张 Image 的不同 mip
同一张 Image 的不同 array layer
同一张 Image 的 color aspect 或 depth aspect
某些兼容格式 reinterpret
```

所以即使通常写一样，ImageView 仍然要明确声明。

---

### 3. AttachmentDescription 参数解释

```cpp
colorAttachment.format = swapChainImageFormat;
```

表示这个 attachment 的像素格式必须和最终绑定的 ImageView 格式兼容。对于 Swapchain color attachment，通常就是 Swapchain format。

```cpp
colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
```

表示采样数。`1_BIT` 表示不开 MSAA。

```cpp
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
```

表示 RenderPass 开始时如何处理 attachment 原内容。`CLEAR` 表示清屏成指定 clear color。

```cpp
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
```

表示 RenderPass 结束时是否保存渲染结果。要 present 到屏幕，所以必须 `STORE`。

```cpp
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
```

表示 RenderPass 开始前不关心这张图原来的 layout 和内容。因为我们会 clear，所以旧内容无所谓。

```cpp
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

表示 RenderPass 结束后，Image 会转换到可以交给 Present Queue 显示的布局。

---

### 4. RenderPass、Subpass、Framebuffer 的职责

`RenderPass`：描述渲染流程结构。它关心有哪些 attachment、load/store 怎么做、layout 怎么变、subpass 如何读写 attachment。

`Subpass`：RenderPass 内部的一个阶段。它声明当前阶段会把哪些 attachment 当 color attachment、depth attachment、input attachment 等。

`Framebuffer`：把 RenderPass 中声明的抽象 attachment 绑定到真实的 ImageView。

一句话：

```text
RenderPass 描述规则
Subpass 描述阶段
Framebuffer 绑定真实图像
```

---

### 5. 为什么要用 vkGetInstanceProcAddr？

因为有些函数来自扩展，不一定被 Vulkan Loader 直接暴露成普通函数入口。

例如：

```cpp
vkCreateDebugUtilsMessengerEXT
```

这个函数属于 `VK_EXT_debug_utils` 扩展。如果扩展没有启用，或者当前环境不支持，函数指针可能是 `nullptr`。

所以需要：

```text
先查询函数地址
如果存在，就调用
如果不存在，就返回 VK_ERROR_EXTENSION_NOT_PRESENT
```

这也是 Vulkan 扩展机制的一部分。

---

### 6. primitiveRestartEnable 是什么？和顶点复用一样吗？

不是一回事。

顶点复用通常指 Indexed Drawing：

```text
多个三角形通过 index buffer 引用同一份 vertex buffer 中的顶点
```

例如：

```text
vertices: A B C D
indices:  0 1 2  2 1 3
```

这里顶点 1 和 2 被复用了。

`primitiveRestartEnable` 是给 strip / fan 这类拓扑用的。例如 `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP` 中，如果 index buffer 里出现特殊 restart index，就表示“当前 strip 结束，从下一个 index 开始一个新的 strip”。

例如：

```text
0, 1, 2, 3, RESTART, 4, 5, 6, 7
```

表示两个独立的 strip。

所以：

```text
Indexed Drawing = 顶点复用
Primitive Restart = 在一个 index buffer 中切断 strip
```

平时导入普通模型时，大多数是 triangle list，不太会主动用 triangle strip 和 primitive restart。

---

## 四、代码理解题参考答案

### 1. Viewport 参数解释

```cpp
viewport.x = 0.0f;
viewport.y = 0.0f;
```

表示 viewport 起点位置。在 Vulkan 默认 viewport transform 中，`x = 0, y = 0` 通常对应 framebuffer 左上区域的起点。

```cpp
viewport.width = static_cast<float>(swapChainExtent.width);
viewport.height = static_cast<float>(swapChainExtent.height);
```

表示 viewport 的宽高。这里让 viewport 覆盖整个 Swapchain image。

```cpp
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

表示深度范围映射。Vulkan NDC 的 z 范围是 `[0, 1]`，所以通常写 0 到 1。

注意：Vulkan 和 OpenGL 的屏幕坐标、NDC z 范围有差异。OpenGL NDC z 是 `[-1, 1]`，Vulkan NDC z 是 `[0, 1]`。

---

### 2. 为什么检查 `func != nullptr`？

因为这个函数可能不存在。

```cpp
vkCreateDebugUtilsMessengerEXT
```

不是 Vulkan core 1.0 的普通函数，而是扩展函数。只有在扩展可用并启用的情况下，`vkGetInstanceProcAddr` 才可能返回有效函数指针。

如果返回 `nullptr`，说明不能调用，否则会崩溃。

---

### 3. subresourceRange 解释

```cpp
aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
```

表示这个 View 看的是 color 部分。如果是深度图，可能是 `VK_IMAGE_ASPECT_DEPTH_BIT`。如果是 stencil，可能是 `VK_IMAGE_ASPECT_STENCIL_BIT`。

```cpp
baseMipLevel = 0;
levelCount = 1;
```

表示从第 0 级 mip 开始，只包含 1 个 mip level。

```cpp
baseArrayLayer = 0;
layerCount = 1;
```

表示从第 0 个 array layer 开始，只包含 1 层。普通 2D Swapchain image 一般就是一层。

---

### 4. Swapchain 创建字段解释

```cpp
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

表示 Swapchain image 会被用作 color attachment。也就是我们会在 RenderPass 中把它当颜色输出目标来写入。

```cpp
createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

表示图像只被一个 QueueFamily 独占使用。这通常性能最好。

```cpp
createInfo.queueFamilyIndexCount = 0;
createInfo.pQueueFamilyIndices = nullptr;
```

当使用 `VK_SHARING_MODE_EXCLUSIVE` 时，不需要提供多个 queue family index。

如果 graphics queue 和 present queue 属于不同 QueueFamily，可能会使用：

```cpp
VK_SHARING_MODE_CONCURRENT
```

这时就需要提供多个 QueueFamily index。

---

## 五、综合设计题参考答案

### 1. 画模型需要新增哪些 GPU 资源？

至少需要：

```text
Vertex Buffer
Vertex Buffer Memory
Index Buffer
Index Buffer Memory
```

通常还会逐渐加入：

```text
Uniform Buffer
Descriptor Set Layout
Descriptor Pool
Descriptor Set
Texture Image
Texture ImageView
Sampler
Depth Buffer
```

---

### 2. 顶点数据和索引数据分别放在哪里？

顶点数据放在 Vertex Buffer 中。  
索引数据放在 Index Buffer 中。

CPU 侧可能来自：

```text
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
```

GPU 侧最终进入：

```text
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
```

---

### 3. Pipeline 是否一定要重建？

不一定。

如果只是换模型数据，但顶点格式不变，例如仍然是：

```cpp
position + color
```

Pipeline 不需要重建。

但如果顶点输入布局变了，例如从：

```cpp
position + color
```

变成：

```cpp
position + normal + uv + tangent
```

那么 `VkPipelineVertexInputStateCreateInfo` 相关配置变了，Pipeline 通常需要重建。

---

### 4. 窗口尺寸改变，哪些对象通常需要重建？

通常需要重建 Swapchain 相关资源：

```text
Swapchain
Swapchain Images
Swapchain ImageViews
Framebuffers
CommandBuffers
```

如果 RenderPass 格式不变，RenderPass 不一定要重建。

如果 Pipeline 里的 viewport/scissor 是静态写死的，也可能需要重建 Pipeline。如果 viewport/scissor 设置成 dynamic state，则可以避免因为窗口尺寸变化重建 Pipeline。

---

### 5. 只是替换模型数据，Swapchain 是否需要重建？

不需要。

Swapchain 只和窗口呈现有关。替换模型数据只影响 vertex/index buffer，不影响 Swapchain。

---

## 下一阶段建议重点

从“硬编码三角形”进入“真正的 GPU 资源管理”，建议下一阶段学习：

```text
Vertex Buffer
Index Buffer
Staging Buffer
Device Local Memory
Uniform Buffer
Descriptor Set
Depth Buffer
Texture Sampling
```

最关键的断点是：

```text
VkBuffer 是什么？
VkDeviceMemory 是什么？
为什么 Buffer 和 Memory 要分开？
为什么需要 staging buffer？
为什么不能随便从 CPU 直接写 GPU 最快的显存？
Descriptor Set 到底解决了什么问题？
```
