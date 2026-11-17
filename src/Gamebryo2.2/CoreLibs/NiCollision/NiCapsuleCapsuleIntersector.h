#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"

class NiCapsuleBV;

// Gamebryo's intersector of two swept capsules.
class NiCapsuleCapsuleIntersector : public NiIntersector {
public:
	NiCapsuleCapsuleIntersector(const NiCapsuleBV& kCapsule0, const NiCapsuleBV& kCapsule1, float fMaxTime,
		float fDerivativeTimeStep, float fPseudodistanceThreshold, int32_t iMaxIterations);
	~NiCapsuleCapsuleIntersector() override;

	float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
	void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

	const NiCapsuleBV& m_kCapsule0;
	const NiCapsuleBV& m_kCapsule1;
	float m_fInvRadiusSumSqr;
	float m_fParameter0;
	float m_fParameter1;
};
