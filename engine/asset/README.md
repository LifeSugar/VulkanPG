# Shader Program 与材质模板

CPU 资产依赖顺序：`ShaderAsset → ShaderProgramAsset → MaterialTemplateAsset → MaterialAsset`。
`AssetManager` 拥有各类型注册表，资产间只保存带 generation 的句柄。

## 创建

```cpp
const auto program = assets.createShaderProgram({
    "PBR", {vertexShader, fragmentShader}
});

asset::MaterialTemplateAsset::CreateInfo info;
info.name = "PBR Material";
info.program = program;
info.textureSlots = {
    {"baseColorTexture", "baseColorTexture", "baseColorSampler"}
};
const auto materialTemplate = assets.createMaterialTemplate(info);
```

示例中的纹理元数据依次是材质槽名称、shader image 名称、shader sampler 名称。
其余反射出的 image/sampler 也须提供配对；不存在的名称和重复绑定都会报错。
纹理槽索引按元数据顺序生成，保持导入器的语义顺序。

模板的 `CreateInfo` 不接受参数类型、offset、buffer 大小或 binding 数字。
`MaterialTemplateBuilder` 从 Program 的 `materialSet`（默认 1）生成这些字段，
保留反射的完整 buffer 大小，包括 padding。默认所有参数必填，
`info.parameters = {{"roughnessFactor", false}}` 可将指定参数改为可选。
未赋值的可选参数初始化为零。纹理用途、默认贴图和颜色空间仍由导入逻辑提供。

## 反射、合并与校验

- importer 中的 SPIRV-Cross 为单阶段生成 `ShaderInterface`，不创建 GPU 对象。
- `ShaderProgramBuilder` 仅依赖 CPU 资产，校验 VS/PS 阶段组合和阶段间输入输出。
- 资源按 `(set, binding)` 合并，保留阶段 mask 和资源名称别名。
- 同一绑定要求资源类型、数组数量和 buffer 物理布局一致；共享 buffer 成员名称也必须一致。
- Buffer 物理签名包含嵌套结构、数组/矩阵步长。Push constant 保留 offset、大小和阶段。
- 材质模板只提取材质 set；相机和对象等其他 set 的资源保留在 Program 中。

当前支持 VS + PS 图形 Program。材质布局支持一个 UBO、标量/向量、列主序且
步长为 16 的 float4x4，以及一对一的独立 image/sampler 槽。材质参数数组、
嵌套结构、描述符数组、共享 sampler 和多个材质 UBO 会被明确拒绝。
Program 可以描述其他 set 中的 storage buffer，但尚未提供 Compute Program。

Vulkan 默认 pipeline 工厂从 Program 取得阶段字节码；材质描述符布局使用生成的
绑定和阶段 mask。顶点 buffer 布局、帧数据接口和 draw push constant 的录制约定
仍由当前 renderer 提供。材质 GPU 后端目前固定使用 set 1。

## 签名与生命周期

- `codeSignature`：各阶段字节码与入口点。
- `layoutSignature`：描述符布局与 push constant 范围，供后端布局复用判断。
- `interfaceSignature`：布局、资源名称、buffer 结构、阶段输入输出。
- 模板 `schemaSignature`：生成后的材质参数、纹理槽和必填规则。

Program 和 Shader 经 AssetManager 发布后只读。新 shader 内容须创建新 Shader 和
Program，构建失败不会修改旧资产。当前没有自动热重载或旧材质数据迁移；模板更换
后需要重新创建材质，按参数名称赋值。`reset()` 按依赖逆序释放并使旧句柄失效。

## 验证

`shader-program-test` 覆盖接口合并、冲突、签名、失效句柄、反射布局变更后的
材质打包和纹理映射。`asset-smoke-test` 使用真实 PBR SPIR-V 验证完整资产导入；
Runtime/Editor smoke tests 验证 GPU 消费路径。
