#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBNode.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionData.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBLeaf.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBRoot.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBox.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>

NiOBBNode::NiOBBNode() : m_pLeft(nullptr), m_pRight(nullptr), m_uiChangeStamp(0) {}

// Computes each triangle's area and centroid, builds the tree through CreateRecursive and frees the scratch arrays.
NiOBBNode::NiOBBNode(
    NiOBBRoot* pRoot,
    int32_t aiTriangles,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    const NiPoint3* pWVertex,
    int32_t iBinSize
)
  : m_uiChangeStamp(0) {
    const uint16_t usTriangles = static_cast<uint16_t>(aiTriangles);
    float* pArea = static_cast<float*>(_NiMalloc(usTriangles * sizeof(float)));
    NiPoint3* pCentroid = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(usTriangles * sizeof(NiPoint3)));
    uint16_t j = 0;
    for (uint16_t i = 0; i < usTriangles; i++) {
        const NiPoint3& kV0 = pVertex[pTriList[j++]];
        const NiPoint3& kV1 = pVertex[pTriList[j++]];
        const NiPoint3& kV2 = pVertex[pTriList[j++]];
        pArea[i] = (kV1 - kV0).Cross(kV2 - kV0).Length() * 0.5f;
        pCentroid[i] = (kV0 + kV1 + kV2) / 3.0f;
    }
    int32_t* pISplit = static_cast<int32_t*>(_NiMalloc(usTriangles * sizeof(int32_t)));
    for (int32_t k = 0; k < usTriangles; k++) {
        pISplit[k] = k;
    }
    int32_t* pOSplit = static_cast<int32_t*>(_NiMalloc(usTriangles * sizeof(int32_t)));
    CreateRecursive(
        pRoot, pTriList, pVertex, pWVertex, pArea, pCentroid, 0, usTriangles - 1, pISplit, pOSplit, iBinSize
    );
    _NiFree(pOSplit);
    _NiFree(pISplit);
    MemoryManager_Singleton.Deallocate(pCentroid);
    _NiFree(pArea);
}

// A node has both children or none.
NiOBBNode::~NiOBBNode() {
    if (m_pLeft) {
        delete m_pLeft;
        delete m_pRight;
    }
}

// An inner node of the tree.
bool NiOBBNode::IsLeaf() {
    return false;
}

bool NiOBBNode::TestCollision(
    NiOBBNode* pTestNode,
    NiAVObject* pkObj0,
    NiAVObject* pkObj1,
    int32_t iMaxDepth,
    int32_t iTestMaxDepth,
    uint32_t uiChangeStamp0,
    uint32_t uiChangeStamp1
) {
    if (!pTestNode) {
        return false;
    }
    if (uiChangeStamp0 != m_uiChangeStamp) {
        m_box.Transform(pkObj0->m_kWorld);
        m_uiChangeStamp = uiChangeStamp0;
    }
    if (uiChangeStamp1 != pTestNode->m_uiChangeStamp) {
        pTestNode->m_box.Transform(pkObj1->m_kWorld);
        pTestNode->m_uiChangeStamp = uiChangeStamp1;
    }
    if (!m_box.TestIntersection(pTestNode->m_box)) {
        return false;
    }
    if (iMaxDepth && iTestMaxDepth) {
        bool v11 = !this->IsLeaf();
        if (v11) {
            bool v13 = pTestNode->IsLeaf();
            NiOBBNode* m_pLeft = this->m_pLeft;
            if (v13) {
                if (m_pLeft && m_pRight) {
                    int32_t iMDm1 = iMaxDepth - 1;
                    return m_pLeft->TestCollision(
                               pTestNode, pkObj0, pkObj1, iMDm1, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1
                           ) ||
                        m_pRight->TestCollision(
                            pTestNode, pkObj0, pkObj1, iMDm1, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1
                        );
                }
            }
            else {
                if (m_pLeft && m_pRight) {
                    int32_t iTMDm1_2 = iTestMaxDepth - 1;
                    int32_t iMDm1_2 = iMaxDepth - 1;
                    return m_pLeft->TestCollision(
                               pTestNode->m_pLeft, pkObj0, pkObj1, iMaxDepth - 1, iTestMaxDepth - 1, uiChangeStamp0,
                               uiChangeStamp1
                           ) ||
                        (this->m_pLeft->TestCollision(
                             pTestNode->m_pRight, pkObj0, pkObj1, iMDm1_2, iTMDm1_2, uiChangeStamp0, uiChangeStamp1
                         ) ||
                         (m_pRight->TestCollision(
                              pTestNode->m_pLeft, pkObj0, pkObj1, iMDm1_2, iTMDm1_2, uiChangeStamp0, uiChangeStamp1
                          ) ||
                          m_pRight->TestCollision(
                              pTestNode->m_pRight, pkObj0, pkObj1, iMDm1_2, iTMDm1_2, uiChangeStamp0, uiChangeStamp1
                          )));
                }
            }
        }
        else {
            if (!pTestNode->IsLeaf() && pTestNode->HasChildren()) {
                int32_t iTMDm1 = iTestMaxDepth - 1;
                return TestCollision(
                           pTestNode->m_pLeft, pkObj0, pkObj1, iMaxDepth, iTMDm1, uiChangeStamp0, uiChangeStamp1
                       ) ||
                    TestCollision(
                           pTestNode->m_pRight, pkObj0, pkObj1, iMaxDepth, iTMDm1, uiChangeStamp0, uiChangeStamp1
                    );
            }
        }
    }
    return true;
}

