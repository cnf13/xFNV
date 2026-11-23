#pragma once

#include <cstdint>

// A navmesh point of view the POV search removes: its master and minor indices.
struct RemoveIndex {
    RemoveIndex();
    RemoveIndex(int32_t aiMasterIndex, int32_t aiMinorIndex);

    // Both indices equal aIndex's. The array's default equality function
    // (BSSimpleArray<RemoveIndex,1024>::DefaultEqualFunction<RemoveIndex>, the Find callback of
    // NavMeshPOVSearch::BuildPOVListForTriangles) calls it.
    bool operator==(const RemoveIndex& aIndex) const;

    int32_t iMasterIndex;
    int32_t iMinorIndex;
};
