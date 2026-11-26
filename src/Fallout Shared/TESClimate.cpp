#include "Fallout Shared/TESClimate.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWeatherList.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

TESClimate::TESClimate() {
    cFormType = CLMT_ID;
    TESClimate::InitializeData();
}

TESClimate::~TESClimate() {}

void TESClimate::InitializeData() {
    ourMemset(ucData, 0, sizeof(ucData));
    SetMoonPhaseDays(3);
}

// An empty EDID chunk leaves the buffer unwritten (malformed data), so one extra byte is reserved and terminated.
bool TESClimate::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 54) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk > PNAM_ID) {
            switch (uiChunk) {
                case WLST_ID: {
                    int32_t iWarningCount = MessageHandler::QWarningCount();
                    MessageHandler::IncDisableWarningCount(true);
                    TESWeatherList::LoadWeatherListChunk(&weatherList, apFile, this);
                    MessageHandler::IncDisableWarningCount(false);
                    if (MessageHandler::QWarningCount() > iWarningCount) {
                        Warning(
                            "MASTERFILE: Warnings were encountered while loading weather list chunk from climate %s",
                            GetFormEditorID()
                        );
                    }
                    break;
                }
                case TNAM_ID: apFile->GetChunkData(ucData, 6); break;
                default: break;
            }
        }
        else if (uiChunk >= FNAM_ID) {
            // The two sky object textures: txSkyObjects is indexed with the chunk's offset from FNAM, and an index past
            // the array is skipped.
            int32_t iIndex = static_cast<int32_t>(uiChunk - FNAM_ID);
            if (iIndex < NUM_TEXTURES) {
                TESTexture::LoadTextureChunk(&txSkyObjects[iIndex], apFile);
            }
        }
        else {
            switch (uiChunk) {
                case EDID_ID: {
                    // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                    // An empty chunk leaves the buffer unwritten, so it is cleared first.
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0x200);
                    SetFormEditorID(pBuffer);
                    break;
                }
                case OBND_ID: LoadObjectBound(apFile); break;
                case MODL_ID: TESModel::LoadModelChunk(&mNightSky, apFile); break;
                default: break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESClimate::Save() {
    StartForm();
    weatherList.Save(WLST_ID);
    for (int32_t i = 0; i < NUM_TEXTURES; ++i) {
        txSkyObjects[i].Save(static_cast<CHUNK_ID>(i + FNAM_ID));
    }
    mNightSky.Save(MODL_ID, MODT_ID);
    AddChunkArray(TNAM_ID, reinterpret_cast<const char*>(ucData), sizeof(ucData));
    CloseForm();
}

void TESClimate::Copy(TESForm* apCopy) {
    TESClimate* pCopy = dynamic_cast<TESClimate*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        weatherList.CopyComponent(&pCopy->weatherList, false);
        for (int32_t i = 0; i < 2; ++i) {
            txSkyObjects[i].CopyComponent(&pCopy->txSkyObjects[i]);
        }
        mNightSky.CopyComponent(&pCopy->mNightSky);
        ourMemcpy(ucData, pCopy->ucData, sizeof(ucData));
    }
}

bool TESClimate::Compare(TESForm* apCompare) {
    TESClimate* pComp = dynamic_cast<TESClimate*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    for (int32_t i = 0; i < 2; ++i) {
        if (txSkyObjects[i].CompareComponent(&pComp->txSkyObjects[i])) {
            return true;
        }
    }
    return mNightSky.CompareComponent(&pComp->mNightSky) ||
        weatherList.CompareComponent(&pComp->weatherList) ||
        std::memcmp(ucData, pComp->ucData, sizeof(ucData)) != 0;
}

// Clamps to ~0xC0 (-193, signed), not to the 0x3F mask: for any count of -193 or more the low six
// bits become 0x3F (the low byte of -193).
void TESClimate::SetMoonPhaseDays(uint32_t auiDays) {
    int32_t iFlags = ucData[MOON_DATA] & 0xC0;
    ucData[MOON_DATA] = static_cast<uint8_t>(iFlags | BSMIN(static_cast<int32_t>(auiDays), ~0xC0));
}

void TESClimate::MakeDefault() {
    txSkyObjects->SetTextureName("Sky\\Sun.dds");
    txSkyObjects[TESClimate::TX_GLARE].SetTextureName("Sky\\SunGlare.dds");
    mNightSky.SetModel("Sky\\Stars.nif");
    ucData[TESClimate::SUNRISE_BEGIN] = 36;
    ucData[TESClimate::SUNRISE_END] = 42;
    ucData[TESClimate::SUNSET_BEGIN] = 108;
    ucData[TESClimate::SUNSET_END] = 114;
    ucData[TESClimate::VOLATILITY] = 0;
    SetUsesMasser(true);
    SetUsesSecunda(true);
    SetMoonPhaseDays(3u);
    weatherList.RemoveAll();
    WeatherEntry* pEntry = new WeatherEntry;
    TESForm* pForm = TESForm::GetFormByNumericID(350u);
    pEntry->pWeather = dynamic_cast<TESWeather*>(pForm);
    pEntry->uiChance = 100;
    if (pEntry->pWeather) {
        weatherList.AddHead(pEntry);
    }
    else {
        ourFree(pEntry);
        Warning("DEFAULT: Unable to add default weather to default climate.  ( TESClimate::MakeDefault() )");
    }
}

uint8_t TESClimate::GetVolatilityIndex() const {
    return ucData[VOLATILITY];
}

void TESClimate::SetUsesMasser(bool abUses) {
    if (abUses) {
        ucData[MOON_DATA] |= MOON_MASSER;
    }
    else {
        ucData[MOON_DATA] &= ~MOON_MASSER;
    }
}

void TESClimate::SetUsesSecunda(bool abUses) {
    if (abUses) {
        ucData[MOON_DATA] |= MOON_SECUNDA;
    }
    else {
        ucData[MOON_DATA] &= ~MOON_SECUNDA;
    }
}

uint32_t TESClimate::GetMoonPhaseDays() const {
    return ucData[MOON_DATA] & 0x3F;
}

bool TESClimate::UsesSecunda() const {
    return (ucData[MOON_DATA] & MOON_SECUNDA) != 0;
}

TESWeather* TESClimate::GetRandomWeather() {
    return weatherList.GetRandomWeather();
}

bool TESClimate::UsesMasser() const {
    return (ucData[MOON_DATA] & MOON_MASSER) != 0;
}
