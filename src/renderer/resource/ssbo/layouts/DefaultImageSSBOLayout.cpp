#include "DefaultImageSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// default_image / anchor_point_scale 共用：geomAttr = (0, 0, 0, alpha)。
DefaultImageSSBOLayout::DefaultImageSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("default_image");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        uiSlotField("color0", offsetof(UIInstanceData, color0), defaultColorSlot()),
        uiSlotField("color1", offsetof(UIInstanceData, color1), defaultColorSlot()),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), alphaGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr), zeroSlot()),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
