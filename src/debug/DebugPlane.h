//
// Created by 0060328 on 25-10-27.
//

#ifndef DEBUGPLANE_H
#define DEBUGPLANE_H
#include <memory>

namespace morrow {
class Window;
struct FrameState;
class MRLabel;

class DebugPlane {
public:
    void initialize(std::shared_ptr<Window> window);

    void update(std::shared_ptr<FrameState> frame_state);

private:
    std::shared_ptr<MRLabel> m_frameLabel;

    std::shared_ptr<MRLabel> m_batchLabel;
};

} // morrow

#endif //DEBUGPLANE_H
