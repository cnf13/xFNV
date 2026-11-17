#pragma once

#include "Fallout Shared/BSSplatterData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// The splatter an object carries for the screen splatter.
class BSSplatterExtraData : public NiExtraData {
public:
	BSSplatterExtraData(const BSSplatterData& aSplatterData);
	~BSSplatterExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	static NiFixedString* GetTag();
	// Creates the "SPD" tag string stored in pTag.
	static void InitTag();
	static void KillSDM();

	BSSplatterData SplatterData;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
