//
// QNX 平台输入提供者：独立 screen 事件线程阻塞采集，主线程 poll() 排空本地队列。
// 采集侧的 screen API 用法与事件翻译规则见类注释（QNXInputProvider.h）。
//

#include "QNXInputProvider.h"

#include <algorithm>
#include <chrono>

#include "math/MathUtils.h"
#include "morrow/utils/Log.h"

namespace morrow {
namespace {
// 采集线程退出看门狗：screen_get_event 的等待上限。不用 -1 无限阻塞是为了
// 不依赖 pulse/用户事件注入即可干净退出；代价是空闲期该后台线程每 200ms 醒来
// 查一次原子标志（微秒级），主线程的空闲等待不受影响（由唤醒契约驱动）。
constexpr int32_t kEventThreadQuitWatchdogNs = 200000000;

bool isUnhandledEventType(int32_t eventType) {
    return eventType != SCREEN_EVENT_NONE &&
        eventType != SCREEN_EVENT_PROPERTY &&
        eventType != SCREEN_EVENT_MTOUCH_MOVE &&
        eventType != SCREEN_EVENT_MTOUCH_TOUCH &&
        eventType != SCREEN_EVENT_MTOUCH_RELEASE &&
        eventType != SCREEN_EVENT_POINTER &&
        eventType != SCREEN_EVENT_KEYBOARD;
}

int32_t countUniqueTouchIDs(const std::vector<TouchEvent>& events) {
    std::vector<int32_t> touchIDs;
    for (const auto& touchEvent : events) {
        if (touchEvent.touchID < 0) {
            continue;
        }
        if (std::find(touchIDs.begin(), touchIDs.end(), touchEvent.touchID) == touchIDs.end()) {
            touchIDs.emplace_back(touchEvent.touchID);
        }
    }
    return static_cast<int32_t>(touchIDs.size());
}
} // namespace

QNXInputProvider::~QNXInputProvider() {
    if (m_threadStarted) {
        m_quit.store(true, std::memory_order_release);
        if (m_eventThread.joinable()) {
            m_eventThread.join();
        }
        LOG_I("QNX input event thread stopped, peakQueue={} overflow={}", m_peakPendingEvents, m_overflowCount);
    }
    if (m_threadScreenEvent) {
        screen_destroy_event(m_threadScreenEvent);
        m_threadScreenEvent = nullptr;
    }
}

void QNXInputProvider::setScreenContext(screen_context_t screenContext) {
    m_screenContext = screenContext;
}

void QNXInputProvider::setWindowOffset(int32_t windowX, int32_t windowY) {
    m_windowX = windowX;
    m_windowY = windowY;
}

void QNXInputProvider::setSkipEvents(bool skipEvents) {
    m_skipEvents = skipEvents;
}

void QNXInputProvider::startEventThread() {
    if (m_threadStarted) {
        return;
    }
    if (!m_screenContext) {
        LOG_I("QNXInputProvider::startEventThread skipped: no screen context");
        return;
    }
    if (screen_create_event(&m_threadScreenEvent) != 0) {
        LOG_I("screen_create_event failed");
        return;
    }
    m_threadStarted = true;
    m_eventThread = std::thread(&QNXInputProvider::eventThreadLoop, this);
}

void QNXInputProvider::eventThreadLoop() {
    // 采集线程是唯一的 screen_get_event 调用者。screen context 支持跨线程使用
    // （事件获取与属性设置为独立 IPC 往返，Qt QPA 同款）；主线程的
    // screen_set_window_property 等调用不受影响。窗口偏移在 startEventThread
    // 之前设置，此后只读。
    while (!m_quit.load(std::memory_order_acquire)) {
        if (screen_get_event(m_screenContext, m_threadScreenEvent, kEventThreadQuitWatchdogNs) != 0) {
            continue;  // 看门狗超时（无事件），回到循环头检查 quit
        }

        int32_t eventType = 0;
        if (screen_get_event_property_iv(m_threadScreenEvent, SCREEN_PROPERTY_TYPE, &eventType) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_TYPE) failed");
            continue;
        }

        // SCREEN_EVENT_NONE：空队列哨兵（正常空路径走超时返回），跳过；
        // SCREEN_EVENT_PROPERTY：窗口属性变化，跳过（独立线程不再截断后续采集）。
        if (eventType == SCREEN_EVENT_NONE || eventType == SCREEN_EVENT_PROPERTY) {
            continue;
        }

        if (isUnhandledEventType(eventType)) {
            // POINTER/KEYBOARD 等在触摸-only 目标上有意丢弃，映射为需求驱动项
            // （road_map/QNX_EVENT_SYSTEM_COMPARISON.md §3 P3-1）。
            LOG_I("screen event type={} not handled by QNXInputProvider", eventType);
            continue;
        }

        TouchEventType mappedType = TOUCH_EVENT_TYPE_NONE;
        switch (eventType) {
            case SCREEN_EVENT_MTOUCH_MOVE:
                mappedType = TOUCH_EVENT_TYPE_MOVE;
                break;
            case SCREEN_EVENT_MTOUCH_TOUCH:
                mappedType = TOUCH_EVENT_TYPE_TOUCH;
                break;
            case SCREEN_EVENT_MTOUCH_RELEASE:
                mappedType = TOUCH_EVENT_TYPE_RELEASE;
                break;
            default:
                continue;
        }

        int32_t touchID = 0;
        if (screen_get_event_property_iv(m_threadScreenEvent, SCREEN_PROPERTY_TOUCH_ID, &touchID) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_TOUCH_ID) failed");
            continue;
        }
        int32_t touchPos[2]{0, 0};
        if (screen_get_event_property_iv(m_threadScreenEvent, SCREEN_PROPERTY_POSITION, touchPos) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_POSITION) failed");
            continue;
        }
        int32_t rawTimestampMs = 0;
        // 时间戳属性按可用性锁存查询：支持的栈逐事件取硬件时间戳（精度优于
        // 本地时钟，事件到达与读取之间的延迟不引入偏差）；不支持的栈首次失败
        // 即降级——后续事件不再查询（每条事件省一次 IPC 往返），仅提示一次，
        // 避免 MOVE 事件流逐条刷失败日志（QNX_EVENT_SYSTEM_COMPARISON.md R3）。
        bool hasTimestamp = false;
        if (m_eventTimestampSupported) {
            hasTimestamp = screen_get_event_property_iv(m_threadScreenEvent, SCREEN_PROPERTY_TIMESTAMP, &rawTimestampMs) == 0;
            if (!hasTimestamp) {
                m_eventTimestampSupported = false;
                LOG_W("SCREEN_PROPERTY_TIMESTAMP not supported by this screen stack, touch timestamps fall back to local monotonic clock");
            }
        }

        TouchEvent touchEvent;
        touchEvent.touchID = touchID;
        touchEvent.positionX = static_cast<float>(touchPos[0] - m_windowX);
        touchEvent.positionY = static_cast<float>(touchPos[1] - m_windowY);
        touchEvent.touchTime = hasTimestamp ? toEngineTimeSeconds(rawTimestampMs) : Math::getCurrentMonotonicTime();
        touchEvent.eventType = mappedType;
        touchEvent.deviceType = TOUCH_DEVICE_TYPE_TOUCH;

        if (mappedType == TOUCH_EVENT_TYPE_TOUCH) {
            LOG_I("SCREEN_EVENT_MTOUCH_TOUCH id={}, pos=({}, {})", touchID, touchEvent.positionX, touchEvent.positionY);
        } else if (mappedType == TOUCH_EVENT_TYPE_RELEASE) {
            LOG_I("SCREEN_EVENT_MTOUCH_RELEASE id={}, pos=({}, {})", touchID, touchEvent.positionX, touchEvent.positionY);
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_pendingEvents.size() >= kMaxPendingEvents) {
                m_pendingEvents.pop_front();
                ++m_overflowCount;
                if (m_overflowCount == 1 || m_overflowCount % 100 == 0) {
                    LOG_W("QNX input queue overflow, dropped={} peak={}", m_overflowCount, m_peakPendingEvents);
                }
            }
            m_pendingEvents.emplace_back(touchEvent);
            const uint64_t queueSize = m_pendingEvents.size();
            if (queueSize > m_peakPendingEvents) {
                m_peakPendingEvents = queueSize;
            }
        }
        wakeEventLoop();
    }
}

