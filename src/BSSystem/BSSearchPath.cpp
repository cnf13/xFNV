#include "BSSystem/BSSearchPath.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSearchPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSSearchPath::BSSearchPath() {}

// Only the base's destructor runs.
BSSearchPath::~BSSearchPath() {}

// The engine has one search directory: the file path, returned once. The exe copies with the buffer size 260, not
// uiStringLen.
bool BSSearchPath::GetNextSearchPath(char* apPath, uint32_t uiStringLen) {
	if (m_uiNextPath) {
		return false;
	}
	strcpy_s(apPath, 260, m_acFilePath);
	++m_uiNextPath;
	return true;
}

bool BSSearchPath::Access(char* apFile) {
	return 1;
}

void BSSearchPath::InitCallback() {
	NiSearchPath::SetSearchPathCreateCallback(&BSSearchPath::GetBSSearchPath);
}

void BSSearchPath::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
NiSearchPath* BSSearchPath::GetBSSearchPath() {
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSSearchPath));
	if (!pMemory) {
		return nullptr;
	}
	return ::new (pMemory) BSSearchPath;
}
