# Vulkan 帧同步：Fence 与 Binary Semaphore

> 基于当前 LearnVulkan 架构整理。本文暂不讨论 Timeline Semaphore。

## 0. 本文要解决的问题

读完后应能回答：

1. 单 CPU 线程为什么能在上一个 Submit 未结束时准备下一帧？
2. Semaphore 的 Signal 到底是什么意思？
3. Fence 和 Semaphore 分别由谁 Signal、谁 Wait、何时 Reset？
4. `currentFrame`（F）与 `imageIndex`（I）为什么是两个索引？
5. `imageInFlight[I] = inFlightFence[F]` 到底在记录什么？
6. 封装后的 `FrameContext`、`SwapchainResources` 如何还原成原生 Vulkan 调用？

---

## 1. 第一原则：Submit 是异步的

单线程只表示 CPU 上只有一个线程顺序调用 Vulkan API，不表示 CPU 必须等待 GPU。

```cpp
vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
```

通常只是把工作放进 GPU Queue。函数返回时，GPU 可能还没开始执行，更不代表渲染已经完成。

```text
CPU 单线程                         GPU
    |                              |
    | 准备 Frame 0                 |
    | vkQueueSubmit(Frame 0) ----->| 开始/等待执行 Frame 0
    | 函数返回                     | 正在执行 Frame 0
    | 准备 Frame 1                 | 仍在执行 Frame 0
    | vkQueueSubmit(Frame 1) ----->| Frame 1 进入队列
```

可以把 CPU 想成提交打印任务的人，GPU 是打印机：一个人可以在打印机处理文档 0 时继续准备文档 1。

只有 CPU 显式调用以下操作时，才会等待 GPU：

```cpp
vkWaitForFences(...);
vkQueueWaitIdle(...);
vkDeviceWaitIdle(...);
```

### Frames in Flight 的含义

当前项目有两个 Frame Slot：

```text
FrameContext[0]
FrameContext[1]
```

使用顺序固定为：

```text
Draw 0 -> F0
Draw 1 -> F1
Draw 2 -> F0
Draw 3 -> F1
```

Draw 0 提交后，CPU 可以用另一套资源 F1 准备 Draw 1。只有 Draw 2 想重新使用 F0 时，才必须等待 Draw 0 的 Fence。

```text
Draw 0: wait Fence[0] -> record F0 -> submit F0
Draw 1: wait Fence[1] -> record F1 -> submit F1
Draw 2: wait Fence[0] -> 等 Draw 0 完成后复用 F0
```

所以 Fence 的意义是：

> CPU 可以领先 GPU，但即将覆盖 GPU 仍在使用的帧槽时，Fence 让 CPU 停下来。

这与 CPU 是否多线程无关。

---

## 2. 两个独立索引：Frame Slot F 与 Swapchain Image I

当前同步架构包含两套循环。

### Frame Slot 索引 F

```cpp
F = currentFrame_;
```

当前固定轮换：

```text
0 -> 1 -> 0 -> 1
```

它选择 CPU 可复用的帧资源：

```text
CommandPool[F]
CommandBuffer[F]
imageAvailable[F]
inFlightFence[F]
FrameData[F]
```

### Swapchain Image 索引 I

```cpp
vkAcquireNextImageKHR(..., &imageIndex);
I = imageIndex;
```

它由 WSI 决定，返回顺序不由应用控制：

```text
I2 -> I0 -> I1 -> I2 -> I1 ...
```

它选择按交换链图像组织的资源：

```text
Swapchain Image[I]
Framebuffer[I]
Depth Image[I]
renderFinished[I]
imageInFlight[I]
```

因此一帧完全可能是：

```text
F = 0
I = 2
```

不要假设 `F == I`。

---

## 3. 当前同步对象总览

先把封装忽略，当前架构可以还原成：

```cpp
// 按 Frame Slot F
VkSemaphore imageAvailable[2];
VkFence     inFlightFence[2];

// 按 Swapchain Image I
VkSemaphore renderFinished[swapchainImageCount];
VkFence     imageInFlight[swapchainImageCount]; // 只保存句柄，不拥有 Fence
```

