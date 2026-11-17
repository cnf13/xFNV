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
    void* v4 = ourMalloc(sizeof(TESRegionDataWeather));
    if (v4) {
        return ::new (v4) TESRegionDataWeather(this);
    }
    else {
        return nullptr;
    }
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataWeather::Blend(const TESRegionData* apRegionData) {
    TESRegionData* pCopy = Copy();
    pCopy->BlendInto(apRegionData, 2u);
    return pCopy;
}

void TESRegionDataWeather::BlendInto(const TESRegionData* a1, uint32_t a2) {
    // The blend source is a weather data entry; GetID is checked first.
    const TESRegionDataWeather* pWeather = static_cast<const TESRegionDataWeather*>(a1);
    if (a1 && (a1->GetID() == REGION_DATA_WEATHER_ID && a2)) {
        if (!bIgnore) {
            if (a1->bIgnore) {
                return;
            }
            if (bOverride) {
                if (a1->bOverride) {
                    int32_t iPriority = a1->cPriority;
                    if (iPriority > cPriority) {
                        SetOverride(a1->bOverride);
                        uint8_t v5 = a1->cPriority;
                        SetPriority(v5);
                        weatherList.CopyComponent(&pWeather->weatherList, 0);
                    }
                }
                return;
            }
            if (!a1->bOverride) {
                weatherList.CopyComponent(&pWeather->weatherList, 1);
                int32_t v6 = cPriority;
                int32_t v7 = cPriority * v6;
                int32_t v8 = 100 - cPriority;
                float v14 = static_cast<float>(a1->cPriority * v8 + v7);
                int32_t v9 = 100 - a1->cPriority;
                int32_t v10 = cPriority * v9;
                int32_t v11 = a1->cPriority;
                float v15 = static_cast<double>(a1->cPriority * v11 + v10) + v14;
                float fnewPriority = v15 * 0.004999999888241291;
                int32_t v12 = static_cast<int32_t>(std::floor(fnewPriority));
                SetPriority(v12);
                return;
            }
        }
        SetOverride(a1->bOverride);
        uint8_t v3 = a1->cPriority;
        SetPriority(v3);
        weatherList.CopyComponent(&pWeather->weatherList, 0);
    }
}

bool TESRegionDataWeather::Validate() {
    return (bool)((uint8_t)(!(&weatherList)->BSSimpleList<WeatherEntry*>::IsEmpty()));
}

// Loads the weather list chunk with warnings suppressed, then warns once if any were raised.
void TESRegionDataWeather::LoadRegionDataWeather(TESFile* apFile, TESRegion* apRegion) {
    // The counts are compared signed.
    int32_t iWarnings = static_cast<int32_t>(MessageHandler::QWarningCount());
    MessageHandler::IncDisableWarningCount(true);
    TESWeatherList::LoadWeatherListChunk(&weatherList, apFile, apRegion);
    MessageHandler::IncDisableWarningCount(false);
    if (static_cast<int32_t>(MessageHandler::QWarningCount()) > iWarnings && apRegion) {
        Warning(
            "Warnings were encountered while loading weather list chunk from region %s", apRegion->GetFormEditorID()
        );
    }
}

TESWeather* TESRegionDataWeather::GetRandomWeather() {
    return (&weatherList)->GetRandomWeather();
}
