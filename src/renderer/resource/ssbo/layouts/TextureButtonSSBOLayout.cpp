#include "TextureButtonSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// geomAttr = (0, 0, 0, alpha)。
TextureButtonSSBOLayout::TextureButtonSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("texture_button");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        uiSlotField("color0", offsetof(UIInstanceData, color0), defaultColorSlot()),
        uiSlotField("color1", offsetof(UIInstanceData, color1), defaultColorSlot()),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), alphaGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr), zeroSlot()),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
