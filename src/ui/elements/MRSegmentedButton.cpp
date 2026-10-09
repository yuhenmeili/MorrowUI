//
// MRSegmentedButton — 分段选择控件（实现）。
//

#include "morrow/elements/MRSegmentedButton.h"

#include <algorithm>
#include <unordered_map>

#include "morrow/base/Transform.h"
#include "morrow/elements/MRImage.h"

namespace morrow {

namespace {

// 点状背景纹理（按段尺寸生成，RGBA：网格小灰点 + 透明底）。
// setTextureData 的 shared_ptr 重载由纹理持有像素缓冲，与缓存条目一起保活。
struct DotTextureEntry {
    TextureSharedPtr texture;
    std::shared_ptr<std::vector<unsigned char>> pixels;
};

TextureSharedPtr getDotTexture(int width, int height) {
    static std::unordered_map<uint64_t, DotTextureEntry> cache;
    const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(width)) << 32) | static_cast<uint32_t>(height);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second.texture;
    }

    constexpr int kPeriod = 5;  // 点间距（像素）
    constexpr int kDotSize = 2; // 点边长（像素）
    auto pixels = std::make_shared<std::vector<unsigned char>>(static_cast<size_t>(width) * height * 4, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x % kPeriod < kDotSize && y % kPeriod < kDotSize) {
                const size_t index = (static_cast<size_t>(y) * width + x) * 4;
                (*pixels)[index + 0] = 190;
                (*pixels)[index + 1] = 192;
                (*pixels)[index + 2] = 198;
                (*pixels)[index + 3] = 150;
            }
        }
    }

    DotTextureEntry entry;
    entry.pixels = pixels;
    entry.texture = Texture::create(ImageType::IMAGE);
    entry.texture->setTextureData(pixels, width, height, PixelDataFormat::RGBA);
    entry.texture->setTextureName("segment_dots_" + std::to_string(width) + "x" + std::to_string(height));
    cache.emplace(key, entry);
    return entry.texture;
}

} // namespace

std::shared_ptr<MRSegmentedButton> MRSegmentedButton::create() {
    return std::shared_ptr<MRSegmentedButton>(new MRSegmentedButton());
}

MRSegmentedButton::MRSegmentedButton() : UIWidget(false) {
    setWidgetType("MRSegmentedButton");
    // 纯容器：不创建渲染组件，底色/毛玻璃由外层容器负责
}

void MRSegmentedButton::addSegment(const std::wstring& text) {
    const int index = static_cast<int>(m_segments.size());

    Segment segment;
    segment.dotOverlay = MRImage::create();
    segment.dotOverlay->setVisible(false);
    addChild(segment.dotOverlay);

    segment.button = MRButton::create();
    segment.button->setText(text, "default");
    segment.button->setTextFontSize(26.0f);
    segment.button->setTextAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    segment.button->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f);
    segment.button->setHoverColor(Vector4(1.0f, 1.0f, 1.0f, 0.06f));
    segment.button->setPressedColor(Vector4(0.0f, 0.0f, 0.0f, 0.12f));
    segment.clickConnection = segment.button->events().onClicked.connect([this, index](BaseButton&) {
        setSelectedIndex(index);
    });
    addChild(segment.button);

    m_segments.push_back(std::move(segment));
    layoutSegments();
    if (m_selectedIndex < 0 && m_segments.size() == 1) {
        setSelectedIndex(0);
    }
}

void MRSegmentedButton::setSelectedIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_segments.size()) || index == m_selectedIndex) {
        return;
    }
    m_selectedIndex = index;
    applySelection();
    if (m_selectionChanged) {
        m_selectionChanged(index);
    }
}

int MRSegmentedButton::getSelectedIndex() const {
    return m_selectedIndex;
}

void MRSegmentedButton::setSelectedColor(const Vector4& color) {
    m_selectedColor = color;
}

void MRSegmentedButton::setNormalColor(const Vector4& color) {
    m_normalColor = color;
}

void MRSegmentedButton::setDotInset(float insetX, float insetY) {
    m_dotInsetX = std::max(insetX, 0.0f);
    m_dotInsetY = std::max(insetY, 0.0f);
    layoutSegments();
}

void MRSegmentedButton::setSelectedTexture(const TextureSharedPtr& texture) {
    m_selectedTexture = texture;
    layoutSegments();
}

void MRSegmentedButton::setSelectedTextureImage(const std::string& imageUrl) {
    auto texture = Texture::create(ImageType::IMAGE);
    texture->setImageUrl(imageUrl);
    setSelectedTexture(texture);
}

void MRSegmentedButton::setOnSelectionChanged(std::function<void(int index)> callback) {
    m_selectionChanged = std::move(callback);
}

void MRSegmentedButton::layoutSegments() {
    if (m_segments.empty()) {
        return;
    }
    const auto transform = getComponent<Transform>();
    if (!transform) {
        return;
    }
    const Vector3 size = transform->getSize();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }
    const float segmentWidth = size.x / static_cast<float>(m_segments.size());

    // 选中背景图略小于段：按内缩量留边（横向/纵向独立可设）
    const float overlayWidth = std::max(1.0f, segmentWidth - m_dotInsetX * 2.0f);
    const float overlayHeight = std::max(1.0f, size.y - m_dotInsetY * 2.0f);
    const TextureSharedPtr backgroundTexture =
        m_selectedTexture ? m_selectedTexture : getDotTexture(static_cast<int>(overlayWidth), static_cast<int>(overlayHeight));

    for (size_t i = 0; i < m_segments.size(); ++i) {
        auto& segment = m_segments[i];
        const float x = static_cast<float>(i) * segmentWidth;
        segment.dotOverlay->setTexture(backgroundTexture);
        segment.dotOverlay->getComponent<Transform>()->setPosition(x + m_dotInsetX, m_dotInsetY, 0.0f);
        segment.dotOverlay->getComponent<Transform>()->setSize(overlayWidth, overlayHeight);
        segment.button->getComponent<Transform>()->setPosition(x, 0.0f, 0.0f);
        segment.button->getComponent<Transform>()->setSize(segmentWidth, size.y);
    }
    applySelection();
}

void MRSegmentedButton::applySelection() {
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const bool selected = (static_cast<int>(i) == m_selectedIndex);
        m_segments[i].dotOverlay->setVisible(selected);
        m_segments[i].button->setTextColor(selected ? m_selectedColor : m_normalColor);
    }
}

} // namespace morrow
