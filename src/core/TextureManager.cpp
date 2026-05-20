//
// Created by lance on 24-8-12.
//

#include "TextureManager.h"

namespace morrow
{
void TextureManager::addTexture(const std::string& name, TextureWeakPtr texture)
{
    m_textures[name] = texture;
}

TextureSharedPtr TextureManager::getTexture(const std::string& name)
{
    if (m_textures.find(name) != m_textures.end()) {
        return m_textures[name].lock();
    }
    return nullptr;
}

void TextureManager::removeTexture(const std::string& name)
{
    if (m_textures.find(name) != m_textures.end()) {
        m_textures.erase(name);
    }
}
} // MORROWGUI