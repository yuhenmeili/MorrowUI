#include "BounceSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// color1 = (meshCenter.xyz, alpha)，extraAttr = (timeDelta, duration, bounceTimes, scaleRange)。
BounceSSBOLayout::BounceSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("bounce");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        uiSlotField("color0", offsetof(UIInstanceData, color0), defaultColorSlot()),
        uiSlotField("color1", offsetof(UIInstanceData, color1),
                    {materialVectorComponent("meshCenter", 0), materialVectorComponent("meshCenter", 1),
                     materialVectorComponent("meshCenter", 2), materialFloat("alpha")}),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), defaultGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr), zeroSlot()),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr),
                    {materialFloat("timeDelta"), materialFloat("duration"), materialFloat("bounceTimes"), materialFloat("scaleRange")})};
}

}  // namespace morrow
