//
// Created by lance on 2023/3/20.
//

#include "Material.h"

#include <cstring>

#include "EmbeddedShaders.h"
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
    const std::string key = m_attributePrefix + name;
    auto current = m_textureMap.find(key);
    if (current != m_textureMap.end() && current->second == texture) {
        return;
    }
    m_textureMap[key] = texture;
    ++m_batchCompatibilityRevision;
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
    ++m_uniformRevision;
}

void Material::setVector(const std::string& name, const Vector3& value) {
    m_vectorMap[m_attributePrefix + name] = value;
    ++m_uniformRevision;
}

void Material::setVector(const std::string& name, const Vector4& value) {
    m_vectorMap[m_attributePrefix + name] = value;
    ++m_uniformRevision;
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
    ++m_uniformRevision;
}

Matrix4 Material::getMatrix4(const std::string& name) const {
    auto it = m_matrix4Map.find(m_attributePrefix + name);
    if (it != m_matrix4Map.end()) {
        return it->second;
    }
    return Matrix4();  // 返回单位矩阵
}

void Material::setFloat(const std::string& name, float value) {
    m_floatMap[m_attributePrefix + name] = value;
    ++m_uniformRevision;
}

float Material::getFloat(const std::string& name) const {
    auto it = m_floatMap.find(m_attributePrefix + name);
    if (it != m_floatMap.end()) {
        return it->second;
    }
    return 0.0f;
}

// ── 类型安全读取（P2：SSBO 字段绑定 / 声明式打包用）──

bool Material::tryGetFloat(std::string_view name, float& value) const {
    auto it = m_floatMap.find(m_attributePrefix + std::string(name));
    if (it == m_floatMap.end()) {
        return false;
    }
    value = it->second;
    return true;
}

float Material::getFloatOr(std::string_view name, float defaultValue) const {
    float value = defaultValue;
    tryGetFloat(name, value);
    return value;
}

bool Material::tryGetVector4(std::string_view name, Vector4& value) const {
    auto it = m_vectorMap.find(m_attributePrefix + std::string(name));
    if (it == m_vectorMap.end()) {
        return false;
    }
    return std::visit(
        [&value](const auto& vec) {
            const size_t count = sizeof(vec.elements) / sizeof(float);
            value = Vector4(
                vec.elements[0],
                count > 1 ? vec.elements[1] : 0.0f,
                count > 2 ? vec.elements[2] : 0.0f,
                count > 3 ? vec.elements[3] : 0.0f);
            return true;
        },
        it->second);
}

Vector4 Material::getVector4Or(std::string_view name, const Vector4& defaultValue) const {
    Vector4 value = defaultValue;
    tryGetVector4(name, value);
    return value;
}

bool Material::tryGetVectorComponent(std::string_view name, int component, float& value) const {
    auto it = m_vectorMap.find(m_attributePrefix + std::string(name));
    if (it == m_vectorMap.end()) {
        return false;
    }
    return std::visit(
        [&value, component](const auto& vec) -> bool {
            const size_t count = sizeof(vec.elements) / sizeof(float);
            if (component < 0 || static_cast<size_t>(component) >= count) {
                return false;
            }
            value = vec.elements[component];
            return true;
        },
        it->second);
}

void Material::setInt(const std::string& name, int32_t value) {
    m_intMap[m_attributePrefix + name] = value;
    ++m_uniformRevision;
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
    ++m_uniformRevision;
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
    ++m_uniformRevision;
}

// void ShaderMaterial::setShader(const ShaderSharedPtr& shader) {
//     m_shader = shader;
// }

void Material::setShader(const std::string& shaderName) {
    if (m_shaderName != shaderName) {
        m_shaderName = shaderName;
        loadShader();
        ++m_batchCompatibilityRevision;
        ++m_uniformRevision;
        LOG_I("set shader {}", shaderName);
    }
}

void Material::setShaderFromMemory(const std::string& shaderName, const std::string& vertexSource, const std::string& fragmentSource) {
    m_shaderName = shaderName;
    m_vertexShaderResource = vertexSource;
    m_fragmentShaderResource = fragmentSource;
    // 清除已缓存的 shader，下次 apply 时重新 buildShader
    m_shader = HwGPUProgram{0};
    m_batchShader = HwGPUProgram{0};
    ++m_batchCompatibilityRevision;
    ++m_uniformRevision;
}

