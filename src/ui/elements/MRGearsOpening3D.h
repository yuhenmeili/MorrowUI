//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_GEARS_OPENING3D_H
#define MORROW_GEARS_OPENING3D_H


#include <memory>
#include "base/Widget.h"
#include "Texture.h"
#include "base/UIWidget.h"

namespace morrow
{
struct MRGearsOpening3DOptions
{
    bool forward = true;
    float blurRadius = 30.0f;
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

using MRGearsOpening3DOptionsSharedPtr = std::shared_ptr<MRGearsOpening3DOptions>;

class MRGearsOpening3D : public UIWidget
{
public:
    static std::shared_ptr<MRGearsOpening3D> create();

    void initialize() override;

private:
    MRGearsOpening3D();
    MRGearsOpening3DOptionsSharedPtr m_options;
};
using MRGearsOpening3DSharedPtr = std::shared_ptr<MRGearsOpening3D>;
}

#endif //MORROW_GEARS_OPENING3D_H
