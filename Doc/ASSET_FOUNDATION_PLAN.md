# Asset 基建整理与实施清单

> 状态：进行中  
> 最近更新：2026-08-21  
> 目标：在保留现有 CPU Asset 和 GPU RenderAssetCache 的基础上，补齐 Editor 所需的资产身份、项目数据库、导入事务、依赖关系和重载边界。

## 1. 当前已有能力

当前 Asset 代码已经具备一套可继续演进的运行时基础，不需要推倒重写：

- `AssetHandle<T>`：类型安全的 index + generation 句柄，可拒绝已经回收的槽位。
- `AssetRegistry<T>`：按类型持有 CPU Asset，支持插入、查询、擦除和整体 reset。
- `AssetManager`：统一创建并校验 Texture、Shader、MaterialTemplate、Material、Mesh 和 Model。
- CPU Asset：保存与来源格式无关、可供 GPU 上传的数据。
- Import 层：已有 SPIR-V、WIC Image 和 GLB 的加载/转换路径。
- `RenderAssetCache`：把 Model 可达的 Texture、Material、Mesh 上传为 Vulkan 资源。
- `--asset-test`：可在不创建 Vulkan Window 的情况下验证 GLB 导入路径。

当前职责链大致为：

```text
文件系统
  -> GLBLoader / WicImageDecoder / SpirvShaderImporter
  -> 各格式 Importer
  -> AssetManager（已校验 CPU Asset）
  -> RenderAssetCache（GPU Asset）
  -> Renderer
```

## 2. Editor 使用前的关键缺口

现有系统面向一次性 Demo 加载，尚未形成项目级 Asset 系统：

- Handle 只在本次进程和本个 `AssetManager` 内有效，不能写入 Scene 文件。
- 没有持久化 `AssetId`、资产类型、源文件路径和导入设置。
- 没有按路径或 ID 查找、枚举、筛选资产的数据库。
- 同一个文件可被重复导入，系统不知道两次导入是否代表同一个资产。
- GLB 导入会连续创建多个子资产；中途失败可能留下部分已插入对象。
- `AssetManager` 没有公开的单资产卸载、替换或版本更新接口。
- 没有正向/反向依赖信息，删除 Texture、Material、Mesh 时无法报告引用者。
- `App::createDemoAssets()` 同时负责默认资源、路径解析、文件解码、导入配置和 Scene 创建，职责过多。
- `RenderAssetCache` 目前一次性批量创建，查找为线性搜索，无法自然处理增量加载和重导入。
- CMake 直接复制整个 Asset 目录，尚未区分项目源资产、导入产物和最终 Runtime 包。

## 3. 核心分层决策

### 3.1 AssetId：持久身份

新增非类型化、可序列化的稳定 `AssetId`：

```cpp
struct AssetId
{
    uint64_t high{};
    uint64_t low{};
};
```

它用于：

- Scene、Prefab、Material 等磁盘文件保存资产引用。
- Editor Selection、Asset Browser 和 Inspector 长期保存选择。
- 在多次启动、重新导入和运行时 Handle 变化后仍指向同一个逻辑资产。

`AssetId` 不替代 `AssetHandle<T>`：

```text
AssetId
  = 项目范围、跨进程、可保存的逻辑身份

AssetHandle<T>
  = 当前 AssetManager 内、类型安全、generation-checked 的已加载对象句柄
```

数据库负责完成二者之间的解析。

### 3.2 AssetDatabase：项目资产目录

新增 `AssetDatabase`，负责元数据和加载状态，不直接持有 Vulkan 资源：

```cpp
enum class AssetType
{
    Unknown,
    Texture,
    Shader,
    MaterialTemplate,
    Material,
    Mesh,
    Model
};

struct AssetRecord
{
    AssetId id;
    AssetType type = AssetType::Unknown;
    std::filesystem::path sourcePath;
    std::string subAssetKey;
    std::string displayName;
    uint64_t revision = 0;
    AssetLoadState state = AssetLoadState::Unloaded;
    std::vector<AssetId> dependencies;
};
```

