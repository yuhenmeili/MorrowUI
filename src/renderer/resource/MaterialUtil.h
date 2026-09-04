//
// Created by lance on 2026/5/10.
//

#ifndef MORROW_GUI_MATERIALUTIL_H
#define MORROW_GUI_MATERIALUTIL_H
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>

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

inline const char* getPlatformShaderVersion() {
#ifdef OPENGL_GLFW
    return "#version 460 core\n";
#else
    return "#version 320 es\n";
#endif
}

inline std::string normalizeShaderVersion(const std::string& source) {
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

inline std::string buildShaderSource(const std::string& shaderSource, const std::string& shaderHead) {
    const std::size_t lineEnd = shaderSource.find('\n');
    if (lineEnd == std::string::npos) {
        return shaderSource + shaderHead;
    }

    return shaderSource.substr(0, lineEnd + 1) + shaderHead + shaderSource.substr(lineEnd + 1);
}

inline void appendDefaultPrecisionIfNeeded(const std::string& shaderSource, std::string& shaderHead) {
    if (shaderSource.find("precision ") != std::string::npos) {
        return;
    }

    shaderHead += "precision mediump float;\n";
    shaderHead += "precision mediump int;\n";
}

// ── shader include 解析（SHADER_SOURCE_ORGANIZATION_PROPOSAL.md §7）──
// 仅识别行首 #include "path"；include-once（同一文件全局展开一次）；
// 循环包含/缺失/超过深度限制时返回 false 并给出 error（fail-fast）。
// loader 由调用方提供（embedded 优先、assets/shaders 文件回退），便于纯 CPU 单测。
using ShaderChunkLoader = std::function<bool(const std::string& fileName, std::string& source)>;

namespace detail {
inline bool resolveIncludesInternal(
    const std::string& source,
    const ShaderChunkLoader& loader,
    std::string& resolved,
    std::string& error,
    std::unordered_set<std::string>& includedFiles,
    std::unordered_set<std::string>& activeFiles,
    int depth) {
    constexpr int kMaxIncludeDepth = 8;
    if (depth > kMaxIncludeDepth) {
        error = "shader include depth limit exceeded";
        return false;
    }
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        const std::size_t carriage = line.find('\r');
        if (carriage != std::string::npos) {
            line.erase(carriage);
        }
        const std::string includeTag = "#include \"";
        if (line.compare(0, includeTag.size(), includeTag) != 0) {
            resolved += line;
            resolved += '\n';
            continue;
        }
        const std::size_t closingQuote = line.find('"', includeTag.size());
        if (closingQuote == std::string::npos) {
            error = "malformed shader include: " + line;
            return false;
        }
        const std::string fileName = line.substr(includeTag.size(), closingQuote - includeTag.size());
        if (fileName.empty()) {
            error = "empty shader include path";
            return false;
        }
        if (includedFiles.count(fileName) != 0) {
            continue;  // include-once：已完整展开过
        }
        if (activeFiles.count(fileName) != 0) {
            error = "cyclic shader include: " + fileName;
            return false;
        }
        std::string chunkSource;
        if (!loader || !loader(fileName, chunkSource)) {
            error = "shader include not found: " + fileName;
            return false;
        }
        activeFiles.insert(fileName);
        const bool expanded = resolveIncludesInternal(chunkSource, loader, resolved, error, includedFiles, activeFiles, depth + 1);
        activeFiles.erase(fileName);
        if (!expanded) {
            return false;
        }
        includedFiles.insert(fileName);
    }
    return true;
}
}  // namespace detail

inline bool resolveIncludes(
    const std::string& source,
    const ShaderChunkLoader& loader,
    std::string& resolved,
    std::string& error) {
    std::unordered_set<std::string> includedFiles;
    std::unordered_set<std::string> activeFiles;
    return detail::resolveIncludesInternal(source, loader, resolved, error, includedFiles, activeFiles, 0);
}
}
} // namespace
#endif //MORROW_GUI_MATERIALUTIL_H