职责表：

| 对象 | 谁 Signal | 谁 Wait | 是否手动 Reset | 保护什么 |
|---|---|---|---|---|
| `imageAvailable[F]` | Acquire / WSI | Graphics Submit | 否 | Acquire 到 Graphics 的依赖 |
| `renderFinished[I]` | Graphics Submit | Present | 否 | Graphics 到 Present 的依赖 |
| `inFlightFence[F]` | Graphics Submit | CPU | 是 | Frame Slot F 的复用 |
| `imageInFlight[I]` | 不会被 Signal；它只是保存 Fence 句柄 | CPU 间接等待所保存的 Fence | 不适用 | Image I 最近一次 Graphics 使用 |

速记：

```text
Acquire Signal imageAvailable，Graphics Wait imageAvailable
Graphics Signal renderFinished，Present Wait renderFinished
Graphics Signal inFlightFence，CPU Wait + Reset inFlightFence
```

---

## 4. Semaphore 的 Signal 到底是什么

不要把 Signal 想成可能丢失的函数通知或回调。

Binary Semaphore 可以理解成一个只能放一张“通行证”的槽位：

```text
Unsignaled = 没有通行证
Signaled   = 有一张通行证
```

生产者完成前置工作后 Signal，相当于放入通行证：

```text
Unsignaled --Signal--> Signaled
```

消费者的 Wait 等到通行证出现，取走后继续：

```text
Signaled --Wait 消费--> Unsignaled
```

完整状态机：

```text
                  Signal
     Unsignaled ----------> Signaled
          ^                    |
          |                    | Wait 消费
          +--------------------+
```

### Signal 是异步操作的一部分

```cpp
submitInfo.pSignalSemaphores = &renderFinished[I];
vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
```

其含义不是“CPU 调用 `vkQueueSubmit` 时立刻 Signal”，而是：

```text
把以下任务提交给 Graphics Queue：

1. 等待指定的 Semaphore
2. 执行 Command Buffer
3. 本次 Submit 完成后 Signal renderFinished[I]
4. 本次 Submit 完成后 Signal fence
```

状态可能是：

```text
CPU 从 vkQueueSubmit 返回       renderFinished[I] = Unsignaled
GPU 正在执行 Command Buffer     renderFinished[I] = Unsignaled
GPU 完成本次 Submit             renderFinished[I] = Signaled
```

准确理解：

> Signal 表示某项异步前置工作已经完成，从而允许依赖它的后续操作继续。

### Wait 可以先排队

消费者不需要等 Semaphore 已经 Signaled 后才提交 Wait。

```text
Wait 先进入 Queue
       |
       | Semaphore 尚未 Signal
       v
后续工作停在等待点
       |
       | 未来 Signal 到达
       v
Wait 满足并消费 Signal，继续执行
```

Signal 先发生也不会丢失：Semaphore 会保持 Signaled，直到匹配的 Wait 消费它。

### Binary Semaphore 的使用约束

合法：

```text
Signal -> Wait -> Signal -> Wait
```

错误：

```text
Signal -> Signal
```

一个尚未被 Wait 消费的 Binary Semaphore 不能再次 Signal。一次 Signal 也只能匹配一次 Wait，不能广播给多个等待者。

---

## 5. imageAvailable[F]：Acquire 到 Graphics

Acquire：

```cpp
vkAcquireNextImageKHR(
    device,
    swapchain,
    UINT64_MAX,
    imageAvailable[F],
    VK_NULL_HANDLE,
    &I);
```

含义：

> WSI 允许应用使用本次获得的 Image I 时，Signal `imageAvailable[F]`。

Graphics Submit 等待它：

```cpp
submitInfo.pWaitSemaphores = &imageAvailable[F];
submitInfo.pWaitDstStageMask =
    &VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
```

周期：

```text
imageAvailable[F] = Unsignaled
        |
        | WSI 允许使用 Image I，Signal
        v
imageAvailable[F] = Signaled
        |
        | Graphics Wait 并消费
        v
imageAvailable[F] = Unsignaled
```

这条依赖保证：

```text
WSI 对 Image I 的使用
        ->
Graphics 对 Image I 的写入
```

