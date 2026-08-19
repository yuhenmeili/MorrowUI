#ifndef MORROW_GUI_MRSCROLLCONTAINER_H
#define MORROW_GUI_MRSCROLLCONTAINER_H

#include <memory>

#include "MRScrollBar.h"

namespace morrow {

class MRScrollContainer : public UIWidget {
public:
    static std::shared_ptr<MRScrollContainer> create();

    void setContent(const std::shared_ptr<UIWidget>& content);
    std::shared_ptr<UIWidget> getContent() const {
        return m_content;
    }
    void addScrollChild(const std::shared_ptr<Widget>& child);

    void setScrollStep(float step);
    float getScrollOffset() const {
        return m_scrollOffset;
    }
    float getMaxScrollOffset() const {
        return m_maxScrollOffset;
    }
    void setScrollOffset(float offset);

    MRScrollBarSharedPtr getVerticalScrollBar() const {
        return m_scrollBar;
    }
    void update(FrameStateSharedPtr frameState) override;

private:
    MRScrollContainer();

    void attachInput(const std::shared_ptr<Widget>& widget);
    void attachInputRecursive(const std::shared_ptr<Widget>& widget);
    void scrollBy(float amount);
    void refreshMetrics();
    void updateChildVisibility();

    std::shared_ptr<UIWidget> m_content;
    MRScrollBarSharedPtr m_scrollBar;
    float m_scrollOffset = 0.0f;
    float m_maxScrollOffset = 0.0f;
    float m_scrollStep = 48.0f;
    bool m_metricsDirty = true;
    bool m_dragging = false;
    float m_lastDragY = 0.0f;
};

using MRScrollContainerSharedPtr = std::shared_ptr<MRScrollContainer>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSCROLLCONTAINER_H
