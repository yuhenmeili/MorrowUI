//
// 渲染设备启动配置：由 EngineOptions 携带，经 Platform::initialize →
// RenderingThread → RenderDeviceProxy → GLRenderDevice 一次性下发。
//
// 这些选项只在设备构造期消费一次，不构成运行时可变状态。
//

#ifndef MORROW_RENDERER_RENDERDEVICEOPTIONS_H_
#define MORROW_RENDERER_RENDERDEVICEOPTIONS_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace morrow {

struct RenderDeviceOptions {
    /// CommandBuffer 单槽容量（字节）。0 = 使用引擎默认（16MB）。
    /// 常驻内存 = 容量 × 环形槽位数（当前 3）。稳态帧命令量通常远小于
    /// 启动帧，量产可按实测峰值调小（如 1~2MB）以降低启动期分配和
    /// 常驻内存。溢出时丢弃命令并输出 LOG_E，不会越界写。
    size_t commandBufferCapacity = 0;

    /// shader 二进制缓存（glProgramBinary）目录；空字符串 = 跟随平台默认
    /// （QNX: "/var/data/shaders"，桌面平台: 关闭）。
    /// 缓存键为内容哈希：驱动标识 + 完整预处理后源码 + formatVersion。
    /// 修改 shader 源码后哈希自动变化，旧缓存文件自然失效（无需手工重置），
    /// 新缓存以新文件名写入；残留旧文件可定期清理。
    std::string shaderBinaryCacheDir;

    /// 缓存格式版本：源码/驱动已由内容哈希覆盖；此值仅在引擎侧的
    /// 缓存文件布局或 shader 预处理逻辑变更时递增，用于强制全量失效。
    uint32_t shaderCacheFormatVersion = 1;

    /// SSBO 能力预设：-1 = 启动期查询（默认）；0 = 强制关闭；1 = 强制开启。
    /// 车型/SoC 固定时预设可消除启动期主线程↔渲染线程的一次同步往返。
    /// 预设为开启前必须确认目标 GPU 为 ES 3.1+ 且驱动支持 SSBO。
    int32_t ssboSupportPreset = -1;
};

} // namespace morrow

#endif // MORROW_RENDERER_RENDERDEVICEOPTIONS_H_
