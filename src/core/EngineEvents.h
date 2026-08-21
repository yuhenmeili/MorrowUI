#ifndef MORROW_ENGINE_EVENTS_H_
#define MORROW_ENGINE_EVENTS_H_

#include "Observable.h"

namespace morrow
{
/**
 * @brief Synchronous lifecycle events owned by one Engine instance.
 *
 * These events describe completed frame boundaries. They are not a thread-safe
 * queue and must only be connected, disconnected and notified on the Engine
 * thread.
 */
struct EngineEvents {
    /// Fired after the render-request gate passes, before Tween/Widget update.
    Observable<> onFrameBegin;

    /// Fired after endFrame/present and one-shot frame callbacks complete.
    Observable<> onFrameEnd;
};
}

#endif // MORROW_ENGINE_EVENTS_H_
