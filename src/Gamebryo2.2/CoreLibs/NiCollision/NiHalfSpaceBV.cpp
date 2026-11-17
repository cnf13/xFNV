#include "Gamebryo2.2/CoreLibs/NiCollision/NiHalfSpaceBV.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

// The plane through kPoint with normal kNormal, taken normalized by SetPlane.
NiHalfSpaceBV::NiHalfSpaceBV(const NiPoint3& kPoint, const NiPoint3& kNormal) : m_kCenter(kPoint) {
    NiPlane kPlane;
    kPlane.m_kNormal = kNormal;
    kPlane.m_fConstant = kPoint.y * kNormal.y + kPoint.x * kNormal.x + kPoint.z * kNormal.z;
    SetPlane(kPlane);
}

NiHalfSpaceBV::NiHalfSpaceBV(const NiHalfSpaceBV& kABV) : m_kCenter(kABV.m_kCenter) {
    SetPlane(kABV.m_kPlane);
}

void NiHalfSpaceBV::LoadBinary(NiStream& kStream) {
    NiBoundingVolume::LoadBinary(kStream);
    m_kPlane.LoadBinary(kStream);
    m_kCenter.LoadBinary(kStream);
}

void NiHalfSpaceBV::SaveBinary(NiStream& kStream) {
    NiBoundingVolume::SaveBinary(kStream);
    m_kPlane.SaveBinary(kStream);
    m_kCenter.SaveBinary(kStream);
}

// Rotates the normal and moves the plane by the translation and scale; the centre is left as it is.
void NiHalfSpaceBV::UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) {
    const NiPlane& kModel = static_cast<const NiHalfSpaceBV&>(kModelABV).m_kPlane;
    const float (*m)[3] = kWorld.m_Rotate.m_pEntry;
    const NiPoint3& n = kModel.m_kNormal;

    NiPoint3 kNormal;
    kNormal.x = m[0][0] * n.x + m[0][1] * n.y + m[0][2] * n.z;
    kNormal.y = m[1][0] * n.x + m[1][1] * n.y + m[1][2] * n.z;
    kNormal.z = m[2][0] * n.x + m[2][1] * n.y + m[2][2] * n.z;
    float fDistance =
        kNormal.x * kWorld.m_Translate.x + kNormal.y * kWorld.m_Translate.y + kNormal.z * kWorld.m_Translate.z;
    m_kPlane.m_kNormal = kNormal;
    m_kPlane.m_fConstant = fDistance + kModel.m_fConstant * kWorld.m_fScale;
}

// A new half space through the origin facing +Y.
NiBoundingVolume* NiHalfSpaceBV::Create() const {
    void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiHalfSpaceBV));
    if (!pvMemory) {
        return nullptr;
    }
    return ::new (pvMemory) NiHalfSpaceBV(NiPoint3_ZERO, NiPoint3_UNIT_Y);
}

NiBoundingVolume* NiHalfSpaceBV::Clone() const {
    NiHalfSpaceBV* v2 = static_cast<NiHalfSpaceBV*>(MemoryManager_Singleton.Allocate(sizeof(NiHalfSpaceBV)));
    if (!v2) {
        return nullptr;
    }
    return ::new (v2) NiHalfSpaceBV(*this);
}

// Only the plane is taken over.
void NiHalfSpaceBV::Copy(const NiBoundingVolume& kABV) {
    SetPlane(static_cast<const NiHalfSpaceBV&>(kABV).m_kPlane);
}

bool NiHalfSpaceBV::operator==(const NiBoundingVolume& kABV) const {
    return kABV.Type() == HALFSPACE_BV && static_cast<const NiHalfSpaceBV&>(kABV).m_kPlane == m_kPlane;
}

bool NiHalfSpaceBV::operator!=(const NiBoundingVolume& kABV) const {
    return !operator==(kABV);
}

void NiHalfSpaceBV::AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const {
    size_t stLen = strlen(pcPrefix) + 16;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "%s = HALFSPACE_BV", pcPrefix);
    pkStrings->Add(pcString);
}

void NiHalfSpaceBV::SetPlane(const NiPlane& kPlane) {
    NiPoint3 kNormal = kPlane.m_kNormal;
    double dConstant = kPlane.m_fConstant;
    float fLength = kNormal.UnitizeGetLength();
    m_kPlane.m_kNormal = kNormal;
    m_kPlane.m_fConstant = static_cast<float>(dConstant / fLength);
}

// Allocates an NiHalfSpaceBV, then calls LoadBinary through the vtable on the result; after a failed allocation that
// call is made through the null pointer too, as the game does.
NiBoundingVolume* NiHalfSpaceBV::CreateFromStream(NiStream& kStream) {
    NiHalfSpaceBV* v1 = static_cast<NiHalfSpaceBV*>(MemoryManager_Singleton.Allocate(sizeof(NiHalfSpaceBV)));
    NiHalfSpaceBV* pkABV = v1 ? ::new (v1) NiHalfSpaceBV(NiPoint3_ZERO, NiPoint3_UNIT_Y) : nullptr;
    pkABV->LoadBinary(kStream);
    return pkABV;
}

