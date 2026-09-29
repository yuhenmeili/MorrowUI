//
// Created by 0060328 on 25-9-18.
//

#ifndef PLATFORMFACTORY_H
#define PLATFORMFACTORY_H
#include <string>

#include "Platform.h"

namespace morrow {
class PlatformFactory {
public:
    static PlatformSharedPtr create(const WindowInfo& info) noexcept;

    static void destroy(Platform** platform) noexcept;
};

} // morrow

#endif //PLATFORMFACTORY_H
