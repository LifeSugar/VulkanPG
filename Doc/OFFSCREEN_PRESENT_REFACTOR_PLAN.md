# Offscreen + Present 渲染路径改造计划

> 状态：进行中  
> 最近更新：2026-08-18  
> 目标：把当前“场景直接渲染到 Swapchain”的路径改成“场景渲染到通用 `RenderTarget`，再由 Present Shader 合成到 Swapchain”。

## 1. 目标与非目标

本轮目标：

- 引入通用 `RenderTarget`，不创建绑定具体业务语义的 `SceneRenderTargets` 类。
- Offscreen Color/Depth 按 Frame Slot `F` 持有。
- Swapchain Framebuffer 按 Swapchain Image `I` 持有。
- Scene Pass 输出线性 HDR；Present Pass 统一负责曝光、Tone Mapping 和 SDR 输出编码。
- 第一版保持一个 Command Buffer、一次 Queue Submit，先保证资源、布局和色彩正确。
- 保留后续扩展 MSAA、多颜色附件、后处理链和不同内部渲染分辨率的空间。

本轮不做：

- 不切换到 Dynamic Rendering。
- 不实现 Render Graph。
- 不在首次接入时拆分 Scene/Present 两次提交。
- 不实现 HDR10、scRGB 等 HDR 显示输出。
- 不把 Swapchain Image 包装成 `RenderTarget`；Swapchain Image 所有权和生命周期继续由 `SwapchainResources` 管理。

## 2. 最终资源关系

```text
Frame Slot F
├── FrameContext[F]
│   ├── CommandPool/CommandBuffer
│   ├── imageAvailable Semaphore
│   └── inFlight Fence
└── RenderTarget[F]
    ├── Attachment 0: Offscreen Color
    ├── Attachment 1: Depth
    └── Scene Framebuffer

Swapchain Image I
└── SwapchainResources::ImageResources[I]
    ├── Swapchain ImageView（非 Image 所有者）
    ├── Present Framebuffer
    ├── renderFinished Semaphore
    └── imageInFlight Fence 引用
```

`F` 与 `I` 没有固定对应关系。每帧在 `recordCommandBuffer(F, I)` 中动态组合：

```text
Scene Pass:   RenderTarget[F]
Present Pass: sample RenderTarget[F].Color -> SwapchainFramebuffer[I]
```

若 `framesInFlight=2`、`swapchainImageCount=3`，资源数量就是 2 个 Offscreen `RenderTarget` 和 3 个 Present Framebuffer。

## 3. RenderTarget 抽象边界

`RenderTarget` 表示一个具体的 Offscreen Framebuffer，而不是一组 Frame Slot，也不是一个 Render Pass。

它拥有：

- 有序的 attachment `Image`。
- 对应的 `ImageView`。
- 由这些 views 创建的 `Framebuffer`。
- extent 和 attachment 创建元数据。

它不拥有：

- `VkRenderPass`。Render Pass 作为非拥有的兼容性契约传入，并且必须活得比 Framebuffer 久。
- Descriptor Set、Sampler 和 Graphics Pipeline。这些属于使用 RenderTarget 的 Pass。
- Swapchain Image。WSI Image 继续由 Swapchain 管理。
- Image layout 运行时状态。布局转换由 Render Pass 和命令录制负责。

当前接口允许调用者按 attachment index 获取 Image/ImageView：

```cpp
constexpr std::size_t kSceneColorAttachment = 0;
constexpr std::size_t kSceneDepthAttachment = 1;

target.image(kSceneColorAttachment);
target.imageView(kSceneColorAttachment);
target.framebuffer();
```

attachment 顺序必须与创建 Framebuffer 所使用的 Render Pass attachment 顺序一致。

## 4. 色彩空间契约

目标 SDR 路径：

```text
材质纹理（sRGB texture 自动解码）
  -> 线性场景光照
  -> RGBA16F Offscreen Color
  -> Exposure
  -> ACES Tone Mapping
  -> 线性显示色
  -> sRGB 输出编码
  -> Present
```

