#include "MRParallax2D.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRParallax2D> MRParallax2D::create() {
    return std::shared_ptr<MRParallax2D>(new MRParallax2D());
}

MRParallax2D::MRParallax2D() : UIWidget(false) {
    setWidgetType("MRParallax2D");
}

void MRParallax2D::setScrollScale(float x, float y) {
    m_scrollScale = Vector2(x, y);
    applyPosition();
}

Vector2 MRParallax2D::getScrollScale() const {
    return m_scrollScale;
}

void MRParallax2D::setBasePosition(float x, float y, float z) {
    m_basePosition = Vector3(x, y, z);
    applyPosition();
}

void MRParallax2D::setScrollOffset(const Vector2& offset) {
    m_scrollOffset = offset;
    applyPosition();
}

Vector2 MRParallax2D::getScrollOffset() const {
    return m_scrollOffset;
}

void MRParallax2D::applyPosition() {
    getComponent<Transform>()->setPosition(
        m_basePosition.x - m_scrollOffset.x * m_scrollScale.x,
        m_basePosition.y - m_scrollOffset.y * m_scrollScale.y,
        m_basePosition.z);
}

}  // namespace morrow
