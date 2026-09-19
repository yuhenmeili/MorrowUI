//
// ShaderBinaryCache —— glProgramBinary 磁盘缓存。
//
// 目标：消除二次冷启动的 GLSL 编译+链接耗时（车载 GPU 上单个 program
// 常见 10~50ms，首屏 5~10 个 program 即 100ms+）。
//
// 失效策略（无需手工版本号）：
//   缓存键 = FNV-1a64( formatVersion
//                       | GL_VERSION | GL_VENDOR | GL_RENDERER   ← 驱动标识
//                       | vertexSource | fragmentSource )         ← 完整预处理后源码
//   - 修改 shader 源码 → 源码内容变 → 哈希变 → 新缓存文件，旧文件自然失效；
//   - 驱动/GPU 更新 → 驱动标识变 → 哈希变；
//   - 引擎侧缓存文件布局或源码预处理逻辑变更 → 递增 formatVersion 强制全量失效。
// 即使键碰撞出错误缓存（哈希相同但内容不同），glProgramBinary 后的
// LINK_STATUS 校验失败会自动回退源码编译，不会产生错误画面。
//
// 线程：仅在持有 GL 上下文的线程使用（多线程模式=渲染线程，单线程=主线程），
// 串行调用，无需加锁。
//

#ifndef MORROW_RENDERER_SHADERBINARYCACHE_H_
#define MORROW_RENDERER_SHADERBINARYCACHE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace morrow {

class ShaderBinaryCache {
public:
    struct Options {
        /// 缓存目录；空 = 功能关闭。
        std::string dir;
        /// 文件布局/预处理逻辑版本，强制全量失效用。
        uint32_t formatVersion = 1;
    };

    bool enabled() const {
        return !m_options.dir.empty();
    }

    /// 设备构造期调用一次。
    void configure(const Options& options) {
        m_options = options;
    }

    /// 内容哈希键。首次调用会读取 GL 驱动标识（需已持有 GL 上下文）。
    uint64_t computeKey(const std::string& vertexSource, const std::string& fragmentSource);

    /// 读取缓存二进制。返回 false 表示未命中/损坏/版本不符。
    /// outBinaryFormat 为 glGetProgramBinary 返回的驱动格式枚举。
    bool loadBinary(uint64_t key, const std::string& programName, std::vector<uint8_t>& outBinary, uint32_t& outBinaryFormat);

    /// 写入缓存（临时文件 + rename，避免掉电产生半截文件）。
    void storeBinary(uint64_t key, const std::string& programName, const void* data, size_t size, uint32_t binaryFormat);

private:
    std::string filePath(uint64_t key, const std::string& programName) const;
    void ensureDriverSignature();

    Options m_options;
    std::string m_driverSignature;
    bool m_driverSignatureReady = false;
};

} // namespace morrow

#endif // MORROW_RENDERER_SHADERBINARYCACHE_H_
