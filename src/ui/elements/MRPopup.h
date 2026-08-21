#ifndef MORROW_GUI_MRPOPUP_H
#define MORROW_GUI_MRPOPUP_H

#include <memory>

#include "MRColor.h"
#include "base/UIWidget.h"

namespace morrow {

class BaseButton;

/// 通用弹出层：提供显示、隐藏、定位和父节点挂载能力。
class MRPopup : public UIWidget {
public:
    struct PopupEvents {
        Observable<MRPopup&> onClosed;
    };

    /// 创建一个基础弹出层。
    static std::shared_ptr<MRPopup> create();

    /// 将弹出层挂载到指定根节点。
    void attachTo(const std::shared_ptr<Widget>& root);
    /// 在根节点坐标中显示弹出层。
    virtual void popup(float x, float y);
    /// 隐藏弹出层。
    virtual void hide();
    /// 返回弹出层是否正在显示。
    bool isOpen() const;

    PopupEvents& popupEvents();

protected:
    explicit MRPopup(const char* widgetType);

    void initializePopup();

    void closeInternal();

    MRColorSharedPtr m_background;
    PopupEvents m_popupEvents;
    bool m_open = false;
};

using MRPopupSharedPtr = std::shared_ptr<MRPopup>;

/// 带标题、内容区域和内边距的面板弹窗。
class MRPopupPanel : public MRPopup {
public:
    /// 创建一个面板弹窗。
    static std::shared_ptr<MRPopupPanel> create();

    /// 设置面板标题。
    void setTitle(const std::wstring& title);
    /// 设置面板内容节点，节点会自动重新挂载到内容区域。
    void setContent(const std::shared_ptr<Widget>& content);
    /// 设置面板尺寸。
    void setPanelSize(float width, float height);

protected:
    MRPopupPanel();

    void layoutPanel();

    std::shared_ptr<class MRLabel> m_title;
    std::shared_ptr<Widget> m_content;
    float m_panelWidth = 460.0f;
    float m_panelHeight = 260.0f;
};

using MRPopupPanelSharedPtr = std::shared_ptr<MRPopupPanel>;

/// 可关闭的窗口弹窗。
class MRWindow : public MRPopupPanel {
public:
    /// 创建一个带关闭按钮的窗口弹窗。
    static std::shared_ptr<MRWindow> create();

    /// 设置窗口关闭按钮是否可见。
    void setCloseButtonVisible(bool visible);
    /// 设置窗口关闭按钮文字。
    void setCloseButtonText(const std::wstring& text);

protected:
    MRWindow();
    std::shared_ptr<class MRButton> m_closeButton;
    Observable<BaseButton&>::Connection m_closeConnection;
};

using MRWindowSharedPtr = std::shared_ptr<MRWindow>;

/// 带确认和取消操作的对话框。
class MRDialog : public MRWindow {
public:
    struct DialogEvents {
        Observable<MRDialog&> onConfirmed;
        Observable<MRDialog&> onCanceled;
    };

    /// 创建一个带确认和取消操作的对话框。
    static std::shared_ptr<MRDialog> create();

    /// 设置对话框正文。
    void setMessage(const std::wstring& message);
    /// 设置确认按钮文字。
    void setConfirmText(const std::wstring& text);
    /// 设置取消按钮文字。
    void setCancelText(const std::wstring& text);

    DialogEvents& dialogEvents();

protected:
    MRDialog();

private:
    std::shared_ptr<class MRLabel> m_message;
    std::shared_ptr<class MRButton> m_confirmButton;
    std::shared_ptr<class MRButton> m_cancelButton;
    Observable<BaseButton&>::Connection m_confirmConnection;
    Observable<BaseButton&>::Connection m_cancelConnection;
    DialogEvents m_dialogEvents;
};

using MRDialogSharedPtr = std::shared_ptr<MRDialog>;

/// 跟随目标区域显示的提示气泡。
class MRTooltip : public MRPopupPanel {
public:
    /// 创建一个提示气泡。
    static std::shared_ptr<MRTooltip> create();

    /// 设置提示文本。
    void setText(const std::wstring& text);
    /// 在目标屏幕区域下方显示提示。
    void showFor(const Math::Rect& targetBounds);
    /// 设置提示偏移。
    void setOffset(float x, float y);

protected:
    MRTooltip();

private:
    Vector2 m_offset{0.0f, 8.0f};
};

using MRTooltipSharedPtr = std::shared_ptr<MRTooltip>;
using Tooltip = MRTooltip;
using Popup = MRPopup;
using PopupPanel = MRPopupPanel;
using Dialog = MRDialog;

}  // namespace morrow

#endif  // MORROW_GUI_MRPOPUP_H
