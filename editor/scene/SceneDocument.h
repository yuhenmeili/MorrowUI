#ifndef MORROW_EDITOR_SCENE_DOCUMENT_H
#define MORROW_EDITOR_SCENE_DOCUMENT_H

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace morrow {
class Widget;

namespace editor {

struct SceneResourceRecord {
    std::string id;
    std::string type;
    std::string path;
};

struct SceneSubResourceRecord {
    std::string id;
    std::string type;
    std::map<std::string, std::string> properties;
    size_t line = 0;
};

struct SceneNodeRecord {
    std::string id;
    std::string type;
    std::string parentId;
    std::string name;
    std::map<std::string, std::string> properties;
    size_t line = 0;
};

class SceneDocument {
public:
    static bool loadFromFile(const std::filesystem::path& path,
                             SceneDocument& document,
                             std::string& error);

    bool instantiate(const std::shared_ptr<Widget>& stage,
                     std::string& error) const;

    const std::vector<SceneResourceRecord>& externalResources() const;

    const std::vector<SceneSubResourceRecord>& subResources() const;

    const std::vector<SceneNodeRecord>& nodes() const;

private:
    std::vector<SceneResourceRecord> m_externalResources;
    std::vector<SceneSubResourceRecord> m_subResources;
    std::vector<SceneNodeRecord> m_nodes;
};

} // namespace editor
} // namespace morrow

#endif // MORROW_EDITOR_SCENE_DOCUMENT_H
