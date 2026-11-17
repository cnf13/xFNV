#pragma once

#include "BSCore/BSSimpleArray.h"

#include <stdint.h>

class NavMeshInfo;

// A triangle of a navmesh, by the navmesh's info record.
struct NavMeshTriHandle {
	// The default constructor leaves the members unset, as NavMeshEdgeHandle's empty constructor does for this part.
	NavMeshTriHandle() = default;

	NavMeshTriHandle(NavMeshInfo* apNavMeshInfo, uint16_t ausTriangle)
		: pNavMeshInfo(apNavMeshInfo), usTriangle(ausTriangle) {}

	NavMeshTriHandle(const NavMeshTriHandle& aTriHandle)
		: pNavMeshInfo(aTriHandle.pNavMeshInfo), usTriangle(aTriHandle.usTriangle) {}

	NavMeshInfo* pNavMeshInfo;
	uint16_t usTriangle;
};

// One edge (0-2) of a navmesh triangle, by the navmesh's info record.
struct NavMeshEdgeHandle : public NavMeshTriHandle {
	// Leaves the handle unset, as the game does.
	NavMeshEdgeHandle() {}
	NavMeshEdgeHandle(const NavMeshEdgeHandle& aHandle) : NavMeshTriHandle(aHandle), iEdgeIndex(aHandle.iEdgeIndex) {}
	// The handle of aFastEdgeHandle's edge: its navmesh's info record, its triangle and its edge.
	NavMeshEdgeHandle(const FastNavMeshEdgeHandle& aFastEdgeHandle);

	// Compares the info record, the triangle and the edge.
	bool operator==(const NavMeshEdgeHandle& aNavMeshEdgeHandle) const;
	// Kept a named member rather than operator= because a user-declared operator= would turn every copy of a handle
	// into this call. Copies arOther into this handle and returns it.
	NavMeshEdgeHandle* Copy(const NavMeshEdgeHandle& arOther);

	int32_t iEdgeIndex;
};

// A vertex of a navmesh, by the navmesh's info record.
struct NavMeshVertHandle {
	NavMeshInfo* pNavMeshInfo;
	uint16_t usVertex;
};
