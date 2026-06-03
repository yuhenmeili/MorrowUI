//
// Created by lance on 2023/3/20.
//

#include "Material.h"
#include <algorithm>
#include <cstring>

#include "GlobalObject.h"
#include "MaterialUtil.h"

namespace morrow {
MaterialSharedPtr Material::create() {
    return MaterialSharedPtr(new Material());
}

MaterialSharedPtr Material::create(const std::string& shaderName) {
    return MaterialSharedPtr(new Material(shaderName));
}

Material::Material() = default;

Material::Material(const std::string& shaderName) : m_shaderName(shaderName) {
    loadShader();
}

void Material::setTexture(const std::string& name, const TextureSharedPtr& texture) {
    m_textureMap[m_attributePrefix + name] = texture;
    
    // 同时更新vector中的textures
    auto it = std::find(m_textures.begin(), m_textures.end(), texture);
    if (it == m_textures.end()) {
        m_textures.push_back(texture);
    }
}

TextureSharedPtr Material::getTexture(const std::string& name) const {
    auto it = m_textureMap.find(m_attributePrefix + name);
    if (it != m_textureMap.end()) {
        return it->second;
    }
    return nullptr;
}

bool Material::hasTexture(const std::string& name) const {
    return m_textureMap.find(m_attributePrefix + name) != m_textureMap.end();
}

void Material::setVector(const std::string& name, const Vector2& value) {
    m_vectorMap[m_attributePrefix + name] = value;
}

void Material::setVector(const std::string& name, const Vector3& value) {
    m_vectorMap[m_attributePrefix + name] = value;
}

void Material::setVector(const std::string& name, const Vector4& value) {
    m_vectorMap[m_attributePrefix + name] = value;
}

VectorVariant Material::getVector(const std::string& name) const {
    auto it = m_vectorMap.find(m_attributePrefix + name);
    if (it != m_vectorMap.end()) {
        return it->second;
    }
    return Vector4::ZERO;
}

void Material::setMatrix4(const std::string& name, const Matrix4& matrix) {
    m_matrix4Map[m_attributePrefix + name] = matrix;
}

Matrix4 Material::getMatrix4(const std::string& name) const {
    auto it = m_matrix4Map.find(m_attributePrefix + name);
    if (it != m_matrix4Map.end()) {
        return it->second;
    }
    return Matrix4(); // 返回单位矩阵
}


void Material::setFloat(const std::string& name, float value) {
    m_floatMap[m_attributePrefix + name] = value;
}

float Material::getFloat(const std::string& name) const {
    auto it = m_floatMap.find(m_attributePrefix + name);
    if (it != m_floatMap.end()) {
        return it->second;
    }
    return 0.0f;
}

void Material::setInt(const std::string& name, int32_t value) {
    m_intMap[m_attributePrefix + name] = value;
}

int32_t Material::getInt(const std::string& name) const {
    auto it = m_intMap.find(m_attributePrefix + name);
    if (it != m_intMap.end()) {
        return it->second;
    }
    return 0;
}

void Material::setBool(const std::string& name, bool value) {
    m_intMap[m_attributePrefix + name] = value ? 1 : 0;
}

bool Material::getBool(const std::string& name) const {
    auto it = m_intMap.find(m_attributePrefix + name);
    if (it != m_intMap.end()) {
        return it->second != 0;
    }
    return false;
}

void Material::setIntArray(const std::string& name, const int32_t* values, int32_t size, int32_t step) {
    IntArrayData int_array_data;
    int_array_data.name = m_attributePrefix + name;
    int_array_data.data = const_cast<int32_t*>(values);
    int_array_data.size = size;
    int_array_data.step = step;
    m_intArrayMap[int_array_data.name] = int_array_data;
}

// void ShaderMaterial::setShader(const ShaderSharedPtr& shader) {
//     m_shader = shader;
// }

void Material::setShader(const std::string& shaderName) {
    m_shaderName = shaderName;
    loadShader();
}

std::string Material::getShaderName() const {
    return m_shaderName;
}

GPUProgramHandle* Material::getShader() const {
    return m_shader;
}

GPUProgramHandle* Material::getBatchShader() const {
    return m_batchShader;
}

void Material::setBlendEnabled(bool enabled) {
    m_blendEnabled = enabled;
}

void Material::setBlendFunc(int srcFactor, int dstFactor) {
    m_srcBlendFactor = srcFactor;
    m_dstBlendFactor = dstFactor;
}

bool Material::isBlendEnabled() const {
    return m_blendEnabled;
}

void Material::getBlendFunc(int& srcFactor, int& dstFactor) const {
    srcFactor = m_srcBlendFactor;
    dstFactor = m_dstBlendFactor;
}

void Material::setDoubleSided(bool doubleSided) {
    m_doubleSided = doubleSided;
}

bool Material::isDoubleSided() const {
    return m_doubleSided;
}

void Material::setScene3DMaterialUBO(const Scene3DMaterialUBO& materialData) {
    m_scene3DMaterialData = materialData;
    m_scene3DMaterialDirty = true;
}

const Scene3DMaterialUBO& Material::getScene3DMaterialUBO() const {
    return m_scene3DMaterialData;
}

void Material::bindScene3DMaterialUBO(GPUProgramHandle* shader) {
    GPUProgramHandle* targetShader = shader ? shader : m_shader;
    if (!targetShader) {
        return;
    }

    if (!m_scene3DMaterialUbo) {
        m_scene3DMaterialUbo = RENDERINGTHREAD->createUBO();
        m_scene3DMaterialDirty = true;
    }

    if (m_scene3DMaterialDirty) {
        RENDERINGTHREAD->updateUBO(m_scene3DMaterialUbo, MaterialUtil::makeUBOData(m_scene3DMaterialData));
        m_scene3DMaterialDirty = false;
    }

    RENDERINGTHREAD->bindUBO(targetShader, m_scene3DMaterialUbo, "Scene3DMaterial", kScene3DMaterialBindingPoint);
}

void Material::apply(GPUProgramHandle* shader) {
    if (!shader && (!m_shader || m_shader->getProgramFileName() != m_shaderName)) {
        m_shader = buildShader();
    }
    GPUProgramHandle* targetShader = shader ? shader : m_shader;
    if (!targetShader) {
        LOG_E("shader is missing");
        return;
    }

    if (m_blendEnabled) {
        RENDERINGTHREAD->enableBlend();
    } else {
        RENDERINGTHREAD->disableBlend();
    }

    RENDERINGTHREAD->useGPUProgram(targetShader);

    // 应用纹理
    int textureIndex = 0;
    for (const auto& pair : m_textureMap) {
        auto texture = pair.second;
        texture->render(nullptr);
        texture->bindTexture(textureIndex);
        targetShader->setInt(pair.first, textureIndex);
        textureIndex++;
    }

    // 应用向量
    for (const auto& pair : m_vectorMap) {
        std::visit([targetShader, &pair](const auto& vec) {
            using T = std::decay_t<decltype(vec)>;
            if constexpr (std::is_same_v<T, Vector2>) {
                targetShader->setVec2(pair.first, vec);
            } else if constexpr (std::is_same_v<T, Vector3>) {
                targetShader->setVec3(pair.first, vec);
            } else if constexpr (std::is_same_v<T, Vector4>) {
                targetShader->setVec4(pair.first, vec);
            }
        }, pair.second);
    }

    // 应用浮点数
    for (const auto& pair : m_floatMap) {
        targetShader->setFloat(pair.first, pair.second);
    }

    // 应用整数
    for (const auto& pair : m_intMap) {
        targetShader->setInt(pair.first, pair.second);
    }

    // 应用矩阵
    for (const auto& pair : m_matrix4Map) {
        targetShader->setMat4(pair.first, pair.second);
    }

    //intArrayData
    for (const auto& pair : m_intArrayMap) {
        targetShader->setIntArray(pair.first, pair.second.data, pair.second.size, pair.second.step);
    }
}

void Material::applyBatch(GPUProgramHandle* shader) {
    if (!shader && (!m_batchShader || m_batchShader->getProgramFileName() != m_shaderName)) {
        m_batchShader = buildShader(true);
    }
    GPUProgramHandle* targetShader = shader ? shader : m_batchShader;
    if (!targetShader) {
        LOG_E("shader is missing");
        return;
    }

    if (m_blendEnabled) {
        RENDERINGTHREAD->enableBlend();
    } else {
        RENDERINGTHREAD->disableBlend();
    }

    RENDERINGTHREAD->useGPUProgram(targetShader);

    // 应用纹理
    int textureIndex = 0;
    for (const auto& pair : m_textureMap) {
        auto texture = pair.second;
        texture->render(nullptr);
        texture->bindTexture(textureIndex);
        targetShader->setInt(pair.first, textureIndex);
        textureIndex++;
    }
}

bool Material::isEqual(std::shared_ptr<Material> other) {
    if (!other) {
        return false;
    }
    if (m_shaderName != other->m_shaderName ||
        m_blendEnabled != other->m_blendEnabled ||
        m_srcBlendFactor != other->m_srcBlendFactor ||
        m_dstBlendFactor != other->m_dstBlendFactor ||
        m_doubleSided != other->m_doubleSided ||
        m_textureMap.size() != other->m_textureMap.size()) {
        return false;
    }

    for (const auto& [name, texture] : m_textureMap) {
        auto it = other->m_textureMap.find(name);
        if (it == other->m_textureMap.end() || it->second != texture) {
            return false;
        }
    }

    return true;
}

bool Material::operator==(const Material& other) const {
    if (m_shaderName != other.m_shaderName ||
        m_blendEnabled != other.m_blendEnabled ||
        m_srcBlendFactor != other.m_srcBlendFactor ||
        m_dstBlendFactor != other.m_dstBlendFactor ||
        m_doubleSided != other.m_doubleSided ||
        m_textureMap.size() != other.m_textureMap.size()) {
        return false;
    }

    for (const auto& [name, texture] : m_textureMap) {
        auto it = other.m_textureMap.find(name);
        if (it == other.m_textureMap.end() || it->second != texture) {
            return false;
        }
    }

    return true;
}

bool Material::isSSBOShader() const {
    return m_vertexShaderResource.find("ENABLE_SSBO") != std::string::npos;
}

void Material::loadShader() {
    std::string vertexShaderPath = "assets/shaders/" + m_shaderName + ".vert";
    std::string fragmentShaderPath = "assets/shaders/" + m_shaderName + ".frag";

    std::ifstream vertexShaderFile(vertexShaderPath);
    m_vertexShaderResource = std::string((std::istreambuf_iterator<char>(vertexShaderFile)), std::istreambuf_iterator<char>());
    vertexShaderFile.close();

    std::ifstream fragmentShaderFile(fragmentShaderPath);
    m_fragmentShaderResource = std::string((std::istreambuf_iterator<char>(fragmentShaderFile)), std::istreambuf_iterator<char>());
    fragmentShaderFile.close();
}

GPUProgramHandle* Material::buildShader(bool enableSSBO) {
    const std::string vertexSource = MaterialUtil::normalizeShaderVersion(m_vertexShaderResource);
    const std::string fragmentSource = MaterialUtil::normalizeShaderVersion(m_fragmentShaderResource);
    std::string vertexHead;
    std::string fragmentHead;
    MaterialUtil::appendDefaultPrecisionIfNeeded(vertexSource, vertexHead);
    MaterialUtil::appendDefaultPrecisionIfNeeded(fragmentSource, fragmentHead);
    if (enableSSBO) {
        for (const auto& define : m_defines) {
            vertexHead += "#define " + define + "\n";
            fragmentHead += "#define " + define + "\n";
        }
    }
    return dynamic_cast<GPUProgramHandle*>(RENDERINGTHREAD->createGPUProgram(
        m_shaderName,
        MaterialUtil::buildShaderSource(vertexSource, vertexHead),
        MaterialUtil::buildShaderSource(fragmentSource, fragmentHead)));
}
} // namespace morrow