职责边界：

- `AssetDatabase`：扫描、元数据、稳定 ID、查找、枚举、导入状态和依赖关系。
- `AssetManager`：持有并校验当前已加载的 CPU Asset。
- Importer：把某种源格式转换成 CPU Asset，并报告生成的子资产和依赖。
- `RenderAssetCache`：持有 Renderer 所需的 GPU 表示。

### 3.3 AssetManager：保留为运行时对象仓库

不要让现有 `AssetManager` 同时承担文件扫描、序列化和 Editor UI 查询。它继续保持以下性质：

- 不关心数据来自 GLB、PNG、SPIR-V 还是程序生成。
- 创建时验证跨 Asset 引用。
- 通过 typed handle 提供高频运行时访问。
- 后续增加受控的 unload/replace 能力和 revision 通知。

## 4. 源资产、子资产和内建资产

### 4.1 源资产

项目内可导入文件，例如：

```text
Assets/Models/ABeautifulGame.glb
Assets/Textures/Grid.png
Assets/shaders/triangle.vert.spv
```

源文件路径必须：

- 统一规范化为相对 Project Asset Root 的路径。
- 统一处理斜杠、`.`、`..` 和 Windows 大小写比较问题。
- 拒绝逃逸到 Project Asset Root 之外的相对路径。

### 4.2 子资产

一个 GLB 会生成 Model、Mesh、Material 和 Texture。它们需要属于同一个导入结果，并具有可重复匹配的 `subAssetKey`：

```text
model:root
mesh:<source-key>
material:<source-key>
texture:<source-key>
```

`subAssetKey` 不应仅依赖本次 `AssetManager` 的 handle，也不应默认把临时 vector 下标当成永久身份。Importer 应优先使用源格式中的稳定名称/路径；重名时再加可确定的消歧信息。

第一阶段对外主要暴露 Model 的 `AssetId`。Mesh、Material 和 Texture 仍需记录为子资产，以便依赖分析、重导入和后续 Inspector 使用。

### 4.3 内建资产

默认白纹理、默认 PBR Material、MaterialTemplate 和内建 Shader 不应继续散落在 `App::createDemoAssets()`。

新增 `BuiltinAssets` 或等价启动服务，集中创建并持有：

- Default White Texture。
- Default Normal Texture，不能继续使用白纹理代替法线贴图。
- Default Metallic-Roughness Texture。
- Default PBR MaterialTemplate。
- Default/Fallback Material。
- Runtime Present Shader 和默认 Scene Shader。

内建资产使用保留的稳定 ID 或明确标记为 builtin，生命周期长于所有项目资产和 Renderer GPU 缓存。

## 5. 元数据持久化

每个可导入源文件应有可提交到 Git 的 sidecar 元数据，例如：

```text
Assets/Models/Suzanne.glb
Assets/Models/Suzanne.glb.meta
```

元数据至少保存：

```text
schemaVersion
sourceAssetId
importer
importerVersion
importSettings
subAssets: subAssetKey -> AssetId
```

约束：

- 首次发现文件时生成 ID；移动或重命名文件时保留 `.meta` 即保留身份。
- 丢失 `.meta` 意味着创建新身份，应由 Editor 给出提示。
- `.meta` 写入必须采用临时文件 + 原子替换，避免进程中断留下半个文件。
- 元数据格式在开始实现序列化前确定；第一版应包含 `schemaVersion`。
- 导入产物缓存不提交 Git，sidecar 元数据应提交 Git。

## 6. 导入器注册与统一入口

把 `App::createDemoAssets()` 中的手工流程移到 Asset 服务：

```cpp
struct AssetImportRequest
{
    std::filesystem::path sourcePath;
    AssetImportOptions options;
};

struct AssetImportResult
{
    AssetId rootAsset;
    std::vector<AssetId> producedAssets;
    std::vector<AssetImportDiagnostic> diagnostics;
};
```

