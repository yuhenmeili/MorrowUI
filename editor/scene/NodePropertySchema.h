#ifndef MORROW_EDITOR_NODE_PROPERTY_SCHEMA_H
#define MORROW_EDITOR_NODE_PROPERTY_SCHEMA_H

#include <string>
#include <utility>
#include <vector>

namespace morrow::editor {

/// 节点属性描述符：Inspector 展示、默认值（重置按钮）与分组标题的单一事实源。
/// 序列化/实例化侧继续按属性名分发（SceneInstantiator），本表只负责"编辑器怎么看"。
struct PropertyDescriptor {
    std::string name;
    std::string type;         // bool / number / string / Vector2 / Vector3 / Vector4 / Color / enum / TextureAsset / MaterialAsset
    std::string defaultValue; // 序列化形态；空 = 无已知默认（不显示重置按钮）
    std::string component;    // Inspector 分组（Transform / Layout / Mesh Filter / Mesh Renderer / Properties）
    std::string displayName;  // 显示名；空 = 由 name 推导
    bool editable = true;
    /// number 属性的取值范围：Inspector 渲染为滑块+数值输入。
    bool hasRange = false;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float stepValue = 1.0f;
    /// enum 属性的取值表：(序列化值, 显示名)。Inspector 渲染为下拉。
    std::vector<std::pair<std::string, std::string>> enumOptions;
};

/// 所有可编辑节点共享的基础属性（Transform + 可见性 + 锁定）。
const std::vector<PropertyDescriptor>& commonProperties(bool sceneNode3D);

/// 渲染组件属性（mesh/renderer/material 等，仅非 SceneNode3D 节点）。
const std::vector<PropertyDescriptor>& meshRendererProperties();

/// 节点类型专属属性（当前为 SceneInstantiator 已支持的最小集，后续按控件扩展）。
const std::vector<PropertyDescriptor>* typeProperties(const std::string& nodeType);

/// 查单个属性的描述符（common + type 合并查找）；找不到返回 nullptr。
const PropertyDescriptor* findProperty(const std::string& nodeType, bool sceneNode3D, const std::string& name);

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_NODE_PROPERTY_SCHEMA_H
