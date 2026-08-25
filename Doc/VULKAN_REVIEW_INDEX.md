# LearnVulkan 复习资料总索引

> 基线：`RAII_Base` 分支，提交 `13da101`（2026-08-06）。
>
> 核心版本：项目请求 Vulkan 1.3；官方资料链接指向 Khronos 当前最新版，并在涉及 1.3/1.4 差异时明确标注。

## 这套资料包含什么

1. [VULKAN_CORE_REVIEW.md](VULKAN_CORE_REVIEW.md)：主复习讲义。以 Vulkan 为核心，把初始化、资源、管线、描述符、命令、同步、交换链重建与 RAII 串成完整心智模型，并映射到当前代码。
2. [VULKAN_REVIEW_FLASHCARDS.md](VULKAN_REVIEW_FLASHCARDS.md)：速记卡与自测题。适合遮住答案口述，也可用于面试式复盘。
3. [VULKAN_FENCE_SEMAPHORE_NOTES.md](VULKAN_FENCE_SEMAPHORE_NOTES.md)：同步专题长文，重点是 Fence、Binary Semaphore、Frame Slot `F` 与 Swapchain Image `I`。
4. [HELLO_WORLD_RENDERER_REVIEW.md](HELLO_WORLD_RENDERER_REVIEW.md)：旧版代码审阅。它记录了架构重构前的关键问题；阅读时结合下文“旧问题当前状态”。
5. [Vulkan_Hello_Triangle_Quiz.md](Vulkan_Hello_Triangle_Quiz.md) / [参考答案](Vulkan_Hello_Triangle_Quiz_Answers.md)：Hello Triangle 基础测试。
6. [VULKAN_TUTORIAL_CN.md](VULKAN_TUTORIAL_CN.md)：最初的逐 API 教程，适合查初始化字段，不应代替当前架构讲义。
7. [OFFSCREEN_PRESENT_REFACTOR_PLAN.md](OFFSCREEN_PRESENT_REFACTOR_PLAN.md)：Offscreen RenderTarget + Present Pass 的进行中改造计划、色彩契约与验收清单。

## 先记住项目已经走到哪里

项目并非停留在 Hello Triangle。当前主路径已经是：

```text
GLFW Window
  -> Instance + Validation + Debug Messenger + Surface
  -> Physical Device + Logical Device + Graphics/Present Queue
  -> CPU Asset（GLB / Shader / Texture / Material / Model）
  -> GPU Asset Cache（Vertex/Index Buffer、Image、Sampler、Material Descriptor）
  -> SwapchainResources（每张交换链图像的 Depth、Framebuffer、Present Semaphore）
  -> FrameContext[F]（Command Pool/Buffer、Acquire Semaphore、Frame Fence）
  -> FrameDataResources[F]（Camera UBO、Object SSBO、Frame Descriptor Set）
  -> SceneRenderExtractor（层级场景 -> 扁平 RenderObject 列表）
  -> 每帧 Record -> Submit -> Present
```

代码演进可概括为：

| 阶段 | 已完成的学习内容 |
|---|---|
| Hello Triangle | Instance、Surface、Device、Swapchain、Render Pass、Pipeline、Command Buffer、同步 |
| 模型绘制 | GLB、顶点/索引缓冲、`vkCmdDrawIndexed`、相机、深度 |
| 帧资源 | 双帧并行、`F`/`I` 分离、UBO/Descriptor、交换链重建 |
| RAII 重构 | Buffer、Image、Swapchain、Device、Pipeline、同步对象等独占所有权与失败回滚 |
| 上传路径 | Staging Buffer、Device-local Buffer/Image、布局转换 |
| 场景与资产 | AssetManager、RenderAssetCache、Scene、模型层级展开、多 Mesh/多 Submesh |
| 材质与纹理 | 材质 UBO、独立 sampled image/sampler descriptors、PBR 风格 Fragment Shader |
| 当前帧数据 | Camera UBO + Object SSBO + 每 Draw Push Constants 索引 |

## 旧审阅问题的当前状态

`HELLO_WORLD_RENDERER_REVIEW.md` 基于较早提交，其中部分结论已经被后续实现修复。

