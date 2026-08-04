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
    DynamicFont();

    ~DynamicFont();

    // 从文件加载字体
    bool LoadFromFile(const std::string& filename, float pixelHeight);

    // 从内存加载字体
    bool LoadFromMemory(const unsigned char* data, size_t size, float pixelHeight);

    // 获取字符字形信息（如果不存在则动态生成）
    const FontGlyph* GetGlyph(int32_t codepoint);

    // 获取字体度量
    const FontMetrics& GetMetrics() const;

    // 获取字体纹理图集
    std::shared_ptr<FontTexture> GetTextureAtlas() const;

    // 获取当前字体图集版本。图集扩容/重建后版本会递增。
    uint64_t GetTextureAtlasVersion() const;

    // 计算文本宽度
    float CalculateTextWidth(const std::wstring& text);

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
    void EnsureStringGlyphs(const std::wstring& text);

private:
    bool InitializeFont();
    // 初始化字体度量
    void InitializeMetrics();

    // 生成字符到纹理图集
    bool GenerateGlyphToAtlas(int32_t codepoint);

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
    float m_pixelHeight = 16.0f;
    float m_scale = 1.0f;

    // 纹理图集管理
    std::shared_ptr<FontTexture> m_textureAtlas;
    uint64_t m_textureAtlasVersion = 0;
    int32_t m_atlasWidth = 512;
    int32_t m_atlasHeight = 512;
    int32_t m_currentX = 1; // 留出1像素边界
    int32_t m_currentY = 1;
    int32_t m_currentRowHeight = 0;

    // 字符缓存
    std::unordered_map<int32_t, FontGlyph> m_glyphCache;
    FontMetrics m_metrics;

    // 渲染设置
    int32_t m_aaQuality = 1; // 抗锯齿质量
    float m_charSpacing = 0.0f; // 字符间距
    float m_lineSpacing = 0.0f; // 行间距

    static const int32_t ATLAS_PADDING = 1; // 字符间填充
};

using DynamicFontSharedPtr = std::shared_ptr<DynamicFont>;
} // morrow

#endif //DYNAMICFONT_H
