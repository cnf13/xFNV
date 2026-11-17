#include "BSMain/BSOcclusionPlane.h"

#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSPortal.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

NiRTTI BSOcclusionPlane::ms_RTTI("BSOcclusionPlane", &NiObject::ms_RTTI);

BSOcclusionPlane::BSOcclusionPlane()
  : NiObject(), kCenter(NiPoint3_ZERO), kRotation(NiMatrix3_IDENTITY), kCullResult(BSMultiBoundShape::BS_CULL_UNTESTED),
    bTwoSided(true), pLinkedPlanes{} {
    kHalfExtents.x = 50.0f;
    kHalfExtents.y = 50.0f;
}

BSOcclusionPlane::BSOcclusionPlane(NiPoint3* apCenter, NiPoint2* apHalfExtents, NiMatrix3* apRotation)
  : NiObject(), kCenter(*apCenter), kHalfExtents(*apHalfExtents), kRotation(*apRotation),
    kCullResult(BSMultiBoundShape::BS_CULL_UNTESTED), bDirtyVertices(true), bDirtyPlanes(true), bTwoSided(true) {}

const NiRTTI* BSOcclusionPlane::GetRTTI() const {
    return &BSOcclusionPlane::ms_RTTI;
}

NiObject* BSOcclusionPlane::CreateClone(NiCloningProcess& a1) {
    BSOcclusionPlane* pobj;
    BSOcclusionPlane* apMemory = static_cast<BSOcclusionPlane*>(NiMemObject::operator new(sizeof(BSOcclusionPlane)));
    if (apMemory) {
        pobj = ::new (apMemory) BSOcclusionPlane;
    }
    else {
        pobj = 0;
    }
    CopyMembers(pobj, a1);
    return pobj;
}

int32_t BSOcclusionPlane::QDebugGeomLineSize() {
    return 16;
}

int32_t BSOcclusionPlane::QDebugGeomShapeSize() {
    return 30;
}

void BSOcclusionPlane::GetVertices(NiPoint3* apPoints) {
    NiPoint3* v2;
    NiPoint3* v3;
    NiPoint3* v4;
    NiPoint3* v5;
    float y;
    NiPoint3 v8;
    NiPoint3 v9;
    NiPoint3 v10;
    NiPoint3 v11;
    NiPoint3 v12;
    NiPoint3 v13;
    NiPoint3 v14;
    NiPoint3 v15;
    NiPoint3 v16;
    NiPoint3 v17;
    NiPoint3 kPoint0;
    NiPoint3 kPoint1;
    if (bDirtyVertices) {
        kPoint0.x = kHalfExtents.x;
        kPoint0.y = (float)0.0;
        kPoint0.z = (float)0.0;
        y = kHalfExtents.y;
        kPoint1.x = (float)0.0;
        kPoint1.y = (float)0.0;
        kPoint1.z = y;
        kPoint0 = *&(v17 = (&kRotation)->operator*(kPoint0));
        kPoint1 = *&(v16 = (&kRotation)->operator*(kPoint1));
        v2 = &(v15 = (&kCenter)->operator-(kPoint0));
        apPoints[3] = *&(v14 = v2->operator+(kPoint1));
        v3 = &(v13 = (&kCenter)->operator-(kPoint0));
        apPoints[2] = *&(v12 = v3->operator-(kPoint1));
        v4 = &(v11 = (&kCenter)->operator+(kPoint0));
        apPoints[1] = *&(v10 = v4->operator-(kPoint1));
        v5 = &(v9 = (&kCenter)->operator+(kPoint0));
        *apPoints = *&(v8 = v5->operator+(kPoint1));
        rt::memcpy(kCachedVertices, apPoints, sizeof(kCachedVertices));
        bDirtyVertices = (bool)0;
    }
    else {
        rt::memcpy(apPoints, kCachedVertices, sizeof(NiPoint3) * 4);
    }
}

