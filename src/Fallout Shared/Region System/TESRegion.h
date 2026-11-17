#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

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
    NiColor* GetEmittanceColor() override {
        return &EmittanceColor;
    }

    // copies the region into apCopyInto, or into a new region when it is null; returns the copy
    virtual TESRegion* Copy(TESRegion* apCopyInto);
    virtual bool Validate();
    using TESForm::Copy;
    using TESForm::Save;

    // Returns the region's current weather (pCurrentWeather).
    TESWeather* GetCurrentWeather();
    // Sets pCurrentWeather.
    void SetCurrentWeather(TESWeather* apWeather);
    float GetLastWeatherUpdate() {
        return fLastWeatherUpdate;
    }
    void SetWorldSpace(TESWorldSpace* apWorldSpace);
    void UpdateWeather();

    // Shows aRegionData's text in the HUD's region text and holds it. Interface::SetRegionText passes the address of
    // its HUDRegionData local.
    static void UpdateHUDRegionIndicator(const HUDMainMenu::HUDRegionData& aRegionData);
    static void SaveGame();

    TESRegionDataList* pDataList;
    BSSimpleList<TESRegionPointList*>* pPointLists;
    TESWorldSpace* pWorldSpace;
    TESWeather* pCurrentWeather;
    float fLastWeatherUpdate;
    NiColor EmittanceColor;
};
