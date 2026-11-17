#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Reconstruction.hpp"

class NiBoxBV;
class NiSphereBV;

// Gamebryo's intersector of a swept box against a sphere.
class NiBoxSphereIntersector : public NiIntersector {
public:
	NiBoxSphereIntersector(const NiBoxBV& kBox, const NiSphereBV& kSphere, float fMaxTime, float fDerivativeTimeStep,
		float fPseudodistanceThreshold, int32_t iMaxIterations);
	~NiBoxSphereIntersector() override;

	float Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const override;
	void ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) override;

	const NiBoxBV& m_kBox;
	const NiSphereBV& m_kSphere;
	float m_fInvRadiusSqr;
	float m_afParameter[3];
};
inline constexpr fnvd::Vtable NiBoxSphereIntersector_vftable{};
