#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

class NiStream;

// the birth rate and active keys of a NiPSysEmitterCtlr.
class NiPSysEmitterCtlrData : public NiObject {
public:
	NiPSysEmitterCtlrData();
	~NiPSysEmitterCtlrData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	// Frees the current birth rate keys and takes over pkKeys; with no keys the birth rate keys are left empty.
	void ReplaceBirthRateKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
	// Frees the current active keys and takes over pkKeys; with no keys the active keys are left empty.
	void ReplaceEmitterActiveKeys(NiBoolKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

	static NiObject* CreateObject();

	uint32_t m_uiNumBirthRateKeys;
	NiFloatKey* m_pkBirthRateKeys;
	NiFloatKey::KeyType m_eBirthRateKeyType;
	uint8_t m_ucBirthRateSize;
	uint32_t m_uiNumEmitterActiveKeys;
	NiBoolKey* m_pkEmitterActiveKeys;
	NiBoolKey::KeyType m_eEmitterActiveKeyType;
	uint8_t m_ucEmitterActiveSize;

	static NiRTTI ms_RTTI;
};
