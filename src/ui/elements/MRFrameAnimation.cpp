//
// Created by lance on 24-7-1.
//

#include "MRFrameAnimation.h"

namespace morrow
{
MRFrameAnimationSharedPtr MRFrameAnimation::create()
{
    return std::shared_ptr<MRFrameAnimation>(new MRFrameAnimation());
}

MRFrameAnimation::MRFrameAnimation()
{
    m_widgetType = "MRFrameAnimation";
    m_material->setShader("frame_animation");
}

void MRFrameAnimation::setTextureAtlas(TextureAtlasSharedPtr textureAtlas)
{
    m_textureAtlas = textureAtlas;
    m_animation = std::make_shared<Animation<AtlasRegionSharedPtr>>(1.0f/ 30.0f, m_textureAtlas->getRegions(), PlayMode::LOOP);
    m_animationTime = 0.0f;
    requestRender("setTextureAtlas");
}

void MRFrameAnimation::resetFrame()
{
    m_animationTime = 0.0f;
    requestRender("resetFrame");
}

void MRFrameAnimation::update(FrameStateSharedPtr frameState)
{
    m_animationTime += float(frameState->deltaTime);
    m_currentFrame = m_animation->getKeyFrame(m_animationTime);
    m_material->setTexture("texture", m_currentFrame->getTexture());
    m_meshFilter->setUVData(m_currentFrame->getU(), m_currentFrame->getV(), m_currentFrame->getU2(), m_currentFrame->getV2());
    //TODO 限制条件暂未实现
    requestRender("update");
    UIWidget::update(frameState);
}
}