void BSOcclusionPlane::GetFrustumPlanes(NiPoint3 akViewPoint, NiFrustumPlanes& akPlanes) {
    char v6;
    alignas(NiPlane) uint8_t v17[16];
    alignas(NiPlane) uint8_t v18[16];
    alignas(NiPlane) uint8_t v19[16];
    alignas(NiPlane) uint8_t v20[16];
    alignas(NiPlane) uint8_t v21[16];
    alignas(NiPlane) uint8_t v22[16];
    alignas(NiPlane) uint8_t v23[16];
    alignas(NiPlane) uint8_t v24[16];
    alignas(NiPlane) uint8_t v25[16];
    alignas(NiPlane) uint8_t v26[16];
    if (bDirtyPlanes ||
        (kCachedViewPoint.x != akViewPoint.x ||
                 (kCachedViewPoint.y != akViewPoint.y || kCachedViewPoint.z != akViewPoint.z)
             ? v6 = 0
             : v6 = 1,
         !v6)) {
        NiPoint3 kPoints[4];
        int32_t v15 = 4;
        for (NiPoint3* i = kPoints; --v15 >= 0; ++i) {
            ;
        }
        GetVertices(kPoints);
        akPlanes.m_uiActivePlanes = 63;
        NiPlane apMemory(kPoints, &kPoints[1], &kPoints[2]);
        if (apMemory.WhichSide(akViewPoint) == 1) {
            NiPlane* v11 = ::new (&reinterpret_cast<NiPlane&>(v21)) NiPlane(&kPoints[1], kPoints, &kPoints[2]);
            akPlanes.m_akCullingPlanes[0] = *v11;
            akPlanes.m_uiActivePlanes &= ~2u;
            NiPlane* v10 = ::new (&reinterpret_cast<NiPlane&>(v20)) NiPlane(&kPoints[1], kPoints, &akViewPoint);
            akPlanes.m_akCullingPlanes[2] = *v10;
            NiPlane* v5 = ::new (&reinterpret_cast<NiPlane&>(v19)) NiPlane(&kPoints[3], &kPoints[2], &akViewPoint);
            akPlanes.m_akCullingPlanes[3] = *v5;
            NiPlane* v9 = ::new (&reinterpret_cast<NiPlane&>(v18)) NiPlane(kPoints, &kPoints[3], &akViewPoint);
            akPlanes.m_akCullingPlanes[4] = *v9;
            NiPlane* v8 = ::new (&reinterpret_cast<NiPlane&>(v17)) NiPlane(&kPoints[2], &kPoints[1], &akViewPoint);
            akPlanes.m_akCullingPlanes[5] = *v8;
        }
        else {
            NiPlane* v14 = ::new (&reinterpret_cast<NiPlane&>(v26)) NiPlane(kPoints, &kPoints[1], &kPoints[2]);
            akPlanes.m_akCullingPlanes[0] = *v14;
            akPlanes.m_uiActivePlanes &= ~2u;
            NiPlane* v3 = ::new (&reinterpret_cast<NiPlane&>(v25)) NiPlane(kPoints, &kPoints[1], &akViewPoint);
            akPlanes.m_akCullingPlanes[2] = *v3;
            NiPlane* v13 = ::new (&reinterpret_cast<NiPlane&>(v24)) NiPlane(&kPoints[2], &kPoints[3], &akViewPoint);
            akPlanes.m_akCullingPlanes[3] = *v13;
            NiPlane* v12 = ::new (&reinterpret_cast<NiPlane&>(v23)) NiPlane(&kPoints[3], kPoints, &akViewPoint);
            akPlanes.m_akCullingPlanes[4] = *v12;
            NiPlane* v4 = ::new (&reinterpret_cast<NiPlane&>(v22)) NiPlane(&kPoints[1], &kPoints[2], &akViewPoint);
            akPlanes.m_akCullingPlanes[5] = *v4;
        }
        for (int32_t j = 0; j < 4; ++j) {
            if (bDisabledPlanes[j]) {
                akPlanes.m_uiActivePlanes &= ~(1 << (j + 2));
            }
        }
        kCachedViewPoint = akViewPoint;
        kCachedPlanes = akPlanes;
        bDirtyPlanes = 0;
    }
    else {
        akPlanes = kCachedPlanes;
    }
}

void BSOcclusionPlane::SetLinkedPlane(BSOP_LINK akConnection, BSOcclusionPlane* apPlane) {
    pLinkedPlanes[(int32_t)akConnection] = apPlane;
}

