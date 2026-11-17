#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

NiSmartPointer(NiColorData);

// The color keys an interpolator samples: one key array plus its interpolation type and key size.
class NiColorData : public NiObject {
public:
	// Calls the engine's constructor in existing storage.
	NiColorData();
	~NiColorData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& stream) override;
	// LinkObject and RegisterStreamables fold into jumps to the NiObject bodies
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Adds keys at afStartTime and afEndTime when the data lacks them.
	void GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime);
	// A new data object holding only the keys between afStartTime and afEndTime.
	void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiColorData>& aspColorData);

	// Frees the current keys and takes over pkKeys.
	void ReplaceAnim(NiColorKey* pkKeys, uint32_t uiNumKeys, NiColorKey::KeyType eType);

	// ~NiColorData: frees the keys through NiAnimationKey's delete
	// function for the key type, then ~NiObject.
	static NiObject* CreateObject();

	uint32_t m_uiNumKeys;
	NiColorKey* m_pkKeys;
	NiColorKey::KeyType m_eType;
	uint8_t m_ucKeySize;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiColorData_vftable{};
