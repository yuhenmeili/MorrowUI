//
// Created by 0060328 on 25-10-21.
//

#include "UniformBuffer.h"
#include "FrameState.h"
#include "GlobalObject.h"
#include "OrthographicCamera.h"
#include "RenderDeviceProxy.h"

#include <cstring>

namespace morrow {
UniformBuffer::UniformBuffer() {
}

UniformBuffer::~UniformBuffer() {
    if (m_globalUBO.isValid()) {
        //TODO release ubo
    }
}

void UniformBuffer::update(std::shared_ptr<FrameState> frameState) {
    if (!m_globalUBO.isValid()) {
        m_globalUBO = RENDERINGTHREAD->createUBO();
    }
    auto pV = frameState->camera->getProjectionView();
    if (m_projectionView == pV) {
        // LOG_I("ProjectionView matrix is not changed, skip UBO update");
    } else {
        m_projectionView.copy(pV);

        auto* pool = RENDERINGTHREAD->getUBODataRecyclePool();
        auto globalUBOData = pool ? pool->acquire() : std::make_shared<UBOData>();
        globalUBOData->size = sizeof(Matrix4);
        globalUBOData->data.resize(sizeof(Matrix4));
        std::memcpy(globalUBOData->data.data(), pV.elements, sizeof(Matrix4));
        RENDERINGTHREAD->updateUBO(m_globalUBO, globalUBOData);
    }
}

void UniformBuffer::bind(HwGPUProgram shader) {
    RENDERINGTHREAD->bindUBO(shader, m_globalUBO, "Global", 0);
}
} // morrow
