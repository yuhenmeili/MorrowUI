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
    virtual void useGPUProgram(GPUProgram* program) = 0;

    virtual GPUProgram* createGPUProgram(const std::string& programFileName,
                                         const std::string& vertexShader,
                                         const std::string& fragmentShader) = 0;

    virtual void deletGPUProgram(GPUProgram* program) = 0;

    virtual GPUProgramParam* getGPUProgramParam(GPUProgram* program, const std::string& name) = 0;

    //------------------------------ 按 GPUProgramParam 句柄设置 Uniform ------------------------------
    virtual void setGPUProgramParamAsInt(GPUProgramParam* param, int32_t value) = 0;

    virtual void setGPUProgramParamAsFloat(GPUProgramParam* param, float value) = 0;

    virtual void setGPUProgramParamAsVec2(GPUProgramParam* param, float x, float y) = 0;

    virtual void setGPUProgramParamAsVec3(GPUProgramParam* param, float x, float y, float z) = 0;

    virtual void setGPUProgramParamAsVec4(GPUProgramParam* param, float x, float y, float z, float w) = 0;

    virtual void setGPUProgramParamAsMat4(GPUProgramParam* param, const Matrix4& mat) = 0;

    virtual void setGPUProgramParamAsIntArray(GPUProgramParam* param, const int32_t* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsFloatArray(GPUProgramParam* param, const float* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsMat4Array(GPUProgramParam* param, const std::vector<Matrix4>& values) = 0;

    //------------------------------ 按名称直接设置 Uniform（便捷重载）------------------------------
    virtual void setGPUProgramParamAsInt(GPUProgram* program, const std::string& uniformName, int32_t value) = 0;

    virtual void setGPUProgramParamAsFloat(GPUProgram* program, const std::string& uniformName, float value) = 0;

    virtual void setGPUProgramParamAsVec2(GPUProgram* program, const std::string& uniformName, float x, float y) = 0;

    virtual void setGPUProgramParamAsVec3(GPUProgram* program, const std::string& uniformName, float x, float y, float z) = 0;

    virtual void setGPUProgramParamAsVec4(GPUProgram* program, const std::string& uniformName, float x, float y, float z, float w) = 0;

    virtual void setGPUProgramParamAsMat4(GPUProgram* program, const std::string& uniformName, const Matrix4& mat) = 0;

    virtual void setGPUProgramParamAsIntArray(GPUProgram* program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsFloatArray(GPUProgram* program, const std::string& uniformName, const float* values, int32_t size, int32_t step) = 0;
};

} // namespace morrow

#endif // MORROW_RENDERER_GPUSHADERDEVICE_H_
