#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"

class NiCapsuleBV;
class NiSphereBV;

// Gamebryo's intersector of a swept capsule against a sphere.
class NiCapsuleSphereIntersector : public NiIntersector {
public:
    NiCapsuleSphereIntersector(
        const NiCapsuleBV& kCapsule,
        const NiSphereBV& kSphere,
        float fMaxTime,
        float fDerivativeTimeStep,
        float fPseudodistanceThreshold,
        int32_t iMaxIterations
    );
    ~NiCapsuleSphereIntersector() override;

    float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
    void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

    const NiCapsuleBV& m_kCapsule;
    const NiSphereBV& m_kSphere;
    float m_fInvRadiusSumSqr;
    float m_fParameter;
};
