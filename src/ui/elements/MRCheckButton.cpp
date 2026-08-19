#include "MRCheckButton.h"

namespace morrow {

std::shared_ptr<MRCheckButton> MRCheckButton::create() {
    return std::shared_ptr<MRCheckButton>(new MRCheckButton());
}

MRCheckButton::MRCheckButton() {
    setWidgetType("MRCheckButton");
}

}  // namespace morrow
