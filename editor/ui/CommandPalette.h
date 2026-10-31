#ifndef MORROW_EDITOR_COMMAND_PALETTE_H
#define MORROW_EDITOR_COMMAND_PALETTE_H

#include <memory>
#include <string>
#include <vector>

#include <functional>

#include "morrow/core/Observable.h"
#include "morrow/base/EventDispatcher.h"
#include "morrow/base/UIWidget.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRColor;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
}  // namespace morrow

namespace morrow::editor {
class EditorShell;

/// 命令面板（Ctrl+P）：中央弹出，过滤搜索 + 命令行列表，Enter/点击执行。
/// 搜索输入复用 Inspector 的 legacy 键盘通道（属性名 "__palette__"）。
class CommandPalette : public morrow::UIWidget {
public:
    struct Entry {
        std::wstring label;
        std::function<void()> action;
    };

    explicit CommandPalette(EditorShell& shell);

    void open();
    void close();
    bool isOpen() const {
        return m_open;
    }

    /// 过滤词变化或打开时重建命令行列表。
    void refreshEntries();

    void setFilter(const std::string& filter);
    const std::string& filter() const {
        return m_filter;
    }

    /// Enter：执行第一条可见命令并关闭。
    void executeFirst();

private:
    void buildChrome();
    std::vector<Entry> collectEntries() const;

    EditorShell& m_shell;
    bool m_chromeBuilt = false;
    bool m_open = false;
    std::string m_filter;
    std::shared_ptr<morrow::MRColor> m_backdrop;
    std::shared_ptr<morrow::MRLabel> m_hint;
    std::shared_ptr<morrow::MRLabel> m_filterLabel;
    std::shared_ptr<morrow::MRLabel> m_emptyHint;
    std::vector<std::shared_ptr<morrow::MRButton>> m_commandButtons;
    std::vector<Observable<morrow::BaseButton&>::Connection> m_buttonConnections;
    std::vector<Entry> m_visible;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_COMMAND_PALETTE_H
