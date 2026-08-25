# Vulkan 复习速记卡与自测题

使用方法：先只读“问”，用 30～90 秒口述答案；再展开“答”核对。不能只回答名词定义，还要说出当前项目中的对象归属或调用位置。

## A. 对象与初始化

### 1. `VkInstance` 是否是程序与 GPU 的直接连接？

答：不准确。Instance 是应用与 Vulkan Loader/实现交互的根对象，管理 Instance-level extensions/layers，并用于枚举 PhysicalDevice、创建 Surface 等。真正启用设备能力并拥有大多数资源的是 `VkDevice`。

### 2. `VkPhysicalDevice` 与 `VkDevice` 的区别？

答：PhysicalDevice 是实现暴露的物理设备能力句柄，用于查询 properties/features/limits/memory/queue/surface support，不由应用销毁。Device 是应用基于某个 PhysicalDevice 创建的逻辑设备，显式启用 queues、features 和 device extensions，拥有 Buffer/Image/Pipeline 等 device-level 对象。

### 3. 为什么选择 PhysicalDevice 时必须已经有 Surface？

答：Graphics 能力不等于能向目标窗口呈现。Present support 是“某 PhysicalDevice 的某 Queue Family 对某 Surface”的关系；还要查询该 Surface 的 formats、present modes、capabilities。

### 4. Queue Family 和 Queue 有什么区别？

答：Family 描述一组兼容 Queue 的能力与数量；Queue 是从 LogicalDevice 取得的实际异步提交入口。Command Pool 绑定 Family，不是绑定某个 Queue handle。

### 5. Graphics Queue 与 Present Queue 一定相同吗？

答：不一定。它们可能来自同一 Family/同一 Queue，也可能不同。不同 Queue 默认无执行顺序；Swapchain Image 还涉及 sharing mode 或 ownership transfer。

### 6. Instance extension 与 Device extension 为什么不能混用？

答：它们属于不同功能层级。Surface/debug utils 是 Instance-level；Swapchain 是 Device-level。启用位置、查询方式和入口函数分发层次都不同。

### 7. 为什么 Debug Messenger 的创建常通过 `vkGetInstanceProcAddr`？

答：`VK_EXT_debug_utils` 是 Instance extension，扩展命令不保证作为核心静态入口存在；启用 extension 后从 Instance dispatch 获取函数指针。

### 8. 当前项目请求 Vulkan 1.3，是否说明系统一定支持 1.3？

答：不说明。健壮程序应查询 Loader instance version 和 PhysicalDevice API version，并检查 features/extensions。头文件出现 1.3/1.4 符号也不代表运行时可用。

## B. Swapchain 与呈现

### 9. Swapchain Image 是谁创建和销毁的？

答：随 Swapchain 由实现/WSI 提供，应用通过 `vkGetSwapchainImagesKHR` 获取句柄，不单独 `vkDestroyImage`。应用自己创建并销毁这些 Image 的 ImageViews、Framebuffers 等 dependents。

### 10. Image 与 ImageView 的区别？

答：Image 是有格式/尺寸/用途/内存绑定的资源；ImageView 选择 view type、format interpretation、component swizzle 与 aspect/mip/layer subresource range，是 Framebuffer/Descriptor 常用的绑定对象。

### 11. `minImageCount + 1` 为什么不是实际图像数？

答：它只是请求值，还要 clamp 到 capabilities；实现可返回满足规范的实际数量，所以创建后必须再次查询。

### 12. `FIFO` 与 `MAILBOX` 的核心差别？

答：FIFO 是规范保证支持的垂直同步队列；MAILBOX 在等待显示时保留更新的帧、替换较旧待显示帧，通常低延迟但需要更多缓冲。当前项目优先 MAILBOX，否则 FIFO。

### 13. `VK_ERROR_OUT_OF_DATE_KHR` 与 `VK_SUBOPTIMAL_KHR`？

