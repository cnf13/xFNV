#include "Fallout Shared/Sky/Sky.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SkyShader.h"
#include "BSShader/SkyShaderProperty.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/Sky/Atmosphere.h"
#include "Fallout Shared/Sky/Clouds.h"
#include "Fallout Shared/Sky/Moon.h"
#include "Fallout Shared/Sky/Precipitation.h"
#include "Fallout Shared/Sky/Stars.h"
#include "Fallout Shared/Sky/Sun.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fMasserAngleFadeStart("fMasserAngleFadeStart", 55.0f);
SettingT<GameSettingCollection> fMasserAngleFadeEnd("fMasserAngleFadeEnd", 45.0f);
SettingT<GameSettingCollection> fMasserAngleShadowEarlyFade("fMasserAngleShadowEarlyFade", 0.5f);
SettingT<GameSettingCollection> fMasserSpeed("fMasserSpeed", 0.25f);
SettingT<GameSettingCollection> fMasserZOffset("fMasserZOffset", 35.0f);
SettingT<GameSettingCollection> iMasserSize("iMasserSize", 94);
SettingT<GameSettingCollection> fSecundaAngleFadeStart("fSecundaAngleFadeStart", 55.0f);
SettingT<GameSettingCollection> fSecundaAngleFadeEnd("fSecundaAngleFadeEnd", 40.0f);
SettingT<GameSettingCollection> fSecundaAngleShadowEarlyFade("fSecundaAngleShadowEarlyFade", 0.5f);
SettingT<GameSettingCollection> fSecundaSpeed("fSecundaSpeed", 0.3f);
SettingT<GameSettingCollection> fSecundaZOffset("fSecundaZOffset", 50.0f);
SettingT<GameSettingCollection> iSecundaSize("iSecundaSize", 40);
SettingT<GameSettingCollection> fWeatherTransMax("fWeatherTransMax", 0.25f);
SettingT<GameSettingCollection> fWeatherTransMin("fWeatherTransMin", 0.01f);
SettingT<GameSettingCollection> fWeatherTransAccel("fWeatherTransAccel", 4.0f);
SettingT<GameSettingCollection> fWeatherFlashDuration("fWeatherFlashDuration", 0.25f);
SettingT<GameSettingCollection> fWeatherFlashDirectional("fWeatherFlashDirectional", 1.0f);
SettingT<GameSettingCollection> fWeatherFlashAmbient("fWeatherFlashAmbient", 0.3f);
SettingT<GameSettingCollection> fSecondsBetweenWindowUpdate("fSecondsBetweenWindowUpdate", 10.0f);
SettingT<GameSettingCollection> fUnderwaterFullDepth("fUnderwaterFullDepth", 1000.0f);
SettingT<GameSettingCollection> Sky::fDaytimeColorExtension("fDaytimeColorExtension", 0.5f);
SettingT<INISettingCollection> bPrecipitation("bPrecipitation:Weather", true);
float fUnderwaterFullDepthRatio = 1.0f / fUnderwaterFullDepth.Float();

SkySound::SkySound(
    BSSoundHandle aSndHandle,
    const TESWeather* apWeather,
    TESWeather::SoundType aeSoundType,
    uint32_t aiFormID
)
  : SndHandle(aSndHandle), pWeather(apWeather), eSoundType(aeSoundType), uiFormID(aiFormID), uiData(0) {}

bool SkySound::HandleCheckTime(int32_t aiInterval) {
    uint32_t uiNow = BSGetTickCount();
    uint32_t uiWhen = GetNextCheck();
    if (uiNow <= BSMIN(0x80000000u, uiWhen)) {
        return false;
    }
    SetNextCheck(aiInterval + uiNow);
    return true;
}

void SkySound::SetNextCheck(uint32_t uiWhen) {
    uiData &= FLAG_KILL;
    uiData |= uiWhen & ~FLAG_KILL;
}

void SkySound::FlagToKill() {
    uiData |= FLAG_KILL;
}

void SkySound::Play(float afVol) {
    if (!Interface::IsInMenuMode() && !Interface::IsInGameLoadingMenuOpen()) {
        if (!SndHandle.IsValid()) {
            uint32_t uiSoundID = SndHandle.iSoundID;
            SndHandle = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 32u);
        }
        if (eSoundType == TESWeather::WST_THUNDER) {
            if (!SndHandle.IsPlaying()) {
                SndHandle.Play(false);
            }
        }
        else if (HandleCheckTime(1000) && !SndHandle.IsPlaying()) {
            SndHandle.Play(true);
        }
        float fVolumeDelta = SndHandle.GetVolume() - afVol;
        if (std::fabs(fVolumeDelta) > 0.009999999776482582) {

            SndHandle.SetVolume(afVol);
        }
    }
}

Sky::Sky()
  : spRoot(nullptr), spMoonsRoot(nullptr), pCurrentClimate(nullptr), pCurrentWeather(nullptr), pLastWeather(nullptr),
    pDefaultWeather(nullptr), pOverrideWeather(nullptr), pAtmosphere(nullptr), pStars(nullptr), pSun(nullptr),
    pClouds(nullptr), pMasser(nullptr), pSecunda(nullptr), pPrecip(nullptr), WaterFogColor(0.0f, 0.0f, 0.0f),
    SunSpecularColor(NiColor::WHITE), fWindSpeed(0.0f), fWindAngle(1.0f), fCurrentGameHour(10.0f),
    fLastWeatherUpdate(0.0f), fCurrentWeatherPct(1.0f), eMode(SM_COUNT), pSkySoundList(new BSSimpleList<SkySound*>()),
    fFlash(0.0f), uiLastMoonPhaseUpdate(0), fWindowReflectionTimer(fSecondsBetweenWindowUpdate.Float()),
    fAccelBeginPct(0.0f), uiFlags(0), pCurrentWeatherImageSpaceMod(nullptr), pCurrentWeatherImageSpaceMod2(nullptr),
    pLastWeatherImageSpaceMod(nullptr), pLastWeatherImageSpaceMod2(nullptr), fHighNoon(12.0f), fMidnightWax(23.99f),
    fMidnightWane(0.0f) {
    for (NiColor& kColor : SkyColor) {
        kColor = NiColor::WHITE;
    }
    UpdateMoonPhase(true);
}

Sky::~Sky() {
    delete pAtmosphere;
    delete pStars;
    delete pClouds;
    delete pSun;
    delete pMasser;
    delete pSecunda;
    delete pPrecip;
    if (spRoot != nullptr && spRoot->m_pkParent) {
        spRoot->m_pkParent->DetachChild(spRoot.m_pObject);
    }
    for (SkySound* pSound : pSkySoundList->ItemsUntilNull()) {
        delete pSound;
    }
    pSkySoundList->RemoveAll();
    delete pSkySoundList;
}

// spRoot's object.
NiNode* Sky::GetRoot() {
    return spRoot;
}

bool Sky::IsRaining() {
    return (pCurrentWeather &&
            (static_cast<uint8_t>(pCurrentWeather->IsRainy()) &&
             static_cast<double>(fCurrentWeatherPct) >
                 pCurrentWeather->GetWeatherData(TESWeather::WD_BEGIN_PRECIP, 0.999f, 0.0))) ||
        (pLastWeather &&
         (static_cast<uint8_t>(pLastWeather->IsRainy()) &&
          static_cast<double>(fCurrentWeatherPct) <
              pLastWeather->GetWeatherData(TESWeather::WD_END_PRECIP, 1.0, 0.001f)));
}

void Sky::ForceWeather(TESWeather* apWeather, bool abOverride) {
    if (abOverride) {
        pOverrideWeather = apWeather;
        pCurrentWeather = apWeather;
        pDefaultWeather = nullptr;
    }
    else {
        pDefaultWeather = apWeather;
        pCurrentWeather = apWeather;
        pOverrideWeather = nullptr;
    }
    pLastWeather = nullptr;
    SetAcceleration(false);
    fCurrentWeatherPct = 1.0f;
    fLastWeatherUpdate = fCurrentGameHour;
    pPlayer->SetOccupiedRegion(nullptr);
    uiFlags |= FLAG_WEATHER_FORCED;
    uiFlags &= ~FLAG_UPDATE_WEATHER;
    UpdateHDRValues();
    if (pClouds) {
        pClouds->ClearTransTextures();
    }
    if (pPrecip) {
        pPrecip->FlushAll();
    }
}

Sky::SKY_MODE Sky::GetMode() {
    return eMode;
}

void Sky::SetMode(SKY_MODE aeNewMode) {
    if ((eMode == SM_NONE || eMode == SM_INTERIOR || eMode == SM_COUNT) &&
        (aeNewMode == SM_FULL || aeNewMode == SM_FAKEEXTERIOR)) {
        ReloadAllTextures();
        if (pClouds) {
            pClouds->ForceTransTextureUpdate();
        }
        if (spRoot) {
            spRoot->SetAppCulled(false);
        }
        if (pSun) {
            pSun->GetSunGlareNode()->SetAppCulled(false);
        }
    }
    else if (
        (eMode == SM_FULL || eMode == SM_FAKEEXTERIOR || eMode == SM_COUNT) &&
        (aeNewMode == SM_NONE || aeNewMode == SM_INTERIOR)
    ) {
        UnloadAllTextures();
        if (spRoot) {
            spRoot->SetAppCulled(true);
        }
        if (pSun) {
            pSun->GetSunGlareNode()->SetAppCulled(true);
        }
        while (!pSkySoundList->IsEmpty()) {
            if (pSkySoundList->GetItem()->SndHandle.IsPlaying()) {
                pSkySoundList->GetItem()->SndHandle.Stop();
            }
            pSkySoundList->GetItem()->SndHandle.Release();
            delete pSkySoundList->GetItem();
            pSkySoundList->RemoveHead();
        }
        SkySound::ucThunderCount = 0;
        eMode = aeNewMode;
        if (pPrecip) {
            pPrecip->Update(0.0f);
        }
        fFlash = 0.0f;
    }
    eMode = aeNewMode;
    UpdateHDRValues();
}

