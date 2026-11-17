#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Reconstruction.hpp"

class NiCapsuleBV;

// Gamebryo's intersector of a swept capsule against a triangle.
class NiCapsuleTriIntersector : public NiIntersector {
public:
	NiCapsuleTriIntersector(const NiCapsuleBV& kCapsule, const NiPoint3& kP0, const NiPoint3& kP1, const NiPoint3& kP2,
		float fMaxTime, float fDerivativeTimeStep, float fPseudodistanceThreshold, int32_t iMaxIterations);
	~NiCapsuleTriIntersector() override;

	float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
	void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

	const NiCapsuleBV& m_kCapsule;
	NiTrigon m_kTri;
	float m_fInvRadiusSqr;
	float m_fParameter0;
	float m_fParameter1;
};
inline constexpr fnvd::Vtable NiCapsuleTriIntersector_vftable{};
