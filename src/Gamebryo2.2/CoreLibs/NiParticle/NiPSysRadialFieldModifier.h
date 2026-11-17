#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// pushes particles away from the field object.
class NiPSysRadialFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysRadialFieldModifier();
	~NiPSysRadialFieldModifier() override;

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
	// Update's path for a field without maximum distance whose attenuation or radial type is zero.
	void UpdateNoAttenuationNoDirNoDist(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	float m_fRadialType;

	static NiRTTI ms_RTTI;
};
