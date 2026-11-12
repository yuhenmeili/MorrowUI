//
// QNX 平台输入提供者：独立 screen 事件线程阻塞采集（screen_get_event），主线程
// 帧首 poll() 只排空本地有界队列。采集与消费分离——QNX Screen 官方推荐的专用
// 事件线程形态（Qt QPA 同款），消费面保持主线程、帧边界、批量不变。
//
// 带来的行为变化（road_map/QNX_EVENT_SYSTEM_COMPARISON.md §6 R1 方案 B）：
// 1. 输入到达即唤醒主线程空闲等待（waitForEventLoopWakeup），空闲画面的触摸
//    响应不再受 Engine 空闲节拍（waitForEvents 超时）限制；
// 2. RenderingThread::requestRender 的 wakeEventLoop 契约在 QNX 上真正生效；
// 3. screen 事件队列不再依赖主线程 drain 节拍排空，PROPERTY 事件也不再截断
//    当轮采集（原 poll 的 P3 遗留）。
//

#ifndef MORROW_QNX_INPUT_PROVIDER_H
#define MORROW_QNX_INPUT_PROVIDER_H

#include <screen/screen.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include "InputProvider.h"

namespace morrow {

class QNXInputProvider : public IInputProvider {
public:
    /// 事件归属反查表的一项：窗口句柄 + 窗口位置（触摸坐标按窗口局部系换算）
    struct ScreenWindowSource {
        screen_window_t handle = nullptr;
        int32_t offsetX = 0;
        int32_t offsetY = 0;
    };

    ~QNXInputProvider() override;

    /// 提供采集线程使用的 screen context（不转移所有权，EGLOperationsQNX 持有）
    void setScreenContext(screen_context_t screenContext);

    void setWindowOffset(int32_t windowX, int32_t windowY);

    /// 多屏：注册全部窗口句柄，采集线程按事件的 SCREEN_PROPERTY_WINDOW
    /// 反查来源窗口（窗口序号写入 TouchEvent::windowIndex，触摸坐标按
    /// 各窗口位置换算为窗口局部系）。可在采集线程启动后调用（互斥保护）。
    void setScreenWindows(const std::vector<ScreenWindowSource>& windows);

    void setSkipEvents(bool skipEvents);

    /// 启动 screen 采集线程。需在 setScreenContext/setWindowOffset 之后调用；
    /// 失败时引擎退化为无输入（poll 恒空），与旧实现的失败行为一致。
    void startEventThread();

    /// 拉取本帧输入事件（IInputProvider）：只排空本地队列，不再访问 screen API
    void poll(std::vector<TouchEvent>& outEvents) override;

    /// Platform::waitForEvents 契约：阻塞至新输入唤醒或超时（Engine 空闲 gate 调用）
    void waitForEventLoopWakeup(double timeoutSeconds);

    /// Platform::wakeEventLoop 契约：唤醒 waitForEventLoopWakeup
    /// （RenderingThread::requestRender 在任务投递时调用）
    void wakeEventLoop();

private:
    void eventThreadLoop();
    bool hasTouchID(const std::vector<TouchEvent>& events, int32_t touchID, int32_t windowIndex) const;

    /// R3：SCREEN_PROPERTY_TIMESTAMP（毫秒）→ 引擎 steady_clock 秒域。
    /// offset 仅初始化一次：事件间 Δt 精确（速度类消费），绝对值与
    /// getCurrentMonotonicTime() 可比（Interaction 长按等阈值类消费）。
    double toEngineTimeSeconds(int32_t rawTimestampMs);

    // 事件线程私有：时间戳属性可用性降级锁存（采集线程单写单读，无需原子）。
    // 部分 screen 栈不为触摸事件提供该属性（逐条查询恒失败），首次失败即置
    // false：后续事件跳过查询（省一次 IPC 往返）并直接走本地单调时钟回退。
    bool m_eventTimestampSupported = true;

    double m_timestampOffsetSeconds = 0.0;
    bool m_timestampOffsetInitialized = false;
    int32_t m_lastRawTimestampMs = 0;
    bool m_lastRawTimestampValid = false;
    uint32_t m_timestampWrapCount = 0;

    screen_context_t m_screenContext = nullptr;
    screen_event_t m_threadScreenEvent = nullptr;  // 采集线程私有事件对象
    int32_t m_windowX = 0;
    int32_t m_windowY = 0;
    // 事件归属反查表：主线程 initialize 晚于采集线程启动写入，采集线程逐
    // 事件读取——互斥锁保护（表极小、触摸事件低频，拷贝快照后无锁查表）
    std::mutex m_windowTableMutex;
    std::vector<ScreenWindowSource> m_screenWindows;

    std::atomic_bool m_quit{false};
    std::thread m_eventThread;
    bool m_threadStarted = false;

    // 有界事件队列（采集线程 → 主线程）。互斥锁 + deque 而非无锁环：输入事件
    // 速率低（≤百级/秒），正确性优先于极限吞吐。溢出丢最旧（新状态优先）。
    // 不变量 13：固定容量必须有峰值与溢出诊断。
    std::mutex m_queueMutex;
    std::deque<TouchEvent> m_pendingEvents;
    static constexpr size_t kMaxPendingEvents = 256;
    uint64_t m_peakPendingEvents = 0;
    uint64_t m_overflowCount = 0;

    // 主线程空闲唤醒：generation 比较，配合 condition_variable 谓词避免虚假唤醒
    std::mutex m_wakeMutex;
    std::condition_variable m_wakeCv;
    uint64_t m_wakeGeneration = 0;

    bool m_skipEvents = false;
    std::vector<TouchEvent> m_lastFrameInputEvents;
};

} // namespace morrow

#endif // MORROW_QNX_INPUT_PROVIDER_H
