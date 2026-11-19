#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxSphereIntersector.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"

// This file's copy of NI_INFINITY (FLT_MAX), which the constructor loads.
static float fNiInfinity = 3.4028235e+38f;

NiBoxSphereIntersector::NiBoxSphereIntersector(
    const NiBoxBV& kBox,
    const NiSphereBV& kSphere,
    float fMaxTime,
    float fDerivativeTimeStep,
    float fPseudodistanceThreshold,
    int32_t iMaxIterations
)
  : NiIntersector(fMaxTime, fDerivativeTimeStep, fPseudodistanceThreshold, iMaxIterations), m_kBox(kBox),
    m_kSphere(kSphere), m_fInvRadiusSqr(1.0 / (kSphere.m_kSphere.m_fRadius * kSphere.m_kSphere.m_fRadius)) {
    for (float& fParameter : m_afParameter) {
        fParameter = 3.4028235e+38f;
    }
}

NiBoxSphereIntersector::~NiBoxSphereIntersector() {}

// The squared distance from the moved sphere centre to the moved box, scaled so that 0 is touching. The parameters are
// mutable in Gamebryo, hence the const_casts.
float NiBoxSphereIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
    NiBox kMBox = m_kBox.m_kBox;
    NiPoint3 kOffset = fTime * kV0;
    kMBox.m_kCenter.x = kOffset.x + kMBox.m_kCenter.x;
    kMBox.m_kCenter.y = kMBox.m_kCenter.y + kOffset.y;
    kMBox.m_kCenter.z = kMBox.m_kCenter.z + kOffset.z;
    kOffset = fTime * kV1;
    const NiPoint3& kCenter = m_kSphere.m_kSphere.m_kCenter;
    NiPoint3 kMCenter;
    kMCenter.y = kCenter.y + kOffset.y;
    kMCenter.z = kCenter.z + kOffset.z;
    kMCenter.x = kCenter.x + kOffset.x;
    float fPseudodistance =
        NiSqrDistance::Compute(
            kMCenter, kMBox, const_cast<float&>(m_afParameter[0]), const_cast<float&>(m_afParameter[1]),
            const_cast<float&>(m_afParameter[2])
        ) * m_fInvRadiusSqr -
        1.0;
    return fPseudodistance;
}

// The contact point and normal between the box and the sphere.
void NiBoxSphereIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
    NiPoint3 kTmp;
    if (m_eIntersectionType == IT_CONTACT) {
        kTmp = m_fContactTime * kV0;
        const NiPoint3& kBoxCenter = m_kBox.m_kBox.m_kCenter;
        NiPoint3 kPoint;
        kPoint.x = kTmp.x + kBoxCenter.x;
        kPoint.y = kBoxCenter.y + kTmp.y;
        kPoint.z = kBoxCenter.z + kTmp.z;
        m_kContactPoint = kPoint;
        for (int32_t i = 0; i < 3; i++) {
            kTmp = m_afParameter[i] * m_kBox.m_kBox.m_akAxis[i];
            m_kContactPoint.x = m_kContactPoint.x + kTmp.x;
            m_kContactPoint.y = m_kContactPoint.y + kTmp.y;
            m_kContactPoint.z = kTmp.z + m_kContactPoint.z;
        }
        kTmp = m_fContactTime * kV1;
        const NiPoint3& kSphereCenter = m_kSphere.m_kSphere.m_kCenter;
        NiPoint3 kMCenter;
        kMCenter.x = kSphereCenter.x + kTmp.x;
        kMCenter.y = kSphereCenter.y + kTmp.y;
        kMCenter.z = kSphereCenter.z + kTmp.z;
        NiPoint3 kNormal;
        kNormal.x = kMCenter.x - m_kContactPoint.x;
        kNormal.y = kMCenter.y - m_kContactPoint.y;
        kNormal.z = kMCenter.z - m_kContactPoint.z;
        m_kContactNormal = kNormal;
        m_kContactNormal.Unitize();
        return;
    }

    float fEpsilon = 1e-05f;
    float fV1SqrLength = kV1.SqrLength();
    float fV0SqrLength = kV0.SqrLength();
    float fDeltaVSqr = fV1SqrLength - fV0SqrLength;
    if (fDeltaVSqr < -fEpsilon) {
        NiPoint3 kV0Unit = kV0;
        kV0Unit.Unitize();
        const NiSphere& kSphere = m_kSphere.m_kSphere;
        float fDiameter = kSphere.m_fRadius + kSphere.m_fRadius;
        kTmp = fDiameter * kV0Unit;
        const NiPoint3& kBoxCenter = m_kBox.m_kBox.m_kCenter;
        NiPoint3 kOffset;
        kOffset.x = kBoxCenter.x - kTmp.x;
        kOffset.y = kBoxCenter.y - kTmp.y;
        kOffset.z = kBoxCenter.z - kTmp.z;
        const NiPoint3& kSphereCenter = kSphere.m_kCenter;
        NiPoint3 kNormal1;
        kNormal1.x = kOffset.x - kSphereCenter.x;
        kNormal1.y = kOffset.y - kSphereCenter.y;
        kNormal1.z = kOffset.z - kSphereCenter.z;
        kNormal1.Unitize();
        kTmp = kNormal1 * kSphere.m_fRadius;
        NiPoint3 kPoint;
        kPoint.x = kSphereCenter.x + kTmp.x;
        kPoint.y = kSphereCenter.y + kTmp.y;
        kPoint.z = kSphereCenter.z + kTmp.z;
        m_kContactPoint = kPoint;
        NiPoint3 kNegated;
        kNegated.x = -kNormal1.x;
        m_kContactNormal.x = kNegated.x;
        kNegated.y = -kNormal1.y;
        m_kContactNormal.y = kNegated.y;
        kNegated.z = -kNormal1.z;
        m_kContactNormal.z = kNegated.z;
        return;
    }
    if (fDeltaVSqr > fEpsilon) {
        m_kContactPoint = m_kBox.m_kBox.m_kCenter;
        for (int32_t i = 0; i < 3; i++) {
            kTmp = m_afParameter[i] * m_kBox.m_kBox.m_akAxis[i];
            m_kContactPoint.x = m_kContactPoint.x + kTmp.x;
            m_kContactPoint.y = m_kContactPoint.y + kTmp.y;
            m_kContactPoint.z = kTmp.z + m_kContactPoint.z;
        }
    }
    else {
        const NiPoint3& kSphereCenter = m_kSphere.m_kSphere.m_kCenter;
        const NiPoint3& kBoxCenter = m_kBox.m_kBox.m_kCenter;
        NiPoint3 kSum;
        kSum.x = kSphereCenter.x + kBoxCenter.x;
        kSum.y = kSphereCenter.y + kBoxCenter.y;
        kSum.z = kSphereCenter.z + kBoxCenter.z;
        NiPoint3 kHalf;
        kHalf.x = kSum.x * 0.5;
        m_kContactPoint.x = kHalf.x;
        kHalf.y = kSum.y * 0.5;
        m_kContactPoint.y = kHalf.y;
        kHalf.z = kSum.z * 0.5;
        m_kContactPoint.z = kHalf.z;
    }
    const NiPoint3& kSphereCenter = m_kSphere.m_kSphere.m_kCenter;
    NiPoint3 kNormal;
    kNormal.x = kSphereCenter.x - m_kContactPoint.x;
    kNormal.y = kSphereCenter.y - m_kContactPoint.y;
    kNormal.z = kSphereCenter.z - m_kContactPoint.z;
    m_kContactNormal = kNormal;
    m_kContactNormal.Unitize();
}