// Flags the camera as underwater, then (outside an interior that is not loading a save) updates the weather, its
// colors, fog, wind and sounds, the sky objects, the moon phase once a day, and fades the lightning flash. The uiFlags
// bits are set and cleared as read-modify-writes of the whole 32-bit value, rewriting all four bytes.
void Sky::Update(float afSecondsPassed) {
    uint32_t& ruFlags = uiFlags;
    // The context lives in raw stack storage and is constructed and destroyed explicitly.
    uint8_t kContextStorage[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextStorage) AutoMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sky.cpp",
        472
    );
    if (!pPlayer->GetParentCell()) {
        pContext->~AutoMemContext();
        return;
    }
    fCurrentGameHour = appCalendar.GetHour();
    bool bUnderwater = false;
    if (pPlayer && pPlayer->GetParentCell()) {
        const NiPoint3& kCameraPos = Main::GetWorldSceneGraph()->GetCameraRoot()->GetWorldTranslate();
        float fWaterHeight = pPlayer->GetRelevantWaterHeight();
        bUnderwater = kCameraPos.z < fWaterHeight;
    }
    if (bUnderwater) {
        ruFlags |= FLAG_UNDERWATER;
    }
    else {
        ruFlags &= ~FLAG_UNDERWATER;
    }
    if (eMode == SM_INTERIOR && !pSaveLoadGame->GetSaveGameLoading()) {
        UpdateInterior(afSecondsPassed);
        pContext->~AutoMemContext();
        return;
    }
    HandleClimateChange();
    if (pSaveLoadGame->GetSaveGameLoading()) {
        ruFlags |= FLAG_UPDATE_WEATHER;
    }
    UpdateWeather();
    UpdateColors();
    UpdateFog();
    UpdateWind();
    UpdateSound();
    if (pAtmosphere) {
        pAtmosphere->Update(this, afSecondsPassed);
    }
    if (pStars) {
        pStars->Update(this, afSecondsPassed);
    }
    if (pClouds) {
        pClouds->Update(this, afSecondsPassed);
    }
    if (pSun) {
        pSun->Update(this, afSecondsPassed);
    }
    // The days passed compare is unsigned.
    if (((uiFlags & FLAG_UPDATE_MOON_PHASE) && uiLastMoonPhaseUpdate < appCalendar.GetDaysPassed()) ||
        pSaveLoadGame->GetSaveGameLoading()) {
        bool bPhaseChange = Moon::UpdatePhase(this);
        if (pMasser && bPhaseChange && pMasser->eUpdateMoonTexture == Moon::US_NOT_REQUIRED) {
            pMasser->eUpdateMoonTexture = pSaveLoadGame->GetSaveGameLoading() ? Moon::US_INITIALIZE
                                                                              : Moon::US_WHEN_CULLED;
        }
        if (pSecunda && bPhaseChange && pSecunda->eUpdateMoonTexture == Moon::US_NOT_REQUIRED) {
            pSecunda->eUpdateMoonTexture = pSaveLoadGame->GetSaveGameLoading() ? Moon::US_INITIALIZE
                                                                               : Moon::US_WHEN_CULLED;
        }
        uiLastMoonPhaseUpdate = appCalendar.GetDaysPassed();
    }
    if (pMasser) {
        pMasser->Update(this, afSecondsPassed);
    }
    if (pSecunda) {
        pSecunda->Update(this, afSecondsPassed);
    }
    if (pPrecip) {
        pPrecip->Update(afSecondsPassed);
    }
    if (fFlash > 0.0) {
        // Widens the elapsed milliseconds (unsigned) to double before it reads the duration setting.
        double fElapsed = static_cast<double>(appTimer.GetTime() - uiFlashTime);
        fFlash = static_cast<float>(1.0 - fElapsed / (static_cast<double>(fWeatherFlashDuration.Float()) * 1000.0));
    }
    if (fFlash < 0.0) {
        fFlash = 0.0f;
    }
    Renderer::SetBackgroundColor(&SkyColor[1]);
    ruFlags &= ~WEATHER_UPDATE_MASK;
    pContext->~AutoMemContext();
}

void Sky::UpdateColors() {
    if (eMode == SM_NONE) {
        SkyColor[4] = NiColor(0.95f, 0.85f, 0.85f);
        SkyColor[3] = NiColor(0.35f, 0.35f, 0.43f);
        if (pClouds) {
            for (int32_t uiLayer = 0; uiLayer < pClouds->GetNumLayers(); ++uiLayer) {
                NiColor kLayerColor(0.1f, 0.1f, 0.1f);
                pClouds->SetColor(uiLayer, kLayerColor);
            }
        }
        SunSpecularColor = NiColor::BLACK;
        SkyColor[8] = SunSpecularColor;
        SkyColor[5] = SkyColor[8];
        SkyColor[0] = SkyColor[5];
        SkyColor[1] = SkyColor[0];
        SkyColor[7] = SkyColor[1];
        return;
    }
    if (eMode == SM_INTERIOR && TES::pInstance && TES::pInstance->GetInterior()) {
        TES::pInstance->GetInterior()->GetDataDirectionalColor(SkyColor[4]);
        TES::pInstance->GetInterior()->GetDataAmbientColor(SkyColor[3]);
        TES::pInstance->GetInterior()->GetDataFogColor(SkyColor[1]);
        SunSpecularColor = SkyColor[1];
        SkyColor[8] = SunSpecularColor;
        SkyColor[5] = SkyColor[8];
        SkyColor[0] = SkyColor[5];
        SkyColor[7] = SkyColor[0];
        if (pClouds) {
            for (int32_t uiLayer = 0; uiLayer < pClouds->GetNumLayers(); ++uiLayer) {
                pClouds->SetColor(uiLayer, SkyColor[1]);
            }
        }
        return;
    }
    COLOR_BLEND kColorBlend;
    TESWeather::ColorTime eTime1;
    TESWeather::ColorTime eTime2;
    FillColorBlend(kColorBlend, pCurrentWeather, fCurrentWeatherPct, eTime1, eTime2);
    for (int32_t eType = TESWeather::SKY_UPPER; eType < static_cast<int32_t>(TESWeather::NUM_TYPES); ++eType) {
        FillColorBlendColors(
            kColorBlend,
            pCurrentWeather,
            pLastWeather,
            static_cast<TESWeather::ColorType>(eType),
            eTime1,
            eTime2
        );
        float fFlashAmount = 0.0f;
        if (!pLastWeather) {
            if (eType == TESWeather::SUNLIGHT) {
                fFlashAmount = fFlash * fWeatherFlashDirectional.Float();
            }
            else if (eType == TESWeather::AMBIENT) {
                fFlashAmount = fFlash * fWeatherFlashAmbient.Float();
            }
        }
        SetColor(SkyColor[eType], &kColorBlend, fFlashAmount);
    }
    if (pClouds) {
        for (int32_t uiLayer = 0; uiLayer < pClouds->GetNumLayers(); ++uiLayer) {
            FillCloudColorBlendColors(kColorBlend, pCurrentWeather, pLastWeather, uiLayer, eTime1, eTime2);
            float fCloudFlash = 0.0f;
            if (!pLastWeather && !uiLayer) {
                fCloudFlash = fFlash;
            }
            NiColor kCloudColor(0.0f, 0.0f, 0.0f);
            SetColor(kCloudColor, &kColorBlend, fCloudFlash);
            pClouds->SetColor(uiLayer, kCloudColor);
        }
    }
    SunSpecularColor = SkyColor[4];
}

