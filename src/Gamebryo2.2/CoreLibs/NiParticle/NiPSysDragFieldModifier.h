#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// slows particles near the field object.
class NiPSysDragFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysDragFieldModifier();
	~NiPSysDragFieldModifier() override;

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
	// Update's paths for a field without attenuation or direction, with and without the maximum distance.
	void UpdateNoAttenuationNoDir(float fTime, NiPSysData* pkData);
	void UpdateNoAttenuationNoDirNoDist(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	bool m_bUseDirection;
	NiPoint3 m_kDirection;

	static NiRTTI ms_RTTI;
};
