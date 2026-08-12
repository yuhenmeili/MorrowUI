//
// Created by 0060328 on 25-10-9.
//

#include "MRLabel.h"

#include <algorithm>

#include "FontManager.h"
#include "GlobalObject.h"
#include "ssbo/ShaderStorageBuffer.h"
#include "base/Transform.h"
#include "renderer/resource/ssbo/layouts/FontSSBOLayout.h"

namespace morrow {
MRLabel::MRLabel() {
    setWidgetType("MRTextRenderer");
    m_material->setShader("font");
    m_material->setSSBOLayout(std::make_shared<FontSSBOLayout>());
    m_material->setVector("fontColor", Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        m_isAlignDirty = true;
        requestRender("labelSizeChanged");
    });
}

void MRLabel::setText(const std::wstring& text, const std::string& fontName) {
    if (m_text != text || m_fontName != fontName) {
        m_text = text;
        m_fontName = fontName;
        m_isTextLayoutDirty = true;
        requestRender("setText");
    }
}

void MRLabel::setFontSize(float fontSize) {
    fontSize = std::max(1.0f, fontSize);
    if (m_fontSize != fontSize) {
        m_fontSize = fontSize;
        m_isTextLayoutDirty = true;
        m_isAlignDirty = true;
        requestRender("setFontSize");
    }
}

float MRLabel::getFontSize() const {
    return m_fontSize;
}

void MRLabel::setFontColor(float r, float g, float b, float a) {
    setFontColor(Vector4(r, g, b, a));
}

void MRLabel::setFontColor(const Vector4& color) {
    m_material->setVector("fontColor", color);
    requestRender("setFontColor");
}

void MRLabel::setAlign(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAlignment) {
    m_horizontalAlignment = horizontalAlignment;
    m_verticalAlignment = verticalAlignment;
    m_isAlignDirty = true;
    requestRender("setAlign");
}

void MRLabel::setAutoWrap(bool enable) {
    if (m_autoWrap != enable) {
        m_autoWrap = enable;
        m_isTextLayoutDirty = true;
        requestRender("setAutoWrap");
    }
}

void MRLabel::setCharacterSpacing(float spacing) {
    if (m_characterSpacing != spacing) {
        m_characterSpacing = spacing;
        m_isTextLayoutDirty = true;
        requestRender("setCharacterSpacing");
    }
}

void MRLabel::setLineSpacing(float spacing) {
    if (m_lineSpacing != spacing) {
        m_lineSpacing = spacing;
        m_isTextLayoutDirty = true;
        requestRender("setLineSpacing");
    }
}

void MRLabel::setMaxLines(int maxLines) {
    if (m_maxLines != maxLines) {
        m_maxLines = maxLines;
        m_isTextLayoutDirty = true;
        requestRender("setMaxLines");
    }
}

Vector2 MRLabel::getTextExtents() const {
    return Vector2(m_textWidth, m_textHeight);
}

void MRLabel::update(FrameStateSharedPtr frameState) {
    // 其他 MRLabel 可能在本帧扩容并替换了共享 DynamicFont 的图集。
    // 当前 Mesh 的 UV 和材质纹理都可能对应旧图集，因此需要重建。
    if (m_font && m_font->GetTextureAtlasVersion() != m_fontAtlasVersion) {
        m_isTextLayoutDirty = true;
        m_isAlignDirty = true;
    }

    if (m_isTextLayoutDirty || m_isAlignDirty) {
        m_font = GlobalObject::getInstance().getFontManager()->getFont(m_fontName);
        if (!m_font && !m_text.empty()) {
            LOG_E("MRLabel failed to resolve font '{}' for text length {}", m_fontName, m_text.size());
        }

        processTextLayout();

        // processTextLayout() 中的 GetGlyph 可能触发图集扩容，
        // 因此只在布局完成后绑定一次，确保使用的是最新图集。
        if (m_font) {
            m_material->setTexture("texture", m_font->GetTextureAtlas());
        }

        applyAlignment();
        createTextMesh();
        m_isTextLayoutDirty = false;
        m_isAlignDirty = false;
        if (m_font) {
            // 在布局完成后读取版本，覆盖本次布局中可能触发的扩容。
            m_fontAtlasVersion = m_font->GetTextureAtlasVersion();
        }
    }
    UIWidget::update(frameState);
}

