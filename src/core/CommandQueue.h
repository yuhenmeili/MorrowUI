//
// Created by lance on 2023/11/29.
//

#ifndef MORROW_CORE_COMMANDQUEUE_H_
#define MORROW_CORE_COMMANDQUEUE_H_


#include <cstdint>
#include <functional>
#include <string>
#include <memory>

struct Command;
using CommandSharedPtr = std::shared_ptr<Command>;

struct Command {
    CommandSharedPtr next;
    std::function<void()> func;
};

class CommandQueue {
public:
    CommandQueue();

    bool empty() const;

    uint32_t size() const;

    CommandSharedPtr front();

    CommandSharedPtr back();

    void push(CommandSharedPtr command);

    CommandSharedPtr pop();

private:
    CommandSharedPtr pHead;
    CommandSharedPtr pTail;
    uint32_t count = 0;
};


#endif //MORROW_CORE_COMMANDQUEUE_H_
