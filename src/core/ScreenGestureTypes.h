//
// Created by lance on 2023/2/14.
//

#ifndef MORROW_SCREEN_GESTURE_TYPES_H
#define MORROW_SCREEN_GESTURE_TYPES_H
namespace morrow
{
enum ScreenGestureTypes : uint32_t
{
    None = 0,
    Single = 1,
    Double = 2,
    Three = 4,
    Four = 8,
    All = 15
};
}
#endif //MORROW_SCREEN_GESTURE_TYPES_H
