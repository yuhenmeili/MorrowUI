//
// Created by lance on 2023/2/22.
//

#ifndef MORROW_FLOWING_LIGHT_H
#define MORROW_FLOWING_LIGHT_H

#include <memory>
#include "base/UIWidget.h"

namespace morrow
{
struct MRFlowingLightOptions
{
    float flowingLightLength = 100.0f;
    Vector4 flowingLightColor = {0.5f,0.2f,1.0f,1.0f};
    float flowingLightThickness = 4.0f;
};

using MRFlowingLightOptionsSharedPtr = std::shared_ptr<MRFlowingLightOptions>;

class MRFlowingLight : public UIWidget
{
public:
    static std::shared_ptr<MRFlowingLight> create();

    void initialize() override;
private:
    MRFlowingLight();
    MRFlowingLightOptionsSharedPtr m_options;
};
using MRFlowingLightSharedPtr = std::shared_ptr<MRFlowingLight>;
}

#endif //MORROW_FLOWING_LIGHT_H
