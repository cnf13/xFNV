#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// The position keys an interpolator samples: one key array plus its interpolation type and key size.
class NiPosData : public NiObject {
public:
	NiPosData();
	~NiPosData() override;

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
	void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiPosData>& aspPosData);

	// Frees the current keys and takes over pkKeys.
	void ReplaceAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
	// Takes pkKeys over without freeing the old ones: no keys
	// or no count leave the data empty (NOINTERP); else the key size is ms_keysizes for POSKEY and eType.
	void SetAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

	static NiObject* CreateObject();

	uint32_t m_uiNumKeys;
	NiPosKey* m_pkKeys;
	NiPosKey::KeyType m_eType;
	uint8_t m_ucKeySize;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPosData_vftable{};

inline NiPosData::NiPosData()
	: NiObject(), m_uiNumKeys(0), m_pkKeys(nullptr), m_eType(NiAnimationKey::NOINTERP), m_ucKeySize(0) {}
