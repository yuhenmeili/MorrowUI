//
// Created by lance on 2023/5/11.
//

#include "MRImage.h"
#include "base/Transform.h"

namespace morrow {
MRImageSharedPtr MRImage::create() {
    return std::shared_ptr<MRImage>(new MRImage());
}

MRImage::MRImage() {
    setWidgetType("MRImage");
    m_material->setShader("image_normal");
    m_material->setFloat("rounding", 0.0f);

    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        auto transform = getComponent<Transform>();
        m_material->setVector("displaySize", transform->getSize());
        requestRender("transformSizeChanged");
    });
}

void MRImage::setRounding(float rounding) {
    if (m_rounding != rounding) {
        m_rounding = rounding;
        m_material->setFloat("rounding", rounding);
        requestRender("setRounding");
    }
}

void MRImage::setScissor(float x, float y, float width, float height) {
    if (m_texture) {
        float imageWidth = m_texture->getWidth();
        float imageHeight = m_texture->getHeight();
        m_meshFilter->setUVData(x / imageWidth, y / imageHeight, (x + width) / imageWidth, (y + height) / imageHeight);
    } else {
        LOG_I("Can't set scissor, atlas texture is not supported");
    }
}

void MRImage::setTexture(TextureSharedPtr texture) {
    m_texture = texture;
    if (!m_texture) {
        LOG_W("MRImage::setTexture received null texture");
        return;
    }

    if (m_texture->getImageType() == ImageType::OES) {
        m_material->setShader("image_oes");
    } else if (m_texture->getImageType() == ImageType::TEXT) {
        m_material->setShader("image_text_debug");
    } else if (m_texture->getImageType() == ImageType::IMAGE) {
        m_material->setShader("image_normal");
    } else {
        m_material->setShader("image_normal");
    }
    m_material->setTexture("texture", texture);
    requestRender("setTexture");
}

void MRImage::setTexture(TextureAtlasSharedPtr texture, const std::string& textureName) {
    if (m_textureAtlas != texture || m_atlasRegionName != textureName) {
        m_textureAtlas = texture;
        m_atlasRegionName = textureName;
        if (m_textureAtlas) {
            m_atlasRegion = m_textureAtlas->findRegion(textureName);
            if (m_atlasRegion) {
                m_meshFilter->setUVData(m_atlasRegion->getU(), m_atlasRegion->getV(), m_atlasRegion->getU2(), m_atlasRegion->getV2());
                m_material->setTexture("texture", m_atlasRegion->getTexture());
            } else {
                LOG_E("Can't find texture {} in atlas", textureName);
            }
        }
    }
}

void MRImage::debugTexture() {
    auto texture = m_texture;
    if (!texture && m_atlasRegion) {
        texture = m_atlasRegion->getTexture();
    }
    if (texture) {
        texture->debugTexture(getIdentityInfo());
    }
    Widget::debugTexture();
}
}
