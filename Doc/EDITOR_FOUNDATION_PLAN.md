# Editor 基建清单

> 状态：进行中
> 最近更新：2026-08-24
> 目标：在保留 Runtime 渲染路径的前提下，以单 EXE 的 Editor 模式建立 Docking、场景层级、属性检查器和场景视口基础设施。

> 当前范围：AssetDatabase、Asset Browser 和 Scene 资产持久化暂缓；Editor 每次启动继续加载固定 GLB。

## 1. 当前阶段决策

- 第一阶段保持单个 `VulkanApp.exe`。
- `VulkanApp.exe` 使用现有 Runtime 模式启动。
- `VulkanApp.exe --editor` 进入 Editor 模式。
- Editor 只有一个主 GLFW 窗口；Hierarchy、Inspector、Scene Viewport 等均为 ImGui Docking 面板。
- 暂不开启 ImGui Multi-Viewport，不手动为每个面板创建 GLFW 窗口。
- Runtime 与 Editor 共用 Renderer、Scene 和 AssetManager，不维护两套引擎实现。
- 现有 `ImGuiLayer` 继续作为 ImGui/GLFW/Vulkan 后端基础设施，不承载具体面板逻辑。
- 等 Editor 的启动、资源和发布需求与 Runtime 明显分离后，再考虑增加独立的 `VulkanEditor.exe`。

## 2. 目标目录结构

```text
head/Editor/
├── EditorApp.h
├── EditorLayer.h
├── EditorContext.h
├── EditorSelection.h
└── Panels/
    ├── SceneHierarchyPanel.h
    ├── InspectorPanel.h
    ├── SceneViewportPanel.h
    └── RendererStatsPanel.h

src/Editor/
├── EditorApp.cpp
├── EditorLayer.cpp
└── Panels/
    ├── SceneHierarchyPanel.cpp
    ├── InspectorPanel.cpp
    ├── SceneViewportPanel.cpp
    └── RendererStatsPanel.cpp
```

## 3. Editor 启动入口

- [x] 解析 `--editor` 启动参数。
- [x] 新增 `EditorApp`，作为 Editor 组合根和运行配置入口。
- [x] 新增 `EditorLayer`，负责 Editor 面板调度。
- [x] Runtime `App` 只依赖通用 `ApplicationGui` 接口，不引用 Editor 类型。
- [x] Runtime 模式不创建 Editor 面板和 Editor 专用渲染资源。
- [x] Editor 模式复用现有 Window、Renderer、Scene 和 AssetManager。
- [x] 退出时等待 Renderer idle，再销毁 ImGui 和 Editor Vulkan 资源。
- [x] 保留现有 `--asset-test`、`--render-test` 行为。
- [x] 新增隐藏窗口 `--editor-test`，验证 Editor GUI 和 Swapchain 重建。

## 4. ImGui 与 Docking 主界面

- [x] 引入 Dear ImGui Docking 分支的必要源码和 License。
- [x] 接入 GLFW 与 Vulkan 后端。
- [x] 在现有 Swapchain Present Pass 中绘制 ImGui。
- [x] 支持 Swapchain 重建后重建 ImGui Renderer Pipeline。
- [x] `ImGuiLayer` 显式配置 `enableDocking` 和 ini 文件路径。
- [ ] Multi-Viewport 阶段再增加 `enableViewports` 和对应 Vulkan 后端配置。
- [x] Editor 模式保存和恢复独立的 `editor_imgui.ini`。
- [x] Runtime 模式继续禁用布局文件。
- [x] 创建覆盖主窗口的 DockSpace。
- [x] 添加 `File`、`View` 菜单骨架。
- [ ] 补齐 `File`、`Edit`、`View` 的实际 Editor 命令。
- [x] 建立默认布局：左侧 Hierarchy、中间 Scene Viewport、右侧 Inspector、底部 Renderer Stats。
- [x] View 菜单可以重新显示被关闭的面板。

## 5. EditorContext

各面板通过统一上下文访问 Editor 状态，避免直接依赖 `App`：

```cpp
struct EditorContext
{
    Scene* scene{};
    AssetManager* assets{};
    VulkanRenderer* renderer{};
    EditorSelection* selection{};
};
```

- [ ] `EditorContext` 只保存非 owning 指针或引用。
- [ ] 明确 Context 指向对象的生命周期均长于所有面板。
- [ ] 面板不直接访问 `App`。
- [ ] 面板不直接操作 Renderer 内部 Vulkan 资源。
- [ ] Editor 参数和 UI 状态不混入 Runtime 的 `RenderFrame`，除非确实属于渲染输入。

## 6. Scene Node 稳定身份

当前 Scene Node 的 `std::vector` 下标不适合作为长期 Editor 选择状态。完整编辑功能开始前应先建立稳定身份。

