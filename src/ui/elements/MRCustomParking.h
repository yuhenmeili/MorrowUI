//
// Created by lance on 24-10-29.
//

#ifndef MRCustomParking_H
#define MRCustomParking_H

#include "base/UIWidget.h"

namespace morrow {
class MRCustomParking : public UIWidget {
public:
    static std::shared_ptr<MRCustomParking> create();

private:
    MRCustomParking();
};

using MRCustomParkingSharedPtr = std::shared_ptr<MRCustomParking>;
}

#endif //MRCustomParking_H
