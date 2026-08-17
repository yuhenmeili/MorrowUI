#include "ProgressBarSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

ProgressBarSSBOLayout::ProgressBarSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData<4>>("progress_bar");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData<4>, model)),
        makeMaterialVectorField("trackColor", defaultBatchAttributeOffset<DefaultBatchData<4>>(0), SSBOValueSource::MaterialVector4, "trackColor"),
        makeMaterialVectorField("fillColor", defaultBatchAttributeOffset<DefaultBatchData<4>>(1), SSBOValueSource::MaterialVector4, "fillColor"),
        makePackedVector4Field("defaultAttr", defaultBatchAttributeOffset<DefaultBatchData<4>>(2),
                               {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1),
                                materialFloat("rounding"), materialFloat("alpha")}),
        makePackedVector4Field("stateAttr", defaultBatchAttributeOffset<DefaultBatchData<4>>(3),
                               {materialFloat("progress"), materialFloat("direction"),
                                materialFloat("useTrackTexture"), materialFloat("useFillTexture")})};
}

}  // namespace morrow
