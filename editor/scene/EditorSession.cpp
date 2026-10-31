#include "EditorSession.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <set>

namespace {

bool parseVector(const std::string& value, const std::string& type, std::vector<float>& components) {
    if (value.compare(0, type.size() + 1, type + "(") != 0 || value.empty() || value.back() != ')') {
        return false;
    }
    std::string body = value.substr(type.size() + 1, value.size() - type.size() - 2);
    size_t start = 0;
    while (start <= body.size()) {
        const auto separator = body.find(',', start);
        const auto part = body.substr(start, separator == std::string::npos ? std::string::npos : separator - start);
        try {
            components.push_back(std::stof(part));
        } catch (...) {
            return false;
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    return true;
}

std::string vector2(float x, float y) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Vector2(%.3f, %.3f)", x, y);
    return buffer;
}

std::string vector3(float x, float y, float z) {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Vector3(%.3f, %.3f, %.3f)", x, y, z);
    return buffer;
}

}  // namespace

namespace morrow::editor {

namespace {

std::string nextAvailableNodeId(const SceneDocument& document, std::set<std::string>& reserved) {
    for (int index = 1;; ++index) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "node_%04d", index);
        if (!document.findNode(buffer) && reserved.insert(buffer).second)
            return buffer;
    }
}

void collectSubtree(const SceneDocument& document, const std::string& rootId, std::vector<SceneNodeRecord>& out) {
    const auto* node = document.findNode(rootId);
    if (!node)
        return;
    out.push_back(*node);
    for (const auto& child : document.nodes()) {
        if (child.parentId == rootId)
            collectSubtree(document, child.id, out);
    }
}

/// 选中集中没有祖先也被选中的节点，即待复制/删除的子树根。
std::vector<std::string> selectionRoots(const SceneDocument& document, const std::vector<std::string>& selection) {
    std::vector<std::string> roots;
    for (const auto& nodeId : selection) {
        bool hasSelectedAncestor = false;
        for (auto* node = document.findNode(nodeId); node && !node->parentId.empty(); node = document.findNode(node->parentId)) {
            if (std::find(selection.begin(), selection.end(), node->parentId) != selection.end()) {
                hasSelectedAncestor = true;
                break;
            }
        }
        if (!hasSelectedAncestor)
            roots.push_back(nodeId);
    }
    return roots;
}

/// 平移子树根的 position 属性；无 position 的根（如 SceneNode3D）保持不动。
void offsetNodePosition(SceneNodeRecord& node, float dx, float dy) {
    const auto position = node.properties.find("position");
    std::vector<float> values;
    if (position == node.properties.end() || !parseVector(position->second, "Vector3", values) || values.size() != 3)
        return;
    node.properties["position"] = vector3(values[0] + dx, values[1] + dy, values[2]);
}

}  // namespace

EditorSession::EditorSession(std::filesystem::path scenePath) : m_scenePath(std::move(scenePath)), m_model(m_document) {
}

bool EditorSession::load(std::string& error) {
    if (!SceneDocument::loadFromFile(m_scenePath, m_document, error)) {
        return false;
    }
    m_history.clear();
    m_dirty = false;
    return true;
}

bool EditorSession::save(std::string& error) {
    if (!m_document.saveToFile(m_scenePath, error))
        return false;
    m_dirty = false;
    return true;
}

SceneDocument& EditorSession::document() {
    return m_document;
}

const SceneDocument& EditorSession::document() const {
    return m_document;
}

SceneEditorModel& EditorSession::model() {
    return m_model;
}

const SceneEditorModel& EditorSession::model() const {
    return m_model;
}

bool EditorSession::selectNode(const std::string& nodeId, bool additive, std::string& error) {
    return m_model.selectNode(nodeId, additive, error);
}

