#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWeatherList.h"

// A climate form (CLMT).
class TESClimate : public TESForm {
public:
    // The sunrise and sunset transition times, the first entries of ucData.
    enum TransTime : int32_t {
        SUNRISE_BEGIN = 0,
        SUNRISE_END = 1,
        SUNSET_BEGIN = 2,
        SUNSET_END = 3,
        NUM_TRANS_TIMES = 4,
    };

    // The other entries of ucData.
    enum MiscData : int32_t {
        VOLATILITY = 4,
        MOON_DATA = 5,
        NUM_SLIDERS = 6,
    };

    // The sky object textures of txSkyObjects.
    enum TextureType : int32_t {
        TX_SUN = 0,
        TX_GLARE = 1,
        NUM_TEXTURES = 2,
    };

    TESClimate();
    ~TESClimate() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    void SetMoonPhaseDays(uint32_t auiDays);
    // The time (in 10-minute steps) the transition aeTT begins or ends, or 0 for a bad index.
    uint8_t GetTransTime(TransTime aeTT);
    // The sky object texture aeTxType, or null for a bad index.
    const TESTexture* GetTexture(TextureType aeTxType);
    // Resets the climate to the engine's defaults.
    void MakeDefault();
    uint8_t GetVolatilityIndex() const;
    void SetUsesMasser(bool abUses);
    void SetUsesSecunda(bool abUses);
    uint32_t GetMoonPhaseDays() const;
    bool UsesSecunda() const;
    TESWeather* GetRandomWeather();
    bool UsesMasser() const;
    const char* GetNightSkyModel();

    TESModel mNightSky;
    TESWeatherList weatherList;
    TESTexture txSkyObjects[2];
    uint8_t ucData[6];
    uint8_t padding56[2];
};