bool NiHalfSpaceBV::HalfSpaceSphereFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiSphereBV& kSphABV = static_cast<const NiSphereBV&>(kABV1);
    float kABV0b = kSphABV.m_kSphere.m_kCenter.y * kHalfABV.m_kPlane.m_kNormal.y +
        kHalfABV.m_kPlane.m_kNormal.x * kSphABV.m_kSphere.m_kCenter.x +
        kSphABV.m_kSphere.m_kCenter.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0c = kABV0b - kHalfABV.m_kPlane.m_fConstant;
    float kABV1a = -kSphABV.m_kSphere.m_fRadius;
    double v12 = kABV0c;
    if (!(kABV1a <= static_cast<double>(kABV0c))) {
        float v21 = kV1.x - kV0.x;
        float v23 = kV1.y - kV0.y;
        float v25 = kV1.z - kV0.z;
        float kABV0a = kHalfABV.m_kPlane.m_kNormal.x * v21 + kHalfABV.m_kPlane.m_kNormal.y * v23 +
            kHalfABV.m_kPlane.m_kNormal.z * v25;
        if (kABV0a <= 0.0f || !(kABV0a * fTime + v12 >= kABV1a)) {
            return false;
        }

        NiPoint3 v38;
        fIntrTime = -((v12 + kSphABV.m_kSphere.m_fRadius) / kABV0a);
        if (bCalcNormals) {
            NiPoint3 v37;
            kNormal1 = kHalfABV.m_kPlane.m_kNormal;
            v37.x = -kNormal1.x;
            v37.y = -kNormal1.y;
            v37.z = -kNormal1.z;
            kNormal0 = v37;
        }
        float kABV0d = kSphABV.m_kSphere.m_fRadius;
        float v27 = kABV0d * kNormal1.x;
        float v28 = kNormal1.y * kABV0d;
        float v29 = kABV0d * kNormal1.z;
        float kABV0e = fIntrTime;
        float v22 = kV1.x * kABV0e;
        float v24 = kABV0e * kV1.y;
        float v26 = kABV0e * kV1.z;
        float v16 = v22 + kSphABV.m_kSphere.m_kCenter.x;
        float v18 = kSphABV.m_kSphere.m_kCenter.y + v24;
        float v20 = kSphABV.m_kSphere.m_kCenter.z + v26;
        v38.x = v16 + v27;
        v38.y = v18 + v28;
        v38.z = v20 + v29;
        kIntrPt = v38;
        return true;
    }

    NiPoint3 v36;
    fIntrTime = 0.0f;
    float v15 = kHalfABV.m_kPlane.m_kNormal.x * v12;
    float v17 = kHalfABV.m_kPlane.m_kNormal.y * v12;
    float v19 = v12 * kHalfABV.m_kPlane.m_kNormal.z;
    v36.x = kSphABV.m_kSphere.m_kCenter.x - v15;
    v36.y = kSphABV.m_kSphere.m_kCenter.y - v17;
    v36.z = kSphABV.m_kSphere.m_kCenter.z - v19;
    kIntrPt = v36;
    if (bCalcNormals) {
        kNormal1 = kHalfABV.m_kPlane.m_kNormal;
        v36.x = -kNormal1.x;
        v36.y = -kNormal1.y;
        v36.z = -kNormal1.z;
        kNormal0 = v36;
    }
    return true;
}

