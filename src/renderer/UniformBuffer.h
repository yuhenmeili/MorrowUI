//
// Created by 0060328 on 25-10-21.
//

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H
#include <memory>

#include "Matrix4.h"

namespace morrow {
using namespace Math;
class GPUProgramHandle;
struct UBOData;
class UBO;
struct FrameState;

class UniformBuffer {
public:
    UniformBuffer();

    ~UniformBuffer();

    // 更新全局UBO（包含投影视图矩阵）
    void update(std::shared_ptr<FrameState> frameState);

    void bind(GPUProgramHandle* shader);

private:
    UBO* m_globalUBO = nullptr;
    Matrix4 m_projectionView;
};
} // morrow

#endif //UNIFORMBUFFER_H
