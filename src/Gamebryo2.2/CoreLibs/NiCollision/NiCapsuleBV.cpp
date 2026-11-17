#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsule.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleCapsuleIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleSphereIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleTriIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiParallelogram.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSegment.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>
#include <optional>

NiCapsuleBV::NiCapsuleBV(const NiCapsuleBV& arCapsule)
  : m_kCenter(arCapsule.m_kCenter), m_kAxis(arCapsule.m_kAxis), m_fExtent(arCapsule.m_fExtent) {
    m_kCapsule.m_fRadius = arCapsule.m_kCapsule.m_fRadius;
    CalculateCapsule();
}

NiCapsuleBV::NiCapsuleBV(float fRadius, float fExtent, const NiPoint3& kCenter, const NiPoint3& kAxis)
  : m_kCenter(kCenter), m_kAxis(kAxis), m_fExtent(fExtent) {
    m_kCapsule.m_fRadius = fRadius;
    CalculateCapsule();
}

void NiCapsuleBV::LoadBinary(NiStream& stream) {
    NiBoundingVolume::LoadBinary(stream);
    m_kCenter.LoadBinary(stream);
    m_kAxis.LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fExtent);
    NiStreamLoadBinary(stream, m_kCapsule.m_fRadius);
    CalculateCapsule();
}

void NiCapsuleBV::SaveBinary(NiStream& stream) {
    NiBoundingVolume::SaveBinary(stream);
    m_kCenter.SaveBinary(stream);
    m_kAxis.SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fExtent);
    NiStreamSaveBinary(stream, m_kCapsule.m_fRadius);
}

// Rotates, scales and translates the centre, rotates the axis, scales the lengths, then rebuilds the segment. It keeps
// the scale in the dead arModelABV argument slot (a local here) and reloads it for the lengths after the stores.
void NiCapsuleBV::UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) {
    const NiCapsuleBV& kModel = static_cast<const NiCapsuleBV&>(kModelABV);
    const float (*m)[3] = kWorld.m_Rotate.m_pEntry;
    const NiPoint3& c = kModel.m_kCenter;

    float fRotX = m[0][1] * c.y + m[0][0] * c.x + m[0][2] * c.z;
    float fRotY = m[1][1] * c.y + m[1][0] * c.x + m[1][2] * c.z;
    float fRotZ = m[2][1] * c.y + m[2][0] * c.x + m[2][2] * c.z;
    float fScale = kWorld.m_fScale;
    float fScaledX = fRotX * fScale;
    float fScaledY = fRotY * fScale;
    float fScaledZ = fScale * fRotZ;
    float fX = kWorld.m_Translate.x + fScaledX;
    float fY = kWorld.m_Translate.y + fScaledY;
    float fZ = kWorld.m_Translate.z + fScaledZ;
    m_kCenter.x = fX;
    m_kCenter.y = fY;
    m_kCenter.z = fZ;

    const NiPoint3& a = kModel.m_kAxis;
    float fAxisX = m[0][0] * a.x + m[0][1] * a.y + m[0][2] * a.z;
    float fAxisY = m[1][1] * a.y + m[1][0] * a.x + m[1][2] * a.z;
    float fAxisZ = m[2][0] * a.x + m[2][1] * a.y + m[2][2] * a.z;
    m_kAxis.x = fAxisX;
    m_kAxis.y = fAxisY;
    m_kAxis.z = fAxisZ;
    m_fExtent = kModel.m_fExtent * kWorld.m_fScale;
    m_kCapsule.m_fRadius = kModel.m_kCapsule.m_fRadius * kWorld.m_fScale;
    CalculateCapsule();
}

NiBoundingVolume* NiCapsuleBV::Create() const {
    NiCapsuleBV* v1 = static_cast<NiCapsuleBV*>(MemoryManager_Singleton.Allocate(sizeof(NiCapsuleBV)));
    if (!v1) {
        return 0;
    }
    return ::new (v1) NiCapsuleBV(1.0f, 1.0f, NiPoint3_ZERO, NiPoint3_UNIT_X);
}

NiBoundingVolume* NiCapsuleBV::Clone() const {
    void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiCapsuleBV));
    if (!pvMemory) {
        return nullptr;
    }
    return ::new (pvMemory) NiCapsuleBV(*this);
}

