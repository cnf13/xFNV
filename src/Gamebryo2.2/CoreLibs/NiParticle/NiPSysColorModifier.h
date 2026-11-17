#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiColorData;
class NiPSysData;
class NiStream;

// colors particles over their life from color keys.
class NiPSysColorModifier : public NiPSysModifier {
public:
	NiPSysColorModifier();
	~NiPSysColorModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;

	// Takes the color keys and resets the key time range to their first and last key times.
	void SetColorData(NiColorData* pkColorData);
	// Sets the key time range to the first and last key times of the color data (0 to 0 without keys).
	void ResetKeyTimeExtrema();

	static NiObject* CreateObject();

	NiPointer<NiColorData> m_spColorData;
	float m_fLoKeyTime;
	float m_fHiKeyTime;

	static NiRTTI ms_RTTI;
};
