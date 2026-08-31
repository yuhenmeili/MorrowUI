#include "panels/EditorPanel.h"

#include "base/UIWidget.h"

namespace morrow::editor {

bool EditorPanel::contains(float x, float y) const {
    return m_root && m_root->getScreenSpaceAABB().Contains(x, y);
}

}
