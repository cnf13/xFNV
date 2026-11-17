#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// pulls particles in one direction.
class NiPSysGravityFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysGravityFieldModifier();
	~NiPSysGravityFieldModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;

protected:
	// Update's path for a field without attenuation or maximum distance.
	void UpdateNoAttenuationNoDist(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	NiPoint3 m_kDirection;
	NiPoint3 m_kUnitDirection;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysGravityFieldModifier_vftable{};
