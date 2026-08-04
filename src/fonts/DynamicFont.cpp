#define STB_TRUETYPE_IMPLEMENTATION
#include "DynamicFont.h"
#include <cmath>
#include <algorithm>

#include "Log.h"

namespace morrow {
DynamicFont::DynamicFont() {
    CreateTextureAtlas(m_atlasWidth, m_atlasHeight);
}

DynamicFont::~DynamicFont() {
}

bool DynamicFont::LoadFromFile(const std::string& filename, float pixelHeight) {
    m_debugObject.setName(filename);
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        LOG_E("fail to load font {}", filename);
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    m_fontData.resize(size);
    fread(m_fontData.data(), 1, size, file);
    fclose(file);

    m_pixelHeight = pixelHeight;
    return InitializeFont();
}

bool DynamicFont::LoadFromMemory(const unsigned char* data, size_t size, float pixelHeight) {
    m_fontData.assign(data, data + size);
    m_pixelHeight = pixelHeight;
    return InitializeFont();
}

const FontGlyph* DynamicFont::GetGlyph(int32_t codepoint) {
    auto it = m_glyphCache.find(codepoint);
    if (it != m_glyphCache.end()) {
        return &it->second;
    }
    // 生成新的字形
    if (GenerateGlyphToAtlas(codepoint)) {
        return &m_glyphCache[codepoint];
    }
    return nullptr;
}

const FontMetrics& DynamicFont::GetMetrics() const {
    return m_metrics;
}

std::shared_ptr<FontTexture> DynamicFont::GetTextureAtlas() const {
    return m_textureAtlas;
}

uint64_t DynamicFont::GetTextureAtlasVersion() const {
    return m_textureAtlasVersion;
}

float DynamicFont::CalculateTextWidth(const std::wstring& text) {
    float width = 0.0f;
    for (wchar_t c : text) {
        const FontGlyph* glyph = GetGlyph(static_cast<int32_t>(c));
        if (glyph) {
            width += glyph->advance + m_charSpacing;
        }
    }
    FlushPendingUploads();
    return width;
}

void DynamicFont::SetAntialiasingQuality(int32_t quality) {
    m_aaQuality = quality;
}

void DynamicFont::SetCharacterSpacing(float spacing) {
    m_charSpacing = spacing;
}

float DynamicFont::GetCharacterSpacing() const {
    return m_charSpacing; }

void DynamicFont::SetLineSpacing(float spacing) {
    m_lineSpacing = spacing;
}

bool DynamicFont::InitializeFont() {
    if (!stbtt_InitFont(&m_fontInfo, m_fontData.data(), 0)) {
        LOG_E("stbtt_InitFont fail");
        return false;
    }
    // 计算缩放比例
    m_scale = stbtt_ScaleForPixelHeight(&m_fontInfo, m_pixelHeight);
    // 初始化字体度量
    InitializeMetrics();
    // 预生成常用 ASCII 字形，整批生成后一次性上传
    const std::wstring commonChars = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?/\\\"'`~ ";
    for (const wchar_t c : commonChars) {
        GetGlyph(c);
    }
    FlushPendingUploads();
    return true;
}

void DynamicFont::InitializeMetrics() {
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&m_fontInfo, &ascent, &descent, &lineGap);

    m_metrics.ascent = ascent * m_scale;
    m_metrics.descent = descent * m_scale;
    m_metrics.lineGap = lineGap * m_scale;
    m_metrics.lineHeight = (ascent - descent + lineGap) * m_scale;
}

