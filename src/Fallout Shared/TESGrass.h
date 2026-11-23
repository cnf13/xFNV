#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"

// A grass form (GRAS). Its own members are not declared yet.
class TESGrass : public TESBoundObject, public TESModel {
public:
    // Where grass may grow relative to the water level, and whether the distance is a minimum or a maximum.
    enum GRASS_WATER_STATE : int32_t {
        GWS_ABOVE_ONLY_ATLEAST = 0x0,
        GWS_ABOVE_ONLY_ATMOST = 0x1,
        GWS_BELOW_ONLY_ATLEAST = 0x2,
        GWS_BELOW_ONLY_ATMOST = 0x3,
        GWS_BOTH_ATLEAST = 0x4,
        GWS_BOTH_ATMOST = 0x5,
        GWS_BOTH_ATMOST_ABOVE = 0x6,
        GWS_BOTH_ATMOST_BELOW = 0x7,
        GWS_COUNT = 0x8,
    };

    // The GNAM record: how the grass grows.
    struct GRASS_DATA {
        enum : int8_t {
            VERTEX_LIGHTING = 0x1,
            UNIFORM_SCALING = 0x2,
            FIT_TO_SLOPE = 0x4,
        };

        // Swaps the fields read from a big-endian file.
        void Endian();

        int8_t cDensity;
        int8_t cMinSlopeDegrees;
        int8_t cMaxSlopeDegrees;
        int16_t sDistanceFromWaterLevel;
        GRASS_WATER_STATE eUnderwater;
        float fPositionRange;
        float fHeightRange;
        float fColorRange;
        float fWavePeriod;
        int8_t cFlags;
    };

    TESGrass();
    ~TESGrass() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    virtual uint8_t GetDensity();
    // false (and unchanged) above 100
    virtual bool SetDensity(uint8_t acDensity);
    virtual uint8_t GetMinSlopeDegrees();
    // false (and unchanged) above 90 or above the maximum slope
    virtual bool SetMinSlopeDegrees(uint8_t acMinSlopeDegrees);
    virtual uint8_t GetMaxSlopeDegrees();
    // false (and unchanged) above 90 or below the minimum slope
    virtual bool SetMaxSlopeDegrees(uint8_t acMaxSlopeDegrees);
    virtual float GetMinSlope();
    virtual float GetMaxSlope();
    virtual uint16_t GetDistanceFromWaterLevel();
    virtual void SetDistanceFromWaterLevel(uint16_t asDistanceFromWaterLevel);
    // the underwater placement
    virtual GRASS_WATER_STATE GetUnderwaterState();
    virtual void SetUnderwaterState(const GRASS_WATER_STATE aeUnderwater) {
        Data.eUnderwater = aeUnderwater;
    }
    virtual float GetPositionRange();
    virtual bool SetPositionRange(float afRange);
    virtual float GetHeightRange();
    virtual bool SetHeightRange(float afRange);
    virtual float GetColorRange();
    virtual bool SetColorRange(float afRange);
    // The wave period.
    virtual float GetWavePeriod();
    virtual bool SetWavePeriod(float afPeriod);
    virtual bool GetVertexLighting();
    virtual void SetVertexLighting(bool abFlag);
    virtual bool GetUniformScaling();
    virtual void SetUniformScaling(bool abFlag);
    virtual bool GetFitToSlope();
    virtual void SetFitToSlope(bool abFlag);
    using TESForm::Save;

    char* GetModelPath();

    static float GetWaveOffsetRange();

    GRASS_DATA Data;

    // fWaveOffsetRange:Grass: the range the wave offset of each grass instance is drawn from.
    static SettingT<INISettingCollection> fWaveOffsetRange;
};
