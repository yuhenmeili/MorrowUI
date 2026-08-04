//
// SafeStaticTextLayout.cpp — 安全静态文本排版器实现
//
// 着色器源码硬编码于 .rodata，字符排版数据由 StaticAtlasManager 外部注入。
// 每个字符 = 一个带 UV 的 Quad，所有 Quad 合并为单一 Mesh。
//

#include "SafeStaticTextLayout.h"

#include "base/Mesh.h"
#include "base/MeshFilter.h"
#include "base/Transform.h"
#include "Material.h"
#include "utils/Log.h"

#include <cstring>

namespace morrow {

using namespace Math;

// =========================================================================
// 硬编码文本着色器（引擎内置 ROM）
//   顶点：position + texCoord → MVP 变换
//   片元：texture 采样 × color tint
// =========================================================================
#ifdef OPENGL_EGL
static const char kTextLayoutVert[] = R"GLSL(#version 320 es
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) out vec2 v_texCoord;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kTextLayoutFrag[] = R"GLSL(#version 320 es
layout (location = 0) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec4 u_color;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    vec4 texColor = texture(u_texture, v_texCoord);
    fragColor = texColor * u_color;
    fragColor.a *= u_alpha;
}
)GLSL";
#else
static const char kTextLayoutVert[] = R"GLSL(#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) out vec2 v_texCoord;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kTextLayoutFrag[] = R"GLSL(#version 460 core
layout (location = 0) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec4 u_color;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    vec4 texColor = texture(u_texture, v_texCoord);
    fragColor = texColor * u_color;
    fragColor.a *= u_alpha;
}
)GLSL";
#endif


// =========================================================================
// SafeStaticTextLayout 实现
// =========================================================================

SafeStaticTextLayoutSharedPtr SafeStaticTextLayout::create(const std::string& atlasName) {
    return SafeStaticTextLayoutSharedPtr(new SafeStaticTextLayout(atlasName));
}

SafeStaticTextLayout::SafeStaticTextLayout(const std::string& atlasName)
    : m_atlasName(atlasName), m_color(1.0f, 1.0f, 1.0f, 1.0f) {
    setWidgetType("SafeStaticTextLayout");

    // 将默认 Quad Mesh 改为空 Mesh（由 buildTextMesh 动态重建）
    auto mesh = m_meshFilter->getMesh();
    mesh->setIndices({});
    mesh->setUVs({});

    initShader();
}