void NiCapsuleBV::Copy(const NiBoundingVolume& kABV) {
    const NiCapsuleBV& kOther = static_cast<const NiCapsuleBV&>(kABV);
    m_kCenter = kOther.m_kCenter;
    m_kAxis = kOther.m_kAxis;
    m_fExtent = kOther.m_fExtent;
    m_kCapsule.m_fRadius = kOther.m_kCapsule.m_fRadius;
    CalculateCapsule();
}

bool NiCapsuleBV::operator==(const NiBoundingVolume& kABV) const {
    return kABV.Type() == CAPSULE_BV && static_cast<const NiCapsuleBV&>(kABV).m_kCapsule == m_kCapsule;
}

bool NiCapsuleBV::operator!=(const NiBoundingVolume& kABV) const {
    return !operator==(kABV);
}

void NiCapsuleBV::AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const {
    size_t stLen = strlen(pcPrefix) + 14;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    NiSprintf(pcString, stLen, "%s = CAPSULE_BV", pcPrefix);
    pkStrings->Add(pcString);
}

// The segment runs through the centre along the axis, stopping a radius short of each end. Each component is rounded to
// float in a temporary, and the direction is doubled with the double 2.0.
void NiCapsuleBV::CalculateCapsule() {
    float fHalfLength = m_fExtent - m_kCapsule.m_fRadius;
    // EXE-INLINED 0x45BB20
    NiPoint3 kTmp;
    kTmp.x = m_kAxis.x * fHalfLength;
    kTmp.y = m_kAxis.y * fHalfLength;
    kTmp.z = fHalfLength * m_kAxis.z;
    // EXE-INLINED 0x439EF0
    NiPoint3 kOrigin;
    kOrigin.x = m_kCenter.x - kTmp.x;
    kOrigin.y = m_kCenter.y - kTmp.y;
    kOrigin.z = m_kCenter.z - kTmp.z;
    m_kCapsule.m_kSegment.m_kOrigin = kOrigin;
    // EXE-INLINED 0x45BB20
    kTmp.x = m_kAxis.x * fHalfLength;
    kTmp.y = m_kAxis.y * fHalfLength;
    kTmp.z = fHalfLength * m_kAxis.z;
    m_kCapsule.m_kSegment.m_kDirection.x = kTmp.x * 2.0;
    m_kCapsule.m_kSegment.m_kDirection.y = kTmp.y * 2.0;
    m_kCapsule.m_kSegment.m_kDirection.z = kTmp.z * 2.0;
}

// NiNew NiCapsuleBV, then its LoadBinary through the vtable.
NiBoundingVolume* NiCapsuleBV::CreateFromStream(NiStream& stream) {
    NiCapsuleBV* pBV = static_cast<NiCapsuleBV*>(MemoryManager_Singleton.Allocate(sizeof(NiCapsuleBV)));
    if (pBV) {
        ::new (pBV) NiCapsuleBV(1.0f, 1.0f, NiPoint3_ZERO, NiPoint3_UNIT_X);
    }
    pBV->LoadBinary(stream);
    return pBV;
}

bool NiCapsuleBV::CapsuleSphereFindIntersect(
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
    // The intersector holds references: the native side constructs it in its declaration.
    NiCapsuleSphereIntersector kIntersector(
        static_cast<const NiCapsuleBV&>(kABV0), static_cast<const NiSphereBV&>(kABV1), fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType == NiIntersector::IT_OVERLAP ||
        kIntersector.m_eIntersectionType == NiIntersector::IT_CONTACT) {
        kIntrPt = kIntersector.GetContactPoint();
        if (bCalcNormals) {
            kNormal0 = kIntersector.GetContactNormal();
            // EXE-INLINED 0x4A0BD0
            NiPoint3 kNegated;
            kNegated.x = -kNormal0.x;
            kNegated.y = -kNormal0.y;
            kNegated.z = -kNormal0.z;
            kNormal1 = kNegated;
        }
        return true;
    }
    return false;
}

