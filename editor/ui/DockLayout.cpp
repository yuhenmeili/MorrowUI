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

bool DockLayout::load(const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "failed to open dock layout: " + path.string();
        return false;
    }
    std::vector<DockPanelState> loaded;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream row(line);
        DockPanelState panel;
        int visible = 1;
        if (!(row >> panel.id >> panel.x >> panel.y >> panel.width >> panel.height >> visible >> panel.order)) {
            error = "invalid dock layout at line " + std::to_string(lineNumber);
            return false;
        }
        panel.visible = visible != 0;
        loaded.push_back(std::move(panel));
    }
    if (loaded.empty()) {
        error = "dock layout contains no panels";
        return false;
    }
    m_panels = std::move(loaded);
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
    output << "# MorrowEditor dock layout v1\n";
    for (const auto& panel : m_panels) {
        output << panel.id << ' ' << panel.x << ' ' << panel.y << ' ' << panel.width << ' ' << panel.height << ' ' << (panel.visible ? 1 : 0) << ' ' << panel.order << '\n';
    }
    return output.good();
}

std::vector<DockPanelState>& DockLayout::panels() {
    return m_panels;
}
const std::vector<DockPanelState>& DockLayout::panels() const {
    return m_panels;
}

}  // namespace morrow::editor
