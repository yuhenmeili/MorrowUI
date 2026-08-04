//
// SafeStaticSprite.cpp — 安全图片组件实现
//
// 着色器源码以 static const 编译期常量硬编码于只读内存段（.rodata），
// 不依赖文件 IO。图集像素数据与精灵表由外部应用层通过
// StaticAtlasManager::registerAtlas() 注入，引擎本身不持有这些数据。
//

#include "SafeStaticSprite.h"

#include "base/Transform.h"
#include "Texture.h"
#include "StaticAtlasManager.h"
#include "GlobalObject.h"
#include "RenderDeviceProxyBase.h"
#include "utils/Log.h"

#include <cstring>

namespace morrow {

// =========================================================================
// 1. 硬编码着色器源码（引擎内置 ROM .rodata）
// =========================================================================
#ifdef OPENGL_EGL
static const char kSafeStaticSpriteVert[] = R"GLSL(#version 320 es
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

uniform mat4 u_mvp;

out vec2 v_texCoord;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kSafeStaticSpriteFrag[] = R"GLSL(#version 320 es
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= u_alpha;
}
)GLSL";
#else
static const char kSafeStaticSpriteVert[] = R"GLSL(#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

uniform mat4 u_mvp;

out vec2 v_texCoord;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kSafeStaticSpriteFrag[] = R"GLSL(#version 460 core
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= u_alpha;
}
)GLSL";
#endif

// =========================================================================
// SafeStaticSprite 实现
// =========================================================================

SafeStaticSpriteSharedPtr SafeStaticSprite::create(const std::string& atlasName) {
    return SafeStaticSpriteSharedPtr(new SafeStaticSprite(atlasName));
}

SafeStaticSprite::SafeStaticSprite(const std::string& atlasName)
    : m_atlasName(atlasName) {
    setWidgetType("SafeStaticSprite");

    // 1) 注入内置着色器 → Material（绕过文件 IO）
    initShader();

    // 2) 从 StaticAtlasManager 获取图集 GPU 纹理
    initTexture();

    // 3) 构建精灵名 → UV 查找哈希表
    initSpriteTable();

    // 4) 监听尺寸变化，更新 shader uniform
    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        auto t = getComponent<Transform>();
        m_material->setVector("displaySize", t->getSize());
        requestRender("SafeStaticSprite::sizeChanged");
    });

    // 5) 设置默认精灵（第一个）
    const auto& table = StaticAtlasManager::getInstance().getSpriteTable(m_atlasName);
    if (!table.empty()) {
        setSprite(table[0].name);
    }
}

// -------------------------------------------------------------------------
// 着色器初始化（引擎内置着色器源码）
// -------------------------------------------------------------------------
void SafeStaticSprite::initShader() {
    m_material->setShaderFromMemory(
        "safe_static_sprite",
        std::string(kSafeStaticSpriteVert),
        std::string(kSafeStaticSpriteFrag)
    );
    m_material->setBlendEnabled(true);
    m_material->setFloat("alpha", 1.0f);
}

// -------------------------------------------------------------------------
// 纹理初始化（从 StaticAtlasManager 获取共享 GPU 纹理）
// -------------------------------------------------------------------------
void SafeStaticSprite::initTexture() {
    m_romTexture = StaticAtlasManager::getInstance().getOrCreateTexture(m_atlasName);
    if (m_romTexture) {
        m_material->setTexture("texture", m_romTexture);
    }
}

// -------------------------------------------------------------------------
// 精灵查找表初始化（从 StaticAtlasManager 获取精灵表，O(1) 哈希查找）
// -------------------------------------------------------------------------
void SafeStaticSprite::initSpriteTable() {
    m_spriteMap.clear();
    const auto& table = StaticAtlasManager::getInstance().getSpriteTable(m_atlasName);
    for (const auto& def : table) {
        m_spriteMap[def.name] = &def;
    }
}

// -------------------------------------------------------------------------
// 精灵切换（O(1) 哈希查找 + 设置 UV）
// -------------------------------------------------------------------------
bool SafeStaticSprite::setSprite(const std::string& spriteName) {
    // 变更检测：相同精灵不重复设置，避免冗余 UV 更新和 requestRender
    if (spriteName == m_currentSpriteName) {
        return true;
    }

    auto it = m_spriteMap.find(spriteName);
    if (it == m_spriteMap.end()) {
        LOG_W("SafeStaticSprite::setSprite - sprite '{}' not found in ROM table", spriteName);
        return false;
    }

    const SafeSpriteDef* def = it->second;
    m_currentSpriteName = spriteName;
    m_currentSpriteDef  = *def;

    // 设置四边形 UV 到 MeshFilter（裁剪 Atlas 对应区域）
    m_meshFilter->setUVData(def->u, def->v, def->u2, def->v2);

    requestRender("SafeStaticSprite::setSprite:" + spriteName);
    return true;
}

const std::string& SafeStaticSprite::getCurrentSpriteName() const {
    return m_currentSpriteName;
}

std::vector<std::string> SafeStaticSprite::getSpriteNames() const {
    std::vector<std::string> names;
    names.reserve(m_spriteMap.size());
    for (const auto& pair : m_spriteMap) {
        names.push_back(pair.first);
    }
    return names;
}

// -------------------------------------------------------------------------
// 运行时纹理替换（非安全路径，产生堆分配，仅用于调试/动态更新）
// -------------------------------------------------------------------------
void SafeStaticSprite::setTexture(TextureSharedPtr texture) {
    m_romTexture = texture;
    if (m_romTexture) {
        m_material->setTexture("texture", m_romTexture);
    }
    requestRender("SafeStaticSprite::setTexture");
}

void SafeStaticSprite::debugTexture() {
    if (m_romTexture) {
        m_romTexture->debugTexture(getIdentityInfo());
    }
    UIWidget::debugTexture();
}

// -------------------------------------------------------------------------
// 着色器源码访问器（引擎内置，不依赖外部数据）
// -------------------------------------------------------------------------
const char* SafeStaticSprite::getHardcodedVertexShader()   { return kSafeStaticSpriteVert; }
const char* SafeStaticSprite::getHardcodedFragmentShader() { return kSafeStaticSpriteFrag; }

} // namespace morrow
