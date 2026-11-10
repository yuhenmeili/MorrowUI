#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SSBOLayoutComponent.h"

namespace morrow {

class SSBOManager {
public:
    const SSBOLayout* getLayout(const SSBOLayoutComponent& component) const {
        return &component.getLayout();
    }

    void updateSSBO(RenderBatch& batch);

private:
    void updateSSBOForShader(const std::string& shaderName, RenderBatch& batch);

    // ── 参数绑定护栏（ARCHITECTURE.md 不变量 22）──
    // 合并 SSBO 批只消费绑定表字段与纹理；材质中未登记的参数会被静默丢弃。
    // 逐帧检测合并批内各材质的参数名与绑定表的差集，违规按 (layout, 参数)
    // 限流输出一次 LOG_W。仅主线程（commitRenderPass）访问。
    void validateBatchParamBindings(const std::string& shaderName, const SSBOLayout& layout, const RenderBatch& batch);

    // layout 静态注册、指针稳定，消费集合按 layout 指针缓存一次
    std::unordered_map<const SSBOLayout*, std::unordered_set<std::string>> m_consumedPropertiesCache;
    std::vector<std::string> m_paramNamesScratch;
    std::unordered_set<std::string> m_warnedParamKeys;
    uint64_t m_droppedParamHits = 0;
};

}  // namespace morrow
