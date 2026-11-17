#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"

#include "Fallout Shared/TESFile.h"

bool FastNavMeshVertHandle::operator==(const FastNavMeshVertHandle& aNavMeshVertHandle) const {
    return pNavMesh == aNavMeshVertHandle.pNavMesh && usVertex == aNavMeshVertHandle.usVertex;
}

void FastNavMeshVertHandle::Endian() {
    ENDIAN(pNavMesh, false);
    ENDIAN(usVertex, false);
}

// The handle's key: the navmesh's address shifted up 32 bits, the vertex in the top half of the low 32 bits.
int64_t FastNavMeshVertHandle::sub_69B640() {
    return static_cast<int64_t>(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pNavMesh)) << 32) + (usVertex << 16);
}