Scene Fragment Shader 必须输出线性场景色，不允许执行最终 Tone Mapping 或 Gamma：

```hlsl
return float4(linearSceneColor, alpha);
```

Present Shader 的输出约定：

- `VK_FORMAT_*_SRGB` Swapchain：`outputTransferFunction=0`，shader 输出线性显示色，由 attachment 自动编码。
- `VK_FORMAT_*_UNORM + VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`：`outputTransferFunction=1`，shader 显式执行标准分段 sRGB 编码。
- 不允许同时执行 shader sRGB 编码和 SRGB attachment 编码。
- 其他输出色域暂时报告不支持，不能静默套用 sRGB。

Swapchain 需要保存完整的 `VkSurfaceFormatKHR` 信息，至少同时保存 `VkFormat` 和 `VkColorSpaceKHR`。

## 5. 分阶段实施清单

### 阶段 A：基础资源抽象

- [x] 添加通用 `RenderTarget` 类。
- [x] `RenderTarget` 支持任意数量、有序排列的自有 attachments。
- [x] `RenderTarget` 事务式创建：所有 Image/View/Framebuffer 创建成功后才替换旧资源。
- [x] `RenderTarget::reset()` 保证先销毁 Framebuffer，再销毁 ImageView/Image。
- [x] `Image` 增加可选 `VkSampleCountFlagBits`，默认保持 `VK_SAMPLE_COUNT_1_BIT`。
- [ ] 为 `RenderTarget` 添加最小创建/移动/越界访问测试，或在集成时通过 Validation Layer 覆盖。

落点：

- `head/RenderTarget.h`
- `src/RenderTarget.cpp`
- `head/Image.h`
- `src/Image.cpp`

### 阶段 B：Present Shader 与色彩接口

- [x] 添加无顶点缓冲的全屏三角形 `present.vert.hlsl`。
- [x] 添加曝光、ACES、可选显式 sRGB 编码的 `present.frag.hlsl`。
- [x] 编译并通过 `spirv-val`。
- [x] 在 CPU 侧添加 16 字节 `PresentPushConstants`，并用 `static_assert` 固定 ABI。
- [x] `AppAssets` 加载 Present Vertex/Fragment SPIR-V。
- [x] Swapchain 保存并公开 color space。

落点：

- `Assets/shaders/present.vert.hlsl`
- `Assets/shaders/present.frag.hlsl`
- `Assets/shaders/compile_shaders.ps1`
- `src/AppAssets.cpp`
- `head/Swapchain.h`
- `src/Swapchain.cpp`

### 阶段 C：Pipeline 状态通用化

- [x] 给 `GraphicsPipeline::CreateInfo` 添加 primitive topology 配置。
- [x] 添加 depth test/write/compare 配置。
- [x] 添加 cull mode/front face 配置。
- [x] 添加 rasterization sample count 配置。
- [x] 默认值保持当前 Scene Pipeline 行为，确保现有画面不变。
- [x] Present Pipeline 使用空 Vertex Input、关闭 Depth、关闭 Cull。

预期配置：

```cpp
presentPipelineInfo.vertexBindings = {};
presentPipelineInfo.vertexAttributes = {};
presentPipelineInfo.cullMode = VK_CULL_MODE_NONE;
presentPipelineInfo.depthTestEnable = false;
presentPipelineInfo.depthWriteEnable = false;
```

落点：

- `head/GraphicsPipeline.h`
- `src/GraphicsPipeline.cpp`

### 阶段 D：创建 Offscreen RenderTarget[F]

- [x] 新建 Scene Render Pass：Attachment 0 为 Color，Attachment 1 为 Depth。
- [x] 检查 `VK_FORMAT_R16G16B16A16_SFLOAT` 是否支持 `COLOR_ATTACHMENT | SAMPLED`。
- [x] 按 `framesInFlight` 创建 `std::vector<RenderTarget>`。
- [x] Color attachment 使用 `COLOR_ATTACHMENT | SAMPLED`。
- [x] Depth attachment 使用 `DEPTH_STENCIL_ATTACHMENT`。
- [x] 明确 attachment index 常量，禁止散落 magic number。
- [x] Resize 时按 Swapchain 实际 extent 重建 RenderTarget[F]。