- [ ] 新增 Importer Registry，根据扩展名和显式 importer 名称选择实现。
- [ ] GLB、Image、SPIR-V 通过统一导入入口进入数据库。
- [ ] Importer 不直接依赖 `App`、Window 或 Renderer。
- [ ] Importer 返回结构化 diagnostics，而不是只拼接异常字符串。
- [ ] 路径解析由 Project/AssetDatabase 负责，不再由 `App` 静态函数兜底。
- [ ] 相同源文件和相同设置在没有变更时避免重复导入。

## 7. 导入事务与失败回滚

GLB 导入属于多资产事务。必须满足“全部成功或全部不生效”：

```text
Decode source
  -> Create textures
  -> Create materials
  -> Create meshes
  -> Create model
  -> Validate complete graph
  -> Commit database records
```

- [ ] 新增 `AssetImportTransaction` 或等价机制。
- [ ] 记录本次创建的 typed handles 和 Asset records。
- [ ] 任一步失败时按 Model -> Mesh -> Material -> Texture 的反向依赖顺序回滚。
- [ ] Commit 前旧版本仍保持可用。
- [ ] 重导入成功后再切换到新版本。
- [ ] diagnostics 记录失败阶段、源路径和具体子资产。

这是 Asset 基建第一阶段的高优先级项目。否则 Editor 中一次失败的重导入会污染当前 AssetManager。

## 8. 依赖与卸载规则

数据库至少维护：

```text
Model -> Mesh
Mesh -> Material
Material -> MaterialTemplate
Material -> Texture
MaterialTemplate -> Shader
```

- [ ] 保存正向 dependencies。
- [ ] 可查询 reverse dependencies。
- [ ] 删除资产前报告所有直接和间接引用者。
- [ ] 默认拒绝删除仍被引用的资产。
- [ ] 卸载一个 GLB 导入结果时按反向依赖顺序处理其子资产。
- [ ] 共享资产只在不再被任何所有者或依赖者使用时卸载。
- [ ] Scene 引用失效时保留原 `AssetId` 并显示 Missing Asset，而不是静默替换。

## 9. 加载状态和错误模型

第一版保持同步加载，但接口不要把同步实现写死：

```cpp
enum class AssetLoadState
{
    Unloaded,
    Loading,
    Ready,
    Failed
};
```

- [ ] `load(AssetId)` 返回或解析为对应 typed handle。
- [ ] `unload(AssetId)` 受依赖和在途 GPU 使用约束。
- [ ] `reload(AssetId)` 采用事务式替换。
- [ ] 数据库保存最近一次结构化错误。
- [ ] Asset Browser 可显示 Unloaded、Loading、Ready、Failed 和 Missing Source。
- [ ] 第一阶段不实现后台线程导入；先保证同步路径正确、可测试。

## 10. Renderer 边界

`RenderAssetCache` 不负责扫描磁盘或决定导入器。它只消费已经验证的 CPU Asset。

第一阶段可以接受安全但较重的重建策略：

```text
Asset revision changed
  -> wait renderer idle
  -> rebuild affected RenderAssetCache
```

之后再演进为：

- 按 handle 的哈希索引，替换当前线性查找。
- `ensureTexture()`、`ensureMaterial()`、`ensureMesh()` 式按需上传。
- 用 Asset revision 判断 GPU 表示是否失效。
- 旧 GPU 资源按 Frame/Fence 延迟销毁，不在 in-flight 时立即释放。

不要在 Asset 基建第一步同时实现异步上传、热重载和无停顿资源替换。

## 11. Scene 的资产引用

Scene 的磁盘数据必须保存 `AssetId`，不能保存 `ModelAssetHandle`：

```text
Serialized SceneNode
  -> Model AssetId

Loaded SceneNode / Render Extraction
  -> resolved ModelAssetHandle
```

