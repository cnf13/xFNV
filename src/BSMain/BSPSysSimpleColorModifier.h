#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// fades particles in and out through three colors.
class BSPSysSimpleColorModifier : public NiPSysModifier {
public:
	BSPSysSimpleColorModifier();
	BSPSysSimpleColorModifier(char* pcString, NiColorA* a3, NiColorA* a4, NiColorA* a5, float a6, float a7, float a8,
		float a9, float a10, float a11);
	~BSPSysSimpleColorModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;

	// NiPSysModifier's part, then the colors and fade points, into apDest; not virtual.
	void CopyMembers(BSPSysSimpleColorModifier* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiColorA kColor1;
	NiColorA kColor2;
	NiColorA kColor3;
	float fFadeInPct;
	float fFadeOutPct;
	float fColor1EndPct;
	float fColor2StartPct;
	float fColor2EndPct;
	float fColor3StartPct;

	static NiRTTI ms_RTTI;
};
