//
// Created by 0060328 on 25-10-23.
//

#include "SSBOManager.h"

#include "GlobalObject.h"
#include "Log.h"
#include "Material.h"
#include "base/Transform.h"

namespace morrow {
void SSBOManager::updateSSBOForShader(const std::string& shaderName, RenderBatch& batch) {
    auto it = m_shaderLayouts.find(shaderName);
    if (it == m_shaderLayouts.end()) {
        SSBOLayout layout = {
            .name = shaderName
        };
        registerSSBOLayout(layout);
        it = m_shaderLayouts.emplace(shaderName, std::move(layout)).first;
    }
    if (batch.ssbo == nullptr) {
        LOG_E("{} batch SSBO is null", shaderName);
        return;
    }
    const auto& layout = it->second;
    auto batchSize = batch.materials.size();
    // 准备SSBO数据缓冲区
    batch.ssbo->resize(batchSize * layout.elementSize);
    auto ptr = static_cast<uint8_t*>(batch.ssbo->getDataPtr());
    // 为每个实例填充数据
    for (int i = 0; i < batchSize; i++) {
        void* instanceData = ptr + i * layout.elementSize;
        layout.filler(instanceData, batch, i);
    }
    // 更新SSBO到GPU
    batch.ssbo->update();
}

bool SSBOManager::hasLayout(const std::string& shaderName) const {
    return m_shaderLayouts.find(shaderName) != m_shaderLayouts.end();
}

// SSBO管理器实现
void SSBOManager::registerSSBOLayout(SSBOLayout& layout) {
    if (layout.name == "default_color") {
        layout.elementSize = sizeof(DefaultBatchData2Attr);
        // 数据填充函数
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData2Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            auto defaultColor = std::get<Vector4>(batch.materials[index]->getVector("color"));
            instanceData->attr1 = defaultColor;
            instanceData->attr2 = Vector4(batch.materials[index]->getFloat("alpha"), 0.0f, 0.0f, 0.0f);
        };
    } else if (layout.name == "default_image" || layout.name == "anchor_point_scale") {
        layout.elementSize = sizeof(DefaultBatchData1Attr);
        // 数据填充函数
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData1Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            instanceData->attr = Vector4(batch.materials[index]->getFloat("alpha"), 0.0f, 0.0f, 0.0f);
        };
    } else if (layout.name == "image_normal" ||
               layout.name == "image_text_debug" ||
               layout.name == "image_oes") {
        layout.elementSize = sizeof(DefaultBatchData2Attr);
        // 数据填充函数
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData2Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            auto size = std::get<Vector3>(batch.materials[index]->getVector("displaySize"));
            instanceData->attr1 = Vector4(size.x, size.y, size.z, 0.0f);
            instanceData->attr2 = Vector4(
                batch.materials[index]->getFloat("rounding"),
                batch.materials[index]->getFloat("alpha"),
                0.0f, 0.0f
            );
        };
    } else if (layout.name == "font") {
        layout.elementSize = sizeof(DefaultBatchData2Attr);
        // 数据填充函数
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData2Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            auto fontColor = std::get<Vector4>(batch.materials[index]->getVector("fontColor"));
            instanceData->attr1 = fontColor;
            instanceData->attr2 = Vector4(batch.materials[index]->getFloat("alpha"), 0.0, 0.0f, 0.0f);
        };
    } else if (layout.name == "bounce") {
        layout.elementSize = sizeof(DefaultBatchData2Attr);
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData2Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            auto meshCenter = std::get<Vector3>(batch.materials[index]->getVector("meshCenter"));
            instanceData->attr1 = Vector4(
                meshCenter.x, meshCenter.y, meshCenter.z,
                batch.materials[index]->getFloat("alpha")
            );
            instanceData->attr2 = Vector4(
                batch.materials[index]->getFloat("timeDelta"),
                batch.materials[index]->getFloat("duration"),
                batch.materials[index]->getFloat("bounceTimes"),
                batch.materials[index]->getFloat("scaleRange")
            );
        };
    } else if (layout.name == "button") {
        layout.elementSize = sizeof(DefaultBatchData3Attr);
        layout.filler = [](void* data, const RenderBatch& batch, int index) {
            auto instanceData = static_cast<DefaultBatchData3Attr*>(data);
            instanceData->model = batch.transforms[index]->getWorldMatrix();
            instanceData->attr1 = std::get<Vector4>(batch.materials[index]->getVector("color"));

            auto size = std::get<Vector3>(batch.materials[index]->getVector("displaySize"));
            instanceData->attr2 = Vector4(
                size.x, size.y,
                batch.materials[index]->getFloat("rounding"),
                batch.materials[index]->getFloat("alpha"));

            instanceData->attr3 = Vector4(
                batch.materials[index]->getFloat("useTexture"),
                0.0f, 0.0f, 0.0f);
        };
    }
}
} // morrow
