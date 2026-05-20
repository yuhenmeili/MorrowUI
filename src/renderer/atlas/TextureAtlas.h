//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_TEXTUREATLAS_H_
#define MORROW_RENDERER_TEXTUREATLAS_H_

#include <set>
#include <unordered_map>
#include "TextureAtlasData.h"
#include "AtlasRegion.h"

namespace morrow
{
class TextureAtlas
{
public:
    TextureAtlas(const std::string& packFileUrl, const std::string& imagesDir, bool flip = false);

    ~TextureAtlas();

    void updateTexture(const std::string& imageUrlOrName);

    TextureSharedPtr getTexture(std::string& name);

    std::vector<AtlasRegionSharedPtr>& getRegions();

    AtlasRegionSharedPtr findRegion(const std::string& name);

    AtlasRegionSharedPtr findRegion(const std::string& name, int32_t index);

    void findRegions(const std::string& name, std::vector<AtlasRegionSharedPtr>& result);

private:
    void preproccess(const std::string& imagesDir);

private:
    std::unordered_map<std::string, TextureSharedPtr> m_textures;
    std::vector<AtlasRegionSharedPtr> m_regions;
    TextureAtlasData m_atlasLoader;
};

using TextureAtlasSharedPtr = std::shared_ptr<TextureAtlas>;

} // MORROWGUI

#endif //MORROW_RENDERER_TEXTUREATLAS_H_
