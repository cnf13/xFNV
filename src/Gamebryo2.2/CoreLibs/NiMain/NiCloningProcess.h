#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Reconstruction.hpp"

// State of one NiObject::Clone pass: originals mapped to their clones, and the objects already processed.
class NiCloningProcess : public NiMemObject {
public:
	enum CloneRelationBehavior {
		CLONE_RELATION_NONE = 0,
		CLONE_RELATION_CLONEDONLY = 1,
		CLONE_RELATION_ALL = 2,
	};

	// NiCloningProcess(uiHashSize): builds the two maps with uiHashSize buckets (thiscall,
	// ret 4, 0x4AD0C0).
	// The hybrid's implicit default constructor (no zero-parameter Construct). Sites construct the process again
	// with FNVD_RECONSTRUCT, which runs the destructor first: it deletes the maps, so they start null.
	NiCloningProcess() : m_pkCloneMap(nullptr), m_pkProcessMap(nullptr) {}
	NiCloningProcess(uint32_t uiHashSize);
	~NiCloningProcess();

	NiTPointerMap<NiObject*, NiObject*>* m_pkCloneMap;
	NiTPointerMap<NiObject*, bool>* m_pkProcessMap;
	NiObjectNET::CopyType m_eCopyType;
	char m_cAppendChar;
};
