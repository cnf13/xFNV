#include "BSMain/BSMultiBoundOBB.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI BSMultiBoundOBB::ms_RTTI("BSMultiBoundOBB", &BSMultiBoundAABB::ms_RTTI);

BSMultiBoundOBB::BSMultiBoundOBB() : BSMultiBoundAABB() {}

BSMultiBoundOBB::BSMultiBoundOBB(NiPoint3* a2, NiPoint3* a3, NiMatrix3* axis) : BSMultiBoundAABB(a2, a3), Axis(*axis) {}

BSMultiBoundOBB::~BSMultiBoundOBB() {}

NiObject* BSMultiBoundOBB::CreateClone(NiCloningProcess& arCloneData) {
    BSMultiBoundOBB* v3;
    void* v5 = NiMemObject::operator new(sizeof(BSMultiBoundOBB));
    if (v5) {
        v3 = ::new (v5) BSMultiBoundOBB;
    }
    else {
        v3 = nullptr;
    }
    CopyMembers(v3, arCloneData);
    return v3;
}

void BSMultiBoundOBB::LoadBinary(NiStream& arStream) {
    BSMultiBoundAABB::LoadBinary(arStream);
    Axis.LoadBinary(arStream);
}

void BSMultiBoundOBB::LinkObject(NiStream& arStream) {
    BSMultiBoundAABB::LinkObject(arStream);
}

bool BSMultiBoundOBB::RegisterStreamables(NiStream& arStream) {
    return BSMultiBoundAABB::RegisterStreamables(arStream);
}

void BSMultiBoundOBB::SaveBinary(NiStream& arStream) {
    BSMultiBoundAABB::SaveBinary(arStream);
    Axis.SaveBinary(arStream);
}

bool BSMultiBoundOBB::IsEqual(NiObject* apObject) {
    if (!BSMultiBoundAABB::IsEqual(apObject)) {
        return false;
    }

    return Axis == static_cast<BSMultiBoundOBB*>(apObject)->Axis;
}

void BSMultiBoundOBB::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSMultiBoundAABB::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSMultiBoundOBB::ms_RTTI.GetName()));
    apStrings->Add(Axis.GetViewerString("Axis"));
}

