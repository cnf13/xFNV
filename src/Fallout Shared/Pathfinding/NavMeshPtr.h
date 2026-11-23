#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

// A smart pointer to a navmesh: NiPointer's reference counting on the navmesh's NiRefObject part.
class NavMeshPtr : public NiPointer<NavMesh> {
public:
    NavMeshPtr();
    NavMeshPtr(NavMesh* apNavMesh);
    // Defined in ExtraDataList.cpp.
    NavMeshPtr(const NavMeshPtr& arPtr);
    ~NavMeshPtr();

    // User-declared copy assignment: performs the base NiPointer<NavMesh>'s assignment and returns *this. Defined in
    // ExtraDataList.cpp.
    NavMeshPtr& operator=(const NavMeshPtr& ptr);
    void DecRefCount();
    bool operator!=(const NavMeshPtr& ptr) const;
    bool operator==(const NavMeshPtr& ptr) const;
    // Hides NiPointer's own conversion; defined out of line.
    operator NavMesh*() const;

    // A pointer holding apNavMesh.
    static NavMeshPtr MakeNavMeshPtr(NavMesh* apNavMesh);
    // Assigns apNavMesh to the existing arPtr through a temporary pointer.
    static void MakeNavMeshPtr(NavMeshPtr& arPtr, NavMesh* apNavMesh);
};