- [ ] 定义 unresolved 和 resolved 引用边界。
- [ ] Scene 加载时通过 AssetDatabase 解析引用。
- [ ] Model 未加载或导入失败时节点仍可存在，并显示 Missing Asset。
- [ ] 资产重导入后 Scene 中的持久 ID 不变化。
- [ ] Runtime 构建可在加载阶段把 AssetId 批量解析为 typed handle。

## 12. Asset Browser 所需查询能力

AssetDatabase 至少提供：

- [ ] 按 `AssetId` 查询记录。
- [ ] 按规范化 source path 查询源资产。
- [ ] 枚举全部顶层资产。
- [ ] 枚举一个源文件产生的子资产。
- [ ] 按 `AssetType` 筛选。
- [ ] 按名称和路径搜索。
- [ ] 查询 dependencies 和 reverse dependencies。
- [ ] 查询当前 load state、revision 和 diagnostics。

Editor 面板枚举 `AssetRecord`，而不是遍历 `AssetManager` 的内部 slot。

## 13. 测试清单

- [x] `AssetId` 文本序列化往返测试。
- [ ] 路径规范化和 Asset Root 逃逸测试。
- [ ] `.meta` 创建、读取、版本和损坏文件测试。
- [ ] 同一路径重复扫描不会生成新 ID。
- [ ] 文件连同 `.meta` 移动后 ID 保持不变。
- [ ] `AssetHandle<T>` 过期 generation 继续被拒绝。
- [ ] GLB 导入失败后 AssetManager 和 AssetDatabase 均无残留。
- [ ] 重导入失败时旧版本仍然可用。
- [ ] 删除被引用资产时能列出引用者并拒绝操作。
- [ ] GLB 的 Model/Mesh/Material/Texture 依赖图正确。
- [ ] Runtime Render Test 在 AssetDatabase 接入后保持通过。

## 14. 推荐实施顺序

### Phase A：身份与数据库骨架

- [x] `AssetId`、哈希和字符串转换。
- [ ] `AssetType`、`AssetRecord`、`AssetLoadState`。
- [ ] Project Asset Root 和路径规范化。
- [ ] `AssetDatabase` 的 register/find/enumerate 基础 API。
- [ ] `.meta` schema 和持久化。

### Phase B：统一导入与事务

- [ ] Importer Registry。
- [ ] 把 SPIR-V、WIC/Texture、GLB 接入统一入口。
- [ ] 多子资产稳定 ID 映射。
- [ ] 导入事务和失败回滚。
- [ ] 将 `createDemoAssets()` 拆为 BuiltinAssets + 项目模型导入。

### Phase C：依赖与 Scene 接入

- [ ] 正向/反向依赖图。
- [ ] 受控 unload/delete/reload。
- [ ] Scene 使用持久 `AssetId` 保存模型引用。
- [ ] Missing Asset 状态。

### Phase D：Renderer 与 Editor 接入

- [ ] revision 变化触发安全的 GPU Cache 重建。
- [ ] Asset Browser 查询接口和基础面板。
- [ ] Inspector 的资产选择器。
- [ ] Scene Hierarchy/Inspector 通过数据库解析 Model 资产。

### Phase E：后续优化

- [ ] 增量 GPU Cache。
- [ ] Fence-aware 延迟销毁。
- [ ] 文件监视和自动重导入。
- [ ] 后台导入与异步上传。
- [ ] 导入产物缓存和 Runtime Asset 包。

## 15. 第一阶段完成标准

Asset 基建可以支撑 Editor 的最低标准为：

- 项目文件具有跨启动稳定的 `AssetId`。
- 可按 ID、路径、类型和名称枚举查询资产。
- GLB 的 Model 及其子资产由一次事务创建，失败无残留。
- 相同源文件不会因重复扫描而生成另一套逻辑资产。
- Scene 能保存 Model `AssetId`，加载时解析为当前 typed handle。
- 删除资产前可以判断并报告 Scene 或其他资产引用。
- Asset 导入测试和 Vulkan Render Test 保持通过。