// The swept capsule against the half space. The NiPoint3 arithmetic is done in the order written out below, with each
// component rounded to float in a temporary. The tolerance is the double 9.999999974752427e-07 (1e-06f widened). The
// NiSphereBV locals are constructed and never destroyed, deliberately, as the game does.
bool NiHalfSpaceBV::HalfSpaceCapsuleFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiCapsuleBV& kCapABV = static_cast<const NiCapsuleBV&>(kABV1);
    const NiPlane& kPlane = kHalfABV.m_kPlane;
    const NiCapsule& kCapsule = kCapABV.m_kCapsule;
    const NiPoint3& kOrigin = kCapsule.m_kSegment.m_kOrigin;
    const NiPoint3& kDirection = kCapsule.m_kSegment.m_kDirection;
    const NiPoint3& kNormal = kPlane.m_kNormal;

    // EXE-INLINED none: NiPlane::Distance
    float fDot = kNormal.x * kOrigin.x + kNormal.y * kOrigin.y + kNormal.z * kOrigin.z;
    float fSignedDistance0 = fDot - kPlane.m_fConstant;
    NiPoint3 kEnd;
    kEnd.x = kOrigin.x + kDirection.x;
    kEnd.y = kDirection.y + kOrigin.y;
    kEnd.z = kDirection.z + kOrigin.z;
    // EXE-INLINED none: NiPlane::Distance
    fDot = kNormal.x * kEnd.x + kNormal.y * kEnd.y + kNormal.z * kEnd.z;
    float fSignedDistance1 = fDot - kPlane.m_fConstant;
    if (((fSignedDistance0 < 0.0f) && (fSignedDistance1 > 0.0f)) ||
        ((fSignedDistance0 > 0.0f) && (fSignedDistance1 < 0.0f))) {
        fIntrTime = 0.0f;
        float fRatio = fSignedDistance0 / (fSignedDistance0 - fSignedDistance1);
        // EXE-INLINED 0x4A3760
        NiPoint3 kTmp;
        kTmp.x = kDirection.x * fRatio;
        kTmp.y = kDirection.y * fRatio;
        kTmp.z = fRatio * kDirection.z;
        NiPoint3 kPoint;
        kPoint.x = kOrigin.x + kTmp.x;
        kPoint.y = kOrigin.y + kTmp.y;
        kPoint.z = kOrigin.z + kTmp.z;
        kIntrPt = kPoint;
    }
    else {
        float fNegRadius = -kCapsule.m_fRadius;
        if (fSignedDistance0 >= fNegRadius) {
            fIntrTime = 0.0f;
            alignas(NiSphereBV) uint8_t aSphere[sizeof(NiSphereBV)];
            NiSphereBV* pSphABV = ::new (aSphere) NiSphereBV(1.0f, NiPoint3_ZERO);
            pSphABV->m_kSphere.m_fRadius = kCapsule.m_fRadius;
            if (fSignedDistance1 >= -kCapsule.m_fRadius) {
                pSphABV->m_kSphere.m_kCenter = kCapABV.m_kCenter;
            }
            else {
                pSphABV->m_kSphere.m_kCenter = kOrigin;
            }
            return HalfSpaceSphereFindIntersect(
                fTime, kABV0, kV0, *pSphABV, kV1, fIntrTime, kIntrPt, bCalcNormals, kNormal0, kNormal1
            );
        }
        if (fSignedDistance1 >= fNegRadius) {
            fIntrTime = 0.0f;
            alignas(NiSphereBV) uint8_t aSphere[sizeof(NiSphereBV)];
            NiSphereBV* pSphABV = ::new (aSphere) NiSphereBV(1.0f, NiPoint3_ZERO);
            pSphABV->m_kSphere.m_fRadius = kCapsule.m_fRadius;
            NiPoint3 kCenter;
            kCenter.x = kOrigin.x + kCapABV.m_kCapsule.m_kSegment.m_kDirection.x;
            pSphABV->m_kSphere.m_kCenter.x = kCenter.x;
            kCenter.y = kOrigin.y + kCapABV.m_kCapsule.m_kSegment.m_kDirection.y;
            pSphABV->m_kSphere.m_kCenter.y = kCenter.y;
            kCenter.z = kOrigin.z + kCapABV.m_kCapsule.m_kSegment.m_kDirection.z;
            pSphABV->m_kSphere.m_kCenter.z = kCenter.z;
            return HalfSpaceSphereFindIntersect(
                fTime, kABV0, kV0, *pSphABV, kV1, fIntrTime, kIntrPt, bCalcNormals, kNormal0, kNormal1
            );
        }
        // EXE-INLINED none: NiPoint3::operator-
        NiPoint3 kDeltaV;
        kDeltaV.x = kV1.x - kV0.x;
        kDeltaV.y = kV1.y - kV0.y;
        kDeltaV.z = kV1.z - kV0.z;
        // EXE-INLINED none: NiPoint3::Dot
        float fNdV = kNormal.x * kDeltaV.x + kNormal.y * kDeltaV.y + kNormal.z * kDeltaV.z;
        if (fNdV <= 0.0f) {
            return false;
        }
        float fRHS = -(fNdV * fTime + kCapsule.m_fRadius);
        if (!((fSignedDistance0 >= fRHS) || (fSignedDistance1 >= fRHS))) {
            return false;
        }
        if (fSignedDistance0 > fSignedDistance1 + 9.999999974752427e-07) {
            float fTime0 = -((fSignedDistance0 + kCapsule.m_fRadius) / fNdV);
            fIntrTime = fTime0;
            NiPoint3 kT1 = kNormal * kCapsule.m_fRadius;
            NiPoint3 kT2 = fTime0 * kV1;
            NiPoint3 kR1 = kOrigin + kT2;
            kIntrPt = kR1 + kT1;
        }
        else if (fSignedDistance1 > fSignedDistance0 + 9.999999974752427e-07) {
            float fTime1 = -((fSignedDistance1 + kCapsule.m_fRadius) / fNdV);
            fIntrTime = fTime1;
            NiPoint3 kT1 = kNormal * kCapsule.m_fRadius;
            NiPoint3 kT2 = fTime1 * kV1;
            NiPoint3 kR0 = kOrigin + kDirection;
            NiPoint3 kR1 = kR0 + kT2;
            kIntrPt = kR1 + kT1;
        }
        else {
            float fTime1 = -((fSignedDistance1 + kCapsule.m_fRadius) / fNdV);
            fIntrTime = fTime1;
            // EXE-INLINED 0x45BB20
            NiPoint3 kHalfDir;
            kHalfDir.x = kDirection.x * 0.5;
            kHalfDir.y = kDirection.y * 0.5;
            kHalfDir.z = 0.5 * kDirection.z;
            NiPoint3 kH;
            kH.x = kOrigin.x + kHalfDir.x;
            kH.y = kOrigin.y + kHalfDir.y;
            kH.z = kOrigin.z + kHalfDir.z;
            NiPoint3 kT2 = fTime1 * kV1;
            NiPoint3 kS;
            kS.x = kT2.x + kH.x;
            kS.y = kT2.y + kH.y;
            kS.z = kT2.z + kH.z;
            NiPoint3 kT1 = kNormal * kCapsule.m_fRadius;
            NiPoint3 kPoint;
            kPoint.x = kT1.x + kS.x;
            kPoint.y = kT1.y + kS.y;
            kPoint.z = kT1.z + kS.z;
            kIntrPt = kPoint;
        }
    }
    if (bCalcNormals) {
        kNormal1 = kNormal;
        NiPoint3 kNegated;
        kNegated.x = -kNormal1.x;
        kNegated.y = -kNormal1.y;
        kNegated.z = -kNormal1.z;
        kNormal0 = kNegated;
    }
    return true;
}

