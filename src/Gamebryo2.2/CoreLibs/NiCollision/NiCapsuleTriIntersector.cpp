#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleTriIntersector.h"

#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsuleBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

NiCapsuleTriIntersector::NiCapsuleTriIntersector(const NiCapsuleBV& kCapsule, const NiPoint3& kP0, const NiPoint3& kP1,
	const NiPoint3& kP2, float fMaxTime, float fDerivativeTimeStep, float fPseudodistanceThreshold,
	int32_t iMaxIterations)
	: NiIntersector(fMaxTime, fDerivativeTimeStep, fPseudodistanceThreshold, iMaxIterations), m_kCapsule(kCapsule),
	  m_fInvRadiusSqr(1.0 / (kCapsule.m_kCapsule.m_fRadius * kCapsule.m_kCapsule.m_fRadius)),
	  m_fParameter0(3.4028235e+38f), m_fParameter1(3.4028235e+38f) {
	m_kTri.m_kOrigin = kP0;
	m_kTri.m_kEdge0.x = kP1.x - kP0.x;
	m_kTri.m_kEdge0.y = kP1.y - kP0.y;
	m_kTri.m_kEdge0.z = kP1.z - kP0.z;
	m_kTri.m_kEdge1.x = kP2.x - kP0.x;
	m_kTri.m_kEdge1.y = kP2.y - kP0.y;
	m_kTri.m_kEdge1.z = kP2.z - kP0.z;
}

NiCapsuleTriIntersector::~NiCapsuleTriIntersector() {}

// The squared distance between the moved capsule segment and the moved triangle, scaled so that 0 is touching. The
// exe inlines the scalar products and sums, each rounded to float in a temporary, copies the triangle with rep movsd
// and subtracts the double 1.0. The parameters are mutable in Gamebryo, hence the const_casts.
float NiCapsuleTriIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
	NiSegment kMSegment = m_kCapsule.m_kCapsule.m_kSegment;
	NiTrigon kMTri = m_kTri;
	// EXE-INLINED 0x4A3760
	NiPoint3 kOffset;
	kOffset.x = kV0.x * fTime;
	kOffset.y = kV0.y * fTime;
	kOffset.z = kV0.z * fTime;
	kMSegment.m_kOrigin.x = kOffset.x + kMSegment.m_kOrigin.x;
	kMSegment.m_kOrigin.y = kMSegment.m_kOrigin.y + kOffset.y;
	kMSegment.m_kOrigin.z = kMSegment.m_kOrigin.z + kOffset.z;
	// EXE-INLINED 0x4A3760
	kOffset.x = kV1.x * fTime;
	kOffset.y = kV1.y * fTime;
	kOffset.z = kV1.z * fTime;
	kMTri.m_kOrigin.x = kOffset.x + kMTri.m_kOrigin.x;
	kMTri.m_kOrigin.y = kMTri.m_kOrigin.y + kOffset.y;
	kMTri.m_kOrigin.z = kMTri.m_kOrigin.z + kOffset.z;
	float fSegParameter;
	float fPseudodistance = NiSqrDistance::Compute(kMSegment, kMTri, fSegParameter, const_cast<float&>(m_fParameter0),
								const_cast<float&>(m_fParameter1)) *
			m_fInvRadiusSqr -
		1.0;
	return fPseudodistance;
}

void NiCapsuleTriIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
	double m_fParameter1;
	double y;
	double z;
	double m_fContactTime;
	float m_fParameter0;
	NiPoint3 v22;
	m_fParameter1 = (double)this->m_fParameter1;
	NiTrigon* p_m_kTri = &m_kTri;
	float v14 = (float)m_fParameter1;
	float v23 = m_kTri.m_kEdge1.x * v14;
	float v24 = m_kTri.m_kEdge1.y * v14;
	float v25 = v14 * m_kTri.m_kEdge1.z;
	m_fParameter0 = this->m_fParameter0;
	float v16 = m_kTri.m_kEdge0.x * m_fParameter0;
	float v18 = m_kTri.m_kEdge0.y * m_fParameter0;
	float v20 = m_fParameter0 * m_kTri.m_kEdge0.z;
	v22.x = (float)(p_m_kTri->m_kOrigin.x + v16);
	v22.y = (float)(p_m_kTri->m_kOrigin.y + v18);
	v22.z = (float)(p_m_kTri->m_kOrigin.z + v20);
	float v17 = v22.x + v23;
	y = (double)v22.y;
	m_kContactPoint.x = v17;
	float v19 = (float)(y + v24);
	z = (double)v22.z;
	m_kContactPoint.y = v19;
	float v21 = (float)(z + v25);
	m_kContactPoint.z = v21;
	NiPoint3* normal = p_m_kTri->GetNormal(&v22);
	uint8_t v9 = (uint8_t)(((int32_t)m_eIntersectionType) == 2);
	v23 = -normal->x;
	v24 = -normal->y;
	float v10 = v24;
	double v11 = (double)normal->z;
	m_kContactNormal.x = v23;
	v25 = (float)(-v11);
	float v12 = v25;
	m_kContactNormal.y = v10;
	m_kContactNormal.z = v12;
	if (v9) {
		m_fContactTime = (double)this->m_fContactTime;
		v23 = (float)((&kV1)->x * m_fContactTime);
		v24 = (float)((&kV1)->y * m_fContactTime);
		v25 = (float)(m_fContactTime * (&kV1)->z);
		m_kContactPoint.x = (float)(m_kContactPoint.x + v23);
		m_kContactPoint.y = (float)(m_kContactPoint.y + v24);
		m_kContactPoint.z = (float)(v25 + m_kContactPoint.z);
	}
}
