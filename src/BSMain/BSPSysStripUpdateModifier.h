#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// advances the strips of a strip particle system.
class BSPSysStripUpdateModifier : public NiPSysModifier {
public:
	BSPSysStripUpdateModifier();
	BSPSysStripUpdateModifier(const char* apcName, float afDelta);
	~BSPSysStripUpdateModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float afTime, NiPSysData* apData) override;
	void Initialize(NiPSysData* apData, uint16_t ausNewParticle) override;
	bool IsBSPSysStripUpdateModifier() override { return true; }

protected:
	void CopyMembers(BSPSysStripUpdateModifier* pkDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	float fUpdateDeltaTime;

	static NiRTTI ms_RTTI;
};