---

## 6. renderFinished[I]：Graphics 到 Present

Graphics Submit 完成后 Signal：

```cpp
submitInfo.pSignalSemaphores = &renderFinished[I];
```

Present 等待：

```cpp
presentInfo.pWaitSemaphores = &renderFinished[I];
vkQueuePresentKHR(presentQueue, &presentInfo);
```

周期：

```text
renderFinished[I] = Unsignaled
        |
        | Graphics 完成本次 Submit，Signal
        v
renderFinished[I] = Signaled
        |
        | Present Wait 并消费
        v
renderFinished[I] = Unsignaled
```

这条依赖保证：

```text
Graphics 对 Image I 的写入
        ->
Present 对 Image I 的读取/显示
```

`vkQueuePresentKHR` 可以在 `renderFinished[I]` 仍然 Unsignaled 时调用。Present 会进入等待状态，而不是要求 CPU 先等 Graphics 完成。

当前项目将 `renderFinished` 按 Image I 分配，因为 Graphics Fence 只能证明 Graphics Submit 已完成，不能单独证明 Present 已经消费了这个 Semaphore。

---

## 7. Fence：GPU Submit 到 CPU

Fence 是 CPU 可等待的完成状态：

```text
Graphics Submit 完成 -> Signal Fence -> CPU Wait 返回
```

当前 Fence 创建时带有：

```cpp
VK_FENCE_CREATE_SIGNALED_BIT
```

因此首次使用 Frame Slot 时，CPU Wait 可以立即通过。否则第一帧之前没有任何 Submit 会 Signal 它，CPU 会永久等待。

### Fence Wait 不消费状态

```text
Signaled --CPU Wait--> 仍然 Signaled
```

所以必须显式 Reset：

```cpp
vkWaitForFences(...);
vkResetFences(...);
vkQueueSubmit(..., fence);
```

Fence 的状态周期：

```text
初始 Signaled
    |
    | CPU Wait，状态不变
    v
Signaled
    |
    | vkResetFences
    v
Unsignaled
    |
    | Graphics Submit 完成
    v
Signaled
```

与 Binary Semaphore 对比：

```text
Fence：Wait 只观察，不消费；需要手动 Reset
Semaphore：Wait 会消费 Signal；没有手动 Reset API
```

### 为什么 Fence 要在 Acquire 成功后 Reset

错误顺序：

```text
wait Fence[F]
reset Fence[F]
Acquire 返回 OUT_OF_DATE
函数提前 return
```

此时没有 Submit 会 Signal 已被 Reset 的 Fence。下一次进入 F 时将永久等待。

正确顺序：

```text
wait Fence[F]
Acquire 成功
完成提交前准备
reset Fence[F]
紧接着 vkQueueSubmit(..., Fence[F])
```

原则：

> Reset Fence 后，必须有一个确定会执行的 Submit 负责重新 Signal 它。

---

## 8. `imageInFlight[I] = inFlightFence[F]` 是什么

这行代码不是 GPU 同步命令，不会 Signal 或 Wait：

```cpp
imageInFlight[I] = inFlightFence[F];
```

它只在 CPU 侧记录一条映射：

> Swapchain Image I 最近一次由 Frame Slot F 的 Graphics Submit 使用。

例如：

```text
本帧 F = 0
Acquire 得到 I = 2
```

记录：

```cpp
imageInFlight[2] = inFlightFence[0];
```

未来再次 Acquire Image 2 时：

```cpp
vkWaitForFences(imageInFlight[2]);
```

实际等待的是上次使用 I2 的 `inFlightFence[0]`。

### 它不是为了 CPU 多线程提交

即使只有一个 CPU 线程，也存在：

```text
CPU
Graphics Queue
Present Engine / WSI
```

它们异步推进，并且 F 与 I 的循环顺序不同。因此仍需要追踪资源最后被哪个 Submit 使用。

如果多个 CPU 线程同时调用同一个 `VkQueue`，还需要 Mutex 等外部同步；`imageInFlight` 不能解决 Queue 的 CPU 多线程访问问题。

### 它保护什么

`inFlightFence[F]` 回答：

