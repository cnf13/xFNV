#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Reconstruction.hpp"

class NiSphereBV;

// Gamebryo's intersector of a swept sphere against a triangle.
class NiSphereTriIntersector : public NiIntersector {
public:
	NiSphereTriIntersector(const NiSphereBV* a1, const NiPoint3* a2, const NiPoint3* a3, const NiPoint3* a4, float a5,
		float a6, float a7, int32_t a8);
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
inline constexpr fnvd::Vtable NiSphereTriIntersector_vftable{};
