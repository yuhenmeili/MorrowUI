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

bool DynamicFont::LoadFromFile(const std::string& filename) {
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

    return InitializeFont();
}

bool DynamicFont::LoadFromMemory(const unsigned char* data, size_t size) {
    m_fontData.assign(data, data + size);
    return InitializeFont();
}

int32_t DynamicFont::NormalizeFontSize(float fontSize) {
    return std::max(1, static_cast<int32_t>(std::lround(fontSize)));
}

DynamicFont::GlyphCache& DynamicFont::GetGlyphCache(float fontSize) {
    const int32_t normalizedSize = NormalizeFontSize(fontSize);
    auto [it, inserted] = m_glyphCaches.try_emplace(normalizedSize);
    if (inserted) {
        it->second.fontSize = static_cast<float>(normalizedSize);
        it->second.scale = stbtt_ScaleForMappingEmToPixels(&m_fontInfo, it->second.fontSize);
        InitializeMetrics(it->second);
    }
    return it->second;
}

const FontGlyph* DynamicFont::GetGlyph(int32_t codepoint, float fontSize) {
    auto& cache = GetGlyphCache(fontSize);
    auto it = cache.glyphs.find(codepoint);
    if (it != cache.glyphs.end()) {
        return &it->second;
    }
    // 生成新的字形
    if (GenerateGlyphToAtlas(codepoint, cache)) {
        return &cache.glyphs[codepoint];
    }
    return nullptr;
}

const FontMetrics& DynamicFont::GetMetrics(float fontSize) {
    return GetGlyphCache(fontSize).metrics;
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
        const FontGlyph* glyph = GetGlyph(static_cast<int32_t>(c), fontSize);
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

namespace {
// ── 自造 SDF：coverage 位图 + 精确欧氏距离变换（Felzenszwalb 1D EDT）──
// stb_truetype v1.26 的 stbtt_GetGlyphSDF 对部分字体（实测 MorrowSansCN /
// SimHei 等 CJK 字体）产生碎片伪影（Arial 正常），故不直接使用；
// 改用已被验证正确的 MakeGlyphBitmap coverage 路径 + 距离变换重建距离场，
// 对任意字体稳定。边缘定位误差 ≤ 0.5px（二值化阈值 128 + 0.5 校正），
// 对 HMI 字号与阴影/描边效果足够。

constexpr int kEdtInf = 0x100000;

// Felzenszwalb-Huttenlocher 一维平方距离变换（lower envelope）
void edt1d(const std::vector<int>& f, int n, std::vector<int>& d) {
    std::vector<int> v(n);
    std::vector<long long> z(n + 1);
    int k = 0;
    v[0] = 0;
    z[0] = -kEdtInf;
    z[1] = kEdtInf;
    for (int q = 1; q < n; ++q) {
        if (f[q] >= kEdtInf) continue;  // INF 抛物线不会进入 lower envelope
        long long s = 0;
        while (true) {
            const long long fq = static_cast<long long>(f[q]) + static_cast<long long>(q) * q;
            const long long fv = static_cast<long long>(f[v[k]]) + static_cast<long long>(v[k]) * v[k];
            s = (fq - fv) / (2 * (q - v[k]));
            if (s > z[k]) break;
            --k;
        }
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = kEdtInf;
    }
    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < q) ++k;
        const long long delta = q - v[k];
        d[q] = static_cast<int>(delta * delta + f[v[k]]);
    }
}

// grid: 每格为"到目标集合的平方距离"；目标集合的格初值为 0，其余求精确 EDT。
void edt2d(std::vector<int>& grid, int width, int height) {
    std::vector<int> f(std::max(width, height));
    std::vector<int> d(std::max(width, height));
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) f[y] = grid[y * width + x];
        edt1d(f, height, d);
        for (int y = 0; y < height; ++y) grid[y * width + x] = d[y];
    }
    for (int y = 0; y < height; ++y) {
        int* row = grid.data() + y * width;
        for (int x = 0; x < width; ++x) f[x] = row[x];
        edt1d(f, width, d);
        for (int x = 0; x < width; ++x) row[x] = d[x];
    }
}

