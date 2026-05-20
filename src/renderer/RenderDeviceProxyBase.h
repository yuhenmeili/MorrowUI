//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_THREADESDEVICEBASE_H_
#define MORROW_RENDERER_THREADESDEVICEBASE_H_

#include "RenderDevice.h"
#include <thread>
#include "PlatformSemaphore.h"
#include "GLRenderDevice.h"
#include "Platform.h"

namespace morrow {
class RenderDeviceProxyBase;

class GPUProgramParamHandle;

class GPUProgramHandle : public GPUProgram {
    friend class RenderDeviceProxy;

public:
    GPUProgram* getReal() override; // returns m_realProgram
    GPUProgramParam* getUniformParam(const std::string& name) override;

    GPUProgramParam* getAttributeParam(const std::string& name) override;

    void use();

    void setBool(const std::string& name, bool value);

    void setInt(const std::string& name, int32_t value);

    void setFloat(const std::string& name, float value);

    void setVec2(const std::string& name, const Vector2& value);

    void setVec2(const std::string& name, float x, float y);

    void setVec3(const std::string& name, const Vector3& value);

    void setVec3(const std::string& name, float x, float y, float z);

    void setVec4(const std::string& name, const Vector4& value);

    void setVec4(const std::string& name, float x, float y, float z, float w);

    void setMat4(const std::string& name, const Matrix4& mat);

    void setIntArray(const std::string& name, const int32_t* values, int32_t size, int32_t step);

    std::string getProgramFileName() const;

    void setProgramFileName(const std::string& programFileName);

protected:
    explicit GPUProgramHandle(RenderDeviceProxyBase* threadDevice);

    ~GPUProgramHandle() override = default;

public:
    RenderDeviceProxyBase* m_threadDevice = nullptr;
    GPUProgram* m_realProgram = nullptr;

private:
    std::unordered_map<std::string, GPUProgramParamHandle*> m_nameToParams;
    std::string m_programFileName;
};

class Texture2DHandle : public Texture2D {
    friend class RenderDeviceProxy;

public:
    Texture2D* getReal() override; // returns m_realTexture

protected:
    Texture2DHandle();

    ~Texture2DHandle() override = default;

public:
    Texture2D* m_realTexture = nullptr;
};

class VBOHandle : public VBO {
    friend class RenderDeviceProxy;

public:
    VBO* getReal() override; // returns m_realVbo

protected:
    VBOHandle();

    ~VBOHandle() override = default;

public:
    VBO* m_realVbo = nullptr;
};

class UBOHandle : public UBO {
    friend class RenderDeviceProxy;

public:
    UBO* getReal() override; // returns m_realUbo

protected:
    UBOHandle();

    ~UBOHandle() override = default;

public:
    UBO* m_realUbo = nullptr;
};

class SSBOHandle : public SSBO {
    friend class RenderDeviceProxy;

public:
    SSBO* getReal() override; // returns m_realSSBO

protected:
    SSBOHandle();

    ~SSBOHandle() override = default;

public:
    SSBO* m_realSSBO = nullptr;
};

class GPUProgramParamHandle : public GPUProgramParam {
    friend class RenderDeviceProxyBase;

public:
    GPUProgramParam* getReal() override;  // returns m_realParam

protected:
    GPUProgramParamHandle();

    ~GPUProgramParamHandle() override = default;

public:
    GPUProgramParam* m_realParam = nullptr;
};

class RenderTargetHandle : public RenderTarget {
    friend class RenderDeviceProxy;

public:
    RenderTarget* getReal() override; // returns m_realRenderTarget

protected:
    RenderTargetHandle();

    ~RenderTargetHandle() override = default;

public:
    RenderTarget* m_realRenderTarget = nullptr;
    int32_t m_width  = 0;
    int32_t m_height = 0;
};

class RenderDeviceProxyBase : public RenderDevice {
public:
    enum WaitType {
        WaitType_Common,
        WaitType_OnwerShip,
        WaitType_Present,
        WaitType_CreateShader,
        WaitType_CreateVBO,
        WaitType_CreateTexture,
        WaitType_Max
    };

protected:
    bool m_returnResImmediately; //是否立即返回资源创建
    bool m_threaded;
    bool m_isInPresenting;
    GLRenderDevicePtr m_realDevice;

private:
    std::shared_ptr<std::thread> m_thread;
    bool m_quit;
    Semaphore m_waitSem[WaitType_Max];

public:
    RenderDeviceProxyBase(PlatformSharedPtr platform, bool returnResImmediately);

    ~RenderDeviceProxyBase() override;

    /** Called by destructor before join; override to unblock render thread (e.g. double-buffer semaphore). */
    virtual void signalThreadToExit() {
    }

    GPUProgramParam* getGPUProgramParam(GPUProgram* program, const std::string& name) override;

    virtual void initThreadGPUProgramParam(GPUProgramHandle* program, GPUProgramParamHandle* param, const std::string& name) = 0;

    bool isCreateResInBlockMode() const;

    void waitForSignal(WaitType waitType = WaitType_Common);

    void signal(WaitType waitType = WaitType_Common);

    void waitForPresent();

    void signalPresent();

    void run(bool multithread);

protected:
    /** Override in derived class for double-buffered command queue (e.g. wait frame then drain). */
    virtual void runCommand();

    bool isQuit() const { return m_quit; }
};
} // MORROWGUI

#endif //MORROW_RENDERER_THREADESDEVICEBASE_H_
