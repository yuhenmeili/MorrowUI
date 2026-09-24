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

TextureAtlas::TextureAtlas(std::shared_ptr<std::vector<unsigned char>> atlasFileData,
                           std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>> pageImageData, bool flip) {
    parseAtlasBuffer(atlasFileData, flip);
    buildTexturesFromMemory(nullptr, pageImageData);
    buildRegions();
}

TextureAtlas::TextureAtlas(std::shared_ptr<std::vector<unsigned char>> atlasFileData, std::shared_ptr<std::vector<unsigned char>> pageImageData,
                           bool flip) {
    parseAtlasBuffer(atlasFileData, flip);
    buildTexturesFromMemory(std::move(pageImageData), {});
    buildRegions();
}

TextureAtlas::~TextureAtlas() {
    m_textures.clear();
    m_regions.clear();
}

void TextureAtlas::parseAtlasBuffer(const std::shared_ptr<std::vector<unsigned char>>& atlasFileData, bool flip) {
    if (atlasFileData && !atlasFileData->empty()) {
        m_parser.parseBuffer(atlasFileData->data(), atlasFileData->size(), flip);
    } else {
        LOG_E("TextureAtlas buffer ctor: empty atlas data");
    }
}

void TextureAtlas::buildTextures(const std::string& imagesDir) {
    // imagesDir 以 png/basis 结尾时视为单个纹理文件，所有页共用。
    const bool isFileUrl = ToolUtils::endsWith(imagesDir, "png") || ToolUtils::endsWith(imagesDir, "basis");
    for (auto& page : m_parser.getPages()) {
        auto texture = Texture::create(ImageType::IMAGE);
        texture->setImageUrl(isFileUrl ? imagesDir : imagesDir + page->name);
        assemblePageTexture(page, texture);
    }
}

void TextureAtlas::buildTexturesFromMemory(const std::shared_ptr<std::vector<unsigned char>>& sharedPageImage,
                                           const std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>>& pageImageData) {
    for (auto& page : m_parser.getPages()) {
        auto texture = Texture::create(ImageType::IMAGE);
        std::shared_ptr<std::vector<unsigned char>> pageImage = sharedPageImage;
        if (!pageImage) {
            auto iter = pageImageData.find(page->name);
            if (iter != pageImageData.end()) {
                pageImage = iter->second;
            }
        }
        if (pageImage) {
            texture->setImageBuffer(pageImage);
        } else {
            LOG_E("TextureAtlas: page {} has no encoded image data", page->name);
        }
        assemblePageTexture(page, texture);
    }
}

void TextureAtlas::assemblePageTexture(const AtlasPageSharedPtr& page, const TextureSharedPtr& texture) {
    texture->setFormat(page->format);
    texture->setMinFilterType(page->minFilter);
    texture->setMagFilterType(page->magFilter);
    texture->setWidth(page->width);
    texture->setHeight(page->height);
    //        texture->setWrap();
    m_textures.emplace(page->name, texture);
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
    if (auto texture = findPageTexture(imageUrlOrName)) {
        texture->setImageUrl(imageUrlOrName);
    }
}

void TextureAtlas::updateTexture(std::shared_ptr<std::vector<unsigned char>> imageData, const std::string& pageName) {
    if (auto texture = findPageTexture(pageName)) {
        texture->setImageBuffer(std::move(imageData));
    }
}

TextureSharedPtr TextureAtlas::findPageTexture(const std::string& imageUrlOrName) const {
    std::string::size_type pos = imageUrlOrName.rfind('/');
    std::string imageName = pos == std::string::npos ? imageUrlOrName : imageUrlOrName.substr(pos + 1);
    auto iter = m_textures.find(imageName);
    return iter != m_textures.end() ? iter->second : nullptr;
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