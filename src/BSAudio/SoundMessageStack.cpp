#include "BSAudio/SoundMessageStack.h"

#include "BSAudio/BSAudioManager.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

SoundMessageStack::SoundMessageStack() : uiCount(0), bBottomless(false), pTop(nullptr), uiTimeLastAllocated(0) {}

SoundMessageStack::~SoundMessageStack() {
    Clear();
}

BSSoundMessage* SoundMessageStack::Pop() {
    BSSoundMessage* presult = 0;
    if (pTop) {
        presult = pTop;
        pTop = static_cast<BSSoundMessage*>(presult->pData);
        presult->pData = 0;
        --uiCount;
    }
    else {
        if (bBottomless) {
            BSSoundMessage* presult_2;
            BSSoundMessage* pMessage =
                static_cast<BSSoundMessage*>(MemoryManager_Singleton.Allocate(sizeof(BSSoundMessage)));
            if (pMessage) {
                presult_2 = ::new (pMessage) BSSoundMessage;
            }
            else {
                presult_2 = 0;
            }
            presult = presult_2;
            presult_2->pData = 0;
            uiTimeLastAllocated = GetTickCount();
        }
    }
    return presult;
}

void SoundMessageStack::Clear() {
    while (this->pTop) {
        BSSoundMessage* pTop = this->pTop;
        this->pTop = static_cast<BSSoundMessage*>(pTop->pData);
        --uiCount;
        pTop->pData = 0;
        if (pTop) {
            pTop->~BSSoundMessage();
            MemoryManager_Singleton.Deallocate(pTop);
        }
    }
}

void SoundMessageStack::Push(BSSoundMessage* apMessage) {
    if (apMessage) {
        if (bBottomless && (uiCount > 512 && GetTickCount() - uiTimeLastAllocated > 1000)) {
            apMessage->~BSSoundMessage();
            MemoryManager_Singleton.Deallocate(apMessage);
        }
        else {
            apMessage->pData = pTop;
            pTop = apMessage;
            ++uiCount;
        }
    }
}