// Each product and sum is stored in a float local so that it is rounded to float precision.
bool NiHalfSpaceBV::HalfSpaceHalfSpaceFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiHalfSpaceBV& kHalfABV0 = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiPlane& kPlane0 = kHalfABV0.m_kPlane;
    const NiHalfSpaceBV& kHalfABV1 = static_cast<const NiHalfSpaceBV&>(kABV1);
    const NiPlane& kPlane1 = kHalfABV1.m_kPlane;

    float fDot = kPlane1.m_kNormal.x * kPlane0.m_kNormal.x + kPlane1.m_kNormal.y * kPlane0.m_kNormal.y +
        kPlane1.m_kNormal.z * kPlane0.m_kNormal.z;
    // Compares with the double nearest -0.999999f.
    if (fDot > -0.9999989867210388) {
        float fFactor = 1.0f / (1.0f - fDot * fDot);
        float fU0 = (kPlane0.m_fConstant - kPlane1.m_fConstant * fDot) * fFactor;
        float fU1 = (kPlane1.m_fConstant - kPlane0.m_fConstant * fDot) * fFactor;
        fIntrTime = 0.0f;
        float fB0 = kPlane1.m_kNormal.x * fU1;
        float fB1 = kPlane1.m_kNormal.y * fU1;
        float fB2 = fU1 * kPlane1.m_kNormal.z;
        float fA0 = kPlane0.m_kNormal.x * fU0;
        float fA1 = kPlane0.m_kNormal.y * fU0;
        float fA2 = fU0 * kPlane0.m_kNormal.z;
        kIntrPt.x = fA0 + fB0;
        kIntrPt.y = fB1 + fA1;
        kIntrPt.z = fA2 + fB2;
        if (bCalcNormals) {
            kNormal0.x = -kPlane0.m_kNormal.x;
            kNormal0.y = -kPlane0.m_kNormal.y;
            kNormal0.z = -kPlane0.m_kNormal.z;
            kNormal1.x = -kPlane1.m_kNormal.x;
            kNormal1.y = -kPlane1.m_kNormal.y;
            kNormal1.z = -kPlane1.m_kNormal.z;
        }
        return true;
    }

    if (kPlane0.m_fConstant <= -kPlane1.m_fConstant) {
        fIntrTime = 0.0f;
        float fDiff = kPlane0.m_fConstant - kPlane1.m_fConstant;
        float fX = kPlane0.m_kNormal.x * fDiff;
        float fY = kPlane0.m_kNormal.y * fDiff;
        float fZ = fDiff * kPlane0.m_kNormal.z;
        kIntrPt.x = fX;
        kIntrPt.y = fY;
        kIntrPt.z = fZ;
    }
    else {
        float fDeltaX = kV1.x - kV0.x;
        float fDeltaY = kV1.y - kV0.y;
        float fDeltaZ = kV1.z - kV0.z;
        float fNdV = kPlane0.m_kNormal.y * fDeltaY + kPlane0.m_kNormal.x * fDeltaX + kPlane0.m_kNormal.z * fDeltaZ;
        if (fNdV <= 0.0f) {
            return false;
        }
        if (!(fNdV * fTime - kPlane1.m_fConstant >= kPlane0.m_fConstant)) {
            return false;
        }
        float fT = (kPlane1.m_fConstant + kPlane0.m_fConstant) / fNdV;
        fIntrTime = fT;
        float fB0 = fT * kV0.x;
        float fB1 = kV0.y * fT;
        float fB2 = fT * kV0.z;
        float fConstant = kPlane0.m_fConstant;
        float fA0 = kPlane0.m_kNormal.x * fConstant;
        float fA1 = kPlane0.m_kNormal.y * fConstant;
        float fA2 = fConstant * kPlane0.m_kNormal.z;
        kIntrPt.x = fA0 + fB0;
        kIntrPt.y = fA1 + fB1;
        kIntrPt.z = fA2 + fB2;
    }
    if (bCalcNormals) {
        kNormal1 = kPlane0.m_kNormal;
        kNormal0 = kPlane1.m_kNormal;
    }
    return true;
}

