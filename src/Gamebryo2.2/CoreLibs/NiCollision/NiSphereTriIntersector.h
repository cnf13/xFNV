#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"

class NiSphereBV;

// Gamebryo's intersector of a swept sphere against a triangle.
class NiSphereTriIntersector : public NiIntersector {
public:
    NiSphereTriIntersector(
        const NiSphereBV* pkSphere,
        const NiPoint3* pkP0,
        const NiPoint3* pkP1,
        const NiPoint3* pkP2,
        float fMaxTime,
        float fDerivativeTimeStep,
        float fPseudodistanceThreshold,
        int32_t iMaxIterations
    );
    ~NiSphereTriIntersector() override;

    float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
    void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

    static void operator delete(void* apMemory);

    const NiSphereBV& m_kSphere;
    NiTrigon m_kTri;
    float m_fInvRadiusSqr;
    float m_fParameter0;
    float m_fParameter1;
};