bool NiCapsuleBV::CapsuleCapsuleFindIntersect(
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
    // The intersector holds references: the native side constructs it in its declaration.
    NiCapsuleCapsuleIntersector kIntersector(
        static_cast<const NiCapsuleBV&>(kABV0), static_cast<const NiCapsuleBV&>(kABV1), fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType == NiIntersector::IT_OVERLAP ||
        kIntersector.m_eIntersectionType == NiIntersector::IT_CONTACT) {
        kIntrPt = kIntersector.GetContactPoint();
        if (bCalcNormals) {
            kNormal0 = kIntersector.GetContactNormal();
            // EXE-INLINED 0x4A0BD0
            NiPoint3 kNegated;
            kNegated.x = -kNormal0.x;
            kNegated.y = -kNormal0.y;
            kNegated.z = -kNormal0.z;
            kNormal1 = kNegated;
        }
        return true;
    }
    return false;
}

bool NiCapsuleBV::CapsuleSphereTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiCapsuleBV& kCapABV = static_cast<const NiCapsuleBV&>(kABV0);
    const NiSphereBV& kSphABV = static_cast<const NiSphereBV&>(kABV1);

    // The relative velocity of the sphere with respect to the capsule.
    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaV;
    kDeltaV.x = kV1.x - kV0.x;
    kDeltaV.y = kV1.y - kV0.y;
    kDeltaV.z = kV1.z - kV0.z;
    float fSqrDist;
    float fS0;

    // EXE-INLINED 0x439090
    if (!(NiPoint3_ZERO.x == kDeltaV.x && NiPoint3_ZERO.y == kDeltaV.y && NiPoint3_ZERO.z == kDeltaV.z)) {
        // The sphere's centre swept over the time interval, against the capsule's segment.
        NiSegment kSegment;
        kSegment.m_kOrigin = kSphABV.m_kSphere.m_kCenter;
        // EXE-INLINED 0x4A3760
        kSegment.m_kDirection.x = fTime * kDeltaV.x;
        kSegment.m_kDirection.y = fTime * kDeltaV.y;
        kSegment.m_kDirection.z = fTime * kDeltaV.z;
        float fS1;
        fSqrDist = NiSqrDistance::Compute(kSegment, kCapABV.m_kCapsule.m_kSegment, fS0, fS1);
    }
    else {
        // No relative motion: the sphere's centre against the capsule's segment.
        fSqrDist = NiSqrDistance::Compute(kSphABV.m_kSphere.m_kCenter, kCapABV.m_kCapsule.m_kSegment, fS0);
    }

    float fRadiusSum = kCapABV.m_kCapsule.m_fRadius + kSphABV.m_kSphere.m_fRadius;
    float fRadiusSumSqr = fRadiusSum * fRadiusSum;
    return fSqrDist <= fRadiusSumSqr;
}

bool NiCapsuleBV::CapsuleCapsuleTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiBoundingVolume& kABV1,
    const NiPoint3& kV1
) {
    const NiCapsuleBV& kCapABV0 = static_cast<const NiCapsuleBV&>(kABV0);
    const NiCapsuleBV& kCapABV1 = static_cast<const NiCapsuleBV&>(kABV1);

    // The relative velocity of the second capsule with respect to the first, each component rounded to a float.
    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaV;
    kDeltaV.x = kV1.x - kV0.x;
    kDeltaV.y = kV1.y - kV0.y;
    kDeltaV.z = kV1.z - kV0.z;
    float fSqrDist;
    float fS0;

    // EXE-INLINED 0x439090
    if (!(NiPoint3_ZERO.x == kDeltaV.x && NiPoint3_ZERO.y == kDeltaV.y && NiPoint3_ZERO.z == kDeltaV.z)) {
        // The second capsule's segment swept over the time interval, against the first capsule's segment.
        NiParallelogram kPgm;
        kPgm.m_kOrigin = kCapABV1.m_kCapsule.m_kSegment.m_kOrigin;
        kPgm.m_kEdge0 = kCapABV1.m_kCapsule.m_kSegment.m_kDirection;
        // EXE-INLINED 0x4A3760
        kPgm.m_kEdge1.x = fTime * kDeltaV.x;
        kPgm.m_kEdge1.y = fTime * kDeltaV.y;
        kPgm.m_kEdge1.z = fTime * kDeltaV.z;
        float fP0;
        float fP1;
        fSqrDist = NiSqrDistance::Compute(kCapABV0.m_kCapsule.m_kSegment, kPgm, fS0, fP0, fP1);
    }
    else {
        // No relative motion: segment against segment.
        float fS1;
        fSqrDist = NiSqrDistance::Compute(kCapABV0.m_kCapsule.m_kSegment, kCapABV1.m_kCapsule.m_kSegment, fS0, fS1);
    }

    float fRadiusSum = kCapABV0.m_kCapsule.m_fRadius + kCapABV1.m_kCapsule.m_fRadius;
    float fRadiusSumSqr = fRadiusSum * fRadiusSum;
    return fSqrDist <= fRadiusSumSqr;
}

