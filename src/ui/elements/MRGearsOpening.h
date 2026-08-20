//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_GEARS_OPENING_H
#define MORROW_GEARS_OPENING_H


#include <memory>
#include "base/UIWidget.h"

namespace morrow
{
/// 挡位开合特效配置。
struct MRGearsOpeningOptions
{
    /// 动画是否按正向播放。
    bool forward = true;
    /// 开合边缘的模糊半径。
    float blurRadius = 50.0f;
};
using MRGearsOpeningOptionsSharedPtr = std::shared_ptr<MRGearsOpeningOptions>;

/// 循环播放挡位开合过渡的 UI 特效组件。
class MRGearsOpening : public UIWidget
{
public:
    /// 创建一个挡位开合特效组件。
    static std::shared_ptr<MRGearsOpening> create();

    /// 根据组件尺寸和配置初始化材质及循环动画。
    void initialize() override;
private:
    MRGearsOpening();
    MRGearsOpeningOptionsSharedPtr m_options;
};
using MRGearsOpeningSharedPtr = std::shared_ptr<MRGearsOpening>;
}

#endif //MORROW_GEARS_OPENING_H