// GetVertices fills one array of four corners, and the planes each corner is outside of are kept per corner in one
// array; both are indexed by the corner.
bool BSOcclusionPlane::WithinFrustum(const NiFrustumPlanes& aPlanes, bool abViewHitPlane, NiPoint3 akViewIntersection) {
    NiPoint3 akVertices[4];
    GetVertices(akVertices);
    int32_t aiOutside[4];
    aiOutside[3] = 0;
    aiOutside[2] = aiOutside[3];
    aiOutside[1] = aiOutside[2];
    aiOutside[0] = aiOutside[1];
    for (int32_t j = 0; j < 6; ++j) {
        if ((aPlanes.GetActivePlaneState() & 1 << j) != 0) {
            char v26 = 1;
            for (int32_t k = 0; k < 4; ++k) {
                if (aPlanes.m_akCullingPlanes[j].WhichSide(akVertices[k]) == 2) {
                    aiOutside[k] |= 1 << j;
                }
                else {
                    v26 = 0;
                }
            }
            if (v26) {
                return 0;
            }
        }
    }
    NiPoint3& kPoint = akVertices[0];
    NiPoint3& akPoint1 = akVertices[1];
    NiPoint3& v30 = akVertices[2];
    NiPoint3& akPoint0 = akVertices[3];
    bDisabledPlanes[0] = (aiOutside[1] & aiOutside[0]) != 0;
    bDisabledPlanes[1] = (aiOutside[2] & aiOutside[3]) != 0;
    bDisabledPlanes[2] = (aiOutside[3] & aiOutside[0]) != 0;
    bDisabledPlanes[3] = (aiOutside[1] & aiOutside[2]) != 0;
    char v32 = 0;
    if (bDisabledPlanes[0] && (bDisabledPlanes[1] && (bDisabledPlanes[2] && bDisabledPlanes[3]))) {
        v32 = 1;
    }
    else {
        if (!bDisabledPlanes[0] && (bDisabledPlanes[1] && (bDisabledPlanes[2] && bDisabledPlanes[3]))) {
            char v24 = 0;
            for (int32_t m = 2; m < 6; ++m) {
                if (TestLineSegmentAllPlanes(kPoint, akPoint1, aPlanes.m_akCullingPlanes[m], aPlanes)) {
                    v24 = 1;
                    break;
                }
            }
            if (!v24) {
                v32 = 1;
            }
        }
        else {
            if (bDisabledPlanes[0] && (!bDisabledPlanes[1] && (bDisabledPlanes[2] && bDisabledPlanes[3]))) {
                char v22 = 0;
                for (int32_t n = 2; n < 6; ++n) {
                    if (TestLineSegmentAllPlanes(akPoint0, v30, aPlanes.m_akCullingPlanes[n], aPlanes)) {
                        v22 = 1;
                        break;
                    }
                }
                if (!v22) {
                    v32 = 1;
                }
            }
            else {
                if (bDisabledPlanes[0] && (bDisabledPlanes[1] && (!bDisabledPlanes[2] && bDisabledPlanes[3]))) {
                    char v20 = 0;
                    for (int32_t ii = 2; ii < 6; ++ii) {
                        if (TestLineSegmentAllPlanes(kPoint, akPoint0, aPlanes.m_akCullingPlanes[ii], aPlanes)) {
                            v20 = 1;
                            break;
                        }
                    }
                    if (!v20) {
                        v32 = 1;
                    }
                }
                else {
                    if (bDisabledPlanes[0] && (bDisabledPlanes[1] && (bDisabledPlanes[2] && !bDisabledPlanes[3]))) {
                        char v18 = 0;
                        for (int32_t jj = 2; jj < 6; ++jj) {
                            if (TestLineSegmentAllPlanes(v30, akPoint1, aPlanes.m_akCullingPlanes[jj], aPlanes)) {
                                v18 = 1;
                                break;
                            }
                        }
                        if (!v18) {
                            v32 = 1;
                        }
                    }
                }
            }
        }
    }
    if (!v32) {
        return 1;
    }
    for (int32_t kk = 2; kk < 6; ++kk) {
        if (TestLineSegmentAllPlanes(kPoint, v30, aPlanes.m_akCullingPlanes[kk], aPlanes) ||
            TestLineSegmentAllPlanes(akPoint1, akPoint0, aPlanes.m_akCullingPlanes[kk], aPlanes)) {
            return 1;
        }
    }
    bool v4 = false;
    if (abViewHitPlane) {
        NiPoint3 result;
        result = akViewIntersection - kCenter;
        NiPoint3 pt = NiPoint3_ZERO;
        pt.x = kRotation.m_pEntry[0][0];
        pt.y = kRotation.m_pEntry[0][1];
        pt.z = kRotation.m_pEntry[0][2];
        float v9 = result.Dot(pt);
        float v6 = std::fabs(static_cast<double>(v9));
        pt.x = kRotation.m_pEntry[1][0];
        pt.y = kRotation.m_pEntry[1][1];
        pt.z = kRotation.m_pEntry[1][2];
        float v8 = result.Dot(pt);
        float v5 = std::fabs(static_cast<double>(v8));
        if (kHalfExtents.x >= static_cast<double>(v6) && kHalfExtents.y >= static_cast<double>(v5)) {
            return true;
        }
    }
    return v4;
}