bool NiCapsuleBV::CapsuleTriTestIntersect(
    float fTime,
    const NiBoundingVolume& kABV0,
    const NiPoint3& kV0,
    const NiPoint3& kVert0,
    const NiPoint3& kVert1,
    const NiPoint3& kVert2,
    const NiPoint3& kV1
) {
    const NiCapsuleBV& kCapABV = static_cast<const NiCapsuleBV&>(kABV0);

    NiTrigon kTri;
    kTri.m_kOrigin = kVert0;
    // EXE-INLINED 0x439EF0
    kTri.m_kEdge0.x = kVert1.x - kVert0.x;
    kTri.m_kEdge0.y = kVert1.y - kVert0.y;
    kTri.m_kEdge0.z = kVert1.z - kVert0.z;
    // EXE-INLINED 0x439EF0
    kTri.m_kEdge1.x = kVert2.x - kVert0.x;
    kTri.m_kEdge1.y = kVert2.y - kVert0.y;
    kTri.m_kEdge1.z = kVert2.z - kVert0.z;

    // EXE-INLINED 0x439EF0
    NiPoint3 kDeltaV;
    kDeltaV.x = kV0.x - kV1.x;
    kDeltaV.y = kV0.y - kV1.y;
    kDeltaV.z = kV0.z - kV1.z;

    // The negated cross product of the edges.
    // EXE-INLINED 0x4B3800
    NiPoint3 kCross;
    kCross.x = kTri.m_kEdge0.y * kTri.m_kEdge1.z - kTri.m_kEdge0.z * kTri.m_kEdge1.y;
    kCross.y = kTri.m_kEdge0.z * kTri.m_kEdge1.x - kTri.m_kEdge0.x * kTri.m_kEdge1.z;
    kCross.z = kTri.m_kEdge0.x * kTri.m_kEdge1.y - kTri.m_kEdge0.y * kTri.m_kEdge1.x;
    // EXE-INLINED 0x4A0BD0
    NiPoint3 kTriNormal;
    kTriNormal.x = -kCross.x;
    kTriNormal.y = -kCross.y;
    kTriNormal.z = -kCross.z;

    // EXE-INLINED 0x4B6190
    float fNdotC =
        kTriNormal.x * kCapABV.m_kCenter.x + kTriNormal.y * kCapABV.m_kCenter.y + kTriNormal.z * kCapABV.m_kCenter.z;
    float PseudoRadius =
        (kCapABV.m_fExtent > kCapABV.m_kCapsule.m_fRadius) ? kCapABV.m_fExtent : kCapABV.m_kCapsule.m_fRadius;
    // EXE-INLINED 0x4B6190
    float fNdotT = kTriNormal.x * kVert0.x + kTriNormal.y * kVert0.y + kTriNormal.z * kVert0.z;
    // EXE-INLINED 0x4A7290
    float fSqrLength = kTriNormal.x * kTriNormal.x + kTriNormal.y * kTriNormal.y + kTriNormal.z * kTriNormal.z;
    float fNormRadiusSqr = fSqrLength * PseudoRadius * PseudoRadius;

    // The capsule's bounding sphere lies off the triangle's plane: rule out a capsule moving away from it, or one
    // that stays in front of it over the interval.
    float fDistCapTri = fNdotC - fNdotT;
    if (fDistCapTri * fDistCapTri > fNormRadiusSqr) {
        NiPoint3 kSurfacePt;
        kTriNormal.Unitize();
        // EXE-INLINED 0x4B6190
        float fVdotN = kDeltaV.x * kTriNormal.x + kDeltaV.y * kTriNormal.y + kDeltaV.z * kTriNormal.z;
        if (fNdotC > fNdotT) {
            if (fVdotN >= 0.0f) {
                return false;
            }
            // EXE-INLINED 0x4A3760
            NiPoint3 kOffset;
            kOffset.x = PseudoRadius * kTriNormal.x;
            kOffset.y = PseudoRadius * kTriNormal.y;
            kOffset.z = PseudoRadius * kTriNormal.z;
            // EXE-INLINED 0x439EF0
            kSurfacePt.x = kCapABV.m_kCenter.x - kOffset.x;
            kSurfacePt.y = kCapABV.m_kCenter.y - kOffset.y;
            kSurfacePt.z = kCapABV.m_kCenter.z - kOffset.z;
        }
        else {
            if (fVdotN <= 0.0f) {
                return false;
            }
            // EXE-INLINED 0x4A3760
            NiPoint3 kOffset;
            kOffset.x = PseudoRadius * kTriNormal.x;
            kOffset.y = PseudoRadius * kTriNormal.y;
            kOffset.z = PseudoRadius * kTriNormal.z;
            // EXE-INLINED 0x439E90
            kSurfacePt.x = kCapABV.m_kCenter.x + kOffset.x;
            kSurfacePt.y = kCapABV.m_kCenter.y + kOffset.y;
            kSurfacePt.z = kCapABV.m_kCenter.z + kOffset.z;
        }

        // EXE-INLINED 0x4B6190
        fNdotT = kTriNormal.x * kVert0.x + kTriNormal.y * kVert0.y + kTriNormal.z * kVert0.z;
        const double fLimit = fNdotT + 0.0010000000474974513;
        // EXE-INLINED 0x4B6190
        float fNdotS = kTriNormal.x * kSurfacePt.x + kTriNormal.y * kSurfacePt.y + kTriNormal.z * kSurfacePt.z;
        if (fNdotS > fLimit) {
            // EXE-INLINED 0x4A3760
            NiPoint3 kStep;
            kStep.x = fTime * kDeltaV.x;
            kStep.y = fTime * kDeltaV.y;
            kStep.z = fTime * kDeltaV.z;
            // EXE-INLINED 0x439E90
            NiPoint3 kSwept;
            kSwept.x = kStep.x + kSurfacePt.x;
            kSwept.y = kStep.y + kSurfacePt.y;
            kSwept.z = kStep.z + kSurfacePt.z;
            // EXE-INLINED 0x4B6190
            float fNdotSwept = kTriNormal.x * kSwept.x + kTriNormal.y * kSwept.y + kTriNormal.z * kSwept.z;
            if (fNdotSwept > fLimit) {
                return false;
            }
        }
    }

    float fSqrDist;
    float fT0;
    float fT1;
    // EXE-INLINED 0x439090
    if (!(NiPoint3_ZERO.x == kDeltaV.x && NiPoint3_ZERO.y == kDeltaV.y && NiPoint3_ZERO.z == kDeltaV.z)) {
        // The triangle against the capsule's segment swept over the time interval.
        NiParallelogram kPgm;
        kPgm.m_kOrigin = kCapABV.m_kCapsule.m_kSegment.m_kOrigin;
        kPgm.m_kEdge0 = kCapABV.m_kCapsule.m_kSegment.m_kDirection;
        // EXE-INLINED 0x4A3760
        kPgm.m_kEdge1.x = fTime * kDeltaV.x;
        kPgm.m_kEdge1.y = fTime * kDeltaV.y;
        kPgm.m_kEdge1.z = fTime * kDeltaV.z;
        float fP0;
        float fP1;
        fSqrDist = NiSqrDistance::Compute(kTri, kPgm, fT0, fT1, fP0, fP1);
    }
    else {
        // No relative motion: the capsule's segment against the triangle.
        float fS0;
        fSqrDist = NiSqrDistance::Compute(kCapABV.m_kCapsule.m_kSegment, kTri, fS0, fT0, fT1);
    }
    return fSqrDist <= kCapABV.m_kCapsule.m_fRadius * kCapABV.m_kCapsule.m_fRadius;
}

