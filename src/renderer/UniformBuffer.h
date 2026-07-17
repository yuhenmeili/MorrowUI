//
// Created by 0060328 on 25-10-21.
//

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H
#include <memory>
#include "Matrix4.h"
#include "GpuTypes.h"

namespace morrow {
using namespace Math;
struct UBOData;
struct FrameState;

class UniformBuffer {
public:
    UniformBuffer();

    ~UniformBuffer();

    // 更新全局UBO（包含投影视图矩阵）
    void update(std::shared_ptr<FrameState> frameState);

    void bind(HwGPUProgram shader);

private:
    HwUBO m_globalUBO{0};
    Matrix4 m_projectionView;
};
} // morrow

#endif //UNIFORMBUFFER_H
