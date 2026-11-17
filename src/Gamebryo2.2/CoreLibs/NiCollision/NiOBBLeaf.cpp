#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBLeaf.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionUtils.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBNode.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBRoot.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

// Gamebryo's constructor: the box of triangle usTriangle, its first edge and the edges' plane as the basis.
NiOBBLeaf::NiOBBLeaf(
    NiOBBRoot* pRoot,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    const NiPoint3* pWVertex,
    uint16_t usTriangle
) {
    auto Dot = [](const NiPoint3& kA, const NiPoint3& kB) { return kA.x * kB.x + kA.y * kB.y + kA.z * kB.z; };
    uint16_t usStart = static_cast<uint16_t>(3 * usTriangle);
    uint16_t usIndex = pTriList[usStart];
    m_pWorldVertex[0] = &pWVertex[usIndex];
    const NiPoint3& v0 = pVertex[usIndex];
    usStart++;
    usIndex = pTriList[usStart];
    m_pWorldVertex[1] = &pWVertex[usIndex];
    const NiPoint3& v1 = pVertex[usIndex];
    usStart++;
    usIndex = pTriList[usStart];
    m_pWorldVertex[2] = &pWVertex[usIndex];
    const NiPoint3& v2 = pVertex[usIndex];

    NiPoint3& center = m_box.m_center;
    NiPoint3* basis = m_box.m_basis;
    float* extent = m_box.m_extent;
    center = (v0 + v1 + v2) / 3.0f;
    basis[0] = v1 - v0;
    basis[0].Unitize();
    basis[1] = v2 - v0;
    basis[1] -= basis[0] * Dot(basis[0], basis[1]);
    basis[1].Unitize();
    basis[2] = basis[0].Cross(basis[1]);

    NiPoint3 lower{0.0f, 0.0f, 0.0f};
    NiPoint3 upper{0.0f, 0.0f, 0.0f};
    const NiPoint3* apVertex[3] = {&v0, &v1, &v2};
    for (const NiPoint3* pV : apVertex) {
        NiPoint3 b = *pV - center;
        for (int32_t i = 0; i < 3; i++) {
            float f = Dot(basis[i], b);
            if (f < lower[i]) {
                lower[i] = f;
            }
            else if (f > upper[i]) {
                upper[i] = f;
            }
        }
    }
    NiPoint3 radius = (upper - lower) * 0.5f;
    extent[0] = radius.x;
    extent[1] = radius.y;
    extent[2] = radius.z;
    NiPoint3 delta = (upper + lower) * 0.5f;
    center += basis[0] * delta.x + basis[1] * delta.y + basis[2] * delta.z;

    NiTransform I;
    I.MakeIdentity();
    m_box.Transform(I);
    m_box.m_pRoot = pRoot;
}

NiOBBLeaf::~NiOBBLeaf() {}

bool NiOBBLeaf::IsLeaf() {
    return true;
}

bool NiOBBLeaf::TestCollision(
    NiOBBNode* pTestNode,
    NiAVObject* pkObj0,
    NiAVObject* pkObj1,
    int32_t iMaxDepth,
    int32_t iTestMaxDepth,
    uint32_t uiChangeStamp0,
    uint32_t uiChangeStamp1
) {
    if (uiChangeStamp0 != m_uiChangeStamp) {
        m_box.Transform(pkObj0->m_kWorld);
        m_uiChangeStamp = uiChangeStamp0;
    }
    if (uiChangeStamp1 != pTestNode->m_uiChangeStamp) {
        pTestNode->m_box.Transform(pkObj1->m_kWorld);
        pTestNode->m_uiChangeStamp = uiChangeStamp1;
    }
    if (!m_box.TestIntersection(pTestNode->m_box)) {
        return 0;
    }
    if (pTestNode->IsLeaf()) {
        return 1;
    }
    if (!iTestMaxDepth) {
        return 1;
    }
    NiOBBNode* m_pLeft = pTestNode->m_pLeft;
    if (!m_pLeft && !pTestNode->m_pRight) {
        return 1;
    }
    return TestCollision(m_pLeft, pkObj0, pkObj1, iMaxDepth, iTestMaxDepth - 1, uiChangeStamp0, uiChangeStamp1) ||
        TestCollision(
               pTestNode->m_pRight, pkObj0, pkObj1, iMaxDepth, iTestMaxDepth - 1, uiChangeStamp0, uiChangeStamp1
        );
}

