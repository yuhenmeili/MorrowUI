#include "DockLayout.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace morrow::editor {

DockLayout DockLayout::defaultLayout(float width, float height) {
    DockLayout layout;
    constexpr float gap = 5.0f;
    const float toolbar = 40.0f;
    const float output = std::max(140.0f, height * 0.15f);
    const float contentY = toolbar + gap;
    const float contentHeight = std::max(1.0f, height - contentY - output - gap);
    const float sceneWidth = std::max(280.0f, width * 0.16f);
    const float inspectorWidth = std::max(320.0f, width * 0.18f);
    const float viewportX = sceneWidth + gap;
    const float viewportWidth = std::max(1.0f, width - sceneWidth - inspectorWidth - gap * 2.0f);
    const float inspectorX = viewportX + viewportWidth + gap;
    layout.m_panels = {
        {"scene_tree", 0.0f, contentY, sceneWidth, contentHeight, true, 0},
        {"viewport", viewportX, contentY, viewportWidth, contentHeight, true, 1},
        {"inspector", inspectorX, contentY, inspectorWidth, contentHeight, true, 2},
        {"output", 0.0f, height - output, width, output, true, 3},
    };
    const float workspaceAvailable = std::max(1.0f, height - toolbar - gap);
    const float mainHeight = std::max(1.0f, height - toolbar - output - gap);
    const float horizontalAvailable = std::max(1.0f, width - gap);
    const float centerAvailable = std::max(1.0f, viewportWidth + inspectorWidth);
    layout.m_splits = {
        {"workspace", mainHeight / workspaceAvailable},
        {"left", sceneWidth / horizontalAvailable},
        {"center", viewportWidth / centerAvailable},
        {"left_stack", 0.48f},
    };
    layout.m_tabs = {
        {"left_dock", "scene_tree", {"scene_tree", "filesystem"}},
        {"bottom_dock", "output", {"output", "build"}},
    };
    return layout;
}

DockPanelState* DockLayout::find(const std::string& id) {
    for (auto& panel : m_panels)
        if (panel.id == id)
            return &panel;
    return nullptr;
}

const DockPanelState* DockLayout::find(const std::string& id) const {
    for (const auto& panel : m_panels)
        if (panel.id == id)
            return &panel;
    return nullptr;
}

DockSplitState* DockLayout::findSplit(const std::string& id) {
    for (auto& split : m_splits)
        if (split.id == id)
            return &split;
    return nullptr;
}

const DockSplitState* DockLayout::findSplit(const std::string& id) const {
    for (const auto& split : m_splits)
        if (split.id == id)
            return &split;
    return nullptr;
}

bool DockLayout::load(const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "failed to open dock layout: " + path.string();
        return false;
    }
    std::vector<DockPanelState> loaded;
    std::vector<DockSplitState> loadedSplits;
    std::vector<DockTabState> loadedTabs;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream row(line);
        std::string kindOrId;
        row >> kindOrId;
        if (kindOrId == "split") {
            DockSplitState split;
            if (!(row >> split.id >> split.ratio)) {
                error = "invalid split layout at line " + std::to_string(lineNumber);
                return false;
            }
            split.ratio = std::clamp(split.ratio, 0.0f, 1.0f);
            loadedSplits.push_back(std::move(split));
            continue;
        }
        if (kindOrId == "tabs") {
            DockTabState tabs;
            size_t panelCount = 0;
            if (!(row >> tabs.id >> tabs.active >> panelCount)) {
                error = "invalid tab layout at line " +
                        std::to_string(lineNumber);
                return false;
            }
            for (size_t index = 0; index < panelCount; ++index) {
                std::string panelId;
                if (!(row >> panelId)) {
                    error = "invalid tab panel list at line " +
                            std::to_string(lineNumber);
                    return false;
                }
                tabs.panels.push_back(std::move(panelId));
            }
            loadedTabs.push_back(std::move(tabs));
            continue;
        }
        DockPanelState panel;
        panel.id = std::move(kindOrId);
        int visible = 1;
        if (!(row >> panel.x >> panel.y >> panel.width >> panel.height >> visible >> panel.order)) {
            error = "invalid dock layout at line " + std::to_string(lineNumber);
            return false;
        }
        panel.visible = visible != 0;
        loaded.push_back(std::move(panel));
    }
    if (loaded.empty() && loadedSplits.empty() && loadedTabs.empty()) {
        error = "dock layout contains no panels";
        return false;
    }
    m_panels = std::move(loaded);
    m_splits = std::move(loadedSplits);
    m_tabs = std::move(loadedTabs);
    if (m_splits.empty() && !m_panels.empty()) {
        float totalWidth = 0.0f;
        float totalHeight = 0.0f;
        for (const auto& panel : m_panels) {
            totalWidth = std::max(totalWidth, panel.x + panel.width);
            totalHeight = std::max(totalHeight, panel.y + panel.height);
        }
        const auto* scene = find("scene_tree");
        const auto* viewport = find("viewport");
        const auto* inspector = find("inspector");
        const auto* output = find("output");
        if (scene)
            m_splits.push_back({"left", scene->width / std::max(1.0f, totalWidth)});
        if (viewport && inspector)
            m_splits.push_back(
                {"center", viewport->width / std::max(1.0f, viewport->width + inspector->width)});
        if (output)
            m_splits.push_back(
                {"workspace", (output->y - 40.0f) / std::max(1.0f, totalHeight - 40.0f)});
    }
    return true;
}

bool DockLayout::save(const std::filesystem::path& path, std::string& error) const {
    std::error_code filesystemError;
    std::filesystem::create_directories(path.parent_path(), filesystemError);
    if (filesystemError) {
        error = "failed to create dock layout directory: " + filesystemError.message();
        return false;
    }
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        error = "failed to write dock layout: " + path.string();
        return false;
    }
    output << "# MorrowEditor dock layout v3\n";
    for (const auto& split : m_splits) {
        output << "split " << split.id << ' ' << split.ratio << '\n';
    }
    for (const auto& tabs : m_tabs) {
        output << "tabs " << tabs.id << ' ' << tabs.active << ' '
               << tabs.panels.size();
        for (const auto& panel : tabs.panels)
            output << ' ' << panel;
        output << '\n';
    }
    return output.good();
}

std::vector<DockPanelState>& DockLayout::panels() {
    return m_panels;
}
const std::vector<DockPanelState>& DockLayout::panels() const {
    return m_panels;
}

std::vector<DockSplitState>& DockLayout::splits() {
    return m_splits;
}

const std::vector<DockSplitState>& DockLayout::splits() const {
    return m_splits;
}

DockTabState* DockLayout::findTabs(const std::string& id) {
    for (auto& tabs : m_tabs) {
        if (tabs.id == id)
            return &tabs;
    }
    return nullptr;
}

const DockTabState* DockLayout::findTabs(const std::string& id) const {
    for (const auto& tabs : m_tabs) {
        if (tabs.id == id)
            return &tabs;
    }
    return nullptr;
}

std::vector<DockTabState>& DockLayout::tabs() {
    return m_tabs;
}

const std::vector<DockTabState>& DockLayout::tabs() const {
    return m_tabs;
}

}  // namespace morrow::editor
