#ifndef MORROW_DRAG_DROP_MANAGER_H
#define MORROW_DRAG_DROP_MANAGER_H

#include <memory>
#include <string>

#include "base/Widget.h"
#include "core/Observable.h"

namespace morrow {

struct DragPayload {
    std::string type;
    std::string id;
    std::shared_ptr<Widget> source;
};

struct DragState {
    bool active = false;
    DragPayload payload;
    float x = 0.0f;
    float y = 0.0f;
};

class DragDropManager {
public:
    Observable<const DragState&> onDragStarted;
    Observable<const DragState&> onDragMoved;
    Observable<const DragState&> onDragDropped;
    Observable<const DragState&> onDragCancelled;

    bool begin(const DragPayload& payload, float x, float y);
    bool update(float x, float y);
    bool drop(float x, float y);
    bool cancel();

    bool isActive() const;
    const DragState& state() const;

private:
    DragState m_state;
};

}  // namespace morrow

#endif  // MORROW_DRAG_DROP_MANAGER_H
