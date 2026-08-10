#include "ImageSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

ImageSSBOLayout::ImageSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData2Attr>("image_normal");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
        makePackedVector4Field("displaySize", defaultBatchAttributeOffset<DefaultBatchData2Attr>(0),
                               {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1),
                                materialVectorComponent("displaySize", 2), constantFloat(0.0f)}),
        makePackedVector4Field("imageAttr", defaultBatchAttributeOffset<DefaultBatchData2Attr>(1),
                               {materialFloat("rounding"), materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