void QNXInputProvider::wakeEventLoop() {
    {
        std::lock_guard<std::mutex> lock(m_wakeMutex);
        ++m_wakeGeneration;
    }
    m_wakeCv.notify_one();
}

void QNXInputProvider::waitForEventLoopWakeup(double timeoutSeconds) {
    if (timeoutSeconds <= 0.0) {
        return;
    }
    std::unique_lock<std::mutex> lock(m_wakeMutex);
    const uint64_t generation = m_wakeGeneration;
    m_wakeCv.wait_for(lock, std::chrono::duration<double>(timeoutSeconds),
        [this, generation] { return m_wakeGeneration != generation; });
}

double QNXInputProvider::toEngineTimeSeconds(int32_t rawTimestampMs) {
    // SCREEN_PROPERTY_TIMESTAMP 为毫秒、32 位约 49.7 天回绕；累计回绕次数
    // 扩展为单调 64 位毫秒（screen 事件时间戳单调递增，回绕表现为符号位翻转）。
    if (m_lastRawTimestampValid && rawTimestampMs < m_lastRawTimestampMs) {
        ++m_timestampWrapCount;
    }
    m_lastRawTimestampMs = rawTimestampMs;
    m_lastRawTimestampValid = true;
    const uint64_t totalMs = static_cast<uint64_t>(m_timestampWrapCount) * (1ULL << 32) +
        static_cast<uint32_t>(rawTimestampMs);
    const double seconds = static_cast<double>(totalMs) / 1000.0;
    if (!m_timestampOffsetInitialized) {
        m_timestampOffsetSeconds = Math::getCurrentMonotonicTime() - seconds;
        m_timestampOffsetInitialized = true;
    }
    return seconds + m_timestampOffsetSeconds;
}