bool NiHalfSpaceBV::HalfSpaceSphereTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiSphereBV& kSphABV = static_cast<const NiSphereBV&>(kABV1);
    float kABV0b = kSphABV.m_kSphere.m_kCenter.y * kHalfABV.m_kPlane.m_kNormal.y +
        kSphABV.m_kSphere.m_kCenter.x * kHalfABV.m_kPlane.m_kNormal.x +
        kSphABV.m_kSphere.m_kCenter.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0c = kABV0b - kHalfABV.m_kPlane.m_fConstant;
    float kABV1a = -kSphABV.m_kSphere.m_fRadius;
    double v6 = kABV0c;
    if (kABV1a <= static_cast<double>(kABV0c)) {
        return true;
    }
    float v8 = kV1.x - kV0.x;
    float v9 = kV1.y - kV0.y;
    float v10 = kV1.z - kV0.z;
    float kABV0a =
        kHalfABV.m_kPlane.m_kNormal.y * v9 + v8 * kHalfABV.m_kPlane.m_kNormal.x + kHalfABV.m_kPlane.m_kNormal.z * v10;
    return !(kABV0a <= 0.0f) && kABV1a <= v6 + kABV0a * fTime;
}

bool NiHalfSpaceBV::HalfSpaceBoxTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiBoxBV& kBoxABV = static_cast<const NiBoxBV&>(kABV1);
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiPoint3* akAxis = kBoxABV.m_kBox.m_akAxis;
    float kABV0b = akAxis[0].y * kHalfABV.m_kPlane.m_kNormal.y + akAxis[0].x * kHalfABV.m_kPlane.m_kNormal.x +
        akAxis[0].z * kHalfABV.m_kPlane.m_kNormal.z;
    float v11 = kABV0b * kBoxABV.m_kBox.m_afExtent[0];
    float kABV0c = akAxis[1].y * kHalfABV.m_kPlane.m_kNormal.y + akAxis[1].x * kHalfABV.m_kPlane.m_kNormal.x +
        akAxis[1].z * kHalfABV.m_kPlane.m_kNormal.z;
    float v13 = kABV0c * kBoxABV.m_kBox.m_afExtent[1];
    float kABV0d = akAxis[2].y * kHalfABV.m_kPlane.m_kNormal.y + akAxis[2].x * kHalfABV.m_kPlane.m_kNormal.x +
        akAxis[2].z * kHalfABV.m_kPlane.m_kNormal.z;
    float v15 = kABV0d * kBoxABV.m_kBox.m_afExtent[2];
    float kABV0e = std::fabs((double)v13);
    double v7 = kABV0e;
    float kABV0f = std::fabs((double)v11);
    double v8 = v7 + kABV0f;
    float kABV0g = std::fabs((double)v15);
    float kABV1b = v8 + kABV0g;
    float kABV0h = kBoxABV.m_kBox.m_kCenter.y * kHalfABV.m_kPlane.m_kNormal.y +
        kBoxABV.m_kBox.m_kCenter.x * kHalfABV.m_kPlane.m_kNormal.x +
        kBoxABV.m_kBox.m_kCenter.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0i = kABV0h - kHalfABV.m_kPlane.m_fConstant;
    float kABV1a = -kABV1b;
    double v9 = kABV0i;
    if (kABV1a <= static_cast<double>(kABV0i)) {
        return true;
    }
    float v12 = kV1.x - kV0.x;
    float v14 = kV1.y - kV0.y;
    float v16 = kV1.z - kV0.z;
    float kABV0a =
        kHalfABV.m_kPlane.m_kNormal.y * v14 + v12 * kHalfABV.m_kPlane.m_kNormal.x + v16 * kHalfABV.m_kPlane.m_kNormal.z;
    return !(kABV0a <= 0.0f) && kABV1a <= v9 + kABV0a * fTime;
}

