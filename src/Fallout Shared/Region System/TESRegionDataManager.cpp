#include "Fallout Shared/Region System/TESRegionDataManager.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataGrass.h"
#include "Fallout Shared/Region System/TESRegionDataImposter.h"
#include "Fallout Shared/Region System/TESRegionDataLandscape.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataMap.h"
#include "Fallout Shared/Region System/TESRegionDataObjects.h"
#include "Fallout Shared/Region System/TESRegionDataSound.h"
#include "Fallout Shared/Region System/TESRegionDataWeather.h"
#include "Fallout Shared/TESFile.h"

TESRegionDataManager::TESRegionDataManager() : pLastLoadedRegion(nullptr) {}

TESRegionData* TESRegionDataManager::ConstructRegionData(REGION_DATA_ID aType) {
    TESRegionData* pData = nullptr;
    switch (aType) {
        case REGION_DATA_WEATHER_ID: {
            pData = new TESRegionDataWeather;
            break;
        }
        case REGION_DATA_MAP_ID: {
            pData = new TESRegionDataMap;
            break;
        }
        case REGION_DATA_LANDSCAPE_ID: {
            pData = new TESRegionDataLandscape;
            break;
        }
        case REGION_DATA_GRASS_ID: {
            pData = new TESRegionDataGrass;
            break;
        }
        case REGION_DATA_SOUND_ID: {
            pData = new TESRegionDataSound;
            break;
        }
        case REGION_DATA_IMPOSTER_ID: {
            pData = new TESRegionDataImposter;
            break;
        }
        default: break;
    }
    return pData;
}

TESRegionDataObjects* TESRegionDataManager::IsRegionDataObjects(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_OBJECTS_ID) {
        return static_cast<TESRegionDataObjects*>(apData);
    }
    return nullptr;
}

TESRegionDataWeather* TESRegionDataManager::IsRegionDataWeather(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_WEATHER_ID) {
        return static_cast<TESRegionDataWeather*>(apData);
    }
    return nullptr;
}

TESRegionDataMap* TESRegionDataManager::IsRegionDataMap(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_MAP_ID) {
        return static_cast<TESRegionDataMap*>(apData);
    }
    return nullptr;
}

TESRegionDataLandscape* TESRegionDataManager::IsRegionDataLandscape(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_LANDSCAPE_ID) {
        return static_cast<TESRegionDataLandscape*>(apData);
    }
    return nullptr;
}

TESRegionDataGrass* TESRegionDataManager::IsRegionDataGrass(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_GRASS_ID) {
        return static_cast<TESRegionDataGrass*>(apData);
    }
    return nullptr;
}

TESRegionDataSound* TESRegionDataManager::IsRegionDataSound(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_SOUND_ID) {
        return static_cast<TESRegionDataSound*>(apData);
    }
    return nullptr;
}

TESRegionDataImposter* TESRegionDataManager::IsRegionDataImposter(TESRegionData* apData) {
    if (apData && apData->GetID() == REGION_DATA_IMPOSTER_ID) {
        return static_cast<TESRegionDataImposter*>(apData);
    }
    return nullptr;
}

bool TESRegionDataManager::LoadRegionData(TESFile* apFile, TESRegion* apRegion) {
    REGION_DATA_BASE data;
    if (!apFile || !apRegion) {
        return false;
    }
    pLastLoadedRegion = apRegion;
    CHUNK_ID eChunk = apFile->GetTESChunk();
    if (eChunk > RDLN_ID) {
        if (eChunk > RDGS_ID) {
            if (eChunk == RDAT_ID) {
                ourMemset(&data, 0, sizeof(data));
                apFile->GetChunkData(&data, 8u);
                if (apFile->QEndian()) {
                    data.Endian();
                }
                pRegData = ConstructRegionData(data.iDataTypeID);
                if (pRegData) {
                    pRegData->LoadRegionData(&data);
                    apRegion->pDataList->AddHead(pRegData);
                }
            }
            else if (eChunk == RDWT_ID) {
                TESRegionDataWeather* pRegDataWea = IsRegionDataWeather(pRegData);
                if (pRegDataWea) {
                    pRegDataWea->LoadRegionDataWeather(apFile, apRegion);
                }
            }
            return true;
        }
        if (eChunk == RDGS_ID) {
            TESRegionDataGrass* pRegDataGrass = IsRegionDataGrass(pRegData);
            if (pRegDataGrass) {
                pRegDataGrass->LoadRegionDataGrass(apFile);
            }
            return true;
        }
        if (eChunk != ICON_ID) {
            if (eChunk == RDMP_ID) {
                TESRegionDataMap* pRegDataMap = IsRegionDataMap(pRegData);
                if (pRegDataMap) {
                    pRegDataMap->LoadRegionDataMap(apFile);
                }
            }
            return true;
        }
    }
    else if (eChunk != RDLN_ID) {
        if (eChunk > RDMD_ID) {
            if (eChunk != RDSD_ID && eChunk != RDSI_ID) {
                return true;
            }
        }
        else if (eChunk != RDMD_ID && eChunk != RDSB_ID) {
            if (eChunk == RDID_ID) {
                TESRegionDataImposter* pRegDataImposter = IsRegionDataImposter(pRegData);
                if (pRegDataImposter) {
                    pRegDataImposter->LoadRegionDataImposter(apFile);
                }
            }
            return true;
        }
        TESRegionDataSound* pRegDataSound = IsRegionDataSound(pRegData);
        if (pRegDataSound) {
            pRegDataSound->LoadRegionDataSound(apFile);
        }
        return true;
    }
    TESRegionDataLandscape* pRegDataLandscape = IsRegionDataLandscape(pRegData);
    if (pRegDataLandscape) {
        pRegDataLandscape->LoadRegionDataLandscape(apFile);
    }
    return true;
}
