#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A point of view used by the navmesh search, with its edges and duplicate POVs.
struct NavMeshPOV {
    // The constructor sets POV_TYPE_INVALID.
    enum POV_TYPE : int32_t {
        POV_TYPE_INVALID = -1,
        POV_TYPE_VERTEX = 0,
        POV_TYPE_PORTAL = 1,
        POV_TYPE_OBSTACLE = 2,
        POV_TYPE_START = 3,
        POV_TYPE_GOAL = 4,
        POV_TYPE_AVOIDNODE = 5,
    };

    NavMeshPOV();
    ~NavMeshPOV();

    static void operator delete(void* apMemory);

    POV_TYPE eType;
    NiPoint3 Center;
    float fRadius;
    NiPoint3 Edge1;
    NiPoint3 Edge2;
    float fCostFactor;
    BSSimpleArray<NavMeshPOV*, 1024>* pDuplicates;
    uint16_t iVertexIndex;
    uint8_t padding36[2];
};
