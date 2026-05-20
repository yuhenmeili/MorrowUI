//
// Created by lance on 2023/11/29.
//

#include "CommandQueue.h"

CommandQueue::CommandQueue() : pHead(nullptr), pTail(nullptr), count(0)
{

}

bool CommandQueue::empty() const
{
    return (pHead == nullptr);
}

uint32_t CommandQueue::size() const
{
    return count;
}

CommandSharedPtr CommandQueue::front()
{
    return pHead;
}

CommandSharedPtr CommandQueue::back()
{
    return pTail;
}

void CommandQueue::push(CommandSharedPtr command)
{
    command->next = nullptr;
    if (pHead == nullptr) {
        pHead = command;
    } else {
        pTail->next = command;
    }
    pTail = command;
    count++;
}

CommandSharedPtr CommandQueue::pop()
{
    auto result = pHead;
    pHead = pHead->next;
    if (pHead == nullptr) {
        pTail = nullptr;
    }
    count--;
    return result;
}
