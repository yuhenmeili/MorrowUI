#ifndef MORROW_EDITOR_NODE_TYPE_CATALOG_H
#define MORROW_EDITOR_NODE_TYPE_CATALOG_H

#include <map>
#include <string>
#include <vector>

#include "SceneDocument.h"

namespace morrow::editor {
struct NodeTypeDescriptor {
    std::string type;
    std::string displayName;
    std::string category;
    std::string description;
    std::map<std::string, std::string> defaultProperties;
};

class NodeTypeCatalog {
public:
    NodeTypeCatalog();

    const std::vector<NodeTypeDescriptor>& types() const;

    const NodeTypeDescriptor* find(const std::string& type) const;

    std::vector<const NodeTypeDescriptor*> filter(const std::string& query) const;

    SceneNodeRecord createNode(const NodeTypeDescriptor& descriptor, const std::string& parentId, const SceneDocument& document) const;

private:
    std::vector<NodeTypeDescriptor> m_types;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_NODE_TYPE_CATALOG_H