//
// Created by lance on 24-8-21.
//

#ifndef MORROW_CORE_GLOBALOBJECT_H_
#define MORROW_CORE_GLOBALOBJECT_H_

#include "RenderingThread.h"

#define RENDERINGTHREAD GlobalObject::getInstance().getRenderingThread()->getDevice()
#define REQUESTRENDER GlobalObject::getInstance().getRenderingThread()->requestRender()

namespace morrow
{
class SSBOManager;
class FontManager;
class TextureManager;
class GlobalObject : public Singleton<GlobalObject>
{
    friend class Singleton<GlobalObject>;
public:
    void destroy();

    std::shared_ptr<FontManager> getFontManager();

    std::shared_ptr<TextureManager> getTextureManager();

    std::shared_ptr<RenderingThread> getRenderingThread();

    std::shared_ptr<SSBOManager> getSSBOManager();

private:
    GlobalObject();

    ~GlobalObject() = default;

    std::shared_ptr<FontManager> m_fontManager;
    std::shared_ptr<TextureManager> m_textureManager;
    std::shared_ptr<RenderingThread> m_renderingThread;
    std::shared_ptr<SSBOManager> m_ssboManager;
};

} // MORROWGUI

#endif //MORROW_CORE_GLOBALOBJECT_H_