void Sky::UpdateFog() {
    Sky::COLOR_BLEND cb;
    float fCamZ = 0.0f;
    TESWaterForm* pWater = TESWaterSystem::GetCurrentWaterType();
    bool bUnderwaterFog = false;
    if (TESWaterSystem::GetPlayerUnderwater() || TESWaterListener::GetUnderwater()) {
        SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
        fCamZ = WorldSceneGraph->GetCameraRoot()->GetWorldTranslate().z;
        bUnderwaterFog = pWater &&
            (TESWaterListener::GetUnderwater() ||
             static_cast<double>(fCamZ) < pPlayer->GetParentCell()->GetWaterHeight());
    }
    if (bUnderwaterFog) {
        fFogDistances[0u] = pWater->Data.fUnderwaterFogDistNear;
        fFogDistances[1] = pWater->Data.fUnderwaterFogDistFar;
        fFogPower = 1.0f;
        float fRelevantHeight = TESWaterSystem::GetCameraRelevantWaterHeight();
        if (static_cast<double>(fCamZ) < pPlayer->GetParentCell()->GetWaterHeight()) {
            fRelevantHeight = pPlayer->GetParentCell()->GetWaterHeight();
        }
        float fDepth = (fRelevantHeight - fCamZ) * fUnderwaterFullDepthRatio;
        if (fDepth > 1.0) {
            fDepth = 1.0f;
        }
        cb.uiRGBVal[0] = pWater->Data.iShallowWaterColor;
        cb.uiRGBVal[1] = pWater->Data.iDeepWaterColor;
        cb.uiRGBVal[2] = 0;
        cb.uiRGBVal[3] = 0;
        cb.fBlend[0] = 1.0 - fDepth;
        cb.fBlend[1] = fDepth;
        cb.fBlend[2] = 0.0f;
        cb.fBlend[3] = 0.0f;
        SetColor(SkyColor[1], &cb, 0.0f);
        if (eMode == Sky::SM_FULL || eMode == Sky::SM_FAKEEXTERIOR) {
            bool bCull = fDepth > 0.5;
            for (int32_t i = 0; i < pClouds->GetNumLayers(); ++i) {
                NiGeometry* pCloudsGeo = pClouds->GetCloudsGeo(i);
                pCloudsGeo->SetAppCulled(bCull);
            }
            pAtmosphere->GetAthmosphereGeo()->SetAppCulled(bCull);
            pSun->GetSunBaseNode()->SetAppCulled(bCull);
            pSun->GetSunGlareNode()->SetAppCulled(bCull);
            spRoot->SetAppCulled(bCull);
        }
    }
    else {
        if (pCurrentWeather && (eMode == Sky::SM_FULL || eMode == Sky::SM_FAKEEXTERIOR)) {
            float fDayBasis;
            float fSunriseBegin = GetBeginSunriseColors();
            float fSunriseEnd = GetSunriseEnd();
            float fSunsetBegin = GetSunsetBegin();
            float fSunsetEnd = GetEndSunsetColors();
            if (!(fSunriseBegin < static_cast<double>(fCurrentGameHour)) ||
                !(fSunriseEnd > static_cast<double>(fCurrentGameHour))) {
                if (!(fSunriseEnd <= static_cast<double>(fCurrentGameHour)) ||
                    !(fSunsetBegin >= static_cast<double>(fCurrentGameHour))) {
                    if (!(fSunsetBegin < static_cast<double>(fCurrentGameHour)) ||
                        !(fSunsetEnd > static_cast<double>(fCurrentGameHour))) {
                        fDayBasis = 0.0f;
                    }
                    else {
                        fDayBasis = (fSunsetEnd - fCurrentGameHour) / (fSunsetEnd - fSunsetBegin);
                    }
                }
                else {
                    fDayBasis = 1.0f;
                }
            }
            else {
                fDayBasis = (fCurrentGameHour - fSunriseBegin) / (fSunriseEnd - fSunriseBegin);
            }
            float fNightBasis = 1.0 - fDayBasis;
            double dNearDay = pCurrentWeather->GetFogData(TESWeather::NEAR_DIST_DAY) * fDayBasis;
            fFogDistances[0u] = pCurrentWeather->GetFogData(TESWeather::NEAR_DIST_NIGHT) * fNightBasis + dNearDay;
            double dFarDay = pCurrentWeather->GetFogData(TESWeather::FAR_DIST_DAY) * fDayBasis;
            fFogDistances[1] = pCurrentWeather->GetFogData(TESWeather::FAR_DIST_NIGHT) * fNightBasis + dFarDay;
            double dPowerDay = pCurrentWeather->GetFogData(TESWeather::FOG_POWER_DAY) * fDayBasis;
            fFogPower = pCurrentWeather->GetFogData(TESWeather::FOG_POWER_NIGHT) * fNightBasis + dPowerDay;
            if (pLastWeather) {
                fFogDistances[0u] = fFogDistances[0u] * fCurrentWeatherPct;
                double dLastPct = 1.0 - fCurrentWeatherPct;
                dNearDay = pLastWeather->GetFogData(TESWeather::NEAR_DIST_DAY) * fDayBasis;
                fFogDistances[0u] = (pLastWeather->GetFogData(TESWeather::NEAR_DIST_NIGHT) * fNightBasis + dNearDay) *
                        dLastPct +
                    fFogDistances[0u];
                fFogDistances[1] = fFogDistances[1] * fCurrentWeatherPct;
                dLastPct = 1.0 - fCurrentWeatherPct;
                dFarDay = pLastWeather->GetFogData(TESWeather::FAR_DIST_DAY) * fDayBasis;
                fFogDistances[1] = (pLastWeather->GetFogData(TESWeather::FAR_DIST_NIGHT) * fNightBasis + dFarDay) *
                        dLastPct +
                    fFogDistances[1];
                fFogPower = fFogPower * fCurrentWeatherPct;
                dLastPct = 1.0 - fCurrentWeatherPct;
                dPowerDay = pLastWeather->GetFogData(TESWeather::FOG_POWER_DAY) * fDayBasis;
                fFogPower = (pLastWeather->GetFogData(TESWeather::FOG_POWER_NIGHT) * fNightBasis + dPowerDay) *
                        dLastPct +
                    fFogPower;
            }
        }
        else if (eMode == Sky::SM_INTERIOR && TES::pInstance && TES::pInstance->pInteriorCell) {
            float fFar = TES::pInstance->pInteriorCell->GetDataFogFarPlane();
            if (fFar <= 0.0 || fFar > 163840.0) {
                fFar = 163840.0f;
            }
            float fNear = TES::pInstance->pInteriorCell->GetDataFogNearPlane();
            if (fNear <= 0.0 || fFar < static_cast<double>(fNear)) {
                fNear = fFar * 0.17000001668930054;
            }
            fFogDistances[0u] = fNear;
            fFogDistances[1] = fFar;
            fFogPower = TES::pInstance->pInteriorCell->GetDataFogPower();
        }
        else {
            fFogDistances[0u] = 163840.0f;
            fFogDistances[1] = 163840.0f;
            fFogPower = 1.0f;
        }
        if (!BGSTerrainManager::GetEnabled()) {
            float fClipFar = Main::GetWorldSceneGraph()->GetFarDistance();
            float fClipNear = BSSceneGraph::fNearDistance.Float();
            float fPctChange = (fFogDistances[1] - fClipFar) / (fFogDistances[1] - fClipNear);
            fFogDistances[0u] = fFogDistances[0u] - (fFogDistances[0u] - fClipNear) * fPctChange;
            fFogDistances[1] = fClipFar;
            fFogDistances[0u] = BSMIN(fClipFar, fFogDistances[0u]);
        }
    }
}

void Sky::UpdateWind() {
    if (pCurrentWeather && (eMode == SM_FULL || eMode == SM_FAKEEXTERIOR)) {
        fWindSpeed = pCurrentWeather->GetWeatherData(TESWeather::WD_WIND_SPEED, 1.0, 0.0);
        if (pLastWeather) {
            fWindSpeed = fWindSpeed * fCurrentWeatherPct;
            double dLastPct = 1.0 - fCurrentWeatherPct;
            fWindSpeed = static_cast<float>(
                pLastWeather->GetWeatherData(TESWeather::WD_WIND_SPEED, 1.0, 0.0) * dLastPct + fWindSpeed
            );
        }
        BSShaderManager::SetWindDirection(NiPoint2::UNIT_Y);
        BSShaderManager::SetWindMagnitude(fWindSpeed);
        NiPoint2 kTranslate;
        float fAngle = static_cast<float>(BSShaderManager::GetTimer(false) / 3600.0 * 6.283180236816406 * 15.0f);
        double dSin = NiSinQ(fAngle);
        kTranslate.x = static_cast<float>(dSin * 15.0f);
        float fCosAngle = static_cast<float>(fAngle * 0.699999988079071);
        double dCos = NiCosQ(fCosAngle);
        kTranslate.y = static_cast<float>(dCos * 15.0f);
        ShadowLightShader::SetProjectedShadowTranslate(kTranslate);
    }
}

void Sky::HandleClimateChange() {
    if ((uiFlags & FLAG_CLIMATE_CHANGED) != 0) {
        if (pStars && (eMode == Sky::SM_FULL || eMode == Sky::SM_FAKEEXTERIOR)) {
            const char* pNightSkyModel = pCurrentClimate->GetNightSkyModel();
            pStars->LoadGeometry(pNightSkyModel);
        }

        if (pSun && (eMode == Sky::SM_FULL || eMode == Sky::SM_FAKEEXTERIOR)) {
            BSStringT<char> strfilename;
            SkyShaderProperty* pProp = static_cast<SkyShaderProperty*>(pSun->GetBase()->GetProperty(3));
            NiTexture* pTex = nullptr;
            const TESTexture* pTESTexture = pCurrentClimate->GetTexture(TESClimate::TX_SUN);
            if (pTESTexture && pTESTexture->GetTextureNameLength()) {
                NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
                strfilename = pTESTexture->GetDefaultPath();
                strfilename += pTESTexture->GetTextureName();
                BSShaderManager::GetTexture(strfilename.pString, true, spTex, true, false);
                pTex = spTex.m_pObject;
                if (pTex) {
                    pProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
                }
            }

            pSun->GetSunBaseNode()->SetDisplayObject(pTex != nullptr);
            NiAVObject* pSunGlare = pSun->GetSunGlareGeo();
            pProp = static_cast<SkyShaderProperty*>(pSunGlare->GetProperty(3));
            pTESTexture = pCurrentClimate->GetTexture(TESClimate::TX_GLARE);
            if (pTESTexture && pTESTexture->GetTextureNameLength()) {
                if (BSShaderManager::bHDR || BSstricmp(pTESTexture->GetTextureName(), "Sky\\SunGlare.dds")) {
                    strfilename = pTESTexture->GetDefaultPath();
                    strfilename += pTESTexture->GetTextureName();
                }
                else {
                    strfilename = "Textures\\Sky\\SunGlareNonHDR.dds";
                }
                NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
                BSShaderManager::GetTexture(strfilename.pString, true, spTex, true, false);
                pTex = spTex.m_pObject;
                if (pTex) {
                    pProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
                }
            }

            pSun->GetSunGlareNode()->SetDisplayObject(pTex != nullptr);
        }

        if (!pCurrentClimate->UsesMasser() || pMasser) {
            if (!pCurrentClimate->UsesMasser()) {
                if (pMasser) {
                    delete pMasser;
                }
                pMasser = nullptr;
            }
        }
        else {
            pMasser = new Moon(
                "Masser",
                fMasserAngleFadeStart.Float(),
                fMasserAngleFadeEnd.Float(),
                fMasserAngleShadowEarlyFade.Float(),
                fMasserSpeed.Float(),
                fMasserZOffset.Float(),
                iMasserSize.Int()
            );
            pMasser->Initialize(spMoonsRoot.m_pObject, "Masser");
        }
        if (!pCurrentClimate->UsesSecunda() || pSecunda) {
            if (!pCurrentClimate->UsesSecunda()) {
                if (pSecunda) {
                    delete pSecunda;
                }
                pSecunda = nullptr;
            }
        }
        else {
            pSecunda = new Moon(
                "Secunda",
                fSecundaAngleFadeStart.Float(),
                fSecundaAngleFadeEnd.Float(),
                fSecundaAngleShadowEarlyFade.Float(),
                fSecundaSpeed.Float(),
                fSecundaZOffset.Float(),
                iSecundaSize.Int()
            );
            pSecunda->Initialize(spMoonsRoot.m_pObject, "Secunda");
        }
        spRoot->UpdateProperties();
        NiUpdateData UpdateData(0.0f, false, false);
        spRoot->Update(UpdateData);
        uiFlags &= ~FLAG_CLIMATE_CHANGED;
    }
}

