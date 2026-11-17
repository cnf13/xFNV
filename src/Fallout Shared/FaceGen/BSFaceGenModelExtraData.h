#pragma once

#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// Hangs a face's BSFaceGenModel on the NiNode of its mesh. The PDB's source module is BSFaceGenModel.cpp.
class BSFaceGenModelExtraData : public NiExtraData {
public:
	BSFaceGenModelExtraData(BSFaceGenModel* apModel);
	~BSFaceGenModelExtraData() override;

	const NiRTTI* GetRTTI() const override;
	virtual BSFaceGenModel* GetModel();

	// Creates the "FMD" tag.
	static void InitTag();
	// The "FMD" tag InitTag creates.
	static const NiFixedString& Tag();
	static void KillSDM();

	NiPointer<BSFaceGenModel> spModel;

	static NiRTTI ms_RTTI;
	// The tag string, created by InitTag.
	static inline NiFixedString* pTag;
};
inline constexpr fnvd::Vtable BSFaceGenModelExtraData_vftable{};
