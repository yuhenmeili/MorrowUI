#ifndef MORROW_EDITOR_BUILD_PANEL_H
#define MORROW_EDITOR_BUILD_PANEL_H

#include <future>
#include <memory>
#include <string>

#include "build/BuildQueue.h"

namespace morrow {
class MRTextEdit;
class UIWidget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;
enum class PreviewState;

class BuildPanel {
public:
    explicit BuildPanel(EditorShell& shell);

    void run(BuildTaskKind kind);

    void poll();

    void stop();

    void setPreviewState(PreviewState state);

    void appendResult(const BuildTaskResult& result);

    void resize();

    std::shared_ptr<UIWidget> panel;
    std::shared_ptr<MRTextEdit> log;

private:
    friend class EditorShell;
    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif