#pragma once

#include <cstddef>
#include <cstdint>

struct FastNavMeshTriHandle;
struct FastNavMeshVertHandle;
class NavMesh;
struct NavMeshTriangle;

// A triangle of a navmesh, named by the mesh and the triangle's index in it.
struct FastNavMeshTriHandle {
    // Leaves the handle unset, as the game does.
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
    // Leaves the handle unset, as the game does.
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

    // Copies the mesh and the vertex; the padding bytes are deliberately left unset, as the game does.
    FastNavMeshVertHandle(const FastNavMeshVertHandle& aVertHandle)
      : pNavMesh(aVertHandle.pNavMesh), usVertex(aVertHandle.usVertex) {}

    FastNavMeshVertHandle(NavMesh* apNavMesh, uint16_t ausVertex) : pNavMesh(apNavMesh), usVertex(ausVertex) {}

    // The key NiTMapBase's generic KeyToHashIndex hashes (`(size_t)key`): the low 32 bits of sub_69B640's key, with the
    // vertex in its top half, as KeyToHashIndex truncates sub_69B640's result.
    explicit operator size_t() const {
        return static_cast<size_t>(usVertex) << 16;
    }
    // Compares the mesh and the vertex.
    bool operator==(const FastNavMeshVertHandle& aNavMeshVertHandle) const;
    // Swaps pNavMesh and usVertex.
    void Endian();
    int64_t sub_69B640();

    NavMesh* pNavMesh;
    uint16_t usVertex;
    uint8_t padding06[2];
};

// BSSimpleArray<FastNavMeshVertHandle, 1024>::DefaultEqualFunction: FastNavMeshVertHandle::operator== on the two items.
// Its one use is as the compare BSSimpleArray<FastNavMeshVertHandle, 1024>::IsInArray passes to Find; the type is that
// array's EqualFunction.
bool DefaultEqualFunction(const FastNavMeshVertHandle* apItem, const FastNavMeshVertHandle* apValue);
