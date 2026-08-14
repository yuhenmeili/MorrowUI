#ifndef MORROW_EDITOR_INPUT_ROUTER_H
#define MORROW_EDITOR_INPUT_ROUTER_H

#include <string>

namespace morrow::editor {

class EditorSession;

class EditorInputRouter {
public:
    explicit EditorInputRouter(EditorSession& session);

    bool handleShortcut(bool control, bool shift, char key, std::string& error);

private:
    EditorSession& m_session;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_INPUT_ROUTER_H
