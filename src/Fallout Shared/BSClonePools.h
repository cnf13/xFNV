#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiAVObject;

// Pools of ready-made clones of loaded models: one pool per model, handing out clones and taking them back.
class BSClonePools {
public:
	// The clones of one model that are not in use.
	struct ClonePool {
		ClonePool();
		~ClonePool();

		// Empties the pool: drops each clone, then destroys the list.
		void Free();

		// The exe's deleting destructor (0x4B08E0) frees through ourFree.
		static void operator delete(void* apMemory);

		NiObject* pObject;
		BSSimpleList<NiPointer<NiRefObject>> kAvailableClones;
	};

	BSClonePools();

	void KillClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount);
	void MakeClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount);
	NiAVObject* RequestClone(NiAVObject* apLoadedObject);

	// Inferred type: return: BSClonePools* from 0x4b0eaa (BSClonePools::pInstance)
	static BSClonePools* GetInstance(bool a1);

	BSSimpleList<ClonePool*> ClonePools;

	// IDA's pSingleton.
	static inline BSClonePools* pInstance;
};