答：OUT_OF_DATE 表示不能继续用当前 Swapchain；SUBOPTIMAL 表示仍可用但不匹配最佳 Surface 属性。两者都应触发/安排重建。

### 14. 为什么最小化时不能立刻创建 Swapchain？

答：Framebuffer extent 可能为 0x0，而 Swapchain/attachments 要求有效非零尺寸。当前项目等待窗口事件直到 extent 恢复非零。

### 15. Resize 后哪些资源通常不需要重建？

答：Device、Queues、FrameContext[F]、Mesh/Texture/Material、与尺寸无关的 Descriptor resources。Camera projection 的 aspect 要更新。Pipeline 若 Viewport/Scissor 动态且 formats/rendering compatibility 不变，也可保留。

## C. 资源与内存

### 16. 为什么 `vkCreateBuffer` 后还不能直接使用？

答：Buffer 只是资源对象；还要查询 memory requirements、选择兼容 memory type、`vkAllocateMemory`、`vkBindBufferMemory`。非 sparse 资源必须完整连续绑定后才能用于 Descriptor/录制等操作。

### 17. `memoryTypeBits` 与 property flags 如何共同选择内存？

答：候选 index 必须在 `memoryTypeBits` 中置位，并且该 memory type 的 `propertyFlags` 包含全部 required flags。不能只按 flags 搜索而忽略资源 requirements。

### 18. `HOST_COHERENT` 是否消除所有同步？

答：否。它简化 Host/Device cache flush/invalidate 规则，但不证明 GPU 已完成访问，也不允许 CPU 覆盖在途 Buffer。执行同步与资源复用仍靠 Fence/Semaphore/Barrier 等。

### 19. 为什么 Vertex/Index Buffer 常用 staging？

答：离散 GPU 的高性能 DEVICE_LOCAL memory 常不可直接 Host map。先写 HOST_VISIBLE staging，再用 transfer command copy 到 DEVICE_LOCAL destination。

### 20. Texture 上传的两个布局转换各解决什么？

答：UNDEFINED->TRANSFER_DST 让 Image 可被 transfer 写，旧内容不保留；TRANSFER_DST->SHADER_READ 让 transfer write 对 shader read 可见，并把布局转为采样使用。

### 21. Image Layout 是整个 Image 的单一状态吗？

答：一般按 subresource 跟踪，即 aspect、mip、array layer；同一 Image 的不同 subresources 可以处于不同布局，但 depth/stencil 分离还受 feature 约束。

### 22. 当前“一资源一次 `vkAllocateMemory`”的缺点？

答：分配调用开销、allocation count limit、碎片和管理成本。工程上常使用 VMA 或大块 memory 子分配；学习阶段独立分配更易理解。

## D. Descriptor 与 Shader ABI

### 23. Descriptor Set Layout、Pipeline Layout、Descriptor Set 分别是什么？

答：Set Layout 描述一个 set 的 bindings/types/count/stages；Pipeline Layout 排列 set layouts 并定义 push constant ranges；Descriptor Set 是按某 Set Layout 分配并写入实际 resources 的实例。

### 24. Descriptor Pool 的 `maxSets` 足够，为什么仍可能分配失败？

答：Pool 还必须为每一种 descriptor type 提供足够总数量。Set 数与 descriptor 数是两套容量。

### 25. 当前 set 0 是什么？

答：Frame Data。binding 1 是 Camera UBO，binding 2 是 Object SSBO；每个 Frame Slot 一套 Buffer 与 Descriptor Set。

### 26. 当前 set 1 是什么？

答：Material。binding 0 是材质参数 UBO；1..5 是五张 sampled images；6..10 是对应 samplers。每个 GPU Material 一个 Descriptor Set。

### 27. 为什么 Object data 用 SSBO 而不是每物体一个 UBO Set？

答：把一帧全部对象放连续数组，只需一个 frame Set；每 Draw 用小 Push Constant index 选记录，减少 Descriptor 分配、更新与绑定数量。SSBO 对可变长大数组也更合适。