bool NiHalfSpaceBV::HalfSpaceCapsuleTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiCapsuleBV& kCapABV = static_cast<const NiCapsuleBV&>(kABV1);
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    float kABV0c = kCapABV.m_kCapsule.m_kSegment.m_kOrigin.y * kHalfABV.m_kPlane.m_kNormal.y +
        kCapABV.m_kCapsule.m_kSegment.m_kOrigin.x * kHalfABV.m_kPlane.m_kNormal.x +
        kCapABV.m_kCapsule.m_kSegment.m_kOrigin.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0d = kABV0c - kHalfABV.m_kPlane.m_fConstant;
    float kABV1a = -kCapABV.m_kCapsule.m_fRadius;
    double v7 = kABV0d;
    if (kABV1a <= static_cast<double>(kABV0d)) {
        return true;
    }
    float kABV0e = kCapABV.m_kCapsule.m_kSegment.m_kDirection.y * kHalfABV.m_kPlane.m_kNormal.y +
        kCapABV.m_kCapsule.m_kSegment.m_kDirection.x * kHalfABV.m_kPlane.m_kNormal.x +
        kCapABV.m_kCapsule.m_kSegment.m_kDirection.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0f = kABV0e + v7;
    double v9 = kABV0f;
    if (kABV0f >= static_cast<double>(kABV1a)) {
        return true;
    }
    float v10 = kV1.x - kV0.x;
    float v11 = kV1.y - kV0.y;
    float v12 = kV1.z - kV0.z;
    float kABV0a =
        kHalfABV.m_kPlane.m_kNormal.y * v11 + v10 * kHalfABV.m_kPlane.m_kNormal.x + kHalfABV.m_kPlane.m_kNormal.z * v12;
    if (kABV0a <= 0.0f) {
        return false;
    }
    float kABV0b = -(kABV0a * fTime + kCapABV.m_kCapsule.m_fRadius);
    return kABV0b <= v7 || v9 >= kABV0b;
}

bool NiHalfSpaceBV::HalfSpaceHalfSpaceTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiHalfSpaceBV& kHalfABV0 = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiHalfSpaceBV& kHalfABV1 = static_cast<const NiHalfSpaceBV&>(kABV1);
    float kABV0b = kHalfABV1.m_kPlane.m_kNormal.y * kHalfABV0.m_kPlane.m_kNormal.y +
        kHalfABV0.m_kPlane.m_kNormal.x * kHalfABV1.m_kPlane.m_kNormal.x +
        kHalfABV1.m_kPlane.m_kNormal.z * kHalfABV0.m_kPlane.m_kNormal.z;
    if (kABV0b > -0.9999989867210388 || -kHalfABV1.m_kPlane.m_fConstant >= kHalfABV0.m_kPlane.m_fConstant) {
        return true;
    }
    float v7 = kV1.x - kV0.x;
    float v8 = kV1.y - kV0.y;
    float v9 = kV1.z - kV0.z;
    float kABV0a =
        kHalfABV0.m_kPlane.m_kNormal.x * v7 + kHalfABV0.m_kPlane.m_kNormal.y * v8 + kHalfABV0.m_kPlane.m_kNormal.z * v9;
    return !(kABV0a <= 0.0f) && kHalfABV0.m_kPlane.m_fConstant <= kABV0a * fTime - kHalfABV1.m_kPlane.m_fConstant;
}

bool NiHalfSpaceBV::HalfSpaceTriTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    float kABV0b = kVert0.y * kHalfABV.m_kPlane.m_kNormal.y + kVert0.x * kHalfABV.m_kPlane.m_kNormal.x +
        kVert0.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0c = kABV0b - kHalfABV.m_kPlane.m_fConstant;
    double v8 = kABV0c;
    if (kABV0c >= 0.0f) {
        return true;
    }
    float kABV0d = kVert1.y * kHalfABV.m_kPlane.m_kNormal.y + kVert1.x * kHalfABV.m_kPlane.m_kNormal.x +
        kVert1.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0e = kABV0d - kHalfABV.m_kPlane.m_fConstant;
    double v9 = kABV0e;
    if (kABV0e >= 0.0f) {
        return true;
    }
    float kABV0f = kVert2.y * kHalfABV.m_kPlane.m_kNormal.y + kVert2.x * kHalfABV.m_kPlane.m_kNormal.x +
        kVert2.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0g = kABV0f - kHalfABV.m_kPlane.m_fConstant;
    double v11 = kABV0g;
    if (kABV0g >= 0.0f) {
        return true;
    }
    float v15 = kV1.x - kV0.x;
    float v16 = kV1.y - kV0.y;
    float v17 = kV1.z - kV0.z;
    float kABV0a =
        kHalfABV.m_kPlane.m_kNormal.y * v16 + kHalfABV.m_kPlane.m_kNormal.x * v15 + kHalfABV.m_kPlane.m_kNormal.z * v17;
    if (kABV0a <= 0.0f) {
        return false;
    }
    float kABV0h = kABV0a * -fTime;
    bool v12 = !(kABV0h >= v8);
    bool v13 = kABV0h == v8;
    double v14 = kABV0h;
    return v12 || v13 || (v9 >= v14 || v14 <= v11);
}

