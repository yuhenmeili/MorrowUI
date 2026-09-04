#include "DefaultColorSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// color0 = color，geomAttr = (displaySize.x, displaySize.y, rounding, alpha)。
DefaultColorSSBOLayout::DefaultColorSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("default_color");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        makeMaterialVectorField("color0", offsetof(UIInstanceData, color0), SSBOValueSource::MaterialVector4, "color"),
        uiSlotField("color1", offsetof(UIInstanceData, color1), defaultColorSlot()),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), displayGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr), zeroSlot()),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
