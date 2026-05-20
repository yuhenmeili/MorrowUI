//
// Created by lance on 2022/10/11.
//

#ifndef MORROW_BOUNCE_H
#define MORROW_BOUNCE_H

#include <memory>
#include "base/Widget.h"
#include "Texture.h"
#include "../helpers/Tween.h"
#include "base/UIWidget.h"

namespace morrow {
class MRBounce : public UIWidget {
public:
    static std::shared_ptr<MRBounce> create();

    ~MRBounce() override = default;

private:
    MRBounce();
};

using MRBounceSharedPtr = std::shared_ptr<MRBounce>;
}


#endif //MORROW_BOUNCE_H
