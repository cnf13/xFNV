#include "Fallout Shared/TESClimate.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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

TESClimate::TESClimate() {
	cFormType = CLMT_ID;
	TESClimate::InitializeData();
}

TESClimate::~TESClimate() {}

void TESClimate::InitializeData() {
	ourMemset(ucData, 0, sizeof(ucData));
	SetMoonPhaseDays(3);
}

// An empty EDID chunk leaves the alloca'd buffer unwritten (malformed data), so src reserves one byte and terminates
// it. The strict check reports a DIFF on that buffer only: the exe's sits at a different place in its frame, so the
// bytes GetChunkData (0x472890) is handed differ (fix queue).
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
					Warning("MASTERFILE: Warnings were encountered while loading weather list chunk from climate %s",
						GetFormEditorID());
				}
				break;
			}
			case TNAM_ID:
				apFile->GetChunkData(ucData, 6);
				break;
			default:
				break;
			}
		} else if (uiChunk >= FNAM_ID) {
			// The two sky object textures: the exe indexes txSkyObjects with the chunk's offset from FNAM and skips
			// an index past the array.
			int32_t iIndex = (int32_t)(uiChunk - FNAM_ID);
			if (iIndex < NUM_TEXTURES) {
				TESTexture::LoadTextureChunk(&txSkyObjects[iIndex], apFile);
			}
		} else {
			switch (uiChunk) {
			case EDID_ID: {
				// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size, read
				// up to 0x200.
				uint32_t uiChunkSize = apFile->GetChunkSize();
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
				// Not in the exe: an empty chunk leaves the buffer unwritten.
				pBuffer[0] = '\0';
				apFile->GetChunkData(pBuffer, 0x200);
				SetFormEditorID(pBuffer);
				break;
			}
			case OBND_ID:
				LoadObjectBound(apFile);
				break;
			case MODL_ID:
				TESModel::LoadModelChunk(&mNightSky, apFile);
				break;
			default:
				break;
			}
		}
	} while (apFile->NextChunk());
	return true;
}

void TESClimate::Save() {
	StartForm();
	weatherList.Save(WLST_ID);
	for (int32_t i = 0; i < NUM_TEXTURES; ++i) {
		txSkyObjects[i].Save((CHUNK_ID)(i + FNAM_ID));
	}
	mNightSky.Save(MODL_ID, MODT_ID);
	AddChunkArray(TNAM_ID, reinterpret_cast<const char*>(ucData), sizeof(ucData));
	CloseForm();
}

void TESClimate::Copy(TESForm* apCopy) {
	TESClimate* pcopy = dynamic_cast<TESClimate*>(apCopy);
	if (pcopy) {
		CopyAllComponents(pcopy);
		weatherList.CopyComponent(&pcopy->weatherList, 0);
		for (int32_t i = 0; i < 2; ++i) {
			txSkyObjects[i].CopyComponent(&pcopy->txSkyObjects[i]);
		}
		mNightSky.CopyComponent(&pcopy->mNightSky);
		ourMemcpy(ucData, pcopy->ucData, 6u);
	}
}

bool TESClimate::Compare(TESForm* apCompare) {
	TESClimate* pcomp = dynamic_cast<TESClimate*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	for (int32_t i = 0; i < 2; ++i) {
		if (txSkyObjects[i].CompareComponent(&pcomp->txSkyObjects[i])) {
			return 1;
		}
	}
	return mNightSky.CompareComponent(&pcomp->mNightSky) ||
		(weatherList.CompareComponent(&pcomp->weatherList) || std::memcmp(ucData, pcomp->ucData, 6u) != 0);
}

// The exe clamps to ~0xC0 (-193, signed), not to the 0x3F mask: for any count of -193 or more the low six bits
// become 0x3F (the low byte of -193).
void TESClimate::SetMoonPhaseDays(uint32_t auiDays) {
	int32_t iFlags = ucData[MOON_DATA] & 0xC0;
	ucData[MOON_DATA] = (uint8_t)(iFlags | BSMIN(static_cast<int32_t>(auiDays), ~0xC0));
}

// The exe's cleanup-only C++ EH frame is left out.
void TESClimate::MakeDefault() {
	WeatherEntry* pEntry;
	txSkyObjects->SetTextureName("Sky\\Sun.dds");
	txSkyObjects[TESClimate::TX_GLARE].SetTextureName("Sky\\SunGlare.dds");
	mNightSky.SetModel("Sky\\Stars.nif");
	ucData[TESClimate::SUNRISE_BEGIN] = 36;
	ucData[TESClimate::SUNRISE_END] = 42;
	ucData[TESClimate::SUNSET_BEGIN] = 108;
	ucData[TESClimate::SUNSET_END] = 114;
	ucData[TESClimate::VOLATILITY] = 0;
	SetUsesMasser(1);
	SetUsesSecunda(1);
	SetMoonPhaseDays(3u);
	weatherList.RemoveAll();
	WeatherEntry* apMemory = static_cast<WeatherEntry*>(ourMalloc(sizeof(WeatherEntry)));
	if (apMemory) {
		pEntry = ::new (apMemory) WeatherEntry;
	} else {
		pEntry = 0;
	}
	WeatherEntry* pEntry_2 = pEntry;
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(350u);
	pEntry->pWeather = dynamic_cast<TESWeather*>(FormByNumericID);
	pEntry_2->uiChance = 100;
	if (pEntry_2->pWeather) {
		weatherList.AddHead(pEntry_2);
	} else {
		ourFree(pEntry_2);
		Warning("DEFAULT: Unable to add default weather to default climate.  ( TESClimate::MakeDefault() )");
	}
}

uint8_t TESClimate::GetVolatilityIndex() const {
	return (uint8_t)ucData[4];
}

void TESClimate::SetUsesMasser(bool abUses) {
	int8_t v2;
	int8_t v3;
	if (abUses) {
		ucData[5] |= 0x80u;
		v2 = ucData[5];
	} else {
		ucData[5] &= (~0x80u);
		v3 = ucData[5];
	}
	int8_t v4 = ucData[5];
}

void TESClimate::SetUsesSecunda(bool abUses) {
	int8_t v2;
	int8_t v3;
	if (abUses) {
		ucData[5] |= 0x40u;
		v2 = ucData[5];
	} else {
		ucData[5] &= (~0x40u);
		v3 = ucData[5];
	}
	int8_t v4 = ucData[5];
}

uint32_t TESClimate::GetMoonPhaseDays() const {
	return (uint32_t)(ucData[5] & 63);
}

bool TESClimate::UsesSecunda() const {
	return ((ucData[5] & 64) != 0);
}

TESWeather* TESClimate::GetRandomWeather() {
	return (&weatherList)->GetRandomWeather();
}

bool TESClimate::UsesMasser() const {
	return ((ucData[5] & 128) != 0);
}
