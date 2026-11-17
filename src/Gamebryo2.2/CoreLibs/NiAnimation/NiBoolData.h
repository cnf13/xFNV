#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// The bool keys an interpolator samples: one key array plus its interpolation type and key size.
class NiBoolData : public NiObject {
public:
	NiBoolData();
	~NiBoolData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the NiObject bodies
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Adds keys at afStartTime and afEndTime when the data lacks them.
	void GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime);
	// A new data object holding only the keys between afStartTime and afEndTime.
	void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiBoolData>& aspBoolData);

	// Frees the current keys and takes over pkKeys.
	void ReplaceAnim(NiBoolKey* pkKeys, uint32_t uiNumKeys, NiBoolKey::KeyType eType);

	static NiObject* CreateObject();

	uint32_t m_uiNumKeys;
	NiBoolKey* m_pkKeys;
	NiBoolKey::KeyType m_eType;
	uint8_t m_ucKeySize;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBoolData_vftable{};
