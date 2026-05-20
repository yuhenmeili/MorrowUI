//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_BRAKE_PEDAL_H
#define MORROW_BRAKE_PEDAL_H

#include <memory>
#include "base/UIWidget.h"

namespace morrow {
class MRBrakePedal : public UIWidget {
public:
    static std::shared_ptr<MRBrakePedal> create();

    virtual ~MRBrakePedal() = default;

private:
    MRBrakePedal();
};

using MRBrakePedalSharedPtr = std::shared_ptr<MRBrakePedal>;
}


#endif //MORROW_BRAKE_PEDAL_H
