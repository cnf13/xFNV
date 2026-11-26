#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// The buffer is untyped bytes: the 4-byte type, then the portal.
void EdgeExtraInfo::SetFromBuffer(TESFile* apFile, char* apChunkBuffer) {
    // The char buffer is read as a whole EDGE_EXTRA_INFO_TYPE, not as a single char.
    eType = *reinterpret_cast<EDGE_EXTRA_INFO_TYPE*>(apChunkBuffer);
    if (apFile->QEndian()) {
        BSEndian<EDGE_EXTRA_INFO_TYPE>(eType, false);
    }
    NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
    Portal.Load(NavMeshInfoMap_, apFile, apChunkBuffer + 4);
}

void EdgeExtraInfo::SetFromBufferOld(TESFile* apFile, char* apChunkBuffer) {
    eType = EIT_PORTAL;
    NavMeshInfoMap* navMeshInfoMap = TES::pInstance->GetNavMeshInfoMap();
    Portal.Load(navMeshInfoMap, apFile, apChunkBuffer);
}
