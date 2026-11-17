#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"

// An interpolator that samples key arrays: one array per channel, described by the key-access virtuals.
class NiKeyBasedInterpolator : public NiInterpolator {
public:
	NiKeyBasedInterpolator();

	~NiKeyBasedInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkOther) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const override;
	virtual uint16_t GetKeyChannelCount() const = 0;
	virtual uint32_t GetKeyCount(uint16_t ausChannel) const = 0;
	virtual NiAnimationKey::KeyType GetKeyType(uint16_t ausChannel) const = 0;
	virtual NiAnimationKey::KeyContent GetKeyContent(uint16_t ausChannel) const = 0;
	virtual NiAnimationKey* GetKeyArray(uint16_t ausChannel) const = 0;
	virtual uint8_t GetKeyStride(uint16_t ausChannel) const = 0;
	virtual bool GetChannelPosed(uint16_t ausChannel) const = 0;

	// Key auiIndex of channel ausChannel.
	NiAnimationKey* GetKeyAt(uint32_t uiWhichKey, uint16_t usChannel) const;

	void CopyMembers(NiKeyBasedInterpolator* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};