// GetVertices fills one array of four corners; the plane is built from the first three.
bool BSOcclusionPlane::ViewerOnCorrectSide(NiPoint3 akViewPoint) {
    if (bTwoSided) {
        return 1;
    }
    NiPoint3 akVertices[4];
    NiFrustumPlanes a5;
    GetFrustumPlanes(akViewPoint, a5);
    GetVertices(akVertices);
    NiPlane apMemory(&akVertices[0], &akVertices[1], &akVertices[2]);
    return apMemory.WhichSide(akViewPoint) == 2;
}

// When the viewer is within 5 units of the frustum's near plane (measured through the point where the view meets the
// occlusion plane) and that point lies inside the plane's extents, only a portal counts as within; otherwise
// WithinFrustum decides. On NaN it falls to WithinFrustum.
bool BSOcclusionPlane::WithinFrustumDistFirst(const NiFrustumPlanes& aPlanes, NiPoint3& aViewPos) {
    NiPoint3 kPoints[4];
    GetVertices(kPoints);
    NiPlane kPlane(&kPoints[0], &kPoints[1], &kPoints[2]);
    float fDist = kPlane.m_kNormal.Dot(aViewPos) - kPlane.m_fConstant;
    NiPoint3 kViewIntersection = aViewPos + -fDist * kPlane.m_kNormal;
    const NiPlane& kNearPlane = aPlanes.m_akCullingPlanes[NiFrustumPlanes::NEAR_PLANE];
    float fNearDist = kNearPlane.m_kNormal.Dot(aViewPos) - kNearPlane.m_fConstant;
    fDist = fNearDist + fDist;
    if (fDist >= -5.0 && fDist < 5.0) {
        NiPoint3 kRot0 = NiPoint3_ZERO;
        NiPoint3 kRot1 = NiPoint3_ZERO;
        kRot0.x = kRotation.m_pEntry[0][0];
        kRot0.y = kRotation.m_pEntry[0][1];
        kRot0.z = kRotation.m_pEntry[0][2];
        kRot1.x = kRotation.m_pEntry[1][0];
        kRot1.y = kRotation.m_pEntry[1][1];
        kRot1.z = kRotation.m_pEntry[1][2];
        float fX = kViewIntersection.Dot(kRot0) - kCenter.x;
        float fY = kViewIntersection.Dot(kRot1) - kCenter.y;
        // Takes the double fabs of the widened float, not the float overload, as the game does.
        float fAbsX = static_cast<float>(std::fabs(static_cast<double>(fX)));
        if (kHalfExtents.x >= fAbsX) {
            float fAbsY = static_cast<float>(std::fabs(static_cast<double>(fY)));
            if (kHalfExtents.y >= fAbsY) {
                return this && GetRTTI() == &BSPortal::ms_RTTI;
            }
        }
    }
    return WithinFrustum(aPlanes, true, kViewIntersection);
}

