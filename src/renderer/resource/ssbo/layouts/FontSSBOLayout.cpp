#include "FontSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// color0 = fontColor，geomAttr = (0, 0, 0, alpha)，
// stateAttr.x = sdfScale（目标字号 / 参考字号 44px，shader 内缩放距离与 AA 带宽）。
FontSSBOLayout::FontSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("font");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        makeMaterialVectorField("color0", offsetof(UIInstanceData, color0), SSBOValueSource::MaterialVector4, "fontColor"),
        uiSlotField("color1", offsetof(UIInstanceData, color1), defaultColorSlot()),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), alphaGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr),
                    {materialFloat("sdfScale"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)}),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
