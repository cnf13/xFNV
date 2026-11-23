#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleSphereIntersector.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"

#include <limits>

NiCapsuleSphereIntersector::NiCapsuleSphereIntersector(
    const NiCapsuleBV& kCapsule,
    const NiSphereBV& kSphere,
    float fMaxTime,
    float fDerivativeTimeStep,
    float fPseudodistanceThreshold,
    int32_t iMaxIterations
)
  : NiIntersector(fMaxTime, fDerivativeTimeStep, fPseudodistanceThreshold, iMaxIterations), m_kCapsule(kCapsule),
    m_kSphere(kSphere), m_fParameter(std::numeric_limits<float>::max()) {
    float fRadiusSum = kCapsule.m_kCapsule.m_fRadius + kSphere.m_kSphere.m_fRadius;
    m_fInvRadiusSumSqr = 1.0 / (fRadiusSum * fRadiusSum);
}

NiCapsuleSphereIntersector::~NiCapsuleSphereIntersector() {}

// The squared distance from the moved sphere centre to the moved capsule segment, scaled so that 0 is touching. The
// scalar products and sums are each rounded to float in a temporary, and the double 1.0 is subtracted.
float NiCapsuleSphereIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
    NiSegment kMSegment = m_kCapsule.m_kCapsule.m_kSegment;
    NiPoint3 kOffset = fTime * kV0;
    kMSegment.m_kOrigin.x = kOffset.x + kMSegment.m_kOrigin.x;
    kMSegment.m_kOrigin.y = kMSegment.m_kOrigin.y + kOffset.y;
    kMSegment.m_kOrigin.z = kMSegment.m_kOrigin.z + kOffset.z;
    kOffset = fTime * kV1;
    const NiPoint3& kCenter = m_kSphere.m_kSphere.m_kCenter;
    NiPoint3 kMCenter;
    kMCenter.y = kCenter.y + kOffset.y;
    kMCenter.z = kCenter.z + kOffset.z;
    kMCenter.x = kCenter.x + kOffset.x;
    float fPseudodistance = NiSqrDistance::Compute(kMCenter, kMSegment, const_cast<float&>(m_fParameter)) *
            m_fInvRadiusSumSqr -
        1.0;
    return fPseudodistance;
}

// The contact point and normal between the capsule's closest segment point and the sphere centre.
void NiCapsuleSphereIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
    NiPoint3 kSeg = m_kCapsule.m_kCapsule.m_kSegment.GetPoint(m_fParameter);
    NiPoint3 kCen = m_kSphere.m_kSphere.m_kCenter;
    NiPoint3 kTmp;
    if (m_eIntersectionType == IT_CONTACT) {
        kTmp = m_fContactTime * kV0;
        kSeg.x = kTmp.x + kSeg.x;
        kSeg.y = kSeg.y + kTmp.y;
        kSeg.z = kSeg.z + kTmp.z;
        kTmp = m_fContactTime * kV1;
        kCen.x = kTmp.x + kCen.x;
        kCen.y = kCen.y + kTmp.y;
        kCen.z = kCen.z + kTmp.z;
        kTmp.x = kCen.x - kSeg.x;
        m_kContactNormal.x = kTmp.x;
        kTmp.y = kCen.y - kSeg.y;
        m_kContactNormal.y = kTmp.y;
        kTmp.z = kCen.z - kSeg.z;
        m_kContactNormal.z = kTmp.z;
        m_kContactNormal.Unitize();
        kTmp = m_kCapsule.m_kCapsule.m_fRadius * m_kContactNormal;
        NiPoint3 kPoint;
        kPoint.x = kTmp.x + kSeg.x;
        m_kContactPoint.x = kPoint.x;
        kPoint.y = kTmp.y + kSeg.y;
        m_kContactPoint.y = kPoint.y;
        kPoint.z = kTmp.z + kSeg.z;
        m_kContactPoint.z = kPoint.z;
    }
    else {
        kTmp.x = kCen.x + kSeg.x;
        kTmp.y = kCen.y + kSeg.y;
        kTmp.z = kCen.z + kSeg.z;
        NiPoint3 kHalf;
        kHalf.x = kTmp.x * 0.5;
        m_kContactPoint.x = kHalf.x;
        kHalf.y = kTmp.y * 0.5;
        m_kContactPoint.y = kHalf.y;
        kHalf.z = 0.5 * kTmp.z;
        m_kContactPoint.z = kHalf.z;
        if (kSeg != kCen) {
            kHalf.x = kCen.x - kSeg.x;
            m_kContactNormal.x = kHalf.x;
            kHalf.y = kCen.y - kSeg.y;
            m_kContactNormal.y = kHalf.y;
            kHalf.z = kCen.z - kSeg.z;
            m_kContactNormal.z = kHalf.z;
            m_kContactNormal.Unitize();
        }
        else {
            m_kContactNormal = NiPoint3::UNIT_Z;
        }
    }
}