// The plane offset plus the double 1e-3 is kept as a double across the Dot call.
bool NiCapsuleBV::CapsuleTriFindIntersect(
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
    double m_fRadius;
    double fLimit;
    double v42;
    NiPoint3* v43;
    NiPoint3 kSurfacePt_2;
    float v61;
    float v63;
    NiPoint3 kTriNormal;
    NiPoint3 result;
    NiPoint3 kSurfacePt;
    NiSegment kSeg;
    NiPoint3 v70;
    NiTrigon kTri;
    float fSegParm;
    float fTriParm0;
    float fTriParm1;
    NiPoint3 v76;
    float x = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kOrigin.x;
    float y = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kOrigin.y;
    float z = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kOrigin.z;
    float v72 = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_fRadius *
        static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_fRadius;
    kSeg.m_kOrigin.x = x;
    double v15 = kVert1.x - kVert0.x;
    kSeg.m_kDirection.x = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kDirection.x;
    float v16 = kVert0.x;
    kSeg.m_kOrigin.y = y;
    float v48 = v15;
    float v17 = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kDirection.y;
    double v18 = kVert1.y;
    kTri.m_kOrigin.x = v16;
    double v19 = v18 - kVert0.y;
    kSeg.m_kDirection.y = v17;
    float v20 = kVert0.y;
    float v52 = v19;
    kSeg.m_kOrigin.z = z;
    float v21 = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_kSegment.m_kDirection.z;
    double v22 = kVert1.z - kVert0.z;
    kTri.m_kOrigin.y = v20;
    kSeg.m_kDirection.z = v21;
    float v23 = kVert0.z;
    float v55 = v22;
    double v24 = kVert2.x;
    kTri.m_kEdge0.x = v48;
    double v25 = v24 - kVert0.x;
    kTri.m_kOrigin.z = v23;
    kTri.m_kEdge0.y = v52;
    float v49 = v25;
    double v26 = kVert2.y;
    kTri.m_kEdge1.x = v49;
    double v27 = v26 - kVert0.y;
    kTri.m_kEdge0.z = v55;
    float v53 = v27;
    double v28 = kVert2.z;
    kTri.m_kEdge1.y = v53;
    float v56 = v28 - kVert0.z;
    kTri.m_kEdge1.z = v56;
    double v29 = NiSqrDistance::Compute(kSeg, kTri, fSegParm, fTriParm0, fTriParm1);
    result.x = v29 - v72;
    if (result.x <= 0.0f) {
        NiPoint3 v78;
        fIntrTime = 0.0f;
        result.x = kTri.m_kEdge1.x * fTriParm1;
        result.y = kTri.m_kEdge1.y * fTriParm1;
        result.z = fTriParm1 * kTri.m_kEdge1.z;
        float v50 = kTri.m_kEdge0.x * fTriParm0;
        float v54 = kTri.m_kEdge0.y * fTriParm0;
        float v57 = fTriParm0 * kTri.m_kEdge0.z;
        kTriNormal.x = kTri.m_kOrigin.x + v50;
        kTriNormal.y = kTri.m_kOrigin.y + v54;
        kTriNormal.z = kTri.m_kOrigin.z + v57;
        v78.x = kTriNormal.x + result.x;
        double v30 = kTriNormal.y;
        kIntrPt.x = v78.x;
        v78.y = v30 + result.y;
        double v31 = kTriNormal.z;
        kIntrPt.y = v78.y;
        v78.z = v31 + result.z;
        kIntrPt.z = v78.z;
        if (bCalcNormals) {
            kNormal1 = *kTri.GetNormal(&kSurfacePt);
            // EXE-INLINED 0x4A0BD0
            v78.x = -kNormal1.x;
            v78.y = -kNormal1.y;
            v78.z = -kNormal1.z;
            kNormal0 = v78;
            return 1;
        }
        return 1;
    }
    kSurfacePt_2.x = kV0.x - kV1.x;
    double v34 = kV0.y;
    v70.x = kSurfacePt_2.x;
    kSurfacePt_2.y = v34 - kV1.y;
    double v35 = kV0.z - kV1.z;
    v70.y = kSurfacePt_2.y;
    kSurfacePt_2.z = v35;
    v70.z = kSurfacePt_2.z;
    result.x = kSurfacePt_2.x * kSurfacePt_2.x + kSurfacePt_2.y * kSurfacePt_2.y + kSurfacePt_2.z * kSurfacePt_2.z;
    if (result.x * fTime <= 9.999999747378752e-06) {
        return 0;
    }
    result.x = kTri.m_kEdge0.y * kTri.m_kEdge1.z - kTri.m_kEdge0.z * kTri.m_kEdge1.y;
    result.y = kTri.m_kEdge0.z * kTri.m_kEdge1.x - kTri.m_kEdge1.z * kTri.m_kEdge0.x;
    result.z = kTri.m_kEdge1.y * kTri.m_kEdge0.x - kTri.m_kEdge0.y * kTri.m_kEdge1.x;
    kTriNormal.x = -result.x;
    kTriNormal.y = -result.y;
    kTriNormal.z = -result.z;
    double v36 = kTriNormal.y;
    double v37 = kTriNormal.x;
    double v38 = kTriNormal.z;
    result.x = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCenter.z * kTriNormal.z +
        (static_cast<const NiCapsuleBV*>(&kABV0)->m_kCenter.y * kTriNormal.y +
         kTriNormal.x * static_cast<const NiCapsuleBV*>(&kABV0)->m_kCenter.x);
    if (!(static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_fRadius <
          static_cast<double>(static_cast<const NiCapsuleBV*>(&kABV0)->m_fExtent))) {
        m_fRadius = static_cast<const NiCapsuleBV*>(&kABV0)->m_kCapsule.m_fRadius;
    }
    else {
        m_fRadius = static_cast<const NiCapsuleBV*>(&kABV0)->m_fExtent;
    }
    float v66 = m_fRadius;
    float v67 = v37 * kVert0.x + kVert0.y * v36 + v38 * kVert0.z;
    float v58 = result.x - v67;
    double v40 = v38 * v38 + (v36 * v36 + v37 * v37);
    double v41 = v58 * v58;
    float v59 = v40;
    float v60 = v59 * v66 * v66;
    if (!(v60 < v41)) {
        goto LABEL_16;
    }
    kTriNormal.Unitize();
    v42 = kTriNormal.x * kSurfacePt_2.x + kTriNormal.y * kSurfacePt_2.y + kTriNormal.z * kSurfacePt_2.z;
    if (!(v67 < static_cast<double>(result.x))) {
        float v62 = v42;
        if (!(v62 <= 0.0f)) {
            NiPoint3* v44 = ::new (&kSurfacePt) NiPoint3(v66 * kTriNormal);
            kSurfacePt_2 = *::new (&result) NiPoint3(static_cast<const NiCapsuleBV*>(&kABV0)->m_kCenter + *v44);
            goto LABEL_14;
        }
        return 0;
    }
    v61 = v42;
    if (v61 >= 0.0f) {
        return 0;
    }
    v43 = ::new (&kSurfacePt) NiPoint3(v66 * kTriNormal);
    kSurfacePt_2 = *::new (&result) NiPoint3(static_cast<const NiCapsuleBV*>(&kABV0)->m_kCenter - *v43);
LABEL_14:;
    v67 = kVert0.y * kTriNormal.y + kTriNormal.x * kVert0.x + kTriNormal.z * kVert0.z;
    fLimit = v67 + 0.0010000000474974513;
    v63 = kTriNormal.y * kSurfacePt_2.y + kTriNormal.x * kSurfacePt_2.x + kTriNormal.z * kSurfacePt_2.z;
    if (v63 > fLimit) {
        NiPoint3* v45 = ::new (&v76) NiPoint3(fTime * v70);
        kSurfacePt.x = kSurfacePt_2.x + v45->x;
        kSurfacePt.y = v45->y + kSurfacePt_2.y;
        kSurfacePt.z = v45->z + kSurfacePt_2.z;
        double v46 = kTriNormal.Dot(kSurfacePt);
        if (v46 > fLimit) {
            return 0;
        }
    }
LABEL_16:;
    NiCapsuleTriIntersector kIntersector(
        static_cast<const NiCapsuleBV&>(kABV0), kVert0, kVert1, kVert2, fTime, 0.001f, 0.0001f, 32
    );
    kIntersector.NiIntersector::Find(kV0, kV1);
    fIntrTime = kIntersector.GetContactTime();
    if (kIntersector.m_eIntersectionType != NiIntersector::IT_OVERLAP &&
        kIntersector.m_eIntersectionType != NiIntersector::IT_CONTACT) {
        return 0;
    }
    kIntrPt = kIntersector.GetContactPoint();
    if (bCalcNormals) {
        kNormal0 = kIntersector.GetContactNormal();
        // EXE-INLINED 0x4A0BD0
        kSurfacePt_2.x = -kNormal0.x;
        kSurfacePt_2.y = -kNormal0.y;
        kSurfacePt_2.z = -kNormal0.z;
        kNormal1 = kSurfacePt_2;
    }
    return 1;
}
