//
// Created by lance on 2023/3/23.
//

#ifndef MORROW_MRGearsShine_H
#define MORROW_MRGearsShine_H

#include <memory>
#include "base/Widget.h"

#include "Texture.h"
#include "base/UIWidget.h"

namespace morrow
{
/// 挡位点阵高亮特效配置。
struct MRGearsShineOptions
{
    /// 高亮移动方向；-1、0、1 分别表示上方、中间和下方。
    int32_t direct = 0;//-1,0,1,up,middle,down
    /// 高亮区域半径。
    float radiusHighlight = 50.0f;
    /// 普通点尺寸。
    float pointSize = 6.8f;
    /// 高亮点尺寸。
    float pointSizeHighlight = 6.0f;
    /// 点阵边缘相对中心的透明度偏移。
    float pointColorAlphaOffset = 0.4f;
    /// 高亮区域使用的点颜色。
    Vector4 pointColorHighlight = {0.0f, 0.0f, 0.0f, 0.0f};//高亮改色，充能用
    /// 点阵行间距。
    float pointRowGap = 36.0f;
    /// 点阵列间距。
    float pointColumnGap = 34.0f;
    /// 点阵行数。
    int32_t pointRow = 20;
    /// 点阵列数。
    int32_t pointColumn = 4;
    /// 需要挖空的行索引。
    std::vector<int32_t> hollowRow{1, 18};//start 0
    /// 需要挖空的列索引。
    std::vector<int32_t> hollowColumn{1, 2};

    /// 判断指定行列位置是否位于挖空区域。
    bool isCurrentRowAndColumnHollow(float row, float column)
    {
        return (std::find(hollowRow.begin(), hollowRow.end(), row) != hollowRow.end())
            && (std::find(hollowColumn.begin(), hollowColumn.end(), column) != hollowColumn.end());
    }
};

using MRGearsShineOptionsSharedPtr = std::shared_ptr<MRGearsShineOptions>;

/// 通过点阵移动高亮表现挡位或充能状态的 UI 特效组件。
class MRGearsShine : public UIWidget
{
public:
    /// 创建一个挡位点阵高亮特效组件。
    static std::shared_ptr<MRGearsShine> create();

    /// 初始化点阵网格、材质参数和循环高亮动画。
    void initialize() override;
private:
    MRGearsShine();
    MRGearsShineOptionsSharedPtr m_options;
};
using MRGearsShineSharedPtr = std::shared_ptr<MRGearsShine>;
}

#endif //MORROW_MRGearsShine_H
