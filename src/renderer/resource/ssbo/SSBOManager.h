#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

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

    void validateReflectionOnce(const std::string& shaderName,
                                const SSBOLayout& layout,
                                const RenderBatch& batch);

    std::unordered_map<std::string, uint32_t> m_reflectionValidated;
};

}  // namespace morrow
