#pragma once

#include "Fallout Shared/BSSplatterData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// The splatter an object carries for the custom screen splatter. The PDB's source module is ScreenCustomSplatter.cpp.
class BSCustomSplatterExtraData : public NiExtraData {
public:
	BSCustomSplatterExtraData(BSSplatterData* aSplatterData);

	~BSCustomSplatterExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// Creates the "CSD" tag string.
	static void InitTag();
	// The address of the "CSD" tag string (cdecl, 0x4DE890; IDA calls it GetTag).
	static const NiFixedString& Tag();

	BSSplatterData SplatterData;

	static NiRTTI ms_RTTI;
	static inline NiFixedString* pTag;
};

inline constexpr fnvd::Vtable BSCustomSplatterExtraData_vftable{};
