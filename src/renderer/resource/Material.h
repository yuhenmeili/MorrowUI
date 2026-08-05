//
// Created by lance on 2023/3/20.
//

#ifndef MORROW_SHADERMATERIAL_H
#define MORROW_SHADERMATERIAL_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "RenderDeviceProxyBase.h"
#include "Scene3DUBO.h"
#include "Texture.h"
#include "Shader.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace morrow {
struct IntArrayData {
    std::string name;
    int32_t* data;
    int32_t size;
    int32_t step;
};
using VectorVariant = std::variant<Vector2, Vector3, Vector4>;

class Material : public std::enable_shared_from_this<Material> {
public:
    static std::shared_ptr<Material> create();

    static std::shared_ptr<Material> create(const std::string& shaderName);

    // Texture相关方法
    void setTexture(const std::string& name, const TextureSharedPtr& texture);

    TextureSharedPtr getTexture(const std::string& name) const;

    bool hasTexture(const std::string& name) const;

    // Vector相关方法
    void setVector(const std::string& name, const Vector2& value);

    void setVector(const std::string& name, const Vector3& value);

    void setVector(const std::string& name, const Vector4& value);

    VectorVariant getVector(const std::string& name) const;

    void setMatrix4(const std::string& name, const Matrix4& matrix);

    Matrix4 getMatrix4(const std::string& name) const;

    // Float相关方法
    void setFloat(const std::string& name, float value);

    float getFloat(const std::string& name) const;

    // ── 类型安全读取（SSBO 字段绑定 / 声明式打包用，P2）──
    // 缺失或类型不匹配时返回 false / 回退默认值，不抛 std::bad_variant_access。
    bool tryGetFloat(std::string_view name, float& value) const;

    float getFloatOr(std::string_view name, float defaultValue) const;

    bool tryGetVector4(std::string_view name, Vector4& value) const;

    Vector4 getVector4Or(std::string_view name, const Vector4& defaultValue) const;

    /// 读取 Vector2/3/4 的第 component 个分量（越界返回 false）
    bool tryGetVectorComponent(std::string_view name, int component, float& value) const;

    // Int相关方法
    void setInt(const std::string& name, int32_t value);

    int32_t getInt(const std::string& name) const;

    void setBool(const std::string& name, bool value);

    bool getBool(const std::string& name) const;

    void setIntArray(const std::string& name, const int32_t* values, int32_t size, int32_t step);

    // Shader相关方法
    // void setShader(const ShaderSharedPtr& shader);
    void setShader(const std::string& shaderName);

    /// 从内存直接设置着色器源码（绕过文件IO，用于安全组件 ROM 硬编码）
    /// @param shaderName  着色器标识名
    /// @param vertexSource  顶点着色器 GLSL 源码
    /// @param fragmentSource  片元着色器 GLSL 源码
    void setShaderFromMemory(const std::string& shaderName, const std::string& vertexSource, const std::string& fragmentSource);

    std::string getShaderName() const;

    HwGPUProgram getShader() const;

    HwGPUProgram getBatchShader() const;

    void setBlendEnabled(bool enabled);

    void setBlendFunc(BlendFactor srcRgbFactor, BlendFactor dstRgbFactor, BlendFactor srcAlphaFactor, BlendFactor dstAlphaFactor);

    bool isBlendEnabled() const;

    void getBlendFunc(BlendFactor& srcRgbFactor, BlendFactor& dstRgbFactor, BlendFactor& srcAlphaFactor, BlendFactor& dstAlphaFactor) const;

    void setDoubleSided(bool doubleSided);

    bool isDoubleSided() const;

    void setDepthTestEnabled(bool enabled);

    void setDepthWriteEnabled(bool enabled);

    void setScene3DMaterialUBO(const Scene3DMaterialUBO& materialData);

    const Scene3DMaterialUBO& getScene3DMaterialUBO() const;

    void bindScene3DMaterialUBO(HwGPUProgram shader = HwGPUProgram{0});

    // 应用材质参数到Shader
    void apply(HwGPUProgram shader = HwGPUProgram{0});

    void applyBatch(HwGPUProgram shader = HwGPUProgram{0});

    bool isEqual(std::shared_ptr<Material> other);

    bool operator==(const Material& other) const;

    /// CPU-only batch compatibility fingerprint. Does not create or access GPU resources.
    uint64_t getBatchCompatibilityHash() const;

    uint64_t getBatchCompatibilityRevision() const;

    uint64_t getUniformRevision() const;

    /// Rendering fingerprint including material state and referenced textures.
    uint64_t getRenderRevisionHash() const;

    /// Compatibility alias for existing callers.
    uint64_t getRevision() const;

    bool isSSBOShader() const;

private:
    Material();

    explicit Material(const std::string& shaderName);

    void loadShader();

    HwGPUProgram buildShader(bool enableSSBO = false);

    HwGPUProgram m_shader{0};
    HwGPUProgram m_batchShader{0};
    std::unordered_map<std::string, TextureSharedPtr> m_textureMap;
    std::unordered_map<std::string, VectorVariant> m_vectorMap;
    std::unordered_map<std::string, float> m_floatMap;
    std::unordered_map<std::string, int32_t> m_intMap;
    std::unordered_map<std::string, Matrix4> m_matrix4Map;
    std::unordered_map<std::string, IntArrayData> m_intArrayMap;
    std::string m_shaderName;
    std::string m_attributePrefix = "u_";
    GraphicsPipelineState m_pipelineState;
    HwUBO m_scene3DMaterialUbo{0};
    Scene3DMaterialUBO m_scene3DMaterialData{};
    bool m_scene3DMaterialDirty = true;
    std::vector<std::string> m_defines = {"ENABLE_SSBO"};
    std::string m_vertexShaderResource;
    std::string m_fragmentShaderResource;
    uint64_t m_batchCompatibilityRevision = 1;
    uint64_t m_uniformRevision = 1;
    mutable uint64_t m_cachedBatchCompatibilityRevision = 0;
    mutable uint64_t m_cachedBatchCompatibilityHash = 0;
};

using MaterialSharedPtr = std::shared_ptr<Material>;
}  // namespace morrow

#endif  // MORROW_SHADERMATERIAL_H
