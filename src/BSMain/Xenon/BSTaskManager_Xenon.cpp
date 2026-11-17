#include "BSHavok/bhkThreadMemoryRouter.h"
#include "Fallout Shared/BSTaskThread.h"

// The PC build sets up Havok's per-thread memory only; the PDB's Xbox body also shuts it down after the loop body.
uint32_t BSTaskThreadProc(void* apTaskThread) {
	BSTaskThread* pTaskThread = static_cast<BSTaskThread*>(apTaskThread);
	bhkThreadMemoryRouter::Initialize(&pTaskThread->HavokThreadMemory);
	pTaskThread->ThreadUpdate();
	return 0;
}
