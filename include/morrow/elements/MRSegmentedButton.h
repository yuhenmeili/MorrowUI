//
// MRSegmentedButton — 分段选择控件（仪表设置页常用的 N 选一按钮组）。
//
// 一行 N 个等宽文本段，单选；选中段显示点状背景图（程序生成、按段尺寸
// 生成并缓存）并将文字切换为高亮色，未选中段为普通灰白文字。
//
// 纯容器（UIWidget(false)），自身不绘制背景；底色/毛玻璃由外层容器负责：
// 毛玻璃用透明 MRColor 作属主挂 BackdropBlur、本组件叠其上（见
// samples/ControlsDemo.cpp 与 samples/BackblurDemo.cpp）。
//

#ifndef MORROW_ELEMENTS_MRSEGMENTEDBUTTON_H
#define MORROW_ELEMENTS_MRSEGMENTEDBUTTON_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "morrow/Texture.h"
#include "morrow/base/UIWidget.h"
#include "morrow/elements/MRButton.h"

namespace morrow {
class MRImage;

class MRSegmentedButton : public UIWidget {
public:
    static std::shared_ptr<MRSegmentedButton> create();

    /// 追加一个分段（文本）。布局使用当前 Transform 尺寸，请先 setSize 再 add。
    void addSegment(const std::wstring& text);

    void setSelectedIndex(int index);

    int getSelectedIndex() const;

    /// 选中段文字颜色（默认仪表绿）
    void setSelectedColor(const Vector4& color);

    /// 未选中段文字颜色
    void setNormalColor(const Vector4& color);

    /// 选中背景图距段边缘的内缩量（横向 / 纵向，像素；背景图略小于按钮）
    void setDotInset(float insetX, float insetY);

    /// 外部指定选中背景纹理（设计师定制图，拉伸铺满内缩后的段区域）；
    /// 传 nullptr 恢复默认的程序生成点状纹理
    void setSelectedTexture(const TextureSharedPtr& texture);

    /// 便捷重载：从图片路径创建选中背景纹理
    void setSelectedTextureImage(const std::string& imageUrl);

    void setOnSelectionChanged(std::function<void(int index)> callback);

private:
    MRSegmentedButton();

    struct Segment {
        std::shared_ptr<MRImage> dotOverlay; // 选中时的背景图（点状纹理或外部定制图）
        std::shared_ptr<MRButton> button; // 段按钮（透明背景，仅文字）
        Observable<BaseButton&>::Connection clickConnection; // 段按钮点击 → setSelectedIndex
    };

    void layoutSegments();

    void applySelection();

    std::vector<Segment> m_segments;
    int m_selectedIndex = -1;
    Vector4 m_selectedColor = Vector4(0.66f, 0.78f, 0.22f, 1.0f);
    Vector4 m_normalColor = Vector4(0.82f, 0.84f, 0.86f, 1.0f);
    TextureSharedPtr m_selectedTexture; // 外部定制选中背景；空 = 程序生成点状纹理
    float m_dotInsetX = 6.0f;
    float m_dotInsetY = 6.0f;
    std::function<void(int index)> m_selectionChanged;
};

using MRSegmentedButtonSharedPtr = std::shared_ptr<MRSegmentedButton>;
} // namespace morrow

#endif // MORROW_ELEMENTS_MRSEGMENTEDBUTTON_H
