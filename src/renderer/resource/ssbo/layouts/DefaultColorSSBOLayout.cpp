#include "DefaultColorSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

DefaultColorSSBOLayout::DefaultColorSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData2Attr>("default_color");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
        makeMaterialVectorField("defaultColor", defaultBatchAttributeOffset<DefaultBatchData2Attr>(0), SSBOValueSource::MaterialVector4, "color"),
        makePackedVector4Field("defaultAttr", defaultBatchAttributeOffset<DefaultBatchData2Attr>(1),
                               {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1),
                                materialFloat("rounding"), materialFloat("alpha")})};
}

}  // namespace morrow
