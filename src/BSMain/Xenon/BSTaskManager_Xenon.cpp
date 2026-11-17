#include "BSHavok/bhkThreadMemoryRouter.h"
#include "Fallout Shared/BSTaskThread.h"

// Only sets up Havok's per-thread memory; it is not shut down after the loop body.
uint32_t BSTaskThreadProc(void* apTaskThread) {
    BSTaskThread* pTaskThread = static_cast<BSTaskThread*>(apTaskThread);
    bhkThreadMemoryRouter::Initialize(&pTaskThread->HavokThreadMemory);
    pTaskThread->ThreadUpdate();
    return 0;
}
