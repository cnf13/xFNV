#pragma once

#include <cstdint>

class NiAVObject;

// Keeps aiCloneCount clones of a model ready in the clone pools for as long as it lives. Not polymorphic.
class BSCloneReserver {
public:
	BSCloneReserver(NiAVObject* apModel, uint32_t aiCloneCount);
	~BSCloneReserver();

	void DecRefCount();
	// Adds a reference by atomically incrementing iRefCount.
	void IncRefCount();

protected:
	// Asks the clone pools for iCloneCount clones of pModel, if there is a model and a count.
	void Init();

public:
	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	NiAVObject* pModel;
	uint32_t iCloneCount;
	int32_t iRefCount;
};

// NiPointer of a reserver takes and releases references through the reserver's own IncRefCount and DecRefCount.
inline void NiIncRefCount(BSCloneReserver* apReserver) {
	apReserver->IncRefCount();
}

inline void NiDecRefCount(BSCloneReserver* apReserver) {
	apReserver->DecRefCount();
}
