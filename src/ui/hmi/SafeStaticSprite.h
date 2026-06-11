//
// SafeStaticSprite.h — 安全图片组件（HMI 仪表盘专用）
//
// 设计目标：
//   编译期将 Atlas 纹理二进制、着色器源码、UV 偏移查找表全部硬编码进
//   只读内存段（.rodata / ROM），确保系统通电即 100% 可靠点亮，
//   不依赖文件系统、不依赖动态内存分配（纹理与着色器数据部分）。
//
// 典型用途：
//   - 数字时速表（0-9 数字纹理切换）
//   - 档位显示（P/R/N/D 纹理切换）
//   - 报警灯（发动机故障、安全带、制动等图标）
//
// 与 MRImage 的关键区别：
//   MRImage: 运行时从文件系统加载纹理/着色器 → 可能因文件损坏无法显示
//   SafeStaticSprite: 编译时硬编码全部数据 → 只要 CPU 取指正常即可渲染
//

#ifndef MORROW_SAFE_STATIC_SPRITE_H
#define MORROW_SAFE_STATIC_SPRITE_H

#include "base/UIWidget.h"
#include "Texture.h"
#include "Vector2.h"

#include <string>
#include <unordered_map>

namespace morrow {

// ---------------------------------------------------------------------------
// SafeSpriteDef — 单个精灵在 Atlas 中的硬编码描述
// ---------------------------------------------------------------------------
struct SafeSpriteDef {
    const char* name;       // 精灵名称（如 "speed_0", "gear_P", "warning_abs"）
    float u;                // 左 UV（归一化 0..1）
    float v;                // 上 UV（归一化 0..1）
    float u2;               // 右 UV（归一化 0..1）
    float v2;               // 下 UV（归一化 0..1）
    float offsetX;          // 在 Atlas 中的像素偏移 X
    float offsetY;          // 在 Atlas 中的像素偏移 Y
    float spriteWidth;      // 精灵原始宽度（像素）
    float spriteHeight;     // 精灵原始高度（像素）
};

// ---------------------------------------------------------------------------
// SafeStaticSprite
// ---------------------------------------------------------------------------
class SafeStaticSprite;
using SafeStaticSpriteSharedPtr = std::shared_ptr<SafeStaticSprite>;

class SafeStaticSprite : public UIWidget {
public:
    /// 工厂方法
    static SafeStaticSpriteSharedPtr create();

    virtual ~SafeStaticSprite() = default;

    // -----------------------------------------------------------------------
    // 运行时接口（轻量、仅做索引查找，不做 IO）
    // -----------------------------------------------------------------------

    /// 通过精灵名称切换到指定 UV 区域（O(1) 哈希查找）
    /// @param spriteName  精灵名称，如 "speed_5", "gear_D", "warning_engine"
    /// @return 是否找到并成功切换
    bool setSprite(const std::string& spriteName);

    /// 获取当前显示的精灵名称
    const std::string& getCurrentSpriteName() const;

    /// 获取所有已注册的精灵名称列表
    std::vector<std::string> getSpriteNames() const;

    /// 运行时替换纹理数据（用于动态更新的场景，如动画数字）
    /// 注意：此方法会产生动态内存分配，仅用于非安全关键路径
    void setTexture(TextureSharedPtr texture);

    void debugTexture() override;

protected:
    SafeStaticSprite();

    // -----------------------------------------------------------------------
    // 初始化：创建 ROM 纹理 + ROM 着色器 + 查找表
    // -----------------------------------------------------------------------
    void initHardcodedTexture();
    void initHardcodedShader();
    void initSpriteTable();

    /// 获取全局共享的 ROM 纹理（所有 SafeStaticSprite 实例共用同一张 GPU 纹理）
    static TextureSharedPtr getSharedROMTexture();

    // -----------------------------------------------------------------------
    // 硬编码数据访问（编译期常量，位于 .rodata）
    // -----------------------------------------------------------------------

    /// @return 硬编码的 Atlas RGBA 像素数据首地址
    static const unsigned char* getHardcodedAtlasData();

    /// @return Atlas 宽度（像素）
    static int getHardcodedAtlasWidth();

    /// @return Atlas 高度（像素）
    static int getHardcodedAtlasHeight();

    /// @return 硬编码的顶点着色器源码
    static const char* getHardcodedVertexShader();

    /// @return 硬编码的片元着色器源码
    static const char* getHardcodedFragmentShader();

    /// @return 硬编码精灵定义表
    static const std::vector<SafeSpriteDef>& getHardcodedSpriteTable();

private:
    TextureSharedPtr    m_romTexture;           // 由硬编码像素数据创建的纹理
    std::string         m_currentSpriteName;    // 当前精灵名
    SafeSpriteDef       m_currentSpriteDef;     // 当前精灵定义（缓存）

    // 精灵名 → 精灵定义 哈希表（编译期数据拷入，查找 O(1)）
    std::unordered_map<std::string, const SafeSpriteDef*> m_spriteMap;
};

} // namespace morrow

#endif // MORROW_SAFE_STATIC_SPRITE_H
