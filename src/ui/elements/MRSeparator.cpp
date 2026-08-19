#include "MRSeparator.h"

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRHSeparator> MRHSeparator::create() {
    return std::shared_ptr<MRHSeparator>(new MRHSeparator());
}

MRHSeparator::MRHSeparator() {
    setWidgetType("MRHSeparator");
    setColor(0.76f, 0.79f, 0.84f, 1.0f);
    getComponent<Transform>()->setSize(120.0f, 1.0f);
}

std::shared_ptr<MRVSeparator> MRVSeparator::create() {
    return std::shared_ptr<MRVSeparator>(new MRVSeparator());
}

MRVSeparator::MRVSeparator() {
    setWidgetType("MRVSeparator");
    setColor(0.76f, 0.79f, 0.84f, 1.0f);
    getComponent<Transform>()->setSize(1.0f, 48.0f);
}

}  // namespace morrow
