#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// The buffer is untyped bytes: the type's dword, then the portal.
void EdgeExtraInfo::SetFromBuffer(TESFile* apFile, char* apChunkBuffer) {
	// EXE-CAST: char as EDGE_EXTRA_INFO_TYPE (the exe reads the buffer's dword)
	eType = *reinterpret_cast<EDGE_EXTRA_INFO_TYPE*>(apChunkBuffer);
	if (apFile->QEndian()) {
		ENDIAN<EDGE_EXTRA_INFO_TYPE>(eType, 0);
	}
	NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
	Portal.Load(NavMeshInfoMap_, apFile, apChunkBuffer + 4);
}

void EdgeExtraInfo::SetFromBufferOld(TESFile* apFile, char* apChunkBuffer) {
	eType = EIT_PORTAL;
	NavMeshInfoMap* navMeshInfoMap = TES::pInstance->GetNavMeshInfoMap();
	(&Portal)->Load(navMeshInfoMap, apFile, apChunkBuffer);
}
