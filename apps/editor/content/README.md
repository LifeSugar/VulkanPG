# 编辑器启动与内容加载

启动以 GUI 可以独立运行作为边界。`App::initVulkan()` 只创建
`VulkanContext` 和基础呈现资源，不读取模型或 shader，不上传场景资源。
相机设置也独立于 Vulkan 初始化。

## 执行流程

1. 创建窗口、设备、交换链、帧命令与同步资源。
2. 初始化 ImGui 和 GUI bridge，进入主循环。
3. 提交第一个 GUI 帧后，根据 `RunConfig::autoLoadDemo` 发起内容加载。
4. 工作线程在独立的 `PreparedContent` 中读取 shader、导入模型和解码贴图。
5. 主线程取得 CPU 准备结果，向 renderer 提交 `SceneResourceRequest`，每帧推进准备会话。
6. Vulkan 后端完成上传和 pipeline 准备后返回 Ready，App 在帧边界启用场景并移交完整的
   AssetManager、Scene、TextureImportRegistry 和 DemoContent。

GUI 只访问主线程的资源。在完成移交之前，资源面板和场景保持为空。
加载阶段、上传进度、完成信息和错误统一写入现有 Console，不创建加载面板。
上传进度按约 10% 的里程碑记录，避免每帧刷屏。加载失败不会退出编辑器。
当前只由主循环在首帧后自动发起一次加载，GUI 不提供加载或重试入口。

## 职责与接口

- `VulkanRenderer::createPresentation()`：创建无需场景 shader 的基础呈现。
  `operator bool()` 表示基础呈现可用，`sceneReady()` 单独表示场景渲染可用。
- `renderGui()`：只清理交换链颜色并绘制 GUI；不会执行场景 pass，也不会读取
  RenderAssetCache。没有场景时，GUI bridge 不注册视口纹理。
- `beginScenePreparation()` / `advanceScenePreparation()` / `scenePreparationStatus()`：
  接收 CPU 资源请求、推进后端准备并返回阶段、已提交数、已完成数和错误。
  请求与状态定义在 `engine/render/SceneResourcePreparation.hpp`，不包含 Vulkan 类型。
- `activatePreparedScene()`：在 Ready 后启用场景，再由 App 移交 CPU 内容。
  Ready 阶段仍允许 GUI 绘制和 resize，`sceneReady()` 直到启用后才为 true。
- `cancelScenePreparation()`：取消尚未启用的准备，清理上传和部分场景资源，
  保留基础呈现。取消已启用的准备不会销毁正在使用的场景。
- `VulkanScenePreparation`：renderer 内部的准备会话，拥有上传命令池和
  UploadContext，负责批次预算、提交、fence 查询、pipeline 准备和失败清理。
  `createSceneResources()`、`RenderAssetCache::beginUpload()` 等是其使用的后端接口。
- `DemoContentLoader::loadBuiltins()` / `loadModel()`：将默认纹理、材质模板和
  shader 的创建与具体模型导入分开；`load()` 保留同步组合入口供 CPU 测试使用。
- `RenderAssetCache::initialize()`：根据材质模板建立布局，不要求存在模型。
  `beginUpload()` 准备资源列表和描述符；`uploadNext()` 推进一个资源。
- `AppContentLoading.cpp`：管理 Preparing、Uploading、Finalizing、Ready、Failed
  应用状态，负责 CPU 工作线程、提交准备请求、展示状态、结果移交和请求取消。
  不创建命令池、不操作上传批次、不查询 fence、不构造 Vulkan pipeline。

当前一个 renderer 同时管理一个准备会话，只接受空场景和空 RenderAssetCache；
失败或取消后可以重新开始，已有场景不会被新请求覆盖。cache 仍由应用持有，
必须比 renderer 活得更久；本次边界调整尚未包含 cache 所有权迁移、场景热切换、
纹理重导入或 App 整体的 RHI 抽象。

## 上传与生命周期

`UploadContext` 默认保留同步调用方式；显式 `beginBatch()` 时只记录命令，
由 `submitBatch()` 提交、`pollBatch()` 查询 fence。staging buffer 和命令缓冲
保留到该批次完成。调用者必须让目标 GPU 资源存活到 fence 完成。

启动上传由准备会话统一维护这些约束。请求通过 `shared_ptr<const AssetManager>`
持有 CPU 来源，调用方在准备期间不得修改它。App 使用别名 shared_ptr 让整个
PreparedContent 存活到启用、取消或失败。只有 fence 确认完成的资源计入 completed，
提交后尚未确认的资源只计入 submitted；全部完成后才进入 pipeline 准备阶段。

启动期间只有主线程提交 Vulkan 队列。每批按 4 ms、16 MiB 或 16 个资源的
阈值停止继续添加，且最多保留一个未完成批次；GUI 在批次之间继续绘制。
上传不再调用 `vkQueueWaitIdle()`，buffer 上传包含 transfer-write 到后续读取的
内存屏障，image 上传保留布局和访问屏障。

阈值在完整资源之间检查，属于软预算：单张大贴图或大型 mesh 的内存分配和
staging 拷贝仍可能超出预算。场景管线创建也暂时在主线程进行。当前改动消除
整个模型加载对 GUI 首帧的依赖，并不保证后续每一帧都严格低于 4 ms。

关闭窗口时先请求 CPU 取消并 join，再等待尚未完成的上传 fence，然后释放
目标资源、GUI 和 Vulkan。取消检查位于导入阶段之间及贴图解码前；单次文件
解析或图片解码可以先完成。工作线程从不 detach，也不持有 App 引用。
EditorApp 保证 App 的清理先于 EditorLayer 日志捕获器的销毁。

## 使用与验证

默认启动保持自动加载 demo；`VulkanApp.exe --editor --empty` 打开空编辑器，
禁用自动加载并保持空场景。

```powershell
cmake --preset debug-vs -DRUBIA_ENABLE_GPU_TESTS=ON
cmake --build --preset debug-vs --target VulkanApp
ctest --test-dir build/debug-vs -C Debug --output-on-failure
```

GPU 测试默认不注册，避免无图形环境下自动失败。它们也可以直接执行：

- `--startup-test`：空 GUI、空场景 resize、CPU 取消、缺失模型、重复请求拒绝、
  上传取消与重启、Ready 时 resize 与取消、CPU 来源生命周期和上传中关闭。
- `--render-test` / `--editor-test`：缺失 shader 后的 GUI 和 resize、测试代码发起重试、上传中
  持续绘制 GUI、完整场景绘制；保留原有贴图替换、重导入及 resize 验证。
- `--asset-test`：原有 CPU 资源导入与校验。
