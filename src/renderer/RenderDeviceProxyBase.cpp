//
// Created by lance on 2023/12/8.
//

#include <functional>
#include "RenderDeviceProxyBase.h"

namespace morrow {
GPUProgram* GPUProgramHandle::getReal() {
    return m_realProgram;
}

GPUProgramParam* GPUProgramHandle::getUniformParam(const std::string& name) {
    auto const& iter = m_nameToParams.find(name);
    if (iter == m_nameToParams.end()) {
        auto param = dynamic_cast<GPUProgramParamHandle*>(m_threadDevice->getGPUProgramParam(this, name));
        m_nameToParams.insert(iter, std::make_pair(name, param));
        return param;
    }
    return iter->second;
}

GPUProgramParam* GPUProgramHandle::getAttributeParam(const std::string& name) {
    return nullptr;
}

GPUProgramHandle::GPUProgramHandle(RenderDeviceProxyBase* threadDevice) : m_threadDevice(threadDevice), m_realProgram(nullptr) {
}

void GPUProgramHandle::use() {
    m_threadDevice->useGPUProgram(this);
}

void GPUProgramHandle::setBool(const std::string& name, bool value) {
    m_threadDevice->setGPUProgramParamAsInt(this, name, int32_t(value));
}

void GPUProgramHandle::setInt(const std::string& name, int32_t value) {
    m_threadDevice->setGPUProgramParamAsInt(this, name, value);
}

void GPUProgramHandle::setFloat(const std::string& name, float value) {
    m_threadDevice->setGPUProgramParamAsFloat(this, name, value);
}

void GPUProgramHandle::setVec2(const std::string& name, const Vector2& value) {
    m_threadDevice->setGPUProgramParamAsFloatArray(this, name, value.elements, 1, 2);
}

void GPUProgramHandle::setVec2(const std::string& name, float x, float y) {
    m_threadDevice->setGPUProgramParamAsVec2(this, name, x, y);
}

void GPUProgramHandle::setVec3(const std::string& name, const Vector3& value) {
    m_threadDevice->setGPUProgramParamAsFloatArray(this, name, value.elements, 1, 3);
}

void GPUProgramHandle::setVec3(const std::string& name, float x, float y, float z) {
    m_threadDevice->setGPUProgramParamAsVec3(this, name, x, y, z);
}

void GPUProgramHandle::setVec4(const std::string& name, const Vector4& value) {
    m_threadDevice->setGPUProgramParamAsFloatArray(this, name, value.elements, 1, 4);
}

void GPUProgramHandle::setVec4(const std::string& name, float x, float y, float z, float w) {
    m_threadDevice->setGPUProgramParamAsVec4(this, name, x, y, z, w);
}

void GPUProgramHandle::setMat4(const std::string& name, const Matrix4& mat) {
    m_threadDevice->setGPUProgramParamAsMat4(this, name, mat);
}

void GPUProgramHandle::setIntArray(const std::string& name, const int32_t* values, int32_t size, int32_t step) {
    m_threadDevice->setGPUProgramParamAsIntArray(this, name, values, size, step);
}

std::string GPUProgramHandle::getProgramFileName() const {
    return m_programFileName;
}

void GPUProgramHandle::setProgramFileName(const std::string& programFileName) {
    m_programFileName = programFileName;
}

Texture2D* Texture2DHandle::getReal() {
    return m_realTexture;
}

Texture2DHandle::Texture2DHandle() : m_realTexture(nullptr) {
}

VBO* VBOHandle::getReal() {
    return m_realVbo;
}

VBOHandle::VBOHandle() : m_realVbo(nullptr) {
}

UBO* UBOHandle::getReal() {
    return m_realUbo;
}

UBOHandle::UBOHandle() : m_realUbo(nullptr) {
}

SSBO* SSBOHandle::getReal() {
    return m_realSSBO;
}

SSBOHandle::SSBOHandle() : m_realSSBO(nullptr) {
}

GPUProgramParam* GPUProgramParamHandle::getReal() {
    return m_realParam;
}

GPUProgramParamHandle::GPUProgramParamHandle() : m_realParam(nullptr) {
}

RenderTarget* RenderTargetHandle::getReal() {
    return m_realRenderTarget;
}

RenderTargetHandle::RenderTargetHandle() : m_realRenderTarget(nullptr) {
}

//-------------------------------------------------------------------------------------------------------------

RenderDeviceProxyBase::RenderDeviceProxyBase(PlatformSharedPtr platform, bool returnResImmediately)
    : m_returnResImmediately(returnResImmediately), m_threaded(false), m_isInPresenting(false), m_quit(false) {
    m_realDevice = std::make_shared<GLRenderDevice>(platform);
}

RenderDeviceProxyBase::~RenderDeviceProxyBase() {
    m_quit = true;
    signalThreadToExit();
    if (m_thread) {
        m_thread->join();
    }
}

GPUProgramParam* RenderDeviceProxyBase::getGPUProgramParam(GPUProgram* program, const std::string& name) {
    auto param = new GPUProgramParamHandle();
    initThreadGPUProgramParam(dynamic_cast<GPUProgramHandle*>(program), param, name);
    return param;
}

bool RenderDeviceProxyBase::isCreateResInBlockMode() const {
    return m_returnResImmediately;
}

void RenderDeviceProxyBase::waitForSignal(WaitType waitType) {
    m_waitSem[waitType].waitForSignal();
}

void RenderDeviceProxyBase::signal(WaitType waitType) {
    m_waitSem[waitType].signal();
}

void RenderDeviceProxyBase::waitForPresent() {
    waitForSignal(WaitType_Present);
}

void RenderDeviceProxyBase::signalPresent() {
    signal(WaitType_Present);
    m_isInPresenting = false;
}

void RenderDeviceProxyBase::runCommand() {
}

void RenderDeviceProxyBase::run(bool multithread) {
    m_threaded = multithread;
    if (m_threaded) {
        m_thread = std::make_shared<std::thread>(std::bind(&RenderDeviceProxyBase::runCommand, this));
    }
}
} // MORROWGUI
