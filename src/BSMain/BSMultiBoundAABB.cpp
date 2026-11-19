#include "BSMain/BSMultiBoundAABB.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI BSMultiBoundAABB::ms_RTTI("BSMultiBoundAABB", &BSMultiBoundShape::ms_RTTI);

BSMultiBoundAABB::BSMultiBoundAABB() : BSMultiBoundShape() {}

BSMultiBoundAABB::BSMultiBoundAABB(NiPoint3* Center, NiPoint3* HalfExtents)
  : BSMultiBoundShape(), Center(*Center), HalfExtents(*HalfExtents) {}

BSMultiBoundAABB::~BSMultiBoundAABB() {}

NiObject* BSMultiBoundAABB::CreateClone(NiCloningProcess& arCloneData) {
    BSMultiBoundAABB* v3;
    v3 = new BSMultiBoundAABB;
    CopyMembers(v3, arCloneData);
    return v3;
}

void BSMultiBoundAABB::LoadBinary(NiStream& arStream) {
    BSMultiBoundShape::LoadBinary(arStream);
    Center.LoadBinary(arStream);
    HalfExtents.LoadBinary(arStream);
}

void BSMultiBoundAABB::LinkObject(NiStream& arStream) {
    BSMultiBoundShape::LinkObject(arStream);
}

bool BSMultiBoundAABB::RegisterStreamables(NiStream& arStream) {
    return BSMultiBoundShape::RegisterStreamables(arStream);
}

void BSMultiBoundAABB::SaveBinary(NiStream& arStream) {
    BSMultiBoundShape::SaveBinary(arStream);
    Center.SaveBinary(arStream);
    HalfExtents.SaveBinary(arStream);
}

bool BSMultiBoundAABB::IsEqual(NiObject* apObject) {
    if (!BSMultiBoundShape::IsEqual(apObject)) {
        return false;
    }

    BSMultiBoundAABB* pkOther = static_cast<BSMultiBoundAABB*>(apObject);
    // Both comparisons run; the result is their bitwise and.
    bool bEqual = Center == pkOther->Center;
    bool bHalfExtents = HalfExtents == pkOther->HalfExtents;
    return bEqual & bHalfExtents;
}

void BSMultiBoundAABB::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSMultiBoundShape::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSMultiBoundAABB::ms_RTTI.GetName()));
    apStrings->Add(Center.GetViewerString("Center"));
    apStrings->Add(HalfExtents.GetViewerString("Half Extents"));
}

float BSMultiBoundAABB::QMultiBoundRadius() const {
    return HalfExtents.Length();
}

// Per axis, whether the sphere lies inside the box and whether it touches it; later axes are only tested while one
// of the two still holds.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundAABB::CheckBound(const NiBound& aTargetBound) {
    float fY = aTargetBound.m_kCenter.y;
    float fZ = aTargetBound.m_kCenter.z;
    float fRadius = aTargetBound.m_fRadius;

    // Uses the double fabs, not the float overload: each component is widened, as the game does.
    float fDelta = aTargetBound.m_kCenter.x - Center.x;
    float fDist = std::fabs(static_cast<double>(fDelta));
    bool bContains = fDist - (HalfExtents.x - fRadius) <= 0.0;
    bool bIntersects = fDist - (HalfExtents.x + fRadius) <= 0.0;
    if (bContains || bIntersects) {
        fDelta = fY - Center.y;
        fDist = std::fabs(static_cast<double>(fDelta));
        if (bContains) {
            bContains &= fDist - (HalfExtents.y - fRadius) <= 0.0;
        }
        bIntersects &= fDist - (HalfExtents.y + fRadius) <= 0.0;
        if (bContains || bIntersects) {
            fDelta = fZ - Center.z;
            fDist = std::fabs(static_cast<double>(fDelta));
            if (bContains) {
                bContains &= fDist - (HalfExtents.z - fRadius) <= 0.0;
            }
            bIntersects &= fDist - (HalfExtents.z + fRadius) <= 0.0;
        }
    }

    if (bContains) {
        return BS_INTERSECT_CONTAINSTARGET;
    }
    return static_cast<BSMBIntersectResult>(bIntersects);
}

