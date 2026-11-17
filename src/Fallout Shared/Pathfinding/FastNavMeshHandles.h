#pragma once

#include "Reconstruction.hpp"

struct FastNavMeshTriHandle;
struct FastNavMeshVertHandle;
class NavMesh;
struct NavMeshTriangle;

// A triangle of a navmesh, named by the mesh and the triangle's index in it.
struct FastNavMeshTriHandle {
	// Leaves the handle unset, as the exe's empty constructor does.
	FastNavMeshTriHandle() {}

	FastNavMeshTriHandle(const FastNavMeshTriHandle& aTriHandle)
		: pNavMesh(aTriHandle.pNavMesh), usTriangle(aTriHandle.usTriangle) {}

	FastNavMeshTriHandle(NavMesh* apNavMesh, uint16_t ausTriangle) : pNavMesh(apNavMesh), usTriangle(ausTriangle) {}

	bool operator==(const FastNavMeshTriHandle& aNavMeshEdgeHandle) const;
	NavMeshTriangle* GetTriangle();

	NavMesh* pNavMesh;
	uint16_t usTriangle;
};

// One edge (0-2) of a navmesh triangle.
struct FastNavMeshEdgeHandle : public FastNavMeshTriHandle {
	// Leaves the handle unset, as the exe's empty constructor does.
	FastNavMeshEdgeHandle() {}

	FastNavMeshEdgeHandle(const FastNavMeshEdgeHandle& aNavMeshEdgeHandle)
		: FastNavMeshTriHandle(aNavMeshEdgeHandle), iEdgeIndex(aNavMeshEdgeHandle.iEdgeIndex) {}

	FastNavMeshEdgeHandle(NavMesh* apNavMesh, uint16_t ausTriangle, int32_t aiEdgeIndex)
		: FastNavMeshTriHandle(apNavMesh, ausTriangle), iEdgeIndex(aiEdgeIndex) {}

	// Copies arOther into this handle and returns it.
	FastNavMeshEdgeHandle* Copy(const FastNavMeshEdgeHandle& arOther);

	int32_t iEdgeIndex;
};

// A vertex of a navmesh, named by the mesh and the vertex's index in it.
struct FastNavMeshVertHandle {
	FastNavMeshVertHandle() = default;

	// Copies the mesh and the vertex; the padding bytes stay unset, as in the exe.
	FastNavMeshVertHandle(const FastNavMeshVertHandle& aVertHandle)
		: pNavMesh(aVertHandle.pNavMesh), usVertex(aVertHandle.usVertex) {}

	FastNavMeshVertHandle(NavMesh* apNavMesh, uint16_t ausVertex) : pNavMesh(apNavMesh), usVertex(ausVertex) {}

	// The key NiTMapBase's generic KeyToHashIndex hashes (`(size_t)key`): the low dword of sub_69B640's key, the
	// vertex in its top half, as the exe's KeyToHashIndex (0x69B620) truncates 0x69B640's result.
	explicit operator size_t() const { return static_cast<size_t>(usVertex) << 16; }
	// Compares the mesh and the vertex (the PDB's const operator==, thiscall, ret 4).
	bool operator==(const FastNavMeshVertHandle& aNavMeshVertHandle) const;
	// Swaps the dword and the word.
	void Endian();
	int64_t sub_69B640();

	NavMesh* pNavMesh;
	uint16_t usVertex;
	uint8_t padding06[2];
};

// The PDB's BSSimpleArray<FastNavMeshVertHandle, 1024>::DefaultEqualFunction<FastNavMeshVertHandle> (cdecl):
// FastNavMeshVertHandle::operator== (0x6D4CA0) on the two items. Its one use is as the compare
// BSSimpleArray<FastNavMeshVertHandle, 1024>::IsInArray (0x6A1460) passes to Find; the type is that array's
// EqualFunction.
bool DefaultEqualFunction(const FastNavMeshVertHandle* apItem, const FastNavMeshVertHandle* apValue);