> Frame Slot F 的 Command Pool、Command Buffer、帧数据能否复用？

`imageInFlight[I]` 指向的 Fence 回答：

> Image I 及其按 Image 组织的资源，最近一次 Graphics 使用是否完成？

它对以下按 Image 且可能被 CPU 修改的资源尤其重要：

```text
uniformBuffer[I]
mappedMemory[I]
descriptorSet[I]
perImageCommandBuffer[I]
CPU 读回资源[I]
```

当前项目主要拥有 GPU 侧的 Framebuffer、Depth Image 等资源，所以这层等待也具有保守防护、明确资源生命周期以及方便未来扩展的意义。

---

## 9. 如何把当前封装还原成原生 Vulkan

当前封装没有改变同步规则，只组织了对象所有权。

`FrameContext` 近似等于：

```cpp
struct FrameContext
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailable;
    VkFence inFlight;
};
```

阅读代码时做以下替换：

| 当前封装 | 脑内展开 |
|---|---|
| `frameContexts_[F]` | Frame Slot F |
| `frame.imageAvailable()` | `imageAvailable[F]` |
| `frame.inFlightFence()` | `inFlightFence[F]` |
| `frame.waitUntilReusable()` | `vkWaitForFences(inFlightFence[F])` |
| `frame.resetFence()` | `vkResetFences(inFlightFence[F])` |
| `swapchainResources_.renderFinished(I)` | `renderFinished[I]` |
| `waitUntilImageReusable(I)` | 等待 `imageInFlight[I]` 保存的 Fence |
| `markImageInFlight(I, Fence[F])` | `imageInFlight[I] = inFlightFence[F]` |

特别注意：

```cpp
frame.imageAvailable();
frame.inFlightFence();
```

只是返回内部 Vulkan Handle 的 Getter，本身不会执行 Signal、Wait 或 Reset。

---

## 10. 将当前 `render()` 完全展开

忽略错误处理后，当前帧同步可以理解为下面的原生伪代码：

```cpp
uint32_t F = currentFrame;

// 1. CPU 等待上次使用 Frame Slot F 的 Submit 完成
vkWaitForFences(inFlightFence[F]);

// 2. 获取 Swapchain Image I；可用时 Signal imageAvailable[F]
uint32_t I;
vkAcquireNextImageKHR(
    imageAvailable[F],
    &I);

// 3. 等待上次使用 Image I 的 Graphics Submit
if (imageInFlight[I] != VK_NULL_HANDLE)
{
    vkWaitForFences(imageInFlight[I]);
}

// 4. 记录 Image I 现在由本次 Frame Slot F 使用
imageInFlight[I] = inFlightFence[F];

// 5. F 的命令资源已经安全，可以 Reset 并重新录制
resetAndRecord(
    commandBuffer[F],
    framebuffer[I]);

// 6. 把 Fence 变成 Unsignaled，准备由本次 Submit Signal
vkResetFences(inFlightFence[F]);

// 7. 提交 Graphics 工作
vkQueueSubmit(
    wait    = imageAvailable[F],
    execute = commandBuffer[F],
    signal  = renderFinished[I],
    fence   = inFlightFence[F]);

// 8. Present 可立即排队，但要 Wait renderFinished[I]
vkQueuePresentKHR(
    image = I,
    wait  = renderFinished[I]);

// 9. 切换 Frame Slot，不等待本次 Submit 完成
currentFrame = (F + 1) % 2;
```

---

## 11. 一帧时间线

```text
CPU                         Graphics Queue             WSI / Present
 |                                |                         |
 | wait inFlightFence[F]          |                         |
 |<-- 上次使用 F 的 Submit 完成 --|                         |
 |                                                          |
 | Acquire(I, imageAvailable[F]) -------------------------->|
 |<-- 返回 I；图像可用时 Signal imageAvailable[F] ---------|
 |                                                          |
 | wait imageInFlight[I]          |                         |
 | imageInFlight[I] = Fence[F]    |                         |
 | 更新帧数据、录制 Command[F]    |                         |
 | reset Fence[F]                 |                         |
 |                                                          |
 | Submit ----------------------->| Wait imageAvailable[F]  |
 |                                | 执行 Command[F]         |
 |                                | 渲染 Framebuffer[I]     |
 |                                | Signal renderFinished[I]|
 |                                | Signal Fence[F]         |
 |                                                          |
 | Present(I) --------------------------------------------->| Wait renderFinished[I]
 |                                                          |
 | currentFrame = 下一个 F       | GPU 仍可能在工作         |
```

