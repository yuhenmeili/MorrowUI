//
// Created by lance on 24-7-1.
//

#include "TextureAtlas.h"
#include "ToolUtils.h"
#include "utils/Log.h"

namespace morrow {
TextureAtlas::TextureAtlas(const std::string& packFileUrl, const std::string& imagesDir, bool flip) {
    m_parser.parse(packFileUrl, flip);
    buildTextures(imagesDir);
    buildRegions();
}

TextureAtlas::~TextureAtlas() {
    m_textures.clear();
    m_regions.clear();
}

void TextureAtlas::buildTextures(const std::string& imagesDir) {
    // imagesDir 以 png/basis 结尾时视为单个纹理文件，所有页共用。
    const bool isFileUrl = ToolUtils::endsWith(imagesDir, "png") || ToolUtils::endsWith(imagesDir, "basis");
    for (auto& page : m_parser.getPages()) {
        auto texture = Texture::create(ImageType::IMAGE);
        texture->setImageUrl(isFileUrl ? imagesDir : imagesDir + page->name);
        texture->setFormat(page->format);
        texture->setMinFilterType(page->minFilter);
        texture->setMagFilterType(page->magFilter);
        texture->setWidth(page->width);
        texture->setHeight(page->height);
        //        texture->setWrap();
        m_textures.emplace(page->name, texture);
    }
}

void TextureAtlas::buildRegions() {
    for (auto& frame : m_parser.getFrames()) {
        auto texture = getTexture(frame->page->name);
        if (!texture) {
            LOG_E("Texture {} of frame {} not found.", frame->page->name, frame->name);
            continue;
        }
        m_regions.emplace_back(std::make_shared<TextureRegion>(frame, texture));
    }
}

void TextureAtlas::updateTexture(const std::string& imageUrlOrName) {
    std::string::size_type pos = imageUrlOrName.rfind('/');
    std::string imageName = pos == std::string::npos ? imageUrlOrName : imageUrlOrName.substr(pos + 1);
    auto iter = m_textures.find(imageName);
    if (iter != m_textures.end()) {
        iter->second->setImageUrl(imageUrlOrName);
    }
}

TextureSharedPtr TextureAtlas::getTexture(const std::string& name) const {
    auto iter = m_textures.find(name);
    return iter != m_textures.end() ? iter->second : nullptr;
}

std::vector<TextureRegionSharedPtr>& TextureAtlas::getRegions() {
    return m_regions;
}

TextureRegionSharedPtr TextureAtlas::findRegion(const std::string& name) const {
    for (auto& region : m_regions) {
        if (region->getName() == name) {
            return region;
        }
    }
    return nullptr;
}

TextureRegionSharedPtr TextureAtlas::findRegion(const std::string& name, int32_t index) const {
    for (auto& region : m_regions) {
        if (region->getName() == name && region->getIndex() == index) {
            return region;
        }
    }
    return nullptr;
}

void TextureAtlas::findRegions(const std::string& name, std::vector<TextureRegionSharedPtr>& result) const {
    for (auto& region : m_regions) {
        if (region->getName() == name) {
            result.emplace_back(region);
        }
    }
}
} // MORROWGUI