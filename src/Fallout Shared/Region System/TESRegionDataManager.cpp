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

// The exe's cleanup-only C++ EH frame is left out.
TESRegionData* TESRegionDataManager::ConstructRegionData(REGION_DATA_ID aType) {
	TESRegionData* v15 = 0;
	switch (aType) {
	case REGION_DATA_WEATHER_ID: {
		TESRegionDataWeather* v8;
		TESRegionDataWeather* apMemory = static_cast<TESRegionDataWeather*>(ourMalloc(sizeof(TESRegionDataWeather)));
		if (apMemory) {
			v8 = ::new (apMemory) TESRegionDataWeather;
		} else {
			v8 = 0;
		}
		v15 = v8;
		break;
	}
	case REGION_DATA_MAP_ID: {
		TESRegionDataMap* TESRegionDataMap_;
		TESRegionDataMap* v13 = static_cast<TESRegionDataMap*>(ourMalloc(sizeof(TESRegionDataMap)));
		if (v13) {
			TESRegionDataMap_ = ::new (v13) TESRegionDataMap;
		} else {
			TESRegionDataMap_ = 0;
		}
		v15 = TESRegionDataMap_;
		break;
	}
	case REGION_DATA_LANDSCAPE_ID: {
		TESRegionDataLandscape* TESRegionDataLandscape_;
		TESRegionDataLandscape* v12 = static_cast<TESRegionDataLandscape*>(ourMalloc(sizeof(TESRegionDataLandscape)));
		if (v12) {
			TESRegionDataLandscape_ = ::new (v12) TESRegionDataLandscape;
		} else {
			TESRegionDataLandscape_ = 0;
		}
		v15 = TESRegionDataLandscape_;
		break;
	}
	case REGION_DATA_GRASS_ID: {
		TESRegionDataGrass* TESRegionDataGrass_;
		TESRegionDataGrass* v11 = static_cast<TESRegionDataGrass*>(ourMalloc(sizeof(TESRegionDataGrass)));
		if (v11) {
			TESRegionDataGrass_ = ::new (v11) TESRegionDataGrass;
		} else {
			TESRegionDataGrass_ = 0;
		}
		v15 = TESRegionDataGrass_;
		break;
	}
	case REGION_DATA_SOUND_ID: {
		TESRegionDataSound* TESRegionDataSound_;
		TESRegionDataSound* v10 = static_cast<TESRegionDataSound*>(ourMalloc(sizeof(TESRegionDataSound)));
		if (v10) {
			TESRegionDataSound_ = ::new (v10) TESRegionDataSound;
		} else {
			TESRegionDataSound_ = 0;
		}
		v15 = TESRegionDataSound_;
		break;
	}
	case REGION_DATA_IMPOSTER_ID: {
		TESRegionDataImposter* TESRegionDataImposter_;
		TESRegionDataImposter* v9 = static_cast<TESRegionDataImposter*>(ourMalloc(sizeof(TESRegionDataImposter)));
		if (v9) {
			TESRegionDataImposter_ = ::new (v9) TESRegionDataImposter;
		} else {
			TESRegionDataImposter_ = 0;
		}
		v15 = TESRegionDataImposter_;
		break;
	}
	default: {
		return v15;
	}
	}
	return v15;
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
		return 0;
	}
	pLastLoadedRegion = apRegion;
	CHUNK_ID TESChunk = apFile->GetTESChunk();
	if (TESChunk > RDLN_ID) {
		if (TESChunk > RDGS_ID) {
			if (TESChunk == RDAT_ID) {
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
			} else {
				if (TESChunk == RDWT_ID) {
					TESRegionDataWeather* pRegDataWea = IsRegionDataWeather(pRegData);
					if (pRegDataWea) {
						pRegDataWea->LoadRegionDataWeather(apFile, apRegion);
					}
				}
			}
			return 1;
		}
		if (TESChunk == RDGS_ID) {
			TESRegionDataGrass* pRegDataGrass = IsRegionDataGrass(pRegData);
			if (pRegDataGrass) {
				pRegDataGrass->LoadRegionDataGrass(apFile);
			}
			return 1;
		}
		if (TESChunk != ICON_ID) {
			if (TESChunk == RDMP_ID) {
				TESRegionDataMap* pRegDataMap = IsRegionDataMap(pRegData);
				if (pRegDataMap) {
					pRegDataMap->LoadRegionDataMap(apFile);
				}
			}
			return 1;
		}
		goto LABEL_39;
	}
	if (TESChunk == RDLN_ID) {
	LABEL_39:;
		TESRegionDataLandscape* pRegDataLandscape = IsRegionDataLandscape(pRegData);
		if (pRegDataLandscape) {
			pRegDataLandscape->LoadRegionDataLandscape(apFile);
		}
		return 1;
	}
	if (TESChunk > RDMD_ID) {
		if (TESChunk != RDSD_ID && TESChunk != RDSI_ID) {
			return 1;
		}
		goto LABEL_36;
	}
	if (TESChunk == RDMD_ID || TESChunk == RDSB_ID) {
	LABEL_36:;
		TESRegionDataSound* pRegDataSound = IsRegionDataSound(pRegData);
		if (pRegDataSound) {
			pRegDataSound->LoadRegionDataSound(apFile);
		}
		return 1;
	}
	if (TESChunk == RDID_ID) {
		TESRegionDataImposter* pRegDataImposter = IsRegionDataImposter(pRegData);
		if (pRegDataImposter) {
			pRegDataImposter->LoadRegionDataImposter(apFile);
		}
	}
	return 1;
}
