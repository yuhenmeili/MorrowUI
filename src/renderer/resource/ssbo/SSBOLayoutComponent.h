#pragma once

#include "ShaderStorageBuffer.h"

namespace morrow {

class SSBOLayoutComponent {
public:
    virtual ~SSBOLayoutComponent() = default;

    const char* getName() const {
        return m_layout.name.c_str();
    }

    const SSBOLayout& getLayout() const {
        return m_layout;
    }

protected:
    SSBOLayout m_layout;
};

}  // namespace morrow
