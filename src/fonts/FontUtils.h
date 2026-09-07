//
// Created by lance on 2026/9/7.
//

#ifndef MORROW_GUI_FONTUTILS_H
#define MORROW_GUI_FONTUTILS_H
#include <vector>
namespace morrow {
namespace FontUtils {
// ── 自造 SDF：coverage 位图 + 精确欧氏距离变换（Felzenszwalb 1D EDT）──
// stb_truetype v1.26 的 stbtt_GetGlyphSDF 不处理三次贝塞尔（STBTT_vcubic）：
// stbtt__compute_crossings_x 与求距循环只覆盖 vline/vcurve，因此 CFF/OTF
// 字体（MorrowSansCN 等，CJK 字形以三次曲线为主）的绕数判定错乱、产生
// 碎片伪影；TrueType（.ttf/.ttc，二次曲线）则正常（msyh/simhei 实测通过，
// 见 tools/sdf_compare.cpp）。另注意 stb 的 SDF 内容绘制在"字形框外扩
// padding"处，与 MakeGlyphBitmap 路径（内容在字形框处）相差一个 padding，
// 消费方需修正原点（DynamicFont 的 GlyphSdf 分支已处理）。
// 本路径对任意字体稳定，边缘定位误差 ≤ 0.5px（二值化阈值 128 + 亚像素
// 校正），作为默认 SDF 生成方式；矢量亚像素精确的 GlyphSdf 方式作为
// 可选项，由 DynamicFont::LoadFromFile 的 method 参数选择。

constexpr int kEdtInf = 0x100000;

// Felzenszwalb-Huttenlocher 一维平方距离变换（lower envelope）
void edt1d(const std::vector<int>& f, int n, std::vector<int>& d);

// grid: 每格为"到目标集合的平方距离"；目标集合的格初值为 0，其余求精确 EDT。
void edt2d(std::vector<int>& grid, int width, int height);

// coverage 位图 → SDF 字节图（0..255，onedge 值编码 0 距离）
void encodeSdfFromCoverage(const unsigned char* coverage, int width, int height, int onedge, float pixelDistScale, std::vector<unsigned char>& out);
}  // namespace FontUtils
}  // namespace morrow
#endif  // MORROW_GUI_FONTUTILS_H
