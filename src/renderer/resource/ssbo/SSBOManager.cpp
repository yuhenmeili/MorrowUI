//
// Created by 0060328 on 25-10-23.
//

#include "SSBOManager.h"

#include <cstdint>

#include "morrow/Material.h"
#include "morrow/utils/Log.h"
#include "ssbo/SSBOFieldBinding.h"

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

    validateBatchParamBindings(shaderName, *layout, batch);

    batch.ssbo->resize(batchSize * layout->elementSize);
    auto ptr = static_cast<uint8_t*>(batch.ssbo->getDataPtr());
    for (size_t i = 0; i < batchSize; i++) {
        void* instanceData = ptr + i * layout->elementSize;
        fillSSBOInstance(*layout, instanceData, batch, i);
    }
    batch.ssbo->update();
}

void SSBOManager::validateBatchParamBindings(const std::string& shaderName, const SSBOLayout& layout, const RenderBatch& batch) {
    if (batch.materials.size() <= 1 || layout.fields.empty()) {
        return;
    }

    auto cached = m_consumedPropertiesCache.find(&layout);
    if (cached == m_consumedPropertiesCache.end()) {
        cached = m_consumedPropertiesCache.emplace(&layout, std::unordered_set<std::string>()).first;
        collectLayoutConsumedProperties(layout, cached->second);
    }
    const std::unordered_set<std::string>& consumed = cached->second;

    for (const auto& material : batch.materials) {
        if (!material) {
            continue;
        }
        m_paramNamesScratch.clear();
        material->collectParamNames(m_paramNamesScratch);
        for (const auto& name : m_paramNamesScratch) {
            if (consumed.find(name) != consumed.end()) {
                continue;
            }
            ++m_droppedParamHits;
            std::string key = layout.name;
            key += '|';
            key += name;
            if (m_warnedParamKeys.insert(std::move(key)).second) {
                LOG_W("SSBO batch '{}' ({} instances): material param '{}' is not bound by layout '{}' "
                      "and is silently dropped in the merged SSBO path; route it through an SSBO slot "
                      "(ARCHITECTURE.md invariant 22)",
                      shaderName, batch.materials.size(), name, layout.name);
            }
        }
    }
}

}  // namespace morrow
