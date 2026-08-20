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
/// 按纹理图集帧序列播放动画的 UI 组件。
class MRFrameAnimation : public UIWidget
{
public:
    /// 创建一个帧动画组件。
    static std::shared_ptr<MRFrameAnimation> create();

    /// 销毁帧动画组件。
    ~MRFrameAnimation() override = default;

    /// 每帧推进动画时间并更新当前图集区域。
    void update(FrameStateSharedPtr frameState) override;

    /// 设置包含动画帧数据的纹理图集。
    void setTextureAtlas(TextureAtlasSharedPtr textureAtlas);

    /// 将动画重置到起始帧。
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
