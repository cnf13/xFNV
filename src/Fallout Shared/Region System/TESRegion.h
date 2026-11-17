#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class TESRegion;
class TESRegionDataList;
class TESRegionPointList;
class TESWeather;
class TESWorldSpace;

// A map region (REGN): an area of a worldspace with its weather, sounds and objects.
class TESRegion : public TESForm {
public:
	TESRegion();
	~TESRegion() override;

	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	NiColor* GetEmittanceColor() override { return &EmittanceColor; }

	// copies the region into apCopyInto, or into a new region when it is null; returns the copy
	virtual TESRegion* Copy(TESRegion* apCopyInto);
	virtual bool Validate();
	using TESForm::Copy;
	using TESForm::Save;

	// The region's current weather (pCurrentWeather); the exe calls the shared +0x24 getter it
	// folds with Menu::GetFadeState (thiscall, ret 0, 0x59BB30).
	TESWeather* GetCurrentWeather();
	// Sets pCurrentWeather through the exe's shared +0x24 one-field setter, folded with
	// Menu::SetFadeState (thiscall, ret 4, 0x7037C0).
	void SetCurrentWeather(TESWeather* apWeather);
	// fLastWeatherUpdate, through the exe's shared one-float getter (thiscall, ST0).
	float GetLastWeatherUpdate() { return fLastWeatherUpdate; }
	void SetWorldSpace(TESWorldSpace* apWorldSpace);
	void UpdateWeather();

	// IDA name; not in the PDB. Shows aRegionData's text in the HUD's region text and holds it
	// (cdecl, 0x7728F0). Its argument has the shape of the PDB's HUDMainMenu::SetRegionText(const HUDRegionData&);
	// Interface::SetRegionText passes the address of its HUDRegionData local.
	static void UpdateHUDRegionIndicator(const HUDMainMenu::HUDRegionData& aRegionData);
	static void SaveGame();

	TESRegionDataList* pDataList;
	BSSimpleList<TESRegionPointList*>* pPointLists;
	TESWorldSpace* pWorldSpace;
	TESWeather* pCurrentWeather;
	float fLastWeatherUpdate;
	NiColor EmittanceColor;
};
inline constexpr fnvd::Vtable TESRegion_vftable{};
