//
// Created by lance on 2022/11/2.
//

#ifndef MORROW_GEARS_SELECT_H
#define MORROW_GEARS_SELECT_H

#include <memory>

#include "base/UIWidget.h"
#include "base/Widget.h"

namespace morrow
{
/// 挡位选中点特效配置。
struct MRGearsSelectOptions
{
    /// 选中点颜色。
    Vector4 color = {125.0f / 255.0f, 51.0f / 255.0f, 1.0f, 1.0f};
    /// 选中点尺寸。
    float pointSize = 6.0f;
};

using MRGearsSelectOptionsSharedPtr = std::shared_ptr<MRGearsSelectOptions>;

/// 使用呼吸亮度表现选中状态的挡位点组件。
class MRGearsSelect : public UIWidget
{
public:
    /// 创建一个挡位选中特效组件。
    static std::shared_ptr<MRGearsSelect> create();

    /// 初始化选中点网格、材质参数和循环动画。
    void initialize() override;

private:
    MRGearsSelect();
    MRGearsSelectOptionsSharedPtr m_options;
};

using MRGearsSelectSharedPtr = std::shared_ptr<MRGearsSelect>;
}

#endif //MORROW_GEARS_SELECT_H
