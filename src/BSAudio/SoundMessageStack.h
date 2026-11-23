#pragma once

#include <cstdint>

class BSSoundMessage;

// A stack of sound messages: how many it holds, whether it is bottomless and the top message.
class SoundMessageStack {
public:
    SoundMessageStack();
    ~SoundMessageStack();

    BSSoundMessage* Pop();
    // Empties the stack.
    void Clear();
    void Push(BSSoundMessage* apMessage);

    uint32_t uiCount;
    bool bBottomless;
    BSSoundMessage* pTop;
    // The tick count of the last message Pop allocated; Push drops a message
    // when more than a second has passed since it.
    uint32_t uiTimeLastAllocated;
};
