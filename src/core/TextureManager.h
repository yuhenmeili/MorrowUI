//
// Created by lance on 24-8-12.
//

#ifndef MORROW_CORE_TEXTUREMANAGER_H_
#define MORROW_CORE_TEXTUREMANAGER_H_

#include <unordered_map>
#include <vector>
#include "Texture.h"

namespace morrow
{

class TextureManager
{
public:
    void addTexture(const std::string& name, TextureWeakPtr texture);

    TextureSharedPtr getTexture(const std::string& name);

    void removeTexture(const std::string& name);

private:
    std::unordered_map<std::string, TextureWeakPtr> m_textures;

};

using TextureManagerSharedPtr = std::shared_ptr<TextureManager>;

} // MORROWGUI

#endif //MORROW_CORE_TEXTUREMANAGER_H_
