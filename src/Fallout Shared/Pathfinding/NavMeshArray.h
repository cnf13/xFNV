#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"

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
    // Constructs the navmesh at aiNavMeshIndex in apResult and returns it.
    NavMeshPtr* GetNavMeshByIndex(NavMeshPtr* apResult, uint32_t aiNavMeshIndex);
    // Adds every navmesh to the garbage collector, then clears the array and frees its buffer.
    void AddToGarbageCollector();
    // Clears the navmesh's cell, adds it to the garbage collector and removes it.
    void RemoveNavMeshByIndex(uint32_t aiNavMeshIndex);

    // The deleting destructor frees the array with ourFree.
    static void operator delete(void* apMemory);

    BSSimpleArray<NavMeshPtr, 1024> NavMeshes;
};
