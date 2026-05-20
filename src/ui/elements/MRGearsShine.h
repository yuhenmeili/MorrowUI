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
struct MRGearsShineOptions
{
    int32_t direct = 0;//-1,0,1,up,middle,down
    float radiusHighlight = 50.0f;
    float pointSize = 6.8f;
    float pointSizeHighlight = 6.0f;
    float pointColorAlphaOffset = 0.4f;
    Vector4 pointColorHighlight = {0.0f, 0.0f, 0.0f, 0.0f};//高亮改色，充能用
    float pointRowGap = 36.0f;
    float pointColumnGap = 34.0f;
    int32_t pointRow = 20;
    int32_t pointColumn = 4;
    std::vector<int32_t> hollowRow{1, 18};//start 0
    std::vector<int32_t> hollowColumn{1, 2};

    bool isCurrentRowAndColumnHollow(float row, float column)
    {
        return (std::find(hollowRow.begin(), hollowRow.end(), row) != hollowRow.end())
            && (std::find(hollowColumn.begin(), hollowColumn.end(), column) != hollowColumn.end());
    }
};

using MRGearsShineOptionsSharedPtr = std::shared_ptr<MRGearsShineOptions>;

class MRGearsShine : public UIWidget
{
public:
    static std::shared_ptr<MRGearsShine> create();

    void initialize() override;
private:
    MRGearsShine();
    MRGearsShineOptionsSharedPtr m_options;
};
using MRGearsShineSharedPtr = std::shared_ptr<MRGearsShine>;
}

#endif //MORROW_MRGearsShine_H
