#ifndef MORROW_EDITOR_PANEL_H
#define MORROW_EDITOR_PANEL_H

#include <memory>
#include <string>

namespace morrow {
class UIWidget;
struct TouchEvent;
}

namespace morrow::editor {

class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    virtual bool canAcceptDrop(const std::string& payloadType) const {
        return false;
    }

    virtual bool onDragMove(const TouchEvent&) {
        return false;
    }

    virtual bool onDrop(const TouchEvent&) {
        return false;
    }

    bool contains(float x, float y) const;

    std::shared_ptr<UIWidget> root() const {
        return m_root;
    }

protected:
    std::shared_ptr<UIWidget> m_root;
};

}

#endif
