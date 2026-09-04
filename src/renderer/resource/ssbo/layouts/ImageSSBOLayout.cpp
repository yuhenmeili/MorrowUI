#include "ImageSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// image_normal / image_oes / image_text_debug 共用：
// geomAttr = (displaySize.x, displaySize.y, rounding, alpha)，其余槽位默认值。
ImageSSBOLayout::ImageSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("image_normal");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        uiSlotField("color0", offsetof(UIInstanceData, color0), defaultColorSlot()),
        uiSlotField("color1", offsetof(UIInstanceData, color1), defaultColorSlot()),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), displayGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr), zeroSlot()),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