bool QNXInputProvider::hasTouchID(const std::vector<TouchEvent>& events, int32_t touchID) const {
    for (const auto& touchEvent : events) {
        if (touchID >= 0 && touchEvent.touchID == touchID) {
            return true;
        }
    }
    return false;
}

void QNXInputProvider::poll(std::vector<TouchEvent>& outEvents) {
    outEvents.clear();
    if (m_skipEvents) {
        // skip 帧（休眠恢复窗口）：连同队列内尚未消费的事件与释放合成追踪一并丢弃
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingEvents.clear();
        m_lastFrameInputEvents.clear();
        LOG_I("skip events for thread wait");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        outEvents.reserve(m_pendingEvents.size());
        for (auto& pendingEvent : m_pendingEvents) {
            outEvents.push_back(std::move(pendingEvent));
        }
        m_pendingEvents.clear();
    }

    const double nowSeconds = Math::getCurrentMonotonicTime();
    // 防止 release 事件不触发，安全冗余：上一帧 TOUCH/MOVE 的 touchID 本帧消失时合成 RELEASE
    for (const auto& lastTouchEvent : m_lastFrameInputEvents) {
        if ((lastTouchEvent.eventType == TOUCH_EVENT_TYPE_TOUCH || lastTouchEvent.eventType == TOUCH_EVENT_TYPE_MOVE) &&
            !hasTouchID(outEvents, lastTouchEvent.touchID)) {
            TouchEvent touchEvent;
            touchEvent.touchID = lastTouchEvent.touchID;
            touchEvent.positionX = lastTouchEvent.positionX;
            touchEvent.positionY = lastTouchEvent.positionY;
            touchEvent.touchTime = nowSeconds;
            touchEvent.eventType = TOUCH_EVENT_TYPE_RELEASE;
            touchEvent.deviceType = TOUCH_DEVICE_TYPE_TOUCH;
            outEvents.emplace_back(touchEvent);
        }
    }

    m_lastFrameInputEvents.clear();
    if (outEvents.empty()) {
        return;
    }

    const int32_t totalTouchIDCount = countUniqueTouchIDs(outEvents);
    for (auto& touchEvent : outEvents) {
        touchEvent.totalTouchIDCount = totalTouchIDCount;
        m_lastFrameInputEvents.emplace_back(touchEvent);
    }
}

} // namespace morrow