// The box's extent projected on the plane normal against its centre's distance to the plane: touching at time 0 (the
// contact point the centre moved onto the plane), else the time the box reaches the plane, with the contact point the
// box's corner nearest the plane there. Each product and sum is rounded to float in a temporary.
bool NiHalfSpaceBV::HalfSpaceBoxFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    const NiBoxBV& kBoxABV = static_cast<const NiBoxBV&>(kABV1);
    const NiPoint3& kNormal = kHalfABV.m_kPlane.m_kNormal;
    const NiBox& kBox = kBoxABV.m_kBox;

    float afProj[3];
    for (int32_t i = 0; i < 3; i++) {
        const NiPoint3& kAxis = kBox.m_akAxis[i];
        float fDot = kAxis.y * kNormal.y + kNormal.x * kAxis.x + kAxis.z * kNormal.z;
        afProj[i] = fDot * kBox.m_afExtent[i];
    }
    float afAbs[3];
    afAbs[1] = std::fabs(static_cast<double>(afProj[1]));
    afAbs[0] = std::fabs(static_cast<double>(afProj[0]));
    afAbs[2] = std::fabs(static_cast<double>(afProj[2]));
    float fRadius = afAbs[0] + afAbs[1] + afAbs[2];
    float fCenterDot = kBox.m_kCenter.y * kNormal.y + kNormal.x * kBox.m_kCenter.x + kBox.m_kCenter.z * kNormal.z;
    float fDistance = fCenterDot - kHalfABV.m_kPlane.m_fConstant;
    float fNegRadius = -fRadius;

    if (fNegRadius <= fDistance) {
        fIntrTime = 0.0f;
        NiPoint3 kOffset;
        kOffset.x = kNormal.x * fDistance;
        kOffset.y = kNormal.y * fDistance;
        kOffset.z = fDistance * kNormal.z;
        NiPoint3 kPoint;
        kPoint.x = kBox.m_kCenter.x - kOffset.x;
        kPoint.y = kBox.m_kCenter.y - kOffset.y;
        kPoint.z = kBox.m_kCenter.z - kOffset.z;
        kIntrPt = kPoint;
        if (!bCalcNormals) {
            return true;
        }
        kNormal1 = kNormal;
        NiPoint3 kNegated;
        kNegated.x = -kNormal1.x;
        kNegated.y = -kNormal1.y;
        kNegated.z = -kNormal1.z;
        kNormal0 = kNegated;
        return true;
    }

    NiPoint3 kRelVel;
    kRelVel.x = kV1.x - kV0.x;
    kRelVel.y = kV1.y - kV0.y;
    kRelVel.z = kV1.z - kV0.z;
    float fSpeed = kNormal.x * kRelVel.x + kNormal.y * kRelVel.y + kRelVel.z * kNormal.z;
    if (fSpeed <= 0.0f) {
        return false;
    }
    if (!(fSpeed * fTime + fDistance >= fNegRadius)) {
        return false;
    }

    float fT = -((fRadius + fDistance) / fSpeed);
    fIntrTime = fT;
    NiPoint3 kMove;
    kMove.x = kV1.x * fT;
    kMove.y = kV1.y * fT;
    kMove.z = fT * kV1.z;
    NiPoint3 kPoint;
    kPoint.x = kBox.m_kCenter.x + kMove.x;
    kPoint.y = kMove.y + kBox.m_kCenter.y;
    kPoint.z = kMove.z + kBox.m_kCenter.z;
    kIntrPt = kPoint;

    // Along each axis not parallel to the plane, the point moves to the box face nearer the plane.
    for (int32_t i = 0; i < 3; i++) {
        if (afAbs[i] > 0.001f) {
            float fExtent = kBox.m_afExtent[i];
            const NiPoint3& kAxis = kBox.m_akAxis[i];
            NiPoint3 kOffset;
            kOffset.x = kAxis.x * fExtent;
            kOffset.y = kAxis.y * fExtent;
            kOffset.z = fExtent * kAxis.z;
            if (afProj[i] > 0.0f) {
                kIntrPt.x = kIntrPt.x + kOffset.x;
                kIntrPt.y = kIntrPt.y + kOffset.y;
                kIntrPt.z = kOffset.z + kIntrPt.z;
            }
            else {
                kIntrPt.x = kIntrPt.x - kOffset.x;
                kIntrPt.y = kIntrPt.y - kOffset.y;
                kIntrPt.z = kIntrPt.z - kOffset.z;
            }
        }
    }

    if (!bCalcNormals) {
        return true;
    }
    NiPoint3 kNegated;
    kNegated.x = -kNormal.x;
    kNegated.y = -kNormal.y;
    kNegated.z = -kNormal.z;
    kNormal0 = kNegated;
    NiPoint3 kDiff;
    kDiff.x = kIntrPt.x - kBox.m_kCenter.x;
    kDiff.y = kIntrPt.y - kBox.m_kCenter.y;
    kDiff.z = kIntrPt.z - kBox.m_kCenter.z;
    kNormal1 = kDiff;
    return true;
}

