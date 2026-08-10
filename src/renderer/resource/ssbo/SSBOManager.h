#pragma once

#include <string>

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
};

}  // namespace morrow