注意最后一行：CPU 切换到下一个 F 时，本次 Graphics/Present 完全可能仍未结束。

---

## 12. 具体数字示例

假设：

```text
currentFrame = 0
Acquire 返回 imageIndex = 2
```

本帧使用：

```text
按 Frame Slot F：
    commandBuffer[0]
    imageAvailable[0]
    inFlightFence[0]

按 Swapchain Image I：
    framebuffer[2]
    renderFinished[2]
    imageInFlight[2]
```

流程：

```text
1. CPU Wait inFlightFence[0]
2. Acquire Image 2；WSI 将 Signal imageAvailable[0]
3. CPU Wait imageInFlight[2] 保存的旧 Fence
4. imageInFlight[2] = inFlightFence[0]
5. 录制 commandBuffer[0]，渲染目标是 framebuffer[2]
6. Reset inFlightFence[0]
7. Graphics Wait imageAvailable[0]
8. Graphics 执行 commandBuffer[0]
9. Graphics Signal renderFinished[2]
10. Graphics Signal inFlightFence[0]
11. Present Wait renderFinished[2]，然后显示 Image 2
12. CPU 已切换到 F1，可能正在准备下一帧
```

---

## 13. 常见误解

### “`vkQueueSubmit` 返回就代表 GPU 完成”

错误。它通常只代表提交调用完成，GPU 工作仍在异步执行。

### “单 CPU 线程不会有多个 Submit 在途”

错误。CPU 可以连续向 GPU Queue 提交多个任务；多帧在途不要求多 CPU 线程。

### “Semaphore Signal 是一个瞬间消息，Wait 晚了会错过”

错误。Binary Semaphore 会保持 Signaled，直到匹配的 Wait 消费它。

### “Semaphore 需要手动 Reset”

错误。Binary Semaphore 的 Wait 会消费 Signal，没有 `vkResetSemaphore`。

### “Fence Wait 后自动变回 Unsignaled”

错误。Fence Wait 只观察状态，必须调用 `vkResetFences`。

### “`imageInFlight[I]` 是另一组 Fence”

错误。它只保存某个 `inFlightFence[F]` 的非 owning Handle。

### “`imageInFlight` 是为 CPU 多线程准备的”

错误。它追踪 Swapchain Image 与异步 Graphics Submit 的资源使用关系。CPU 多线程访问 Queue 需要另外的外部同步。

### “Frame Slot F 就是 Swapchain Image I”

错误。F 由应用固定轮换，I 由 Acquire 返回。

---

## 14. 最终记忆模型

### Semaphore：接力棒

```text
Signal = 生产者完成后放下接力棒
Wait   = 消费者等待并拿走接力棒，然后继续
```

```text
WSI --imageAvailable[F]--> Graphics --renderFinished[I]--> Present
```

### Fence：GPU 写给 CPU 的完成标志

```text
GPU Signal = 完成标志设为真
CPU Wait   = 等待并观察，不清除
CPU Reset  = 手动把标志改回假
```

### imageInFlight：资源到 Fence 的记录表

```text
imageInFlight[I] = Fence[F]

含义：Image I 最近一次由受 Fence F 追踪的 Submit 使用
```

### 看封装代码时只问四件事

1. 当前使用的是 Frame Slot `F`，还是 Swapchain Image `I`？
2. 谁 Signal？
3. 谁 Wait？
4. Wait 保护的是帧槽复用，还是图像资源复用？

最终同步链：

```text
                     imageAvailable[F]
WSI / Acquire  ----------------------------> Graphics Submit
                                                   |
                         +-------------------------+------------------+
                         |                                            |
                         | renderFinished[I]                          | inFlightFence[F]
                         v                                            v
                      Present                                        CPU
```
