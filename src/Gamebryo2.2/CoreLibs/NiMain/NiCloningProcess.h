#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

// State of one NiObject::Clone pass: originals mapped to their clones, and the objects already processed.
class NiCloningProcess : public NiMemObject {
public:
	enum CloneRelationBehavior {
		CLONE_RELATION_NONE = 0,
		CLONE_RELATION_CLONEDONLY = 1,
		CLONE_RELATION_ALL = 2,
	};

	// NiCloningProcess(uiHashSize) builds the two maps with uiHashSize buckets.
	NiCloningProcess() : m_pkCloneMap(nullptr), m_pkProcessMap(nullptr) {}
	NiCloningProcess(uint32_t uiHashSize);
	~NiCloningProcess();

	NiTPointerMap<NiObject*, NiObject*>* m_pkCloneMap;
	NiTPointerMap<NiObject*, bool>* m_pkProcessMap;
	NiObjectNET::CopyType m_eCopyType;
	char m_cAppendChar;
};
