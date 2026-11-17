#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A smart pointer to a navmesh: NiPointer's reference counting on the navmesh's NiRefObject part.
class NavMeshPtr : public NiPointer<NavMesh> {
public:
	NavMeshPtr();
	NavMeshPtr(NavMesh* apNavMesh);
	// Defined in ExtraDataList.cpp.
	NavMeshPtr(const NavMeshPtr& arPtr);
	~NavMeshPtr();

	// The PDB's user-declared copy assignment: the base NiPointer<NavMesh>'s assignment
	// (0x42FDC0), whose this it returns (thiscall, ret 4, 0x42F4C0). Defined in ExtraDataList.cpp.
	NavMeshPtr& operator=(const NavMeshPtr& ptr);
	void DecRefCount();
	bool operator!=(const NavMeshPtr& ptr) const;
	bool operator==(const NavMeshPtr& ptr) const;
	// The PDB's own conversion, hiding NiPointer's: it returns the pointer through the one-field getter (0x559450),
	// which makes it an exe copy of its own (the debug build calls NavMeshPtr::operator NavMesh* in
	// GarbageCollector::Add, ObstacleTaskData::ReplaceNavMesh and Pathing's searches, which call 0x458B50).
	operator NavMesh*() const;

	// A pointer holding apNavMesh, constructed in apResult (the by-value result; cdecl,
	// 0x464FC0).
	static NavMeshPtr* MakeNavMeshPtr(NavMeshPtr* apResult, NavMesh* apNavMesh);
	// The exe's second function of this name: assigns apNavMesh to the existing arPtr through a
	// temporary pointer (cdecl, 0x690890).
	static void MakeNavMeshPtr(NavMeshPtr& arPtr, NavMesh* apNavMesh);
};
