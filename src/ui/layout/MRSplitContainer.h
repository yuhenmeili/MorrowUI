#ifndef MORROW_MR_SPLIT_CONTAINER_H
#define MORROW_MR_SPLIT_CONTAINER_H

#include <memory>
#include <vector>

#include "base/EventDispatcher.h"
#include "base/UIWidget.h"
#include "core/Observable.h"

namespace morrow {

class MRButton;

enum class SplitOrientation {
    Horizontal,
    Vertical,
};

class MRSplitContainer : public UIWidget {
public:
    struct Events {
        Observable<MRSplitContainer&, float> onSplitRatioChanged;
        Observable<MRSplitContainer&, float> onDragFinished;
    };

    static std::shared_ptr<MRSplitContainer> create();

    void setOrientation(SplitOrientation orientation);
    SplitOrientation getOrientation() const;

    void setFirst(const std::shared_ptr<UIWidget>& widget);
    void setSecond(const std::shared_ptr<UIWidget>& widget);

    std::shared_ptr<UIWidget> getFirst() const;
    std::shared_ptr<UIWidget> getSecond() const;

    void setSplitRatio(float ratio);
    float getSplitRatio() const;

    void setFirstMinSize(float size);
    void setSecondMinSize(float size);
    void setHandleWidth(float width);

    float getFirstMinSize() const;
    float getSecondMinSize() const;
    float getHandleWidth() const;

    Events& events();

    void update(FrameStateSharedPtr frameState) override;

private:
    MRSplitContainer();

    void initializeHandle();
    void layoutChildren();
    float clampRatio(float ratio) const;
    void updateRatioFromPointer(float x, float y);
    void updateCursor(bool active);

    SplitOrientation m_orientation = SplitOrientation::Horizontal;
    std::shared_ptr<UIWidget> m_first;
    std::shared_ptr<UIWidget> m_second;
    std::shared_ptr<MRButton> m_handle;
    std::vector<EventConnection> m_handleConnections;
    Events m_events;
    float m_splitRatio = 0.5f;
    float m_firstMinSize = 80.0f;
    float m_secondMinSize = 80.0f;
    float m_handleWidth = 5.0f;
    bool m_dragging = false;
};

using MRSplitContainerSharedPtr = std::shared_ptr<MRSplitContainer>;

}  // namespace morrow

#endif  // MORROW_MR_SPLIT_CONTAINER_H