建议创建参数：

```cpp
RenderTarget::CreateInfo targetInfo{};
targetInfo.renderPass = sceneRenderPass.get();
targetInfo.extent = renderExtent;
targetInfo.attachments = {
    {
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    },
    {
        depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        depthAspectMask
    }
};
```

第一版可由 `VulkanRenderer` 直接持有：

```cpp
RenderPass sceneRenderPass_;
std::vector<RenderTarget> sceneRenderTargets_;
```

当 Pass 数量继续增长时，再提取 `ScenePass`/`PresentPass` 对象；当前不提前增加层级。

### 阶段 E：Present Descriptor 与 Pipeline

- [x] 创建 set 0/binding 0 `SAMPLED_IMAGE` layout。
- [x] 创建 set 0/binding 1 `SAMPLER` layout。
- [x] 创建一个线性过滤、Clamp-To-Edge 的 Present Sampler。
- [x] Descriptor Pool 按 `framesInFlight` 分配 sampled image 与 sampler 数量。
- [x] 每个 DescriptorSet[F] 指向 `RenderTarget[F]` 的 Color ImageView。
- [x] 创建绑定 Swapchain Present Render Pass 的 Present Pipeline。
- [x] Push Constant stage 仅设置 `VK_SHADER_STAGE_FRAGMENT_BIT`。

Descriptor 不放入 `RenderTarget`，因为同一 attachment 可以被多个 Pass 以不同 descriptor layout 使用。

### 阶段 F：SwapchainResources 收缩为 Present 资源

- [ ] 从 `SwapchainResources::ImageResources` 移除 Depth Image/View。
- [ ] Swapchain Render Pass 改为单 Color Attachment、无 Depth。
- [ ] Present Framebuffer[I] 只绑定 Swapchain ImageView[I]。
- [ ] 保留 `renderFinished[I]` 与 `imageInFlight[I]`。
- [ ] 将访问器命名明确为 Present 语义，例如 `presentFramebuffer(I)` / `presentRenderPass()`。

这一阶段完成后，Depth 的生命周期完全从 Swapchain Image `I` 转移到 Offscreen RenderTarget `F`。

### 阶段 G：切换命令录制

- [x] `recordCommandBuffer` 同时接收 `frameIndex` 和 `imageIndex`。
- [x] Scene Pass 使用 `sceneRenderTargets_[frameIndex]`。
- [x] Scene Pass 后加入 Color Attachment Write -> Fragment Shader Read 依赖。
- [x] Present Pass 使用 Swapchain Framebuffer `[imageIndex]`。
- [x] Present Pass 绑定 DescriptorSet `[frameIndex]`。
- [x] 使用 `vkCmdDraw(commandBuffer, 3, 1, 0, 0)` 绘制全屏三角形。
- [x] Scene Shader 移除已有 Tone Mapping 与 `pow(1/2.2)`。

第一版命令顺序：

```text
Begin CommandBuffer
  Begin Scene RenderPass(RenderTarget[F])
    Draw Scene
  End Scene RenderPass

  Color write -> shader read dependency/layout transition

  Begin Present RenderPass(SwapchainFramebuffer[I])
    Bind Present Pipeline
    Bind Present DescriptorSet[F]
    Push PresentPushConstants
    Draw 3 vertices
  End Present RenderPass
End CommandBuffer
```

布局策略只能选一套并保持一致：

1. Scene Color Render Pass 最终布局设为 `SHADER_READ_ONLY_OPTIMAL`，并设置正确的 outgoing subpass dependency；或
2. 最终保持 `COLOR_ATTACHMENT_OPTIMAL`，Render Pass 后显式 barrier 转为 `SHADER_READ_ONLY_OPTIMAL`。

第一版建议采用显式 barrier，便于 RenderDoc 中检查和学习同步关系。

### 阶段 H：Resize、生命周期与失败回滚

