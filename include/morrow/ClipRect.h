#ifndef MORROW_CLIP_RECT_H
#define MORROW_CLIP_RECT_H

#include <algorithm>

namespace morrow {

struct ClipRect {
    bool enabled = false;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    static ClipRect fromBounds(float left, float top, float right, float bottom) {
        return {true, left, top, right, bottom};
    }

    bool contains(float x, float y) const {
        return !enabled || (x >= left && x <= right && y >= top && y <= bottom);
    }

    bool empty() const {
        return enabled && (right <= left || bottom <= top);
    }

    ClipRect intersected(const ClipRect& other) const {
        if (!enabled)
            return other;
        if (!other.enabled)
            return *this;
        return {
            true,
            std::max(left, other.left),
            std::max(top, other.top),
            std::min(right, other.right),
            std::min(bottom, other.bottom),
        };
    }

    bool operator==(const ClipRect& other) const {
        return enabled == other.enabled &&
               (!enabled || (left == other.left && top == other.top &&
                             right == other.right && bottom == other.bottom));
    }

    bool operator!=(const ClipRect& other) const {
        return !(*this == other);
    }
};

}  // namespace morrow

#endif  // MORROW_CLIP_RECT_H