void Sky::UpdateWeather() {
    if (pCurrentClimate) {
        double dHoursPassed;
        int32_t iWrapHours;
        TESWeather* pNewWeather;
        bool bPickWeather = (uiFlags & FLAG_UPDATE_WEATHER) != 0 || !pDefaultWeather;
        if (!bPickWeather) {
            if (!(fLastWeatherUpdate > static_cast<double>(fCurrentGameHour))) {
                iWrapHours = 0;
            }
            else {
                iWrapHours = 24;
            }
            dHoursPassed = static_cast<double>(iWrapHours) + fCurrentGameHour - fLastWeatherUpdate;
            bPickWeather = static_cast<double>(22 * (255 - pCurrentClimate->GetVolatilityIndex())) *
                            0.003921568859368563 +
                        1.0 <
                    dHoursPassed &&
                !pLastWeather;
        }
        if (bPickWeather) {
            pDefaultWeather = pCurrentClimate->GetRandomWeather();
            if (!pDefaultWeather) {
                TESForm* pForm = TESForm::GetFormByNumericID(350u);
                pDefaultWeather = dynamic_cast<TESWeather*>(pForm);
            }
            TESRegionList* pRegions = pDataHandler->GetRegionList();
            for (BSSimpleList<TESRegion*>* pRegionList = static_cast<BSSimpleList<TESRegion*>*>(pRegions);
                 pRegionList && pRegionList->m_item;
                 pRegionList = pRegionList->m_pkNext) {
                pRegionList->m_item->UpdateWeather();
            }
        }
        bool bLoadNewWeather = pOverrideWeather == nullptr;
        if (pCurrentWeather && pSaveLoadGame->GetSaveGameLoading()) {
            bLoadNewWeather = false;
        }

        if (bLoadNewWeather) {
            pNewWeather = pDefaultWeather;
            TESRegion* pPlayerRegion = pPlayer->GetOccupiedRegion();
            if (pPlayerRegion && pPlayerRegion->pCurrentWeather && eMode != Sky::SM_FAKEEXTERIOR) {
                pNewWeather = pPlayerRegion->pCurrentWeather;
            }
        }
        else {
            pNewWeather = pOverrideWeather;
        }
        if (!pNewWeather || (pLastWeather && (uiFlags & FLAG_FAST_TRAVEL) == 0) || pNewWeather == pCurrentWeather) {
            uiFlags &= ~FLAG_UPDATE_WEATHER;
        }
        else {
            if ((uiFlags & FLAG_FAST_TRAVEL) != 0 || eMode == Sky::SM_INTERIOR) {
                pLastWeather = nullptr;
                if (pPrecip) {
                    pPrecip->FlushAll();
                }
            }
            else {
                pLastWeather = pCurrentWeather;
            }
            pCurrentWeather = pNewWeather;
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                fLastWeatherUpdate = fCurrentGameHour;
            }
            uiFlags |= FLAG_UPDATE_WEATHER;
        }
        if (pCurrentWeather) {
            if (pLastWeather) {
                iWrapHours = fLastWeatherUpdate > static_cast<double>(fCurrentGameHour) ? 24 : 0;
                dHoursPassed = static_cast<double>(iWrapHours) + fCurrentGameHour - fLastWeatherUpdate;
                float fWeatherTransMin = ::fWeatherTransMin.Float();
                fCurrentWeatherPct = dHoursPassed /
                    pCurrentWeather
                        ->GetWeatherData(TESWeather::WD_TRANS_DELTA, fWeatherTransMax.Float(), fWeatherTransMin);
            }
            else {
                fCurrentWeatherPct = 1.0f;
            }
        }
        else {
            fCurrentWeatherPct = 0.0f;
        }
        if ((uiFlags & FLAG_ACCELERATION) != 0) {
            double dAccelPct = fCurrentWeatherPct - fAccelBeginPct;
            fCurrentWeatherPct = (fWeatherTransAccel.Float() + 1.0) * dAccelPct + fAccelBeginPct;
        }
        if (fCurrentWeatherPct > 1.0) {
            pLastWeather = nullptr;
            fCurrentWeatherPct = 1.0f;
            SetAcceleration(false);
        }
        UpdateHDRValues();
    }
}

void Sky::UpdateSound() {
    SkySound* pCurrentSound;
    BSSimpleList<SkySound*>* pCurrentNode = pSkySoundList;
    if (Sky::bCleanList) {
        BSSimpleList<SkySound*>* pSaveList = new BSSimpleList<SkySound*>;
        BSSimpleList<SkySound*> oldHandleList;
        while (pCurrentNode) {
            pCurrentSound = pCurrentNode->m_item;
            if (!pCurrentSound) {
                break;
            }
            if (pCurrentSound->IsFlaggedToBeKilled()) {
                if (pCurrentSound->eSoundType == TESWeather::WST_THUNDER) {
                    --SkySound::ucThunderCount;
                }
                pCurrentSound->SndHandle.Release();
                delete pCurrentSound;
            }
            else {
                pSaveList->AddHead(pCurrentSound);
            }
            pCurrentNode = pCurrentNode->m_pkNext;
        }
        BSSimpleList<SkySound*>* pOldHandleNode = &oldHandleList;
        while (true) {
            SkySound* pOldHandle;
            if (!pOldHandleNode || (pOldHandle = pOldHandleNode->m_item) == nullptr) {
                oldHandleList.RemoveAll();
                pSkySoundList->RemoveAll();
                if (pSkySoundList) {
                    delete pSkySoundList;
                }
                pSkySoundList = pSaveList;
                pCurrentNode = pSaveList;
                Sky::bCleanList = false;
                break;
            }
            pOldHandleNode = pOldHandleNode->m_pkNext;
            if (pOldHandle->SndHandle.IsValid()) {
                bool bFound = false;
                for (pCurrentNode = pSaveList; !bFound; pCurrentNode = pCurrentNode->m_pkNext) {
                    if (!pCurrentNode) {
                        break;
                    }
                    pCurrentSound = pCurrentNode->m_item;
                    if (!pCurrentSound) {
                        break;
                    }
                    bFound = pCurrentSound->uiFormID == pOldHandle->uiFormID;
                }
                if (bFound) {
                    continue;
                }
                pOldHandle->SndHandle.Release();
            }
            delete pOldHandle;
        }
    }
    if (GetWeatherUpdateFlag()) {
        ActivateWeatherSounds(this->pCurrentWeather);
    }
    while (pCurrentNode) {
        pCurrentSound = pCurrentNode->m_item;
        if (!pCurrentSound) {
            break;
        }
        if (pCurrentSound->SndHandle.IsValid()) {
            if ((eMode == Sky::SM_FULL || eMode == Sky::SM_FAKEEXTERIOR) &&
                (pCurrentSound->pWeather == this->pCurrentWeather || pCurrentSound->pWeather == this->pLastWeather)) {
                if (pCurrentSound->eSoundType == TESWeather::WST_THUNDER) {
                    if (!pCurrentSound->SndHandle.IsPlaying() && pCurrentSound->HandleCheckTime(30)) {
                        uint32_t uiRandom = Random();
                        if (!(uiRandom %
                              (SkySound::ucThunderCount *
                               static_cast<uint32_t>(
                                   pCurrentSound->pWeather->GetWeatherDataRaw(TESWeather::WD_THUNDER_FREQ)
                               )))) {
                            float fWeight;
                            if (pCurrentSound->pWeather == this->pCurrentWeather) {
                                double dBlend;
                                float fBeginPct = this->pCurrentWeather
                                                      ->GetWeatherData(TESWeather::WD_BEGIN_THUNDER, 0.999f, 0.0f);
                                if (!(fBeginPct > static_cast<double>(fCurrentWeatherPct))) {
                                    dBlend = (fCurrentWeatherPct - fBeginPct) / (1.0 - fBeginPct);
                                }
                                else {
                                    dBlend = 0.0;
                                }
                                fWeight = dBlend;
                            }
                            else {
                                double dBlend;
                                float fEndPct = this->pLastWeather
                                                    ->GetWeatherData(TESWeather::WD_END_THUNDER, 1.0f, 0.001f);
                                if (!(fEndPct > static_cast<double>(fCurrentWeatherPct))) {
                                    dBlend = 0.0;
                                }
                                else {
                                    dBlend = 1.0 - fCurrentWeatherPct / fEndPct;
                                }
                                fWeight = dBlend;
                            }
                            if (fWeight != 0.0) {
                                pCurrentSound->Play(fWeight);
                                fFlash = fWeight;
                                uiFlashTime = appTimer.iLastTime;
                            }
                        }
                    }
                }
                else {
                    float fAltWeight;
                    float fWeight;
                    if (pCurrentSound->eSoundType == TESWeather::WST_PRECIP) {
                        float fEndPct;
                        float fBeginPct = this->pCurrentWeather->GetWeatherData(TESWeather::WD_DIVIDER1, 0.999f, 0.0f);
                        if (this->pLastWeather) {
                            fEndPct = this->pLastWeather->GetWeatherData(TESWeather::WD_END_PRECIP, 1.0f, 0.001f);
                        }
                        else {
                            fEndPct = 0.0f;
                        }
                        if (pCurrentSound->pWeather == this->pCurrentWeather) {
                            double dAltBlend;
                            double dBlend;
                            if (!(fBeginPct > static_cast<double>(fCurrentWeatherPct))) {
                                dBlend = (fCurrentWeatherPct - fBeginPct) / (1.0 - fBeginPct);
                            }
                            else {
                                dBlend = 0.0;
                            }
                            fWeight = dBlend;
                            if (!(fEndPct > static_cast<double>(fCurrentWeatherPct))) {
                                dAltBlend = 0.0;
                            }
                            else {
                                dAltBlend = 1.0 - fCurrentWeatherPct / fEndPct;
                            }
                            fAltWeight = dAltBlend;
                        }
                        else {
                            double dAltBlend;
                            double dBlend;
                            if (!(fEndPct > static_cast<double>(fCurrentWeatherPct))) {
                                dBlend = 0.0;
                            }
                            else {
                                dBlend = 1.0 - fCurrentWeatherPct / fEndPct;
                            }
                            fWeight = dBlend;
                            if (!(fBeginPct > static_cast<double>(fCurrentWeatherPct))) {
                                dAltBlend = (fCurrentWeatherPct - fBeginPct) / (1.0 - fBeginPct);
                            }
                            else {
                                dAltBlend = 0.0;
                            }
                            fAltWeight = dAltBlend;
                        }
                    }
                    else if (pCurrentSound->pWeather == this->pCurrentWeather) {
                        fWeight = fCurrentWeatherPct;
                        fAltWeight = 1.0 - fWeight;
                    }
                    else {
                        fWeight = 1.0 - fCurrentWeatherPct;
                        fAltWeight = fCurrentWeatherPct;
                    }
                    const TESWeather* pOtherWeather = pCurrentSound->pWeather == this->pCurrentWeather
                        ? this->pLastWeather
                        : this->pCurrentWeather;
                    if (!FindSkySound(pCurrentSound->uiFormID, pOtherWeather) ||
                        fAltWeight <= static_cast<double>(fWeight)) {
                        pCurrentSound->Play(fWeight);
                    }
                }
            }
            else {
                if (pCurrentSound->SndHandle.IsPlaying()) {
                    const TESWeather* pOtherWeather = pCurrentSound->pWeather == this->pCurrentWeather
                        ? this->pLastWeather
                        : this->pCurrentWeather;

                    if (!FindSkySound(pCurrentSound->uiFormID, pOtherWeather)) {
                        pCurrentSound->SndHandle.Stop();
                    }
                }
                pCurrentSound->FlagToKill();
                Sky::bCleanList = true;
            }
        }
        pCurrentNode = pCurrentNode->m_pkNext;
    }
}