- [ ] Resize 时重建 Swapchain Present Framebuffer[I]。
- [ ] Resize 时按新 extent 重建 RenderTarget[F]。
- [x] RenderTarget 重建后更新 Present DescriptorSet[F]。
- [ ] 仅 extent 变化时不重建使用动态 viewport/scissor 的 Pipeline。
- [ ] Swapchain format 变化时重建 Present Render Pass/Pipeline。
- [ ] Scene Color/Depth format 变化时重建 Scene Render Pass/Pipeline。
- [ ] 检查成员声明顺序和显式 reset 顺序，确保 Framebuffer 先于所引用的 views/render pass 销毁。

第一版继续使用 `device.waitIdle()` 完成安全 Resize；延迟销毁不在本轮范围内。

### 阶段 I：正确性验证

- [x] Debug + Validation Layer 隐藏窗口三帧 smoke test 无 Vulkan VUID 错误。
- [ ] 2 个 Frame Slot + 3 个 Swapchain Image 连续运行。
- [ ] 连续拖动 Resize、最小化、恢复正常。
- [ ] RenderDoc 中能看到 Scene 与 Present 两个 Render Pass。
- [ ] Offscreen Color 格式为 `R16G16B16A16_SFLOAT`。
- [ ] Present Pass 采样的 attachment 与当前 `F` 一致。
- [ ] Present Framebuffer 与 Acquire 返回的 `I` 一致。
- [ ] Present 前 Color layout 为 `SHADER_READ_ONLY_OPTIMAL`。
- [ ] Swapchain Image 最终 layout 为 `PRESENT_SRC_KHR`。
- [ ] SRGB Swapchain 使用 `outputTransferFunction=0`，确认没有双重 Gamma。
- [ ] Tone Mapping 开关和 Exposure 调整结果符合预期。

## 6. 基础版 render() 轮廓

```cpp
FrameContext& frame = frameContexts_[F];
frame.waitUntilReusable();

acquireSwapchainImage(frame.imageAvailable(), I);
waitUntilSwapchainImageReusable(I);

updateFrameData(F, frameData);
recordCommandBuffer(frame.commandBuffer(), F, I, frameData);

submit(
    frame.commandBuffer(),
    frame.imageAvailable(),
    swapchainResources_.renderFinished(I),
    frame.inFlightFence());

present(I);
currentFrame_ = (currentFrame_ + 1) % framesInFlight;
```

安全性来源：

- `Fence[F]` 保证重新写入 `RenderTarget[F]` 前，上一次 Present Shader 已采样完成。
- Acquire 与 `imageInFlight[I]` 保证写入 Swapchain Image[I] 时它可用。
- Scene Color 的 pipeline barrier/subpass dependency 保证 Color Attachment 写入对 Present Fragment Shader 可见。

## 7. 后续优化（不阻塞本轮）

基础路径稳定后，再单独处理：

- [ ] Scene/Present 分离 Command Buffer。
- [ ] Scene Submit 不等待 `imageAvailable`，减少等待 Swapchain 造成的 bubble。
- [ ] 内部渲染分辨率与窗口分辨率分离。
- [ ] MSAA Color/Depth + Resolve Attachment。
- [ ] 多级后处理 RenderTarget ping-pong。
- [ ] Descriptor Indexing 或统一 Pass Resource 管理。
- [ ] Dynamic Rendering。
- [ ] Render Graph 与自动 layout/synchronization 推导。

## 8. 文档维护规则

每次相关改动都应同步更新本文：

1. 完成一个原子步骤后勾选对应 checklist。
2. 若代码结构与本文不同，先更新“抽象边界”和“最终资源关系”，避免只修改零散任务项。
3. 每次更新修改顶部“最近更新”日期。
4. 新增色彩空间或同步策略时，必须写明输入/输出契约和验证方式。
5. 阶段 G 完成后，同步更新 `VULKAN_CORE_REVIEW.md` 与 `VULKAN_REVIEW_INDEX.md` 中仍描述“Swapchain 每张 Image 自带 Depth”的内容。
