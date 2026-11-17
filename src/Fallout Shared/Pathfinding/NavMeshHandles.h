#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NavMeshInfo;

// A triangle of a navmesh, by the navmesh's info record (PDB names).
struct NavMeshTriHandle {
	// The exe has no default constructor of its own: NavMeshEdgeHandle's empty one leaves this part unset.
	NavMeshTriHandle() = default;

	NavMeshTriHandle(NavMeshInfo* apNavMeshInfo, uint16_t ausTriangle)
		: pNavMeshInfo(apNavMeshInfo), usTriangle(ausTriangle) {}

	NavMeshTriHandle(const NavMeshTriHandle& aTriHandle)
		: pNavMeshInfo(aTriHandle.pNavMeshInfo), usTriangle(aTriHandle.usTriangle) {}

	NavMeshInfo* pNavMeshInfo;
	uint16_t usTriangle;
};

// One edge (0-2) of a navmesh triangle, by the navmesh's info record (PDB names).
struct NavMeshEdgeHandle : public NavMeshTriHandle {
	// Leaves the handle unset, as the exe's empty constructor does.
	NavMeshEdgeHandle() {}
	NavMeshEdgeHandle(const NavMeshEdgeHandle& aHandle) : NavMeshTriHandle(aHandle), iEdgeIndex(aHandle.iEdgeIndex) {}
	// The handle of aFastEdgeHandle's edge: its navmesh's info record, its triangle and its edge.
	NavMeshEdgeHandle(const FastNavMeshEdgeHandle& aFastEdgeHandle);

	// Compares the info record, the triangle and the edge.
	bool operator==(const NavMeshEdgeHandle& aNavMeshEdgeHandle) const;
	// IDA name (FastNavMeshEdgeHandle::Copy); the PDB's NavMeshEdgeHandle::operator=, kept a named member because a
	// user-declared operator= would turn every src copy of a handle into this call. Copies arOther into this handle
	// and returns it (thiscall, ret 4).
	NavMeshEdgeHandle* Copy(const NavMeshEdgeHandle& arOther);

	int32_t iEdgeIndex;
};

// A vertex of a navmesh, by the navmesh's info record (PDB names). The exe's empty constructor is shared (folded).
struct NavMeshVertHandle {
	NavMeshInfo* pNavMeshInfo;
	uint16_t usVertex;
};
