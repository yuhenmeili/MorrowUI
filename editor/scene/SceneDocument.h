#ifndef MORROW_EDITOR_SCENE_DOCUMENT_H
#define MORROW_EDITOR_SCENE_DOCUMENT_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace morrow::editor {
struct SceneResourceRecord {
    std::string id;
    std::string assetId;
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
    static bool loadFromFile(const std::filesystem::path& path, SceneDocument& document, std::string& error);

    bool saveToFile(const std::filesystem::path& path, std::string& error) const;

    SceneNodeRecord* findNode(const std::string& id);

    const SceneNodeRecord* findNode(const std::string& id) const;

    const SceneResourceRecord* findExternalResource(const std::string& id) const;

    const SceneSubResourceRecord* findSubResource(const std::string& id) const;

    bool setNodeProperty(const std::string& nodeId, const std::string& property, std::string value, std::string& error);

    bool removeNodeProperty(const std::string& nodeId, const std::string& property, std::string& error);

    bool reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error);

    bool renameNode(const std::string& nodeId, std::string name, std::string& error);

    bool addNode(SceneNodeRecord node, std::string& error);

    bool removeNodeSubtree(const std::string& nodeId, std::vector<SceneNodeRecord>& removed, std::string& error);

    /// 编辑器锁定标记（节点属性 locked == "true"），锁定的节点在视口不可点选。
    bool isNodeLocked(const std::string& nodeId) const;

    /// 视口参考线（画布坐标系），随场景保存到 [guides] 段。
    const std::vector<float>& guideXs() const;

    const std::vector<float>& guideYs() const;

    void addGuideX(float value);

    void addGuideY(float value);

    void removeGuideX(float value);

    void removeGuideY(float value);

    void clearGuides();

    const std::vector<SceneResourceRecord>& externalResources() const;

    const std::vector<SceneSubResourceRecord>& subResources() const;

    const std::vector<SceneNodeRecord>& nodes() const;

private:
    std::vector<SceneResourceRecord> m_externalResources;
    std::vector<SceneSubResourceRecord> m_subResources;
    std::vector<SceneNodeRecord> m_nodes;
    std::vector<float> m_guideXs;
    std::vector<float> m_guideYs;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_SCENE_DOCUMENT_H