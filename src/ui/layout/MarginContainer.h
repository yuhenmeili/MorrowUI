//
// 内边距容器：为唯一子节点添加四周内边距，子节点被放置在 (left, top) 且尺寸为容器尺寸减去边距
//

#ifndef MARGINCONTAINER_H
#define MARGINCONTAINER_H

#include "base/UIWidget.h"
#include "base/Transform.h"

namespace morrow {
class MarginContainer : public UIWidget {
public:
    MarginContainer();

    void setMarginLeft(float value);

    void setMarginTop(float value);

    void setMarginRight(float value);

    void setMarginBottom(float value);

    void setMargin(float left, float top, float right, float bottom);

    void setMarginAll(float value);

    float getMarginLeft() const { return m_marginLeft; }

    float getMarginTop() const { return m_marginTop; }

    float getMarginRight() const { return m_marginRight; }

    float getMarginBottom() const { return m_marginBottom; }

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();

    float m_marginLeft = 0.0f;
    float m_marginTop = 0.0f;
    float m_marginRight = 0.0f;
    float m_marginBottom = 0.0f;
};
} // morrow

#endif // MARGINCONTAINER_H
