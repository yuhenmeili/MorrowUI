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
struct MRGearsSelectOptions
{
    Vector4 color = {125.0f / 255.0f, 51.0f / 255.0f, 1.0f, 1.0f};
    float pointSize = 6.0f;
};

using MRGearsSelectOptionsSharedPtr = std::shared_ptr<MRGearsSelectOptions>;

class MRGearsSelect : public UIWidget
{
public:
    static std::shared_ptr<MRGearsSelect> create();

    void initialize() override;

private:
    MRGearsSelect();
    MRGearsSelectOptionsSharedPtr m_options;
};

using MRGearsSelectSharedPtr = std::shared_ptr<MRGearsSelect>;
}

#endif //MORROW_GEARS_SELECT_H