### 28. 当前 Push Constants 是什么？

答：8 字节的 `cameraIndex` 与 `objectIndex`。每 Draw 更新 objectIndex，Shader 用它索引 Object SSBO；cameraIndex 当前保持 0。

### 29. 为什么 CPU struct 和 HLSL 看起来字段相同仍可能错？

答：alignment、padding、matrix major order、SPIR-V layout rules 与编译器 ABI 都可能不同。应使用显式 layout、`alignas`、`sizeof/offsetof static_assert` 与 shader reflection/RenderDoc 核对。

### 30. 分离 Image/Sampler descriptor 与 Combined Image Sampler 的取舍？

答：分离可独立复用 sampler 和 image，但占用更多 bindings/descriptors；combined 更简单并符合很多材质系统的“一纹理一采样器”路径。两者都是 Vulkan 正式模型。

## E. Pipeline 与绘制

### 31. Shader Module 是 GPU 最终机器码吗？

答：不是。它封装 SPIR-V；驱动通常在 Pipeline 创建时继续编译/优化为设备相关形式。Pipeline 创建成功后 Shader Module 通常可销毁。

### 32. Pipeline Layout 为什么可称为 Shader 资源 ABI？

答：它规定 set 0..N 的 layouts 与 push constant ranges。Shader 中 statically used 的 descriptors/push constants 必须与它的 set/binding/type/stage/range 匹配。

### 33. 当前哪些 Pipeline 状态是动态的？

答：Viewport 与 Scissor。录制每帧 Command Buffer 时按 Swapchain extent 设置；所以纯尺寸变化无需因这两个状态重建 Pipeline。

### 34. `primitiveRestartEnable` 和顶点复用是一回事吗？

答：不是。顶点复用来自 Index Buffer 重复引用同一顶点。Primitive restart 是在 strip/fan 等 topology 的 index stream 中用特殊 index 结束当前 primitive strip 并开始新段；Triangle List 当前不需要。

### 35. Back-face culling 的 front face 由什么决定？

答：窗口坐标中顶点绕序与 Pipeline 的 `frontFace`。还受投影/viewport Y 翻转影响。当前是 CCW front、Cull Back，必须用实际变换后绕序核对。

### 36. `vkCmdDrawIndexed` 的 `firstIndex` 与 `vertexOffset`？

答：firstIndex 选择 Index Buffer 起点；每个读出的 index 再加 vertexOffset 得到最终 vertex index。当前 submesh 用 firstIndex/indexCount 切片，vertexOffset 为 0。

## F. Render Target 与 Command Buffer

### 37. Render Pass 与 Framebuffer 的区别？

答：Render Pass 描述 attachment/subpass/load-store/layout/dependency 结构；Framebuffer 提供匹配该结构的具体 ImageViews 与尺寸。前者是抽象契约，后者是具体 render targets。

### 38. 为什么 Color `storeOp=STORE` 而 Depth 可以 `DONT_CARE`？

答：Color 结果要交给 Present，所以要保留；当前 Depth 只在该 Pass 内测试，之后不读，结束时可不保存，从而允许实现优化。

### 39. 为什么每个 Swapchain Image 要独立 Depth？

答：多帧可能并行，若所有 Framebuffers 共用一张 Depth，不同 submissions 会对同一资源产生跨帧 WAW hazard，并串行化。Depth[I] 让资源归属与 Framebuffer[I] 一致。

### 40. Command Buffer 记录命令时是否复制了 Buffer/Descriptor 数据？

答：没有。它保留对 Vulkan objects/resources 的引用和命令参数；相关对象与内存必须活到 GPU 执行完成，更新被引用 Descriptor/Buffer 也要遵守 update 与同步规则。

### 41. Pending Command Buffer 可以 reset 吗？

答：不可以。当前先等 Fence[F]，再 reset Command Pool[F]，从而证明其中 Command Buffer 不再 pending。