std::string Material::getShaderName() const {
    return m_shaderName;
}

HwGPUProgram Material::getShader() const {
    return m_shader;
}

HwGPUProgram Material::getBatchShader() const {
    return m_batchShader;
}

void Material::setBlendEnabled(bool enabled) {
    if (m_pipelineState.blendEnabled != enabled) {
        m_pipelineState.blendEnabled = enabled;
        ++m_batchCompatibilityRevision;
    }
}

void Material::setBlendFunc(BlendFactor srcRgbFactor, BlendFactor dstRgbFactor, BlendFactor srcAlphaFactor, BlendFactor dstAlphaFactor) {
    if (m_pipelineState.srcRgbBlendFactor != srcRgbFactor || m_pipelineState.dstRgbBlendFactor != dstRgbFactor ||
        m_pipelineState.srcAlphaBlendFactor != srcAlphaFactor || m_pipelineState.dstAlphaBlendFactor != dstAlphaFactor) {
        m_pipelineState.srcRgbBlendFactor = srcRgbFactor;
        m_pipelineState.dstRgbBlendFactor = dstRgbFactor;
        m_pipelineState.srcAlphaBlendFactor = srcAlphaFactor;
        m_pipelineState.dstAlphaBlendFactor = dstAlphaFactor;
        ++m_batchCompatibilityRevision;
    }
}

bool Material::isBlendEnabled() const {
    return m_pipelineState.blendEnabled;
}

void Material::getBlendFunc(BlendFactor& srcRgbFactor, BlendFactor& dstRgbFactor, BlendFactor& srcAlphaFactor, BlendFactor& dstAlphaFactor) const {
    srcRgbFactor = m_pipelineState.srcRgbBlendFactor;
    dstRgbFactor = m_pipelineState.dstRgbBlendFactor;
    srcAlphaFactor = m_pipelineState.srcAlphaBlendFactor;
    dstAlphaFactor = m_pipelineState.dstAlphaBlendFactor;
}

void Material::setDoubleSided(bool doubleSided) {
    const CullFaceMode cullFaceMode = doubleSided ? CullFaceMode::NONE : CullFaceMode::BACK;
    if (m_pipelineState.cullFaceMode != cullFaceMode) {
        m_pipelineState.cullFaceMode = cullFaceMode;
        ++m_batchCompatibilityRevision;
    }
}

bool Material::isDoubleSided() const {
    return m_pipelineState.cullFaceMode == CullFaceMode::NONE;
}

void Material::setDepthTestEnabled(bool enabled) {
    if (m_pipelineState.depthTestEnabled != enabled) {
        m_pipelineState.depthTestEnabled = enabled;
        ++m_batchCompatibilityRevision;
    }
}

void Material::setDepthWriteEnabled(bool enabled) {
    if (m_pipelineState.depthWriteEnabled != enabled) {
        m_pipelineState.depthWriteEnabled = enabled;
        ++m_batchCompatibilityRevision;
    }
}

void Material::setScene3DMaterialUBO(const Scene3DMaterialUBO& materialData) {
    m_scene3DMaterialData = materialData;
    m_scene3DMaterialDirty = true;
    ++m_uniformRevision;
}

const Scene3DMaterialUBO& Material::getScene3DMaterialUBO() const {
    return m_scene3DMaterialData;
}

void Material::bindScene3DMaterialUBO(HwGPUProgram shader) {
    HwGPUProgram targetShader = shader.isValid() ? shader : m_shader;
    if (!targetShader.isValid()) {
        return;
    }

    if (!m_scene3DMaterialUbo.isValid()) {
        m_scene3DMaterialUbo = RENDERINGTHREAD->createUBO();
        m_scene3DMaterialDirty = true;
    }

    if (m_scene3DMaterialDirty) {
        RENDERINGTHREAD->updateUBO(m_scene3DMaterialUbo, MaterialUtil::makeUBOData(m_scene3DMaterialData));
        m_scene3DMaterialDirty = false;
    }

    RENDERINGTHREAD->bindUBO(targetShader, m_scene3DMaterialUbo, "Scene3DMaterial", kScene3DMaterialBindingPoint);
}