// Containment only: a box or sphere that merely overlaps counts as no intersection, and a capsule is never tested. The
// box test's last compare treats NaN as contained, so each test is `!(x > 0)`. Uses the double _fabs, not the float
// overload.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundAABB::CheckBound(const BSMultiBound& aTargetBound) {
    switch (aTargetBound.spShape->QType()) {
        case BS_SHAPE_AABB: {
            BSMultiBoundAABB* pkBox = static_cast<BSMultiBoundAABB*>(aTargetBound.spShape.m_pObject);
            NiPoint3 kDelta = Center - pkBox->Center;
            float fDistX = std::fabs(static_cast<double>(kDelta.x));
            float fX = fDistX + pkBox->HalfExtents.x - HalfExtents.x;
            float fDistY = std::fabs(static_cast<double>(kDelta.y));
            float fY = fDistY + pkBox->HalfExtents.y - HalfExtents.y;
            float fDistZ = std::fabs(static_cast<double>(kDelta.z));
            float fZ = fDistZ + pkBox->HalfExtents.z - HalfExtents.z;
            if (!(fX > 0.0) && !(fY > 0.0) && !(fZ > 0.0)) {
                return BS_INTERSECT_CONTAINSTARGET;
            }
            return BS_INTERSECT_NONE;
        }
        case BS_SHAPE_OBB: {
            BSMultiBoundOBB* pkBox = static_cast<BSMultiBoundOBB*>(aTargetBound.spShape.m_pObject);
            if (!FastPointWithin(pkBox->Center)) {
                return BS_INTERSECT_NONE;
            }

            NiPoint3 akVertices[8];
            pkBox->GetVertices(akVertices);
            for (int32_t i = 0; i < 8; ++i) {
                if (!FastPointWithin(akVertices[i])) {
                    return BS_INTERSECT_NONE;
                }
            }
            return BS_INTERSECT_CONTAINSTARGET;
        }
        case BS_SHAPE_SPHERE: {
            BSMultiBoundShape* pkSphere = aTargetBound.spShape.m_pObject;
            NiBound kBound;
            pkSphere->GetNiBound(kBound);
            return CheckBound(kBound);
        }
        default: return BS_INTERSECT_NONE;
    }
}

// Culled by any active plane with every corner behind it; a flat box (zero Z extent) only tests its odd corners.
bool BSMultiBoundAABB::WithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiPoint3 akVertices[8];
    bool bFlat = HalfExtents.z == 0.0;
    GetVertices(akVertices);
    for (int32_t i = 0; i < 6; ++i) {
        if ((aPlanes.m_uiActivePlanes & (1 << i)) != 0) {
            bool bAllBehind = true;
            for (int32_t j = 0; j < 8; ++j) {
                if ((!bFlat || j % 2) &&
                    aPlanes.m_akCullingPlanes[i].WhichSide(akVertices[j]) != NiPlane::NEGATIVE_SIDE) {
                    bAllBehind = false;
                }
            }
            if (bAllBehind) {
                return false;
            }
        }
    }
    return true;
}

bool BSMultiBoundAABB::CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiPoint3 akVertices[8];
    GetVertices(akVertices);
    for (int32_t i = 0; i < 6; ++i) {
        if ((aPlanes.m_uiActivePlanes & (1 << i)) != 0) {
            for (const NiPoint3& rVertex : akVertices) {
                if (aPlanes.m_akCullingPlanes[i].WhichSide(rVertex) == NiPlane::NEGATIVE_SIDE) {
                    return false;
                }
            }
        }
    }
    return true;
}

void BSMultiBoundAABB::GetNiBound(NiBound& aBound) {
    float fRadius = HalfExtents.Length();
    aBound.m_kCenter = Center;
    aBound.m_fRadius = fRadius;
}

