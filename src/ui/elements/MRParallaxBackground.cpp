#include "MRParallaxBackground.h"

#include <algorithm>

namespace morrow {

std::shared_ptr<MRParallaxBackground> MRParallaxBackground::create() {
    return std::shared_ptr<MRParallaxBackground>(new MRParallaxBackground());
}

MRParallaxBackground::MRParallaxBackground() : UIWidget(false) {
    setWidgetType("MRParallaxBackground");
}

void MRParallaxBackground::addLayer(const MRParallax2DSharedPtr& layer, float scaleX, float scaleY) {
    if (!layer)
        return;
    layer->setScrollScale(scaleX, scaleY);
    layer->setScrollOffset(m_scrollOffset);
    if (layer->m_parent != shared_from_this())
        addChild(layer);
    if (std::find(m_layers.begin(), m_layers.end(), layer) == m_layers.end())
        m_layers.push_back(layer);
}

bool MRParallaxBackground::removeLayer(const MRParallax2DSharedPtr& layer) {
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it == m_layers.end())
        return false;
    removeChild(layer);
    m_layers.erase(it);
    return true;
}

void MRParallaxBackground::setScrollOffset(float x, float y) {
    m_scrollOffset = Vector2(x, y);
    for (const auto& layer : m_layers)
        layer->setScrollOffset(m_scrollOffset);
}

Vector2 MRParallaxBackground::getScrollOffset() const {
    return m_scrollOffset;
}

const std::vector<MRParallax2DSharedPtr>& MRParallaxBackground::getLayers() const {
    return m_layers;
}

}  // namespace morrow
