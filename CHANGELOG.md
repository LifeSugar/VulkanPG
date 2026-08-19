# Changelog

本文件记录项目中值得关注的功能、架构调整与问题修复。

项目目前没有正式版本号，因此以完成日期组织里程碑；正在开发但尚未提交的内容记录在 `Unreleased` 下。

## Unreleased

### Added

- 添加强类型 `RenderLayer` 与可组合的 `LayerMask`。
- `SceneNode` 现在保存实例级渲染 Layer Mask，并支持运行时修改。
- `Camera` 现在保存 Culling Mask，为后续按 View 构建 RenderList 和 Layer 剔除提供数据。
- 添加后端无关的 `MaterialRenderState`，包含 Opaque/Transparent、Alpha Clip、Double-Sided 与 Depth Test/Write/Compare 设置。
- 添加强类型 `MaterialKey` 与后端无关的 `PipelineVariantKey`，对 Material Template、Shader Feature、Blend、Cull 和 Depth State 提供规范化映射及逐字段确定性比较。
- `MaterialAsset` 和 `GpuMaterial` 现在保存经过校验的 Material Render State，为后续 RenderList 分类和 Pipeline Variant Key 提供数据。
- 添加携带 Mesh、Submesh、Material、Layer Mask 与 Object GPU Data 的 `RenderCandidate`。
- 添加拥有 Opaque/Transparent 有序列表的 `RenderList`，并让 `RenderFrame` 与现有 Scene Draw 路径改为使用该结构。
- 添加后端无关的 `Aabb`，在 `MeshAsset` 创建时从有限顶点位置生成 Mesh Local AABB，并将其复制到 GPU `Mesh` 供后续 Candidate World Bounds 与视锥剔除使用。
- `RenderCandidate` 现在通过实例 World Transform 将 Mesh Local AABB 转换为保守 World AABB，支持旋转、非均匀缩放、负缩放与仿射剪切。
- 添加从 Vulkan Zero-to-One ViewProjection 提取六个归一化平面的 `Frustum`，并使用 AABB 中心投影半径法测试相交。
- 添加支持 Camera/View LayerMask、Frustum 开关和 SceneObject Bounds Culling 开关的 `CullingSystem`，输出可复用的可见 Candidate 索引与剔除统计。
- `SceneRenderExtractor` 现在只生成原始 Candidate；`RenderListBuilder` 根据 CullingResults 和 Material Surface Type 完成列表分类与排序。
- `RenderCandidate` 改为仅持有 Mesh/Material Asset Handle，彻底解除 Scene 提取与 GPU `RenderAssetCache` 的依赖；新增执行态 `RenderItem`，由 `RenderListBuilder` 集中解析 GPU 资源。
- 完善 `RenderView` 的 CPU 视图快照，新增稳定 `RenderViewId` 与明确的 `gpuDataRevision`；Renderer 现在用二者识别 GPU Camera Data 版本，同时由 `PerFrameBuffer` 保证每个 Frame Slot 至少同步一次最新快照。
- 定义 `RenderQueue` 的 Opaque、AlphaClip、Transparent 顺序，并完善执行态 `RenderItem`：保存资源、Candidate/Object 索引、Queue、View Depth、MaterialKey 与 PipelineVariantKey；对象矩阵改由 `RenderList` 集中存储，使后续排序无需搬动 GPU Object Data。
- 添加确定性的 Opaque/Transparent RenderItem Comparator：Opaque 参考 Unity 2019.4，按 IEEE-754 浮点高 8 bit 进行粗粒度前到后深度分桶，再在同桶内依次按 Pipeline、Material、Mesh 合并状态；Transparent 严格远到近，并在等深度时采用相同的状态排序与 CandidateIndex 平局规则。
- `VulkanRenderer` 现在直接使用 `RenderItem::objectIndex` 选择 Object GPU Data，并缓存当前 Mesh 与 Material DescriptorSet，跳过连续 RenderItem 间的冗余 Bind。