// -------------------------------------------------------------------------
// 着色器初始化
// -------------------------------------------------------------------------
void SafeStaticTextLayout::initShader() {
    m_material->setShaderFromMemory(
        "safe_text_layout",
        std::string(kTextLayoutVert),
        std::string(kTextLayoutFrag)
    );
    m_material->setBlendEnabled(true);
    m_material->setFloat("alpha", 1.0f);
    m_material->setVector("color", Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

// -------------------------------------------------------------------------
// 文本内容设置
// -------------------------------------------------------------------------
void SafeStaticTextLayout::setText(const std::vector<std::string>& charSpriteNames) {
    m_charSprites = charSpriteNames;
    buildTextMesh();
}

void SafeStaticTextLayout::setText(const char* const* spriteNames, int count) {
    m_charSprites.clear();
    m_charSprites.reserve(count);
    for (int i = 0; i < count; ++i) {
        m_charSprites.emplace_back(spriteNames[i]);
    }
    buildTextMesh();
}

void SafeStaticTextLayout::clearText() {
    m_charSprites.clear();
    auto mesh = m_meshFilter->getMesh();
    mesh->setVertices({});
    mesh->setUVs({});
    mesh->setIndices({});
}

// -------------------------------------------------------------------------
// 外观设置
// -------------------------------------------------------------------------
void SafeStaticTextLayout::setColor(float r, float g, float b, float a) {
    m_color = Vector4(r, g, b, a);
    m_material->setVector("u_color", m_color);
    m_material->setFloat("u_alpha", a);
}

// -------------------------------------------------------------------------
// 排版网格构建
//   每个字符 = 1 个 Quad（4 顶点 + 6 索引）
//   从左到右排列，Y 轴居中
// -------------------------------------------------------------------------
void SafeStaticTextLayout::buildTextMesh() {
    if (m_charSprites.empty()) {
        clearText();
        return;
    }

    auto& atlasMgr = StaticAtlasManager::getInstance();
    if (!atlasMgr.hasAtlas(m_atlasName)) {
        LOG_E("SafeStaticTextLayout: atlas '{}' not registered", m_atlasName);
        return;
    }

    // 获取共享纹理，设为 NEAREST 滤波（位图字体不需要线性插值）
    auto texture = atlasMgr.getOrCreateTexture(m_atlasName);
    if (texture) {
        texture->setMinFilterType(SamplerMinFilter::NEAREST);
        texture->setMagFilterType(SamplerMagFilter::NEAREST);
        m_material->setTexture("u_texture", texture);
    }

    const int charCount = static_cast<int>(m_charSprites.size());
    const int vertexCount = charCount * 4;
    const int indexCount  = charCount * 6;

    std::vector<Vector3> vertices;
    std::vector<Vector2> uvs;
    std::vector<int16_t> indices;
    vertices.reserve(vertexCount);
    uvs.reserve(vertexCount);
    indices.reserve(indexCount);

    // 收集所有 sprite 定义，计算总宽度用于居中
    std::vector<const SafeSpriteDef*> defs;
    defs.reserve(charCount);
    float totalWidth = 0.0f;
    float maxHeight  = 0.0f;

    for (const auto& name : m_charSprites) {
        const auto* def = atlasMgr.getSpriteDef(m_atlasName, name);
        if (!def) {
            LOG_W("SafeStaticTextLayout: sprite '{}' not found in atlas '{}'", name, m_atlasName);
            defs.push_back(nullptr);
            continue;
        }
        defs.push_back(def);
        totalWidth += def->spriteWidth + m_charSpacing;
        if (def->spriteHeight > maxHeight) maxHeight = def->spriteHeight;
    }
    // 去掉最后一个多余间距
    if (totalWidth > 0.0f) totalWidth -= m_charSpacing;

    // 从左到右排列，Y 轴居中
    float cursorX = -totalWidth * 0.5f;
    const float halfH = maxHeight * 0.5f;

    for (int i = 0; i < charCount; ++i) {
        const auto* def = defs[i];
        if (!def) {
            cursorX += maxHeight * 0.5f + m_charSpacing; // 缺失字符占位
            continue;
        }

        float w = def->spriteWidth;
        float h = def->spriteHeight;
        float x0 = cursorX;
        float y0 = -halfH;
        float x1 = cursorX + w;
        float y1 = halfH;

        // 4 顶点（左上、右上、右下、左下）
        int vi = static_cast<int>(vertices.size());
        vertices.emplace_back(x0, y1, 0.0f);
        vertices.emplace_back(x1, y1, 0.0f);
        vertices.emplace_back(x1, y0, 0.0f);
        vertices.emplace_back(x0, y0, 0.0f);

        uvs.emplace_back(def->u,  def->v);
        uvs.emplace_back(def->u2, def->v);
        uvs.emplace_back(def->u2, def->v2);
        uvs.emplace_back(def->u,  def->v2);

        // 6 索引（两个三角形）
        indices.push_back(static_cast<int16_t>(vi));
        indices.push_back(static_cast<int16_t>(vi + 1));
        indices.push_back(static_cast<int16_t>(vi + 2));
        indices.push_back(static_cast<int16_t>(vi));
        indices.push_back(static_cast<int16_t>(vi + 2));
        indices.push_back(static_cast<int16_t>(vi + 3));

        cursorX += w + m_charSpacing;
    }

    // 更新 Mesh
    auto mesh = m_meshFilter->getMesh();
    mesh->setDrawMode(PrimitiveType::TRIANGLES);
    mesh->setVertices(vertices);
    mesh->setUVs(uvs);
    mesh->setIndices(indices);
    mesh->setColors({});  // 不使用逐顶点颜色

    requestRender("SafeStaticTextLayout::buildTextMesh");
}

} // namespace morrow