int32_t BSOcclusionPlane::QEdgeVisibleState() {
    return !bDisabledPlanes[0] | 2 * (!bDisabledPlanes[1] | 2 * (!bDisabledPlanes[2] | 2 * !bDisabledPlanes[3]));
}

// Copies the point one field at a time, each load before its store, so an overlapping arCenter reads the fields already
// written, as the game does.
void BSOcclusionPlane::SetCenter(const NiPoint3& arCenter) {
    kCenter.x = arCenter.x;
    kCenter.y = arCenter.y;
    kCenter.z = arCenter.z;
    bDirtyVertices = true;
    bDirtyPlanes = true;
}

void BSOcclusionPlane::SetHalfExtents(const NiPoint2& arHalfExtents) {
    float y;
    y = (&arHalfExtents)->y;
    kHalfExtents.x = (&arHalfExtents)->x;
    kHalfExtents.y = y;
    bDirtyVertices = (bool)1;
    bDirtyPlanes = (bool)1;
}

void BSOcclusionPlane::SetRotation(const NiMatrix3& arRotation) {
    __builtin_memcpy(&kRotation, &arRotation, sizeof(kRotation));
    bDirtyVertices = (bool)1;
    bDirtyPlanes = (bool)1;
}

bool BSOcclusionPlane::TestIntersection(const NiBound& aBound) {
    NiMatrix3 inv;
    NiPoint3 result;
    NiPoint3 v21;
    float m_fRadius;
    NiMatrix3 v23;
    if (!(&kRotation)->Inverse(inv)) {
        (&inv)->MakeZero();
    }
    __builtin_memcpy(&v23, &inv, sizeof(v23));
    NiPoint3* v2 = &(result = (&(&aBound)->m_kCenter)->operator-(kCenter));
    (v21 = (&v23)->operator*(*v2));
    float v11 = (float)std::fabs((double)v21.x);
    v21.x = v11;
    float v10 = (float)std::fabs((double)v21.y);
    v21.y = v10;
    float v9 = (float)std::fabs((double)v21.z);
    v21.z = v9;
    m_fRadius = (&aBound)->m_fRadius;
    float v13 = v21.x - m_fRadius;
    float v14 = v21.y - m_fRadius;
    float v15 = v9 - m_fRadius;
    float v18 = v13;
    float v19 = v14;
    float v20 = v15;
    if ((!(kHalfExtents.x >= ((double)v13))) || ((!(kHalfExtents.y >= ((double)v20))) || (!(v19 <= 0.0)))) {
        return (bool)0;
    }
    float v8 = (float)std::fabs((double)v19);
    float v7 = v8 / m_fRadius;
    double v6 = (double)m_fRadius;
    float v5 = (float)std::cos((double)v7);
    float v24 = (float)(v5 * v6);
    return (bool)((uint8_t)((kHalfExtents.x >= (v21.x - v24)) && (kHalfExtents.y >= (v21.y - v24))));
}

BSOcclusionPlane::BSOP_LINKTYPE BSOcclusionPlane::GetJoinedPlaneType() {
    if (pLinkedPlanes[0] || pLinkedPlanes[2]) {
        return BSOcclusionPlane::BSOP_LINKTYPE_CHILD;
    }
    return static_cast<BSOcclusionPlane::BSOP_LINKTYPE>((uint8_t)(pLinkedPlanes[1] || pLinkedPlanes[3]));
}