### 42. `ONE_TIME_SUBMIT` 是否意味着只能创建一次？

答：它是录制 usage hint，表示该次录制内容预期只 Submit 一次；完成后可 reset 并重新录制。当前每帧正是这种用法。

## G. 同步核心

### 43. `vkQueueSubmit` 返回意味着什么？

答：通常只表示工作已提交，不是 GPU 完成。完成要由 Fence、Semaphore/timeline value、Queue/Device Idle 等同步观察。

### 44. Fence 与 Semaphore 最短区别？

答：Fence 典型由 Queue signal、Host wait，用于 CPU 知道 GPU 完成；Semaphore 用于 Queue/WSI 等异步操作之间建立依赖，不由普通 Host wait binary 状态（timeline 另有 Host wait API）。

### 45. Fence wait 会自动 reset 吗？

答：不会。当前 `Fence::wait()` 后还要在确定 Submit 前调用 reset。

### 46. Binary Semaphore 需要手动 reset 吗？

答：没有 reset API。它通过合法的一次 signal 与一次 wait 配对回到可再次 signal 的状态；不能在上一 wait 尚未安全完成时再次 signal。

### 47. 为什么 Fence 要在 Acquire 成功后再 reset？

答：若先 reset，而 Acquire 返回 OUT_OF_DATE 后提前结束，本帧没有 Submit 会 signal Fence；下一次轮到该 F 时 Host 永远等待。

### 48. 为什么 Present Semaphore 不能简单按 F 复用？

答：Frame Fence 只覆盖 Graphics Submit 完成，不规范保证 Present Engine 已消费对该 semaphore 的 wait。按 I 使用 `renderFinished[I]`，重新 Acquire I 可作为相关 Present 进度的安全复用依据。

### 49. `imageInFlight[I]` 是同步对象吗？

答：不是。它是 CPU 侧表项，保存最近占用 Image I 的 Frame Fence。CPU 通过等待所保存的 Fence 间接保护 Framebuffer/Depth 等 I 资源。

### 50. 为什么等待 Acquire Semaphore 的 stage 是 `COLOR_ATTACHMENT_OUTPUT`？

答：Swapchain image 在本次命令中首次实际访问是颜色附件输出；只阻塞到该 stage，可让不访问 Swapchain image 的更早阶段并行执行。若命令先通过 transfer/compute 访问 Swapchain image，stage 必须相应改变。

### 51. 同一 Queue 提交顺序是否自动解决 RAW？

答：不一定。Queue operations 有一定 submission/order 规则，但 memory side effects 的 availability/visibility 仍需适当 memory dependency。必须按 source stage/access 与 destination stage/access 分析。

### 52. WAR 为什么常只需执行依赖？

答：前操作只是读，没有需要让后操作“看见”的写入内容；只需确保读完成后再允许写，通常不需要 availability/visibility access masks。

### 53. Barrier 中 stage mask 与 access mask 分别控制什么？

答：Stage masks 定义前后操作的执行范围；Access masks 定义需要做 available/visible 的具体内存访问类型。两者要与真实访问匹配，不能把 stage 当作 access 的替代。

### 54. Synchronization2 的核心改进？

答：把 stage/access 成对放进 barrier/semaphore submit structures，使用 64-bit flags 与更清晰的 `VkDependencyInfo`/`VkSubmitInfo2`，减少旧 API 把 stage 和 resource barrier 分散在不同参数处的认知负担。

## H. RAII、调试与设计题

### 55. RAII wrapper 的析构是否可以直接销毁任何 Vulkan object？

答：只有在 GPU 不再使用时。RAII 解决 Host 侧所有权和异常路径，不自动建立 GPU 完成同步；外层必须用 Fence/Idle/延迟销毁保证安全。

### 56. 为什么 replacement/commit 模式比先 reset 再逐项 create 更稳？

答：构建中途失败时局部对象自动回滚，旧有效对象仍可保留；全部成功后一次提交，避免半初始化状态和资源泄漏。

