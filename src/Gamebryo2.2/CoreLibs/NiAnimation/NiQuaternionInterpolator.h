#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

// Interpolates a rotation from an NiRotData key array.
class NiQuaternionInterpolator : public NiKeyBasedInterpolator {
public:
	NiQuaternionInterpolator();
	~NiQuaternionInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue) override;
	bool IsQuaternionValueSupported() const override { return true; }
	void Collapse() override;
	void GetActiveTimeRange(float& afBeginKeyTime, float& afEndKeyTime) const override;
	void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
	NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
	uint16_t GetKeyChannelCount() const override { return 1; }
	uint32_t GetKeyCount(uint16_t usChannel) const override;
	NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
	NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
	NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
	uint8_t GetKeyStride(uint16_t usChannel) const override;
	bool GetChannelPosed(uint16_t usChannel) const override;
	using NiInterpolator::Update;

	void CopyMembers(NiQuaternionInterpolator* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiQuaternion m_kQuaternionValue;
	NiPointer<NiRotData> m_spQuaternionData;
	uint32_t m_uiLastIdx;

	static NiRTTI ms_RTTI;
};
