#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereTriIntersector.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiIntersector.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

NiSphereTriIntersector::NiSphereTriIntersector(const NiSphereBV* a1, const NiPoint3* a2, const NiPoint3* a3,
	const NiPoint3* a4, float a5, float a6, float a7, int32_t a8)
	: NiIntersector(a5, a6, a7, a8), m_kSphere(*a1),
	  m_fInvRadiusSqr(1.0 / (a1->m_kSphere.m_fRadius * a1->m_kSphere.m_fRadius)), m_fParameter0(NI_INFINITY),
	  m_fParameter1(NI_INFINITY) {
	m_kTri.m_kOrigin = *a2;
	m_kTri.m_kEdge0 = *a3 - *a2;
	m_kTri.m_kEdge1 = *a4 - *a2;
}

NiSphereTriIntersector::~NiSphereTriIntersector() {}

// The squared distance from the moved sphere centre to the moved triangle, scaled so that 0 is touching. The exe
// inlines the scalar products and sums, each rounded to float in a temporary, copies the triangle with rep movsd
// and subtracts the double 1.0. The parameters are mutable in Gamebryo, hence the const_casts.
float NiSphereTriIntersector::Pseudodistance(const NiPoint3& kV0, const NiPoint3& kV1, float fTime) const {
	// EXE-INLINED 0x4A3760
	NiPoint3 kOffset;
	kOffset.x = kV0.x * fTime;
	kOffset.y = kV0.y * fTime;
	kOffset.z = kV0.z * fTime;
	const NiPoint3& kCenter = m_kSphere.m_kSphere.m_kCenter;
	NiPoint3 kMCenter;
	kMCenter.y = kCenter.y + kOffset.y;
	kMCenter.z = kCenter.z + kOffset.z;
	kMCenter.x = kCenter.x + kOffset.x;
	NiTrigon kMTri = m_kTri;
	// EXE-INLINED 0x4A3760
	kOffset.x = kV1.x * fTime;
	kOffset.y = kV1.y * fTime;
	kOffset.z = kV1.z * fTime;
	kMTri.m_kOrigin.x = kOffset.x + kMTri.m_kOrigin.x;
	kMTri.m_kOrigin.y = kMTri.m_kOrigin.y + kOffset.y;
	kMTri.m_kOrigin.z = kMTri.m_kOrigin.z + kOffset.z;
	float fPseudodistance =
		NiSqrDistance::Compute(kMCenter, kMTri, const_cast<float&>(m_fParameter0), const_cast<float&>(m_fParameter1)) *
			m_fInvRadiusSqr -
		1.0;
	return fPseudodistance;
}

// The contact point is the triangle's point at the closest parameters, moved along kV1 by the contact time when the
// sphere hits; the contact normal is the triangle's normal reversed.
void NiSphereTriIntersector::ComputeContactInformation(const NiPoint3& kV0, const NiPoint3& kV1) {
	float fParameter1 = m_fParameter1;
	float fEdge1X = m_kTri.m_kEdge1.x * fParameter1;
	float fEdge1Y = m_kTri.m_kEdge1.y * fParameter1;
	float fEdge1Z = fParameter1 * m_kTri.m_kEdge1.z;
	float fParameter0 = m_fParameter0;
	float fEdge0X = m_kTri.m_kEdge0.x * fParameter0;
	float fEdge0Y = m_kTri.m_kEdge0.y * fParameter0;
	float fEdge0Z = fParameter0 * m_kTri.m_kEdge0.z;
	NiPoint3 kPoint;
	kPoint.x = m_kTri.m_kOrigin.x + fEdge0X;
	kPoint.y = m_kTri.m_kOrigin.y + fEdge0Y;
	kPoint.z = m_kTri.m_kOrigin.z + fEdge0Z;
	m_kContactPoint.x = kPoint.x + fEdge1X;
	m_kContactPoint.y = kPoint.y + fEdge1Y;
	m_kContactPoint.z = kPoint.z + fEdge1Z;
	NiPoint3* pkNormal = m_kTri.GetNormal(&kPoint);
	bool bContact = m_eIntersectionType == IT_CONTACT;
	m_kContactNormal.x = -pkNormal->x;
	m_kContactNormal.y = -pkNormal->y;
	m_kContactNormal.z = -pkNormal->z;
	if (bContact) {
		double dContactTime = m_fContactTime;
		float fMoveX = kV1.x * dContactTime;
		float fMoveY = kV1.y * dContactTime;
		float fMoveZ = dContactTime * kV1.z;
		m_kContactPoint.x = m_kContactPoint.x + fMoveX;
		m_kContactPoint.y = m_kContactPoint.y + fMoveY;
		m_kContactPoint.z = fMoveZ + m_kContactPoint.z;
	}
}

void NiSphereTriIntersector::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