//核心布局处理函数
void MRLabel::processTextLayout() {
    if (!m_font || m_text.empty()) {
        m_lines.clear();
        m_textWidth = 0.0f;
        m_textHeight = 0.0f;
        return;
    }
    const auto& metrics = m_font->GetMetrics(m_fontSize);
    auto transform = getComponent<Transform>();
    Vector3 containerSize = transform->getSize(); // 默认大尺寸

    m_lines.clear();
    TextLine currentLine;
    float maxLineWidth = 0.0f;

    float currentX = 0.0f;
    float spaceAdvance = 0.0f;

    // 获取空格字符的advance
    const FontGlyph* spaceGlyph = m_font->GetGlyph(L' ', m_fontSize);
    if (spaceGlyph) {
        spaceAdvance = spaceGlyph->advance + m_characterSpacing;
    }

    for (size_t i = 0; i < m_text.length(); ++i) {
        wchar_t c = m_text[i];
        const FontGlyph* glyph = m_font->GetGlyph(c, m_fontSize);

        if (!glyph || !glyph->generated) {
            // 对于不可渲染字符，使用空格宽度
            currentX += spaceAdvance;
            continue;
        }

        float glyphWidth = glyph->advance + m_characterSpacing;
        float potentialLineWidth = currentX + glyphWidth;

        // 检查自动换行
        if (m_autoWrap && !currentLine.characters.empty() &&
            potentialLineWidth > containerSize.x) {
            // 保存当前行
            currentLine.width = currentX - m_characterSpacing; // 减去最后一个字符的间距
            m_lines.push_back(currentLine);
            maxLineWidth = std::max(maxLineWidth, currentLine.width);

            // 开始新行
            currentLine.characters.clear();
            currentX = 0.0f;
        }

        // 添加到当前行
        currentLine.characters.push_back(c);
        currentX += glyphWidth;

        // 处理换行符
        if (c == L'\n') {
            currentLine.width = currentX - glyphWidth; // 不包含换行符的宽度
            m_lines.push_back(currentLine);
            maxLineWidth = std::max(maxLineWidth, currentLine.width);

            currentLine.characters.clear();
            currentX = 0.0f;
        }

        // 检查最大行数限制
        if (m_maxLines > 0 && m_lines.size() >= m_maxLines) {
            break;
        }
    }

    // 添加最后一行
    if (!currentLine.characters.empty()) {
        currentLine.width = currentX - m_characterSpacing;
        m_lines.push_back(currentLine);
        maxLineWidth = std::max(maxLineWidth, currentLine.width);
    }

    // 计算文本总尺寸
    m_textWidth = maxLineWidth;
    m_textHeight = m_lines.size() * metrics.lineHeight * m_lineSpacing;
    // 本串所需字形已在本轮 GetGlyph 中生成，整串一次性提交到 GPU
    m_font->FlushPendingUploads();
}

//核心对齐应用函数（局部坐标系：左上角为 (0,0)，x 向右、y 向下，与 Transform 的 localPosition=左上角 一致）
void MRLabel::applyAlignment() {
    auto transform = getComponent<Transform>();
    if (!transform || m_lines.empty()) return;
    Vector3 size = transform->getSize();

    const auto& metrics = m_font->GetMetrics(m_fontSize);
    float lineHeight = metrics.lineHeight * m_lineSpacing;

    // 垂直对齐：第一行在容器内的 y 起始位置
    float verticalOffset = 0.0f;
    switch (m_verticalAlignment) {
        case VerticalAlignment::TOP:
            verticalOffset = 0.0f;
            break;
        case VerticalAlignment::CENTER:
            verticalOffset = (size.y - m_textHeight) * 0.5f;
            break;
        case VerticalAlignment::BOTTOM:
            verticalOffset = size.y - m_textHeight;
            break;
        default: break;
    }

    for (size_t i = 0; i < m_lines.size(); ++i) {
        auto& line = m_lines[i];

        switch (m_horizontalAlignment) {
            case HorizontalAlignment::LEFT:
                line.x_offset = 0.0f;
                break;
            case HorizontalAlignment::CENTER:
                line.x_offset = (size.x - line.width) * 0.5f;
                break;
            case HorizontalAlignment::RIGHT:
                line.x_offset = size.x - line.width;
                break;
            case HorizontalAlignment::JUSTIFY:
                if (i == m_lines.size() - 1 || m_lines.size() == 1) {
                    line.x_offset = 0.0f;
                } else {
                    line.x_offset = 0.0f;
                }
                break;
        }

        line.y_offset = verticalOffset + i * lineHeight;
    }
}

