//
// 居中布局容器：将子节点（通常仅一个）在容器内水平、垂直居中
//

#ifndef CENTERCONTAINER_H
#define CENTERCONTAINER_H

#include "base/UIWidget.h"
#include "base/Transform.h"

namespace morrow {
class CenterContainer : public UIWidget {
public:
    CenterContainer();

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();
};
} // morrow

#endif // CENTERCONTAINER_H
