#include "Fallout Shared/Pathfinding/NavMeshArray.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/GarbageCollector.h"

NavMeshArray::~NavMeshArray() {
    AddToGarbageCollector();
}

// The callee destroys the by-value apNavMesh where it returns, so the body does not release it itself.
uint32_t NavMeshArray::AddNavMesh(NavMeshPtr apNavMesh) {
    return NavMeshes.Add(apNavMesh);
}

// As AddNavMesh, the by-value apNavMesh is destroyed where the function returns; GetNavMeshIndex's by-value copy is
// destroyed by GetNavMeshIndex.
void NavMeshArray::RemoveNavMesh(NavMeshPtr apNavMesh) {
    // SWEEP-INLINE 0x42FA20
    uint32_t uiIndex = GetNavMeshIndex(apNavMesh);
    if (uiIndex != 0xFFFFFFFF) {
        RemoveNavMeshByIndex(uiIndex);
    }
}

uint32_t NavMeshArray::GetNavMeshIndex(NavMeshPtr apNavMesh) {
    for (uint32_t uiIndex = 0; uiIndex < NavMeshes.iSize; ++uiIndex) {
        if (NavMeshes.GetAt(uiIndex) == apNavMesh) {
            return uiIndex;
        }
    }
    return 0xFFFFFFFF;
}

uint32_t NavMeshArray::GetNavMeshCount() {
    return NavMeshes.iSize;
}

NavMeshPtr* NavMeshArray::GetNavMeshByIndex(NavMeshPtr* apResult, uint32_t aiNavMeshIndex) {
    if (aiNavMeshIndex < NavMeshes.iSize) {
        ::new (apResult) NavMeshPtr(NavMeshes.GetAt(aiNavMeshIndex));
    }
    else {
        NavMeshPtr::MakeNavMeshPtr(apResult, nullptr);
    }
    return apResult;
}

void NavMeshArray::AddToGarbageCollector() {
    for (uint32_t uiIndex = 0; uiIndex < NavMeshes.iSize; ++uiIndex) {
        GarbageCollector::Add(NavMeshes.GetAt(uiIndex));
    }
    NavMeshes.Clear(true);
}

void NavMeshArray::RemoveNavMeshByIndex(uint32_t aiNavMeshIndex) {
    NavMeshes.GetAt(aiNavMeshIndex)->pParentCell = nullptr;
    GarbageCollector::Add(NavMeshes.GetAt(aiNavMeshIndex));
    NavMeshes.Remove(aiNavMeshIndex, true);
}

void NavMeshArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}
