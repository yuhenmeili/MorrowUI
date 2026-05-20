//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_GEARS_OPENING_H
#define MORROW_GEARS_OPENING_H


#include <memory>
#include "base/UIWidget.h"

namespace morrow
{
struct MRGearsOpeningOptions
{
    bool forward = true;
    float blurRadius = 50.0f;
};
using MRGearsOpeningOptionsSharedPtr = std::shared_ptr<MRGearsOpeningOptions>;

class MRGearsOpening : public UIWidget
{
public:
    static std::shared_ptr<MRGearsOpening> create();

    void initialize() override;
private:
    MRGearsOpening();
    MRGearsOpeningOptionsSharedPtr m_options;
};
using MRGearsOpeningSharedPtr = std::shared_ptr<MRGearsOpening>;
}

#endif //MORROW_GEARS_OPENING_H
