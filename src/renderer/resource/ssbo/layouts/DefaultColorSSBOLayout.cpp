#include "DefaultColorSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

DefaultColorSSBOLayout::DefaultColorSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData2Attr>("default_color");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
        makeMaterialVectorField("defaultColor", offsetof(DefaultBatchData2Attr, attr1), SSBOValueSource::MaterialVector4, "color"),
        makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData2Attr, attr2),
                               {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
