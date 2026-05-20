//
// Created by lance on 2023/4/21.
//

#ifndef MORROW_GEARS_GRIDS_H
#define MORROW_GEARS_GRIDS_H

#include <memory>
#include "base/Widget.h"

#include "Texture.h"
#include "base/UIWidget.h"

namespace morrow
{
struct MRGearsGridsOptions
{
    float pointSize = 5.0f;
    float pointColorAlphaOffset = 0.4f;
    float pointRowGap = 36.0f;
    float pointColumnGap = 34.0f;
    int32_t pointRow = 20;
    int32_t pointColumn = 4;
    std::vector<int32_t> hollowRow {1,18};//start 0
    std::vector<int32_t> hollowColumn {1,2};

    bool isCurrentRowAndColumnHollow(float row, float column){
        return (std::find(hollowRow.begin(), hollowRow.end(), row) != hollowRow.end())
        && (std::find(hollowColumn.begin(), hollowColumn.end(), column) != hollowColumn.end());
    }
};
using MRGearsGridsOptionsSharedPtr = std::shared_ptr<MRGearsGridsOptions>;

class MRGearsGrids : public UIWidget
{
public:
    static std::shared_ptr<MRGearsGrids> create();

    void initialize() override;

private:
    MRGearsGrids();
    MRGearsGridsOptionsSharedPtr m_options;
};
using MRGearsGridsSharedPtr = std::shared_ptr<MRGearsGrids>;
}

#endif //MORROW_GEARS_GRIDS_H

