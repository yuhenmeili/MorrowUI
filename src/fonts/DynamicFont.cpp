#define STB_TRUETYPE_IMPLEMENTATION
#include "DynamicFont.h"

#include <algorithm>
#include <cmath>

#include "FontUtils.h"
#include "Log.h"

namespace morrow {
DynamicFont::DynamicFont() {
    CreateTextureAtlas(m_atlasWidth, m_atlasHeight);
}

DynamicFont::~DynamicFont() {
}

bool DynamicFont::LoadFromFile(const std::string& filename, SdfMethod method) {
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

    m_sdfMethod = method;
    return InitializeFont();
}

bool DynamicFont::LoadFromMemory(const unsigned char* data, size_t size, SdfMethod method) {
    m_fontData.assign(data, data + size);
    m_sdfMethod = method;
    return InitializeFont();
}

SdfMethod DynamicFont::GetSdfMethod() const {
    return m_sdfMethod;
}
float DynamicFont::GetScaleForFontSize(float fontSize) {
    return std::max(fontSize, 1.0f) / REFERENCE_FONT_SIZE;
}

DynamicFont::GlyphCache& DynamicFont::GetGlyphCache() {
    if (m_glyphCache.scale == 1.0f) {
        // 惰性初始化：统一按参考字号建立度量
        m_glyphCache.scale = stbtt_ScaleForMappingEmToPixels(&m_fontInfo, REFERENCE_FONT_SIZE);
        InitializeMetrics(m_glyphCache);
    }
    return m_glyphCache;
}

FontGlyph DynamicFont::GetGlyph(int32_t codepoint, float fontSize) {
    auto& cache = GetGlyphCache();
    auto it = cache.glyphs.find(codepoint);
    if (it == cache.glyphs.end()) {
        // 生成新的字形
        GenerateGlyphToAtlas(codepoint, cache);
        it = cache.glyphs.find(codepoint);
    }
    // 缓存保存参考字号的字形；返回按目标字号缩放的副本（纹理坐标不缩放）
    FontGlyph result = it != cache.glyphs.end() ? it->second : FontGlyph{codepoint};
    const float scale = GetScaleForFontSize(fontSize);
    result.advance *= scale;
    result.bearingX *= scale;
    result.bearingY *= scale;
    result.width *= scale;
    result.height *= scale;
    return result;
}

FontMetrics DynamicFont::GetMetrics(float fontSize) {
    const float scale = GetScaleForFontSize(fontSize);
    FontMetrics metrics = GetGlyphCache().metrics;
    metrics.ascent *= scale;
    metrics.descent *= scale;
    metrics.lineGap *= scale;
    metrics.lineHeight *= scale;
    return metrics;
}

std::shared_ptr<FontTexture> DynamicFont::GetTextureAtlas() const {
    return m_textureAtlas;
}

uint64_t DynamicFont::GetTextureAtlasVersion() const {
    return m_textureAtlasVersion;
}

float DynamicFont::CalculateTextWidth(const std::wstring& text, float fontSize) {
    float width = 0.0f;
    for (wchar_t c : text) {
        const FontGlyph glyph = GetGlyph(static_cast<int32_t>(c), fontSize);
        if (glyph.generated) {
            width += glyph.advance + m_charSpacing;
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
    return true;
}

void DynamicFont::InitializeMetrics(GlyphCache& cache) {
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&m_fontInfo, &ascent, &descent, &lineGap);

    cache.metrics.ascent = ascent * cache.scale;
    cache.metrics.descent = descent * cache.scale;
    cache.metrics.lineGap = lineGap * cache.scale;
    cache.metrics.lineHeight = (ascent - descent + lineGap) * cache.scale;
}

bool DynamicFont::GenerateGlyphToAtlas(int32_t codepoint, GlyphCache& cache) {
    FontGlyph glyph;
    glyph.codepoint = codepoint;
    int glyphIndex = stbtt_FindGlyphIndex(&m_fontInfo, codepoint);
    // 获取字形度量
    int advance, lsb, x0, y0, x1, y1;
    stbtt_GetGlyphHMetrics(&m_fontInfo, glyphIndex, &advance, &lsb);
    stbtt_GetGlyphBitmapBox(&m_fontInfo, glyphIndex, cache.scale, cache.scale, &x0, &y0, &x1, &y1);
    glyph.advance = advance * cache.scale;
    if (x1 - x0 <= 0 || y1 - y0 <= 0) {
        // 空格等不可见字符
        glyph.generated = true;
        cache.glyphs[codepoint] = glyph;
        return true;
    }
    // 光栅化为 SDF（有向距离场）：GlyphSdf 方式由 stbtt 对矢量轮廓逐像素求距，
    // BitmapEdt 方式按"字形框 ± SDF_SPREAD"先光栅化 coverage 位图（MakeGlyphBitmap
    // 路径对任意字体已验证正确），再做 EDT 距离变换编码。两者输出同一种编码
    // （onedge=128、16 单位/px、内部为正），shader 常量共用。位图原点
    // （xoff/yoff，y-down 相对基线）与 stb SDF 语义一致，placement 语义由
    // bearingX/bearingY/width/height 承载（消费方不变）。
    int sdfWidth = 0;
    int sdfHeight = 0;
    int sdfXoff = 0;
    int sdfYoff = 0;
    std::vector<unsigned char> bitmap;
    if (m_sdfMethod == SdfMethod::GlyphSdf) {
        unsigned char* sdf = stbtt_GetGlyphSDF(&m_fontInfo, cache.scale, glyphIndex, SDF_SPREAD, SDF_ONEDGE, SDF_PIXEL_DIST_SCALE, &sdfWidth, &sdfHeight, &sdfXoff, &sdfYoff);
        if (!sdf) {
            return false;
        }
        bitmap.assign(sdf, sdf + static_cast<size_t>(sdfWidth) * sdfHeight);
        stbtt_FreeSDF(sdf, nullptr);
        // stb 的内容绘制在"字形框外扩 padding"处，而 MakeGlyphBitmap 路径因
        // shift 与 box 原点抵消、内容绘制在字形框处；将原点回退一个 padding，
        // 使两种方式的 bearing 语义一致（实测两路径内容位置因此对齐）。
        sdfXoff -= SDF_SPREAD;
        sdfYoff -= SDF_SPREAD;
    } else {
        sdfWidth = (x1 - x0) + SDF_SPREAD * 2;
        sdfHeight = (y1 - y0) + SDF_SPREAD * 2;
        sdfXoff = x0 - SDF_SPREAD;
        sdfYoff = y0 - SDF_SPREAD;
        std::vector<unsigned char> coverage(static_cast<size_t>(sdfWidth) * sdfHeight, 0);
        stbtt_MakeGlyphBitmapSubpixel(&m_fontInfo, coverage.data(),
                                      sdfWidth, sdfHeight, sdfWidth,
                                      cache.scale, cache.scale,
                                      static_cast<float>(-sdfXoff), static_cast<float>(-sdfYoff),
                                      glyphIndex);
        FontUtils::encodeSdfFromCoverage(coverage.data(), sdfWidth, sdfHeight,
                              SDF_ONEDGE, SDF_PIXEL_DIST_SCALE, bitmap);
    }
    glyph.bearingX = static_cast<float>(sdfXoff);
    glyph.bearingY = static_cast<float>(sdfYoff);
    glyph.width = static_cast<float>(sdfWidth);
    glyph.height = static_cast<float>(sdfHeight);
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
    m_currentRowHeight = std::max(m_currentRowHeight, static_cast<int32_t>(glyph.height));
    // 延迟上传：加入待提交列表，由 FlushPendingUploads / EnsureStringGlyphs 整批提交
    m_pendingUploads.push_back({ m_currentX, m_currentY, sdfWidth, sdfHeight, bitmap });
    // 设置纹理坐标 (归一化)
    glyph.texCoordX = static_cast<float>(m_currentX) / m_atlasWidth;
    glyph.texCoordY = static_cast<float>(m_currentY) / m_atlasHeight;
    glyph.texCoordWidth = static_cast<float>(sdfWidth) / m_atlasWidth;
    glyph.texCoordHeight = static_cast<float>(sdfHeight) / m_atlasHeight;
    glyph.generated = true;
    cache.glyphs[codepoint] = glyph;
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

    // 图集扩容后重建全部字形（单一参考字号缓存）。
    std::vector<int32_t> cachedCodepoints;
    cachedCodepoints.reserve(m_glyphCache.glyphs.size());
    for (auto& pair : m_glyphCache.glyphs) {
        cachedCodepoints.push_back(pair.first);
        pair.second.generated = false;
    }

    for (const int32_t codepoint : cachedCodepoints) {
        if (!GenerateGlyphToAtlas(codepoint, m_glyphCache)) {
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

void DynamicFont::EnsureStringGlyphs(const std::wstring& text, float fontSize) {
    for (wchar_t c : text) {
        GetGlyph(static_cast<int32_t>(c), fontSize);
    }
    FlushPendingUploads();
}
} // morrow
