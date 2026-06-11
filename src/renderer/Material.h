//
// Created by lance on 2023/3/20.
//

#ifndef MORROW_SHADERMATERIAL_H
#define MORROW_SHADERMATERIAL_H

#include <vector>
#include <string>
#include <unordered_map>
#include <variant>

#include "Texture.h"
#include "Shader.h"
#include "RenderDeviceProxyBase.h"
#include "Scene3DUBO.h"
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
    void setShaderFromMemory(const std::string& shaderName,
                             const std::string& vertexSource,
                             const std::string& fragmentSource);

    std::string getShaderName() const;

    GPUProgramHandle* getShader() const;

    GPUProgramHandle* getBatchShader() const;

    void setBlendEnabled(bool enabled);

    void setBlendFunc(int srcFactor, int dstFactor);

    bool isBlendEnabled() const;

    void getBlendFunc(int& srcFactor, int& dstFactor) const;

    void setDoubleSided(bool doubleSided);

    bool isDoubleSided() const;

    void setScene3DMaterialUBO(const Scene3DMaterialUBO& materialData);

    const Scene3DMaterialUBO& getScene3DMaterialUBO() const;

    void bindScene3DMaterialUBO(GPUProgramHandle* shader = nullptr);

    // 应用材质参数到Shader
    void apply(GPUProgramHandle* shader = nullptr);

    void applyBatch(GPUProgramHandle* shader = nullptr);

    bool isEqual(std::shared_ptr<Material>  other);

    bool operator==(const Material& other) const;

    bool isSSBOShader() const;

private:
    Material();

    explicit Material(const std::string& shaderName);

    void loadShader();

    GPUProgramHandle* buildShader(bool enableSSBO = false);

    GPUProgramHandle* m_shader = nullptr;
    GPUProgramHandle* m_batchShader = nullptr;
    std::unordered_map<std::string, TextureSharedPtr> m_textureMap;
    std::unordered_map<std::string, VectorVariant> m_vectorMap;
    std::unordered_map<std::string, float> m_floatMap;
    std::unordered_map<std::string, int32_t> m_intMap;
    std::unordered_map<std::string, Matrix4> m_matrix4Map;
    std::unordered_map<std::string, IntArrayData> m_intArrayMap;
    std::string m_shaderName;
    std::string m_attributePrefix = "u_";
    bool m_blendEnabled = true;
    int m_srcBlendFactor = 1; // 默认为GL_ONE
    int m_dstBlendFactor = 0; // 默认为GL_ZERO
    bool m_doubleSided = false;
    UBO* m_scene3DMaterialUbo = nullptr;
    Scene3DMaterialUBO m_scene3DMaterialData{};
    bool m_scene3DMaterialDirty = true;
    std::vector<TextureSharedPtr> m_textures{};
    std::vector<std::string> m_defines =  {"ENABLE_SSBO"};
    std::string m_vertexShaderResource;
    std::string m_fragmentShaderResource;
};

using MaterialSharedPtr = std::shared_ptr<Material>;
}


#endif //MORROW_SHADERMATERIAL_H
