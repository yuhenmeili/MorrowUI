#include "ShaderBinaryCache.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "utils/Log.h"

#ifdef OPENGL_GLFW
#include "platform/wgl/OpenglHeader.h"
#else
#include "platform/egl/EGLHeader.h"
#include "platform/egl/GLESHeader.h"
#endif

namespace morrow {
namespace {
// FNV-1a 64-bit：非加密哈希，仅用于缓存键生成。
constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnvPrime = 1099511628211ull;

void hashBytes(uint64_t& hash, const void* data, size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kFnvPrime;
    }
}

void hashString(uint64_t& hash, const std::string& value) {
    hashBytes(hash, value.data(), value.size());
    // 追加分隔符，避免 "ab"+"c" 与 "a"+"bc" 哈希相同
    const uint8_t separator = 0;
    hashBytes(hash, &separator, 1);
}

// 缓存文件头：魔数 + 版本 + 键 + 二进制格式 + 长度。
// payload 紧随其后。固定布局，字段只增不改，改布局递增 kCacheLayoutVersion。
struct CacheFileHeader {
    uint32_t magic;          // kMagic
    uint32_t formatVersion;  // Options::formatVersion
    uint64_t sourceKey;      // computeKey 结果
    uint32_t binaryFormat;   // glGetProgramBinary 输出的驱动格式枚举
    uint32_t payloadSize;    // 二进制字节数
};
constexpr uint32_t kMagic = 0x4D534243; // "MSBC"
constexpr uint32_t kCacheLayoutVersion = 1;
static_assert(sizeof(CacheFileHeader) == 24, "unexpected padding in CacheFileHeader");
} // namespace

void ShaderBinaryCache::ensureDriverSignature() {
    if (m_driverSignatureReady) {
        return;
    }
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    m_driverSignature = std::string(version ? version : "") + "|" + std::string(vendor ? vendor : "") + "|" +
                        std::string(renderer ? renderer : "");
    m_driverSignatureReady = true;
}

uint64_t ShaderBinaryCache::computeKey(const std::string& vertexSource, const std::string& fragmentSource) {
    ensureDriverSignature();
    uint64_t hash = kFnvOffsetBasis;
    const uint32_t version = kCacheLayoutVersion * 1000000u + m_options.formatVersion;
    hashBytes(hash, &version, sizeof(version));
    hashString(hash, m_driverSignature);
    hashString(hash, vertexSource);
    hashString(hash, fragmentSource);
    return hash;
}

std::string ShaderBinaryCache::filePath(uint64_t key, const std::string& programName) const {
    char keyHex[17];
    std::snprintf(keyHex, sizeof(keyHex), "%016llx", static_cast<unsigned long long>(key));
    // programName 仅用于人工辨识，同一份源码在不同名字下会共享同一份缓存。
    std::filesystem::path dir(m_options.dir);
    return (dir / (programName + "." + keyHex + ".bin")).string();
}

bool ShaderBinaryCache::loadBinary(uint64_t key, const std::string& programName, std::vector<uint8_t>& outBinary, uint32_t& outBinaryFormat) {
    outBinary.clear();
    outBinaryFormat = 0;
    if (!enabled()) {
        return false;
    }

    std::ifstream file(filePath(key, programName), std::ios::binary);
    if (!file) {
        return false;
    }

    CacheFileHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || header.magic != kMagic || header.formatVersion != m_options.formatVersion || header.sourceKey != key) {
        return false;
    }

    outBinary.resize(header.payloadSize);
    if (header.payloadSize > 0) {
        file.read(reinterpret_cast<char*>(outBinary.data()), header.payloadSize);
        if (!file) {
            outBinary.clear();
            return false;
        }
    }
    outBinaryFormat = header.binaryFormat;
    return true;
}

void ShaderBinaryCache::storeBinary(uint64_t key, const std::string& programName, const void* data, size_t size, uint32_t binaryFormat) {
    if (!enabled() || !data || size == 0) {
        return;
    }

    std::error_code error;
    const std::filesystem::path dir(m_options.dir);
    std::filesystem::create_directories(dir, error);

    const std::string finalPath = filePath(key, programName);
    const std::string tempPath = finalPath + ".tmp";

    {
        CacheFileHeader header{};
        header.magic = kMagic;
        header.formatVersion = m_options.formatVersion;
        header.sourceKey = key;
        header.binaryFormat = binaryFormat;
        header.payloadSize = static_cast<uint32_t>(size);

        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            LOG_E("shader cache: cannot write {}", tempPath);
            return;
        }
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!file) {
            LOG_E("shader cache: write failed {}", tempPath);
            return;
        }
    }

    // 先写临时文件再原子替换，掉电时不留半截缓存文件。
    std::filesystem::remove(finalPath, error);
    std::filesystem::rename(tempPath, finalPath, error);
    if (error) {
        LOG_E("shader cache: rename failed {} ({})", finalPath, error.message());
    }
}

} // namespace morrow
