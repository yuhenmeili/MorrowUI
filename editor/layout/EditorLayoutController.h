#ifndef MORROW_EDITOR_LAYOUT_CONTROLLER_H
#define MORROW_EDITOR_LAYOUT_CONTROLLER_H

#include <filesystem>
#include <memory>
#include <string>

#include "Vector2.h"
#include "layout/DragDropManager.h"
#include "ui/DockDropOverlay.h"
#include "ui/DockLayout.h"

namespace morrow {
class MRSplitContainer;
class MRTabContainer;
class TouchEvent;
class UIWidget;
}  // namespace morrow

namespace morrow::editor {
class EditorShell;

class EditorLayoutController {
public:
    explicit EditorLayoutController(EditorShell& shell);

    void attach(DockLayout& layout, const std::filesystem::path& layoutPath, const std::shared_ptr<MRSplitContainer>& workspace, const std::shared_ptr<MRSplitContainer>& main,
                const std::shared_ptr<MRSplitContainer>& center, const std::shared_ptr<MRTabContainer>& leftTabs, const std::shared_ptr<MRTabContainer>& centerTabs,
                const std::shared_ptr<MRTabContainer>& bottomTabs, const std::shared_ptr<DockDropOverlay>& overlay);

    void apply();

    void save();

    void syncTabs();

    void resize(const Vector2& size);

    bool handleDrop(const TouchEvent& event);

    DockDropZone dropZoneAt(float x, float y) const;

    std::shared_ptr<MRTabContainer> tabContainerForId(const std::string& id) const;

    DockLayout& model() {
        return *m_layout;
    }

    std::shared_ptr<MRSplitContainer> workspace;
    std::shared_ptr<MRSplitContainer> main;
    std::shared_ptr<MRSplitContainer> center;
    std::shared_ptr<MRTabContainer> leftTabs;
    std::shared_ptr<MRTabContainer> centerTabs;
    std::shared_ptr<MRTabContainer> bottomTabs;
    std::shared_ptr<DockDropOverlay> overlay;
    std::filesystem::path layoutPath;

private:
    void completeDrop(float x, float y);

    EditorShell& m_shell;
    DockLayout* m_layout = nullptr;
    std::string m_pendingTab;
    std::string m_pendingGroup;
    float m_pendingX = 0.0f;
    float m_pendingY = 0.0f;
    DragDropManager m_dragDrop;
};
}  // namespace morrow::editor

#endif