- [ ] 新增稳定的 `SceneNodeId`，可采用 index + generation 或 UUID。
- [ ] 能够判断一个 Node ID 是否仍然有效。
- [ ] 父节点关系改为稳定 Node ID，不长期保存裸 vector 下标。
- [ ] Scene 提供 `createNode()`。
- [ ] Scene 提供 `destroyNode()`。
- [ ] Scene 提供 `findNode()`。
- [ ] Scene 提供 `renameNode()`。
- [ ] Scene 提供 `reparentNode()`。
- [ ] Scene 提供 Transform、Layer 和 Culling 等字段的修改接口。
- [ ] 明确删除父节点时对子节点采用递归删除还是重新挂接。
- [ ] `reparentNode()` 阻止节点形成循环父子关系。

## 7. EditorSelection

- [x] 建立统一 `EditorSelection`，不保存 Scene/Asset 裸指针。
- [x] 使用 `InspectorTarget` variant 区分 SceneNode、ModelNode、Model、Mesh、Submesh、Material、MaterialTemplate 和 Texture。
- [x] 支持主选择、清空选择、Inspector 引用导航和 Back 历史。
- [x] Hierarchy 和 Inspector 共用同一选择状态。
- [x] SceneNode/ModelNode 索引或 AssetHandle 失效时显示安全空状态或清理选择。
- [ ] `SceneNodeId` 建立后，用稳定 ID 替换当前临时 SceneNode vector 下标。
- [ ] Scene Viewport、Gizmo 和多选接入同一选择状态。

## 8. Scene Hierarchy Panel

- [x] 独立 `SceneHierarchyPanel` 依赖 Scene/Asset 数据，不依赖 Renderer。
- [x] 按 `SceneNode::parent` 一次构建 roots/children 并递归绘制场景树。
- [x] 在引用模型的 SceneNode 下直接只读投影 `ModelAsset::ModelNode` 层级，不添加 Asset 包装节点。
- [x] 在 ModelNode 下直接显示 Mesh/Submesh，点击 SceneNode、ModelNode、Mesh 或 Submesh 后写入统一 `EditorSelection`。
- [x] 正确显示当前选择和展开状态。
- [x] 点击空白区域取消选择。
- [ ] 右键菜单支持创建和删除节点。
- [ ] 支持节点重命名。
- [ ] 支持拖放调整父子关系。
- [ ] 拒绝无效或会形成循环的拖放操作。

当前固定 Demo Scene 只有一个真正的 `SceneNode`；GLB 的内部 ModelNode、Mesh 和 Submesh 会作为只读子树直接显示，但不会复制成 SceneNode，避免与现有 Render Extractor 重复展开。

## 9. Inspector Panel

第一阶段建立只读、多类型 Inspector：

- [x] 未选择对象时显示明确的空状态。
- [x] 使用 `std::visit` 将选择路由到类型专属 drawer，不使用 RTTI，也不让引擎对象依赖 ImGui。
- [x] 支持 SceneNode、ModelNode、Model、Mesh、Submesh、Material、MaterialTemplate 和 Texture 的最小只读信息。
- [x] Mesh/Submesh 由 Hierarchy 直接选择；Inspector 支持从 SceneNode 进入 Model，以及从 Submesh 继续进入 Material → Texture。
- [x] Inspector 只依赖 Scene/Asset 数据，不依赖 Renderer。
- [ ] 补全 Transform、Layer、Culling、Bounds、材质参数和采样器等只读字段。
- [ ] 编辑阶段的所有修改通过 Scene/Asset 编辑 API 完成，不直接修改内部容器。

Transform 建议逐步从单独的 `glm::mat4` 提升为可编辑数据：

```cpp
struct Transform
{
    glm::vec3 position{};
    glm::quat rotation{};
    glm::vec3 scale{1.0f};
};
```

需要渲染矩阵时再由 Transform 计算，避免 Inspector 反复对矩阵做不稳定的分解和重组。

## 10. Scene Viewport 渲染路径

Editor 不能继续把场景最终结果直接占满 Swapchain。目标路径为：

```text
Scene HDR Pass
    -> Tone Mapping
    -> Editor LDR Image
    -> ImGui::Image()
    -> Swapchain UI Pass
    -> Present
```

Runtime 路径继续保持：

```text
Scene HDR Pass
    -> Tone Mapping to Swapchain
    -> Optional ImGui Overlay
    -> Present
```

