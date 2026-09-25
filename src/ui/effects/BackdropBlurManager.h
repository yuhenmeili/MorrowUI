//
// BackdropBlurManager — 共享 Kawase 背景模糊链的帧编排器
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5 / §6 S2）。
//
// 帧结构由 BatchManager::renderBatches 驱动：
//
//   阶段 3a（dirty 时）
//     bindRenderTarget(RT_backdrop) + clear
//     BatchManager 绘制 underlay 批次 + displayLayer < boundary 的批次
//   阶段 3b（dirty 时，逐级至本帧最大所需层级）
//     downsample → Li，kawase(offset = i + 0.5) → Li'（ping-pong）
//   阶段 3c（每渲染帧）
//     回屏背景恢复拷贝 → 各层级模糊面片合并绘制（每层级 1 个 draw，≤ 3）
//     BatchManager 绘制 displayLayer ≥ boundary 的批次（面板内容等）
//
// 分级（§5.2）：半径量化到 L0'/L1'/L2' 三个采样层级（1/2、1/4、1/8 分辨率，
// LowCost 档为 1/4、1/8、1/16），层内不再连续可调。
//
// 缓存（§5.5 方案 B）：backdrop 段内容未变（批次结构等价 + 全部 Transform /
// Material / Mesh 版本号聚合签名一致）时跳过 3a/3b，仅重绘 3c。静止场景
// 叠加按需渲染门控 = 整帧零成本。
//
// 模糊面片不走 BatchManager 的通用收集通道（相邻合批无法把分散在各面板
// 之前的面片合成一批），而是由 BackdropBlur 组件每帧 submitQuad 提交、
// 这里按层级在 CPU 侧合并成自管 VBO 一次绘制 —— 每层级恒为 1 个 draw
// call，且不依赖 SSBO 能力（无非 SSBO 回退分叉）。
//
// 全局开关 setEnabled(false)：模糊面片降级为 tint 纯色面板（由组件走普通
// 通道注册），本管理器不再分段帧结构，模糊子系统零成本（§5.7）。
//

#ifndef MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H
#define MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H

#include <array>
#include <memory>
#include <vector>

#include "BatchDataDefine.h"
#include "Material.h"
#include "Matrix4.h"
#include "OffscreenRenderTarget.h"
#include "Vector2.h"
#include "Vector4.h"
#include "utils/Singleton.h"

namespace morrow {
struct FrameState;
using FrameStateSharedPtr = std::shared_ptr<FrameState>;

/// 启动 / 运行档位（§5.7 三级开关的档位层）。
enum class BackdropBlurQuality : uint8_t {
    Off = 0,      ///< 全程退化路径，RT 链不分配（等价 setEnabled(false)）
    Standard = 1, ///< 1/2 基准链（L0/L1/L2 = 1/2、1/4、1/8 分辨率）
    LowCost = 2,  ///< 1/4 基准链（L0/L1/L2 = 1/4、1/8、1/16 分辨率）
};

class BackdropBlurManager : public Singleton<BackdropBlurManager> {
    friend class Singleton<BackdropBlurManager>;

public:
    /// 全局运行时开关（§5.7）：关闭后模糊面片降级为 tint 面板、帧结构退化为
    /// 现有单段路径；组件不销毁，重开即恢复。翻转只改标志位，下一帧生效。
    /// RT 链短暂保留以便快速重开；重开时缓存签名失效，强制重渲一次 backdrop。
    void setEnabled(bool enabled);

    bool isEnabled() const;

    /// 切换启动 / 运行档位。Off 会连带关闭开关（仅由档位引入的关闭会在切回
    /// Standard/LowCost 时自动恢复）；Standard ↔ LowCost 重建 RT 链（懒分配，
    /// 释放命令经渲染命令流按序执行，等价 fence 安全回收）。
    void setQuality(BackdropBlurQuality quality);

    BackdropBlurQuality getQuality() const;

    /// BackdropBlur 组件在 update 阶段提交一个模糊面片（世界矩阵与尺寸取自
    /// 属主 Transform；displayLayer 取自属主 Widget，用于推导分段边界）。
    /// levelF 为连续采样层级（0..kMaxLevel）：整数部分取该层级纹理，小数
    /// 部分在相邻层级间双层混合插值（S3 半径动画）。clipRect 取自提交时的
    /// frameState->currentClip（滚动容器等裁剪，S3 专项）。
    void submitQuad(const Matrix4& worldMatrix, const Vector2& size, float rounding, const Vector4& tint, float levelF, int32_t displayLayer,
                    const ClipRect& clipRect);

    /// 本帧是否存在活跃模糊（enabled 且已提交面片）。
    bool isFrameActive() const;

    /// 分段边界：本帧全部模糊面片属主的最小 displayLayer。严格小于边界的
    /// 批次画进 backdrop 段，其余画在回屏合成之后。
    [[nodiscard]] int32_t getBoundaryLayer() const;

    /// 半径 → 采样层级（§5.2 映射表）：0 < r ≤ 12 → L0，12 < r ≤ 40 → L1，
    /// r > 40 → L2。层内半径不再连续可调（分级量化是刻意取舍；半径动画用
    /// 组件的 setBlurLevel 双层插值表达）。
    static uint8_t quantizeRadius(float blurRadius);

