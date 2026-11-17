#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

NiCloningProcess::NiCloningProcess(uint32_t uiHashSize)
	: m_pkCloneMap(new NiTPointerMap<NiObject*, NiObject*>(uiHashSize)),
	  m_pkProcessMap(new NiTPointerMap<NiObject*, bool>(uiHashSize)), m_eCopyType(NiObjectNET::GetDefaultCopyType()),
	  m_cAppendChar(NiObjectNET::GetDefaultAppendCharacter()) {}

NiCloningProcess::~NiCloningProcess() {
	delete m_pkCloneMap;
	delete m_pkProcessMap;
}
