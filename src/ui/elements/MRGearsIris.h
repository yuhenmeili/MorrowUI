//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_GEARS_IRIS_H
#define MORROW_GEARS_IRIS_H

#include <memory>

#include "base/UIWidget.h"
#include "base/Widget.h"

namespace morrow
{
struct MRGearsIrisOptions
{
    float circleRadius = 30.0f;
    float circleDelay = 0.4f;
    Vector4 circleColor = {0.2f, 0.2f, 0.2f, 0.2f};
    float gridGap = 13.0f;
    float gridResetTime = 0.2f;//ms
    Vector4 gridHalfWidthParameter = {4.0f, 3.0f, 2.0f, 1.0f};//小格子属性
    Vector4 gridColor = {0.57f, 0.57f, 0.57f, 0.6f};
};
using MRGearsIrisOptionsSharedPtr = std::shared_ptr<MRGearsIrisOptions>;

class MRGearsIris : public UIWidget
{
public:
    static std::shared_ptr<MRGearsIris> create();

    void initialize() override;
private:
    MRGearsIris();
    MRGearsIrisOptionsSharedPtr m_options;
};

using MRGearsIrisSharedPtr = std::shared_ptr<MRGearsIris>;
}

#endif //MORROW_GEARS_IRIS_H
