#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Reconstruction.hpp"

// The navmeshes of one cell (a BSSimpleArray<NavMeshPtr> in the engine).
class NavMeshArray {
public:
	NavMeshArray() {}
	// Hands every navmesh to the garbage collector; the compiler destroys NavMeshes.
	~NavMeshArray();

	// Appends apNavMesh; returns its index. The engine takes the
	// pointer by value and releases it.
	uint32_t AddNavMesh(NavMeshPtr apNavMesh);
	// Removes apNavMesh from the array. The engine takes the
	// pointer by value and releases it.
	void RemoveNavMesh(NavMeshPtr apNavMesh);
	// The index of apNavMesh in the array, or 0xFFFFFFFF.
	uint32_t GetNavMeshIndex(NavMeshPtr apNavMesh);
	// The number of navmeshes.
	uint32_t GetNavMeshCount();
	// The navmesh at aiNavMeshIndex, constructed in apResult (the by-value result; thiscall,
	// ret 8, 0x464F60).
	NavMeshPtr* GetNavMeshByIndex(NavMeshPtr* apResult, uint32_t aiNavMeshIndex);
	// IDA name; the PDB's ClearAllMeshes or RemoveAllMeshPointers (not settled). Adds every navmesh to the garbage
	// collector, then clears the array and frees its buffer (thiscall, 0x69BCA0).
	void AddToGarbageCollector();
	// Clears the navmesh's cell, adds it to the garbage collector and removes it.
	void RemoveNavMeshByIndex(uint32_t aiNavMeshIndex);

	// The exe's deleting destructor frees the array with ourFree.
	static void operator delete(void* apMemory);

	BSSimpleArray<NavMeshPtr, 1024> NavMeshes;
};
