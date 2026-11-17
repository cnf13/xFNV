#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Reconstruction.hpp"

// A lighting template form (LGTM): interior lighting a cell can inherit.
class BGSLightingTemplate : public TESForm {
public:
	BGSLightingTemplate();
	~BGSLightingTemplate() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// The getters of the template's INTERIOR_DATA. Each is a one-field getter the exe shares with other classes'
	// getters of the same shape (thiscall, ret 0).
	uint32_t GetDataAmbient() { return Data.iAmbient; }
	uint32_t GetDataDirectional() { return Data.iDirectional; }
	uint32_t GetDataDirectionalXY() { return Data.iDirectionalXY; }
	uint32_t GetDataDirectionalZ() { return Data.iDirectionalZ; }
	uint32_t GetDataFog() { return Data.iFog; }
	float GetDataFogNearPlane() { return Data.fFogNear; }
	// The fog's far plane distance.
	float GetDataFogFarPlane();
	float GetDataClipDist();
	float GetDataFogPower();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	INTERIOR_DATA Data;
};
inline constexpr fnvd::Vtable BGSLightingTemplate_vftable{};
