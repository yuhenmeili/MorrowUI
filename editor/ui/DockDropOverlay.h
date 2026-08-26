#ifndef MORROW_EDITOR_DOCK_DROP_OVERLAY_H
#define MORROW_EDITOR_DOCK_DROP_OVERLAY_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow::editor {
enum class DockDropZone {
    None,
    Center,
    Left,
    Right,
    Top,
    Bottom,
};

class DockDropOverlay : public UIWidget {
public:
    static std::shared_ptr<DockDropOverlay> create();

    void setWorkspaceBounds(const Math::Rect& bounds);

    void setZone(DockDropZone zone);

    DockDropZone zone() const;

private:
    DockDropOverlay();

    void initializeZones();

    void layoutZones();

    Math::Rect m_workspaceBounds;
    DockDropZone m_zone = DockDropZone::None;
    std::shared_ptr<UIWidget> m_center;
    std::shared_ptr<UIWidget> m_left;
    std::shared_ptr<UIWidget> m_right;
    std::shared_ptr<UIWidget> m_top;
    std::shared_ptr<UIWidget> m_bottom;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_DOCK_DROP_OVERLAY_H