bool BSOcclusionPlane::TestLineSegmentAllPlanes(
    NiPoint3& akPoint0,
    NiPoint3& akPoint1,
    const NiPlane& akPlane0,
    const NiFrustumPlanes& akPlanes
) {
    float v8;
    float m_fConstant;
    NiPoint3 v10;
    int32_t i;
    NiPoint3 v12;
    NiPoint3 v15;
    (v15 = (&akPoint1)->operator-(akPoint0));
    NiPoint3 kNormal = (&akPlane0)->m_kNormal;
    float v16 = (float)(&kNormal)->Dot(v15);
    if (v16 == 0.0) {
        return (bool)0;
    }
    m_fConstant = (&akPlane0)->m_fConstant;
    double v6 = (double)(&kNormal)->Dot(akPoint0);
    float v14 = (float)((m_fConstant - v6) / v16);
    if ((!(v14 >= 0.0)) || (!(v14 <= 1.0))) {
        return (bool)0;
    }
    NiPoint3* v7 = &(v10 = (&v15)->operator*(v14));
    (v12 = (&akPoint0)->operator+(*v7));
    for (i = 0; i < 6; ++i) {
        if (((&akPlanes)->m_uiActivePlanes & (1 << i)) != 0) {
            v8 = (&akPlanes)->m_akCullingPlanes[i].m_fConstant;
            if (((&(&akPlanes)->m_akCullingPlanes[i].m_kNormal)->Dot(v12) - v8) <= (-0.0001)) {
                return (bool)0;
            }
        }
    }
    return (bool)1;
}

void BSOcclusionPlane::CopyMembers(BSOcclusionPlane* pDest, NiCloningProcess& arCloneData) {
    pDest->kCenter = kCenter;
    pDest->bDirtyVertices = 1;
    pDest->bDirtyPlanes = 1;
    float y = kHalfExtents.y;
    pDest->kHalfExtents.x = kHalfExtents.x;
    pDest->kHalfExtents.y = y;
    pDest->bDirtyVertices = 1;
    pDest->bDirtyPlanes = 1;
    pDest->kRotation = kRotation;
    pDest->bDirtyVertices = 1;
    pDest->bDirtyPlanes = 1;
    pDest->kCullResult = kCullResult;
}

bool BSOcclusionPlane::WithinFrustumFullTest(const NiFrustumPlanes& aPlanes) {
    NiPoint3 kPoint[4];
    int32_t v4 = 4;
    for (NiPoint3* i = kPoint; --v4 >= 0; ++i) {
        ;
    }
    GetVertices(kPoint);
    // Per vertex, the planes it is on the outside of.
    int32_t aiOutside[4] = {0, 0, 0, 0};
    bool v10 = 1;
    for (int32_t j = 0; j < 6; ++j) {
        if (((&aPlanes)->GetActivePlaneState() & 1 << j) != 0) {
            char v7 = 1;
            for (int32_t k = 0; k < 4; ++k) {
                if (aPlanes.m_akCullingPlanes[j].WhichSide(kPoint[k]) == 2) {
                    aiOutside[k] |= 1 << j;
                }
                else {
                    v7 = 0;
                }
            }
            if (v7) {
                v10 = 0;
            }
        }
    }
    bDisabledPlanes[0] = (aiOutside[1] & aiOutside[0]) != 0;
    bDisabledPlanes[1] = (aiOutside[2] & aiOutside[3]) != 0;
    bDisabledPlanes[2] = (aiOutside[3] & aiOutside[0]) != 0;
    bDisabledPlanes[3] = (aiOutside[1] & aiOutside[2]) != 0;
    return v10;
}

