#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The rotation keys an interpolator samples: one key array plus its interpolation type and key size.
class NiRotData : public NiObject {
public:
	NiRotData();
	~NiRotData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	// Adds keys at afStartTime and afEndTime when the data lacks them. Gamebryo's source passes the position key
	// content type, as NiPosData's does.
	void GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime);
	// A new data object holding only the keys between afStartTime and afEndTime.
	void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiRotData>& aspRotData);

	// Frees the current keys and takes over pkKeys.
	void ReplaceAnim(NiRotKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

	static NiObject* CreateObject();

	uint32_t m_uiNumKeys;
	NiRotKey* m_pkKeys;
	NiRotKey::KeyType m_eType;
	uint8_t m_ucKeySize;

	static NiRTTI ms_RTTI;
};
