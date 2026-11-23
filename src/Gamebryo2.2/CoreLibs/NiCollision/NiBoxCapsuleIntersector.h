#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"

class NiBoxBV;
class NiCapsuleBV;

// Gamebryo's intersector of a swept box against a capsule.
class NiBoxCapsuleIntersector : public NiIntersector {
public:
    NiBoxCapsuleIntersector(
        const NiBoxBV& kBox,
        const NiCapsuleBV& kCapsule,
        float fMaxTime,
        float fDerivativeTimeStep,
        float fPseudodistanceThreshold,
        int32_t iMaxIterations
    );
    ~NiBoxCapsuleIntersector() override;

    float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
    void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

    const NiBoxBV& m_kBox;
    const NiCapsuleBV& m_kCapsule;
    float m_fInvRadiusSqr;
    float m_afParameter[3];
};
