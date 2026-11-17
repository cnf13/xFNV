#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// bounces particles off a sphere.
class NiPSysSphericalCollider : public NiPSysCollider {
public:
	NiPSysSphericalCollider(float fBounce, bool bSpawnOnCollide, bool bDieOnCollide,
		NiPSysSpawnModifier* pkSpawnModifier, NiAVObject* pkColliderObj, float fRadius);
	~NiPSysSphericalCollider() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	NiPSysCollider* Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData,
		int32_t usCurParticle) override;
	void Update(float fTime, NiPSysData* pkData, int32_t usCurParticle) override;
	void SetResolveConstants() override;

	static NiObject* CreateObject();

	NiAVObject* m_pkColliderObj;
	float m_fRadius;
	float m_fScaledRadius;
	float m_fRadiusSq;
	NiPoint3 m_kPosition;
	NiTransform m_kLastColliderTransform;
	NiTransform m_kLastPSysTransform;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysSphericalCollider_vftable{};
