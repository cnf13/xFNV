#pragma once

#include <cstdint>

class hkMemoryRouter;

// The Havok memory a worker thread owns.
class HavokThreadMemoryStruct {
public:
	hkMemoryRouter* pHavokThreadRouter;
	char* pHavokThreadStackBuffer;
	uint32_t iThreadID;
};