void Sky::FillCloudColorBlendColors(
    Sky::COLOR_BLEND& aColorBlend,
    TESWeather* apCurrentWeather,
    TESWeather* apLastWeather,
    int32_t aiLayer,
    TESWeather::ColorTime& aeTime1,
    TESWeather::ColorTime& aeTime2
) {
    if (!apCurrentWeather) {
        return;
    }
    aColorBlend.uiRGBVal[0] = apCurrentWeather->GetCloudColor(aiLayer, aeTime1);
    aColorBlend.uiRGBVal[1] = apCurrentWeather->GetCloudColor(aiLayer, aeTime2);
    if (apLastWeather) {
        aColorBlend.uiRGBVal[2] = apLastWeather->GetCloudColor(aiLayer, aeTime1);
        aColorBlend.uiRGBVal[3] = apLastWeather->GetCloudColor(aiLayer, aeTime2);
        if (!aColorBlend.uiRGBVal[0]) {
            aColorBlend.uiRGBVal[0] = aColorBlend.uiRGBVal[2];
        }
        if (!aColorBlend.uiRGBVal[1]) {
            aColorBlend.uiRGBVal[1] = aColorBlend.uiRGBVal[3];
        }
        if (!aColorBlend.uiRGBVal[2]) {
            aColorBlend.uiRGBVal[2] = aColorBlend.uiRGBVal[0];
        }
        if (!aColorBlend.uiRGBVal[3]) {
            aColorBlend.uiRGBVal[3] = aColorBlend.uiRGBVal[1];
        }
    }
    else {
        aColorBlend.uiRGBVal[3] = 0;
        aColorBlend.uiRGBVal[2] = 0;
    }
}

float Sky::GetCellFogPower() {
    return fFogPower;
}

float Sky::GetWeatherPercent() {
    return fCurrentWeatherPct;
}

float Sky::GetFlash() {
    return fFlash;
}

float Sky::GetCurrentGameHour() {
    return fCurrentGameHour;
}

bool Sky::GetWeatherUpdateFlag() {
    return (uiFlags & WEATHER_UPDATE_MASK) != 0;
}

void Sky::InvalidateCachedValues() {
    uiFlags |= CACHED_VALUES_MASK;
}

void Sky::UpdateMoonPhase(bool abUpdate) {
    if (abUpdate) {
        uiFlags |= FLAG_UPDATE_MOON_PHASE;
    }
    else {
        uiFlags &= ~FLAG_UPDATE_MOON_PHASE;
    }
}

float Sky::GetSunriseBegin() {
    if ((uiFlags & FLAG_UPDATE_SUNRISE_BEGIN) != 0 && pCurrentClimate != nullptr) {
        fSunriseBegin = static_cast<float>(
            static_cast<double>(pCurrentClimate->GetTransTime(TESClimate::SUNRISE_BEGIN)) / 6.0
        );
        uiFlags &= ~FLAG_UPDATE_SUNRISE_BEGIN;
    }
    return fSunriseBegin;
}

float Sky::GetSunriseEnd() {
    if ((uiFlags & FLAG_UPDATE_SUNRISE_END) != 0 && pCurrentClimate != nullptr) {
        fSunriseEnd = static_cast<float>(
            static_cast<double>(pCurrentClimate->GetTransTime(TESClimate::SUNRISE_END)) / 6.0
        );
        uiFlags &= ~FLAG_UPDATE_SUNRISE_END;
    }
    return fSunriseEnd;
}

float Sky::GetSunsetBegin() {
    if ((uiFlags & FLAG_UPDATE_SUNSET_BEGIN) != 0 && pCurrentClimate != nullptr) {
        fSunsetBegin = static_cast<float>(
            static_cast<double>(pCurrentClimate->GetTransTime(TESClimate::SUNSET_BEGIN)) / 6.0
        );
        uiFlags &= ~FLAG_UPDATE_SUNSET_BEGIN;
    }
    return fSunsetBegin;
}

float Sky::GetSunsetEnd() {
    if ((uiFlags & FLAG_UPDATE_SUNSET_END) != 0 && pCurrentClimate != nullptr) {
        fSunsetEnd = static_cast<float>(
            static_cast<double>(pCurrentClimate->GetTransTime(TESClimate::SUNSET_END)) / 6.0
        );
        uiFlags &= ~FLAG_UPDATE_SUNSET_END;
    }
    return fSunsetEnd;
}

void Sky::SetAcceleration(bool abOn) {
    if (abOn && pLastWeather != nullptr && (uiFlags & FLAG_ACCELERATION) == 0) {
        uiFlags |= FLAG_ACCELERATION;
        fAccelBeginPct = fCurrentWeatherPct;
    }
    else if (!abOn || pLastWeather == nullptr) {
        uiFlags &= ~FLAG_ACCELERATION;
        fAccelBeginPct = 0.0f;
    }
}

void Sky::ResetWeather() {
    uiFlags |= FLAG_UPDATE_WEATHER;
    pOverrideWeather = nullptr;
    pLastWeather = nullptr;
    pCurrentWeather = nullptr;
    SetAcceleration(false);
    if (pClouds != nullptr) {
        pClouds->ClearTransTextures();
    }
    if (pPrecip != nullptr) {
        pPrecip->FlushAll();
    }
    UpdateHDRValues();
}

// Constructs and destroys an AutoMemContext on a stack context and ignores the constructor's result.
void Sky::SetCurrentClimate(TESClimate* apClimate, bool abForce) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sky.cpp",
        1464
    );
    if (!apClimate && (!pCurrentClimate || abForce)) {
        apClimate = dynamic_cast<TESClimate*>(TESForm::GetFormByNumericID(0x15F));
    }
    if (apClimate && (pCurrentClimate != apClimate || abForce)) {
        pCurrentClimate = apClimate;
        InvalidateCachedValues();
        ResetWeather();
        uiFlags |= FLAG_CLIMATE_CHANGED;
    }
    pContext->~AutoMemContext();
}

float Sky::GetBeginSunriseColors() {
    if ((uiFlags & FLAG_UPDATE_SUNRISE_COLORS_BEGIN) != 0) {
        double dSunrise = GetSunriseBegin();
        float fExtension = fDaytimeColorExtension.Float();
        float fValue = static_cast<float>(dSunrise - fExtension);
        fSunriseColorBegin = BSMAX(0.0f, fValue);
        uiFlags &= ~FLAG_UPDATE_SUNRISE_COLORS_BEGIN;
    }
    return fSunriseColorBegin;
}

float Sky::GetEndSunsetColors() {
    if ((uiFlags & FLAG_UPDATE_SUNSET_COLORS_END) != 0) {
        double dSunset = GetSunsetEnd();
        float fExtension = fDaytimeColorExtension.Float();
        float fValue = static_cast<float>(fExtension + dSunset);
        fSunsetColorEnd = BSMIN(23.99f, fValue);
        uiFlags &= ~FLAG_UPDATE_SUNSET_COLORS_END;
    }
    return fSunsetColorEnd;
}

void Sky::StopPlayingAllWeatherSounds() {
    for (BSSimpleList<SkySound*>* pNode = pSkySoundList; pNode != nullptr; pNode = pNode->GetNext()) {
        SkySound* pSound = pNode->GetItem();
        if (pSound == nullptr) {
            break;
        }
        pSound->SndHandle.Stop();
    }
}

uint16_t Sky::GetSaveSize() {
    uint16_t usSize = 0;
    usSize += 4;
    usSize += 4;
    usSize += 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x5D) {
        usSize += 4;
    }
    usSize += 4;
    usSize += 4;
    usSize += 4;
    usSize += 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x69) {
        usSize += 4;
        usSize += 4;
    }
    return usSize;
}

void Sky::SaveGame() {
    uint32_t uiCurrentID = 0;
    if (pCurrentWeather) {
        uiCurrentID = pCurrentWeather->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiCurrentID, 4);
    uint32_t uiLastID = 0;
    if (pLastWeather) {
        uiLastID = pLastWeather->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiLastID, 4);
    uint32_t uiDefaultID = 0;
    if (pDefaultWeather) {
        uiDefaultID = pDefaultWeather->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiDefaultID, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x5D) {
        uint32_t uiOverrideID = 0;
        if (pOverrideWeather) {
            uiOverrideID = pOverrideWeather->GetFormID();
        }
        pSaveLoadGameOLD->SaveNumericID(&uiOverrideID, 4);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&fCurrentGameHour, sizeof(fCurrentGameHour));
    pSaveLoadGameOLD->SaveGameDataOLD(&fLastWeatherUpdate, sizeof(fLastWeatherUpdate));
    pSaveLoadGameOLD->SaveGameDataOLD(&fCurrentWeatherPct, sizeof(fCurrentWeatherPct));
    pSaveLoadGameOLD->SaveGameDataOLD(&eMode, sizeof(eMode));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x69) {
        pSaveLoadGameOLD->SaveGameDataOLD(&uiFlags, sizeof(uiFlags));
        pSaveLoadGameOLD->SaveGameDataOLD(&fAccelBeginPct, sizeof(fAccelBeginPct));
    }
}

