//
// Created by 0060328 on 25-10-9.
//

#ifndef DYNAMICFONT_H
#define DYNAMICFONT_H


#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

// 使用stb_truetype作为TrueType解析器
// #define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "FontTexture.h"
#include "debug/ObjectRegistry.h"

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

class DynamicFont {
public:
    // SDF 图集统一按参考字号光栅化一次，任意目标字号通过缩放渲染
    // （d/AA 带宽在 shader 内乘 fontSize / REFERENCE_FONT_SIZE）。
    static constexpr float REFERENCE_FONT_SIZE = 44.0f;

    DynamicFont();

    ~DynamicFont();

    // 从文件加载字体资源。字号在 GetGlyph/GetMetrics 时按需指定（仅影响缩放）。
    bool LoadFromFile(const std::string& filename);

    // 从内存加载字体
    bool LoadFromMemory(const unsigned char* data, size_t size);

    // 获取字符字形信息（如果不存在则动态生成）。
    // 返回值已按目标字号缩放（纹理坐标除外——始终指向图集原始单元格）。
    FontGlyph GetGlyph(int32_t codepoint, float fontSize);

    // 获取字体度量（已按目标字号缩放）
    FontMetrics GetMetrics(float fontSize);

    // 获取字体纹理图集
    std::shared_ptr<FontTexture> GetTextureAtlas() const;

    // 获取当前字体图集版本。图集扩容/重建后版本会递增。
    uint64_t GetTextureAtlasVersion() const;

    // 计算文本宽度
    float CalculateTextWidth(const std::wstring& text, float fontSize);

    // 设置抗锯齿质量 (0-3, 0=无抗锯齿, 3=高质量)
    void SetAntialiasingQuality(int32_t quality);

    // 设置字符间距
    void SetCharacterSpacing(float spacing);

    float GetCharacterSpacing() const;

    // 设置行间距
    void SetLineSpacing(float spacing);

    /// 将当前延迟的上传一次性提交到 GPU（整串/整批只提交一次时调用）
    void FlushPendingUploads();

    /// 确保字符串所需字形已生成并一次性提交纹理更新；渲染前调用可实现「整串提交一次」
    void EnsureStringGlyphs(const std::wstring& text, float fontSize);

private:
    struct GlyphCache {
        float scale = 1.0f;  // 参考字号的 em->px 缩放
        FontMetrics metrics;
        std::unordered_map<int32_t, FontGlyph> glyphs;
    };

    static float GetScaleForFontSize(float fontSize);

    GlyphCache& GetGlyphCache();

    bool InitializeFont();
    // 初始化参考字号的字体度量
    void InitializeMetrics(GlyphCache& cache);

    // 生成字符到纹理图集
    bool GenerateGlyphToAtlas(int32_t codepoint, GlyphCache& cache);

    // 扩展纹理图集
    bool ExpandTextureAtlas();

    // 创建新的纹理图集
    bool CreateTextureAtlas(int32_t width, int32_t height);

    // 将位图数据上传到纹理（内部用：现为延迟到 FlushPendingUploads）
    void UploadBitmapToTexture(int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* bitmap);

private:
    struct PendingUpload {
        int32_t x = 0, y = 0, width = 0, height = 0;
        std::vector<unsigned char> bitmap;
    };
    std::vector<PendingUpload> m_pendingUploads;

private:
    DebugObjectHandle m_debugObject{DebugObjectCategory::Font, "DynamicFont"};
    stbtt_fontinfo m_fontInfo;
    std::vector<unsigned char> m_fontData;
    // 纹理图集管理
    std::shared_ptr<FontTexture> m_textureAtlas;
    uint64_t m_textureAtlasVersion = 0;
    int32_t m_atlasWidth = 1024;
    int32_t m_atlasHeight = 1024;
    int32_t m_currentX = 1; // 留出1像素边界
    int32_t m_currentY = 1;
    int32_t m_currentRowHeight = 0;

    // 字符缓存（单一参考字号）
    GlyphCache m_glyphCache;

    // 渲染设置
    int32_t m_aaQuality = 1; // 抗锯齿质量（SDF 光栅化下不再使用，保留 API 兼容）
    float m_charSpacing = 0.0f; // 字符间距
    float m_lineSpacing = 0.0f; // 行间距

    // ── SDF 图集参数（必须与 font.frag / font_shadow.frag 内的常量一致）──
    // d_px = (sample - SDF_EDGE_NORM) * SDF_PX_PER_UNIT
    // 场覆盖字形框 ± SDF_SPREAD px（外 128/16 = 8px，内 127/16 ≈ 7.9px），
    // 匹配通用 UI 推荐配置（参考字号 44px + spread 8px）。
    static constexpr int SDF_SPREAD = 8;
    static constexpr unsigned char SDF_ONEDGE = 128;
    static constexpr float SDF_PIXEL_DIST_SCALE = 16.0f;
    static constexpr float SDF_EDGE_NORM = 128.0f / 255.0f;
    static constexpr float SDF_PX_PER_UNIT = 255.0f / 16.0f;

    static const int32_t ATLAS_PADDING = 1; // 字符间填充
};

using DynamicFontSharedPtr = std::shared_ptr<DynamicFont>;
} // morrow

#endif //DYNAMICFONT_H