// coverage 位图 → SDF 字节图（0..255，onedge 值编码 0 距离）
void encodeSdfFromCoverage(const unsigned char* coverage, int width, int height,
                           int onedge, float pixelDistScale, std::vector<unsigned char>& out) {
    const size_t count = static_cast<size_t>(width) * height;
    // 距离场语义 = 到"源集合"（初始化为 0 的像素）的最近距离：
    // distOutside 的源是 outside 像素 → inside 像素取值 = 到边缘的深度；
    // distInside  的源是 inside 像素 → outside 像素取值 = 到字形的距离。
    std::vector<int> distOutside(count, kEdtInf);
    std::vector<int> distInside(count, kEdtInf);
    for (size_t i = 0; i < count; ++i) {
        if (coverage[i] >= 128) {
            distInside[i] = 0;
        } else {
            distOutside[i] = 0;
        }
    }
    edt2d(distOutside, width, height);
    edt2d(distInside, width, height);

    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
        // 亚像素校正：二值化（阈值 128）把边缘量化到整像素台阶会产生锯齿，
        // 用原始 coverage 的覆盖率恢复边缘像素的真实亚像素位置——
        // 1D 近似下覆盖 50%+x% 的像素，其中心距边缘约 |x| px（x = cov - 0.5）：
        //   inside: d = D_out - 1.5 + cov   （D_out=1、cov=0.78 → 0.28px；深处 cov=1 → D_out-0.5）
        //   outside: d = -(D_in - 0.5) + cov（D_in=1、cov=0.25 → -0.25px；远处 cov=0 → -(D_in-0.5)）
        const float coverageNorm = coverage[i] / 255.0f;
        float d;
        if (coverage[i] >= 128) {
            d = std::sqrt(static_cast<float>(distOutside[i])) - 1.5f + coverageNorm;   // inside：到边缘深度
        } else {
            d = -(std::sqrt(static_cast<float>(distInside[i])) - 0.5f) + coverageNorm; // outside：到边缘负距离
        }
        const float value = static_cast<float>(onedge) + d * pixelDistScale;
        out[i] = static_cast<unsigned char>(std::clamp(std::lround(value), 0L, 255L));
    }
}
}  // namespace

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
    // 光栅化为 SDF（有向距离场）：按"字形框 ± SDF_PADDING"先光栅化 coverage
    // 位图（MakeGlyphBitmap 路径对任意字体已验证正确），再做 EDT 距离变换
    // 编码。位图原点（xoff/yoff，y-down 相对基线）与 stb SDF 语义一致，
    // placement 语义由 bearingX/bearingY/width/height 承载（消费方不变）。
    const int sdfWidth = (x1 - x0) + SDF_PADDING * 2;
    const int sdfHeight = (y1 - y0) + SDF_PADDING * 2;
    const int sdfXoff = x0 - SDF_PADDING;
    const int sdfYoff = y0 - SDF_PADDING;
    std::vector<unsigned char> coverage(static_cast<size_t>(sdfWidth) * sdfHeight, 0);
    stbtt_MakeGlyphBitmapSubpixel(&m_fontInfo, coverage.data(),
                                  sdfWidth, sdfHeight, sdfWidth,
                                  cache.scale, cache.scale,
                                  static_cast<float>(-sdfXoff), static_cast<float>(-sdfYoff),
                                  glyphIndex);
    std::vector<unsigned char> bitmap;
    encodeSdfFromCoverage(coverage.data(), sdfWidth, sdfHeight,
                          SDF_ONEDGE, SDF_PIXEL_DIST_SCALE, bitmap);
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

    // 按字号重建全部字形。不同字号共享同一张 atlas，但分别拥有度量和字形缓存。
    for (auto& [size, cache] : m_glyphCaches) {
        std::vector<int32_t> cachedCodepoints;
        cachedCodepoints.reserve(cache.glyphs.size());
        for (auto& pair : cache.glyphs) {
            cachedCodepoints.push_back(pair.first);
            pair.second.generated = false;
        }

        for (const int32_t codepoint : cachedCodepoints) {
            if (!GenerateGlyphToAtlas(codepoint, cache)) {
                LOG_E("failed to rebuild glyph {} at {}px after expanding font atlas to {}x{}",
                      codepoint, size, width, height);
                return false;
            }
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
