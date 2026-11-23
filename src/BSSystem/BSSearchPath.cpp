#include "BSSystem/BSSearchPath.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSearchPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSSearchPath::BSSearchPath() {}

// Only the base's destructor runs.
BSSearchPath::~BSSearchPath() {}

// The engine has one search directory: the file path, returned once.
bool BSSearchPath::GetNextSearchPath(char* apPath, uint32_t uiStringLen) {
    if (m_uiNextPath) {
        return false;
    }
    strcpy_s(apPath, 260, m_acFilePath);
    ++m_uiNextPath;
    return true;
}

bool BSSearchPath::Access(char* apFile) {
    return true;
}

void BSSearchPath::InitCallback() {
    NiSearchPath::SetSearchPathCreateCallback(&BSSearchPath::GetBSSearchPath);
}

void BSSearchPath::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

NiSearchPath* BSSearchPath::GetBSSearchPath() {
    return ::new BSSearchPath;
}
