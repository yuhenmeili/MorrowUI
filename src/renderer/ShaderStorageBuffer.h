//
// Created by 0060328 on 25-10-21.
//

#ifndef SSBOBUFFER_H
#define SSBOBUFFER_H
#include <functional>
#include <memory>
#include <string>
#include "GpuTypes.h"

namespace morrow {
class SSBOData;
struct RenderBatch;

enum class ShaderDataType {
    Int,
    Float,
    Vector2,
    Vector3,
    Vector4,
    Matrix3,
    Matrix4
};

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
    size_t elementSize;
    // std::vector<ShaderAttribute> attributes;
    std::function<void(void* data, const RenderBatch& batch, int index)> filler;
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