int32_t NiOBBLeaf::FindCollisions(
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
) {
    if (uiChangeStamp0 != m_uiChangeStamp) {
        m_box.Transform(pkObj0->m_kWorld);
        m_uiChangeStamp = uiChangeStamp0;
    }
    if (uiChangeStamp1 != pTestNode->m_uiChangeStamp) {
        pTestNode->m_box.Transform(pkObj1->m_kWorld);
        pTestNode->m_uiChangeStamp = uiChangeStamp1;
    }
    int32_t ret = m_box.TestIntersection(pTestNode->m_box);
    if (ret) {
        if (pTestNode->IsLeaf()) {
            if (iMaxDepth < 0 && iTestMaxDepth < 0) {
                return FaceFaceCollision(
                    static_cast<NiOBBLeaf*>(pTestNode), pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision
                );
            }
        }
        else {
            if (iTestMaxDepth) {
                NiOBBNode* m_pLeft = pTestNode->m_pLeft;
                if (m_pLeft || pTestNode->m_pRight) {
                    ret = FindCollisions(
                        m_pLeft, pkRoot0, pkRoot1, pkObj0, pkObj1, iMaxDepth, iTestMaxDepth - 1, uiChangeStamp0,
                        uiChangeStamp1, bCollision
                    );
                    if (ret < 1) {
                        int32_t ret_2 = FindCollisions(
                            pTestNode->m_pRight, pkRoot0, pkRoot1, pkObj0, pkObj1, iMaxDepth, iTestMaxDepth - 1,
                            uiChangeStamp0, uiChangeStamp1, bCollision
                        );
                        return ret_2 < 1 ? 0 : ret_2;
                    }
                    return ret;
                }
            }
        }
        return BoxBoxCollision(pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision);
    }
    return ret;
}

int32_t NiOBBLeaf::FindIntersections(
    const NiPoint3& origin,
    const NiPoint3& dir,
    NiPick& pick,
    bool& bIntersects,
    NiAVObject* pObj,
    uint32_t uiChangeStamp
) {
    if (uiChangeStamp != m_uiChangeStamp) {
        m_box.Transform(pObj->m_kWorld);
        m_uiChangeStamp = uiChangeStamp;
    }
    NiPoint3 intersect;
    float fLineParam;
    float fTriParam1;
    float fTriParam2;
    // NiCollisionUtils::IntersectTriangle; it returns 0 or 1.
    bIntersects = NiCollisionUtils::IntersectTriangle(
        &origin, &dir, m_pWorldVertex[0], m_pWorldVertex[1], &m_pWorldVertex[2]->x, pick.m_bFrontOnly, &intersect,
        &fLineParam, &fTriParam1, &fTriParam2
    );
    if (bIntersects) {
        NiPick::Record* pRecord = pick.m_pickResults.AddNew(pObj);
        pRecord->m_intersect = intersect;
        pRecord->m_fDistance = fLineParam;
        if (pick.m_bReturnNormal) {
            // normal = (v1 - v0).Cross(v2 - v0), each component rounded to float.
            const NiPoint3* pV0 = m_pWorldVertex[0];
            const NiPoint3* pV1 = m_pWorldVertex[1];
            const NiPoint3* pV2 = m_pWorldVertex[2];
            float e0x = pV1->x - pV0->x;
            float e0y = pV1->y - pV0->y;
            float e0z = pV1->z - pV0->z;
            float e1x = pV2->x - pV0->x;
            float e1y = pV2->y - pV0->y;
            float e1z = pV2->z - pV0->z;
            NiPoint3 normal;
            normal.x = e1z * e0y - e0z * e1y;
            normal.y = e0z * e1x - e1z * e0x;
            normal.z = e1y * e0x - e1x * e0y;
            normal.Unitize();
            pRecord->m_normal = normal;
        }
    }
    return 0;
}

int32_t NiOBBLeaf::FaceFaceCollision(
    NiOBBLeaf* pTestLeaf,
    NiAVObject* pkRoot0,
    NiAVObject* pkRoot1,
    NiAVObject* pkObj0,
    NiAVObject* pkObj1,
    bool& bCollision
) {
    NiCollisionGroup::Intersect intr;
    intr.pkRoot1 = pkRoot1;
    intr.pkObj0 = pkObj0;
    // Clears the triangle pointers before the search.
    intr.appkTri1 = nullptr;
    intr.appkTri2 = nullptr;
    intr.pkRoot0 = pkRoot0;
    intr.pkObj1 = pkObj1;
    if (!FindIntersection(this, pTestLeaf, intr.fTime, intr.kPoint)) {
        return NiCollisionGroup::CONTINUE_COLLISIONS;
    }
    bCollision = true;
    intr.appkTri1 = m_pWorldVertex;
    intr.appkTri2 = pTestLeaf->m_pWorldVertex;
    m_box.ComputeNormal(intr.kPoint, intr.kNormal0);
    pTestLeaf->m_box.ComputeNormal(intr.kPoint, intr.kNormal1);
    return CollisionCallback(intr);
}