bool EditorSession::selectAt(float x, float y, std::string& error, bool additive) {
    const auto& nodes = m_document.nodes();
    for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); ++iterator) {
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeZ = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!m_model.nodeWorldRect(iterator->id, nodeX, nodeY, nodeZ, nodeWidth, nodeHeight)) {
            continue;
        }
        if (x >= nodeX && y >= nodeY && x <= nodeX + nodeWidth && y <= nodeY + nodeHeight) {
            return m_model.selectNode(iterator->id, additive, error);
        }
    }
    error = "no selectable 2D node at the requested position";
    return false;
}

bool EditorSession::moveSelection(float dx, float dy, bool continuous, std::string& error) {
    // 只移动带合法 Vector3 position 的选中节点；整组一条命令，可连续合并
    std::vector<std::string> nodeIds;
    for (const auto& nodeId : m_model.selection().nodeIds) {
        const auto node = m_document.findNode(nodeId);
        const auto position = node ? node->properties.find("position") : std::map<std::string, std::string>::const_iterator{};
        std::vector<float> values;
        if (node && position != node->properties.end() && parseVector(position->second, "Vector3", values) && values.size() == 3)
            nodeIds.push_back(nodeId);
    }
    if (nodeIds.empty()) {
        error = "selection has no movable nodes";
        return false;
    }
    auto command = std::make_unique<MoveSelectionCommand>(std::move(nodeIds), dx, dy);
    const bool changed = continuous ? m_history.executeOrMerge(std::move(command), m_document, error) : m_history.execute(std::move(command), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::setProperty(const std::string& nodeId, const std::string& property, std::string value, bool continuous, std::string& error) {
    auto command = std::make_unique<SetNodePropertyCommand>(nodeId, property, std::move(value));
    const bool changed = continuous ? m_history.executeOrMerge(std::move(command), m_document, error) : m_history.execute(std::move(command), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::addNode(SceneNodeRecord node, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<AddNodeCommand>(std::move(node)), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::deleteNode(const std::string& nodeId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<DeleteNodeCommand>(nodeId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::duplicateNode(const std::string& nodeId, const std::string& duplicateId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<DuplicateNodeCommand>(nodeId, duplicateId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::duplicateSelection(std::string& error) {
    const auto roots = selectionRoots(m_document, m_model.selection().nodeIds);
    if (roots.empty()) {
        error = "select a scene node to duplicate";
        return false;
    }
    std::vector<std::unique_ptr<SceneCommand>> commands;
    std::vector<std::string> duplicateRootIds;
    std::set<std::string> usedRootIds;
    for (const auto& rootId : roots) {
        // 根副本 id 用 "_copy" 后缀空间，与 DuplicateNodeCommand 内部生成的
        // node_NNNN 后代 id 空间隔离，宏内多条命令不会互相抢占 id
        std::string duplicateId = rootId + "_copy";
        for (int suffix = 2; m_document.findNode(duplicateId) || usedRootIds.count(duplicateId) != 0; ++suffix)
            duplicateId = rootId + "_copy" + std::to_string(suffix);
        usedRootIds.insert(duplicateId);
        commands.push_back(std::make_unique<DuplicateNodeCommand>(rootId, duplicateId));
        duplicateRootIds.push_back(duplicateId);
    }
    const bool changed = m_history.execute(std::make_unique<MacroCommand>(std::move(commands)), m_document, error);
    if (!changed)
        return false;
    m_dirty = true;
    m_model.clearSelection();
    for (const auto& duplicateId : duplicateRootIds)
        m_model.selectNode(duplicateId, true, error);
    return true;
}

bool EditorSession::copySelection(std::string& error) {
    const auto roots = selectionRoots(m_document, m_model.selection().nodeIds);
    if (roots.empty()) {
        error = "select a scene node to copy";
        return false;
    }
    m_clipboard.clear();
    for (const auto& rootId : roots)
        collectSubtree(m_document, rootId, m_clipboard);
    return true;
}

bool EditorSession::cutSelection(std::string& error) {
    if (!copySelection(error))
        return false;
    const auto roots = selectionRoots(m_document, m_model.selection().nodeIds);
    std::vector<std::unique_ptr<SceneCommand>> commands;
    for (const auto& rootId : roots)
        commands.push_back(std::make_unique<DeleteNodeCommand>(rootId));
    const bool changed = m_history.execute(std::make_unique<MacroCommand>(std::move(commands)), m_document, error);
    if (!changed)
        return false;
    m_dirty = true;
    m_model.clearSelection();
    return true;
}

bool EditorSession::pasteClipboard(std::vector<std::string>& pastedRootIds, std::string& error) {
    pastedRootIds.clear();
    if (m_clipboard.empty()) {
        error = "clipboard is empty";
        return false;
    }
    // 粘贴目标：当前选中节点；无选中则退回场景根
    std::string parentId;
    const auto& selection = m_model.selection().nodeIds;
    if (!selection.empty() && m_document.findNode(selection.back()))
        parentId = selection.back();
    else
        for (const auto& node : m_document.nodes())
            if (node.parentId.empty()) {
                parentId = node.id;
                break;
            }

    std::map<std::string, std::string> idMap;
    std::set<std::string> reservedIds;
    std::vector<SceneNodeRecord> nodes;
    for (const auto& source : m_clipboard) {
        SceneNodeRecord copy = source;
        copy.id = nextAvailableNodeId(m_document, reservedIds);
        idMap[source.id] = copy.id;
        if (!copy.parentId.empty()) {
            const auto mappedParent = idMap.find(copy.parentId);
            copy.parentId = mappedParent != idMap.end() ? mappedParent->second : parentId;
        } else {
            copy.parentId = parentId;
        }
        // 子树根（剪贴板中没有其父记录的顶层节点）偏移粘贴，避免与原节点完全重叠
        const bool isRoot = std::none_of(m_clipboard.begin(), m_clipboard.end(),
                                         [&source](const SceneNodeRecord& candidate) { return candidate.id == source.parentId; });
        if (isRoot) {
            offsetNodePosition(copy, 8.0f, 8.0f);
            pastedRootIds.push_back(copy.id);
        }
        nodes.push_back(std::move(copy));
    }
    const bool changed = m_history.execute(std::make_unique<AddNodesCommand>(std::move(nodes)), m_document, error);
    if (!changed)
        return false;
    m_dirty = true;
    m_model.clearSelection();
    for (const auto& rootId : pastedRootIds)
        m_model.selectNode(rootId, true, error);
    return true;
}

bool EditorSession::hasClipboard() const {
    return !m_clipboard.empty();
}

bool EditorSession::reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<ReparentNodeCommand>(nodeId, parentId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::renameNode(const std::string& nodeId, std::string name, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<RenameNodeCommand>(nodeId, std::move(name)), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::moveGizmo(const std::string& nodeId, float x, float y, float z, bool continuous, std::string& error) {
    return setProperty(nodeId, "position", vector3(x, y, z), continuous, error);
}

bool EditorSession::resizeGizmo(const std::string& nodeId, float width, float height, bool continuous, std::string& error) {
    return setProperty(nodeId, "size", vector2(width, height), continuous, error);
}

bool EditorSession::rotateGizmo(const std::string& nodeId, float zDegrees, bool continuous, std::string& error) {
    return setProperty(nodeId, "rotation", vector3(0.0f, 0.0f, zDegrees), continuous, error);
}

bool EditorSession::setNodeRect(const std::string& nodeId, float x, float y, float z, float width, float height, bool continuous, std::string& error) {
    auto command = std::make_unique<SetNodeRectCommand>(nodeId, vector3(x, y, z), vector2(width, height));
    const bool changed = continuous ? m_history.executeOrMerge(std::move(command), m_document, error) : m_history.execute(std::move(command), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::undo(std::string& error) {
    const bool changed = m_history.undo(m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::redo(std::string& error) {
    const bool changed = m_history.redo(m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::isDirty() const {
    return m_dirty;
}

void EditorSession::markDirty() {
    m_dirty = true;
}

bool EditorSession::clearGuides(std::string& error) {
    (void)error;
    m_document.clearGuides();
    m_dirty = true;
    return true;
}

}  // namespace morrow::editor
