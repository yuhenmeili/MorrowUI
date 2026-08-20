//
// Created by lance on 2022/11/2.
//

#ifndef MORROW_ANCHORPOINT_SCALE_H
#define MORROW_ANCHORPOINT_SCALE_H

#include <memory>
#include "ui/helpers/Tween.h"
#include "base/UIWidget.h"

namespace morrow {
class MRAnchorPointScale;

using MRAnchorPointScaleSharedPtr = std::shared_ptr<MRAnchorPointScale>;

/// 使用锚点缩放着色器绘制的 UI 特效组件。
class MRAnchorPointScale : public UIWidget {
public:
    /// 创建一个锚点缩放特效组件。
    static MRAnchorPointScaleSharedPtr create();

    /// 销毁锚点缩放特效组件。
    ~MRAnchorPointScale() override = default;

private:
    MRAnchorPointScale();
};
}

#endif //MORROW_ANCHORPOINT_SCALE_H
