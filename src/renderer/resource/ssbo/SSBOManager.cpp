//
// Created by 0060328 on 25-10-23.
//

#include "SSBOManager.h"

#include <cstdint>

#include "Log.h"
#include "SSBOFieldBinding.h"

namespace morrow {

void SSBOManager::updateSSBO(RenderBatch& batch) {
    if (!batch.ssboLayout) {
        LOG_E("No SSBO layout selected for shader '{}', skip SSBO update", batch.shaderName);
        return;
    }
    updateSSBOForShader(batch.shaderName, batch);
}

void SSBOManager::updateSSBOForShader(const std::string& shaderName, RenderBatch& batch) {
    if (batch.ssbo == nullptr) {
        LOG_E("{} batch SSBO is null", shaderName);
        return;
    }

    const SSBOLayout* layout = getLayout(*batch.ssboLayout);
    if (!layout->isValid()) {
        LOG_E("Invalid SSBO layout for shader '{}': elementSize={}, fields={}, filler={}", shaderName, layout->elementSize, layout->fields.size(),
              static_cast<bool>(layout->filler));
        return;
    }

    const auto batchSize = batch.materials.size();
    if (batch.transforms.size() != batchSize || batch.meshFilters.size() != batchSize) {
        LOG_E("Batch array length mismatch for shader '{}': materials={}, transforms={}, meshFilters={}", shaderName, batchSize, batch.transforms.size(), batch.meshFilters.size());
        return;
    }

    batch.ssbo->resize(batchSize * layout->elementSize);
    auto ptr = static_cast<uint8_t*>(batch.ssbo->getDataPtr());
    for (size_t i = 0; i < batchSize; i++) {
        void* instanceData = ptr + i * layout->elementSize;
        fillSSBOInstance(*layout, instanceData, batch, i);
    }
    batch.ssbo->update();
}

}  // namespace morrow