void Sky::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pCurrentWeather, 0);
    apSaveGameBuffer->SaveFormID(pLastWeather, 0);
    apSaveGameBuffer->SaveFormID(pDefaultWeather, 0);
    apSaveGameBuffer->SaveFormID(pOverrideWeather, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCurrentGameHour), sizeof(fCurrentGameHour), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fLastWeatherUpdate), sizeof(fLastWeatherUpdate), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCurrentWeatherPct), sizeof(fCurrentWeatherPct), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&uiFlags), sizeof(uiFlags), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAccelBeginPct), sizeof(fAccelBeginPct), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&WaterFogColor.r), sizeof(WaterFogColor.r), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&WaterFogColor.g), sizeof(WaterFogColor.g), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&WaterFogColor.b), sizeof(WaterFogColor.b), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fFogHeight), sizeof(fFogHeight), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fFogPower), sizeof(fFogPower), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eMode), sizeof(eMode), 0);
}

void Sky::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    if (pSaveLoadGame->GetPlayerLocationInvalid()) {
        return;
    }
    if (pPrecip) {
        pPrecip->FlushAll();
    }
    if (pClouds) {
        pClouds->ClearTransTextures();
        pClouds->ForceTransTextureUpdate();
    }
    pCurrentWeather = dynamic_cast<TESWeather*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pLastWeather = dynamic_cast<TESWeather*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pDefaultWeather = dynamic_cast<TESWeather*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    pOverrideWeather = dynamic_cast<TESWeather*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCurrentGameHour), sizeof(fCurrentGameHour));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fLastWeatherUpdate), sizeof(fLastWeatherUpdate));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCurrentWeatherPct), sizeof(fCurrentWeatherPct));
    uint32_t uiSavedFlags = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiSavedFlags), sizeof(uiSavedFlags));
    uiFlags = (uiFlags & ~FLAG_ACCELERATION) | (uiSavedFlags & FLAG_ACCELERATION);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAccelBeginPct), sizeof(fAccelBeginPct));
    // Save version 0x12 split the water fog color into its three channels.
    if (apLoadGameBuffer->GetVersion() >= 0x12) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&WaterFogColor.r), sizeof(WaterFogColor.r));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&WaterFogColor.g), sizeof(WaterFogColor.g));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&WaterFogColor.b), sizeof(WaterFogColor.b));
    }
    else {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&WaterFogColor), sizeof(WaterFogColor));
    }
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFogHeight), sizeof(fFogHeight));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fFogPower), sizeof(fFogPower));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eMode), sizeof(eMode));
    Update(0.0f);
    UpdateHDRValues();
}

void Sky::UpdateInterior(float afSecondsPassed) {
    UpdateColors();
    UpdateFog();
    if (pAtmosphere != nullptr) {
        pAtmosphere->Update(this, afSecondsPassed);
    }
    if (pSun != nullptr) {
        pSun->Update(this, afSecondsPassed);
    }
    Renderer::SetBackgroundColor(&SkyColor[1]);
}

void Sky::UnloadAllTextures() {
    if (pClouds != nullptr) {
        pClouds->RemoveTextures();
        pClouds->ClearTransTextures();
    }
    if (pMasser != nullptr) {
        pMasser->RemoveTextures();
    }
    if (pSecunda != nullptr) {
        pSecunda->RemoveTextures();
    }
    if (pSun != nullptr) {
        pSun->RemoveTextures();
    }
    if (pStars != nullptr) {
        pStars->UnloadGeometry();
    }
}

// Creates the four weather image space modifier instances on first use, then sets their modifiers and strengths for
// the current game hour: the current and last weather each blend two times of day by the transition weight.
void Sky::UpdateHDRValues() {
    if (!pCurrentWeatherImageSpaceMod) {
        pCurrentWeatherImageSpaceMod = new ImageSpaceModifierInstanceForm;
        pLastWeatherImageSpaceMod = new ImageSpaceModifierInstanceForm;
        {
            NiPointer<ImageSpaceModifierInstance> spMod(pLastWeatherImageSpaceMod);
            TES::pInstance->GetActiveImageSpaceModifiers()->AddHead(spMod);
        }
        {
            NiPointer<ImageSpaceModifierInstance> spMod(pCurrentWeatherImageSpaceMod);
            TES::pInstance->GetActiveImageSpaceModifiers()->AddHead(spMod);
        }
        pCurrentWeatherImageSpaceMod2 = new ImageSpaceModifierInstanceForm;
        pLastWeatherImageSpaceMod2 = new ImageSpaceModifierInstanceForm;
        {
            NiPointer<ImageSpaceModifierInstance> spMod(pLastWeatherImageSpaceMod2);
            TES::pInstance->GetActiveImageSpaceModifiers()->AddHead(spMod);
        }
        {
            NiPointer<ImageSpaceModifierInstance> spMod(pCurrentWeatherImageSpaceMod2);
            TES::pInstance->GetActiveImageSpaceModifiers()->AddHead(spMod);
        }
        pCurrentWeatherImageSpaceMod->SetPermanent(true);
        pLastWeatherImageSpaceMod->SetPermanent(true);
        pCurrentWeatherImageSpaceMod2->SetPermanent(true);
        pLastWeatherImageSpaceMod2->SetPermanent(true);
        pCurrentWeatherImageSpaceMod->bIsWeather = true;
        pLastWeatherImageSpaceMod->bIsWeather = true;
        pCurrentWeatherImageSpaceMod2->bIsWeather = true;
        pLastWeatherImageSpaceMod2->bIsWeather = true;
    }
    if (eMode == SM_INTERIOR || eMode == SM_NONE) {
        pCurrentWeatherImageSpaceMod->SetStrength(0.0f);
        pCurrentWeatherImageSpaceMod2->SetStrength(0.0f);
        pLastWeatherImageSpaceMod->SetStrength(0.0f);
        pLastWeatherImageSpaceMod2->SetStrength(0.0f);
        return;
    }
    // The second time of day and the first's weight.
    uint32_t uiTime2 = 0;
    bool bBlend = false;
    float fWeight = 1.0f;
    float fBeginSunrise = GetBeginSunriseColors();
    float fSunriseEnd = GetSunriseEnd();
    float fSunsetBegin = GetSunsetBegin();
    float fEndSunset = GetEndSunsetColors();
    uint32_t uiTime1;
    if (fBeginSunrise < fCurrentGameHour && fSunriseEnd > fCurrentGameHour) {
        bBlend = true;
        uiTime1 = TESWeather::SUNRISE;
        // The half width is a float product of the extended difference and the double 0.5.
        float fHalf = (fSunriseEnd - fBeginSunrise) * 0.5;
        float fMid = fBeginSunrise + fHalf;
        if (fMid > fCurrentGameHour) {
            fWeight = 1.0f - (fMid - fCurrentGameHour) / fHalf;
            uiTime2 = TESWeather::NIGHT;
        }
        else {
            fWeight = 1.0f - (fCurrentGameHour - fMid) / fHalf;
            uiTime2 = TESWeather::DAY;
        }
    }
    else if (fSunriseEnd < fCurrentGameHour && fHighNoon > fCurrentGameHour) {
        bBlend = true;
        uiTime1 = TESWeather::DAY;
        uiTime2 = TESWeather::NOON;
        float fSpan = fHighNoon - fSunriseEnd;
        fWeight = 1.0f - (fHighNoon - fCurrentGameHour) / fSpan;
    }
    else if (fCurrentGameHour > fHighNoon && fSunsetBegin > fCurrentGameHour) {
        bBlend = true;
        uiTime1 = TESWeather::NOON;
        uiTime2 = TESWeather::DAY;
        float fSpan = fSunsetBegin - fHighNoon;
        fWeight = 1.0f - (fSunsetBegin - fCurrentGameHour) / fSpan;
    }
    else if (fSunsetBegin < fCurrentGameHour && fEndSunset > fCurrentGameHour) {
        bBlend = true;
        uiTime1 = TESWeather::SUNSET;
        float fHalf = (fEndSunset - fSunsetBegin) * 0.5;
        float fMid = fSunsetBegin + fHalf;
        if (fMid > fCurrentGameHour) {
            fWeight = 1.0f - (fMid - fCurrentGameHour) / fHalf;
            uiTime2 = TESWeather::DAY;
        }
        else {
            fWeight = 1.0f - (fCurrentGameHour - fMid) / fHalf;
            uiTime2 = TESWeather::NIGHT;
        }
    }
    else if (!(fEndSunset <= fCurrentGameHour) && !(fBeginSunrise >= fCurrentGameHour)) {
        // Unordered compares take this branch too.
        Warning("MASTERFILE: Data error detected--Transition times stored in climate data are invalid.");
        uiTime1 = TESWeather::DAY;
    }
    else {
        uiTime1 = TESWeather::NIGHT;
    }

    TESImageSpaceModifier* pImageSpace = nullptr;
    if (pCurrentWeather) {
        pImageSpace = pCurrentWeather->GetFormImageSpaceModifying(uiTime1);
    }
    if (!pImageSpace) {
        pImageSpace = TESImageSpaceModifier::QDefault();
    }
    if (pImageSpace) {
        float fStrength = fCurrentWeatherPct * fWeight;
        pCurrentWeatherImageSpaceMod->SetStrength(fStrength);
        pCurrentWeatherImageSpaceMod->SetImageSpaceModifier(pImageSpace);
    }
    if (bBlend) {
        pImageSpace = nullptr;
        if (pCurrentWeather) {
            pImageSpace = pCurrentWeather->GetFormImageSpaceModifying(uiTime2);
        }
        if (!pImageSpace) {
            pImageSpace = TESImageSpaceModifier::QDefault();
        }
        if (pImageSpace) {
            float fStrength = (1.0f - fWeight) * fCurrentWeatherPct;
            pCurrentWeatherImageSpaceMod2->SetStrength(fStrength);
            pCurrentWeatherImageSpaceMod2->SetImageSpaceModifier(pImageSpace);
        }
    }
    else {
        pCurrentWeatherImageSpaceMod2->SetStrength(0.0f);
    }
    // Compares against the double 1.0.
    if (pLastWeather && fCurrentWeatherPct < 1.0) {
        pImageSpace = pLastWeather->GetFormImageSpaceModifying(uiTime1);
        if (!pImageSpace) {
            pImageSpace = TESImageSpaceModifier::QDefault();
        }
        if (pImageSpace) {
            float fStrength = (1.0f - fCurrentWeatherPct) * fWeight;
            pLastWeatherImageSpaceMod->SetStrength(fStrength);
            pLastWeatherImageSpaceMod->SetImageSpaceModifier(pImageSpace);
        }
        if (bBlend) {
            pImageSpace = pLastWeather->GetFormImageSpaceModifying(uiTime2);
            if (!pImageSpace) {
                pImageSpace = TESImageSpaceModifier::QDefault();
            }
            if (pImageSpace) {
                float fStrength = (1.0f - fCurrentWeatherPct) * (1.0f - fWeight);
                pLastWeatherImageSpaceMod2->SetStrength(fStrength);
                pLastWeatherImageSpaceMod2->SetImageSpaceModifier(pImageSpace);
            }
        }
        else {
            pLastWeatherImageSpaceMod2->SetStrength(0.0f);
        }
    }
    else {
        pLastWeatherImageSpaceMod->SetStrength(0.0f);
        pLastWeatherImageSpaceMod2->SetStrength(0.0f);
    }
}

