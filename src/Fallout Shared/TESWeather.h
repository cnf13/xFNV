#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture1024.h"
#include "SDK/MSVC/ehvec.h"

template <class T> void ENDIAN(T& arValue, bool abUnused);

// A weather form (WTHR).
class TESWeather : public TESForm, public TESImageSpaceModifiableCountForm<6> {
public:
	// The weather's color sets, one per sky part and light.
	enum ColorType : uint32_t {
		SKY_UPPER = 0x0,
		FOG = 0x1,
		CLOUD_LAYER = 0x2,
		AMBIENT = 0x3,
		SUNLIGHT = 0x4,
		SUN = 0x5,
		STARS = 0x6,
		SKY_LOWER = 0x7,
		HORIZON = 0x8,
		CLOUDS_UNUSED = 0x9,
		NUM_TYPES = 0xA,
	};

	// The times of day a weather has colors for.
	enum ColorTime : uint32_t {
		SUNRISE = 0x0,
		DAY = 0x1,
		SUNSET = 0x2,
		NIGHT = 0x3,
		NOON = 0x4,
		MIDNIGHT = 0x5,
		NUM_TIMES = 0x6,
	};

	// The kinds of sound a weather plays.
	enum SoundType : uint32_t {
		WST_DEFAULT = 0x0,
		WST_PRECIP = 0x1,
		WST_WIND = 0x2,
		WST_THUNDER = 0x3,
		WST_COUNT = 0x4,
	};

	// The bytes of ucWeatherData, which is a 15-byte array.
	enum WeatherData : int32_t {
		WD_WIND_SPEED = 0,
		WD_CLOUD_SPEED = 1,
		WD_CLOUD_SPEED_UNUSED = 2,
		WD_TRANS_DELTA = 3,
		WD_SUN_GLARE = 4,
		WD_SUN_DAMAGE = 5,
		WD_DIVIDER1 = 6,
		WD_BEGIN_PRECIP = 6,
		WD_END_PRECIP = 7,
		WD_BEGIN_THUNDER = 8,
		WD_END_THUNDER = 9,
		WD_THUNDER_FREQ = 10,
		WD_DIVIDER2 = 11,
		WD_FLAGS = 11,
		WD_LIGHTNING_R = 12,
		WD_LIGHTNING_G = 13,
		WD_LIGHTNING_B = 14,
		WD_DIVIDER3 = 15,
		WD_COUNT = 15,
	};

	// The floats of fFogData, a 6-float array.
	enum FogData : int32_t {
		NEAR_DIST_DAY = 0,
		FAR_DIST_DAY = 1,
		NEAR_DIST_NIGHT = 2,
		FAR_DIST_NIGHT = 3,
		FOG_POWER_DAY = 4,
		FOG_POWER_NIGHT = 5,
		FD_COUNT = 6,
	};

	// A sound the weather plays: the sound form's ID and what it is for.
	struct WeatherSound {
		WeatherSound();
		WeatherSound(uint32_t auiSoundFormID, SoundType aeType);

		// Byte-swaps both fields.
		void Endian();

		uint32_t uiSoundFormID;
		SoundType eType;
	};

	// The sounds a weather plays.
	class WeatherSoundList : public BSSimpleList<WeatherSound*> {
	public:
		// Frees every sound and empties the list.
		void FlushList();
	};

	TESWeather();
	~TESWeather() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Resets the weather to the engine's defaults.
	void MakeDefault();

	// One weather data value scaled into [fMin, fMax].
	float GetWeatherData(WeatherData aeType, float fMax, float fMin);
	// The packed color of cloud layer aiLayer at time aeColorTime. aiLayer is signed: it is compared with
	// iNumCloudLayers as a signed value.
	uint32_t GetCloudColor(int32_t aiLayer, TESWeather::ColorTime aeColorTime);
	bool IsPleasant() const;
	bool IsCloudy() const;
	float GetCloudLayerSpeed(int32_t aiLayer, float fMax, float fMin) const;
	const TESWeather::WeatherSoundList* GetSoundList() const;
	const TESTexture* GetCloudTexture(int32_t aiLayer) const;
	bool IsSnow() const;
	// Returns the fog distance selected by aType.
	float GetFogData(FogData aType) const;
	// One weather data byte unscaled.
	uint8_t GetWeatherDataRaw(WeatherData aeType) const;
	int32_t IsRainy();
	// One packed sky color of a weather at a time of day.
	uint32_t GetColor(TESWeather::ColorType aeColorType, TESWeather::ColorTime aeColorTime) const;

	TESTexture1024 cloudTexture[4];
	uint8_t ucCloudLayerSpeed[4];
	uint32_t uiCloudColorData[4][6];
	TESModel precipNIF;
	uint8_t ucWeatherData[15];
	uint8_t padding0EF[1];
	float fFogData[6];
	uint32_t uiColorData[10][6];
	WeatherSoundList soundList;
	float fHDRData[14];
	ImageSpaceParameterData pImageSpaceParameterMods[2];
	int32_t iNumCloudLayers;
};
