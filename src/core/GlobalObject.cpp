//
// Created by lance on 24-8-21.
//

#include "GlobalObject.h"

#include "FontManager.h"
#include "TextureManager.h"
#include "effects/BackdropBlurManager.h"
#include "ssbo/SSBOManager.h"

namespace morrow
{
void GlobalObject::destroy()
{
    // 先释放背景模糊链的 RT/VBO（仍需要存活的渲染设备），再拆除设备本体
    BackdropBlurManager::getInstance().destroy();
    m_fontManager.reset();
    m_textureManager.reset();
    m_renderingThread.reset();
}

std::shared_ptr<FontManager> GlobalObject::getFontManager()
{
    return m_fontManager;
}

std::shared_ptr<TextureManager> GlobalObject::getTextureManager()
{
    return m_textureManager;
}

std::shared_ptr<RenderingThread> GlobalObject::getRenderingThread()
{
    return m_renderingThread;
}

std::shared_ptr<SSBOManager> GlobalObject::getSSBOManager() {
    return m_ssboManager;
}

GlobalObject::GlobalObject() {
    m_fontManager = std::make_shared<FontManager>();
    m_textureManager = std::make_shared<TextureManager>();
    m_renderingThread = std::make_shared<RenderingThread>();
    m_ssboManager = std::make_shared<SSBOManager>();
}
} // MORROWGUI