void Sky::FillColorBlendColors(
    Sky::COLOR_BLEND& aColorBlend,
    TESWeather* apCurrentWeather,
    TESWeather* apLastWeather,
    TESWeather::ColorType aeColorType,
    TESWeather::ColorTime& aeTime1,
    TESWeather::ColorTime& aeTime2
) {
    if (apCurrentWeather != nullptr) {
        aColorBlend.uiRGBVal[0] = apCurrentWeather->GetColor(aeColorType, aeTime1);
        aColorBlend.uiRGBVal[1] = apCurrentWeather->GetColor(aeColorType, aeTime2);
        if (apLastWeather != nullptr) {
            aColorBlend.uiRGBVal[2] = apLastWeather->GetColor(aeColorType, aeTime1);
            aColorBlend.uiRGBVal[3] = apLastWeather->GetColor(aeColorType, aeTime2);
        }
        else {
            aColorBlend.uiRGBVal[2] = 0;
            aColorBlend.uiRGBVal[3] = 0;
        }
    }
}

// The float compares keep the game's NaN behaviour: each test is written so an unordered compare takes the same branch
// the game takes.
void Sky::FillColorBlend(
    COLOR_BLEND& aColorBlend,
    TESWeather* apCurrentWeather,
    float afCurrentWeatherPct,
    TESWeather::ColorTime& aeTime1,
    TESWeather::ColorTime& aeTime2
) {
    if (!apCurrentWeather) {
        return;
    }
    float fSunriseColorsBegin = GetBeginSunriseColors();
    float fSunriseEnd = GetSunriseEnd();
    float fSunsetBegin = GetSunsetBegin();
    float fSunsetColorsEnd = GetEndSunsetColors();
    if (fSunriseColorsBegin < fCurrentGameHour && fSunriseEnd > fCurrentGameHour) {
        aeTime1 = TESWeather::SUNRISE;
        float fHalfSunrise = static_cast<float>((fSunriseEnd - fSunriseColorsBegin) * 0.5);
        float fSunriseMid = fSunriseColorsBegin + fHalfSunrise;
        if (fSunriseMid > fCurrentGameHour) {
            aColorBlend.fBlend[0] = 1.0f - (fSunriseMid - fCurrentGameHour) / fHalfSunrise;
            aeTime2 = TESWeather::NIGHT;
        }
        else {
            aColorBlend.fBlend[0] = 1.0f - (fCurrentGameHour - fSunriseMid) / fHalfSunrise;
            aeTime2 = TESWeather::DAY;
        }
    }
    else if (fSunriseEnd <= fCurrentGameHour && fHighNoon >= fCurrentGameHour) {
        aeTime2 = TESWeather::DAY;
        aeTime1 = TESWeather::NOON;
        float fMorning = fHighNoon - fSunriseEnd;
        aColorBlend.fBlend[0] = 1.0f - (fHighNoon - fCurrentGameHour) / fMorning;
    }
    else if (fCurrentGameHour >= fHighNoon && fSunsetBegin >= fCurrentGameHour) {
        aeTime2 = TESWeather::NOON;
        aeTime1 = TESWeather::DAY;
        float fAfternoon = fSunsetBegin - fHighNoon;
        aColorBlend.fBlend[0] = 1.0f - (fSunsetBegin - fCurrentGameHour) / fAfternoon;
    }
    else if (fSunsetBegin < fCurrentGameHour && fSunsetColorsEnd > fCurrentGameHour) {
        aeTime1 = TESWeather::SUNSET;
        float fHalfSunset = static_cast<float>((fSunsetColorsEnd - fSunsetBegin) * 0.5);
        float fSunsetMid = fSunsetBegin + fHalfSunset;
        if (fSunsetMid > fCurrentGameHour) {
            aColorBlend.fBlend[0] = 1.0f - (fSunsetMid - fCurrentGameHour) / fHalfSunset;
            aeTime2 = TESWeather::DAY;
        }
        else {
            aColorBlend.fBlend[0] = 1.0f - (fCurrentGameHour - fSunsetMid) / fHalfSunset;
            aeTime2 = TESWeather::NIGHT;
        }
    }
    else if (!(fSunsetColorsEnd <= fCurrentGameHour) && !(fSunriseColorsBegin >= fCurrentGameHour)) {
        Warning("MASTERFILE: Data error detected--Transition times stored in climate data are invalid.");
        aeTime2 = TESWeather::DAY;
        aeTime1 = TESWeather::DAY;
        aColorBlend.fBlend[0] = 1.0f;
    }
    else {
        aeTime2 = TESWeather::NIGHT;
        aeTime1 = TESWeather::NIGHT;
        aColorBlend.fBlend[0] = 1.0f;
    }
    aColorBlend.fBlend[1] = 1.0f - aColorBlend.fBlend[0];
    aColorBlend.fBlend[2] = aColorBlend.fBlend[0];
    aColorBlend.fBlend[3] = aColorBlend.fBlend[1];
    aColorBlend.fBlend[0] = aColorBlend.fBlend[0] * fCurrentWeatherPct;
    aColorBlend.fBlend[1] = aColorBlend.fBlend[1] * fCurrentWeatherPct;
    aColorBlend.fBlend[2] = (1.0f - fCurrentWeatherPct) * aColorBlend.fBlend[2];
    aColorBlend.fBlend[3] = (1.0f - fCurrentWeatherPct) * aColorBlend.fBlend[3];
}

// The weather data types 0xC, 0xD and 0xE are the lightning color's red, green and blue (TESWeather::WeatherData).
void Sky::SetColor(NiColor& aColor, COLOR_BLEND* apColorBlend, float afAddFlash) const {
    float fColor = 0.0f;
    for (int32_t r = 0; r < 4; ++r) {
        fColor = static_cast<float>(static_cast<int32_t>(apColorBlend->uiRGBVal[r] & 0xFF)) * apColorBlend->fBlend[r] +
            fColor;
    }
    aColor.r = static_cast<float>(fColor * 0.003921568859368563);
    fColor = 0.0f;
    for (int32_t g = 0; g < 4; ++g) {
        fColor = static_cast<float>(
                     static_cast<int32_t>((static_cast<uint16_t>(apColorBlend->uiRGBVal[g]) >> 8) & 0xFF)
                 ) *
                apColorBlend->fBlend[g] +
            fColor;
    }
    aColor.g = static_cast<float>(fColor * 0.003921568859368563);
    fColor = 0.0f;
    for (int32_t b = 0; b < 4; ++b) {
        fColor = static_cast<float>(static_cast<int32_t>((apColorBlend->uiRGBVal[b] >> 16) & 0xFF)) *
                apColorBlend->fBlend[b] +
            fColor;
    }
    aColor.b = static_cast<float>(fColor * 0.003921568859368563);
    afAddFlash = BSMIN(1.0f, afAddFlash);
    if (afAddFlash > 0.0) {
        aColor += NiColor(afAddFlash, afAddFlash, afAddFlash);
        if (pCurrentWeather) {
            BSClamp(aColor.r, 0.0f, pCurrentWeather->GetWeatherData(TESWeather::WD_LIGHTNING_R, 1.0f, 0.0f));
            BSClamp(aColor.g, 0.0f, pCurrentWeather->GetWeatherData(TESWeather::WD_LIGHTNING_G, 1.0f, 0.0f));
            BSClamp(aColor.b, 0.0f, pCurrentWeather->GetWeatherData(TESWeather::WD_LIGHTNING_B, 1.0f, 0.0f));
            return;
        }
    }
    aColor.Clamp();
}

