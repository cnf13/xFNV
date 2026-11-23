#include "BSAudio/SoundMessageStack.h"

#include "BSAudio/BSAudioManager.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

SoundMessageStack::SoundMessageStack() : uiCount(0), bBottomless(false), pTop(nullptr), uiTimeLastAllocated(0) {}

SoundMessageStack::~SoundMessageStack() {
    Clear();
}

BSSoundMessage* SoundMessageStack::Pop() {
    BSSoundMessage* presult = nullptr;
    if (pTop) {
        presult = pTop;
        pTop = static_cast<BSSoundMessage*>(presult->pData);
        presult->pData = nullptr;
        --uiCount;
    }
    else {
        if (bBottomless) {
            BSSoundMessage* presult_2 = new BSSoundMessage;
            presult = presult_2;
            presult_2->pData = nullptr;
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
        pTop->pData = nullptr;
        delete pTop;
    }
}

void SoundMessageStack::Push(BSSoundMessage* apMessage) {
    if (apMessage) {
        if (bBottomless && (uiCount > 512 && GetTickCount() - uiTimeLastAllocated > 1000)) {
            delete apMessage;
        }
        else {
            apMessage->pData = pTop;
            pTop = apMessage;
            ++uiCount;
        }
    }
}
