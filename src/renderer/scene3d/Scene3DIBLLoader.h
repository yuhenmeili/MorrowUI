#ifndef MORROW_GUI_SCENE3D_IBL_LOADER_H
#define MORROW_GUI_SCENE3D_IBL_LOADER_H

#include <string>

#include "Scene3DPassContext.h"

namespace morrow {
class Scene3DIBLLoader {
public:
    static bool loadFromDirectory(const std::string& iblDirectory,
                                  Scene3DIBLState& outIBL,
                                  float intensity = 1.0f);
};
} // namespace morrow

#endif // MORROW_GUI_SCENE3D_IBL_LOADER_H

