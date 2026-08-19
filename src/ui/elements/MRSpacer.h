#ifndef MORROW_GUI_MRSPACER_H
#define MORROW_GUI_MRSPACER_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow {

class MRSpacer : public UIWidget {
public:
    static std::shared_ptr<MRSpacer> create(float flex = 1.0f);

    void setFlex(float flex);

    float getFlex() const {
        return m_flex;
    }

    void setMinimumSize(const Vector2& size);

    Vector2 getMinimumSize() const {
        return m_minimumSize;
    }

private:
    explicit MRSpacer(float flex);

    float m_flex = 1.0f;
    Vector2 m_minimumSize = Vector2(0.0f, 0.0f);
};

using MRSpacerSharedPtr = std::shared_ptr<MRSpacer>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSPACER_H
