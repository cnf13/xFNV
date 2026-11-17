#include "Fallout Shared/IDLE_ANIM_ROOT.h"

#include "BSCore/MemoryManager.h"

void IDLE_ANIM_ROOT::operator delete(void* apMemory) {
    ourFree(apMemory);
}
