//
// SafeStaticTextLayout.h — 安全静态文本排版器（HMI 仪表盘专用）
//
// 设计目标：
//   基于 StaticAtlasManager 中的预定义字符 sprite，将文本消息作为
//   sprite 名称序列进行排版渲染。所有字符 UV、advance 等排版数据
//   均为编译期常量或由外部注入，不依赖文件 IO 和运行时堆分配。
//
// 典型用途：
//   - 里程表数字（ODO 累加器）
//   - 强文字告警（"请系好安全带"、"制动系统故障"）
//   - 档位/模式指示（P/R/N/D、ECO/SPORT）
//
// 使用流程：
//   1. 通过 StaticAtlasManager 注册包含字符 sprite 的图集
//   2. SafeStaticTextLayout::create("atlasName")
//   3. setText({"char_请", "char_系", "char_安", "char_全", "char_带"})
//
// 与 SafeStaticSprite 的关系：
//   SafeStaticSprite: 单 sprite 切换（时速数字、档位字母）
//   SafeStaticTextLayout: 多 sprite 序列排版（完整告警词句）
//

#ifndef MORROW_SAFE_STATIC_TEXT_LAYOUT_H
#define MORROW_SAFE_STATIC_TEXT_LAYOUT_H

#include "base/UIWidget.h"
#include "StaticAtlasManager.h"
#include "Vector4.h"

#include <string>
#include <vector>
#include <memory>

namespace morrow {

// ---------------------------------------------------------------------------
// SafeStaticTextLayout
// ---------------------------------------------------------------------------
class SafeStaticTextLayout;
using SafeStaticTextLayoutSharedPtr = std::shared_ptr<SafeStaticTextLayout>;

class SafeStaticTextLayout : public UIWidget {
public:
    /// 工厂方法
    /// @param atlasName  字符图集名称（需事先注册到 StaticAtlasManager）
    static SafeStaticTextLayoutSharedPtr create(const std::string& atlasName);

    virtual ~SafeStaticTextLayout() = default;

    // -----------------------------------------------------------------------
    // 文本内容设置
    // -----------------------------------------------------------------------

    /// 设置要显示的文本（字符 sprite 名称序列）
    /// @param charSpriteNames  每个字符在 atlas 中的 sprite 名称
    void setText(const std::vector<std::string>& charSpriteNames);

    /// 设置要显示的文本（C 数组版本）
    void setText(const char* const* spriteNames, int count);

    /// 清空文本内容
    void clearText();

    /// 获取当前字符数量
    int getCharCount() const { return static_cast<int>(m_charSprites.size()); }

    // -----------------------------------------------------------------------
    // 外观设置
    // -----------------------------------------------------------------------

    /// 设置文本颜色（会与图集中 sprite 颜色混合）
    void setColor(float r, float g, float b, float a = 1.0f);

    /// 设置字符间距（默认 0，即紧密排列）
    void setCharSpacing(float spacing) { m_charSpacing = spacing; }

protected:
    explicit SafeStaticTextLayout(const std::string& atlasName);

    /// 初始化着色器（复用 font shader 模式）
    void initShader();

    /// 根据当前字符序列重建排版网格
    void buildTextMesh();

private:
    std::string m_atlasName;                  // 绑定的字符图集名称
    std::vector<std::string> m_charSprites;   // 当前显示的字符序列
    Math::Vector4 m_color;                    // 文本颜色
    float m_charSpacing = 0.0f;              // 字符间距
};

} // namespace morrow

#endif // MORROW_SAFE_STATIC_TEXT_LAYOUT_H
