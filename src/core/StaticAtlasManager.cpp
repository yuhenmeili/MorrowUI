//
// StaticAtlasManager.cpp — 全局静态图集管理器实现
//

#include "StaticAtlasManager.h"

#include "GlobalObject.h"
#include "RenderDeviceProxyBase.h"
#include "utils/Log.h"

namespace morrow {

// =========================================================================
// 公共接口
// =========================================================================

void StaticAtlasManager::registerAtlas(const std::string& name,
                                        const unsigned char* pixelData,
                                        int width, int height,
                                        const SafeSpriteDef* spriteDefs,
                                        int count) {
    AtlasEntry entry;
    entry.pixelData = pixelData;
    entry.width     = width;
    entry.height    = height;
    if (spriteDefs && count > 0) {
        entry.spriteTable.assign(spriteDefs, spriteDefs + count);
    }
    m_atlases[name] = std::move(entry);
    LOG_I("StaticAtlasManager: registered atlas '{}' ({}x{}, {} sprites)",
          name, width, height, count);
}

bool StaticAtlasManager::hasAtlas(const std::string& name) const {
    return m_atlases.find(name) != m_atlases.end();
}

const unsigned char* StaticAtlasManager::getPixelData(const std::string& name) const {
    auto it = findAtlas(name);
    return (it != m_atlases.end()) ? it->second.pixelData : nullptr;
}

int StaticAtlasManager::getWidth(const std::string& name) const {
    auto it = findAtlas(name);
    return (it != m_atlases.end()) ? it->second.width : 0;
}

int StaticAtlasManager::getHeight(const std::string& name) const {
    auto it = findAtlas(name);
    return (it != m_atlases.end()) ? it->second.height : 0;
}

const std::vector<SafeSpriteDef>& StaticAtlasManager::getSpriteTable(const std::string& name) const {
    static const std::vector<SafeSpriteDef> kEmptyTable;
    auto it = findAtlas(name);
    return (it != m_atlases.end()) ? it->second.spriteTable : kEmptyTable;
}

const SafeSpriteDef* StaticAtlasManager::getSpriteDef(const std::string& atlasName,
                                                       const std::string& spriteName) const {
    auto it = findAtlas(atlasName);
    if (it == m_atlases.end()) {
        return nullptr;
    }
    for (const auto& def : it->second.spriteTable) {
        if (def.name == spriteName) {
            return &def;
        }
    }
    LOG_E("StaticAtlasManager: sprite '{}' not found in atlas '{}'", spriteName, atlasName);
    return nullptr;
}

TextureSharedPtr StaticAtlasManager::getOrCreateTexture(const std::string& name) {
    auto it = m_atlases.find(name);
    if (it == m_atlases.end()) {
        LOG_E("StaticAtlasManager: atlas '{}' not found, cannot create texture", name);
        return nullptr;
    }

    if (!it->second.texture) {
        auto tex = Texture::create(ImageType::IMAGE);
        tex->setTextureData(
            const_cast<unsigned char*>(it->second.pixelData),
            it->second.width,
            it->second.height,
            PixelDataFormat::RGBA,
            0,
            false
        );
        tex->setMinFilterType(SamplerMinFilter::LINEAR);
        tex->setMagFilterType(SamplerMagFilter::LINEAR);
        tex->setTextureName("StaticAtlas_" + name);
        it->second.texture = tex;
        LOG_I("StaticAtlasManager: created GPU texture for atlas '{}' ({}x{})",
              name, it->second.width, it->second.height);
    }
    return it->second.texture;
}

// =========================================================================
// 私有辅助
// =========================================================================

std::unordered_map<std::string, StaticAtlasManager::AtlasEntry>::const_iterator
StaticAtlasManager::findAtlas(const std::string& name) const {
    auto it = m_atlases.find(name);
    if (it == m_atlases.end()) {
        LOG_E("StaticAtlasManager: atlas '{}' not registered", name);
    }
    return it;
}

} // namespace morrow
