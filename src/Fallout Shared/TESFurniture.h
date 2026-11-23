#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESObjectACTI.h"

// A furniture base form (FURN).
class TESFurniture : public TESObjectACTI {
public:
    TESFurniture();
    ~TESFurniture() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjectToGet,
        int32_t aicount
    ) override;

    using TESObjectACTI::Save;

    // Whether the furniture is a bed: the top bit of its marker flags.
    bool CanSleepOn();
    bool CanSitOn();
    void VerifyMarkers();
    // The offset of marker number aiIndex from the reference, scaled by afScale.
    NiPoint3 GetMarkerTargetOffset(uint32_t aiIndex, float afScale);
    float GetMarkerHeadingOffset(uint32_t aiIndex);
    // Whether furniture marker aiMarkerIndex is enabled.
    bool GetMarkerEnabled(uint32_t aiMarkerIndex);

    static bool IsSleepMarker(uint32_t aiNumber);
    static bool IsSitMarker(uint32_t aiNumber);

    enum FURNITURE_FLAGS : uint32_t {
        FLAG_CAN_SIT_ON = 0x40000000,
        FLAG_CAN_SLEEP_ON = 0x80000000,
    };

    // The marker flags, unsigned.
    uint32_t iFurnFlags;
};

using FurnitureMarkerDeltaSettings = SettingT<GameSettingCollection>* [40][4];
extern FurnitureMarkerDeltaSettings pFurnitureMarkerDelta;
