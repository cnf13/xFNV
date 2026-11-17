#include "Fallout Shared/Pathfinding/NavMeshArray.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/GarbageCollector.h"

NavMeshArray::~NavMeshArray() {
	AddToGarbageCollector();
}

// The exe's cleanup-only C++ EH frame is left out. The callee destroys the by-value apNavMesh where it returns (the
// exe's 0x42FA40 call is that destructor), so the body does not release it itself.
uint32_t NavMeshArray::AddNavMesh(NavMeshPtr apNavMesh) {
	return NavMeshes.Add(apNavMesh);
}

// The exe's cleanup-only C++ EH frame is left out. As AddNavMesh, the by-value apNavMesh is destroyed where the
// function returns; GetNavMeshIndex's by-value copy is destroyed by GetNavMeshIndex.
void NavMeshArray::RemoveNavMesh(NavMeshPtr apNavMesh) {
	// SWEEP-INLINE 0x42FA20
	uint32_t uiIndex = GetNavMeshIndex(apNavMesh);
	if (uiIndex != 0xFFFFFFFF) {
		RemoveNavMeshByIndex(uiIndex);
	}
}

// The exe's cleanup-only C++ EH frame is left out. As AddNavMesh, the by-value apNavMesh is destroyed where the
// function returns. The item count is read through the one-field getter 0x44DDC0, written as the field.
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

// The item count is read through the one-field getter 0x44DDC0 (a trivial accessor), written as the field.
NavMeshPtr* NavMeshArray::GetNavMeshByIndex(NavMeshPtr* apResult, uint32_t aiNavMeshIndex) {
	if (aiNavMeshIndex < NavMeshes.iSize) {
		::new (apResult) NavMeshPtr(NavMeshes.GetAt(aiNavMeshIndex));
	} else {
		NavMeshPtr::MakeNavMeshPtr(apResult, nullptr);
	}
	return apResult;
}

// IDA name; the PDB's name is ClearAllMeshes or RemoveAllMeshPointers, which one is not settled. The item count is
// read through the one-field getter 0x44DDC0, written as the field.
void NavMeshArray::AddToGarbageCollector() {
	for (uint32_t uiIndex = 0; uiIndex < NavMeshes.iSize; ++uiIndex) {
		GarbageCollector::Add(NavMeshes.GetAt(uiIndex));
	}
	NavMeshes.Clear(true);
}

// The exe clears the navmesh's cell through two one-field accessors (operator-> 0x559450, the setter 0x7037C0 of
// +0x24), written as the members.
void NavMeshArray::RemoveNavMeshByIndex(uint32_t aiNavMeshIndex) {
	NavMeshes.GetAt(aiNavMeshIndex)->pParentCell = nullptr;
	GarbageCollector::Add(NavMeshes.GetAt(aiNavMeshIndex));
	NavMeshes.Remove(aiNavMeshIndex, true);
}

void NavMeshArray::operator delete(void* apMemory) {
	ourFree(apMemory);
}