//核心网格生成函数
void MRLabel::createTextMesh() {
    auto transform = getComponent<Transform>();
    auto mesh = getComponent<MeshFilter>()->getMesh();
    mesh->clear();
    if (!m_font || m_lines.empty() || !transform) {
        return;
    }

    std::vector<Vector3> vertices;
    std::vector<Vector2> uvs;
    std::vector<int16_t> indices;

    const auto& metrics = m_font->GetMetrics(m_fontSize);

    // 为每一行生成网格
    for (const auto& line : m_lines) {
        float currentX = line.x_offset;
        float baselineY = line.y_offset + metrics.ascent; // 基线位置

        for (wchar_t c : line.characters) {
            if (c == L'\n') continue; // 跳过换行符

            const FontGlyph* glyph = m_font->GetGlyph(c, m_fontSize);
            if (!glyph || !glyph->generated) continue;

            // 渲染字符四边形
            renderGlyphQuad(glyph, currentX, baselineY, vertices, uvs, indices);
            currentX += glyph->advance + m_characterSpacing;
        }
    }

    const Vector3 size = transform->getSize();
    const float halfW = size.x * 0.5f;
    const float halfH = size.y * 0.5f;
    for (auto& v : vertices) {
        v.x -= halfW;
        v.y = halfH - v.y;
    }

    m_originalVertices = vertices;

    mesh->setVertices(vertices);
    mesh->setUVs(uvs);
    mesh->setIndices(indices);
}

void MRLabel::renderGlyphQuad(const FontGlyph* glyph, float x, float baselineY,
                              std::vector<Vector3>& vertices,
                              std::vector<Vector2>& uvs,
                              std::vector<int16_t>& indices) {
    // 计算四边形顶点（考虑bearing）
    float x0 = x + glyph->bearingX;
    float y0 = baselineY + glyph->bearingY; // 注意坐标系
    float x1 = x0 + glyph->width;
    float y1 = y0 + glyph->height;
    // float x0 = 0.0f;
    // float y0 = 0.0f; //
    // float x1 = 100.0f;
    // float y1 = 100.0f;

    // 纹理坐标
    float u0 = glyph->texCoordX;
    float v0 = glyph->texCoordY;
    float u1 = u0 + glyph->texCoordWidth;
    float v1 = v0 + glyph->texCoordHeight;
    // float u0 = 0.0f;
    // float v0 = 0.0f;
    // float u1 = 1.0f;
    // float v1 = 1.0f;

    // 获取当前顶点索引
    auto currentIndex = static_cast<int16_t>(vertices.size());

    // 添加四个顶点（逆时针顺序）
    vertices.emplace_back(x0, y0, 0.0f); // 左上
    vertices.emplace_back(x1, y0, 0.0f); // 右上
    vertices.emplace_back(x1, y1, 0.0f); // 右下
    vertices.emplace_back(x0, y1, 0.0f); // 左下

    // 添加纹理坐标
    uvs.emplace_back(u0, v0); // 左上
    uvs.emplace_back(u1, v0); // 右上
    uvs.emplace_back(u1, v1); // 右下
    uvs.emplace_back(u0, v1); // 左下

    // LOG_I("x0: {}, y0: {}, x1:{}, y1:{}", x0, y0, x1, y1);
    // LOG_I("u0: {}, v0: {}, u1:{}, v1:{}", u0, v0, u1, v1);

    // 添加三角形索引（两个三角形组成四边形）
    indices.emplace_back(currentIndex); // 左上
    indices.emplace_back(currentIndex + 1); // 右上
    indices.emplace_back(currentIndex + 2); // 右下

    indices.emplace_back(currentIndex + 2); // 右下
    indices.emplace_back(currentIndex + 3); // 左下
    indices.emplace_back(currentIndex); // 左上
}
} // namespace morrow