### 57. 为什么成员声明顺序会影响 Vulkan 正确性？

答：C++ 按声明逆序析构。依赖对象要先析构、父对象后析构；例如 Framebuffer 要先于其 ImageViews/RenderPass，Device 要晚于所有 device children。

### 58. Validation 没报错是否证明渲染器完全正确？

答：不证明。Validation 只能覆盖实现到的规则和实际执行到的路径，也不能判断所有画面/性能/色彩语义。仍需测试边界、RenderDoc、跨厂商运行与代码因果分析。

### 59. 当前最值得用 RenderDoc 核对哪五项？

答：Vertex/Index interpretation、set 0/1 descriptors、Push Constants 的 objectIndex、Depth/Color attachments 与 layouts、Swapchain SRGB 和 Shader gamma 的最终色彩路径。

### 60. 下一版优化时应先做 GPU-driven 还是 Upload 批处理？

答：通常先 Upload 批处理、持久映射和资源子分配；它们直接消除当前可见的 Queue Idle/分配开销。只有 Profile 表明 Draw/CPU 录制成为瓶颈时再引入 GPU-driven。

## I. 必做画图题

不看答案，手画以下五张图：

1. Instance、Surface、PhysicalDevice、Device、Queue 的依赖图。
2. Swapchain Image[I]、ImageView[I]、Depth[I]、Framebuffer[I]、Render Pass 的关系。
3. Shader set/binding -> Descriptor Set Layout -> Pipeline Layout -> Descriptor Set 的关系。
4. Frame Slot `F` 与 Swapchain Image `I` 的资源归属表。
5. Acquire -> Submit -> Present 时间线，标出两个 Semaphore 与 Fence。

画完后与 [VULKAN_CORE_REVIEW.md](VULKAN_CORE_REVIEW.md) 第 2、6、10 章核对。

## J. 实战题

### 61. 把 Texture upload 改成 Synchronization2，先写转换表

答题要点：

- UNDEFINED -> TRANSFER_DST：src stage `NONE`，src access `NONE`；dst stage `COPY/TRANSFER`，dst access `TRANSFER_WRITE`。
- TRANSFER_DST -> SHADER_READ：src stage `COPY/TRANSFER`，src access `TRANSFER_WRITE`；dst stage `FRAGMENT_SHADER`，dst access `SHADER_SAMPLED_READ`。
- 使用 `VkImageMemoryBarrier2` + `VkDependencyInfo` + `vkCmdPipelineBarrier2`。
- 实际 flag 名称以目标 Vulkan header/spec 为准。

### 62. 把 UploadContext 的 Queue Idle 改成 Fence，生命周期要怎么变？

答题要点：Submit 带 Fence；staging Buffer、Command Buffer 与 destination transition 所需对象必须活到 Fence signal。同步版接口可 wait fence 后返回；异步版要返回 upload ticket，并把 staging/command resources 放入待回收批次。

### 63. 改成 Dynamic Rendering 时哪些对象消失，哪些信息仍需提供？

答题要点：简单路径可不创建 RenderPass/Framebuffer；录制时用 rendering attachment infos 提供 ImageView、layout、load/store/clear。Pipeline 创建仍需知道 color/depth formats；Image layout transitions 与同步并没有消失。

### 64. 支持两个 Camera 时怎么改？

答题要点：FrameData 已容纳最多 16 个 `CameraGpuData`；stage 两条 Camera records；每个 RenderView/Draw 正确设置 cameraIndex；验证 Push Constants 的 cameraIndex 不再总为 0，并处理不同 viewport/scissor 或多 Pass 需求。

### 65. 支持 Mipmap 时 Vulkan 路径增加什么？

答题要点：Image 创建多个 mipLevels，并加 transfer src/dst usage；逐级 blit/compute 或离线生成；每级 subresource layout/barrier；ImageView levelCount 覆盖 mip；Sampler maxLod；查询 format blit/filter feature。

