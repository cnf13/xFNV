#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxCapsuleIntersector.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoxBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

NiBoxCapsuleIntersector::NiBoxCapsuleIntersector(
    const NiBoxBV& kBox,
    const NiCapsuleBV& kCapsule,
    float fMaxTime,
    float fDerivativeTimeStep,
    float fPseudodistanceThreshold,
    int32_t iMaxIterations
)
  : NiIntersector(fMaxTime, fDerivativeTimeStep, fPseudodistanceThreshold, iMaxIterations), m_kBox(kBox),
    m_kCapsule(kCapsule), m_fInvRadiusSqr(1.0 / (kCapsule.m_kCapsule.m_fRadius * kCapsule.m_kCapsule.m_fRadius)) {
    for (int32_t i = 0; i < 3; i++) {
        m_afParameter[i] = 3.4028235e+38f;
    }
}

NiBoxCapsuleIntersector::~NiBoxCapsuleIntersector() {}

// The squared distance between the moved capsule segment and the moved box, scaled so that 0 is touching. The scalar
// products and sums are each rounded to float in a temporary. The parameters are mutable in Gamebryo, hence the
// const_casts.
float NiBoxCapsuleIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
    // EXE-INLINED 0x4A3760
    NiPoint3 kOffset;
    kOffset.x = kV0.x * fTime;
    NiBox kMBox = m_kBox.m_kBox;
    kOffset.y = kV0.y * fTime;
    kOffset.z = kV0.z * fTime;
    kMBox.m_kCenter.x = kOffset.x + kMBox.m_kCenter.x;
    kMBox.m_kCenter.y = kMBox.m_kCenter.y + kOffset.y;
    kMBox.m_kCenter.z = kMBox.m_kCenter.z + kOffset.z;
    NiSegment kMSegment = m_kCapsule.m_kCapsule.m_kSegment;
    // EXE-INLINED 0x4A3760
    kOffset.x = kV1.x * fTime;
    kOffset.y = kV1.y * fTime;
    kOffset.z = kV1.z * fTime;
    kMSegment.m_kOrigin.x = kOffset.x + kMSegment.m_kOrigin.x;
    kMSegment.m_kOrigin.y = kMSegment.m_kOrigin.y + kOffset.y;
    kMSegment.m_kOrigin.z = kMSegment.m_kOrigin.z + kOffset.z;
    float fS;
    float fPseudodistance =
        NiSqrDistance::Compute(
            kMSegment, kMBox, fS, const_cast<float&>(m_afParameter[0]), const_cast<float&>(m_afParameter[1]),
            const_cast<float&>(m_afParameter[2])
        ) * m_fInvRadiusSqr -
        1.0;
    return fPseudodistance;
}

// The contact point is the moved box centre plus the parameters along the box axes, and the normal the box's face, edge
// or corner normal the parameters' signs select. Each product and sum is rounded to float in a temporary or straight
// into the field.
void NiBoxCapsuleIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
    // EXE-INLINED 0x4A3760
    float fContactTime = m_fContactTime;
    NiPoint3 kTmp;
    kTmp.x = kV0.x * fContactTime;
    kTmp.y = kV0.y * fContactTime;
    kTmp.z = fContactTime * kV0.z;
    const NiPoint3& kBoxCenter = m_kBox.m_kBox.m_kCenter;
    NiPoint3 kPoint;
    kPoint.x = kBoxCenter.x + kTmp.x;
    kPoint.y = kBoxCenter.y + kTmp.y;
    kPoint.z = kBoxCenter.z + kTmp.z;
    m_kContactPoint = kPoint;
    int32_t aiSign[3];
    for (int32_t i = 0; i < 3; i++) {
        // EXE-INLINED 0x4A3760
        float fParameter = m_afParameter[i];
        const NiPoint3& kAxis = m_kBox.m_kBox.m_akAxis[i];
        kTmp.x = kAxis.x * fParameter;
        kTmp.y = kAxis.y * fParameter;
        kTmp.z = fParameter * kAxis.z;
        m_kContactPoint.x = m_kContactPoint.x + kTmp.x;
        m_kContactPoint.y = m_kContactPoint.y + kTmp.y;
        m_kContactPoint.z = m_kContactPoint.z + kTmp.z;

        if (m_afParameter[i] == -m_kBox.m_kBox.m_afExtent[i]) {
            aiSign[i] = -1;
        }
        else if (m_afParameter[i] == m_kBox.m_kBox.m_afExtent[i]) {
            aiSign[i] = 1;
        }
        else {
            aiSign[i] = 0;
        }
    }

    // EXE-INLINED 0x439EF0
    NiPoint3 kRelVel;
    kRelVel.x = kV1.x - kV0.x;
    kRelVel.y = kV1.y - kV0.y;
    kRelVel.z = kV1.z - kV0.z;
    NiPoint3 kNormal;
    m_kContactNormal = *m_kBox.GetNormal(&kNormal, aiSign[0], aiSign[1], aiSign[2], kRelVel);
}