bool DynamicFont::GenerateGlyphToAtlas(int32_t codepoint) {
    FontGlyph glyph;
    glyph.codepoint = codepoint;
    int glyphIndex = stbtt_FindGlyphIndex(&m_fontInfo, codepoint);
    // 获取字形度量
    int advance, lsb, x0, y0, x1, y1;
    stbtt_GetGlyphHMetrics(&m_fontInfo, glyphIndex, &advance, &lsb);
    stbtt_GetGlyphBitmapBox(&m_fontInfo, glyphIndex, m_scale, m_scale, &x0, &y0, &x1, &y1);
    glyph.advance = advance * m_scale;
    glyph.bearingX = lsb * m_scale;
    glyph.bearingY = y0;
    glyph.width = x1 - x0;
    glyph.height = y1 - y0;
    if (glyph.width <= 0 || glyph.height <= 0) {
        // 空格等不可见字符
        glyph.generated = true;
        m_glyphCache[codepoint] = glyph;
        return true;
    }
    // 检查纹理图集是否有足够空间
    if (m_currentX + glyph.width + ATLAS_PADDING > m_atlasWidth) {
        m_currentX = ATLAS_PADDING;
        m_currentY += m_currentRowHeight + ATLAS_PADDING;
        m_currentRowHeight = 0;
    }

    if (m_currentY + glyph.height + ATLAS_PADDING > m_atlasHeight) {
        if (!ExpandTextureAtlas()) {
            return false;
        }
    }
    // 更新行高
    m_currentRowHeight = std::max(m_currentRowHeight, int(glyph.height));
    // 生成字形位图
    int bitmapWidth = glyph.width;
    int bitmapHeight = glyph.height;
    std::vector<unsigned char> bitmap(bitmapWidth * bitmapHeight);
    // 根据抗锯齿质量选择渲染模式
    if (m_aaQuality > 0) {
        stbtt_MakeGlyphBitmap(&m_fontInfo, bitmap.data(),
                              bitmapWidth, bitmapHeight,
                              bitmapWidth, m_scale, m_scale, glyphIndex);
    } else {
        // 无抗锯齿模式
        stbtt_MakeGlyphBitmapSubpixel(&m_fontInfo, bitmap.data(),
                                      bitmapWidth, bitmapHeight,
                                      bitmapWidth, m_scale, m_scale, 0, 0, glyphIndex);
    }
    // 延迟上传：加入待提交列表，由 FlushPendingUploads / EnsureStringGlyphs 整批提交
    m_pendingUploads.push_back({ m_currentX, m_currentY, bitmapWidth, bitmapHeight, bitmap });
    // 设置纹理坐标 (归一化)
    glyph.texCoordX = static_cast<float>(m_currentX) / m_atlasWidth;
    glyph.texCoordY = static_cast<float>(m_currentY) / m_atlasHeight;
    glyph.texCoordWidth = static_cast<float>(bitmapWidth) / m_atlasWidth;
    glyph.texCoordHeight = static_cast<float>(bitmapHeight) / m_atlasHeight;
    glyph.generated = true;
    m_glyphCache[codepoint] = glyph;
    // 更新当前位置
    m_currentX += glyph.width + ATLAS_PADDING;
    return true;
}

bool DynamicFont::ExpandTextureAtlas() {
    int32_t newWidth = m_atlasWidth * 2;
    int32_t newHeight = m_atlasHeight * 2;
    return CreateTextureAtlas(newWidth, newHeight);
}

bool DynamicFont::CreateTextureAtlas(int32_t width, int32_t height) {
    auto newAtlas = std::make_shared<FontTexture>(width, height);
    if (!newAtlas->Initialize()) {
        return false;
    }

    // 扩容会替换纹理对象，同时改变归一化 UV 的分母。
    // 新图集不能直接复用旧字形的 generated/UV 状态，因此需要
    // 在新图集中完整重建已有字形。
    m_textureAtlas = newAtlas;
    m_atlasWidth = width;
    m_atlasHeight = height;
    // 从这里开始使用方必须重新绑定纹理并重建 Mesh。
    ++m_textureAtlasVersion;

    m_currentX = ATLAS_PADDING;
    m_currentY = ATLAS_PADDING;
    m_currentRowHeight = 0;

    // 旧图集的待上传区域不能提交到新图集。
    m_pendingUploads.clear();

    // 先复制 key，避免 GenerateGlyphToAtlas 更新 unordered_map 时
    // 直接遍历 m_glyphCache。
    std::vector<int32_t> cachedCodepoints;
    cachedCodepoints.reserve(m_glyphCache.size());
    for (auto& pair : m_glyphCache) {
        cachedCodepoints.push_back(pair.first);
        pair.second.generated = false;
    }

    for (const int32_t codepoint : cachedCodepoints) {
        if (!GenerateGlyphToAtlas(codepoint)) {
            LOG_E("failed to rebuild glyph {} after expanding font atlas to {}x{}",
                  codepoint, width, height);
            return false;
        }
    }

    return true;
}

void DynamicFont::UploadBitmapToTexture(int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* bitmap) {
    if (m_textureAtlas) {
        m_textureAtlas->UpdateRegion(x, y, width, height, bitmap);
        m_textureAtlas->FlushToGPU();
    }
}

void DynamicFont::FlushPendingUploads() {
    if (!m_textureAtlas) return;
    for (const auto& p : m_pendingUploads) {
        if (!p.bitmap.empty()) {
            m_textureAtlas->UpdateRegion(p.x, p.y, p.width, p.height, p.bitmap.data());
        }
    }
    m_textureAtlas->FlushToGPU();
    m_pendingUploads.clear();
}

void DynamicFont::EnsureStringGlyphs(const std::wstring& text) {
    for (wchar_t c : text) {
        GetGlyph(static_cast<int32_t>(c));
    }
    FlushPendingUploads();
}
} // morrow
