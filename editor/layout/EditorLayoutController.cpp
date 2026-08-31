#include "layout/EditorLayoutController.h"

#include <algorithm>

#include "EditorShell.h"
#include "base/Transform.h"
#include "layout/MRSplitContainer.h"
#include "layout/MRTabContainer.h"
#include "ui/DockDropOverlay.h"

namespace morrow::editor {

EditorLayoutController::EditorLayoutController(EditorShell& shell) : m_shell(shell) {
}

void EditorLayoutController::attach(DockLayout& layout, const std::filesystem::path& path, const std::shared_ptr<MRSplitContainer>& workspaceSplit,
                                    const std::shared_ptr<MRSplitContainer>& mainSplit, const std::shared_ptr<MRSplitContainer>& centerSplit,
                                    const std::shared_ptr<MRTabContainer>& left, const std::shared_ptr<MRTabContainer>& centerTabsContainer,
                                    const std::shared_ptr<MRTabContainer>& bottom, const std::shared_ptr<DockDropOverlay>& dockOverlay) {
    m_layout = &layout;
    layoutPath = path;
    workspace = workspaceSplit;
    main = mainSplit;
    center = centerSplit;
    leftTabs = left;
    centerTabs = centerTabsContainer;
    bottomTabs = bottom;
    overlay = dockOverlay;
}

void EditorLayoutController::apply() {
    if (!m_layout)
        return;
    if (const auto* split = m_layout->findSplit("workspace"))
        workspace->setSplitRatio(split->ratio);
    if (const auto* split = m_layout->findSplit("left"))
        main->setSplitRatio(split->ratio);
    if (const auto* split = m_layout->findSplit("center"))
        center->setSplitRatio(split->ratio);

    const auto applyTabs = [](const DockTabState* state, const std::shared_ptr<MRTabContainer>& tabs) {
        if (!state || !tabs)
            return;
        for (size_t target = 0; target < state->panels.size(); ++target) {
            size_t current = target;
            while (current < tabs->tabs().size() && tabs->tabs()[current].id != state->panels[target])
                ++current;
            if (current < tabs->tabs().size() && current != target)
                tabs->moveTab(current, target);
        }
        tabs->selectTab(state->active);
    };
    applyTabs(m_layout->findTabs("left_dock"), leftTabs);
    applyTabs(m_layout->findTabs("center_dock"), centerTabs);
    applyTabs(m_layout->findTabs("bottom_dock"), bottomTabs);
    if (overlay)
        overlay->setWorkspaceBounds(workspace->getScreenSpaceAABB());
}

void EditorLayoutController::save() {
    std::string error;
    if (m_layout && !m_layout->save(layoutPath, error))
        m_shell.setStatus(error);
}

void EditorLayoutController::syncTabs() {
    const auto sync = [this](const std::string& id, const std::shared_ptr<MRTabContainer>& tabs) {
        if (m_layout) {
            auto* state = m_layout->findTabs(id);
            if (!state)
                return;
            state->active = tabs->currentTabId();
            state->panels.clear();
            for (const auto& tab : tabs->tabs())
                state->panels.push_back(tab.id);
        }
    };
    sync("left_dock", leftTabs);
    sync("center_dock", centerTabs);
    sync("bottom_dock", bottomTabs);
}

void EditorLayoutController::resize(const Vector2& size) {
    if (workspace)
        workspace->getTransform()->setSize(size.x, std::max(1.0f, size.y - 40.0f));
    apply();
}

DockDropZone EditorLayoutController::dropZoneAt(float x, float y) const {
    if (!workspace)
        return DockDropZone::None;
    const Math::Rect bounds = workspace->getScreenSpaceAABB();
    if (!bounds.Contains(x, y))
        return DockDropZone::None;
    const float nx = (x - bounds.Min.x) / std::max(1.0f, bounds.GetWidth());
    const float ny = (y - bounds.Min.y) / std::max(1.0f, bounds.GetHeight());
    if (nx < 0.20f)
        return DockDropZone::Left;
    if (nx > 0.80f)
        return DockDropZone::Right;
    if (ny < 0.20f)
        return DockDropZone::Top;
    if (ny > 0.80f)
        return DockDropZone::Bottom;
    return DockDropZone::Center;
}

std::shared_ptr<MRTabContainer> EditorLayoutController::tabContainerForId(const std::string& id) const {
    if (id == "left_dock")
        return leftTabs;
    if (id == "center_dock")
        return centerTabs;
    if (id == "bottom_dock")
        return bottomTabs;
    return nullptr;
}

void EditorLayoutController::completeDrop(float x, float y) {
    const auto state = m_dragDrop.state();
    const DockDropZone zone = dropZoneAt(x, y);
    if (overlay) {
        overlay->setVisible(false);
        overlay->setZone(DockDropZone::None);
    }
    m_dragDrop.drop(x, y);
    if (zone == DockDropZone::None)
        return;

    std::shared_ptr<MRTabContainer> target;
    if (zone == DockDropZone::Bottom)
        target = bottomTabs;
    else if (zone == DockDropZone::Left)
        target = leftTabs;
    else if (zone == DockDropZone::Right || zone == DockDropZone::Top)
        target = centerTabs;
    else {
        target = bottomTabs->getScreenSpaceAABB().Contains(x, y) ? bottomTabs : (leftTabs->getScreenSpaceAABB().Contains(x, y) ? leftTabs : centerTabs);
    }
    const auto source = tabContainerForId(m_pendingGroup);
    if (!target || !source)
        return;
    MRTabContainer::DetachedTab detached;
    if (!source->detachTab(state.payload.id, detached))
        return;
    if (!target->addTab(detached.id, detached.title, detached.content)) {
        source->addTab(detached.id, detached.title, detached.content);
        return;
    }
    target->selectTab(detached.id);
    syncTabs();
    save();
}

bool EditorLayoutController::handleDrop(const TouchEvent& event) {
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        const std::string leftId = leftTabs ? leftTabs->tabIdForWidget(event.target) : std::string{};
        const std::string bottomId = bottomTabs ? bottomTabs->tabIdForWidget(event.target) : std::string{};
        const std::string centerId = centerTabs ? centerTabs->tabIdForWidget(event.target) : std::string{};
        if (!leftId.empty()) {
            m_pendingTab = leftId;
            m_pendingGroup = "left_dock";
        } else if (!bottomId.empty()) {
            m_pendingTab = bottomId;
            m_pendingGroup = "bottom_dock";
        } else if (!centerId.empty()) {
            m_pendingTab = centerId;
            m_pendingGroup = "center_dock";
        } else
            return false;
        m_pendingX = event.positionX;
        m_pendingY = event.positionY;
        return false;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE) {
        if (m_dragDrop.isActive() && m_dragDrop.state().payload.type == "editor/dock-panel") {
            m_dragDrop.update(event.positionX, event.positionY);
            if (overlay) {
                overlay->setVisible(true);
                overlay->setZone(dropZoneAt(event.positionX, event.positionY));
            }
            return true;
        }
        if (!m_pendingTab.empty()) {
            const float dx = event.positionX - m_pendingX;
            const float dy = event.positionY - m_pendingY;
            if (dx * dx + dy * dy >= 36.0f) {
                DragPayload payload{"editor/dock-panel", m_pendingTab, event.target};
                m_dragDrop.begin(payload, event.positionX, event.positionY);
                return true;
            }
        }
        return false;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        if (m_dragDrop.isActive() && m_dragDrop.state().payload.type == "editor/dock-panel") {
            completeDrop(event.positionX, event.positionY);
            m_pendingTab.clear();
            m_pendingGroup.clear();
            return true;
        }
        m_pendingTab.clear();
        m_pendingGroup.clear();
    }
    return false;
}

}  // namespace morrow::editor
