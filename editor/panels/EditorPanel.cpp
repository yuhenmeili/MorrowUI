#include "panels/EditorPanel.h"

#include "morrow/base/UIWidget.h"

namespace morrow::editor {

bool EditorPanel::contains(float x, float y) const {
    return m_root && m_root->getWorldSpaceAABB().Contains(x, y);
}

}
