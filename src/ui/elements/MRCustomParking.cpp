//
// Created by lance on 24-10-29.
//

#include "MRCustomParking.h"

namespace morrow
{
MRCustomParkingSharedPtr MRCustomParking::create()
{
    return std::shared_ptr<MRCustomParking>(new MRCustomParking());
}

MRCustomParking::MRCustomParking()
{
    m_widgetType = "MRCustomParking";
    m_material->setShader("custom_parking");
}
}
