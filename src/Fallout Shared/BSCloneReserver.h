#pragma once

#include "Reconstruction.hpp"

class NiAVObject;

// Keeps aiCloneCount clones of a model ready in the clone pools for as long as it lives. Not polymorphic. IDA names
// the members pModel, uiCloneCount and uiRefCount and Init MakeClones.
class BSCloneReserver {
public:
	BSCloneReserver(NiAVObject* apModel, uint32_t aiCloneCount);
	~BSCloneReserver();

	void DecRefCount();
	// Adds a reference (NiAtomicIncrement of iRefCount, +8; thiscall, ret 0, 0x92C870; called by NiPointer's
	// assignments 0x92C8B0 and 0x92C900).
	void IncRefCount();

protected:
	// Asks the clone pools for iCloneCount clones of pModel, if there is a model and a count.
	void Init();

public:
	// The exe's deleting destructor (0x4B0F40) frees through ourFree.
	static void operator delete(void* apMemory);

	NiAVObject* pModel;
	uint32_t iCloneCount;
	int32_t iRefCount;
};

// NiPointer of a reserver takes and releases references through the reserver's own IncRefCount and DecRefCount
// (the exe's NiPointer<BSCloneReserver> assignments 0x92C8B0 and 0x92C900 call them).
inline void NiIncRefCount(BSCloneReserver* apReserver) {
	apReserver->IncRefCount();
}

inline void NiDecRefCount(BSCloneReserver* apReserver) {
	apReserver->DecRefCount();
}
