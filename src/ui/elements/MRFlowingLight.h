//
// Created by lance on 2023/2/22.
//

#ifndef MORROW_FLOWING_LIGHT_H
#define MORROW_FLOWING_LIGHT_H

#include <memory>
#include "base/UIWidget.h"

namespace morrow
{
/// 流光特效配置。
struct MRFlowingLightOptions
{
    /// 流光沿运动方向覆盖的长度。
    float flowingLightLength = 100.0f;
    /// 流光颜色。
    Vector4 flowingLightColor = {0.5f,0.2f,1.0f,1.0f};
    /// 流光带宽度。
    float flowingLightThickness = 4.0f;
};

using MRFlowingLightOptionsSharedPtr = std::shared_ptr<MRFlowingLightOptions>;

/// 沿组件表面循环移动的流光特效组件。
class MRFlowingLight : public UIWidget
{
public:
    /// 创建一个流光特效组件。
    static std::shared_ptr<MRFlowingLight> create();

    /// 根据组件尺寸和配置初始化流光材质及循环动画。
    void initialize() override;
private:
    MRFlowingLight();
    MRFlowingLightOptionsSharedPtr m_options;
};
using MRFlowingLightSharedPtr = std::shared_ptr<MRFlowingLight>;
}

#endif //MORROW_FLOWING_LIGHT_H