## 2026-08-18 - Offscreen Scene Pass 与 Present Pass

### Added

- 添加通用 `RenderTarget` 抽象，统一持有 Offscreen Attachment、ImageView 与 Framebuffer。
- 为每个 Frame Slot 创建线性 HDR Scene Color 与 Depth Render Target。
- 添加全屏三角形 Present Pipeline、Present Shader 和独立采样 Descriptor。
- 添加曝光、ACES Tone Mapping，以及根据 Swapchain 格式选择的 sRGB 输出编码路径。
- 添加通用 `Sampler` RAII 封装。
- 添加 Offscreen/Present 渲染路径的设计与验证文档。

### Changed

- 渲染命令改为先执行 Scene Pass，再将 Scene Color 转换为 Shader Read 布局并执行 Present Pass。
- `GraphicsPipeline::CreateInfo` 支持配置拓扑、深度测试、深度写入、比较操作、Cull Mode、Front Face 与采样数。
- Swapchain 保存完整 Surface Format 信息，用于区分格式和 Color Space。
- Scene Shader 只输出线性场景颜色，Tone Mapping 与最终输出编码统一由 Present Shader 负责。

### Verified

- Present Shader 编译并通过 SPIR-V 校验。
- Debug + Validation Layer 隐藏窗口三帧 Smoke Test 无 Vulkan VUID 错误。

Commit: `a54d017`

## 2026-08-14 - GPU 选择修复

### Fixed

- 修复 `preferIntegratedGPU` 配置未被正确遵守的问题。

Commit: `06a6fbf`

## 2026-08-06 - Camera Buffer 与 Draw Push Constants

### Changed

- Camera 数据由 Storage Buffer 调整为 Uniform Buffer。
- 使用 Push Constants 传递每次 Draw 所需的 Camera/Object 索引。
- 固定 CPU 与 Shader 之间的 Camera 数据布局与容量约束。

Commit: `13da101`

## 2026-08-04 - Asset、Scene 与 Rendering 解耦

### Added

- 添加与来源格式无关的 Texture、Material、Mesh、Shader 和 Model Asset。
- 添加 `AssetManager`、类型安全 Asset Handle 与 Asset Registry。
- 添加 GLB Texture、Material、Mesh、Model Importer 和 WIC 图片解码路径。
- 添加 `RenderAssetCache`，负责从 CPU Asset 创建并缓存 GPU Mesh、Material 与 Texture。
- 添加独立 Scene 层，以及将 Scene 拍平为 `RenderFrame` 的 `SceneRenderExtractor`。
- 添加 `ABeautifulGame.glb` 作为完整材质和场景渲染验证资源。

### Changed

- GLB 解析、CPU Asset、GPU Resource 和逐帧渲染输入之间建立明确边界。
- Renderer 改为消费提取后的 `RenderFrame`，不再直接依赖 GLB 数据结构。

Commit: `c4d044e`

## 2026-07-27 - Vulkan Resource RAII

### Added

- 添加 `Fence`、`Semaphore`、`FrameContext`、`Framebuffer`、`RenderPass` 和 `SwapchainResources` RAII 封装。

### Changed

- Swapchain 创建、同步、Framebuffer 和资源销毁逻辑迁移到职责明确的资源类中。
- 加强资源创建失败时的清理与参数校验。

Commit: `fcc598a`

## 2026-07-20 - Frame Resources 与同步

### Added

- 添加按 Frame Slot 管理的 Frame Resources。

### Changed

- 重构 Acquire、Submit、Present 和 In-flight Fence 的同步流程。

Commit: `ab14176`

## 2026-06-17 - GLB Loader

### Added

- 添加 GLB 模型加载基础能力。
- 添加从 Hello Triangle 演进到模型渲染器的初始路线图。

Commit: `5f5d40d`

## 2026-05-19 - 项目初始化

### Added

- 初始化 LearnVulkan 项目。

Commit: `3d39a37`
