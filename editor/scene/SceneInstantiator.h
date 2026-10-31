#ifndef MORROW_EDITOR_SCENE_INSTANTIATOR_H
#define MORROW_EDITOR_SCENE_INSTANTIATOR_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace morrow {
class Widget;

namespace editor {
class SceneDocument;
class ThemeAsset;
class AssetDatabase;
struct SceneNodeRecord;

using SceneInstanceMap = std::unordered_map<std::string, std::shared_ptr<Widget>>;

class SceneInstantiator {
public:
    /// instanceChain：当前场景实例化链（规范路径），用于阻止 A→B→A 递归实例化。
    /// theme：项目默认主题，节点未显式设置的属性先取主题默认值再应用。
    static bool instantiate(const SceneDocument& document, const std::shared_ptr<Widget>& stage, const AssetDatabase* assets, std::string& error,
                            SceneInstanceMap* instancesOut = nullptr, const std::vector<std::filesystem::path>* instanceChain = nullptr,
                            const ThemeAsset* theme = nullptr);

    static bool updateNode(const SceneDocument& document, const SceneNodeRecord& record, const std::shared_ptr<Widget>& instance, const AssetDatabase* assets, std::string& error);

    static bool updateNodeTransform(const SceneNodeRecord& record, const std::shared_ptr<Widget>& instance, std::string& error);
};
} // namespace editor
} // namespace morrow

#endif  // MORROW_EDITOR_SCENE_INSTANTIATOR_H