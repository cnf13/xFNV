#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"

#include "Fallout Shared/TESFile.h"

bool FastNavMeshVertHandle::operator==(const FastNavMeshVertHandle& aNavMeshVertHandle) const {
    return pNavMesh == aNavMeshVertHandle.pNavMesh && usVertex == aNavMeshVertHandle.usVertex;
}

void FastNavMeshVertHandle::Endian() {
    BSEndian(pNavMesh, false);
    BSEndian(usVertex, false);
}
