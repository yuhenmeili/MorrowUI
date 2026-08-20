//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_GEARS_IRIS_H
#define MORROW_GEARS_IRIS_H

#include <memory>

#include "base/UIWidget.h"
#include "base/Widget.h"

namespace morrow
{
/// 挡位光圈特效配置。
struct MRGearsIrisOptions
{
    /// 扩散圆环的半径。
    float circleRadius = 30.0f;
    /// 圆环动画相对网格动画的延迟时间。
    float circleDelay = 0.4f;
    /// 圆环颜色。
    Vector4 circleColor = {0.2f, 0.2f, 0.2f, 0.2f};
    /// 网格单元之间的间距。
    float gridGap = 13.0f;
    /// 网格动画的重置时间。
    float gridResetTime = 0.2f;//ms
    /// 各级网格单元的半宽参数。
    Vector4 gridHalfWidthParameter = {4.0f, 3.0f, 2.0f, 1.0f};//小格子属性
    /// 网格颜色。
    Vector4 gridColor = {0.57f, 0.57f, 0.57f, 0.6f};
};
using MRGearsIrisOptionsSharedPtr = std::shared_ptr<MRGearsIrisOptions>;

/// 循环播放圆环和网格扩散效果的挡位光圈组件。
class MRGearsIris : public UIWidget
{
public:
    /// 创建一个挡位光圈特效组件。
    static std::shared_ptr<MRGearsIris> create();

    /// 根据组件尺寸和配置初始化材质及循环动画。
    void initialize() override;
private:
    MRGearsIris();
    MRGearsIrisOptionsSharedPtr m_options;
};

using MRGearsIrisSharedPtr = std::shared_ptr<MRGearsIris>;
}

#endif //MORROW_GEARS_IRIS_H
