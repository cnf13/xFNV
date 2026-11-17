#pragma once

#include "Reconstruction.hpp"

class hkMemoryRouter;

// The Havok memory a worker thread owns (IDA names it BSHavokThreadData; its fields are pHavokMemoryRouter,
// pHavokAlloc and uiOwnerID).
class HavokThreadMemoryStruct {
public:
	hkMemoryRouter* pHavokThreadRouter;
	char* pHavokThreadStackBuffer;
	uint32_t iThreadID;
};
