//
// 垂直布局容器：子节点自上而下排列，局部坐标系左上角为 (0,0)
//

#ifndef VBOXCONTAINER_H
#define VBOXCONTAINER_H

#include "base/UIWidget.h"
#include "base/Transform.h"

namespace morrow {
class VBoxContainer : public UIWidget {
public:
    VBoxContainer();

    void setSpacing(float spacing);

    float getSpacing() const { return m_spacing; }

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();

    float m_spacing = 0.0f;
};
} // morrow

#endif // VBOXCONTAINER_H
