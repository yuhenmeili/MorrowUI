//
// Created by lance on 24-7-1.
//

#ifndef MORROW_MRFrameAnimation_H_
#define MORROW_MRFrameAnimation_H_

#include <memory>
#include "atlas/AtlasRegion.h"
#include "atlas/TextureAtlas.h"
#include "atlas/Animation.h"
#include "base/UIWidget.h"

namespace morrow
{
class MRFrameAnimation : public UIWidget
{
public:
    static std::shared_ptr<MRFrameAnimation> create();

    ~MRFrameAnimation() override = default;

    void update(FrameStateSharedPtr frameState) override;

    void setTextureAtlas(TextureAtlasSharedPtr textureAtlas);

    void resetFrame();

private:
    MRFrameAnimation();

    TextureAtlasSharedPtr m_textureAtlas;
    AnimationSharedPtr<AtlasRegionSharedPtr> m_animation;
    AtlasRegionSharedPtr m_currentFrame;
    float m_animationTime = 0.0f;
};

using MRFrameAnimationSharedPtr = std::shared_ptr<MRFrameAnimation>;
}

#endif //MORROW_MRFrameAnimation_H_