void Sky::ReloadAllTextures() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sky.cpp",
        2268
    );
    if (pClouds && pCurrentWeather && !pSaveLoadGame->GetSaveGameLoading()) {
        for (int32_t i = 0; i < pClouds->GetNumLayers(); ++i) {
            if (pCurrentWeather->GetCloudTexture(i) && pCurrentWeather->GetCloudTexture(i)->GetTextureNameLength()) {
                BSStringT<char> strFile;
                const TESTexture* pCloudTexture = pCurrentWeather->GetCloudTexture(i);
                const char* pTextureName = pCurrentWeather->GetCloudTexture(i)->GetTextureName();
                strFile.SPrintF("%s%s", pCloudTexture->GetDefaultPath(), pTextureName);
                if (FileFinder::Locate(strFile.pString, nullptr, 0, 0xFFFFFFFFu)) {
                    NiGeometry* pCloudsGeo = pClouds->GetCloudsGeo(i);
                    if (pCloudsGeo->GetProperty(5)) {
                        NiPointer<NiTexture> spTexture(static_cast<NiTexture*>(nullptr));
                        BSShaderManager::GetTexture(strFile.pString, true, spTexture, true, false);
                        NiSourceTexture* pSourceTexture = static_cast<NiSourceTexture*>(spTexture.m_pObject);
                        pCloudsGeo = pClouds->GetCloudsGeo(i);
                        SkyShaderProperty* pProp = static_cast<SkyShaderProperty*>(pCloudsGeo->GetProperty(3));
                        pProp->SetTexture(pSourceTexture);
                    }
                }
            }
        }
    }

    if (pMasser) {
        SkyShaderProperty* pProp = static_cast<SkyShaderProperty*>(pMasser->spShadowMesh->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture("Textures\\Sky\\MoonShadow.dds", true, spTex, true, false);
        NiTexture* pTex = spTex.m_pObject;
        if (pTex) {
            pProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
        }
        pMasser->spShadowMesh->SetDisplayObject(pTex != nullptr);
        pMasser->eUpdateMoonTexture = Moon::US_INITIALIZE;
    }
    if (pSecunda) {
        SkyShaderProperty* pProp = static_cast<SkyShaderProperty*>(pSecunda->spShadowMesh->GetProperty(3));
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        BSShaderManager::GetTexture("Textures\\Sky\\MoonShadow.dds", true, spTex, true, false);
        NiTexture* pTex = spTex.m_pObject;
        if (pTex) {
            pProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
        }
        pSecunda->spShadowMesh->SetDisplayObject(pTex != nullptr);
        pSecunda->eUpdateMoonTexture = Moon::US_INITIALIZE;
    }
    if (pSun && pCurrentClimate) {
        BSStringT<char> strfilename;
        SkyShaderProperty* pProp = static_cast<SkyShaderProperty*>(pSun->GetBase()->GetProperty(3));
        NiTexture* pTex = nullptr;
        NiPointer<NiTexture> spTex(static_cast<NiTexture*>(nullptr));
        const TESTexture* pTESTexture = pCurrentClimate->GetTexture(TESClimate::TX_SUN);
        if (pTESTexture && pTESTexture->GetTextureNameLength()) {
            strfilename = pTESTexture->GetDefaultPath();
            strfilename += pTESTexture->GetTextureName();
            BSShaderManager::GetTexture(strfilename.pString, true, spTex, true, false);
            pTex = spTex.m_pObject;
            if (pTex) {
                pProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
            }
        }

        pSun->GetSunBaseNode()->SetDisplayObject(pTex != nullptr);
        NiAVObject* pSunGlare = pSun->GetSunGlareGeo();
        SkyShaderProperty* pGlareProp = static_cast<SkyShaderProperty*>(pSunGlare->GetProperty(3));
        pTESTexture = pCurrentClimate->GetTexture(TESClimate::TX_SUN);
        if (pTESTexture && pTESTexture->GetTextureNameLength()) {
            if (BSShaderManager::bHDR || BSstricmp(pTESTexture->GetTextureName(), "Sky\\SunGlare.dds")) {
                strfilename = pTESTexture->GetDefaultPath();
                strfilename += pTESTexture->GetTextureName();
            }
            else {
                strfilename = "Textures\\Sky\\SunGlareNonHDR.dds";
            }
            NiPointer<NiTexture> spGlareTex(static_cast<NiTexture*>(nullptr));
            BSShaderManager::GetTexture(strfilename.pString, true, spGlareTex, true, false);
            pTex = spGlareTex.m_pObject;
            if (pTex) {
                pGlareProp->SetTexture(static_cast<NiSourceTexture*>(pTex));
            }
        }

        pSun->GetSunGlareNode()->SetDisplayObject(pTex != nullptr);
    }

    if (pStars && pCurrentClimate) {
        const char* pNightSkyModel = pCurrentClimate->GetNightSkyModel();
        pStars->LoadGeometry(pNightSkyModel);
    }
}

NiFogProperty* Sky::GetFog() {
    if (pAtmosphere != nullptr) {
        return pAtmosphere->GetFog();
    }
    return nullptr;
}

NiDirectionalLight* Sky::GetLight() {
    if (pSun != nullptr) {
        return pSun->GetSunLight();
    }
    return nullptr;
}

// Replaces the root (detaching the old one from its parent), then rebuilds every sky object under it.
void Sky::Initialize(BSMultiBoundNode* apRoot, BSFogProperty* apFog) {
    // The context lives in raw stack storage and is constructed and destroyed explicitly.
    uint8_t kContextStorage[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextStorage) AutoMemContext(
        MEM_CONTEXT_SKY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sky.cpp",
        338
    );
    if (spRoot != nullptr) {
        if (spRoot->GetParent()) {
            NiNode* pParent = spRoot->GetParent();
            pParent->DetachChild(spRoot);
        }
        spRoot = nullptr;
    }
    if (apRoot) {
        spRoot = apRoot;
    }
    else {
        BSMultiBoundNode* pNode = new BSMultiBoundNode;
        spRoot = pNode;
        NiSmartPointerCast<BSMultiBoundNode>(spRoot)->SetCullingMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        spRoot->SetSelectiveUpdate(true);
    }
    spRoot->SetTranslate(NiPoint3::ZERO);

    if (pAtmosphere) {
        delete pAtmosphere;
    }
    pAtmosphere = new Atmosphere;
    NiNode* pAtmosphereRoot = spRoot;
    pAtmosphere->Initialize(pAtmosphereRoot, apFog);

    if (pStars) {
        delete pStars;
    }
    pStars = new Stars;
    NiNode* pStarsRoot = spRoot;
    pStars->Initialize(pStarsRoot);

    if (pSun) {
        delete pSun;
    }
    pSun = new Sun;
    NiNode* pSunRoot = spRoot;
    pSun->Initialize(pSunRoot);

    NiNode* pMoonsRoot = new NiNode(0);
    spMoonsRoot = pMoonsRoot;
    NiNode* pRoot = spRoot;
    pRoot->AttachChild(spMoonsRoot, true);

    if (pPrecip) {
        delete pPrecip;
    }
    if (bPrecipitation.Bool()) {
        Precipitation* pNewPrecip = new Precipitation;
        pPrecip = pNewPrecip;
        pNewPrecip->Initialize();
    }
    else {
        pPrecip = nullptr;
    }

    if (pClouds) {
        delete pClouds;
    }
    pClouds = new Clouds;
    NiNode* pCloudsRoot = spRoot;
    pClouds->Initialize(pCloudsRoot);

    NiAlphaProperty* pNewAlpha = new NiAlphaProperty;
    {
        NiPointer<NiAlphaProperty> spAlpha(pNewAlpha);
        spAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        spAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_INVSRCALPHA);
        spAlpha->SetAlphaBlending(true);
        spAlpha->SetNoSorter(true);
        NiAlphaProperty* pAlpha = spAlpha;
        spRoot->AttachProperty(pAlpha);
        spAlpha = nullptr;
    }

    if (BSShaderManager::bHDR) {
        SkyShader::pLUMRamp[0] = 1.0f;
        SkyShader::pLUMRamp[1] = 1.0f;
    }
    spRoot->UpdateProperties();
    pContext->~AutoMemContext();
}

Sun* Sky::GetSun() {
    return pSun;
}

bool Sky::IsUnderwater() const {
    return (uiFlags & FLAG_UNDERWATER) != 0;
}

void Sky::SetFastTravel(bool abOn) {
    uint32_t uiNewFlags;
    if (abOn) {
        pOverrideWeather = nullptr;
        uiNewFlags = uiFlags | FLAG_FAST_TRAVEL;
    }
    else {
        uiNewFlags = uiFlags & ~FLAG_FAST_TRAVEL;
    }
    uiFlags = uiNewFlags;
}

void Sky::ActivateWeatherSounds(const TESWeather* apWeather) {
    if (!apWeather) {
        return;
    }
    for (const BSSimpleList<TESWeather::WeatherSound*>* pSoundsToAdd = apWeather->GetSoundList(); pSoundsToAdd;
         pSoundsToAdd = pSoundsToAdd->m_pkNext) {
        TESWeather::WeatherSound* pSoundData = pSoundsToAdd->m_item;
        if (!pSoundData) {
            break;
        }
        BSSoundHandle kSoundHand;
        if (bPrecipitation.Bool() || pSoundData->eType != TESWeather::WST_PRECIP) {
            uint32_t uiSoundFlags = BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND;
            if (pSoundData->eType != TESWeather::WST_THUNDER) {
                uiSoundFlags |= BSGameSound::SOUND_FLAG_LOOP;
            }
            kSoundHand = BSAudio::QInstance().GetSoundHandleByNumericID(pSoundData->uiSoundFormID, uiSoundFlags);
        }
        bool bDup = false;
        for (BSSimpleList<SkySound*>* pListNode = this->pSkySoundList; !bDup; pListNode = pListNode->m_pkNext) {
            if (!pListNode) {
                break;
            }
            SkySound* pSound = pListNode->m_item;
            if (!pSound) {
                break;
            }
            bDup = pSound->uiFormID == pSoundData->uiSoundFormID &&
                pSound->pWeather == apWeather &&
                pSound->eSoundType == pSoundData->eType;
        }
        if (bDup) {
            kSoundHand.Release();
        }
        else {
            SkySound* pNewSound = new SkySound(kSoundHand, apWeather, pSoundData->eType, pSoundData->uiSoundFormID);

            this->pSkySoundList->AddHead(pNewSound);
            if (pSoundData->eType == TESWeather::WST_THUNDER) {
                ++SkySound::ucThunderCount;
            }
        }
    }
}

void Sky::Destroy() {
    if (Sky::pInstance) {
        delete Sky::pInstance;
    }
    Sky::pInstance = nullptr;
}

// Whether the sound list, up to its first empty item, holds a sound of form aiFormID for apWeather.
bool Sky::FindSkySound(uint32_t aiFormID, const TESWeather* apWeather) const {
    for (SkySound*& rpItem : pSkySoundList->ItemsUntilNull()) {
        if (rpItem->uiFormID == aiFormID && rpItem->pWeather == apWeather) {
            return true;
        }
    }
    return false;
}

void Sky::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void ImageSpaceModifierInstanceForm::SetImageSpaceModifier(TESImageSpaceModifier* apMod) {
    pImageSpaceMod = apMod;
}

const TESTexture* TESClimate::GetTexture(TextureType aeTxType) {
    if (aeTxType >= NUM_TEXTURES) {
        return nullptr;
    }
    return &txSkyObjects[aeTxType];
}

const char* TESClimate::GetNightSkyModel() {
    return mNightSky.GetModel();
}

float TESWaterSystem::GetCameraRelevantWaterHeight() {
    return fCameraRelevantWaterHeight;
}

float TESWeather::GetFogData(FogData aType) const {
    return fFogData[aType];
}

uint8_t TESWeather::GetWeatherDataRaw(WeatherData aeType) const {
    return ucWeatherData[aeType];
}
