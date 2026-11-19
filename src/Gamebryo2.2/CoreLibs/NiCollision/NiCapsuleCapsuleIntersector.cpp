#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleCapsuleIntersector.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"

NiCapsuleCapsuleIntersector::NiCapsuleCapsuleIntersector(
    const NiCapsuleBV& kCapsule0,
    const NiCapsuleBV& kCapsule1,
    float fMaxTime,
    float fDerivativeTimeStep,
    float fPseudodistanceThreshold,
    int32_t iMaxIterations
)
  : NiIntersector(fMaxTime, fDerivativeTimeStep, fPseudodistanceThreshold, iMaxIterations), m_kCapsule0(kCapsule0),
    m_kCapsule1(kCapsule1), m_fParameter0(3.4028235e+38f), m_fParameter1(3.4028235e+38f) {
    float fRadiusSum = kCapsule0.m_kCapsule.m_fRadius + kCapsule1.m_kCapsule.m_fRadius;
    m_fInvRadiusSumSqr = 1.0 / (fRadiusSum * fRadiusSum);
}

NiCapsuleCapsuleIntersector::~NiCapsuleCapsuleIntersector() {}

// The squared distance between the two moved capsule segments, scaled so that 0 is touching. The scalar products and
// sums are each rounded to float in a temporary, and the double 1.0 is subtracted. The parameters are mutable in
// Gamebryo (Compute writes them from this const method), hence the const_casts.
float NiCapsuleCapsuleIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
    NiSegment kSegment0 = m_kCapsule0.m_kCapsule.m_kSegment;
    NiSegment kSegment1 = m_kCapsule1.m_kCapsule.m_kSegment;
    NiPoint3 kOffset = fTime * kV0;
    kSegment0.m_kOrigin.x = kOffset.x + kSegment0.m_kOrigin.x;
    kSegment0.m_kOrigin.y = kSegment0.m_kOrigin.y + kOffset.y;
    kSegment0.m_kOrigin.z = kSegment0.m_kOrigin.z + kOffset.z;
    kOffset = fTime * kV1;
    kSegment1.m_kOrigin.x = kOffset.x + kSegment1.m_kOrigin.x;
    kSegment1.m_kOrigin.y = kOffset.y + kSegment1.m_kOrigin.y;
    kSegment1.m_kOrigin.z = kOffset.z + kSegment1.m_kOrigin.z;
    float fPseudodistance =
        NiSqrDistance::Compute(
            kSegment0, kSegment1, const_cast<float&>(m_fParameter0), const_cast<float&>(m_fParameter1)
        ) * m_fInvRadiusSumSqr -
        1.0;
    return fPseudodistance;
}

// The contact point and normal between the two capsules' closest segment points.
void NiCapsuleCapsuleIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
    NiPoint3 kSeg0 = m_kCapsule0.m_kCapsule.m_kSegment.GetPoint(m_fParameter0);
    NiPoint3 kSeg1 = m_kCapsule1.m_kCapsule.m_kSegment.GetPoint(m_fParameter1);
    NiPoint3 kTmp;
    if (m_eIntersectionType == IT_CONTACT) {
        kTmp = m_fContactTime * kV0;
        kSeg0.x = kTmp.x + kSeg0.x;
        kSeg0.y = kSeg0.y + kTmp.y;
        kSeg0.z = kSeg0.z + kTmp.z;
        kTmp = m_fContactTime * kV1;
        kSeg1.x = kTmp.x + kSeg1.x;
        kSeg1.y = kSeg1.y + kTmp.y;
        kSeg1.z = kSeg1.z + kTmp.z;
        kTmp.x = kSeg1.x - kSeg0.x;
        m_kContactNormal.x = kTmp.x;
        kTmp.y = kSeg1.y - kSeg0.y;
        m_kContactNormal.y = kTmp.y;
        kTmp.z = kSeg1.z - kSeg0.z;
        m_kContactNormal.z = kTmp.z;
        m_kContactNormal.Unitize();
        kTmp = m_kCapsule0.m_kCapsule.m_fRadius * m_kContactNormal;
        NiPoint3 kPoint;
        kPoint.x = kTmp.x + kSeg0.x;
        m_kContactPoint.x = kPoint.x;
        kPoint.y = kTmp.y + kSeg0.y;
        m_kContactPoint.y = kPoint.y;
        kPoint.z = kTmp.z + kSeg0.z;
        m_kContactPoint.z = kPoint.z;
    }
    else {
        kTmp.x = kSeg1.x + kSeg0.x;
        kTmp.y = kSeg1.y + kSeg0.y;
        kTmp.z = kSeg1.z + kSeg0.z;
        NiPoint3 kHalf;
        kHalf.x = kTmp.x * 0.5;
        m_kContactPoint.x = kHalf.x;
        kHalf.y = kTmp.y * 0.5;
        m_kContactPoint.y = kHalf.y;
        kHalf.z = 0.5 * kTmp.z;
        m_kContactPoint.z = kHalf.z;
        if (kSeg1 != kSeg0) {
            kHalf.x = kSeg1.x - kSeg0.x;
            m_kContactNormal.x = kHalf.x;
            kHalf.y = kSeg1.y - kSeg0.y;
            m_kContactNormal.y = kHalf.y;
            kHalf.z = kSeg1.z - kSeg0.z;
            m_kContactNormal.z = kHalf.z;
            m_kContactNormal.Unitize();
        }
        else {
            m_kContactNormal = NiPoint3_UNIT_Z;
        }
    }
}
