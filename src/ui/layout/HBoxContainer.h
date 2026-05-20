//
// 水平布局容器：子节点自左向右排列，局部坐标系左上角为 (0,0)
//

#ifndef HBOXCONTAINER_H
#define HBOXCONTAINER_H

#include "base/UIWidget.h"
#include "base/Transform.h"

namespace morrow {
class HBoxContainer : public UIWidget {
public:
    HBoxContainer();

    void setSpacing(float spacing);

    float getSpacing() const { return m_spacing; }

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();

    float m_spacing = 0.0f;
};
} // morrow

#endif // HBOXCONTAINER_H
