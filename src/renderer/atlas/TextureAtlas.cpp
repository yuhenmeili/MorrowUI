//
// Created by lance on 24-7-1.
//

#include "TextureAtlas.h"
#include "ToolUtils.h"

namespace morrow
{
TextureAtlas::TextureAtlas(const std::string& packFileUrl, const std::string& imagesDir, bool flip)
{
    m_atlasLoader.load(packFileUrl, flip);
    preproccess(imagesDir);
}

TextureAtlas::~TextureAtlas()
{
    this->m_textures.clear();
    this->m_regions.clear();
}

void TextureAtlas::preproccess(const std::string& imagesDir)
{
    bool isFileUrl = ToolUtils::endsWith(imagesDir, "png") || ToolUtils::endsWith(imagesDir, "basis");
    for (auto& page : m_atlasLoader.getPages()) {
        auto texture = Texture::create(ImageType::IMAGE);
        if (isFileUrl) {
            texture->setImageUrl(imagesDir);
        } else {
            texture->setImageUrl(imagesDir + page->name);
        }
        texture->setFormat(page->format);
        texture->setMinFilterType(page->minFilter);
        texture->setMagFilterType(page->magFilter);
        texture->setWidth(page->width);
        texture->setHeight(page->height);
//        texture->setWrap();
        this->m_textures.insert(std::make_pair(page->name, texture));
    }

    for (auto& region : m_atlasLoader.getRegions()) {
        auto atlasRegion = std::make_shared<AtlasRegion>(getTexture(region->page->name),
                                                         region->left, region->top,
                                                         region->rotate ? region->height : region->width,
                                                         region->rotate ? region->width : region->height);
        atlasRegion->m_index = region->index;
        atlasRegion->m_name = region->name;
        atlasRegion->m_offsetX = region->offsetX;
        atlasRegion->m_offsetY = region->offsetY;
        atlasRegion->m_originalHeight = region->originalHeight;
        atlasRegion->m_originalWidth = region->originalWidth;
        atlasRegion->m_rotate = region->rotate;
        atlasRegion->m_degrees = region->degrees;
        atlasRegion->m_names = region->names;
        atlasRegion->m_values = region->values;
        if (region->flip) {
            atlasRegion->flip(false, true);
        }
        this->m_regions.emplace_back(atlasRegion);
    }
}

void TextureAtlas::updateTexture(const std::string& imageUrlOrName)
{
    std::string::size_type pos = imageUrlOrName.rfind('/');
    std::string imageName = pos == std::string::npos ? imageUrlOrName : imageUrlOrName.substr(pos + 1);
    if (this->m_textures.find(imageName) != this->m_textures.end()) {
        this->m_textures[imageName]->setImageUrl(imageUrlOrName);
    }
}

TextureSharedPtr TextureAtlas::getTexture(std::string& name)
{
    if (this->m_textures.find(name) != this->m_textures.end()) {
        return this->m_textures[name];
    }
    return nullptr;
}

std::vector<AtlasRegionSharedPtr>& TextureAtlas::getRegions()
{
    return this->m_regions;
}

AtlasRegionSharedPtr TextureAtlas::findRegion(const std::string& name)
{
    for (auto& region : m_regions) {
        if (region->m_name == name) {
            return region;
        }
    }
    return nullptr;
}

AtlasRegionSharedPtr TextureAtlas::findRegion(const std::string& name, int32_t index)
{
    for (auto& region : m_regions) {
        if (region->m_name == name && region->m_index == index) {
            return region;
        }
    }
    return nullptr;
}

void TextureAtlas::findRegions(const std::string& name, std::vector<AtlasRegionSharedPtr>& result)
{
    for (auto& region : m_regions) {
        if (region->m_name == name) {
            result.emplace_back(region);
        }
    }
}
} // MORROWGUI