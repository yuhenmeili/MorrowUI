//
// 文本渲染的数据模型：字形、字体度量与 SDF 生成方式。
// 独立于 DynamicFont（无 stb_truetype 依赖），供公共文本控件使用。
//

#ifndef MORROW_FONTS_FONTGLYPH_H_
#define MORROW_FONTS_FONTGLYPH_H_

#include <cstdint>

namespace morrow {
// 字符字形信息
struct FontGlyph {
    int32_t codepoint; // Unicode码点
    float advance; // 水平步进
    float bearingX; // 水平偏移
    float bearingY; // 垂直偏移
    float width, height; // 字形尺寸

    // 纹理坐标
    float texCoordX, texCoordY; // 左下角
    float texCoordWidth, texCoordHeight; // 宽高

    bool generated = false; // 是否已生成纹理
};

// 字体度量信息
struct FontMetrics {
    float ascent; // 上升高度
    float descent; // 下降高度
    float lineGap; // 行间距
    float lineHeight; // 行高
};

// SDF 生成方式
enum class SdfMethod {
    // coverage 位图 + EDT 距离变换（默认）。对任意字体稳定；细笔画边缘
    // 有 ±0.5px 级别的重建误差（二值化 + 一维亚像素校正的近似性）。
    BitmapEdt,
    // stbtt 矢量轮廓逐像素求距（stbtt_GetGlyphSDF）。边缘亚像素精确；
    // 绕数判定使用整型截断坐标，部分密集曲线的 CJK 字体可能产生碎片
    // 伪影（实测 MorrowSansCN / SimHei 有，Arial 无），且生成更耗时。
    GlyphSdf,
};
} // namespace morrow

#endif //MORROW_FONTS_FONTGLYPH_H_
