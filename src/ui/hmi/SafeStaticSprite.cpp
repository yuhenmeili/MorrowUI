//
// SafeStaticSprite.cpp — 安全图片组件实现
//
// 所有渲染关键数据（着色器源码、Atlas 像素、精灵 UV 表）
// 均以 static const 编译期常量形式硬编码于只读内存段（.rodata）。
// 不需文件 IO，不需运行时堆分配（纹理/着色器数据部分），
// 确保只要 CPU 能取指、GPU 能执行，组件即 100% 可点亮。
//

#include "SafeStaticSprite.h"

#include "base/Transform.h"
#include "Texture.h"
#include "GlobalObject.h"
#include "RenderDeviceProxyBase.h"
#include "utils/Log.h"

#include <cstring>

namespace morrow {

// =========================================================================
// 1. 硬编码着色器源码（ROM .rodata）
//    GLSL 源码以 static const char[] 存在于只读段，无需文件读取
// =========================================================================

static const char kSafeStaticSpriteVert[] = R"GLSL(#version 330 core
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

uniform mat4 u_mvp;

out vec2 v_texCoord;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kSafeStaticSpriteFrag[] = R"GLSL(#version 330 core
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= u_alpha;
}
)GLSL";

// =========================================================================
// 2. 硬编码 Atlas 纹理像素数据（ROM .rodata）
//    图集规格：64×32 像素，RGBA8，共 8 KB
//    由外部工具链从 PNG 图集自动生成为 C 数组头文件
//    （参见 tools/generate_atlas_c_array.py）
// =========================================================================

static constexpr int kAtlasWidth  = 64;
static constexpr int kAtlasHeight = 32;

// 图集像素数据 — 由外部工具生成后替换此 #include
// 当前为内联占位图集（含数字/档位/报警灯色块）
#include "SafeStaticSpriteAtlas.h"

// 编译期静态断言：确保图集数据大小正确
static_assert(sizeof(kAtlasPixelData) == kAtlasWidth * kAtlasHeight * 4,
              "Atlas pixel data size mismatch!");

// =========================================================================
// 3. 硬编码精灵 UV 定义表（ROM .rodata）
//    精灵名称、像素偏移、归一化 UV 全部编译期确定
//    UV 计算公式：u = offsetX / atlasWidth, v = offsetY / atlasHeight
// =========================================================================

static const SafeSpriteDef kSpriteTable[] = {
    // ---- 数字时速表（0-9），每数字 6×8 像素 ----
    { "speed_0",   0.00000f, 0.00000f, 0.09375f, 0.25000f,   0,  0,  6,  8 },
    { "speed_1",   0.09375f, 0.00000f, 0.18750f, 0.25000f,   6,  0,  6,  8 },
    { "speed_2",   0.18750f, 0.00000f, 0.28125f, 0.25000f,  12,  0,  6,  8 },
    { "speed_3",   0.28125f, 0.00000f, 0.37500f, 0.25000f,  18,  0,  6,  8 },
    { "speed_4",   0.37500f, 0.00000f, 0.46875f, 0.25000f,  24,  0,  6,  8 },
    { "speed_5",   0.46875f, 0.00000f, 0.56250f, 0.25000f,  30,  0,  6,  8 },
    { "speed_6",   0.56250f, 0.00000f, 0.65625f, 0.25000f,  36,  0,  6,  8 },
    { "speed_7",   0.65625f, 0.00000f, 0.75000f, 0.25000f,  42,  0,  6,  8 },
    { "speed_8",   0.75000f, 0.00000f, 0.84375f, 0.25000f,  48,  0,  6,  8 },
    { "speed_9",   0.84375f, 0.00000f, 0.93750f, 0.25000f,  54,  0,  6,  8 },

    // ---- 档位显示（P/R/N/D），每档位 16×8 像素 ----
    { "gear_P",    0.00000f, 0.25000f, 0.25000f, 0.50000f,   0,  8, 16,  8 },
    { "gear_R",    0.25000f, 0.25000f, 0.50000f, 0.50000f,  16,  8, 16,  8 },
    { "gear_N",    0.50000f, 0.25000f, 0.75000f, 0.50000f,  32,  8, 16,  8 },
    { "gear_D",    0.75000f, 0.25000f, 1.00000f, 0.50000f,  48,  8, 16,  8 },

    // ---- 报警灯图标，每灯 8×8 像素 ----
    { "warning_engine",      0.00000f, 0.50000f, 0.12500f, 0.75000f,   0, 16,  8,  8 },
    { "warning_oil",         0.12500f, 0.50000f, 0.25000f, 0.75000f,   8, 16,  8,  8 },
    { "warning_battery",     0.25000f, 0.50000f, 0.37500f, 0.75000f,  16, 16,  8,  8 },
    { "warning_brake",       0.37500f, 0.50000f, 0.50000f, 0.75000f,  24, 16,  8,  8 },
    { "warning_seatbelt",    0.50000f, 0.50000f, 0.62500f, 0.75000f,  32, 16,  8,  8 },
    { "warning_abs",         0.62500f, 0.50000f, 0.75000f, 0.75000f,  40, 16,  8,  8 },
    { "warning_airbag",      0.75000f, 0.50000f, 0.87500f, 0.75000f,  48, 16,  8,  8 },
    { "warning_tire",        0.87500f, 0.50000f, 1.00000f, 0.75000f,  56, 16,  8,  8 },
};

