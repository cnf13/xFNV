#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// shakes particles randomly.
class NiPSysTurbulenceFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysTurbulenceFieldModifier();
	~NiPSysTurbulenceFieldModifier() override;

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
	// Update's paths for a field without attenuation, without and with the maximum distance. The names are spelled
	// Upate on purpose.
	void UpateNoAttenuationNoDistance(float fTime, NiPSysData* pkData);
	void UpateNoAttenuation(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	float m_fFrequency;
	float m_fFrequencyUpdateTime;
	float m_fLastUpdateTime;

	static NiRTTI ms_RTTI;
};
