//
// Created by 0060328 on 25-10-23.
//

#include "SSBOManager.h"

#include "GlobalObject.h"
#include "Log.h"
#include "Material.h"
#include "SSBOFieldBinding.h"
#include "ShaderReflection.h"

namespace morrow {
// SSBO block 名（所有内建 shader 统一）
constexpr const char* kSSBOBlockName = "InstanceBuffer";

void SSBOManager::updateSSBO(RenderBatch& batch) {
    if (!batch.ssboLayout) {
        LOG_E("No SSBO layout selected for shader '{}', skip SSBO update", batch.shaderName);
        return;
    }
    updateSSBOForShader(batch.shaderName, batch);
}

// ---------------------------------------------------------------------------
// P3：shader 首次使用时执行一次 SSBO reflection 校验并缓存结果
// ---------------------------------------------------------------------------
void SSBOManager::validateReflectionOnce(const std::string& shaderName, const SSBOLayout& layout, const RenderBatch& batch) {
    if (batch.materials.empty()) {
        return;
    }
    const HwGPUProgram program = batch.materials[0]->getBatchShader();
    if (!program.isValid()) {
        LOG_W("SSBO reflection skipped for shader '{}': batch shader invalid", shaderName);
        return;
    }

    // 已对同一 program 校验过则跳过；shader hot reload（新 program id）会重新校验
    auto cached = m_reflectionValidated.find(shaderName);
    if (cached != m_reflectionValidated.end() && cached->second == program.id) {
        return;
    }
    m_reflectionValidated[shaderName] = program.id;

    const auto reflected = RENDERINGTHREAD->reflectSSBOBlock(program, kSSBOBlockName);
    if (!reflected.valid) {
        LOG_W("SSBO reflection unavailable for shader '{}', skip layout validation", shaderName);
        return;
    }

    const std::string error = validateSSBOLayout(layout, reflected);
    if (!error.empty()) {
        LOG_E("SSBO layout mismatch for shader '{}': {}", shaderName, error);
    }
}

void SSBOManager::updateSSBOForShader(const std::string& shaderName, RenderBatch& batch) {
    // P0：SSBO 对象缺失时安全返回
    if (batch.ssbo == nullptr) {
        LOG_E("{} batch SSBO is null", shaderName);
        return;
    }

    // P0：未注册 shader 明确报错并停止 SSBO 更新
    const SSBOLayout* layout = getLayout(*batch.ssboLayout);

    // P0：验证 layout 有效性（elementSize > 0 且 fields/filler 有效）
    if (!layout->isValid()) {
        LOG_E("Invalid SSBO layout for shader '{}': elementSize={}, fields={}, filler={}", shaderName, layout->elementSize, layout->fields.size(),
              static_cast<bool>(layout->filler));
        return;
    }

    // P0：验证 batch 各数组长度一致，避免填充时越界
    const auto batchSize = batch.materials.size();
    if (batch.transforms.size() != batchSize || batch.meshFilters.size() != batchSize) {
        LOG_E("Batch array length mismatch for shader '{}': materials={}, transforms={}, meshFilters={}", shaderName, batchSize, batch.transforms.size(), batch.meshFilters.size());
        return;
    }

    // P3：首次遇到该 shader 时反射校验 CPU/GLSL 布局一致性（结果缓存）
    validateReflectionOnce(shaderName, *layout, batch);

    // 准备SSBO数据缓冲区
    batch.ssbo->resize(batchSize * layout->elementSize);
    auto ptr = static_cast<uint8_t*>(batch.ssbo->getDataPtr());
    // 为每个实例填充数据（P2：声明式字段绑定 + custom filler）
    for (size_t i = 0; i < batchSize; i++) {
        void* instanceData = ptr + i * layout->elementSize;
        fillSSBOInstance(*layout, instanceData, batch, i);
    }
    // 更新SSBO到GPU
    batch.ssbo->update();
}
} // namespace morrow
