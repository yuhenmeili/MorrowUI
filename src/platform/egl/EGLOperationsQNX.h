//
// Created by 0060328 on 25-9-18.
//

#ifndef EGLOPERATIONSQNX_H
#define EGLOPERATIONSQNX_H
#include <memory>
#include <screen/screen.h>

#include "EGLOperations.h"

namespace morrow {
class EGLOperationsQNX : public EGLOperations {
public:
    explicit EGLOperationsQNX(int32_t samples = 1);

    ~EGLOperationsQNX() override;

    bool initialize() override;

    ESContextSharedPtr getContext() override;

private:
    int32_t initScreen();

    int32_t initEGL();

    ESContextSharedPtr m_esContext;
    int32_t m_requestedSamples = 1;
};

using EGLOperationsQNXSharedPtr = std::shared_ptr<EGLOperationsQNX>;
} // morrow

#endif //EGLOPERATIONSQNX_H
