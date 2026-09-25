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

    /// 模糊半径（px）。经共享链分级量化（§5.2）：≤12 → L0'（轻）、
    /// ≤40 → L1'（中，面板常用）、>40 → L2'（重）；0 = 关闭本面板模糊。
    /// 层内半径不再连续可调。
    void setBlurRadius(float radius);

    /// 连续采样层级（0..2，S3 半径动画）：整数部分取该层级，小数部分在相邻
    /// 层级间双层混合插值——层级固定、链不变，只有采样混合因子动画（§5.2
    /// "层级固定 + 过渡"的规范用法）。设置后覆盖 setBlurRadius 的量化结果，
    /// 再次调用 setBlurRadius 恢复量化行为。
    void setBlurLevel(float level);

    /// acrylic 混色：rgb 为面板底色，a 为混色强度（也是面片透明度）。
    void setTintColor(const Vector4& color);

    void setTintColor(float r, float g, float b, float a);

    /// 显式指定模糊面片圆角（px），此后不再跟随属主。
    void setRounding(float rounding);

    /// 恢复圆角跟随属主材质的 rounding 参数（默认行为）。
    void setRoundingFollowOwner(bool follow);

private:
    float m_blurRadius = 24.0f;
    float m_blurLevel = 1.0f;   // 连续层级（setBlurLevel 设置后生效）
    bool m_explicitLevel = false; // true = 用 m_blurLevel，false = 量化 m_blurRadius
    Vector4 m_tintColor = Vector4(1.0f, 1.0f, 1.0f, 0.45f);
    float m_rounding = 0.0f;
    bool m_roundingFollowOwner = true;
    MaterialSharedPtr m_tintMaterial; // 关闭模糊时的降级面板材质
};

} // namespace morrow

#endif // MORROW_UI_EFFECTS_BACKDROPBLUR_H
