//
// Created by lance on 2025/10/2.
//
#include "MRButton.h"
#include "GlobalObject.h"
#include "base/Transform.h"
#include "base/MeshFilter.h"
#include "base/MeshRenderer.h"
#include "renderer/resource/ssbo/layouts/ButtonSSBOLayout.h"
#include <functional>

namespace morrow {
MRButton::MRButton() {
    setWidgetType("MRButton");
    // 创建标签组件
    m_label = std::make_shared<MRLabel>();
    m_label->setFontColor(m_currentTextColor);
    m_label->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    // 设置材质
    m_material->setShader("button");
    m_material->setSSBOLayout(std::make_shared<ButtonSSBOLayout>());
    m_material->setFloat("rounding", 0.0f);
    // 监听尺寸变化
    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this, transform]() {
        // 更新标签尺寸为按钮内部尺寸（考虑边框�?
        Vector3 size = transform->getSize();
        m_material->setVector("displaySize", size);

        m_isBackgroundDirty = true;
        m_isBorderDirty = true;
        if (m_label) {
            Vector3 innerSize = Vector3(
                size.x - m_borderWidth * 2.0f,
                size.y - m_borderWidth * 2.0f,
                size.z
            );
            m_label->getComponent<Transform>()->setSize(innerSize);
        }
    });
}

std::shared_ptr<MRButton> MRButton::create() {
    return std::shared_ptr<MRButton>(new MRButton());
}

// 文本相关方法
void MRButton::setText(const std::wstring& text, const std::string& fontName) {
    if (m_label) {
        m_label->setText(text, fontName);
        addChild(m_label);
    }
}

void MRButton::setTextColor(const Vector4& color) {
    m_currentTextColor = color;
    if (m_label) {
        m_label->setFontColor(color);
    }
}

void MRButton::setTextColor(float r, float g, float b, float a) {
    setTextColor(Vector4(r, g, b, a));
}

// 背景颜色相关方法
void MRButton::setBackgroundImage(TextureSharedPtr texture) {
    m_backgroundTexture = texture;
    m_hasBackgroundImage = (texture != nullptr);
    if (m_hasBackgroundImage) {
        m_material->setTexture("texture", texture);
        m_material->setFloat("useTexture", 1.0f);
        // 默认白色底，让原图完整显示
        m_backgroundColorNormal = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        m_backgroundColorHover = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        m_backgroundColorPressed = Vector4(0.85f, 0.85f, 0.85f, 1.0f);
        m_backgroundColorDisabled = Vector4(0.6f, 0.6f, 0.6f, 0.6f);
    } else {
        m_material->setFloat("useTexture", 0.0f);
        m_backgroundColorNormal = Vector4(0.2f, 0.4f, 0.8f, 1.0f);
        m_backgroundColorHover = Vector4(0.3f, 0.5f, 0.9f, 1.0f);
        m_backgroundColorPressed = Vector4(0.1f, 0.3f, 0.7f, 1.0f);
        m_backgroundColorDisabled = Vector4(0.5f, 0.5f, 0.5f, 0.5f);
    }
    updateVisualState();
    m_isBackgroundDirty = true;
}

void MRButton::setBackgroundColor(const Vector4& color) {
    m_backgroundColorNormal = color;
    updateVisualState();
}

void MRButton::setBackgroundColor(float r, float g, float b, float a) {
    setBackgroundColor(Vector4(r, g, b, a));
}

void MRButton::setHoverColor(const Vector4& color) {
    m_backgroundColorHover = color;
    updateVisualState();
}

void MRButton::setPressedColor(const Vector4& color) {
    m_backgroundColorPressed = color;
    updateVisualState();
}

void MRButton::setDisabledColor(const Vector4& color) {
    m_backgroundColorDisabled = color;
    updateVisualState();
}

// 边框相关方法
void MRButton::setBorderColor(const Vector4& color) {
    m_borderColor = color;
    m_isBorderDirty = true;
}

void MRButton::setBorderWidth(float width) {
    if (m_borderWidth != width) {
        m_borderWidth = width;
        m_isBorderDirty = true;
        m_isBackgroundDirty = true;

        // 更新标签尺寸
        auto transform = getComponent<Transform>();
        Vector3 size = transform->getSize();
        Vector3 innerSize = Vector3(
            size.x - m_borderWidth * 2.0f,
            size.y - m_borderWidth * 2.0f,
            size.z
        );
        if (m_label) {
            m_label->getComponent<Transform>()->setSize(innerSize);
        }
    }
}

void MRButton::setCornerRadius(float radius) {
    if (m_cornerRadius != radius) {
        m_cornerRadius = radius;
        m_material->setFloat("rounding", radius);
    }
}

// 文本布局相关方法
void MRButton::setTextAlign(HorizontalAlignment horizontal, VerticalAlignment vertical) {
    if (m_label) {
        m_label->setAlign(horizontal, vertical);
    }
}

void MRButton::setAutoWrap(bool enable) {
    if (m_label) {
        m_label->setAutoWrap(enable);
    }
}

void MRButton::setCharacterSpacing(float spacing) {
    if (m_label) {
        m_label->setCharacterSpacing(spacing);
    }
}

void MRButton::setLineSpacing(float spacing) {
    if (m_label) {
        m_label->setLineSpacing(spacing);
    }
}

void MRButton::setMaxLines(int maxLines) {
    if (m_label) {
        m_label->setMaxLines(maxLines);
    }
}