int32_t NiOBBNode::FindCollisions(
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
    int32_t ret;
    if (!pTestNode) {
        return 0;
    }
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
    if (!iMaxDepth || !iTestMaxDepth) {
        return BoxBoxCollision(pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision);
    }
    bool v13 = !this->IsLeaf();
    if (!v13) {
        if (pTestNode->IsLeaf() || !pTestNode->HasChildren()) {
            ret = BoxBoxCollision(pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision);
            if (ret >= 1) {
                return ret;
            }
        }
        else {
            int32_t iTMDm1 = iTestMaxDepth - 1;
            ret = FindCollisions(
                pTestNode->m_pLeft, pkRoot0, pkRoot1, pkObj0, pkObj1, iMaxDepth, iTMDm1, uiChangeStamp0, uiChangeStamp1,
                bCollision
            );
            if (ret >= 1) {
                return ret;
            }
            ret = FindCollisions(
                pTestNode->m_pRight, pkRoot0, pkRoot1, pkObj0, pkObj1, iMaxDepth, iTMDm1, uiChangeStamp0,
                uiChangeStamp1, bCollision
            );
            if (ret >= 1) {
                return ret;
            }
        }
        return 0;
    }
    bool v15 = pTestNode->IsLeaf();
    NiOBBNode* m_pLeft = this->m_pLeft;
    if (!v15) {
        int32_t iMDm1_2 = iMaxDepth - 1;
        if (m_pLeft && m_pRight) {
            ret = m_pLeft->FindCollisions(
                pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, iMDm1_2, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1,
                bCollision
            );
            if (ret < 1) {
                ret = m_pRight->FindCollisions(
                    pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, iMDm1_2, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1,
                    bCollision
                );
                if (ret < 1) {
                    return 0;
                }
            }
            return ret;
        }
        return BoxBoxCollision(pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision);
    }
    if (!m_pLeft || !m_pRight) {
        return BoxBoxCollision(pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, bCollision);
    }
    int32_t iMDm1 = iMaxDepth - 1;
    ret = m_pLeft->FindCollisions(
        pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, iMDm1, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1, bCollision
    );
    if (ret < 1) {
        ret = m_pRight->FindCollisions(
            pTestNode, pkRoot0, pkRoot1, pkObj0, pkObj1, iMDm1, iTestMaxDepth, uiChangeStamp0, uiChangeStamp1,
            bCollision
        );
        if (ret < 1) {
            return 0;
        }
    }
    return ret;
}

int32_t NiOBBNode::FindIntersections(
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
    int32_t result = m_box.TestIntersection(origin, dir);
    if (result) {
        NiOBBNode* m_pLeft = this->m_pLeft;
        if (!m_pLeft ||
            (result = m_pLeft->FindIntersections(origin, dir, pick, bIntersects, pObj, uiChangeStamp), result < 1)) {
            NiOBBNode* m_pRight = this->m_pRight;
            if (!m_pRight) {
                return 0;
            }
            result = m_pRight->FindIntersections(origin, dir, pick, bIntersects, pObj, uiChangeStamp);
            if (result < 1) {
                return 0;
            }
        }
    }
    return result;
}

bool NiOBBNode::HasChildren() {
    return (bool)((uint8_t)(m_pLeft || m_pRight));
}

// The recursion for the right child runs as a loop: each pass splits the node's triangles, builds its left child, then
// goes on with the right child as the node and the two split arrays swapped.
void NiOBBNode::CreateRecursive(
    NiOBBRoot* pRoot,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    const NiPoint3* pWVertex,
    float* pArea,
    NiPoint3* pCentroid,
    int32_t i0,
    int32_t i1,
    int32_t* pISplit,
    int32_t* pOSplit,
    int32_t iBinSize
) {
    NiOBBNode* pNode = this;
    ComputeOBB(pRoot, pTriList, pVertex, pArea, pCentroid, i0, i1, pISplit);
    while (i1 - i0 >= iBinSize) {
        int32_t j0;
        int32_t j1;
        pNode->DivideTriangles(pCentroid, i0, i1, pISplit, j0, j1, pOSplit);

        if (i0 < j0) {
            NiOBBNode* pLeft = static_cast<NiOBBNode*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBNode)));
            if (pLeft) {
                ::new (pLeft) NiOBBNode;
            }
            pNode->m_pLeft = pLeft;
            pLeft->CreateRecursive(
                pRoot, pTriList, pVertex, pWVertex, pArea, pCentroid, i0, j0, pOSplit, pISplit, iBinSize
            );
        }
        else if (i0 == j0) {
            NiOBBLeaf* pLeaf = static_cast<NiOBBLeaf*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBLeaf)));
            pNode->m_pLeft = pLeaf ? ::new (pLeaf) NiOBBLeaf(pRoot, pTriList, pVertex, pWVertex, pOSplit[i0]) : nullptr;
        }

        if (j1 >= i1) {
            if (j1 == i1) {
                NiOBBLeaf* pLeaf = static_cast<NiOBBLeaf*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBLeaf)));
                pNode->m_pRight =
                    pLeaf ? ::new (pLeaf) NiOBBLeaf(pRoot, pTriList, pVertex, pWVertex, pOSplit[i1]) : nullptr;
            }
            return;
        }

        NiOBBNode* pRight = static_cast<NiOBBNode*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBNode)));
        if (pRight) {
            ::new (pRight) NiOBBNode;
        }
        pNode->m_pRight = pRight;
        pNode = pRight;
        int32_t* pSplit = pISplit;
        pISplit = pOSplit;
        pOSplit = pSplit;
        i0 = j1;
        pNode->ComputeOBB(pRoot, pTriList, pVertex, pArea, pCentroid, i0, i1, pISplit);
    }
    pNode->m_pLeft = nullptr;
    pNode->m_pRight = nullptr;
}

