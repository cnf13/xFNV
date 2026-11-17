#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESWeather.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class Atmosphere;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BSFogProperty;
class BSMultiBoundNode;
class Clouds;
class ImageSpaceModifierInstanceForm;
class Moon;
class NiDirectionalLight;
class NiFogProperty;
class NiNode;
class Precipitation;
class Stars;
class Sun;
class TESClimate;

// A weather's ambient sound, kept in the sky's sound list while the weather is current.
class SkySound {
public:
	// Copies the handle, stores the weather, sound type and form and clears uiData. The handle is taken by value.
	SkySound(BSSoundHandle aSndHandle, const TESWeather* apWeather, TESWeather::SoundType aeSoundType,
		uint32_t aiFormID);

	// Whether the sound's time check passes at auiTime.
	bool HandleCheckTime(int32_t aiInterval);
	// In auiFlags, 1 means free the memory.
	void* DeletingDestructor(uint32_t auiFlags);
	uint32_t GetNextCheck() const { return uiData & 0x7FFFFFFF; }
	bool IsFlaggedToBeKilled() const { return 0 < (uiData & 0x80000000); }
	void SetNextCheck(uint32_t uiWhen);
	// Marks the sound to be killed (the top bit of uiData).
	void FlagToKill();
	void Play(float afVol);

	BSSoundHandle SndHandle;
	const TESWeather* pWeather;
	TESWeather::SoundType eSoundType;
	uint32_t uiFormID;
	uint32_t uiData;

	// The thunder sounds playing; SetMode clears it when the sky leaves the exterior modes.
	static inline uint8_t ucThunderCount;
};

// The exterior sky: the sun, moons, clouds, stars and weather rendered above the world, blended between
// two weather types while the weather changes.
class Sky {
public:
	// Which parts of the sky the current cell renders.
	enum SKY_MODE : int32_t {
		SM_NONE = 0,
		SM_INTERIOR = 1,
		SM_FAKEEXTERIOR = 2,
		SM_FULL = 3,
		SM_COUNT = 4,
	};

	// Four colors the sky blends between two weather types, and their blend weights.
	struct COLOR_BLEND {
		uint32_t uiRGBVal[4];
		float fBlend[4];
	};

	using BlendColor_t = NiColorA[3];

	Sky();
	// The class has a vtable on PC; the root destructor provides it.
	virtual ~Sky();

