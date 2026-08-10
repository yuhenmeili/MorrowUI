#include "DefaultImageSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

DefaultImageSSBOLayout::DefaultImageSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData1Attr>("default_image");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData1Attr, model)),
        makePackedVector4Field("defaultAttr", defaultBatchAttributeOffset<DefaultBatchData1Attr>(0),
                               {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
