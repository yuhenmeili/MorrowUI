#include "FontSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

FontSSBOLayout::FontSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData2Attr>("font");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
        makeMaterialVectorField("fontColor", defaultBatchAttributeOffset<DefaultBatchData2Attr>(0), SSBOValueSource::MaterialVector4, "fontColor"),
        makePackedVector4Field("fontAttr", defaultBatchAttributeOffset<DefaultBatchData2Attr>(1),
                               {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
