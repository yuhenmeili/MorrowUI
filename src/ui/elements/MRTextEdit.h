#ifndef MORROW_GUI_MRTEXTEDIT_H
#define MORROW_GUI_MRTEXTEDIT_H

#include <memory>
#include <string>
#include <vector>

#include "MRColor.h"
#include "MRLabel.h"
#include "base/EventDispatcher.h"
#include "base/TouchEvent.h"
#include "base/UIWidget.h"

namespace morrow {

class MRTextEdit : public UIWidget {
public:
    struct Events {
        Observable<MRTextEdit&, const std::wstring&> onTextChanged;
        Observable<MRTextEdit&, const std::wstring&> onSubmitted;
        Observable<MRTextEdit&, const std::wstring&> onCopyRequested;
    };

    /// 创建一个支持多行输入的文本编辑框。
    static std::shared_ptr<MRTextEdit> create();

    /// 设置编辑框中的原始文本。
    void setText(const std::wstring& text);

    /// 获取编辑框中的原始文本。
    const std::wstring& getText() const {
        return m_text;
    }

    /// 设置无内容且未聚焦时显示的占位提示文字。
    void setPlaceholder(const std::wstring& placeholder);

    /// 获取当前占位提示文字。
    const std::wstring& getPlaceholder() const {
        return m_placeholder;
    }

    /// 设置文本渲染使用的字体名称。
    void setFontName(const std::string& fontName);

    /// 设置输入文本和占位文字的字号。
    void setFontSize(float fontSize);

    void setAutoWrap(bool enabled);

    /// 设置正常输入文本的颜色。
    void setTextColor(const Vector4& color);

    /// 设置占位提示文字的颜色。
    void setPlaceholderColor(const Vector4& color);

    /// 设置编辑框背景颜色。
    void setBackgroundColor(const Vector4& color);

    /// 设置编辑框聚焦时的背景颜色。
    void setFocusedBackgroundColor(const Vector4& color);

    /// 设置最大字符数；传入 0 表示不限制。
    void setMaxLength(size_t maxLength);

    /// 获取最大字符数限制；0 表示不限制。
    size_t getMaxLength() const {
        return m_maxLength;
    }

    /// 设置是否只读；只读状态仍可聚焦，但不会修改文本。
    void setReadOnly(bool readOnly);

    /// 获取当前是否为只读状态。
    bool isReadOnly() const {
        return m_readOnly;
    }

    Events& events();

    /// 设置光标位置，位置会自动限制在文本长度范围内。
    void setCursorPosition(size_t position);

    /// 获取当前光标位置。
    size_t getCursorPosition() const {
        return m_cursorPosition;
    }

    /// 返回当前是否获得键盘焦点。
    bool hasFocus() const {
        return m_focused;
    }

    void selectAll();

    void clearSelection();

    bool hasSelection() const;

    std::wstring getSelectedText() const;

    /// 由输入系统通知键盘焦点变化，并同步背景与光标显示。
    void onFocusChanged(bool focused) override;

protected:
    explicit MRTextEdit(bool multiline);

    void initializeChildren();

    virtual std::wstring buildDisplayText() const;

    virtual void handleEnter();

    virtual bool acceptsCharacter(wchar_t character) const;

    void refreshVisuals();

    void notifyTextChanged();

    void notifySubmitted();

private:
    void handleCharacter(uint32_t codepoint);

    void handleKeyDown(const TouchEvent& event);

    void handlePointerDown(const TouchEvent& event);

    void handlePointerMove(const TouchEvent& event);

    void handlePointerRelease(const TouchEvent& event);

    void insertCharacter(wchar_t character);

    void eraseBeforeCursor();

    void eraseAtCursor();

    void eraseSelection();

    size_t textPositionAt(float screenX, float screenY) const;

    void layoutChildren();

    void updateCursorVisual();

    void updateSelectionVisuals();

protected:
    MRColorSharedPtr m_background;
    std::vector<MRColorSharedPtr> m_selectionRects;
    std::shared_ptr<MRLabel> m_label;
    MRColorSharedPtr m_cursor;
    std::wstring m_text;
    std::wstring m_placeholder;
    std::string m_fontName = "default";
    Vector4 m_textColor = Vector4(0.08f, 0.1f, 0.14f, 1.0f);
    Vector4 m_placeholderColor = Vector4(0.48f, 0.52f, 0.58f, 1.0f);
    Vector4 m_backgroundColor = Vector4(0.94f, 0.95f, 0.97f, 1.0f);
    Vector4 m_focusedBackgroundColor = Vector4(0.86f, 0.92f, 0.98f, 1.0f);
    Events m_events;
    EventConnection m_characterConnection;
    EventConnection m_keyDownConnection;
    EventConnection m_pointerDownConnection;
    EventConnection m_pointerMoveConnection;
    EventConnection m_pointerReleaseConnection;
    size_t m_cursorPosition = 0;
    size_t m_selectionAnchor = 0;
    size_t m_selectionPosition = 0;
    size_t m_maxLength = 0;
    float m_fontSize = 22.0f;
    bool m_multiline = true;
    bool m_readOnly = false;
    bool m_focused = false;
    bool m_selectingWithPointer = false;
};

using MRTextEditSharedPtr = std::shared_ptr<MRTextEdit>;

}  // namespace morrow

#endif  // MORROW_GUI_MRTEXTEDIT_H