    /// S2 脏标记（§5.5 方案 B）：对 backdrop 段（underlay + 边界以下）的全部
    /// 批次做"结构等价 + 版本号聚合"签名，与上一渲染帧比对。clean 时本帧
    /// 跳过阶段 3a/3b，RT 内容沿用上一帧。必须在 beginBackdropPass 之前、
    /// 批次构建之后调用（每分段帧一次）。
    bool computeBackdropDirty(const std::vector<RenderBatch>& batches, const FrameStateSharedPtr& frameState);

    /// 阶段 3a 开始：本帧有活跃模糊时返回 true（分段继续）；dirty 时额外
    /// 绑定 backdrop RT 并按窗口清屏色清屏，clean 时不触碰任何 GPU 状态。
    bool beginBackdropPass(const FrameStateSharedPtr& frameState);

    /// 阶段 3b + 3c：dirty 时跑模糊链（逐级 downsample + kawase 至本帧最大
    /// 所需层级）；回屏背景恢复与各层级面片绘制每渲染帧执行。
    void renderBackdropChain(const FrameStateSharedPtr& frameState);

    /// 帧收尾：清空本场面片提交，重置分段边界与最大所需层级。
    void endFrame();

    /// 释放全部 GPU 资源（GlobalObject::destroy 时调用，渲染线程仍存活）。
    /// 静态单例析构时渲染设备已销毁，故析构函数不再触碰 GPU 资源。
    void destroy();

private:
    BackdropBlurManager() = default;

    ~BackdropBlurManager() = default;

    struct BlurQuad {
        Matrix4 worldMatrix;
        Vector2 size;
        float rounding = 0.0f;
        Vector4 tint;
        float level = 0.0f; // 连续采样层级（整数部分 = 层级，小数 = 双层插值因子）
        int32_t displayLayer = 0;
        ClipRect clipRect;
    };

    /// 链上一个层级：降采样输入 RT + kawase 输出 RT（后者即该层级采样层 Li'）。
    struct ChainLevel {
        std::unique_ptr<OffscreenRenderTarget> downsampledRT;
        std::unique_ptr<OffscreenRenderTarget> blurredRT;
    };

    /// 懒创建 / 随窗口尺寸与档位重建 RT 链与 pass 材质（约 7.5MB @1080p）。
    void ensureResources(int32_t framebufferWidth, int32_t framebufferHeight);

    /// 释放 RT 链与面片 VBO（档位切换 / destroy 用；材质与全屏面片保留）。
    void releaseChainResources();

    /// 构建链 pass 共用的全屏裁剪空间面片（-1..1，Position + UV 平面布局）。
    void buildPassQuad();

    /// 执行一次全屏 pass：绑定 dst RT，以 texel 为源分辨率倒数采样 src 绘制。
    void drawChainPass(const FrameStateSharedPtr& frameState, OffscreenRenderTarget& dst, HwTexture2D src, Material& material,
                       float kawaseOffset, int32_t texelW, int32_t texelH);

    /// 回屏背景恢复：backdrop RT → 默认帧缓冲的全屏拷贝（blend 关闭）。
    void renderComposite(const FrameStateSharedPtr& frameState);

    /// 按分组烘焙模糊面片并绘制（S3）：分组键 = (低层级, 高层级, clipRect)——
    /// 整数 levelF 并入单层组（每层级 1 draw）；带小数的进双层插值组（相邻
    /// 层级对 1 draw）；不同 clipRect（滚动容器等）独立分组并施加 scissor。
    void renderBlurQuads(const FrameStateSharedPtr& frameState);

    /// 把一组面片烘焙成平面布局 VBOData（S1 单层级路径的同款顶点格式）。
    static VBODataSharedPtr buildQuadVBOData(const std::vector<const BlurQuad*>& quads, const FrameStateSharedPtr& frameState);

    static constexpr int32_t kNoBoundary = 11;     // displayLayer 域为 [-10,10]，11 = 本帧尚无提交
    static constexpr uint8_t kLevelCount = 3;      // L0 / L1 / L2
    static constexpr uint8_t kMaxLevel = kLevelCount - 1;
    static constexpr float kLevel0MaxRadius = 12.0f; // (0, 12] → L0
    static constexpr float kLevel1MaxRadius = 40.0f; // (12, 40] → L1

    bool m_enabled = true;
    bool m_disabledByQuality = false; // Off 档引入的关闭，切回时自动恢复
    BackdropBlurQuality m_quality = BackdropBlurQuality::Standard;
    std::vector<BlurQuad> m_quads;
    int32_t m_boundaryLayer = kNoBoundary;
    uint8_t m_maxNeededLevel = 0;

    // ---- S2 缓存（§5.5）----
    bool m_backdropDirty = true;     // computeBackdropDirty 的本帧结果
    uint64_t m_backdropSignature = 0; // 上一渲染帧签名；0 = 强制脏（首帧/切换/重开）

    // ---- RT 链 ----
    int32_t m_framebufferWidth = 0;
    int32_t m_framebufferHeight = 0;
    std::unique_ptr<OffscreenRenderTarget> m_backdropRT; // backdrop 段渲染目标（基准分辨率）
    std::vector<ChainLevel> m_levels;                    // kLevelCount 层

    MaterialSharedPtr m_downsampleMaterial;
    MaterialSharedPtr m_kawaseMaterial;
    MaterialSharedPtr m_compositeMaterial;
    MaterialSharedPtr m_blurQuadMaterial;

    HwVBO m_passQuadVBO{0};
    VBODataSharedPtr m_passQuadData;
    bool m_passQuadUploaded = false;
    std::array<HwVBO, kLevelCount> m_blurQuadVBOs{};
};
} // namespace morrow

#endif // MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H