NiMatrix3* NiOBBNode::ComputeCovarianceAndMean(
    NiMatrix3* apResult,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    float* pArea,
    NiPoint3* pCentroid,
    int32_t i0,
    int32_t i1,
    int32_t* pISplit
) {
    NiPoint3 sum;
    sum.x = 0.0f;
    sum.y = 0.0f;
    sum.z = 0.0f;
    float fTotArea = 0.0f;
    float pSumCofactors[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    for (uint16_t i = static_cast<uint16_t>(i0); i <= i1; i++) {
        int32_t usTriangle = pISplit[i];
        int32_t j = 3 * usTriangle;
        const NiPoint3& p = pVertex[pTriList[j++]];
        const NiPoint3& q = pVertex[pTriList[j++]];
        const NiPoint3& r = pVertex[pTriList[j++]];
        const NiPoint3& avr = pCentroid[usTriangle];
        float area = pArea[usTriangle];
        fTotArea += area;
        // sum += area * avr, the product rounded to float first.
        float fX = avr.x * area;
        float fY = avr.y * area;
        float fZ = avr.z * area;
        sum.x += fX;
        sum.y += fY;
        sum.z += fZ;
        pSumCofactors[0] += area * (avr.x * avr.x + p.x * p.x + q.x * q.x + r.x * r.x);
        pSumCofactors[1] += area * (avr.x * avr.y + p.x * p.y + q.x * q.y + r.x * r.y);
        pSumCofactors[2] += area * (avr.x * avr.z + p.x * p.z + q.x * q.z + r.x * r.z);
        pSumCofactors[3] += area * (avr.y * avr.y + p.y * p.y + q.y * q.y + r.y * r.y);
        pSumCofactors[4] += area * (avr.y * avr.z + p.y * p.z + q.y * q.z + r.y * r.z);
        pSumCofactors[5] += area * (avr.z * avr.z + p.z * p.z + q.z * q.z + r.z * r.z);
    }
    float fInverseTotalArea = 1.0f / fTotArea;
    float fScale = 0.25f * fInverseTotalArea;
    NiPoint3& center = m_box.m_center;
    center.x = fInverseTotalArea * sum.x;
    center.y = fInverseTotalArea * sum.y;
    center.z = fInverseTotalArea * sum.z;
    apResult->m_pEntry[0][0] = pSumCofactors[0] * fScale - center.x * center.x;
    apResult->m_pEntry[0][1] = pSumCofactors[1] * fScale - center.x * center.y;
    apResult->m_pEntry[0][2] = pSumCofactors[2] * fScale - center.x * center.z;
    apResult->m_pEntry[1][1] = pSumCofactors[3] * fScale - center.y * center.y;
    apResult->m_pEntry[1][2] = pSumCofactors[4] * fScale - center.y * center.z;
    apResult->m_pEntry[2][2] = pSumCofactors[5] * fScale - center.z * center.z;
    apResult->m_pEntry[1][0] = apResult->m_pEntry[0][1];
    apResult->m_pEntry[2][0] = apResult->m_pEntry[0][2];
    apResult->m_pEntry[2][1] = apResult->m_pEntry[1][2];
    return apResult;
}

// The source's NiOBBNode::ComputeOBB: fits the box to the covariance's eigenvectors, then to the triangles' extent
// along them.
void NiOBBNode::ComputeOBB(
    NiOBBRoot* pRoot,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    float* pArea,
    NiPoint3* pCentroid,
    int32_t i0,
    int32_t i1,
    int32_t* pISplit
) {
    NiOBBox* pBox = &m_box;
    NiPoint3* basis = m_box.m_basis;
    NiMatrix3 C;
    ComputeCovarianceAndMean(&C, pTriList, pVertex, pArea, pCentroid, i0, i1, pISplit);
    float eigenvalue[3];
    NiMatrix3::EigenSolveSymmetric(&C, nullptr, eigenvalue, &basis[0].x);

    float fLowerX = 0.0f;
    float fLowerY = 0.0f;
    float fLowerZ = 0.0f;
    float fUpperX = 0.0f;
    float fUpperY = 0.0f;
    float fUpperZ = 0.0f;
    // The loop counter is the source's unsigned short.
    for (uint16_t i = static_cast<uint16_t>(i0); i <= i1; i++) {
        const int32_t j = 3 * pISplit[i];
        const NiPoint3* apVertex[3] = {&pVertex[pTriList[j]], &pVertex[pTriList[j + 1]], &pVertex[pTriList[j + 2]]};
        for (const NiPoint3* pPoint : apVertex) {
            const float bx = pPoint->x - pBox->m_center.x;
            const float by = pPoint->y - pBox->m_center.y;
            const float bz = pPoint->z - pBox->m_center.z;
            const float x = basis[0].x * bx + by * basis[0].y + bz * basis[0].z;
            const float y = basis[1].y * by + basis[1].x * bx + basis[1].z * bz;
            const float z = bz * basis[2].z + (bx * basis[2].x + by * basis[2].y);
            if (x < fLowerX) {
                fLowerX = x;
            }
            else if (x > fUpperX) {
                fUpperX = x;
            }
            if (y < fLowerY) {
                fLowerY = y;
            }
            else if (y > fUpperY) {
                fUpperY = y;
            }
            if (z < fLowerZ) {
                fLowerZ = z;
            }
            else if (z > fUpperZ) {
                fUpperZ = z;
            }
        }
    }

    const float fDiffX = fUpperX - fLowerX;
    const float fDiffY = fUpperY - fLowerY;
    const float fDiffZ = fUpperZ - fLowerZ;
    const float fRadiusX = fDiffX * 0.5;
    const float fRadiusY = fDiffY * 0.5;
    const float fRadiusZ = fDiffZ * 0.5;
    m_box.m_extent[0] = fRadiusX;
    m_box.m_extent[1] = fRadiusY;
    m_box.m_extent[2] = fRadiusZ;
    const float fSumX = fLowerX + fUpperX;
    const float fSumY = fLowerY + fUpperY;
    const float fSumZ = fLowerZ + fUpperZ;
    const float fDeltaX = fSumX * 0.5;
    const float fDeltaY = fSumY * 0.5;
    const float fDeltaZ = 0.5 * fSumZ;
    const float fB2X = basis[2].x * fDeltaZ;
    const float fB2Y = basis[2].y * fDeltaZ;
    const float fB2Z = fDeltaZ * basis[2].z;
    const float fB1X = basis[1].x * fDeltaY;
    const float fB1Y = basis[1].y * fDeltaY;
    const float fB1Z = fDeltaY * basis[1].z;
    const float fB0X = basis[0].x * fDeltaX;
    const float fB0Y = basis[0].y * fDeltaX;
    const float fB0Z = fDeltaX * basis[0].z;
    const float fB01X = fB0X + fB1X;
    const float fB01Y = fB0Y + fB1Y;
    const float fB01Z = fB0Z + fB1Z;
    const float fOffsetX = fB01X + fB2X;
    const float fOffsetY = fB01Y + fB2Y;
    const float fOffsetZ = fB01Z + fB2Z;
    pBox->m_center.x = fOffsetX + pBox->m_center.x;
    pBox->m_center.y = fOffsetY + pBox->m_center.y;
    pBox->m_center.z = pBox->m_center.z + fOffsetZ;

    NiTransform I;
    I.MakeIdentity();
    pBox->Transform(I);
    m_box.m_pRoot = pRoot;
}

int32_t NiOBBNode::BoxBoxCollision(
    NiOBBNode* pTestNode,
    NiAVObject* pkRoot0,
    NiAVObject* pkRoot1,
    NiAVObject* pkObj0,
    NiAVObject* pkObj1,
    bool& bCollision
) {
    NiCollisionGroup::Intersect intr;
    intr.pkRoot0 = pkRoot0;
    intr.pkObj1 = pkObj1;
    intr.appkTri1 = nullptr;
    intr.appkTri2 = nullptr;
    intr.pkRoot1 = pkRoot1;
    intr.pkObj0 = pkObj0;

    const NiPoint3& velocity0 = m_box.m_pRoot->m_velocity;
    const NiPoint3& velocity1 = pTestNode->m_box.m_pRoot->m_velocity;
    int32_t iIntersects;
    if (NiPoint3_ZERO.x == velocity0.x && NiPoint3_ZERO.y == velocity0.y && NiPoint3_ZERO.z == velocity0.z &&
        velocity1.x == NiPoint3_ZERO.x && velocity1.y == NiPoint3_ZERO.y && velocity1.z == NiPoint3_ZERO.z) {
        // Both boxes stand still: they meet halfway between their centres at time 0.
        intr.fTime = 0.0f;
        const float fSumX = pTestNode->m_box.m_wCenter.x + m_box.m_wCenter.x;
        const float fSumY = pTestNode->m_box.m_wCenter.y + m_box.m_wCenter.y;
        const float fSumZ = pTestNode->m_box.m_wCenter.z + m_box.m_wCenter.z;
        intr.kPoint.x = fSumX * 0.5;
        intr.kPoint.y = fSumY * 0.5;
        intr.kPoint.z = 0.5 * fSumZ;
        iIntersects = 1;
    }
    else {
        iIntersects = m_box.FindIntersection(pTestNode->m_box, intr.fTime, intr.kPoint);
    }

    if (!iIntersects) {
        return NiCollisionGroup::CONTINUE_COLLISIONS;
    }
    bCollision = true;
    m_box.ComputeNormal(intr.kPoint, intr.kNormal0);
    pTestNode->m_box.ComputeNormal(intr.kPoint, intr.kNormal1);
    return CollisionCallback(intr);
}

// The source's NiOBBNode::CollisionCallback: both objects' collide callbacks, the second one with the roots and
// normals swapped; a breakout from either wins over a terminate.
int32_t NiOBBNode::CollisionCallback(NiCollisionGroup::Intersect& intr) {
    int32_t ret0 = NiCollisionGroup::CONTINUE_COLLISIONS;
    int32_t ret1 = NiCollisionGroup::CONTINUE_COLLISIONS;

    if (intr.pkRoot0) {
        // EXE-INLINED 0x6838B0
        NiCollisionData* pkData =
            static_cast<NiCollisionData*>(static_cast<NiCollisionObject*>(intr.pkRoot0->m_spCollisionObject.m_pObject));
        if (!pkData) {
            // EXE-INLINED 0x6838B0
            pkData = static_cast<NiCollisionData*>(
                static_cast<NiCollisionObject*>(intr.pkObj0->m_spCollisionObject.m_pObject)
            );
        }
        if (pkData) {
            NiCollisionGroup::Callback cb0 = pkData->m_pfnCollideCallback;
            if (cb0) {
                ret0 = cb0(intr);
            }
        }
    }

    if (intr.pkRoot1) {
        // EXE-INLINED 0x6838B0
        NiCollisionData* pkData =
            static_cast<NiCollisionData*>(static_cast<NiCollisionObject*>(intr.pkRoot1->m_spCollisionObject.m_pObject));
        if (!pkData) {
            // EXE-INLINED 0x6838B0
            pkData = static_cast<NiCollisionData*>(
                static_cast<NiCollisionObject*>(intr.pkObj1->m_spCollisionObject.m_pObject)
            );
        }
        if (pkData) {
            NiCollisionGroup::Callback cb1 = pkData->m_pfnCollideCallback;
            if (cb1) {
                NiAVObject* pSaveRoot = intr.pkRoot0;
                intr.pkRoot0 = intr.pkRoot1;
                intr.pkRoot1 = pSaveRoot;
                const NiPoint3 saveNormal = intr.kNormal0;
                intr.kNormal0 = intr.kNormal1;
                intr.kNormal1 = saveNormal;
                ret1 = cb1(intr);
            }
        }
    }

    if (ret0 == NiCollisionGroup::CONTINUE_COLLISIONS && ret1 == NiCollisionGroup::CONTINUE_COLLISIONS) {
        return NiCollisionGroup::CONTINUE_COLLISIONS;
    }
    if (ret0 == NiCollisionGroup::BREAKOUT_COLLISIONS || ret1 == NiCollisionGroup::BREAKOUT_COLLISIONS) {
        return NiCollisionGroup::BREAKOUT_COLLISIONS;
    }

    return NiCollisionGroup::TERMINATE_COLLISIONS;
}

void NiOBBNode::DivideTriangles(
    NiPoint3* pCentroid,
    int32_t i0,
    int32_t i1,
    int32_t* pISplit,
    int32_t& j0,
    int32_t& j1,
    int32_t* pOSplit
) {
    int32_t v8 = i1;
    int32_t v10 = i0;
    j0 = i0 - 1;
    int32_t v13 = (i0 + i1) / 2;
    j1 = i1 + 1;
    int32_t v33 = v13;
    int32_t v36 = 0;
    float* p_z = &m_box.m_basis[0].z;
    do {
        if (j0 >= v10 && j1 <= v8) {
            break;
        }
        j0 = v10 - 1;
        j1 = v8 + 1;
        int32_t j1a = v10;
        if (v8 - v10 + 1 >= 4) {
            int32_t v34 = v10 + 2;
            int32_t* pOSplita = &pISplit[v10 + 2];
            do {
                int32_t v18;
                int32_t v21;
                int32_t v24;
                int32_t v27;
                int32_t v16 = *(pOSplita - 2);
                NiPoint3* v17 = &pCentroid[v16];
                float v37 = v17->x - this->m_box.m_center.x;
                float v42 = v17->y - this->m_box.m_center.y;
                float v47 = v17->z - this->m_box.m_center.z;
                float j0a = *(p_z - 1) * v42 + *(p_z - 2) * v37 + v47 * *p_z;
                if (!(j0a > 0.0f)) {
                    if (j0a < 0.0f || j1a > v33) {
                        v18 = --j1;
                    }
                    else {
                        v18 = ++j0;
                    }
                }
                else {
                    v18 = ++j0;
                }
                pOSplit[v18] = v16;
                int32_t v19 = *(pOSplita - 1);
                NiPoint3* v20 = &pCentroid[v19];
                float v38 = v20->x - this->m_box.m_center.x;
                float v43 = v20->y - this->m_box.m_center.y;
                float v48 = v20->z - this->m_box.m_center.z;
                float j0b = *(p_z - 1) * v43 + *(p_z - 2) * v38 + v48 * *p_z;
                if (!(j0b > 0.0f)) {
                    if (j0b < 0.0f || v34 - 1 > v33) {
                        v21 = --j1;
                    }
                    else {
                        v21 = ++j0;
                    }
                }
                else {
                    v21 = ++j0;
                }
                pOSplit[v21] = v19;
                int32_t v22 = *pOSplita;
                NiPoint3* v23 = &pCentroid[*pOSplita];
                float v39 = v23->x - this->m_box.m_center.x;
                float v44 = v23->y - this->m_box.m_center.y;
                float v49 = v23->z - this->m_box.m_center.z;
                float j0c = *(p_z - 1) * v44 + *(p_z - 2) * v39 + v49 * *p_z;
                if (!(j0c > 0.0f)) {
                    if (j0c < 0.0f || v34 > v33) {
                        v24 = --j1;
                    }
                    else {
                        v24 = ++j0;
                    }
                }
                else {
                    v24 = ++j0;
                }
                pOSplit[v24] = v22;
                int32_t v25 = pOSplita[1];
                NiPoint3* v26 = &pCentroid[v25];
                float v40 = v26->x - this->m_box.m_center.x;
                float v45 = v26->y - this->m_box.m_center.y;
                float v50 = v26->z - this->m_box.m_center.z;
                float j0d = *(p_z - 1) * v45 + *(p_z - 2) * v40 + v50 * *p_z;
                if (!(j0d > 0.0f)) {
                    if (j0d < 0.0f || v34 + 1 > v33) {
                        v27 = --j1;
                    }
                    else {
                        v27 = ++j0;
                    }
                }
                else {
                    v27 = ++j0;
                }
                pOSplita += 4;
                v34 += 4;
                pOSplit[v27] = v25;
                v8 = i1;
                j1a += 4;
            }
            while (j1a <= i1 - 3);
            v10 = i0;
        }
        if (j1a <= v8) {
            do {
                int32_t v30;
                int32_t v28 = pISplit[j1a];
                NiPoint3* v29 = &pCentroid[v28];
                float v41 = v29->x - this->m_box.m_center.x;
                float v46 = v29->y - this->m_box.m_center.y;
                float v51 = v29->z - this->m_box.m_center.z;
                float j0e = *(p_z - 1) * v46 + *(p_z - 2) * v41 + v51 * *p_z;
                if (!(j0e > 0.0f)) {
                    if (j0e < 0.0f || j1a > v33) {
                        v30 = --j1;
                    }
                    else {
                        v30 = ++j0;
                    }
                }
                else {
                    v30 = ++j0;
                }
                pOSplit[v30] = v28;
                v8 = i1;
                ++j1a;
            }
            while (j1a <= i1);
            v10 = i0;
        }
        p_z += 3;
        ++v36;
        v13 = (i0 + i1) / 2;
    }
    while (v36 < 3);
    if (v36 == 3 && (j0 < v10 || j1 > v8)) {
        j0 = v13;
        j1 = v13 + 1;
        if (v10 <= v8) {
            int32_t* v31 = &pOSplit[v10];
            int32_t v32 = v8 - v10 + 1;
            do {
                *v31 = pISplit[v31 - pOSplit];
                ++v31;
                --v32;
            }
            while (v32);
        }
    }
}

void NiOBBNode::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// KEEP: the exe's callers (TestEdgeTri at 0xEB7E35) push edx again after the call, relying on this body leaving it
// as it was (it never writes edx); clang does not guarantee that.
float* NiOBBLeaf::ComputeBarycenter(const float* a1, const float* a2, const float* a3, const float* a4, float* a5) {
    float v10 = *a1 - *a3;
    float v12 = a1[1] - a3[1];
    float v13 = a1[2] - a3[2];
    float v14 = *a2 - *a3;
    float v15 = a2[1] - a3[1];
    float v16 = a2[2] - a3[2];
    float v17 = *a4 - *a3;
    float v18 = a4[1] - a3[1];
    float v19 = a4[2] - a3[2];
    float a2a = v13 * v13 + (v10 * v10 + v12 * v12);
    float a1a = v14 * v10 + v15 * v12 + v16 * v13;
    float a3a = v14 * v14 + v15 * v15 + v16 * v16;
    float a4a = v13 * v19 + (v12 * v18 + v10 * v17);
    float v11 = v19 * v16 + (v15 * v18 + v14 * v17);
    double v6 = a1a;
    float a1b = 1.0 / (a3a * a2a - a1a * a1a);
    double v7 = a1b;
    float a1c = (a3a * a4a - v11 * v6) * a1b;
    *a5 = a1c;
    double v8 = a2a * v11 - v6 * a4a;
    double v9 = a1c;
    float a1d = v7 * v8;
    a5[1] = a1d;
    a5[2] = 1.0 - v9 - a1d;
    return a5;
}

int32_t NiOBBLeaf::TentativeCollision(
    float a1,
    const float* a2,
    float a3,
    const float* a4,
    const float* a5,
    const float* a6,
    float* a7,
    float* a8,
    float* a9,
    float* a10
) {
    double v29;
    double v31;
    double v38;
    double v39;
    float v40;
    float v56;
    float v66;
    float v74;
    float v102;
    float v108;
    float v124;
    float v134;
    float v144;
    float a1b;
    double v10 = a1;
    float v47 = a1 - 0.0;
    float a1c = *a4 * 0.0;
    double v14 = a1c;
    float a1d = a4[1] * 0.0;
    double v15 = a1d;
    float a1e = a4[2] * 0.0;
    double v16 = a1e;
    float v75 = *a5 + a1c;
    float v84 = a5[1] + a1d;
    float v93 = a5[2] + a1e;
    float a1f = v84 * a2[1] + v75 * *a2 + v93 * a2[2];
    float a2a = a1f - a3;
    float v76 = v14;
    float v85 = v15;
    float v94 = v16;
    float v49 = *a6 + v76;
    float v59 = a6[1] + v85;
    float v67 = v94 + a6[2];
    float a1g = v49 * *a2 + v59 * a2[1] + v67 * a2[2];
    float a5a = a1g - a3;
    float a1h = *a4 * v10;
    double v17 = a1h;
    int32_t v18 = 0;
    float a1i = a4[1] * v10;
    double v19 = a1i;
    float a1j = a4[2] * v10;
    float v50 = *a5 + a1h;
    float v60 = a5[1] + a1i;
    float v68 = a1j + a5[2];
    float a4b = v50 * *a2 + v60 * a2[1] + v68 * a2[2];
    float a4a = a4b - a3;
    float v78 = v17;
    float v87 = v19;
    float v51 = *a6 + v78;
    float v61 = a6[1] + v87;
    float v69 = a1j + a6[2];
    float a1k = v51 * *a2 + v61 * a2[1] + v69 * a2[2];
    float a3a = a1k - a3;
    double v20 = a5a;
    double v21 = a2a;
    if (a5a * a2a <= 0.0) {
        if (0.0 == v21 && 0.0 == v20) {
            *a7 = 0.0;
            *a9 = 0.0;
            float a1l = *a7;
            float v79 = *a4 * a1l;
            float v88 = a4[1] * a1l;
            float v95 = a1l * a4[2];
            float v52 = *a5 + v79;
            float v62 = a5[1] + v88;
            double v22 = v95 + a5[2];
            *a8 = v52;
            float v70 = v22;
            a8[1] = v62;
            a8[2] = v70;
            double v23 = *a9;
            float v80 = *a4 * v23;
            float v89 = a4[1] * v23;
            float v96 = v23 * a4[2];
            float v53 = *a6 + v80;
            float v63 = a6[1] + v89;
            double v24 = v96 + a6[2];
            *a10 = v53;
            a10[1] = v63;
            float v71 = v24;
            a10[2] = v71;
            return 1;
        }
        *a7 = 0.0;
        v18 = 1;
        float a1m = v21 / (v21 - v20);
        float v118 = *a4 * 0.0;
        float v128 = a4[1] * 0.0;
        float v138 = a4[2] * 0.0;
        float v54 = *a6 * a1m;
        float v64 = a6[1] * a1m;
        float v72 = a6[2] * a1m;
        float a1n = 1.0 - a1m;
        float v81 = *a5 * a1n;
        float v90 = a5[1] * a1n;
        float v97 = a1n * a5[2];
        float v100 = v81 + v54;
        float v106 = v90 + v64;
        float v112 = v97 + v72;
        float v82 = v100 + v118;
        *a8 = v82;
        float v91 = v106 + v128;
        a8[1] = v91;
        float v98 = v112 + v138;
        a8[2] = v98;
    }
    double v26 = a4a;
    if (!(a4a * v21 <= 0.0)) {
        v31 = a4a;
    }
    else {
        if (0.0 == v21 && 0.0 == v26) {
            double v27;
            double v28;
            float v101;
            float v107;
            float v113;
            float v119;
            float v120;
            float v129;
            float v130;
            float v139;
            float v140;
            float a1o;
            *a7 = 0.0;
            *a9 = v10;
            a1o = *a7;
            v119 = *a4 * a1o;
            v129 = a4[1] * a1o;
            v139 = a1o * a4[2];
            v101 = *a5 + v119;
            v107 = a5[1] + v129;
            v27 = v139 + a5[2];
            *a8 = v101;
            a8[1] = v107;
            v113 = v27;
            a8[2] = v113;
            v28 = *a9;
            v120 = *a4 * v28;
            v130 = v28 * a4[1];
            v140 = v28 * a4[2];
            v102 = *a5 + v120;
            v108 = a5[1] + v130;
            v29 = v140 + a5[2];
        LABEL_10:;
            float v114 = v29;
            *a10 = v102;
            a10[1] = v108;
            a10[2] = v114;
            return 1;
        }
        double v30 = v21 * v47 / (v21 - v26);
        v31 = a4a;
        float a1a = v30 + 0.0;
        float v121 = *a4 * a1a;
        float v131 = a4[1] * a1a;
        float v141 = a4[2] * a1a;
        float v55 = *a5 + v121;
        float v65 = a5[1] + v131;
        float v73 = v141 + a5[2];
        if (v18) {
            *a9 = a1a;
            *a10 = v55;
            a10[1] = v65;
            a10[2] = v73;
            if (*a9 < static_cast<double>(*a7)) {
                float a1p = *a7;
                *a7 = *a9;
                *a9 = a1p;
                int32_t v32 = *reinterpret_cast<uint32_t*>(a8);
                int32_t v33 = *(reinterpret_cast<uint32_t*>(a8) + 1);
                int32_t v34 = *(reinterpret_cast<uint32_t*>(a8) + 2);
                *a8 = *a10;
                a8[1] = a10[1];
                a8[2] = a10[2];
                *reinterpret_cast<uint32_t*>(a10) = v32;
                *(reinterpret_cast<uint32_t*>(a10) + 1) = v33;
                *(reinterpret_cast<uint32_t*>(a10) + 2) = v34;
            }
            return 1;
        }
        *a7 = a1a;
        *a8 = v55;
        a8[1] = v65;
        a8[2] = v73;
        v18 = 1;
    }
    double v35 = a3a;
    if (!(a3a * v20 <= 0.0)) {
        v39 = a3a;
    LABEL_28:;
        if (!(v39 * v31 <= 0.0)) {
            if (!v18) {
                return 0;
            }
        }
        else {
            double v44;
            if (0.0 == v31) {
                v44 = v31;
                if (0.0 == v39) {
                    *a7 = v10;
                    *a9 = v10;
                    float a1s = *a7;
                    float v125 = *a4 * a1s;
                    float v135 = a1s * a4[1];
                    float v145 = a1s * a4[2];
                    float v104 = *a5 + v125;
                    float v110 = a5[1] + v135;
                    double v45 = v145 + a5[2];
                    *a8 = v104;
                    float v116 = v45;
                    a8[1] = v110;
                    a8[2] = v116;
                    double v46 = *a9;
                    float v126 = *a4 * v46;
                    float v136 = v46 * a4[1];
                    float v146 = v46 * a4[2];
                    v102 = *a6 + v126;
                    v108 = a6[1] + v136;
                    v29 = v146 + a6[2];
                    goto LABEL_10;
                }
            }
            else {
                v44 = v31;
            }
            float a1t = v44 / (v44 - v39);
            float v148 = *a4 * v10;
            float v149 = a4[1] * v10;
            float v150 = a4[2] * v10;
            float v105 = *a6 * a1t;
            float v111 = a6[1] * a1t;
            float v117 = a6[2] * a1t;
            float a1u = 1.0 - a1t;
            float v127 = *a5 * a1u;
            float v137 = a5[1] * a1u;
            float v147 = a1u * a5[2];
            float v83 = v127 + v105;
            float v92 = v137 + v111;
            float v99 = v147 + v117;
            v66 = v92 + v149;
            v74 = v99 + v150;
            if (v18) {
                v40 = v83 + v148;
                goto LABEL_24;
            }
            *a7 = v10;
            float v57 = v83 + v148;
            *a8 = v57;
            a8[1] = v66;
            a8[2] = v74;
        }
        *a9 = *a7;
        *a10 = *a8;
        a10[1] = a8[1];
        a10[2] = a8[2];
        return 1;
    }
    if (0.0 == v20 && 0.0 == v35) {
        *a7 = 0.0;
        *a9 = v10;
        float a1q = *a7;
        float v122 = *a4 * a1q;
        float v132 = a1q * a4[1];
        float v142 = a1q * a4[2];
        float v103 = *a6 + v122;
        float v109 = a6[1] + v132;
        double v36 = v142 + a6[2];
        *a8 = v103;
        a8[1] = v109;
        float v115 = v36;
        a8[2] = v115;
        double v37 = *a9;
        float v123 = *a4 * v37;
        float v133 = v37 * a4[1];
        float v143 = v37 * a4[2];
        v102 = *a6 + v123;
        v108 = a6[1] + v133;
        v29 = v143 + a6[2];
        goto LABEL_10;
    }
    v38 = v20 * v47 / (v20 - v35);
    v39 = a3a;
    a1b = v38 + 0.0;
    v124 = *a4 * a1b;
    v134 = a4[1] * a1b;
    v144 = a4[2] * a1b;
    v56 = *a6 + v124;
    v40 = v56;
    v66 = a6[1] + v134;
    v74 = v144 + a6[2];
    if (!v18) {
        *a7 = a1b;
        *a8 = v56;
        a8[1] = v66;
        a8[2] = v74;
        v18 = 1;
        goto LABEL_28;
    }
    v10 = a1b;
LABEL_24:;
    *a9 = v10;
    *a10 = v40;
    a10[1] = v66;
    a10[2] = v74;
    if (!(*a9 < static_cast<double>(*a7))) {
        return 1;
    }
    float a1r = *a7;
    *a7 = *a9;
    *a9 = a1r;
    int32_t v41 = *reinterpret_cast<uint32_t*>(a8);
    int32_t v42 = *(reinterpret_cast<uint32_t*>(a8) + 1);
    int32_t v43 = *(reinterpret_cast<uint32_t*>(a8) + 2);
    *a8 = *a10;
    a8[1] = a10[1];
    a8[2] = a10[2];
    *reinterpret_cast<uint32_t*>(a10) = v41;
    *(reinterpret_cast<uint32_t*>(a10) + 1) = v42;
    *(reinterpret_cast<uint32_t*>(a10) + 2) = v43;
    return 1;
}

// 1 - fS is rounded to a float only where it scales p0 (NiPoint3::operator*(float) takes a float); in the barycentric
// and time blends it stays unrounded.
int32_t NiOBBLeaf::TestEdgeTri(
    const NiPoint3& edge0,
    const NiPoint3& edge1,
    const NiPoint3** pVer,
    const NiPoint3& N,
    float fC,
    const NiPoint3& V,
    float fDT,
    float& fTMin,
    NiPoint3& pmin
) {
    float fT0;
    float fT1;
    NiPoint3 p0;
    NiPoint3 p1;
    if (!TentativeCollision(fDT, &N.x, fC, &V.x, &edge0.x, &edge1.x, &fT0, &p0.x, &fT1, &p1.x)) {
        return 0;
    }
    float pBary0[3];
    float pBary1[3];
    ComputeBarycenter(&pVer[0]->x, &pVer[1]->x, &pVer[2]->x, &p0.x, pBary0);
    ComputeBarycenter(&pVer[0]->x, &pVer[1]->x, &pVer[2]->x, &p1.x, pBary1);
    float fS;
    if (pBary0[0] < 0.0f) {
        if (pBary1[0] < 0.0f) {
            return 0;
        }
        fS = pBary0[0] / (pBary0[0] - pBary1[0]);
        float fA1 = (1.0f - fS) * pBary0[1] + fS * pBary1[1];
        if (!(fA1 >= 0.0f)) {
            return 0;
        }
        float fA2 = (1.0f - fS) * pBary0[2] + fS * pBary1[2];
        if (!(fA2 >= 0.0f)) {
            return 0;
        }
    }
    else if (pBary0[1] < 0.0f) {
        if (pBary1[1] < 0.0f) {
            return 0;
        }
        fS = pBary0[1] / (pBary0[1] - pBary1[1]);
        float fA0 = (1.0f - fS) * pBary0[0] + fS * pBary1[0];
        if (!(fA0 >= 0.0f)) {
            return 0;
        }
        float fA2 = (1.0f - fS) * pBary0[2] + fS * pBary1[2];
        if (!(fA2 >= 0.0f)) {
            return 0;
        }
    }
    else if (pBary0[2] < 0.0f) {
        if (pBary1[2] < 0.0f) {
            return 0;
        }
        fS = pBary0[2] / (pBary0[2] - pBary1[2]);
        float fA0 = (1.0f - fS) * pBary0[0] + fS * pBary1[0];
        if (!(fA0 >= 0.0f)) {
            return 0;
        }
        float fA1 = (1.0f - fS) * pBary0[1] + fS * pBary1[1];
        if (!(fA1 >= 0.0f)) {
            return 0;
        }
    }
    else {
        if (fT0 < fTMin) {
            fTMin = fT0;
            pmin = p0;
        }
        return 1;
    }
    float fT = fT0 * (1.0f - fS) + fT1 * fS;
    if (fT < fTMin) {
        fTMin = fT;
        // pmin = p0 * (1 - fS) + p1 * fS, each product and the sum rounded to float.
        float fX1 = p1.x * fS;
        float fY1 = p1.y * fS;
        float fZ1 = p1.z * fS;
        float fOneMinusS = 1.0f - fS;
        float fX0 = p0.x * fOneMinusS;
        float fY0 = p0.y * fOneMinusS;
        float fZ0 = p0.z * fOneMinusS;
        pmin.x = fX0 + fX1;
        pmin.y = fY0 + fY1;
        pmin.z = fZ0 + fZ1;
    }
    return 1;
}

// The triangle pVer1's plane (N, fC), then each edge of pVer0 against it.
int32_t NiOBBLeaf::FindIntersection(
    const NiPoint3** pVer0,
    const NiPoint3** pVer1,
    const NiPoint3& V,
    float fDT,
    float& fTMin,
    NiPoint3& pmin
) {
    const NiPoint3* v7 = pVer1[0];
    const NiPoint3* v8 = pVer1[1];
    float v19 = v8->x - pVer1[0]->x;
    const NiPoint3* v10 = pVer0[1];
    float v20 = v8->y - v7->y;
    const NiPoint3* a7 = pVer1[2];
    float v21 = v8->z - v7->z;
    float v16 = a7->x - v7->x;
    float v17 = a7->y - v7->y;
    float v18 = a7->z - v7->z;
    NiPoint3 N;
    N.x = v18 * v20 - v17 * v21;
    N.y = v21 * v16 - v18 * v19;
    N.z = v19 * v17 - v16 * v20;
    float fC = v7->y * N.y + v7->x * N.x + v7->z * N.z;
    int32_t iResult = TestEdgeTri(*pVer0[0], *v10, pVer1, N, fC, V, fDT, fTMin, pmin);
    iResult = TestEdgeTri(*pVer0[0], *pVer0[2], pVer1, N, fC, V, fDT, fTMin, pmin) | iResult;
    return iResult | TestEdgeTri(*pVer0[1], *pVer0[2], pVer1, N, fC, V, fDT, fTMin, pmin);
}

// Takes leaf 1's contact unless fT1 >= fT0, so an unordered pair takes leaf 1's, where Gamebryo's source tests fT0 <=
// fT1.
// KEEP: an uninitialised read. The triangle FindIntersection returns 1 without writing pmin when TestEdgeTri's time is
// not below fTMin (a NaN or infinite time against the initial FLT_MAX), and the exe then reads p0 or p1 (0xEB84DF,
// 0xEB8542).
int32_t NiOBBLeaf::FindIntersection(NiOBBLeaf* pLeaf0, NiOBBLeaf* pLeaf1, float& fT, NiPoint3& p) {
    float fDT = pLeaf0->m_box.m_pRoot->m_fDeltaTime;
    const NiPoint3& V0 = pLeaf0->m_box.m_pRoot->m_velocity;
    const NiPoint3& V1 = pLeaf1->m_box.m_pRoot->m_velocity;
    NiPoint3 V;
    float fT0 = 3.4028235e+38f;
    NiPoint3 p0;
    V.x = V0.x - V1.x;
    V.y = V0.y - V1.y;
    V.z = V0.z - V1.z;
    int32_t iRes0 = FindIntersection(pLeaf0->m_pWorldVertex, pLeaf1->m_pWorldVertex, V, fDT, fT0, p0);
    float fT1 = 3.4028235e+38f;
    NiPoint3 p1;
    V.x = V1.x - V0.x;
    V.y = V1.y - V0.y;
    V.z = V1.z - V0.z;
    int32_t iRes1 = FindIntersection(pLeaf1->m_pWorldVertex, pLeaf0->m_pWorldVertex, V, fDT, fT1, p1);
    bool bUse1;
    if (iRes0) {
        bUse1 = iRes1 && !(fT1 >= fT0);
    }
    else if (iRes1) {
        bUse1 = true;
    }
    else {
        return 0;
    }
    if (bUse1) {
        // p = p1 + fT1 * V0, the product and the sum rounded to float.
        fT = fT1;
        float fX = V0.x * fT1;
        float fY = V0.y * fT1;
        float fZ = V0.z * fT1;
        float fPX = p1.x + fX;
        float fPY = p1.y + fY;
        float fPZ = p1.z + fZ;
        p.x = fPX;
        p.y = fPY;
        p.z = fPZ;
    }
    else {
        // p = p0 + fT0 * V1.
        fT = fT0;
        float fX = V1.x * fT0;
        float fY = V1.y * fT0;
        float fZ = V1.z * fT0;
        float fPX = p0.x + fX;
        float fPY = p0.y + fY;
        float fPZ = p0.z + fZ;
        p.x = fPX;
        p.y = fPY;
        p.z = fPZ;
    }
    return 1;
}
