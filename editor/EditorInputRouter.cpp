#include "EditorInputRouter.h"

#include <cctype>

#include "scene/EditorSession.h"

namespace morrow::editor {

EditorInputRouter::EditorInputRouter(EditorSession& session) : m_session(session) {
}

bool EditorInputRouter::handleShortcut(bool control, bool shift, char key, std::string& error) {
    if (!control)
        return false;
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(key)))) {
        case 's':
            return m_session.save(error);
        case 'z':
            return shift ? m_session.redo(error) : m_session.undo(error);
        case 'y':
            return m_session.redo(error);
        default:
            return false;
    }
}

}  // namespace morrow::editor