| 旧问题 | 当前状态 | 当前落点 |
|---|---|---|
| Present wait semaphore 按 Frame Slot 复用 | 已修复：按 Swapchain Image `I` 持有 | `SwapchainResources::ImageResources::renderFinished` |
| 所有 Framebuffer 共用一张 Depth | 已修复：每个 `I` 一张 Depth | `SwapchainResources::makeImageResources` |
| 原始句柄异常路径易泄漏 | 大部分已用 RAII/事务式替换收口 | `VulkanContext`、`Buffer`、`Image`、`GraphicsPipeline` 等 |
| Command Buffer 静态按 Swapchain Image 录制 | 已改为每个 Frame Slot 每帧重录 | `FrameContext` + `VulkanRenderer::recordCommandBuffer` |
| 上传每次 `vkQueueWaitIdle` | 仍存在，简单但会串行化 | `UploadContext` |
| UBO 每帧 Map/Unmap | 仍存在，可改持久映射 | `PerFrameBuffer::sync` |
| Resize 使用 `vkDeviceWaitIdle` | 仍存在，正确但偏保守 | `VulkanRenderer::resize` |
| GPU 偏好逻辑与命名相反 | 仍存在 | `Device::create` |

## 三轮复习法

### 第一轮：只建地图

目标是能脱离代码说出：

- Vulkan 为什么把对象、内存、执行与同步显式暴露给应用。
- `Instance -> PhysicalDevice -> Device -> Queue` 的关系。
- `Surface -> Swapchain -> Image -> ImageView -> Framebuffer` 的关系。
- `Shader -> DescriptorSetLayout/PushConstantRange -> PipelineLayout -> Pipeline` 的关系。
- `CommandPool -> CommandBuffer -> Queue Submit` 的关系。
- `Acquire -> Submit -> Present` 的两条 Semaphore 边和一条 Fence 边。

建议：先读主讲义第 1～5、8～10 章，再做速记卡 1～30。

### 第二轮：把地图投影到当前代码

目标是能沿着 `App::initVulkan` 和 `VulkanRenderer::render` 逐行解释：

- 对象由谁创建、由谁拥有、依赖谁、何时销毁。
- 每个数组应该按 `F` 还是 `I` 索引。
- 每个 Descriptor binding 在 HLSL 中对应什么。
- 每次 Draw 前绑定了哪些状态，Push Constants 选择了哪条 Object 数据。

建议：读主讲义第 6～14 章，并用 RenderDoc 打开仓库中的 `.rdc` 抓帧核对。

### 第三轮：从“能跑”走向“能设计”

目标是能独立设计下一版：

- 用 Synchronization2 表达 Barrier/Submit。
- 用 Dynamic Rendering 替代单 Subpass Render Pass（不是强制，但适合作为 Vulkan 1.3 路线）。
- 用持久映射、批量 Upload、Fence/Timeline Semaphore 避免 Queue Idle。
- 用 VMA 或自建子分配器避免“一资源一次 `VkDeviceMemory`”。
- 设计材质/纹理 Descriptor 策略、Pipeline Cache、Render Graph 或 GPU-driven 路线。

建议：读主讲义第 15～18 章，完成速记卡中的设计题和实验题。

## 官方文档阅读顺序

优先级从高到低：

1. [Vulkan Guide](https://docs.vulkan.org/guide/latest/)：先建立正确直觉，尤其同步、版本、常见坑。
2. [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/)：按可运行路线理解完整流程；新版教程偏向 Dynamic Rendering。
3. [Vulkan Specification](https://docs.vulkan.org/spec/latest/)：查精确定义、Valid Usage、对象生命周期与同步范围。
4. [Vulkan Samples](https://docs.vulkan.org/samples/latest/)：查更接近工程实践的实现与性能建议。
5. [Vulkan Registry](https://registry.khronos.org/vulkan/)：查最新规范、扩展和 Reference Pages。

不要试图从第一页顺读完整 Specification。更有效的方式是：先在 Guide/Tutorial 形成模型，再带着一个具体对象、命令或 VUID 回到 Spec。

## 当前验证结果

在此复习资料整理时已验证：

- `cmake --build --preset debug-vs`：通过。
- `VulkanApp.exe --asset-test`：通过；导入 33 张纹理、16 个材质、15 个 Mesh、50 个节点。
- `VulkanApp.exe --render-test`：通过；Validation 开启，连续绘制 3 帧，每帧提取 49 个 Submesh Draw。
- 运行输出中只有本机重复 Layer 注册警告，没有 Vulkan Valid Usage 错误。