static constexpr int kSpriteTableSize = sizeof(kSpriteTable) / sizeof(kSpriteTable[0]);

// =========================================================================
// SafeStaticSprite 实现
// =========================================================================

SafeStaticSpriteSharedPtr SafeStaticSprite::create() {
    return SafeStaticSpriteSharedPtr(new SafeStaticSprite());
}

SafeStaticSprite::SafeStaticSprite() {
    m_widgetType = "SafeStaticSprite";

    // 1) 注入 ROM 着色器 → Material（绕过文件 IO）
    initHardcodedShader();

    // 2) 创建 ROM 纹理（从硬编码像素数据）
    initHardcodedTexture();

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
    if (kSpriteTableSize > 0) {
        setSprite(kSpriteTable[0].name);
    }
}

// -------------------------------------------------------------------------
// 着色器初始化
// -------------------------------------------------------------------------
void SafeStaticSprite::initHardcodedShader() {
    m_material->setShaderFromMemory(
        "safe_static_sprite",
        std::string(kSafeStaticSpriteVert),
        std::string(kSafeStaticSpriteFrag)
    );
    m_material->setBlendEnabled(true);
    m_material->setFloat("alpha", 1.0f);
}

// -------------------------------------------------------------------------
// 纹理初始化（从硬编码像素创建 GPU 纹理）— 全局共享单例
// -------------------------------------------------------------------------

TextureSharedPtr SafeStaticSprite::getSharedROMTexture() {
    // 线程安全的懒加载单例（C++11 static 局部变量保证）
    static TextureSharedPtr s_sharedTexture = []() {
        auto tex = Texture::create(ImageType::IMAGE);
        tex->setTextureData(
            const_cast<unsigned char*>(kAtlasPixelData),
            kAtlasWidth,
            kAtlasHeight,
            PixelDataFormat::RGBA,
            0,
            false
        );
        tex->setMinFilterType(SamplerMinFilter::LINEAR);
        tex->setMagFilterType(SamplerMagFilter::LINEAR);
        tex->setTextureName("SafeStaticSprite_Shared_ROM_Atlas");
        return tex;
    }();
    return s_sharedTexture;
}

void SafeStaticSprite::initHardcodedTexture() {
    // 所有 SafeStaticSprite 实例共享同一张 ROM 纹理，避免重复创建 GPU 纹理
    m_romTexture = getSharedROMTexture();

    // 绑定到 Material
    m_material->setTexture("texture", m_romTexture);
}

// -------------------------------------------------------------------------
// 精灵查找表初始化（O(1) 哈希查找）
// -------------------------------------------------------------------------
void SafeStaticSprite::initSpriteTable() {
    m_spriteMap.clear();
    for (int i = 0; i < kSpriteTableSize; ++i) {
        m_spriteMap[std::string(kSpriteTable[i].name)] = &kSpriteTable[i];
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
// ROM 数据访问器（供外部工具/测试读取硬编码数据）
// -------------------------------------------------------------------------
const unsigned char* SafeStaticSprite::getHardcodedAtlasData() {
    return kAtlasPixelData;
}

int SafeStaticSprite::getHardcodedAtlasWidth()  { return kAtlasWidth; }
int SafeStaticSprite::getHardcodedAtlasHeight() { return kAtlasHeight; }

const char* SafeStaticSprite::getHardcodedVertexShader()   { return kSafeStaticSpriteVert; }
const char* SafeStaticSprite::getHardcodedFragmentShader() { return kSafeStaticSpriteFrag; }

const std::vector<SafeSpriteDef>& SafeStaticSprite::getHardcodedSpriteTable() {
    static std::vector<SafeSpriteDef> table(kSpriteTable, kSpriteTable + kSpriteTableSize);
    return table;
}

} // namespace morrow
