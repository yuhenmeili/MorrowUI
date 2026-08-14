#ifndef MORROW_EDITOR_DOCK_LAYOUT_H
#define MORROW_EDITOR_DOCK_LAYOUT_H

#include <filesystem>
#include <string>
#include <vector>

namespace morrow::editor {

struct DockPanelState {
    std::string id;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool visible = true;
    int order = 0;
};

class DockLayout {
public:
    static DockLayout defaultLayout(float width, float height);

    DockPanelState* find(const std::string& id);

    const DockPanelState* find(const std::string& id) const;

    bool load(const std::filesystem::path& path, std::string& error);

    bool save(const std::filesystem::path& path, std::string& error) const;

    std::vector<DockPanelState>& panels();

    const std::vector<DockPanelState>& panels() const;

private:
    std::vector<DockPanelState> m_panels;
};

}  // namespace morrow::editor

#endif
