#ifndef MORROW_GUI_MRSEPARATOR_H
#define MORROW_GUI_MRSEPARATOR_H

#include "MRColor.h"

namespace morrow {

class MRHSeparator : public MRColor {
public:
    static std::shared_ptr<MRHSeparator> create();

private:
    MRHSeparator();
};

using MRHSeparatorSharedPtr = std::shared_ptr<MRHSeparator>;

class MRVSeparator : public MRColor {
public:
    static std::shared_ptr<MRVSeparator> create();

private:
    MRVSeparator();
};

using MRVSeparatorSharedPtr = std::shared_ptr<MRVSeparator>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSEPARATOR_H