	TESClimate* GetCurrentClimate() const;
	// spRoot's object.
	NiNode* GetRoot();
	// Whether it is raining.
	bool IsRaining();
	// Whether it is snowing: the current weather is snowy and far enough in, or the last one snowy and not yet out.
	bool IsSnowing();
	// Forces apWeather, overriding the scheduled one when abOverride.
	void ForceWeather(TESWeather* apWeather, bool abOverride);
	// Ends the override of ForceWeather.
	void ReleaseWeatherOverride();
	SKY_MODE GetMode();
	// Switches which parts of the sky render, loading or unloading the sky textures and sounds.
	void SetMode(SKY_MODE aeNewMode);
	// The per-frame sky update: the underwater flag, weather, colors, fog, wind, sounds, sky objects, moon phase
	// and lightning flash.
	void Update(float afSecondsPassed);
	// Blends the sky, light and cloud colors between the two weather types (fixed colors without a sky, the cell's
	// colors in an interior).
	void UpdateColors();
	// Blends the fog distances between the two weather types.
	void UpdateFog();
	// Blends the wind between the two weather types.
	void UpdateWind();
	// Picks the climate of the current cell and resets the weather on a change.
	void HandleClimateChange();
	// Advances the weather transition and picks the next weather.
	void UpdateWeather();
	// Starts and stops the weathers' ambient sounds.
	void UpdateSound();
	// Fills aColorBlend with one cloud layer's colors of the two weathers at the two times of day; a weather's
	// missing (black) color is taken from the other weather.
	void FillCloudColorBlendColors(Sky::COLOR_BLEND& aColorBlend, TESWeather* apCurrentWeather,
		TESWeather* apLastWeather, int32_t aiLayer, TESWeather::ColorTime& aeTime1, TESWeather::ColorTime& aeTime2);
	float GetWindSpeed();
	float GetWindAngle();
	float GetCellFogPower();
	float GetFogHeight() { return fFogHeight; }
	float GetWeatherPercent();
	float GetFlash();
	float GetCurrentGameHour();
	bool GetWeatherUpdateFlag();
	void InvalidateCachedValues();
	void UpdateMoonPhase(bool abUpdate);
	float GetSunriseBegin();
	float GetSunriseEnd();
	float GetSunsetBegin();
	float GetSunsetEnd();
	void SetAcceleration(bool abOn);
	void ResetWeather();
	// Makes apClimate (the default climate 0x15F when null and there is none, or when forced) the current climate
	// and resets the weather when it changes or abForce.
	void SetCurrentClimate(TESClimate* apClimate, bool abForce);
	float GetBeginSunriseColors();
	float GetEndSunsetColors();
	void StopPlayingAllWeatherSounds();
	// The size of the sky's save data; later save versions add one and then two more dwords.
	uint16_t GetSaveSize();
	// The old save format: the weather IDs (the override from version 0x5D), hour, timing and mode, then the flags
	// and acceleration from version 0x69.
	void SaveGame();
	// Saves the four weathers, then the hour, weather timing, flags, acceleration, water fog color, fog and mode.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// Unless the player's location is invalid, flushes the precipitation and cloud transitions, reads what
	// SaveGame writes (keeping only flag 8 of the saved flags), then updates the sky.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	void UpdateInterior(float afSecondsPassed);
	void UnloadAllTextures();
	void UpdateHDRValues();
	// The two times of day arrive by reference: the engine reads them through pointers.
	void FillColorBlendColors(Sky::COLOR_BLEND& aColorBlend, TESWeather* apCurrentWeather, TESWeather* apLastWeather,
		TESWeather::ColorType aeColorType, TESWeather::ColorTime& aeTime1, TESWeather::ColorTime& aeTime2);
	// Fills aColorBlend's time-of-day blend weights for the current game hour and returns the two times of day it
	// blends (TESWeather::ColorTime values).
	void FillColorBlend(COLOR_BLEND& aColorBlend, TESWeather* apCurrentWeather, float afCurrentWeatherPct,
		TESWeather::ColorTime& aeTime1, TESWeather::ColorTime& aeTime2);
	// Blends apColorBlend's colors into aColor, brightened by afAddFlash.
	void SetColor(NiColor& aColor, COLOR_BLEND* apColorBlend, float afAddFlash) const;
	// Reloads the sky objects' textures.
	void ReloadAllTextures();
	NiFogProperty* GetFog();
	NiDirectionalLight* GetLight();
	// Builds the sky's root, atmosphere, stars, sun, moons root, precipitation, clouds and alpha property under
	// apRoot (a new multibound node when null) with apFog.
	void Initialize(BSMultiBoundNode* apRoot, BSFogProperty* apFog);
	Sun* GetSun();
	// The sunlight (directional) color, SkyColor[4].
	NiColor* GetSunLightColor();
	bool IsUnderwater() const;
	void SetFastTravel(bool abOn);
	TESWeather* GetCurrentWeather();
	TESWeather* GetLastWeather();
	// Adds a sound to the list for each of apWeather's sounds the list lacks.
	void ActivateWeatherSounds(const TESWeather* apWeather);
	void Destroy();
	// Whether pSkySoundList holds a sound of form aiFormID for apWeather, comparing each item's form ID and weather.
	bool FindSkySound(uint32_t aiFormID, const TESWeather* apWeather) const;

	// The sky singleton, created on first use.
	static Sky* GetInstance();
	// The deleting destructor frees with ourFree.
	static void operator delete(void* apMemory);

