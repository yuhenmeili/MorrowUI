//
// StaticAtlasManager.h — 全局静态图集管理器
//
// 设计目标：
//   作为全局单例，存储应用层注入的多组静态图集数据（像素 + 精灵表）。
//   SafeStaticSprite 等功能安全组件通过图集名称从本管理器获取数据，
//   引擎本身不持有任何硬编码的图集像素或精灵表数据。
//
// 命名说明：
//   StaticAtlasManager 遵循项目现有命名惯例（SSBOManager、BatchManager 等）。
//
// 使用流程：
//   1. 应用层调用 registerAtlas() 注入图集数据
//   2. SafeStaticSprite::create("atlasName") 创建组件并绑定图集
//   3. 组件通过本管理器按名称获取像素数据、纹理、精灵定义
//

#ifndef STATICATLASMANAGER_H
#define STATICATLASMANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "Texture.h"
#include "utils/Singleton.h"

namespace morrow {

// ---------------------------------------------------------------------------
// SafeSpriteDef — 单个精灵在 Atlas 中的描述
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
// StaticAtlasManager
// ---------------------------------------------------------------------------
class StaticAtlasManager : public Singleton<StaticAtlasManager> {
    friend class Singleton<StaticAtlasManager>;

public:
    /// 注册一组图集数据（在创建任何使用该图集的组件之前调用）
    /// @param name        图集名称，用作后续查找 key
    /// @param pixelData   图集 RGBA 像素数据首地址（生命周期需覆盖所有使用期）
    /// @param width       图集宽度（像素）
    /// @param height      图集高度（像素）
    /// @param spriteDefs  精灵定义数组
    /// @param count       精灵定义数组元素个数
    void registerAtlas(const std::string& name,
                       const unsigned char* pixelData, int width, int height,
                       const SafeSpriteDef* spriteDefs, int count);

    /// 检查指定名称的图集是否已注册
    bool hasAtlas(const std::string& name) const;

    /// 获取图集像素数据，若不存在则打印错误日志并返回 nullptr
    const unsigned char* getPixelData(const std::string& name) const;

    /// 获取图集宽度，若不存在返回 0
    int getWidth(const std::string& name) const;

    /// 获取图集高度，若不存在返回 0
    int getHeight(const std::string& name) const;

    /// 获取精灵定义表（只读），若不存在返回空 vector
    const std::vector<SafeSpriteDef>& getSpriteTable(const std::string& name) const;

    /// 按精灵名称查找精灵定义，若图集或精灵不存在返回 nullptr 并打印错误日志
    const SafeSpriteDef* getSpriteDef(const std::string& atlasName,
                                      const std::string& spriteName) const;

    /// 获取或惰性创建指定图集的 GPU 纹理（同一图集的所有实例共享）
    TextureSharedPtr getOrCreateTexture(const std::string& name);

private:
    StaticAtlasManager() = default;
    ~StaticAtlasManager() = default;

    struct AtlasEntry {
        const unsigned char* pixelData = nullptr;
        int width = 0;
        int height = 0;
        std::vector<SafeSpriteDef> spriteTable;
        TextureSharedPtr texture; // 惰性创建，同图集实例共享
    };

    /// 查找图集条目，不存在则打印错误并返回 m_atlases.end()
    std::unordered_map<std::string, AtlasEntry>::const_iterator
    findAtlas(const std::string& name) const;

    std::unordered_map<std::string, AtlasEntry> m_atlases;
};

} // namespace morrow

#endif // STATICATLASMANAGER_H
