#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiParticleSystem;
class NiPSysData;

// One step of a particle system's update (forces, colors, spawning...); a system runs its modifiers by order.
class NiPSysModifier : public NiObject {
public:
	NiPSysModifier();
	NiPSysModifier(const NiFixedString& kName, uint32_t uiOrder);
	~NiPSysModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	virtual void Update(float afTime, NiPSysData* apData) = 0;
	virtual void Initialize(NiPSysData*, uint16_t) {}
	virtual void HandleReset() {}
	virtual void SetActive(bool bActive) { m_bActive = bActive; }
	virtual void SetSystemPointer(NiParticleSystem* pkTarget) { m_pkTarget = pkTarget; }
	virtual bool IsBSPSysStripUpdateModifier() { return false; }

	void CopyMembers(NiPSysModifier* pkDest, NiCloningProcess& kCloning);

	NiFixedString m_kName;
	uint32_t m_uiOrder;
	NiParticleSystem* m_pkTarget;
	bool m_bActive;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiPSysModifier);
