#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BSOcclusionQuery;

// A bounding sphere the shader accumulator keeps an occlusion query for: the sphere of a reference, the query that
// tests it and the last result.
struct ReferenceVolume {
    NiPoint3 Center;
    float fRadius;
    uint32_t uiReferenceID;
    BSOcclusionQuery* pOcclusionQuery;
    bool bOccluded;
    bool bWaiting;
    bool bPerformTest;
    uint32_t uiPixelCount;
};
