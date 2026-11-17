#pragma once

#include "Fallout Shared/BSSplatterData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// The splatter an object carries for the screen splatter. The PDB's source module is ScreenSplatter.cpp.
class BSSplatterExtraData : public NiExtraData {
public:
	BSSplatterExtraData(const BSSplatterData& aSplatterData);
	~BSSplatterExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// Inferred type: return: NiFixedString* from 0x4e0328 (BSSplatterExtraData::pTag)
	static NiFixedString* GetTag();
	// Creates the "SPD" tag string stored in pTag.
	static void InitTag();
	static void KillSDM();

	BSSplatterData SplatterData;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSSplatterExtraData_vftable{};