void Material::apply(HwGPUProgram shader) {
    if (!shader.isValid() && (!m_shader.isValid())) {
        m_shader = buildShader();
    }
    HwGPUProgram targetShader = shader.isValid() ? shader : m_shader;
    if (!targetShader.isValid()) {
        LOG_E("shader is missing");
        return;
    }

    m_pipelineState.program = targetShader;
    RENDERINGTHREAD->bindPipelineState(m_pipelineState);

    // 应用纹理
    int textureIndex = 0;
    for (const auto& pair : m_textureMap) {
        auto texture = pair.second;
        texture->render(nullptr);
        texture->bindTexture(textureIndex);
        RENDERINGTHREAD->setGPUProgramParamAsInt(targetShader, pair.first, textureIndex);
        textureIndex++;
    }

    // 应用向量
    for (const auto& pair : m_vectorMap) {
        std::visit(
            [targetShader, &pair](const auto& vec) {
                using T = std::decay_t<decltype(vec)>;
                if constexpr (std::is_same_v<T, Vector2>) {
                    RENDERINGTHREAD->setGPUProgramParamAsFloatArray(targetShader, pair.first, vec.elements, 1, 2);
                } else if constexpr (std::is_same_v<T, Vector3>) {
                    RENDERINGTHREAD->setGPUProgramParamAsFloatArray(targetShader, pair.first, vec.elements, 1, 3);
                } else if constexpr (std::is_same_v<T, Vector4>) {
                    RENDERINGTHREAD->setGPUProgramParamAsFloatArray(targetShader, pair.first, vec.elements, 1, 4);
                }
            },
            pair.second);
    }

    // 应用浮点数
    for (const auto& pair : m_floatMap) {
        RENDERINGTHREAD->setGPUProgramParamAsFloat(targetShader, pair.first, pair.second);
    }

    // 应用整数
    for (const auto& pair : m_intMap) {
        RENDERINGTHREAD->setGPUProgramParamAsInt(targetShader, pair.first, pair.second);
    }

    // 应用矩阵
    for (const auto& pair : m_matrix4Map) {
        RENDERINGTHREAD->setGPUProgramParamAsMat4(targetShader, pair.first, pair.second);
    }

    // intArrayData
    for (const auto& pair : m_intArrayMap) {
        RENDERINGTHREAD->setGPUProgramParamAsIntArray(targetShader, pair.first, pair.second.data, pair.second.size, pair.second.step);
    }
}

void Material::applyBatch(HwGPUProgram shader) {
    if (!shader.isValid() && (!m_batchShader.isValid())) {
        m_batchShader = buildShader(true);
    }
    HwGPUProgram targetShader = shader.isValid() ? shader : m_batchShader;
    if (!targetShader.isValid()) {
        LOG_E("shader is missing");
        return;
    }

    m_pipelineState.program = targetShader;
    RENDERINGTHREAD->bindPipelineState(m_pipelineState);

    // 应用纹理
    int textureIndex = 0;
    for (const auto& pair : m_textureMap) {
        auto texture = pair.second;
        texture->render(nullptr);
        texture->bindTexture(textureIndex);
        RENDERINGTHREAD->setGPUProgramParamAsInt(targetShader, pair.first, textureIndex);
        textureIndex++;
    }
}

