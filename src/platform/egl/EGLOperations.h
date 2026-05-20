//
// Created by 0060328 on 25-9-18.
//

#ifndef EGLOPERATIONS_H
#define EGLOPERATIONS_H

#include "EGL/egl.h"
#include "ESContext.h"
namespace morrow {
class EGLOperations {
public:
    virtual ~EGLOperations() = default;

    virtual bool initialize() = 0;

    virtual ESContextSharedPtr getContext() = 0;
};
}
#endif //EGLOPERATIONS_H
