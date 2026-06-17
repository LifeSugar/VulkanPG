#pragma once

#include "GLBTypes.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>

// ============================================================================
// GLBLoader —— 将 glTF 2.0 / GLB 文件解析为 GLBModel
// ============================================================================
// 内部使用 Assimp 完成解析，再提取为简洁的下游数据结构。
// 用法：
//   GLBLoader loader;
//   auto model = loader.load("path/to/model.glb");
//   if (model) { /* 使用 model->meshes / materials / rootNode ... */ }
// ============================================================================

class GLBLoader {
public:
    GLBLoader();
    ~GLBLoader();

    // 禁止拷贝（内部有 Assimp::Importer 成员）
    GLBLoader(const GLBLoader&) = delete;
    GLBLoader& operator=(const GLBLoader&) = delete;

    // ── 加载 ────────────────────────────────────────────────────────────────
    // 返回 nullptr 表示加载失败，可通过 getLastError() 获取错误信息
    std::unique_ptr<GLBModel> load(const std::string& filePath);

    // ── 错误信息 ────────────────────────────────────────────────────────────
    const std::string& getLastError() const { return m_lastError; }

private:
    // ── 内部转换方法 ────────────────────────────────────────────────────────
    void extractMeshes(const void* aiScenePtr);
    void extractMaterials(const void* aiScenePtr);
    void extractTextures(const void* aiScenePtr);
    void extractNode(const void* aiNodePtr, GLBNode& outNode);
    void extractAnimations(const void* aiScenePtr);
    void extractSkins(const void* aiScenePtr);

    // 工具方法
    static glm::mat4 convertMatrix(const void* aiMatrix4x4Ptr);
    static glm::vec3 convertVec3(const void* aiVector3DPtr);
    static glm::vec4 convertVec4(const void* aiColor4DPtr);
    int              findOrAddTexture(const std::string& uri);

    // ── 状态 ────────────────────────────────────────────────────────────────
    class Impl;  // pimpl 隐藏 Assimp 头文件
    std::unique_ptr<Impl> m_impl;

    std::unique_ptr<GLBModel> m_model;
    std::string               m_lastError;
};