	NiPointer<NiNode> spRoot;
	NiPointer<NiNode> spMoonsRoot;
	TESClimate* pCurrentClimate;
	TESWeather* pCurrentWeather;
	TESWeather* pLastWeather;
	TESWeather* pDefaultWeather;
	TESWeather* pOverrideWeather;
	Atmosphere* pAtmosphere;
	Stars* pStars;
	Sun* pSun;
	Clouds* pClouds;
	Moon* pMasser;
	Moon* pSecunda;
	Precipitation* pPrecip;
	NiColor SkyColor[10];
	NiColor WaterFogColor;
	NiColor SunSpecularColor;
	float fWindSpeed;
	float fWindAngle;
	float fFogDistances[4];
	float fFogHeight;
	float fFogPower;
	float fCurrentGameHour;
	float fLastWeatherUpdate;
	float fCurrentWeatherPct;
	SKY_MODE eMode;
	BSSimpleList<SkySound*>* pSkySoundList;
	float fFlash;
	uint32_t uiFlashTime;
	uint32_t uiLastMoonPhaseUpdate;
	float fWindowReflectionTimer;
	float fAccelBeginPct;
	BSTextureManager* textureManager;
	uint32_t uiFlags;
	ImageSpaceModifierInstanceForm* pCurrentWeatherImageSpaceMod;
	ImageSpaceModifierInstanceForm* pCurrentWeatherImageSpaceMod2;
	ImageSpaceModifierInstanceForm* pLastWeatherImageSpaceMod;
	ImageSpaceModifierInstanceForm* pLastWeatherImageSpaceMod2;
	float fHighNoon;
	float fMidnightWax;
	float fMidnightWane;

	// How far the daytime colors extend past sunrise and sunset, in hours.
	static SettingT<GameSettingCollection> fDaytimeColorExtension;
	// The singleton GetInstance creates.
	static inline Sky* pInstance;
	static inline BlendColor_t BlendColor;
	// A function-local static of UpdateSound.
	static inline bool bCleanList;
};

// The sunrise and sunset hours the climate sets, cached in the sky.
inline float fSunriseBegin;
inline float fSunriseEnd;
inline float fSunsetBegin;
inline float fSunsetEnd;
inline float fSunriseColorBegin;
inline float fSunsetColorEnd;

// The game settings Sky.cpp defines.
extern SettingT<INISettingCollection> bPrecipitation;
extern SettingT<GameSettingCollection> fSecundaAngleFadeStart;
extern SettingT<GameSettingCollection> fMasserAngleShadowEarlyFade;
extern SettingT<GameSettingCollection> fWeatherFlashAmbient;
extern SettingT<GameSettingCollection> fSecundaSpeed;
extern SettingT<GameSettingCollection> fSecundaAngleFadeEnd;
extern SettingT<GameSettingCollection> fMasserSpeed;
extern SettingT<GameSettingCollection> fWeatherTransMin;
extern SettingT<GameSettingCollection> iMasserSize;
extern SettingT<GameSettingCollection> fWeatherTransAccel;
extern SettingT<GameSettingCollection> fWeatherFlashDuration;
extern SettingT<GameSettingCollection> fUnderwaterFullDepth;
extern SettingT<GameSettingCollection> fMasserZOffset;
extern SettingT<GameSettingCollection> fSecundaAngleShadowEarlyFade;
extern SettingT<GameSettingCollection> fSecundaZOffset;
extern SettingT<GameSettingCollection> fWeatherTransMax;
extern SettingT<GameSettingCollection> fMasserAngleFadeEnd;
extern SettingT<GameSettingCollection> fSecondsBetweenWindowUpdate;
extern SettingT<GameSettingCollection> fMasserAngleFadeStart;
extern SettingT<GameSettingCollection> fWeatherFlashDirectional;
extern SettingT<GameSettingCollection> iSecundaSize;
extern float fUnderwaterFullDepthRatio;
