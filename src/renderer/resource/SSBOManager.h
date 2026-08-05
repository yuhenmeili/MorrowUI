//
// Created by 0060328 on 25-10-23.
//

#ifndef SSBOMANAGER_H
#define SSBOMANAGER_H
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "ShaderStorageBuffer.h"

namespace morrow {

// P1：以 shader -> layout factory 注册表替代 if/else 注册链。
// 别名 shader 可共享同一个 factory，layout 首次访问时构建并缓存。
class SSBOManager {
public:
    using SSBOLayoutFactory = std::function<SSBOLayout()>;

    SSBOManager();

    // 为指定Shader更新SSBO数据；
    // P0：未注册或 layout 无效或 batch 数组长度不一致时明确报错并安全返回。
    void updateSSBOForShader(const std::string& shaderName, RenderBatch& batch);

    bool hasLayout(const std::string& shaderName) const;

    // 只读访问已注册 layout（测试 / 调试用），未注册返回 nullptr
    const SSBOLayout* getLayout(const std::string& shaderName) const;

private:
    // 注册 shader -> layout factory
    void registerLayout(const std::string& shaderName, SSBOLayoutFactory factory);

    // 首次访问时构建并缓存 layout；未注册返回 nullptr
    const SSBOLayout* findLayout(const std::string& shaderName) const;

    // P3：shader 首次使用时执行一次 SSBO reflection 校验并缓存结果
    void validateReflectionOnce(const std::string& shaderName, const SSBOLayout& layout, const RenderBatch& batch);

    std::unordered_map<std::string, SSBOLayoutFactory> m_layoutFactories;
    mutable std::unordered_map<std::string, SSBOLayout> m_shaderLayouts;
    // P3：已执行过 reflection 校验的 shader 集合（shaderName -> program id）。
    // 缓存 program id，shader hot reload（新 program）时自动重新校验。
    std::unordered_map<std::string, uint32_t> m_reflectionValidated;
};
} // morrow

#endif //SSBOMANAGER_H
