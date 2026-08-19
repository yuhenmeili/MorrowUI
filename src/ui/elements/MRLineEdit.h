#ifndef MORROW_GUI_MRLINEEDIT_H
#define MORROW_GUI_MRLINEEDIT_H

#include "MRTextEdit.h"

namespace morrow {

class MRLineEdit : public MRTextEdit {
public:
    /// 创建一个仅接受单行文本的输入框。
    static std::shared_ptr<MRLineEdit> create();

    /// 设置是否启用密码模式；启用后界面显示掩码，原始文本保持不变。
    void setPasswordMode(bool enabled);

    /// 获取当前是否启用了密码模式。
    bool isPasswordMode() const {
        return m_passwordMode;
    }

    /// 设置密码模式下用于显示的掩码字符。
    void setPasswordCharacter(wchar_t character);

protected:
    MRLineEdit();

    std::wstring buildDisplayText() const override;

    void handleEnter() override;
    
    bool acceptsCharacter(wchar_t character) const override;

private:
    bool m_passwordMode = false;
    wchar_t m_passwordCharacter = L'*';
};

using MRLineEditSharedPtr = std::shared_ptr<MRLineEdit>;

}  // namespace morrow

#endif  // MORROW_GUI_MRLINEEDIT_H