bool Material::isEqual(std::shared_ptr<Material> other) {
    if (!other) {
        return false;
    }
    if (m_shaderName != other->m_shaderName || !m_pipelineState.isEqual(other->m_pipelineState) || m_textureMap.size() != other->m_textureMap.size()) {
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
    if (m_shaderName != other.m_shaderName || !m_pipelineState.isEqual(other.m_pipelineState) || m_textureMap.size() != other.m_textureMap.size()) {
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

uint64_t Material::getBatchCompatibilityHash() const {
    if (m_cachedBatchCompatibilityRevision == m_batchCompatibilityRevision) {
        return m_cachedBatchCompatibilityHash;
    }

    auto hashCombine = [](uint64_t seed, uint64_t value) { return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2)); };

    uint64_t hash = std::hash<std::string>{}(m_shaderName);
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.blendEnabled));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.srcRgbBlendFactor));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.dstRgbBlendFactor));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.srcAlphaBlendFactor));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.dstAlphaBlendFactor));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.cullFaceMode));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.depthTestEnabled));
    hash = hashCombine(hash, static_cast<uint64_t>(m_pipelineState.depthWriteEnabled));
    hash = hashCombine(hash, static_cast<uint64_t>(m_textureMap.size()));
    hash = hashCombine(hash, static_cast<uint64_t>(isSSBOShader()));

    // unordered_map iteration order is not stable, so fold each entry independently.
    uint64_t textureHash = 0;
    for (const auto& [name, texture] : m_textureMap) {
        uint64_t entryHash = std::hash<std::string>{}(name);
        entryHash = hashCombine(entryHash, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(texture.get())));
        textureHash ^= entryHash;
    }
    m_cachedBatchCompatibilityHash = hashCombine(hash, textureHash);
    m_cachedBatchCompatibilityRevision = m_batchCompatibilityRevision;
    return m_cachedBatchCompatibilityHash;
}

uint64_t Material::getBatchCompatibilityRevision() const {
    return m_batchCompatibilityRevision;
}

uint64_t Material::getUniformRevision() const {
    return m_uniformRevision;
}

uint64_t Material::getRenderRevisionHash() const {
    uint64_t hash = m_batchCompatibilityRevision;
    hash ^= m_uniformRevision + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    for (const auto& [name, texture] : m_textureMap) {
        uint64_t textureHash = static_cast<uint64_t>(std::hash<std::string>{}(name));
        textureHash ^= static_cast<uint64_t>(reinterpret_cast<uintptr_t>(texture.get())) + 0x9e3779b97f4a7c15ULL + (textureHash << 6U) + (textureHash >> 2U);
        if (texture) {
            textureHash ^= texture->getRevision() + 0x9e3779b97f4a7c15ULL + (textureHash << 6U) + (textureHash >> 2U);
        }
        hash ^= textureHash;
    }
    return hash;
}

uint64_t Material::getRevision() const {
    return getBatchCompatibilityRevision();
}

bool Material::isSSBOShader() const {
    return m_vertexShaderResource.find("ENABLE_SSBO") != std::string::npos;
}

void Material::loadShader() {
    const std::string vertexShaderName = m_shaderName + ".vert";
    const std::string fragmentShaderName = m_shaderName + ".frag";

    // Packaged builds load shaders directly from the shared library.
    if (embedded_shaders::get(vertexShaderName, m_vertexShaderResource) && embedded_shaders::get(fragmentShaderName, m_fragmentShaderResource)) {
        return;
    }

    // Keep external files as a fallback for custom shaders and development.
    const std::string vertexShaderPath = "assets/shaders/" + vertexShaderName;
    const std::string fragmentShaderPath = "assets/shaders/" + fragmentShaderName;

    std::ifstream vertexShaderFile(vertexShaderPath);
    std::ifstream fragmentShaderFile(fragmentShaderPath);
    if (!vertexShaderFile || !fragmentShaderFile) {
        LOG_E("Failed to load shader '{}' from embedded resources or assets/shaders", m_shaderName);
        m_vertexShaderResource.clear();
        m_fragmentShaderResource.clear();
        return;
    }

    m_vertexShaderResource.assign(std::istreambuf_iterator<char>(vertexShaderFile), std::istreambuf_iterator<char>());
    m_fragmentShaderResource.assign(std::istreambuf_iterator<char>(fragmentShaderFile), std::istreambuf_iterator<char>());
}

HwGPUProgram Material::buildShader(bool enableSSBO) {
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
    return RENDERINGTHREAD->createGPUProgram(m_shaderName, MaterialUtil::buildShaderSource(vertexSource, vertexHead), MaterialUtil::buildShaderSource(fragmentSource, fragmentHead));
}
}  // namespace morrow
