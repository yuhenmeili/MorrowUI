//
// BackdropBlur — 毛玻璃 / acrylic 背景模糊组件
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.4，组件式 opt-in，沿用 Shadow 的
// "复用属主几何 + 组件自持降级材质" 模式）。
//
// 启用（默认）：update 阶段把属主的世界矩阵 / 尺寸 / 圆角 / tint 提交给
// BackdropBlurManager，由共享模糊链统一渲染 —— 属主面板应使用透明背景
//（模糊面片即面板背景），普通子内容（文字 / 按钮）叠在其上。
//
// 关闭（setBlurRadius(0) 或 BackdropBlurManager::setEnabled(false)）：
// 组件降级为 tint 纯色半透明面板，复用属主 MeshFilter/Transform 走普通
// 通道注册（Shadow 同款），模糊子系统零成本。
//
// 语义（提案 §4 方案 A）：模糊源只包含分段边界以下的内容；模糊面板之间
// 不会互相出现在对方的模糊背景里。
//

#ifndef MORROW_UI_EFFECTS_BACKDROPBLUR_H
#define MORROW_UI_EFFECTS_BACKDROPBLUR_H

#include "base/Component.h"
#include "Material.h"
#include "Vector4.h"

namespace morrow {

class BackdropBlur : public Component {
public:
    BackdropBlur();

    void update(FrameStateSharedPtr frameState) override;

    /// 模糊半径（px）。S1 为单层级固定半径档：> 0 即启用共享链采样，
    /// 分级量化（L0/L1/L2）与半径动画见提案 S2/S3。
    void setBlurRadius(float radius);

    /// acrylic 混色：rgb 为面板底色，a 为混色强度（也是面片透明度）。
    void setTintColor(const Vector4& color);

    void setTintColor(float r, float g, float b, float a);

    /// 显式指定模糊面片圆角（px），此后不再跟随属主。
    void setRounding(float rounding);

    /// 恢复圆角跟随属主材质的 rounding 参数（默认行为）。
    void setRoundingFollowOwner(bool follow);

private:
    float m_blurRadius = 24.0f;
    Vector4 m_tintColor = Vector4(1.0f, 1.0f, 1.0f, 0.45f);
    float m_rounding = 0.0f;
    bool m_roundingFollowOwner = true;
    MaterialSharedPtr m_tintMaterial; // 关闭模糊时的降级面板材质
};

} // namespace morrow

#endif // MORROW_UI_EFFECTS_BACKDROPBLUR_H
