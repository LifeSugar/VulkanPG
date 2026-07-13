# Hello Triangle → 模型渲染器 进化路线图

## 阶段 1：顶点缓冲 & 索引缓冲（VBO / IBO）

- [ ] **Shader 改造**
  - [ ] `triangle.vert.hlsl`：新增 `VSInput` 结构体（`POSITION`, `NORMAL`, `TEXCOORD`），移除 `SV_VertexID` 逻辑
  - [ ] 重新编译 SPIR-V
- [ ] **VKApp：创建 Vertex/Index Buffer**
  - [ ] 添加 `VkBuffer m_vertexBuffer` / `VkDeviceMemory m_vertexMemory`
  - [ ] 添加 `VkBuffer m_indexBuffer` / `VkDeviceMemory m_indexMemory`
  - [ ] 实现 `createBuffer()` 工具函数（含 staging buffer 上传）
  - [ ] 实现 `createVertexBuffer()` / `createIndexBuffer()`（从 `GLBPrimitive` 填充数据）
- [ ] **VKApp：Pipeline 绑定顶点数据**
  - [ ] 定义 `VkVertexInputBindingDescription`（对应 `GLBVertex`）
  - [ ] 定义 `VkVertexInputAttributeDescription[]`（position + normal + texcoord）
  - [ ] `createGraphicsPipeline()` 中启用 `pVertexInputState`
- [ ] **VKApp：CommandBuffer 改用索引绘制**
  - [ ] `vkCmdBindVertexBuffers()` + `vkCmdBindIndexBuffer()`
  - [ ] `vkCmdDraw()` → `vkCmdDrawIndexed()`

---

## 阶段 2：Uniform Buffer（MVP 矩阵 + Camera）

- [ ] **Shader 改造**
  - [ ] `triangle.vert.hlsl`：新增 `cbuffer UniformBufferObject { float4x4 mvp; }`
  - [ ] VS 中将顶点变换到裁剪空间
  - [ ] 重新编译 SPIR-V
- [ ] **VKApp：UBO 基础设施**
  - [ ] 添加 `std::vector<VkBuffer> m_uniformBuffers` / `std::vector<VkDeviceMemory> m_uniformBuffersMemory`
  - [ ] 实现 `createUniformBuffers()`（每帧一个 UBO，大小 = `sizeof(UniformBufferObject)`）
- [ ] **VKApp：Descriptor Set**
  - [ ] 实现 `createDescriptorSetLayout()`（1 个 UBO binding）
  - [ ] 实现 `createDescriptorPool()`
  - [ ] 实现 `createDescriptorSets()`（每帧一个 set）
  - [ ] `createGraphicsPipeline()` 中 `pipelineLayout` 绑定 descriptorSetLayout
- [ ] **VKApp：Camera**
  - [ ] 定义 `UniformBufferObject` 结构体（MVP 矩阵）
  - [ ] 实现简易 orbit camera 或静态 view/proj 矩阵
  - [ ] 每帧 `updateUniformBuffer()` 写入 MVP 数据

---

## 阶段 3：深度缓冲

- [ ] **VKApp：深度资源**
  - [ ] 添加 `VkImage m_depthImage` / `VkDeviceMemory m_depthImageMemory` / `VkImageView m_depthImageView`
  - [ ] 实现 `createDepthResources()`（`VK_FORMAT_D32_SFLOAT`）
  - [ ] 实现 `findDepthFormat()` 辅助函数
- [ ] **RenderPass 改造**
  - [ ] `createRenderPass()` 新增 depth attachment（`loadOp=CLEAR`, `storeOp=DONT_CARE`）
  - [ ] Subpass 添加 `pDepthStencilAttachment`
- [ ] **Framebuffer 改造**
  - [ ] `createFramebuffers()` 中附加 depth imageView
- [ ] **Pipeline 开启深度测试**
  - [ ] `createGraphicsPipeline()` 添加 `VkPipelineDepthStencilStateCreateInfo`
  - [ ] `depthTestEnable=VK_TRUE`, `depthWriteEnable=VK_TRUE`
- [ ] **CommandBuffer**
  - [ ] `VkClearValue` 增加 depth clear value

---

## 阶段 4：场景图遍历 & 多 Mesh 渲染

- [ ] **VKApp：加载模型**
  - [ ] `VKApp` 持有 `std::unique_ptr<GLBModel> m_model`
  - [ ] `initVulkan()` 末尾调用 `GLBLoader::load()` 加载模型
- [ ] **VKApp：场景图递归**
  - [ ] 实现 `renderNode(GLBNode& node, glm::mat4 parentTransform)`
  - [ ] 遍历 `node.meshIndices`，为每个 primitive 设置 model 矩阵后 draw
  - [ ] 递归处理 `node.children`
- [ ] **CommandBuffer 动态录制**
  - [ ] 每帧重新录制 command buffer（或在录制时遍历场景图）
  - [ ] 每个 primitive 调用一次 `vkCmdDrawIndexed()`
- [ ] **UBO 拆分**
  - [ ] `UniformBufferObject` 拆为 `model` + `viewProj`，viewProj 一帧只写一次

---

## 阶段 5：基础材质（Push Constants）

- [ ] **Shader 改造**
  - [ ] `triangle.frag.hlsl`：新增 `pushConstant { float4 baseColor; ... }`
  - [ ] Fragment Shader 实现简单 Blinn-Phong 光照（方向光 + ambient）
  - [ ] 重新编译 SPIR-V
- [ ] **VKApp：Push Constants**
  - [ ] `createGraphicsPipeline()` 的 `pipelineLayout` 添加 `VkPushConstantRange`
  - [ ] 每个 primitive 绘制前 `vkCmdPushConstants()` 传入该 primitive 的 material 参数
- [ ] **从 GLBMaterial 提取数据**
  - [ ] 读取 `GLBMaterial::baseColorFactor` 等字段传给 push constant

---

## 阶段 6：纹理

- [ ] **纹理加载**
  - [ ] 引入 `stb_image.h`
  - [ ] 实现 `createTextureImage()`（staging buffer → `VkImage`）
  - [ ] 实现 `createTextureImageView()` / `createTextureSampler()`
- [ ] **Descriptor Set 扩展**
  - [ ] `createDescriptorSetLayout()` 新增 combined image sampler binding
  - [ ] `createDescriptorSets()` 绑定纹理 imageView + sampler
  - [ ] 每个 material 的 baseColorTexture 对应一个 descriptor set（或按材质数量创建）
- [ ] **Shader 改造**
  - [ ] `triangle.frag.hlsl`：新增 `Texture2D` + `SamplerState`，PBR 使用纹理颜色

---

## 阶段 7（远期）：骨骼动画 & Skinning

- [ ] 新增 skinning shader（vertex shader 中蒙皮计算）
- [ ] 上传 bone matrices 到 UBO / SSBO
- [ ] 从 `GLBAnimation` 驱动动画播放

---

## 架构优化（可穿插进行）

- [ ] 拆分 `VKApp.cpp`：`VKBuffers.cpp` / `VKPipeline.cpp` / `VKDescriptor.cpp` / `VKTexture.cpp`
- [ ] 封装 `VKHelpers.h` 公共工具函数（`createBuffer`, `copyBuffer`, `createImage` 等）
- [x] 添加 swapchain recreation（窗口 resize 时自动重建，并在连续拖动时防抖）
- [ ] 添加 ImGui 用于调试面板（显示 FPS、模型信息等）
