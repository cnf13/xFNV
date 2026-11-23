#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBNode.h"

class NiAVObject;
class NiOBBRoot;
class NiPick;

// A leaf of an oriented-bounding-box tree: one triangle, with pointers to its three world-space vertices.
class NiOBBLeaf : public NiOBBNode {
public:
    NiOBBLeaf(
        NiOBBRoot* pRoot,
        const uint16_t* pTriList,
        const NiPoint3* pVertex,
        const NiPoint3* pWVertex,
        uint16_t usTriangle
    );
    ~NiOBBLeaf() override;

    bool IsLeaf() override;
    bool TestCollision(
        NiOBBNode* pTestNode,
        NiAVObject* pkObj0,
        NiAVObject* pkObj1,
        int32_t iMaxDepth,
        int32_t iTestMaxDepth,
        uint32_t uiChangeStamp0,
        uint32_t uiChangeStamp1
    ) override;
    int32_t FindCollisions(
        NiOBBNode* pTestNode,
        NiAVObject* pkRoot0,
        NiAVObject* pkRoot1,
        NiAVObject* pkObj0,
        NiAVObject* pkObj1,
        int32_t iMaxDepth,
        int32_t iTestMaxDepth,
        uint32_t uiChangeStamp0,
        uint32_t uiChangeStamp1,
        bool& bCollision
    ) override;
    int32_t FindIntersections(
        const NiPoint3& origin,
        const NiPoint3& dir,
        NiPick& pick,
        bool& bIntersects,
        NiAVObject* pObj,
        uint32_t uiChangeStamp
    ) override;

    // The triangle-triangle test against pTestLeaf: on a hit sets bCollision, fills an intersection record (both
    // leaves' world vertices, their box normals at the point) and returns the collision callback's result, else
    // CONTINUE_COLLISIONS (protected in the engine).
    int32_t FaceFaceCollision(
        NiOBBLeaf* pTestLeaf,
        NiAVObject* pkRoot0,
        NiAVObject* pkRoot1,
        NiAVObject* pkObj0,
        NiAVObject* pkObj1,
        bool& bCollision
    );

    // The barycentric coordinates of p in the triangle v0 v1 v2, into pCenter. Protected in the engine.
    static float* ComputeBarycenter(const float* v0, const float* v1, const float* v2, const float* p, float* pCenter);
    // Protected in the engine.
    static int32_t TentativeCollision(
        float fDT,
        const float* N,
        float fC,
        const float* V,
        const float* q0,
        const float* q1,
        float* fT0,
        float* p0,
        float* fT1,
        float* p1
    );
    // The earliest time and point at which the moving edge edge0-edge1 meets the triangle pVer.
    // Protected in the engine.
    static int32_t TestEdgeTri(
        const NiPoint3& edge0,
        const NiPoint3& edge1,
        const NiPoint3** pVer,
        const NiPoint3& N,
        float fC,
        const NiPoint3& V,
        float fDT,
        float& fTMin,
        NiPoint3& pmin
    );
    // The earliest contact of the edges of triangle pVer0 with triangle pVer1. Protected in the
    // engine.
    static int32_t FindIntersection(
        const NiPoint3** pVer0,
        const NiPoint3** pVer1,
        const NiPoint3& V,
        float fDT,
        float& fTMin,
        NiPoint3& pmin
    );
    // The earliest contact time and point of the two leaves' moving triangles. Protected in the
    // engine.
    static int32_t FindIntersection(NiOBBLeaf* pLeaf0, NiOBBLeaf* pLeaf1, float& fT, NiPoint3& p);

    const NiPoint3* m_pWorldVertex[3];
};
