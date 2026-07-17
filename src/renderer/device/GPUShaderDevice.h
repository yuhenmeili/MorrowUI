//
// GPUShaderDevice.h — GPU 着色器设备接口
//
// 职责：GPUProgram (Shader) 的创建、编译、使用、Uniform 参数设置。
// 这是从 RenderDevice 拆分出的子接口，可按需单独依赖。
//

#ifndef MORROW_RENDERER_GPUSHADERDEVICE_H_
#define MORROW_RENDERER_GPUSHADERDEVICE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "GpuTypes.h"
#include "Matrix4.h"

namespace morrow {
using namespace Math;

class GPUShaderDevice {
public:
    virtual ~GPUShaderDevice() = default;

    //---------------------------------------------------GPUProgram---------------------------------------------------
    virtual void useGPUProgram(HwGPUProgram program) = 0;

    virtual HwGPUProgram createGPUProgram(const std::string& programFileName,
                                          const std::string& vertexShader,
                                          const std::string& fragmentShader) = 0;

    virtual void deletGPUProgram(HwGPUProgram program) = 0;

    //------------------------------ 按名称设置 Uniform（统一接口，GPUProgramParam 不再独立存在）------------------------------
    virtual void setGPUProgramParamAsInt(HwGPUProgram program, const std::string& uniformName, int32_t value) = 0;

    virtual void setGPUProgramParamAsFloat(HwGPUProgram program, const std::string& uniformName, float value) = 0;

    virtual void setGPUProgramParamAsVec2(HwGPUProgram program, const std::string& uniformName, float x, float y) = 0;

    virtual void setGPUProgramParamAsVec3(HwGPUProgram program, const std::string& uniformName, float x, float y, float z) = 0;

    virtual void setGPUProgramParamAsVec4(HwGPUProgram program, const std::string& uniformName, float x, float y, float z, float w) = 0;

    virtual void setGPUProgramParamAsMat4(HwGPUProgram program, const std::string& uniformName, const Matrix4& mat) = 0;

    virtual void setGPUProgramParamAsIntArray(HwGPUProgram program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsFloatArray(HwGPUProgram program, const std::string& uniformName, const float* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsMat4Array(HwGPUProgram program, const std::string& uniformName, const std::vector<Matrix4>& values) = 0;
};

} // namespace morrow

#endif // MORROW_RENDERER_GPUSHADERDEVICE_H_
