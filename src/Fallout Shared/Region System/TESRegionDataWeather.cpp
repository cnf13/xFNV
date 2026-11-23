#include "Fallout Shared/Region System/TESRegionDataWeather.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWeatherList.h"

#include <cmath>

TESRegionDataWeather::TESRegionDataWeather() : TESRegionData() {}

TESRegionDataWeather::TESRegionDataWeather(TESRegionDataWeather* apSource) : TESRegionData(apSource) {
    weatherList.CopyComponent(&apSource->weatherList, false);
}

TESRegionDataWeather::~TESRegionDataWeather() {}

void TESRegionDataWeather::Save() {
    TESRegionData::Save();
    weatherList.Save(RDWT_ID);
}

REGION_DATA_ID TESRegionDataWeather::GetID() const {
    return REGION_DATA_WEATHER_ID;
}

TESRegionData* TESRegionDataWeather::Copy() {
    return new TESRegionDataWeather(this);
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataWeather::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataWeather::BlendInto(const TESRegionData* apRegionData, uint32_t aiTotalBlending) {
    // The blend source is a weather data entry; GetID is checked first.
    const TESRegionDataWeather* pWeather = static_cast<const TESRegionDataWeather*>(apRegionData);
    if (apRegionData && apRegionData->GetID() == REGION_DATA_WEATHER_ID && aiTotalBlending) {
        if (!bIgnore) {
            if (apRegionData->bIgnore) {
                return;
            }
            if (bOverride) {
                if (apRegionData->bOverride) {
                    int32_t iPriority = apRegionData->cPriority;
                    if (iPriority > cPriority) {
                        SetOverride(apRegionData->bOverride);
                        SetPriority(apRegionData->cPriority);
                        weatherList.CopyComponent(&pWeather->weatherList, false);
                    }
                }
                return;
            }
            if (!apRegionData->bOverride) {
                weatherList.CopyComponent(&pWeather->weatherList, true);
                float fSelfWeighted = static_cast<float>(
                    apRegionData->cPriority * (100 - cPriority) + cPriority * cPriority
                );
                float fBlendedPriority = static_cast<double>(
                                             apRegionData->cPriority * apRegionData->cPriority +
                                             cPriority * (100 - apRegionData->cPriority)
                                         ) +
                    fSelfWeighted;
                float fNewPriority = fBlendedPriority * 0.004999999888241291;
                SetPriority(static_cast<int32_t>(std::floor(fNewPriority)));
                return;
            }
        }
        SetOverride(apRegionData->bOverride);
        SetPriority(apRegionData->cPriority);
        weatherList.CopyComponent(&pWeather->weatherList, false);
    }
}

bool TESRegionDataWeather::Validate() {
    return !weatherList.IsEmpty();
}

// Loads the weather list chunk with warnings suppressed, then warns once if any were raised.
void TESRegionDataWeather::LoadRegionDataWeather(TESFile* apFile, TESRegion* apRegion) {
    // The counts are compared signed.
    int32_t iWarnings = MessageHandler::QWarningCount();
    MessageHandler::IncDisableWarningCount(true);
    TESWeatherList::LoadWeatherListChunk(&weatherList, apFile, apRegion);
    MessageHandler::IncDisableWarningCount(false);
    if (MessageHandler::QWarningCount() > iWarnings && apRegion) {
        Warning(
            "Warnings were encountered while loading weather list chunk from region %s",
            apRegion->GetFormEditorID()
        );
    }
}

TESWeather* TESRegionDataWeather::GetRandomWeather() {
    return weatherList.GetRandomWeather();
}