- [x] 创建每 Frame Slot 一个 Editor 专用、可采样的 LDR Color Image。
- [x] 为其创建 ImageView，并复用 ImGui Vulkan Backend 的线性 Sampler。
- [x] 建立 Tone Mapping 到 Editor LDR Image 的 Render Pass/Framebuffer 路径。
- [x] 将输出注册为 ImGui Vulkan Texture。
- [x] 在 `SceneViewportPanel` 中通过 `ImGui::Image()` 显示。
- [ ] 根据面板可用区域更新目标渲染尺寸。
- [ ] 尺寸变化时延迟、安全地重建目标资源。
- [x] 正确处理 Color Attachment 与 Shader Read 之间的 image layout transition。
- [x] Swapchain 重建前等待 GPU，并先释放 ImGui texture descriptor。
- [x] 避免销毁仍被在途帧引用的纹理和 descriptor set。
- [ ] Swapchain 重建不应无条件重建 Editor Viewport 资源。
- [x] Runtime 模式不承担 Editor Viewport 的额外显存和渲染开销。

建议向 Editor 暴露小而明确的 Renderer 接口：

```cpp
struct EditorViewportOutput
{
    VkImageView imageView{};
    VkSampler sampler{};
    VkExtent2D extent{};
};

void resizeEditorViewport(VkExtent2D extent);
void renderEditorViewport(const RenderFrame& frame);
const EditorViewportOutput& editorViewportOutput() const;
```

最终接口不必原样照搬，但应保持“面板消费输出、Renderer 管理 Vulkan 资源”的边界。

## 11. Editor Camera 与输入

- [ ] Editor Camera 与 Runtime Camera 分离。
- [ ] Scene Viewport 记录 hover、focus 和可用尺寸；当前已回传内容区宽高比。
- [ ] 只有 Scene Viewport 处于合适交互状态时才接收相机输入。
- [ ] 支持右键 + WASD 漫游。
- [ ] 支持鼠标旋转视角。
- [ ] 支持滚轮调节速度或观察距离。
- [ ] Viewport 未聚焦时不抢占其他面板键鼠输入。
- [x] 使用 Scene Viewport 内容区宽高比更新当前 Camera projection。

## 12. 编辑命令与 Undo/Redo

可以在基础面板可用后实现，但所有修改入口需要为命令系统保留边界。

- [ ] 定义 `IEditorCommand` 或等价命令接口。
- [ ] 建立 Command Stack。
- [ ] 支持 `Ctrl+Z` 和 `Ctrl+Y`。
- [ ] Transform 连续拖动合并为一次操作。
- [ ] 创建、删除、重命名和 Reparent 均可撤销。
- [ ] 删除命令保留恢复节点及其子树所需的数据。
- [ ] 新操作发生后清空 Redo Stack。

## 13. Scene 保存与加载

- [ ] 定义 Scene 序列化格式和版本字段。
- [ ] 支持 New、Open、Save、Save As。
- [ ] 跟踪 Scene dirty 状态。
- [ ] 未保存场景在关闭或切换前提示用户。
- [ ] Editor 偏好、Docking 布局与 Scene 数据分别保存。
- [ ] 对失效模型资源提供清晰的降级显示。

## 14. Multi-Viewport 与独立 Editor EXE

以下内容不属于第一阶段：

- [ ] 完成单窗口 Docking 后再评估 `ImGuiConfigFlags_ViewportsEnable`。
- [ ] 配置 ImGui Vulkan Backend 的 Viewport Pipeline。
- [ ] 在主帧结束后调用 `ImGui::UpdatePlatformWindows()` 和 `ImGui::RenderPlatformWindowsDefault()`。
- [ ] 验证附加 GLFW 窗口的 Surface、Swapchain 和同步生命周期。
- [ ] 根据产品需求评估是否拆分 `VulkanEditor.exe`。
- [ ] 若拆分 EXE，将共享代码整理为 Engine/Renderer Library，而不是复制实现。

## 15. 推荐实施顺序

1. [x] `--editor`、`EditorApp`、`EditorLayer`、DockSpace 和面板占位。
2. [ ] `EditorContext`、稳定 `SceneNodeId` 和 Scene 编辑 API。
3. [ ] `EditorSelection`、Scene Hierarchy 和基础 Inspector。
4. [x] 固定 Swapchain 尺寸的 Editor LDR 输出和 Scene Viewport。
5. [ ] Scene Viewport 尺寸驱动的 RenderTarget 和 Editor Camera。
6. [ ] Viewport 输入路由。
7. [ ] Undo/Redo 和完整 Scene 编辑操作。
8. [ ] 恢复 AssetDatabase、Scene 保存加载和 dirty 状态工作。
9. [ ] 最后评估 Multi-Viewport 与独立 `VulkanEditor.exe`。

## 16. 第一阶段完成标准

- 使用 `VulkanApp.exe --editor` 能进入稳定的单窗口 Docking 界面。
- Runtime 模式行为和渲染结果不受影响。
- Hierarchy 能显示场景树并选择节点。
- Inspector 能编辑所选节点的名称和 Transform。
- Scene Viewport 能显示 Renderer 输出并随面板尺寸更新。
- Swapchain resize、窗口最小化和恢复不触发 Vulkan Validation Error。
- Editor 关闭时不存在仍在使用的 ImGui descriptor、image、framebuffer 或 pipeline。
