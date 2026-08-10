#include "BounceSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

BounceSSBOLayout::BounceSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData2Attr>("bounce");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
        makePackedVector4Field("meshCenter", defaultBatchAttributeOffset<DefaultBatchData2Attr>(0),
                               {materialVectorComponent("meshCenter", 0), materialVectorComponent("meshCenter", 1),
                                materialVectorComponent("meshCenter", 2), materialFloat("alpha")}),
        makePackedVector4Field("defaultAttr", defaultBatchAttributeOffset<DefaultBatchData2Attr>(1),
                               {materialFloat("timeDelta"), materialFloat("duration"), materialFloat("bounceTimes"),
                                materialFloat("scaleRange")})};
}

}  // namespace morrow
