//
// Created by 0060328 on 25-10-21.
//

#ifndef SSBOBUFFER_H
#define SSBOBUFFER_H
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "GpuTypes.h"
#include "SSBOFieldBinding.h"

namespace morrow {
class SSBOData;
struct RenderBatch;

// Shader属性描述
struct ShaderAttribute {
    std::string name;
    size_t offset;
    size_t size;
    ShaderDataType type;
};

// SSBO数据结构描述
struct SSBOLayout {
    std::string name;
    // P0：默认初始化为 0，避免未注册 shader 使用未定义大小
    size_t elementSize = 0;
    // P2：声明式字段绑定（替代手工 filler）；shaderField 供 P3 reflection 校验
    std::vector<SSBOFieldBinding> fields;
    // P1：custom packer 通道，供复杂 shader 使用（fields 之外）
    std::function<void(void* data, const RenderBatch& batch, int index)> filler;

    // P0/P2：布局是否可用于填充实例数据（fields 或 filler 任一有效即可）
    bool isValid() const {
        return elementSize > 0 && (filler || !fields.empty());
    }
};

class ShaderStorageBuffer {
public:
    ShaderStorageBuffer();

    void resize(size_t size);

    void update();

    void* getDataPtr() const;

private:
    HwSSBO m_ssbo{0};
    std::shared_ptr<SSBOData> m_ssboData;
};

} // morrow

#endif //SSBOBUFFER_H
