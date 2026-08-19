#include "MRSpacer.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRSpacer> MRSpacer::create(float flex) {
    return std::shared_ptr<MRSpacer>(new MRSpacer(flex));
}

MRSpacer::MRSpacer(float flex) : UIWidget(false), m_flex(std::max(0.0f, flex)) {
    setWidgetType("MRSpacer");
}

void MRSpacer::setFlex(float flex) {
    m_flex = std::max(0.0f, flex);
    requestRender("spacer flex");
}

void MRSpacer::setMinimumSize(const Vector2& size) {
    m_minimumSize = Vector2(std::max(0.0f, size.x), std::max(0.0f, size.y));
    getComponent<Transform>()->setSize(m_minimumSize.x, m_minimumSize.y);
}

}  // namespace morrow
