//
// Created by 0060328 on 25-10-21.
//

#include "ShaderStorageBuffer.h"
#include "GlobalObject.h"
#include "RenderDeviceProxy.h"

namespace morrow {
ShaderStorageBuffer::ShaderStorageBuffer() {
}

void ShaderStorageBuffer::resize(size_t size) {
    if (m_ssbo == nullptr) {
        m_ssbo = RENDERINGTHREAD->createSSBO();
    }
    auto* pool = RENDERINGTHREAD->getSSBODataRecyclePool();
    m_ssboData = pool ? pool->acquire() : std::make_shared<SSBOData>();
    m_ssboData->data.resize(size);
    m_ssboData->size = static_cast<uint32_t>(size);
}

void ShaderStorageBuffer::update() {
    RENDERINGTHREAD->updateSSBO(m_ssbo, m_ssboData, 0);
}

void* ShaderStorageBuffer::getDataPtr() const {
    return m_ssboData->data.data();
}
} // morrow