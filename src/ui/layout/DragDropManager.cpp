#include "DragDropManager.h"

namespace morrow {

bool DragDropManager::begin(const DragPayload& payload, float x, float y) {
    if (m_state.active || payload.type.empty() || payload.id.empty())
        return false;
    m_state = {true, payload, x, y};
    onDragStarted.notify(m_state);
    return true;
}

bool DragDropManager::update(float x, float y) {
    if (!m_state.active)
        return false;
    m_state.x = x;
    m_state.y = y;
    onDragMoved.notify(m_state);
    return true;
}

bool DragDropManager::drop(float x, float y) {
    if (!m_state.active)
        return false;
    m_state.x = x;
    m_state.y = y;
    onDragDropped.notify(m_state);
    m_state = {};
    return true;
}

bool DragDropManager::cancel() {
    if (!m_state.active)
        return false;
    onDragCancelled.notify(m_state);
    m_state = {};
    return true;
}

bool DragDropManager::isActive() const {
    return m_state.active;
}

const DragState& DragDropManager::state() const {
    return m_state;
}

}  // namespace morrow
