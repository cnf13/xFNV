#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// The extra data of an addon node that holds the sound it plays.
class BGSAddonNodeSoundHandleExtra : public NiExtraData {
public:
	BGSAddonNodeSoundHandleExtra();
	~BGSAddonNodeSoundHandleExtra() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// Creates the "ASH" tag.
	static void InitTag();
	// The "ASH" tag the extra data is named by.
	static const NiFixedString& Tag();
	static NiFixedString* KillSDM();

	BSSoundHandle SoundHandle;

	static NiRTTI ms_RTTI;
	static inline NiFixedString* pTag;
};
inline constexpr fnvd::Vtable BGSAddonNodeSoundHandleExtra_vftable{};
