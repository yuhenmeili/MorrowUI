#include "ButtonSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

ButtonSSBOLayout::ButtonSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData3Attr>("button");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData3Attr, model)),
        makeMaterialVectorField("bgColor", offsetof(DefaultBatchData3Attr, attr1), SSBOValueSource::MaterialVector4, "color"),
        makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData3Attr, attr2),
                               {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1),
                                materialFloat("rounding"), materialFloat("alpha")}),
        makePackedVector4Field("textureAttr", offsetof(DefaultBatchData3Attr, attr3),
                               {materialFloat("useTexture"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