// Writes the twelve edges (white) into the lines' data and the twelve faces (aColor) into the shape's data, both
// relative to the box center, which becomes their local translation.
void BSMultiBoundAABB::MakeDebugGeom(NiLines* apLines, NiTriShape* apShape, NiColorA akBaseColor) {
    // Vertex pairs of the twelve edges and vertex triples of the twelve faces, indexing GetVertices' corners.
    static constexpr uint16_t kDebugLines[12][2] = {
        {0, 1}, {0, 2}, {1, 3}, {3, 2}, {4, 5}, {4, 6}, {5, 7}, {7, 6}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    static constexpr uint16_t kDebugTriangles[12][3] = {
        {2, 3, 1}, {2, 1, 0}, {5, 7, 6}, {5, 6, 4}, {7, 2, 6}, {7, 3, 2},
        {4, 0, 5}, {5, 0, 1}, {6, 0, 4}, {6, 2, 0}, {3, 7, 1}, {7, 5, 1},
    };

    NiPoint3 akVertices[8];
    GetVertices(akVertices);
    uint16_t aakLines[12][2];
    for (int32_t i = 0; i < 12; ++i) {
        aakLines[i][0] = kDebugLines[i][0];
        aakLines[i][1] = kDebugLines[i][1];
    }
    uint16_t aakTriangles[12][3];
    for (int32_t i = 0; i < 12; ++i) {
        aakTriangles[i][0] = kDebugTriangles[i][0];
        aakTriangles[i][1] = kDebugTriangles[i][1];
        aakTriangles[i][2] = kDebugTriangles[i][2];
    }
    NiColorA kShapeColor = akBaseColor;
    NiColorA kLineColor = NiColorA_WHITE;

    NiLinesData* pkLinesData = static_cast<NiLinesData*>(apLines->m_spModelData.m_pObject);
    NiPoint3* pkVertex = pkLinesData->m_pkVertex;
    NiColorA* pkColor = pkLinesData->m_pkColor;
    uint8_t* pkFlags = pkLinesData->m_pkFlags;
    apLines->m_kLocal.m_Translate = Center;
    for (int32_t i = 0; i < 12; ++i) {
        pkVertex[2 * i] = akVertices[aakLines[i][0]] - Center;
        pkColor[2 * i] = kLineColor;
        pkVertex[2 * i + 1] = akVertices[aakLines[i][1]] - Center;
        pkColor[2 * i + 1] = kLineColor;
        pkFlags[2 * i] = true;
        pkFlags[2 * i + 1] = false;
    }

    NiTriShapeData* pkShapeData = static_cast<NiTriShapeData*>(apShape->m_spModelData.m_pObject);
    pkVertex = pkShapeData->m_pkVertex;
    pkColor = pkShapeData->m_pkColor;
    uint16_t* pusTriList = pkShapeData->GetTriList();
    apShape->m_kLocal.m_Translate = Center;
    for (uint16_t i = 0; i < 12; ++i) {
        pkVertex[3 * i] = akVertices[aakTriangles[i][0]] - Center;
        pkColor[3 * i] = kShapeColor;
        pkVertex[3 * i + 1] = akVertices[aakTriangles[i][1]] - Center;
        pkColor[3 * i + 1] = kShapeColor;
        pkVertex[3 * i + 2] = akVertices[aakTriangles[i][2]] - Center;
        pkColor[3 * i + 2] = kShapeColor;
        pusTriList[3 * i] = 3 * i;
        pusTriList[3 * i + 1] = 3 * i + 1;
        pusTriList[3 * i + 2] = 3 * i + 2;
    }
}

bool BSMultiBoundAABB::QPointWithin(NiPoint3& kpoint) {
    NiPoint3 kDelta = kpoint - Center;
    if (std::fabs(static_cast<double>(kDelta.x)) - HalfExtents.x > 0.01f) {
        return false;
    }
    if (std::fabs(static_cast<double>(kDelta.y)) - HalfExtents.y > 0.01f) {
        return false;
    }
    if (std::fabs(static_cast<double>(kDelta.z)) - HalfExtents.z > 0.01f) {
        return false;
    }
    return true;
}

void BSMultiBoundAABB::GetVertices(NiPoint3* apPoints) {
    NiPoint3 kX = NiPoint3_UNIT_X * HalfExtents.x;
    NiPoint3 kY = NiPoint3_UNIT_Y * HalfExtents.y;
    NiPoint3 kZ = NiPoint3_UNIT_Z * HalfExtents.z;
    apPoints[0] = Center + kX + kY + kZ;
    apPoints[1] = Center + kX + kY - kZ;
    apPoints[2] = Center + kX - kY + kZ;
    apPoints[3] = Center + kX - kY - kZ;
    apPoints[4] = Center - kX + kY + kZ;
    apPoints[5] = Center - kX + kY - kZ;
    apPoints[6] = Center - kX - kY + kZ;
    apPoints[7] = Center - kX - kY - kZ;
}

// QPointWithin with the point passed by value.
bool BSMultiBoundAABB::FastPointWithin(NiPoint3 kpoint) {
    NiPoint3 kDelta = kpoint - Center;
    if (std::fabs(static_cast<double>(kDelta.x)) - HalfExtents.x > 0.01f) {
        return false;
    }
    if (std::fabs(static_cast<double>(kDelta.y)) - HalfExtents.y > 0.01f) {
        return false;
    }
    if (std::fabs(static_cast<double>(kDelta.z)) - HalfExtents.z > 0.01f) {
        return false;
    }
    return true;
}

void BSMultiBoundAABB::SetHalfExtents(const NiPoint3& arHalfExtents) {
    HalfExtents = arHalfExtents;
}

void BSMultiBoundAABB::CopyMembers(BSMultiBoundAABB* pDest, NiCloningProcess& arCloneData) {
    BSMultiBoundShape::CopyMembers(pDest, arCloneData);
    pDest->SetCenter(Center);
    pDest->HalfExtents = HalfExtents;
}

NiObject* BSMultiBoundAABB::CreateObject() {
    return new BSMultiBoundAABB;
}
