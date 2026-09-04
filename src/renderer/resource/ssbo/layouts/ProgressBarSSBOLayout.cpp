#include "ProgressBarSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

// color0 = trackColor，color1 = fillColor，
// geomAttr = (displaySize.x, displaySize.y, rounding, alpha)，
// stateAttr = (progress, direction, useTrackTexture, useFillTexture)。
ProgressBarSSBOLayout::ProgressBarSSBOLayout() {
    m_layout = makeLayout<UIInstanceData>("progress_bar");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(UIInstanceData, model)),
        makeMaterialVectorField("color0", offsetof(UIInstanceData, color0), SSBOValueSource::MaterialVector4, "trackColor"),
        makeMaterialVectorField("color1", offsetof(UIInstanceData, color1), SSBOValueSource::MaterialVector4, "fillColor"),
        uiSlotField("geomAttr", offsetof(UIInstanceData, geomAttr), displayGeomSlot()),
        uiSlotField("stateAttr", offsetof(UIInstanceData, stateAttr),
                    {materialFloat("progress"), materialFloat("direction"), materialFloat("useTrackTexture"), materialFloat("useFillTexture")}),
        uiSlotField("extraAttr", offsetof(UIInstanceData, extraAttr), zeroSlot())};
}

}  // namespace morrow