bool BSOcclusionPlane::CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiPoint3 kVerts[4];
    int32_t v3 = 4;
    for (NiPoint3* i = kVerts; --v3 >= 0; ++i) {
        ;
    }
    GetVertices(kVerts);
    for (int32_t j = 0; j < 6; ++j) {
        if (((&aPlanes)->GetActivePlaneState() & 1 << j) != 0) {
            for (int32_t k = 0; k < 4; ++k) {
                if (aPlanes.m_akCullingPlanes[j].WhichSide(kVerts[k]) == 2) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// The eight corners are one array: GetVertices fills the plane's four, and the far four are pushed 2500 units away
// from the viewer. The line and triangle corner indices are two word arrays on the stack.
void BSOcclusionPlane::MakeDebugGeom(
    NiPoint3 akViewPoint,
    NiLines* apLines,
    NiTriShape* apShape,
    NiColorA akBaseColor,
    bool abWithinFrustum
) {
    bool v39;
    FNVD_INLINED_LOCAL(NiColorA, v50, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, v51, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, v52, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, v55, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, v56, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, v57, std::nullopt, std::nullopt);
    NiPoint3 akPoints[8];
    uint16_t j;
    FNVD_INLINED_LOCAL(NiColorA, kcolor, std::nullopt, std::nullopt);
    int32_t v103 = 4;
    int32_t v73 = 2;
    GetVertices(akPoints);
    NiPoint3 ktemp = akPoints[0] - akViewPoint;
    ktemp.Unitize();
    akPoints[4] = akPoints[0] + ktemp * 2500.0f;
    ktemp = akPoints[1] - akViewPoint;
    ktemp.Unitize();
    akPoints[5] = akPoints[1] + ktemp * 2500.0f;
    ktemp = akPoints[2] - akViewPoint;
    ktemp.Unitize();
    akPoints[6] = akPoints[2] + ktemp * 2500.0f;
    ktemp = akPoints[3] - akViewPoint;
    ktemp.Unitize();
    akPoints[7] = akPoints[3] + ktemp * 2500.0f;
    uint16_t ausLines[16] = {0, 1, 3, 2, 0, 3, 2, 1, 0, 4, 1, 5, 2, 6, 3, 7};
    uint16_t ausTris[30] = {0, 1, 2, 2, 3, 0, 5, 0, 4, 5, 1, 0, 5, 2, 1, 6, 2, 5, 6, 7, 2, 7, 3, 2, 0, 3, 7, 7, 4, 0};
    kcolor = akBaseColor;
    float a = NiColorA_WHITE.a;
    float v82 = akBaseColor.a;
    float v83 = akBaseColor.a;
    float v84 = akBaseColor.a;
    float v89 = 1.0f;
    float v90 = 0.7f;
    float v91 = 0.7f;
    float v92 = akBaseColor.a;
    NiPoint3* m_pkVertex = apLines->m_spModelData.m_pObject->m_pkVertex;
    NiColorA* m_pkColor = apLines->m_spModelData.m_pObject->m_pkColor;
    uint8_t* pkFlags = static_cast<NiLinesData*>(apLines->m_spModelData.m_pObject)->m_pkFlags;
    for (j = 0; j < v103; ++j) {
        NiColorA* v34;
        NiColorA* v36;
        NiPoint3* v10 = &akPoints[ausLines[2 * j]];
        m_pkVertex[2 * j] = *v10;
        if (kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE) {
            NiColorA* v37;
            if (bDisabledPlanes[j]) {
                v57.r = 1.0f;
                v57.g = 1.0f;
                v57.b = 0.1f;
                v57.a = akBaseColor.a;
                v37 = &v57;
            }
            else {
                v56.r = 0.1f;
                v56.g = 1.0f;
                v56.b = 0.1f;
                v56.a = akBaseColor.a;
                v37 = &v56;
            }
            v36 = v37;
        }
        else {
            v55.r = 0.7f;
            v55.g = 0.15f;
            v55.b = 0.15f;
            v55.a = akBaseColor.a;
            v36 = &v55;
        }
        m_pkColor[2 * j] = *v36;
        NiPoint3* v13 = &akPoints[ausLines[2 * j + 1]];
        m_pkVertex[2 * j + 1] = *v13;
        if (kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE) {
            NiColorA* v35;
            if (bDisabledPlanes[j]) {
                v52.r = 1.0f;
                v52.g = 1.0f;
                v52.b = 0.1f;
                v52.a = akBaseColor.a;
                v35 = &v52;
            }
            else {
                v51.r = 0.1f;
                v51.g = 1.0f;
                v51.b = 0.1f;
                v51.a = akBaseColor.a;
                v35 = &v51;
            }
            v34 = v35;
        }
        else {
            v50.r = 0.7f;
            v50.g = 0.15f;
            v50.b = 0.15f;
            v50.a = akBaseColor.a;
            v34 = &v50;
        }
        m_pkColor[2 * j + 1] = *v34;
        pkFlags[2 * j] = 1;
        pkFlags[2 * j + 1] = 0;
    }
    if (this) {
        v39 = GetRTTI() == &BSPortal::ms_RTTI;
    }
    else {
        v39 = 0;
    }
    if (v39) {
        NiPoint3 ktangent = NiPoint3_ZERO;
        ktangent.x = kRotation.m_pEntry[1][0];
        ktangent.y = kRotation.m_pEntry[1][1];
        ktangent.z = kRotation.m_pEntry[1][2];
        m_pkVertex[2 * j] = akPoints[0] + ktangent * 30.0f;
        NiColorA* v18 = &m_pkColor[2 * j];
        v18->r = v82;
        v18->g = v83;
        v18->b = v84;
        v18->a = a;
        m_pkVertex[2 * j + 1] = akPoints[0] - ktangent * 30.0f;
        NiColorA* v21 = &m_pkColor[2 * j + 1];
        v21->r = v82;
        v21->g = v83;
        v21->b = v84;
        v21->a = a;
        pkFlags[2 * j] = 1;
        pkFlags[2 * j++ + 1] = 0;
        m_pkVertex[2 * j] = akPoints[3] + ktangent * 30.0f;
        NiColorA* v24 = &m_pkColor[2 * j];
        v24->r = v82;
        v24->g = v83;
        v24->b = v84;
        v24->a = a;
        m_pkVertex[2 * j + 1] = akPoints[3] - ktangent * 30.0f;
        NiColorA* v27 = &m_pkColor[2 * j + 1];
        v27->r = v82;
        v27->g = v83;
        v27->b = v84;
        v27->a = a;
        pkFlags[2 * j] = 1;
        pkFlags[2 * j++ + 1] = 0;
        ++v103;
    }
    while (j < 8u) {
        pkFlags[2 * j] = 0;
        pkFlags[2 * j++ + 1] = 0;
    }
    m_pkVertex = apShape->m_spModelData.m_pObject->m_pkVertex;
    m_pkColor = apShape->m_spModelData.m_pObject->m_pkColor;
    if (abWithinFrustum) {
        kcolor.a = 0.2f;
    }
    else {
        kcolor.a = 0.1f;
    }
    uint16_t* TriList = static_cast<NiTriShapeData*>(apShape->m_spModelData.m_pObject)->GetTriList();
    for (j = 0; j < v73; ++j) {
        NiPoint3* v28 = &akPoints[ausTris[3 * j]];
        m_pkVertex[3 * j] = *v28;
        m_pkColor[3 * j] = kcolor;
        NiPoint3* v30 = &akPoints[ausTris[3 * j + 1]];
        m_pkVertex[3 * j + 1] = *v30;
        m_pkColor[3 * j + 1] = kcolor;
        NiPoint3* v32 = &akPoints[ausTris[3 * j + 2]];
        m_pkVertex[3 * j + 2] = *v32;
        m_pkColor[3 * j + 2] = kcolor;
        TriList[3 * j] = 3 * j;
        TriList[3 * j + 1] = 3 * j + 1;
        TriList[3 * j + 2] = 3 * j + 2;
    }
    uint16_t v86 = 3 * (j - 1) + 2;
    while (j < 10u) {
        TriList[3 * j] = v86;
        TriList[3 * j + 1] = v86;
        TriList[3 * j++ + 2] = v86;
    }
}

bool BSOcclusionPlane::TestLineSegment(NiPoint3* a1, NiPoint3* a2, float* a3, NiPoint3* a4, NiPoint3* a5) {
    NiPoint3* v7;
    float v8;
    float x;
    NiPoint3 v11;
    NiPoint3 v12;
    NiPoint3 v13;
    NiPoint3 v15;
    (v15 = a2->operator-(*a1));
    v13.x = (float)(*a3);
    v13.y = a3[1];
    v13.z = a3[2];
    float v16 = (float)(&v13)->Dot(v15);
    if (v16 == 0.0) {
        return (bool)0;
    }
    float v10 = a3[3];
    double v6 = (double)(&v13)->Dot(*a1);
    float v14 = (float)((v10 - v6) / v16);
    uint8_t result = 0;
    if ((v14 >= 0.0) && (v14 <= 1.0)) {
        v7 = &(v11 = (&v15)->operator*(v14));
        (v12 = a1->operator+(*v7));
        x = a4[1].x;
        if ((a4->Dot(v12) - x) >= (-0.0001)) {
            v8 = a5[1].x;
            if ((a5->Dot(v12) - v8) >= (-0.0001)) {
                return (bool)1;
            }
        }
    }
    return (bool)result;
}