// 尺寸相关
Vector2 MRButton::getPreferredSize() const {
    Vector2 textSize(100.0f, 30.0f); // 默认尺寸

    if (m_label) {
        textSize = m_label->getTextExtents();
    }

    // 加上边框和内边距
    return Vector2(
        textSize.x + m_borderWidth * 2.0f + 20.0f, // 水平内边�?
        textSize.y + m_borderWidth * 2.0f + 10.0f // 垂直内边�?
    );
}

void MRButton::update(FrameStateSharedPtr frameState) {
    // 更新背景网格
    if (m_isBackgroundDirty) {
        createBackgroundMesh();
        m_isBackgroundDirty = false;
    }

    // 更新边框网格
    if (m_isBorderDirty) {
        // createBorderMesh();
        m_isBorderDirty = false;
    }

    // 应用当前颜色
    applyCurrentColors();

    UIWidget::update(frameState);
}

// 私有方法实现
void MRButton::updateVisualState() {
    switch (m_currentState) {
        case ButtonState::NORMAL:
            m_currentBackgroundColor = m_backgroundColorNormal;
            break;
        case ButtonState::HOVER:
            m_currentBackgroundColor = m_backgroundColorHover;
            break;
        case ButtonState::PRESSED:
            m_currentBackgroundColor = m_backgroundColorPressed;
            break;
        case ButtonState::DISABLED:
            m_currentBackgroundColor = m_backgroundColorDisabled;
            break;
    }
}

void MRButton::createBackgroundMesh() {
    auto transform = getComponent<Transform>();
    Vector3 size = transform->getSize();

    auto backgroundMesh = getComponent<MeshFilter>()->getMesh();
    backgroundMesh->clear();
    m_backgroundVertices.clear();
    m_backgroundUVs.clear();
    m_backgroundIndices.clear();

    const float halfW = size.x * 0.5f;
    const float halfH = size.y * 0.5f;
    const float left = -halfW + m_borderWidth;
    const float right = halfW - m_borderWidth;
    const float top = halfH - m_borderWidth;
    const float bottom = -halfH + m_borderWidth;

    // TL, TR, BR, BL in center-origin y-up local space
    m_backgroundVertices.emplace_back(left, top, 0.0f);
    m_backgroundVertices.emplace_back(right, top, 0.0f);
    m_backgroundVertices.emplace_back(right, bottom, 0.0f);
    m_backgroundVertices.emplace_back(left, bottom, 0.0f);

    m_backgroundUVs.emplace_back(0.0f, 0.0f);
    m_backgroundUVs.emplace_back(1.0f, 0.0f);
    m_backgroundUVs.emplace_back(1.0f, 1.0f);
    m_backgroundUVs.emplace_back(0.0f, 1.0f);

    m_backgroundIndices.emplace_back(0);
    m_backgroundIndices.emplace_back(1);
    m_backgroundIndices.emplace_back(2);
    m_backgroundIndices.emplace_back(2);
    m_backgroundIndices.emplace_back(3);
    m_backgroundIndices.emplace_back(0);

    backgroundMesh->setVertices(m_backgroundVertices);
    backgroundMesh->setUVs(m_backgroundUVs);
    backgroundMesh->setIndices(m_backgroundIndices);
}

void MRButton::createBorderMesh() {
    if (m_borderWidth <= 0.0f) {
        return;
    }

    auto transform = getComponent<Transform>();
    Vector3 size = transform->getSize();

    m_borderVertices.clear();
    m_borderUVs.clear();
    m_borderIndices.clear();

    // Center-origin, y-up local space.
    const float halfW = size.x * 0.5f;
    const float halfH = size.y * 0.5f;
    const float outerLeft = -halfW;
    const float outerRight = halfW;
    const float outerTop = halfH;
    const float outerBottom = -halfH;

    const float innerLeft = outerLeft + m_borderWidth;
    const float innerRight = outerRight - m_borderWidth;
    const float innerTop = outerTop - m_borderWidth;
    const float innerBottom = outerBottom + m_borderWidth;

    // Top, Right, Bottom, Left
    addBorderQuad(outerLeft, outerTop, outerRight, innerTop);
    addBorderQuad(innerRight, outerTop, outerRight, outerBottom);
    addBorderQuad(outerLeft, innerBottom, outerRight, outerBottom);
    addBorderQuad(outerLeft, outerTop, innerLeft, outerBottom);
}

void MRButton::addBorderQuad(float left, float top, float right, float bottom) {
    auto startIndex = static_cast<int16_t>(m_borderVertices.size());

    // TL, TR, BR, BL
    m_borderVertices.emplace_back(left, top, 0.0f);
    m_borderVertices.emplace_back(right, top, 0.0f);
    m_borderVertices.emplace_back(right, bottom, 0.0f);
    m_borderVertices.emplace_back(left, bottom, 0.0f);

    m_borderUVs.emplace_back(0.0f, 0.0f);
    m_borderUVs.emplace_back(1.0f, 0.0f);
    m_borderUVs.emplace_back(1.0f, 1.0f);
    m_borderUVs.emplace_back(0.0f, 1.0f);

    m_borderIndices.emplace_back(startIndex);
    m_borderIndices.emplace_back(startIndex + 1);
    m_borderIndices.emplace_back(startIndex + 2);

    m_borderIndices.emplace_back(startIndex + 2);
    m_borderIndices.emplace_back(startIndex + 3);
    m_borderIndices.emplace_back(startIndex);
}

void MRButton::applyCurrentColors() {
    // 设置背景颜色
    m_material->setVector("color", m_currentBackgroundColor);

    // 设置边框颜色（如果有边框�?
    if (m_borderWidth > 0.0f) {
        // 如果有单独的边框材质，这里设置边框颜�?
        // m_borderMaterial->setVector("color", m_borderColor);
    }
}
} // namespace morrowow
