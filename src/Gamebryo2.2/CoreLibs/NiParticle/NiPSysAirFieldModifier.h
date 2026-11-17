#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// pushes particles along like moving air.
class NiPSysAirFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysAirFieldModifier();
	~NiPSysAirFieldModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;

	// Sets the direction and its unit vector; a zero direction gets the unit x axis as unit vector.
	void SetDirection(const NiPoint3& kDirection);

protected:
	// Update's paths for a field without attenuation, inherited velocity or spread, with and without the
	// maximum distance.
	void UpdateNoAttenuationNoInheritNoSpread(float fTime, NiPSysData* pkData);
	void UpdateNoAttenuationNoInheritNoSpreadNoMaxDistance(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	NiPoint3 m_kLastFieldPosition;
	float m_fLastUpdateTime;
	NiPoint3 m_kDirection;
	NiPoint3 m_kUnitDirection;
	float m_fAirFriction;
	float m_fInheritVelocity;
	bool m_bInheritRotation;
	bool m_bComponentOnly;
	bool m_bEnableSpread;
	float m_fSpread;

	static NiRTTI ms_RTTI;
	// The last field position before the first update (every coordinate FLT_MAX).
	static inline NiPoint3 m_ckUninitializePosition{3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f};
};
inline constexpr fnvd::Vtable NiPSysAirFieldModifier_vftable{};
