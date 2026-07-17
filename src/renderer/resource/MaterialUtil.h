//
// Created by lance on 2026/5/10.
//

#ifndef MORROW_GUI_MATERIALUTIL_H
#define MORROW_GUI_MATERIALUTIL_H
#include <memory>

#include "GlobalObject.h"
#include "GpuTypes.h"

namespace morrow {
namespace MaterialUtil {
template <typename T>
std::shared_ptr<morrow::UBOData> makeUBOData(const T& payload) {
    auto* pool = RENDERINGTHREAD->getUBODataRecyclePool();
    auto uboData = pool ? pool->acquire() : std::make_shared<UBOData>();
    uboData->size = static_cast<uint32_t>(sizeof(T));
    uboData->data.resize(sizeof(T));
    std::memcpy(uboData->data.data(), &payload, sizeof(T));
    return uboData;
}

const char* getPlatformShaderVersion() {
#ifdef OPENGL_GLFW
    return "#version 460 core\n";
#else
    return "#version 320 es\n";
#endif
}

std::string normalizeShaderVersion(const std::string& source) {
    std::string normalized = source;
    const std::string versionTag = "#version";
    const std::size_t versionPos = normalized.find(versionTag);

    if (versionPos == std::string::npos) {
        normalized.insert(0, getPlatformShaderVersion());
        return normalized;
    }

    const std::size_t lineEnd = normalized.find('\n', versionPos);
    normalized.replace(
        versionPos,
        lineEnd == std::string::npos ? normalized.size() - versionPos : lineEnd - versionPos + 1,
        getPlatformShaderVersion());
    return normalized;
}

std::string buildShaderSource(const std::string& shaderSource, const std::string& shaderHead) {
    const std::size_t lineEnd = shaderSource.find('\n');
    if (lineEnd == std::string::npos) {
        return shaderSource + shaderHead;
    }

    return shaderSource.substr(0, lineEnd + 1) + shaderHead + shaderSource.substr(lineEnd + 1);
}

void appendDefaultPrecisionIfNeeded(const std::string& shaderSource, std::string& shaderHead) {
    if (shaderSource.find("precision ") != std::string::npos) {
        return;
    }

    shaderHead += "precision mediump float;\n";
    shaderHead += "precision mediump int;\n";
}
}
} // namespace
#endif //MORROW_GUI_MATERIALUTIL_H
