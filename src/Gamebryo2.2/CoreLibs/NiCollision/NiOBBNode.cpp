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
#include <limits>

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
    NiPoint3* pCentroid = static_cast<NiPoint3*>(MemoryManager::Instance().Allocate(usTriangles * sizeof(NiPoint3)));
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
        pRoot,
        pTriList,
        pVertex,
        pWVertex,
        pArea,
        pCentroid,
        0,
        usTriangles - 1,
        pISplit,
        pOSplit,
        iBinSize
    );
    _NiFree(pOSplit);
    _NiFree(pISplit);
    MemoryManager::Instance().Deallocate(pCentroid);
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
                               pTestNode,
                               pkObj0,
                               pkObj1,
                               iMDm1,
                               iTestMaxDepth,
                               uiChangeStamp0,
                               uiChangeStamp1
                           ) ||
                        m_pRight->TestCollision(
                            pTestNode,
                            pkObj0,
                            pkObj1,
                            iMDm1,
                            iTestMaxDepth,
                            uiChangeStamp0,
                            uiChangeStamp1
                        );
                }
            }
            else {
                if (m_pLeft && m_pRight) {
                    int32_t iTMDm1_2 = iTestMaxDepth - 1;
                    int32_t iMDm1_2 = iMaxDepth - 1;
                    return m_pLeft->TestCollision(
                               pTestNode->m_pLeft,
                               pkObj0,
                               pkObj1,
                               iMaxDepth - 1,
                               iTestMaxDepth - 1,
                               uiChangeStamp0,
                               uiChangeStamp1
                           ) ||
                        (this->m_pLeft->TestCollision(
                             pTestNode->m_pRight,
                             pkObj0,
                             pkObj1,
                             iMDm1_2,
                             iTMDm1_2,
                             uiChangeStamp0,
                             uiChangeStamp1
                         ) ||
                         (m_pRight->TestCollision(
                              pTestNode->m_pLeft,
                              pkObj0,
                              pkObj1,
                              iMDm1_2,
                              iTMDm1_2,
                              uiChangeStamp0,
                              uiChangeStamp1
                          ) ||
                          m_pRight->TestCollision(
                              pTestNode->m_pRight,
                              pkObj0,
                              pkObj1,
                              iMDm1_2,
                              iTMDm1_2,
                              uiChangeStamp0,
                              uiChangeStamp1
                          )));
                }
            }
        }
        else {
            if (!pTestNode->IsLeaf() && pTestNode->HasChildren()) {
                int32_t iTMDm1 = iTestMaxDepth - 1;
                return TestCollision(
                           pTestNode->m_pLeft,
                           pkObj0,
                           pkObj1,
                           iMaxDepth,
                           iTMDm1,
                           uiChangeStamp0,
                           uiChangeStamp1
                       ) ||
                    TestCollision(
                           pTestNode->m_pRight,
                           pkObj0,
                           pkObj1,
                           iMaxDepth,
                           iTMDm1,
                           uiChangeStamp0,
                           uiChangeStamp1
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
                pTestNode->m_pLeft,
                pkRoot0,
                pkRoot1,
                pkObj0,
                pkObj1,
                iMaxDepth,
                iTMDm1,
                uiChangeStamp0,
                uiChangeStamp1,
                bCollision
            );
            if (ret >= 1) {
                return ret;
            }
            ret = FindCollisions(
                pTestNode->m_pRight,
                pkRoot0,
                pkRoot1,
                pkObj0,
                pkObj1,
                iMaxDepth,
                iTMDm1,
                uiChangeStamp0,
                uiChangeStamp1,
                bCollision
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
                pTestNode,
                pkRoot0,
                pkRoot1,
                pkObj0,
                pkObj1,
                iMDm1_2,
                iTestMaxDepth,
                uiChangeStamp0,
                uiChangeStamp1,
                bCollision
            );
            if (ret < 1) {
                ret = m_pRight->FindCollisions(
                    pTestNode,
                    pkRoot0,
                    pkRoot1,
                    pkObj0,
                    pkObj1,
                    iMDm1_2,
                    iTestMaxDepth,
                    uiChangeStamp0,
                    uiChangeStamp1,
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
        pTestNode,
        pkRoot0,
        pkRoot1,
        pkObj0,
        pkObj1,
        iMDm1,
        iTestMaxDepth,
        uiChangeStamp0,
        uiChangeStamp1,
        bCollision
    );
    if (ret < 1) {
        ret = m_pRight->FindCollisions(
            pTestNode,
            pkRoot0,
            pkRoot1,
            pkObj0,
            pkObj1,
            iMDm1,
            iTestMaxDepth,
            uiChangeStamp0,
            uiChangeStamp1,
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
            ((result = m_pLeft->FindIntersections(origin, dir, pick, bIntersects, pObj, uiChangeStamp)) < 1)) {
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
    return m_pLeft || m_pRight;
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
            NiOBBNode* pLeft = ::new NiOBBNode;
            pNode->m_pLeft = pLeft;
            pLeft->CreateRecursive(
                pRoot,
                pTriList,
                pVertex,
                pWVertex,
                pArea,
                pCentroid,
                i0,
                j0,
                pOSplit,
                pISplit,
                iBinSize
            );
        }
        else if (i0 == j0) {
            pNode->m_pLeft = ::new NiOBBLeaf(pRoot, pTriList, pVertex, pWVertex, pOSplit[i0]);
        }

        if (j1 >= i1) {
            if (j1 == i1) {
                pNode->m_pRight = ::new NiOBBLeaf(pRoot, pTriList, pVertex, pWVertex, pOSplit[i1]);
            }
            return;
        }

        NiOBBNode* pRight = ::new NiOBBNode;
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
    if (NiPoint3::ZERO.x == velocity0.x &&
        NiPoint3::ZERO.y == velocity0.y &&
        NiPoint3::ZERO.z == velocity0.z &&
        velocity1.x == NiPoint3::ZERO.x &&
        velocity1.y == NiPoint3::ZERO.y &&
        velocity1.z == NiPoint3::ZERO.z) {
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
        NiCollisionData* pkData = static_cast<NiCollisionData*>(intr.pkRoot0->GetCollisionObject());
        if (!pkData) {
            pkData = static_cast<NiCollisionData*>(intr.pkObj0->GetCollisionObject());
        }
        if (pkData) {
            NiCollisionGroup::Callback cb0 = pkData->m_pfnCollideCallback;
            if (cb0) {
                ret0 = cb0(intr);
            }
        }
    }

    if (intr.pkRoot1) {
        NiCollisionData* pkData = static_cast<NiCollisionData*>(intr.pkRoot1->GetCollisionObject());
        if (!pkData) {
            pkData = static_cast<NiCollisionData*>(intr.pkObj1->GetCollisionObject());
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
    MemoryManager::Instance().Deallocate(apMemory);
}

// KEEP: callers (TestEdgeTri) reuse a caller-saved register's value after the call, relying on this body leaving it
// as it was (it never writes that register); clang does not guarantee that.
float* NiOBBLeaf::ComputeBarycenter(const float* v0, const float* v1, const float* v2, const float* p, float* pCenter) {
    float v10 = v0[0] - v2[0];
    float v12 = v0[1] - v2[1];
    float v13 = v0[2] - v2[2];
    float v14 = v1[0] - v2[0];
    float v15 = v1[1] - v2[1];
    float v16 = v1[2] - v2[2];
    float v17 = p[0] - v2[0];
    float v18 = p[1] - v2[1];
    float v19 = p[2] - v2[2];
    float fM00 = v13 * v13 + (v10 * v10 + v12 * v12);
    float fM01 = v14 * v10 + v15 * v12 + v16 * v13;
    float fM11 = v14 * v14 + v15 * v15 + v16 * v16;
    float fR0 = v13 * v19 + (v12 * v18 + v10 * v17);
    float v11 = v19 * v16 + (v15 * v18 + v14 * v17);
    double v6 = fM01;
    float fInvDet = 1.0 / (fM11 * fM00 - fM01 * fM01);
    double v7 = fInvDet;
    float fCenter0 = (fM11 * fR0 - v11 * v6) * fInvDet;
    pCenter[0] = fCenter0;
    double v8 = fM00 * v11 - v6 * fR0;
    double v9 = fCenter0;
    float fCenter1 = v7 * v8;
    pCenter[1] = fCenter1;
    pCenter[2] = 1.0 - v9 - fCenter1;
    return pCenter;
}

int32_t NiOBBLeaf::TentativeCollision(
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
) {
    double v10 = fDT;
    float v47 = fDT - 0.0;
    float fMinOffsetX = V[0] * 0.0;
    double v14 = fMinOffsetX;
    float fMinOffsetY = V[1] * 0.0;
    double v15 = fMinOffsetY;
    float fMinOffsetZ = V[2] * 0.0;
    double v16 = fMinOffsetZ;
    float v75 = q0[0] + fMinOffsetX;
    float v84 = q0[1] + fMinOffsetY;
    float v93 = q0[2] + fMinOffsetZ;
    float fDot00 = v84 * N[1] + v75 * N[0] + v93 * N[2];
    float fC00 = fDot00 - fC;
    float v76 = v14;
    float v85 = v15;
    float v94 = v16;
    float v49 = q1[0] + v76;
    float v59 = q1[1] + v85;
    float v67 = v94 + q1[2];
    float fDot10 = v49 * N[0] + v59 * N[1] + v67 * N[2];
    float fC10 = fDot10 - fC;
    float fMaxOffsetX = V[0] * v10;
    double v17 = fMaxOffsetX;
    int32_t v18 = 0;
    float fMaxOffsetY = V[1] * v10;
    double v19 = fMaxOffsetY;
    float fMaxOffsetZ = V[2] * v10;
    float v50 = q0[0] + fMaxOffsetX;
    float v60 = q0[1] + fMaxOffsetY;
    float v68 = fMaxOffsetZ + q0[2];
    float fDot01 = v50 * N[0] + v60 * N[1] + v68 * N[2];
    float fC01 = fDot01 - fC;
    float v78 = v17;
    float v87 = v19;
    float v51 = q1[0] + v78;
    float v61 = q1[1] + v87;
    float v69 = fMaxOffsetZ + q1[2];
    float fDot11 = v51 * N[0] + v61 * N[1] + v69 * N[2];
    float fC11 = fDot11 - fC;
    double v20 = fC10;
    double v21 = fC00;
    if (fC10 * fC00 <= 0.0) {
        if (0.0 == v21 && 0.0 == v20) {
            *fT0 = 0.0;
            *fT1 = 0.0;
            float fTime0 = *fT0;
            float v79 = V[0] * fTime0;
            float v88 = V[1] * fTime0;
            float v95 = fTime0 * V[2];
            float v52 = q0[0] + v79;
            float v62 = q0[1] + v88;
            double v22 = v95 + q0[2];
            float v70 = v22;
            p0[0] = v52;
            p0[1] = v62;
            p0[2] = v70;
            double v23 = *fT1;
            float v80 = V[0] * v23;
            float v89 = V[1] * v23;
            float v96 = v23 * V[2];
            float v53 = q1[0] + v80;
            float v63 = q1[1] + v89;
            double v24 = v96 + q1[2];
            float v71 = v24;
            p1[0] = v53;
            p1[1] = v63;
            p1[2] = v71;
            return 1;
        }
        *fT0 = 0.0;
        v18 = 1;
        float fS = v21 / (v21 - v20);
        float v118 = V[0] * 0.0;
        float v128 = V[1] * 0.0;
        float v138 = V[2] * 0.0;
        float v54 = q1[0] * fS;
        float v64 = q1[1] * fS;
        float v72 = q1[2] * fS;
        float fOneMinusS = 1.0 - fS;
        float v81 = q0[0] * fOneMinusS;
        float v90 = q0[1] * fOneMinusS;
        float v97 = fOneMinusS * q0[2];
        float v100 = v81 + v54;
        float v106 = v90 + v64;
        float v112 = v97 + v72;
        float v82 = v100 + v118;
        float v91 = v106 + v128;
        float v98 = v112 + v138;
        p0[0] = v82;
        p0[1] = v91;
        p0[2] = v98;
    }
    double v26 = fC01;
    if (fC01 * v21 <= 0.0) {
        if (0.0 == v21 && 0.0 == v26) {
            *fT0 = 0.0;
            *fT1 = v10;
            float fTime0 = *fT0;
            float v119 = V[0] * fTime0;
            float v129 = V[1] * fTime0;
            float v139 = fTime0 * V[2];
            float v101 = q0[0] + v119;
            float v107 = q0[1] + v129;
            double v27 = v139 + q0[2];
            float v113 = v27;
            p0[0] = v101;
            p0[1] = v107;
            p0[2] = v113;
            double v28 = *fT1;
            float v120 = V[0] * v28;
            float v130 = v28 * V[1];
            float v140 = v28 * V[2];
            float v102 = q0[0] + v120;
            float v108 = q0[1] + v130;
            double v29 = v140 + q0[2];
            float v114 = v29;
            p1[0] = v102;
            p1[1] = v108;
            p1[2] = v114;
            return 1;
        }
        double v30 = v21 * v47 / (v21 - v26);
        float fTimeQ0 = v30 + 0.0;
        float v121 = V[0] * fTimeQ0;
        float v131 = V[1] * fTimeQ0;
        float v141 = V[2] * fTimeQ0;
        float v55 = q0[0] + v121;
        float v65 = q0[1] + v131;
        float v73 = v141 + q0[2];
        if (v18) {
            *fT1 = fTimeQ0;
            p1[0] = v55;
            p1[1] = v65;
            p1[2] = v73;
            if (*fT1 < static_cast<double>(*fT0)) {
                float fSwap = *fT0;
                *fT0 = *fT1;
                *fT1 = fSwap;
                int32_t v32 = reinterpret_cast<uint32_t*>(p0)[0];
                int32_t v33 = reinterpret_cast<uint32_t*>(p0)[1];
                int32_t v34 = reinterpret_cast<uint32_t*>(p0)[2];
                p0[0] = p1[0];
                p0[1] = p1[1];
                p0[2] = p1[2];
                reinterpret_cast<uint32_t*>(p1)[0] = v32;
                reinterpret_cast<uint32_t*>(p1)[1] = v33;
                reinterpret_cast<uint32_t*>(p1)[2] = v34;
            }
            return 1;
        }
        *fT0 = fTimeQ0;
        p0[0] = v55;
        p0[1] = v65;
        p0[2] = v73;
        v18 = 1;
    }
    double v35 = fC11;
    if (fC11 * v20 <= 0.0) {
        if (0.0 == v20 && 0.0 == v35) {
            *fT0 = 0.0;
            *fT1 = v10;
            float fTime0 = *fT0;
            float v122 = V[0] * fTime0;
            float v132 = fTime0 * V[1];
            float v142 = fTime0 * V[2];
            float v103 = q1[0] + v122;
            float v109 = q1[1] + v132;
            double v36 = v142 + q1[2];
            float v115 = v36;
            p0[0] = v103;
            p0[1] = v109;
            p0[2] = v115;
            double v37 = *fT1;
            float v123 = V[0] * v37;
            float v133 = v37 * V[1];
            float v143 = v37 * V[2];
            float v102 = q1[0] + v123;
            float v108 = q1[1] + v133;
            double v29 = v143 + q1[2];
            float v114 = v29;
            p1[0] = v102;
            p1[1] = v108;
            p1[2] = v114;
            return 1;
        }
        double v38 = v20 * v47 / (v20 - v35);
        float fTimeQ1 = v38 + 0.0;
        float v124 = V[0] * fTimeQ1;
        float v134 = V[1] * fTimeQ1;
        float v144 = V[2] * fTimeQ1;
        float v56 = q1[0] + v124;
        float v66 = q1[1] + v134;
        float v74 = v144 + q1[2];
        if (v18) {
            *fT1 = fTimeQ1;
            p1[0] = v56;
            p1[1] = v66;
            p1[2] = v74;
            if (*fT1 < static_cast<double>(*fT0)) {
                float fSwap = *fT0;
                *fT0 = *fT1;
                *fT1 = fSwap;
                int32_t v41 = reinterpret_cast<uint32_t*>(p0)[0];
                int32_t v42 = reinterpret_cast<uint32_t*>(p0)[1];
                int32_t v43 = reinterpret_cast<uint32_t*>(p0)[2];
                p0[0] = p1[0];
                p0[1] = p1[1];
                p0[2] = p1[2];
                reinterpret_cast<uint32_t*>(p1)[0] = v41;
                reinterpret_cast<uint32_t*>(p1)[1] = v42;
                reinterpret_cast<uint32_t*>(p1)[2] = v43;
            }
            return 1;
        }
        *fT0 = fTimeQ1;
        p0[0] = v56;
        p0[1] = v66;
        p0[2] = v74;
        v18 = 1;
    }
    if (v35 * v26 <= 0.0) {
        if (0.0 == v26 && 0.0 == v35) {
            *fT0 = v10;
            *fT1 = v10;
            float fTime0 = *fT0;
            float v125 = V[0] * fTime0;
            float v135 = fTime0 * V[1];
            float v145 = fTime0 * V[2];
            float v104 = q0[0] + v125;
            float v110 = q0[1] + v135;
            double v45 = v145 + q0[2];
            float v116 = v45;
            p0[0] = v104;
            p0[1] = v110;
            p0[2] = v116;
            double v46 = *fT1;
            float v126 = V[0] * v46;
            float v136 = v46 * V[1];
            float v146 = v46 * V[2];
            float v102 = q1[0] + v126;
            float v108 = q1[1] + v136;
            double v29 = v146 + q1[2];
            float v114 = v29;
            p1[0] = v102;
            p1[1] = v108;
            p1[2] = v114;
            return 1;
        }
        float fS = v26 / (v26 - v35);
        float v148 = V[0] * v10;
        float v149 = V[1] * v10;
        float v150 = V[2] * v10;
        float v105 = q1[0] * fS;
        float v111 = q1[1] * fS;
        float v117 = q1[2] * fS;
        float fOneMinusS = 1.0 - fS;
        float v127 = q0[0] * fOneMinusS;
        float v137 = q0[1] * fOneMinusS;
        float v147 = fOneMinusS * q0[2];
        float v83 = v127 + v105;
        float v92 = v137 + v111;
        float v99 = v147 + v117;
        float v57 = v83 + v148;
        float v66 = v92 + v149;
        float v74 = v99 + v150;
        if (v18) {
            *fT1 = v10;
            p1[0] = v57;
            p1[1] = v66;
            p1[2] = v74;
            if (*fT1 < static_cast<double>(*fT0)) {
                float fSwap = *fT0;
                *fT0 = *fT1;
                *fT1 = fSwap;
                int32_t v41 = reinterpret_cast<uint32_t*>(p0)[0];
                int32_t v42 = reinterpret_cast<uint32_t*>(p0)[1];
                int32_t v43 = reinterpret_cast<uint32_t*>(p0)[2];
                p0[0] = p1[0];
                p0[1] = p1[1];
                p0[2] = p1[2];
                reinterpret_cast<uint32_t*>(p1)[0] = v41;
                reinterpret_cast<uint32_t*>(p1)[1] = v42;
                reinterpret_cast<uint32_t*>(p1)[2] = v43;
            }
            return 1;
        }
        *fT0 = v10;
        p0[0] = v57;
        p0[1] = v66;
        p0[2] = v74;
        v18 = 1;
    }
    if (v18) {
        *fT1 = *fT0;
        p1[0] = p0[0];
        p1[1] = p0[1];
        p1[2] = p0[2];
        return 1;
    }
    return 0;
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
    const NiPoint3* pV2 = pVer1[2];
    float v21 = v8->z - v7->z;
    float v16 = pV2->x - v7->x;
    float v17 = pV2->y - v7->y;
    float v18 = pV2->z - v7->z;
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
int32_t NiOBBLeaf::FindIntersection(NiOBBLeaf* pLeaf0, NiOBBLeaf* pLeaf1, float& fT, NiPoint3& p) {
    float fDT = pLeaf0->m_box.m_pRoot->m_fDeltaTime;
    const NiPoint3& V0 = pLeaf0->m_box.m_pRoot->m_velocity;
    const NiPoint3& V1 = pLeaf1->m_box.m_pRoot->m_velocity;
    NiPoint3 V;
    float fT0 = std::numeric_limits<float>::max();
    // The triangle FindIntersection returns 1 without writing its point when TestEdgeTri's time is not below fTMin (a
    // NaN or infinite time against the initial FLT_MAX), and the point is then read. p0 starts at the origin (a
    // departure), where the exe reads it unset; p1 starts as the second relative velocity, the value the exe reads for
    // it.
    NiPoint3 p0(0.0f, 0.0f, 0.0f);
    V.x = V0.x - V1.x;
    V.y = V0.y - V1.y;
    V.z = V0.z - V1.z;
    int32_t iRes0 = FindIntersection(pLeaf0->m_pWorldVertex, pLeaf1->m_pWorldVertex, V, fDT, fT0, p0);
    float fT1 = std::numeric_limits<float>::max();
    NiPoint3 p1;
    V.x = V1.x - V0.x;
    V.y = V1.y - V0.y;
    V.z = V1.z - V0.z;
    p1 = V;
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