bool NiHalfSpaceBV::HalfSpaceTriFindIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1,
    float& fIntrTime,
    NiPoint3& kIntrPt,
    bool bCalcNormals,
    NiPoint3& kNormal0,
    NiPoint3& kNormal1
) {
    const NiHalfSpaceBV& kHalfABV = static_cast<const NiHalfSpaceBV&>(kABV0);
    NiPoint3 dst;
    NiPoint3 result;
    float kABV0a = kVert0.y * kHalfABV.m_kPlane.m_kNormal.y + kHalfABV.m_kPlane.m_kNormal.x * kVert0.x +
        kVert0.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0b = kABV0a - kHalfABV.m_kPlane.m_fConstant;
    double v13 = kABV0b;
    if (kABV0b >= 0.0f) {
        fIntrTime = 0.0f;
        kIntrPt = kVert0;
        if (bCalcNormals) {
            kNormal1 = kHalfABV.m_kPlane.m_kNormal;
            dst.x = -kNormal1.x;
            dst.y = -kNormal1.y;
            float y = dst.y;
            double v15 = -kNormal1.z;
            kNormal0.x = dst.x;
            dst.z = v15;
            float z = dst.z;
            kNormal0.y = y;
            kNormal0.z = z;
        }
        return true;
    }
    float kABV0c = kVert1.y * kHalfABV.m_kPlane.m_kNormal.y + kHalfABV.m_kPlane.m_kNormal.x * kVert1.x +
        kVert1.z * kHalfABV.m_kPlane.m_kNormal.z;
    float kABV0d = kABV0c - kHalfABV.m_kPlane.m_fConstant;
    double v18 = kABV0d;
    if (!(kABV0d >= 0.0f)) {
        NiPoint3* v28;
        NiPoint3* v34;
        float kABV0e = kVert2.y * kHalfABV.m_kPlane.m_kNormal.y + kVert2.x * kHalfABV.m_kPlane.m_kNormal.x +
            kVert2.z * kHalfABV.m_kPlane.m_kNormal.z;
        float kABV0f = kABV0e - kHalfABV.m_kPlane.m_fConstant;
        double v22 = kABV0f;
        if (kABV0f >= 0.0f) {
            fIntrTime = 0.0f;
            kIntrPt = kVert2;
            if (bCalcNormals) {
                kNormal1 = kHalfABV.m_kPlane.m_kNormal;
                dst.x = -kNormal1.x;
                dst.y = -kNormal1.y;
                float v23 = dst.y;
                double v24 = kNormal1.z;
                kNormal0.x = dst.x;
                dst.z = -v24;
                float v25 = dst.z;
                kNormal0.y = v23;
                kNormal0.z = v25;
                return true;
            }
            return true;
        }
        dst.x = kV1.x - kV0.x;
        dst.y = kV1.y - kV0.y;
        dst.z = kV1.z - kV0.z;
        float kABV0g = kHalfABV.m_kPlane.m_kNormal.y * dst.y + kHalfABV.m_kPlane.m_kNormal.x * dst.x +
            kHalfABV.m_kPlane.m_kNormal.z * dst.z;
        double v26 = kABV0g;
        if (kABV0g <= 0.0f) {
            return false;
        }
        float kABV0h = -fTime * v26;
        double v27 = kABV0h;
        if (!(kABV0h <= v13) && (!(v27 <= v18) && !(v27 <= v22))) {
            return false;
        }
        if (!(v13 >= v18)) {
            if (v22 <= v18) {
                fIntrTime = -v18 / v26;
                NiPoint3* v33 = ::new (&result) NiPoint3(fTime * kV1);
                v28 = ::new (&dst) NiPoint3(kVert1 + *v33);
                goto LABEL_23;
            }
        }
        else {
            if (v22 <= v13) {
                fIntrTime = -v13 / v26;
                NiPoint3* v32 = ::new (&dst) NiPoint3(fTime * kV1);
                v28 = ::new (&result) NiPoint3(kVert0 + *v32);
                goto LABEL_23;
            }
        }
        fIntrTime = -v22 / v26;
        v34 = ::new (&result) NiPoint3(fTime * kV1);
        v28 = ::new (&dst) NiPoint3(kVert2 + *v34);
    LABEL_23:;
        kIntrPt = *v28;
        if (bCalcNormals) {
            kNormal1 = kHalfABV.m_kPlane.m_kNormal;
            dst.x = -kNormal1.x;
            dst.y = -kNormal1.y;
            float v29 = dst.y;
            double v30 = -kNormal1.z;
            kNormal0.x = dst.x;
            dst.z = v30;
            float v31 = dst.z;
            kNormal0.y = v29;
            kNormal0.z = v31;
        }
        return true;
    }
    fIntrTime = 0.0f;
    kIntrPt = kVert1;
    if (bCalcNormals) {
        kNormal1 = kHalfABV.m_kPlane.m_kNormal;
        dst.x = -kNormal1.x;
        dst.y = -kNormal1.y;
        float v19 = dst.y;
        double v20 = -kNormal1.z;
        kNormal0.x = dst.x;
        dst.z = v20;
        float v21 = dst.z;
        kNormal0.y = v19;
        kNormal0.z = v21;
    }
    return true;
}
