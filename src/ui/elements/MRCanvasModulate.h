#ifndef MORROW_GUI_MRCANVASMODULATE_H
#define MORROW_GUI_MRCANVASMODULATE_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow {

class MRCanvasModulate : public UIWidget {
public:
    /// 创建一个通过乘法混合调制整幅 UI 画面的全局色调组件。
    static std::shared_ptr<MRCanvasModulate> create();

    /// 设置目标调制颜色；白色保持原画面，较暗颜色可用于夜间模式。
    void setModulateColor(const Vector4& color);

    /// 获取当前目标调制颜色。
    const Vector4& getModulateColor() const {
        return m_modulateColor;
    }

    /// 设置调制强度，0 表示不改变画面，1 表示完全使用目标调制颜色。
    void setStrength(float strength);

    /// 获取当前调制强度。
    float getStrength() const {
        return m_strength;
    }

    /// 设置是否自动铺满父节点；默认开启。
    void setAutoFitParent(bool enabled);

    /// 获取当前是否自动铺满父节点。
    bool isAutoFitParent() const {
        return m_autoFitParent;
    }

    /// 快速切换夜间模式；开启时使用预设冷色调，关闭时恢复原画面。
    void setNightMode(bool enabled);

    /// 获取当前夜间模式状态。
    bool isNightMode() const {
        return m_nightMode;
    }

    /// 每帧在自动适配开启时同步父节点尺寸，并提交全屏调制绘制。
    void update(FrameStateSharedPtr frameState) override;

private:
    MRCanvasModulate();

    void applyModulation();

    Vector4 m_modulateColor = Vector4(0.45f, 0.55f, 0.72f, 1.0f);
    float m_strength = 0.0f;
    bool m_autoFitParent = true;
    bool m_nightMode = false;
};

using MRCanvasModulateSharedPtr = std::shared_ptr<MRCanvasModulate>;

}  // namespace morrow

#endif  // MORROW_GUI_MRCANVASMODULATE_H