// BSMultiBoundAABB::CheckBound measured along the Axis columns instead of the world axes.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundOBB::CheckBound(const NiBound& aTargetBound) {
    NiPoint3 kTarget = aTargetBound.m_kCenter;
    float fRadius = aTargetBound.m_fRadius;
    bool bContains = true;
    NiPoint3 kAxis0 = NiPoint3_ZERO;
    NiPoint3 kAxis1 = NiPoint3_ZERO;
    NiPoint3 kAxis2 = NiPoint3_ZERO;

    kAxis0.x = Axis.m_pEntry[0][0];
    kAxis0.y = Axis.m_pEntry[1][0];
    kAxis0.z = Axis.m_pEntry[2][0];
    double dCenter = kAxis0.Dot(Center);
    float fDelta = kTarget.Dot(kAxis0) - dCenter;
    float fDist = std::fabs(static_cast<double>(fDelta));
    bContains = fDist - (HalfExtents.x - fRadius) <= 0.0;
    bool bIntersects = fDist - (HalfExtents.x + fRadius) <= 0.0;
    if (bContains || bIntersects) {
        kAxis1.x = Axis.m_pEntry[0][1];
        kAxis1.y = Axis.m_pEntry[1][1];
        kAxis1.z = Axis.m_pEntry[2][1];
        dCenter = kAxis1.Dot(Center);
        fDelta = kTarget.Dot(kAxis1) - dCenter;
        fDist = std::fabs(static_cast<double>(fDelta));
        if (bContains) {
            bContains &= fDist - (HalfExtents.y - fRadius) <= 0.0;
        }
        bIntersects &= fDist - (HalfExtents.y + fRadius) <= 0.0;
        if (bContains || bIntersects) {
            kAxis2.x = Axis.m_pEntry[0][2];
            kAxis2.y = Axis.m_pEntry[1][2];
            kAxis2.z = Axis.m_pEntry[2][2];
            dCenter = kAxis2.Dot(Center);
            fDelta = kTarget.Dot(kAxis2) - dCenter;
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

// Containment only: every corner of a target box must lie between this box's face planes; spheres go through the
// NiBound test and capsules are never tested.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundOBB::CheckBound(const BSMultiBound& aTargetBound) {
    BSMBShapeType eType = aTargetBound.spShape->QType();
    switch (eType) {
        case BS_SHAPE_AABB:
        case BS_SHAPE_OBB: {
            NiPoint3 akVertices[8];
            if (eType == BS_SHAPE_AABB) {
                static_cast<BSMultiBoundAABB*>(aTargetBound.spShape.m_pObject)->GetVertices(akVertices);
            }
            else {
                static_cast<BSMultiBoundOBB*>(aTargetBound.spShape.m_pObject)->GetVertices(akVertices);
            }
            NiPlane akPlanes[3];
            GetPlanes(akPlanes);
            for (int32_t i = 0; i < 8; ++i) {
                float fDelta = akPlanes[0].m_kNormal.Dot(akVertices[i]) - akPlanes[0].m_fConstant;
                float fDist = std::fabs(static_cast<double>(fDelta));
                if (fDist - HalfExtents.x > 0.0) {
                    return BS_INTERSECT_NONE;
                }
                fDelta = akPlanes[1].m_kNormal.Dot(akVertices[i]) - akPlanes[1].m_fConstant;
                fDist = std::fabs(static_cast<double>(fDelta));
                if (fDist - HalfExtents.y > 0.0) {
                    return BS_INTERSECT_NONE;
                }
                fDelta = akPlanes[2].m_kNormal.Dot(akVertices[i]) - akPlanes[2].m_fConstant;
                fDist = std::fabs(static_cast<double>(fDelta));
                if (fDist - HalfExtents.z > 0.0) {
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
bool BSMultiBoundOBB::WithinFrustum(const NiFrustumPlanes& aPlanes) {
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

bool BSMultiBoundOBB::CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiPoint3 akVertices[8];
    GetVertices(akVertices);
    for (int32_t i = 0; i < 6; ++i) {
        if ((aPlanes.m_uiActivePlanes & (1 << i)) != 0) {
            for (int32_t j = 0; j < 8; ++j) {
                if (aPlanes.m_akCullingPlanes[i].WhichSide(akVertices[j]) == NiPlane::NEGATIVE_SIDE) {
                    return false;
                }
            }
        }
    }
    return true;
}

void BSMultiBoundOBB::GetNiBound(NiBound& aBound) {
    float fRadius = HalfExtents.Length();
    aBound.m_kCenter = Center;
    aBound.m_fRadius = fRadius;
}

// Writes the twelve edges (white) into the lines' data and the twelve faces (aColor) into the shape's data, both
// relative to the box center, which becomes their local translation.
void BSMultiBoundOBB::MakeDebugGeom(NiLines* apLines, NiTriShape* apShape, NiColorA akBaseColor) {
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

// Each _fabs is the double one, compared against the double 0.009999999776482582, and an unordered last compare counts
// as within.
bool BSMultiBoundOBB::QPointWithin(NiPoint3& aPoint) {
    NiPlane akPlanes[3];
    GetPlanes(akPlanes);
    float fDelta = akPlanes[0].m_kNormal.Dot(aPoint) - akPlanes[0].m_fConstant;
    float fDist = std::fabs(static_cast<double>(fDelta));
    if (fDist - HalfExtents.x > 0.009999999776482582) {
        return false;
    }
    fDelta = akPlanes[1].m_kNormal.Dot(aPoint) - akPlanes[1].m_fConstant;
    fDist = std::fabs(static_cast<double>(fDelta));
    if (fDist - HalfExtents.y > 0.009999999776482582) {
        return false;
    }
    fDelta = akPlanes[2].m_kNormal.Dot(aPoint) - akPlanes[2].m_fConstant;
    fDist = std::fabs(static_cast<double>(fDelta));
    return !(fDist - HalfExtents.z > 0.009999999776482582);
}

void BSMultiBoundOBB::GetVertices(NiPoint3* apPoints) {
    NiPoint3 kCol0 = NiPoint3_ZERO;
    NiPoint3 kCol1 = NiPoint3_ZERO;
    NiPoint3 kCol2 = NiPoint3_ZERO;
    kCol0.x = Axis.m_pEntry[0][0];
    kCol0.y = Axis.m_pEntry[1][0];
    kCol0.z = Axis.m_pEntry[2][0];
    kCol0 *= HalfExtents.x;
    kCol1.x = Axis.m_pEntry[0][1];
    kCol1.y = Axis.m_pEntry[1][1];
    kCol1.z = Axis.m_pEntry[2][1];
    kCol1 *= HalfExtents.y;
    kCol2.x = Axis.m_pEntry[0][2];
    kCol2.y = Axis.m_pEntry[1][2];
    kCol2.z = Axis.m_pEntry[2][2];
    kCol2 *= HalfExtents.z;
    apPoints[0] = Center + kCol0 + kCol1 + kCol2;
    apPoints[1] = Center + kCol0 + kCol1 - kCol2;
    apPoints[2] = Center + kCol0 - kCol1 + kCol2;
    apPoints[3] = Center + kCol0 - kCol1 - kCol2;
    apPoints[4] = Center - kCol0 + kCol1 + kCol2;
    apPoints[5] = Center - kCol0 + kCol1 - kCol2;
    apPoints[6] = Center - kCol0 - kCol1 + kCol2;
    apPoints[7] = Center - kCol0 - kCol1 - kCol2;
}

void BSMultiBoundOBB::GetPlanes(NiPlane* apPlanes) {
    for (int32_t i = 0; i < 3; ++i) {
        NiPoint3 kNormal = NiPoint3_ZERO;
        kNormal.x = Axis.m_pEntry[0][i];
        kNormal.y = Axis.m_pEntry[1][i];
        kNormal.z = Axis.m_pEntry[2][i];
        apPlanes[i].m_kNormal = kNormal;
        float fConstant = kNormal.Dot(Center);
        apPlanes[i].m_fConstant = fConstant;
    }
}

void BSMultiBoundOBB::CopyMembers(BSMultiBoundOBB* pDest, NiCloningProcess& arCloneData) {
    BSMultiBoundAABB::CopyMembers(pDest, arCloneData);
    pDest->Axis = Axis;
}

NiObject* BSMultiBoundOBB::CreateObject() {
    BSMultiBoundOBB* apMemory = static_cast<BSMultiBoundOBB*>(NiMemObject::operator new(sizeof(BSMultiBoundOBB)));
    if (apMemory) {
        return ::new (apMemory) BSMultiBoundOBB;
    }
    else {
        return 0;
    }
}
