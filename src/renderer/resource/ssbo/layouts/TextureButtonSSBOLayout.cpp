#include "TextureButtonSSBOLayout.h"

#include <cstddef>

#include "../SSBOLayoutBuilder.h"

namespace morrow {

TextureButtonSSBOLayout::TextureButtonSSBOLayout() {
    m_layout = makeLayout<DefaultBatchData1Attr>("texture_button");
    m_layout.fields = {
        makeWorldMatrixField("model", offsetof(DefaultBatchData1Attr, model)),
        makePackedVector4Field("defaultAttr", defaultBatchAttributeOffset<DefaultBatchData1Attr>(0),
                               {materialFloat("alpha"), constantFloat(0.0f),
                                constantFloat(0.0f), constantFloat(0.0f)})};
}

}  // namespace morrow
