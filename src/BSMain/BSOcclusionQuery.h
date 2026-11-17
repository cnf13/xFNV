#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

struct IDirect3DQuery9;

// A hardware occlusion query: counts the pixels that pass the depth test.
// The Xbox build also has uiNumTiles at 0x0; the PC build has only the query.
class BSOcclusionQuery {
public:
	BSOcclusionQuery();
	~BSOcclusionQuery();

	// Reads the query's result into apData; true when it is ready (thiscall, ret 8, 0xC4EDF0).
	// The exe tests only the flag's byte (0xC4EE00) and passes D3DGETDATA_FLUSH when it is set, so bool fits.
	bool GetResult(uint32_t* pResult, bool bFlush);
	bool Begin();
	void End();
	bool GetResultNoWait(uint32_t* pResult, bool bFlush);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	IDirect3DQuery9* pQuery;

	static inline char bQueryInProgress;
};
