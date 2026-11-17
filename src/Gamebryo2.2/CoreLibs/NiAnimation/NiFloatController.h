#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class NiFloatData;

// A keyframed float controller driven by float data.
class NiFloatController : public NiTimeController {
public:
	~NiFloatController() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;

	// The clone gets its own clone of the float data.
	void CopyMembers(NiFloatController* pDest, NiCloningProcess& kCloning);
	// The float data's keys, with their count, type and size; no keys (and zeros) without data.
	NiFloatKey* GetAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;

	uint32_t m_uiLastIdx;
	NiPointer<NiFloatData> m_spFloatData;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiFloatController_vftable{};
