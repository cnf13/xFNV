#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

// Blends a transform between the start and end transforms of a list of named links by a lerp value it samples from an
// NiFloatData.
class BSTreadTransfInterpolator : public NiKeyBasedInterpolator {
public:
	// One named link and the transforms it blends between.
	struct LinkInfo {
		LinkInfo();

		NiQuatTransform Start;
		NiQuatTransform End;
		NiFixedString Name;
	};

	BSTreadTransfInterpolator();
	~BSTreadTransfInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	bool Update(float afTime, NiObjectNET* apInterpTarget, NiQuatTransform& arValue) override;
	bool IsTransformValueSupported() const override { return true; }
	uint16_t GetKeyChannelCount() const override { return 1; }
	uint32_t GetKeyCount(uint16_t usChannel) const override;
	NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
	NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
	NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
	uint8_t GetKeyStride(uint16_t usChannel) const override;
	bool GetChannelPosed(uint16_t usChannel) const override;
	using NiInterpolator::Update;

protected:
	// Frees the destination's links, then copies the links into a fresh new[] array, the lerp data and the current and
	// difference lerps; the last lerp key index is not copied.
	void CopyMembers(BSTreadTransfInterpolator* pkDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	LinkInfo* Links;
	uint32_t uiNumLinks;
	float fCurrLerp;
	float fDiffLerp;
	uint32_t uiLastLerpIdx;
	NiPointer<NiFloatData> spLerpData;

	static NiRTTI ms_RTTI;
};
