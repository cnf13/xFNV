#include "Fallout Shared/TESWeather.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTexture1024.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

TESWeather::WeatherSound::WeatherSound() : uiSoundFormID(0), eType() {}

TESWeather::WeatherSound::WeatherSound(uint32_t auiSoundFormID, SoundType aeType)
  : uiSoundFormID(auiSoundFormID), eType(aeType) {}

void TESWeather::WeatherSound::Endian() {
    ENDIAN(uiSoundFormID, false);
    ENDIAN(eType, false);
}

void TESWeather::WeatherSoundList::FlushList() {
    for (BSSimpleList<TESWeather::WeatherSound*>* pCurrentNode = this; pCurrentNode && pCurrentNode->m_item;
         pCurrentNode = pCurrentNode->m_pkNext) {
        ourFree(pCurrentNode->m_item);
    }
    RemoveAll();
}

TESWeather::TESWeather() {
    cFormType = 53;
    TESWeather::InitializeData();
}

TESWeather::~TESWeather() {
    TESWeather::ClearData();
}

void TESWeather::InitializeData() {
    iNumCloudLayers = 3;
    ourMemset(uiColorData, 0, sizeof(uiColorData));
    ourMemset(ucWeatherData, 0, 0xFu);
    ourMemset(fFogData, 0, sizeof(fFogData));
    fFogData[4] = (float)1.0;
    fFogData[5] = (float)1.0;
    ourMemset(ucCloudLayerSpeed, 0, 4u);
    ourMemset(uiCloudColorData, 0, sizeof(uiCloudColorData));
}

void TESWeather::ClearData() {
    soundList.FlushList();
}

// When the EDID chunk is empty (malformed data), the editor ID buffer is initialised to "" (one zeroed byte, allocated
// only for an empty chunk). The record's chunks: the editor ID, the bound, the precipitation model, the cloud textures,
// the per-time image space modifiers, the cloud speeds and colors, the sky colors, the fog distances, the HDR and image
// space data, the weather data and the sounds. Older files store the colors as 4 times instead of 6, which are moved
// into place.
bool TESWeather::Load(TESFile* apFile) {
    if (static_cast<uint8_t>(apFile->GetTESForm()) != 0x35) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            // Each image space chunk holds a modifier's form ID, which is kept in the pointer (untyped until InitItem
            // resolves it).
            case SIAD_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    0, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            // MNAM and '\1IAD' both set modifier 1; '\2IAD' to '\6IAD' set modifiers 2 to 6.
            case MNAM_ID:
            case SIAD_ID + 1: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    1, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case SIAD_ID + 2: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    2, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case SIAD_ID + 3: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    3, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case SIAD_ID + 4: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    4, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case SIAD_ID + 5: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    5, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            // Passes index 6, one past the six modifiers, deliberately, as the game does.
            case SIAD_ID + 6: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESImageSpaceModifiableCountForm<6>::SetFormImageSpaceModifying(
                    6, reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case DNAM_ID: TESTexture::LoadTextureChunk(&cloudTexture[0], apFile); break;
            case CNAM_ID: TESTexture::LoadTextureChunk(&cloudTexture[1], apFile); break;
            case ANAM_ID: TESTexture::LoadTextureChunk(&cloudTexture[2], apFile); break;
            case BNAM_ID: TESTexture::LoadTextureChunk(&cloudTexture[3], apFile); break;
            // ENAM is loaded as a fifth cloud texture, one past the four (over the cloud layer speeds), as the game
            // does.
            case ENAM_ID: TESTexture::LoadTextureChunk(&cloudTexture[4], apFile); break;
            case ONAM_ID: apFile->GetChunkData(ucCloudLayerSpeed, 4); break;
            case PNAM_ID:
                if (apFile->GetChunkSize() == 0x60) {
                    apFile->GetChunkData(uiCloudColorData, 0x60);
                }
                else if (apFile->GetChunkSize() == 0x40) {
                    uint32_t uiOldCloudColorData[4][4];
                    apFile->GetChunkData(uiOldCloudColorData, 0x40);
                    for (int32_t i = 0; i < 4; ++i) {
                        for (int32_t j = 0; j < 4; ++j) {
                            uiCloudColorData[i][j] = uiOldCloudColorData[i][j];
                        }
                    }
                }
                if (apFile->QEndian()) {
                    for (int32_t i = 0; i < 4; ++i) {
                        for (int32_t j = 0; j < 6; ++j) {
                            ENDIAN(uiCloudColorData[i][j], false);
                        }
                    }
                }
                break;
            case MODL_ID: TESModel::LoadModelChunk(&precipNIF, apFile); break;
            case NAM0_ID:
                if (apFile->GetChunkSize() == 0xF0) {
                    apFile->GetChunkData(uiColorData, 0xF0);
                }
                else if (apFile->GetChunkSize() == 0xA0) {
                    uint32_t uiOldColorData[10][4];
                    apFile->GetChunkData(uiOldColorData, 0xA0);
                    for (int32_t i = 0; i < 10; ++i) {
                        for (int32_t j = 0; j < 4; ++j) {
                            uiColorData[i][j] = uiOldColorData[i][j];
                        }
                    }
                }
                if (apFile->QEndian()) {
                    for (int32_t i = 0; i < 10; ++i) {
                        for (int32_t j = 0; j < 6; ++j) {
                            ENDIAN(uiColorData[i][j], false);
                        }
                    }
                }
                break;
            case DATA_ID:
                if (apFile->GetChunkSize() == 0xF) {
                    apFile->GetChunkData(ucWeatherData, 0xF);
                }
                else if (apFile->GetChunkSize() == 0xC) {
                    apFile->GetChunkData(ucWeatherData, 0xC);
                    ourMemset(&ucWeatherData[12], 0xFF, 3);
                }
                break;
            case FNAM_ID: {
                // Older files store four fog distances; all six are read, but only the stored ones are swapped.
                int32_t iCount = 6;
                if (apFile->GetChunkSize() != 0x18) {
                    iCount -= 2;
                }
                apFile->GetChunkData(fFogData, 0x18);
                if (apFile->QEndian()) {
                    for (int32_t i = 0; i < iCount; ++i) {
                        ENDIAN(fFogData[i], false);
                    }
                }
                break;
            }
            case HNAM_ID:
                apFile->GetChunkData(fHDRData, 0x38);
                if (apFile->QEndian()) {
                    for (float& fHDR : fHDRData) {
                        ENDIAN(fHDR, false);
                    }
                }
                break;
            case INAM_ID:
                apFile->GetChunkData(pImageSpaceParameterMods, 0x130);
                if (apFile->QEndian()) {
                    for (ImageSpaceParameterData& kMods : pImageSpaceParameterMods) {
                        kMods.Endian();
                    }
                }
                break;
            case LNAM_ID:
                apFile->GetChunkData(&iNumCloudLayers, 4);
                if (apFile->QEndian()) {
                    ENDIAN(iNumCloudLayers, false);
                }
                break;
            case SNAM_ID: {
                void* pMemory = ourMalloc(sizeof(WeatherSound));
                WeatherSound* pSound = pMemory ? ::new (pMemory) WeatherSound : nullptr;
                apFile->GetChunkData(pSound, 8);
                if (apFile->QEndian()) {
                    pSound->Endian();
                }
                TESForm::AddCompileIndex(pSound->uiSoundFormID, apFile);
                soundList.AddTail(pSound);
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

// The cloud layers' textures are saved under the chunk IDs of a local table, one per layer. The fog data is saved as
// the array it is in the form buffer.
void TESWeather::Save() {
    StartForm();
    TESImageSpaceModifiableCountForm<6>::Save();
    CHUNK_ID aeCloudTextureIDs[8] = {DNAM_ID, CNAM_ID, ANAM_ID, BNAM_ID, ENAM_ID, GNAM_ID, JNAM_ID, KNAM_ID};
    for (int32_t i = 0; i < iNumCloudLayers; ++i) {
        cloudTexture[i].Save(aeCloudTextureIDs[i]);
    }
    precipNIF.Save(MODL_ID, MODT_ID);
    // The whole count is written as a 32-bit value through the uint32_t overload.
    TESForm::AddChunk(LNAM_ID, static_cast<uint32_t>(iNumCloudLayers));
    TESForm::AddChunkArray(ONAM_ID, reinterpret_cast<const char*>(ucCloudLayerSpeed), 4);
    TESForm::AddChunkArray(PNAM_ID, &uiCloudColorData[0][0], 0x18);
    TESForm::AddChunkArray(NAM0_ID, &uiColorData[0][0], 0x3C);
    // The float array is passed to the uint32_t-array overload as raw data.
    TESForm::AddChunkArray(FNAM_ID, reinterpret_cast<const uint32_t*>(fFogData), 6);
    if (TESForm::QEndianSwapOnSave()) {
        for (ImageSpaceParameterData& kMods : pImageSpaceParameterMods) {
            kMods.Endian();
        }
    }
    TESForm::__AddChunkData(INAM_ID, pImageSpaceParameterMods, 0x130);
    if (TESForm::QEndianSwapOnSave()) {
        for (ImageSpaceParameterData& kMods : pImageSpaceParameterMods) {
            kMods.Endian();
        }
    }
    TESForm::AddChunkArray(DATA_ID, reinterpret_cast<const char*>(ucWeatherData), 0xF);
    for (BSSimpleList<WeatherSound*>* pIter = &soundList; pIter; pIter = pIter->GetNext()) {
        if (!pIter->GetItem()) {
            break;
        }
        if (TESForm::QEndianSwapOnSave()) {
            pIter->GetItem()->Endian();
        }
        TESForm::__AddChunkData(SNAM_ID, pIter->GetItem(), 8);
        if (TESForm::QEndianSwapOnSave()) {
            pIter->GetItem()->Endian();
        }
    }
    CloseForm();
}

void TESWeather::InitItem() {
    if (GetInitialized()) {
        return;
    }
    TESImageSpaceModifiableCountForm<6>::InitItem(this);
    SetInitialized(true);
}

void TESWeather::Copy(TESForm* apCopy) {
    TESWeather* pcopy = dynamic_cast<TESWeather*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        iNumCloudLayers = pcopy->iNumCloudLayers;
        for (int32_t i = 0; i < iNumCloudLayers; ++i) {
            cloudTexture[i].CopyComponent(&pcopy->cloudTexture[i]);
        }
        precipNIF.CopyComponent(&pcopy->precipNIF);
        ourMemcpy(ucCloudLayerSpeed, pcopy->ucCloudLayerSpeed, 4u);
        ourMemcpy(uiCloudColorData, pcopy->uiCloudColorData, sizeof(uiCloudColorData));
        ourMemcpy(ucWeatherData, pcopy->ucWeatherData, 15u);
        ourMemcpy(uiColorData, pcopy->uiColorData, sizeof(uiColorData));
        ourMemcpy(fFogData, pcopy->fFogData, sizeof(fFogData));
        ourMemcpy(pImageSpaceParameterMods, pcopy->pImageSpaceParameterMods, sizeof(pImageSpaceParameterMods));
        soundList.RemoveAll();
        for (BSSimpleList<WeatherSound*>* pIter = &pcopy->soundList; pIter && pIter->GetItem();
             pIter = pIter->GetNext()) {
            WeatherSound* pMemory = static_cast<WeatherSound*>(ourMalloc(sizeof(WeatherSound)));
            WeatherSound* pSound;
            if (pMemory) {
                pSound = ::new (pMemory) WeatherSound(pIter->GetItem()->uiSoundFormID, pIter->GetItem()->eType);
            }
            else {
                pSound = nullptr;
            }
            soundList.AddTail(pSound);
        }
    }
}

bool TESWeather::Compare(TESForm* apCompare) {
    TESWeather* pcomp = dynamic_cast<TESWeather*>(apCompare);
    if (!pcomp) {
        return true;
    }
    BSSimpleList<TESWeather::WeatherSound*>* pCompSL = &pcomp->soundList;
    BSSimpleList<TESWeather::WeatherSound*>* pThisSL = &soundList;
    int32_t v3 = pcomp->soundList.ItemsInList();
    if (v3 != soundList.ItemsInList()) {
        return true;
    }
    while (pCompSL && (pThisSL && (pCompSL->m_item && pThisSL->m_item))) {
        if (std::memcmp(pCompSL->m_item, pThisSL->m_item, sizeof(TESWeather::WeatherSound))) {
            return true;
        }
        pCompSL = pCompSL->m_pkNext;
        pThisSL = pThisSL->m_pkNext;
    }
    if (iNumCloudLayers != pcomp->iNumCloudLayers) {
        return true;
    }
    for (int32_t i = 0; i < iNumCloudLayers; ++i) {
        if (cloudTexture[i].CompareComponent(&pcomp->cloudTexture[i])) {
            return true;
        }
    }
    return CompareAllComponents(pcomp) ||
        (precipNIF.CompareComponent(&pcomp->precipNIF) ||
         (std::memcmp(ucWeatherData, pcomp->ucWeatherData, 15u) ||
          (std::memcmp(uiColorData, pcomp->uiColorData, sizeof(uiColorData)) ||
           (std::memcmp(fFogData, pcomp->fFogData, sizeof(fFogData)) ||
            (std::memcmp(ucCloudLayerSpeed, pcomp->ucCloudLayerSpeed, 4u) ||
             (std::memcmp(uiCloudColorData, pcomp->uiCloudColorData, sizeof(uiCloudColorData)) ||
              std::memcmp(
                  pImageSpaceParameterMods, pcomp->pImageSpaceParameterMods, sizeof(pImageSpaceParameterMods)
              )))))));
}

void TESWeather::MakeDefault() {
    iNumCloudLayers = 3;
    cloudTexture[0].SetTextureName("Sky\\WastelandCloudCloudyUpper01.dds");
    cloudTexture[1].SetTextureName("Sky\\WastelandCloudHorizon01.dds");
    cloudTexture[2].SetTextureName("Sky\\WastelandCloudCloudyLower01.dds");
    ucCloudLayerSpeed[0] = 51;
    uiCloudColorData[0][0] = 0x5B4B46;
    uiCloudColorData[0][1] = 0x9B9B9B;
    uiCloudColorData[0][2] = 0x6C5953;
    uiCloudColorData[0][3] = 0x1C140F;
    ucWeatherData[TESWeather::WD_WIND_SPEED] = 51;
    ucWeatherData[TESWeather::WD_TRANS_DELTA] = -1;
    ucWeatherData[TESWeather::WD_SUN_GLARE] = -1;
    ucWeatherData[TESWeather::WD_SUN_DAMAGE] = -1;
    ucWeatherData[TESWeather::WD_DIVIDER1] = 0;
    ucWeatherData[TESWeather::WD_END_PRECIP] = -1;
    ucWeatherData[TESWeather::WD_BEGIN_THUNDER] = 0;
    ucWeatherData[TESWeather::WD_END_THUNDER] = -1;
    ucWeatherData[TESWeather::WD_THUNDER_FREQ] = 50;
    ucWeatherData[TESWeather::WD_DIVIDER2] = 0;
    fFogData[TESWeather::NEAR_DIST_NIGHT] = 27852.8f;
    fFogData[TESWeather::NEAR_DIST_DAY] = 27852.8f;
    fFogData[TESWeather::FAR_DIST_NIGHT] = 163840.0f;
    fFogData[TESWeather::FAR_DIST_DAY] = 163840.0f;
    fFogData[TESWeather::FOG_POWER_NIGHT] = 1.0f;
    fFogData[TESWeather::FOG_POWER_DAY] = 1.0f;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::SKY_UPPER] = 0x846C66;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::SKY_UPPER] = 0x846C66;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::FOG] = 0x71533E;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::FOG] = 0x71533E;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::SUNLIGHT] = 0x71533E;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::SUNLIGHT] = 0x71533E;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::CLOUD_LAYER] = 0x846D66;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::CLOUD_LAYER] = 0x846D66;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::AMBIENT] = 0x100A05;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::AMBIENT] = 0x100A05;
    uiColorData[TESWeather::SKY_LOWER][TESWeather::SUN] = 0x100A05;
    uiColorData[TESWeather::SKY_UPPER][TESWeather::SUN] = 0x100A05;
    uiColorData[TESWeather::HORIZON][TESWeather::SKY_UPPER] = 0x5F6DB6;
    uiColorData[TESWeather::FOG][TESWeather::SKY_UPPER] = 0x5F6DB6;
    uiColorData[TESWeather::HORIZON][TESWeather::FOG] = 0xD19A83;
    uiColorData[TESWeather::FOG][TESWeather::FOG] = 0xD19A83;
    uiColorData[TESWeather::HORIZON][TESWeather::SUNLIGHT] = 0xD19A83;
    uiColorData[TESWeather::FOG][TESWeather::SUNLIGHT] = 0xD19A83;
    uiColorData[TESWeather::HORIZON][TESWeather::CLOUD_LAYER] = 0x5782A8;
    uiColorData[TESWeather::FOG][TESWeather::CLOUD_LAYER] = 0x5782A8;
    uiColorData[TESWeather::HORIZON][TESWeather::AMBIENT] = 0x261B0F;
    uiColorData[TESWeather::FOG][TESWeather::AMBIENT] = 0x261B0F;
    uiColorData[TESWeather::HORIZON][TESWeather::SUN] = 0x261B0F;
    uiColorData[TESWeather::FOG][TESWeather::SUN] = 0x261B0F;
    uiColorData[TESWeather::AMBIENT][TESWeather::SKY_UPPER] = 0x252212;
    uiColorData[TESWeather::AMBIENT][TESWeather::FOG] = 0x857065;
    uiColorData[TESWeather::AMBIENT][TESWeather::SUNLIGHT] = 0x857065;
    uiColorData[TESWeather::AMBIENT][TESWeather::CLOUD_LAYER] = 0x35221A;
    uiColorData[TESWeather::AMBIENT][TESWeather::AMBIENT] = 0x281C15;
    uiColorData[TESWeather::AMBIENT][TESWeather::SUN] = 0x281C15;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::SKY_UPPER] = 0x79A5FF;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::FOG] = 0xCAEDFF;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::SUNLIGHT] = 0xCAEDFF;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::CLOUD_LAYER] = 0x499AE2;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::AMBIENT] = 0x764D34;
    uiColorData[TESWeather::SUNLIGHT][TESWeather::SUN] = 0x764D34;
    uiColorData[TESWeather::SUN][TESWeather::SKY_UPPER] = 0x779FF2;
    uiColorData[TESWeather::SUN][TESWeather::FOG] = 0xBED9DE;
    uiColorData[TESWeather::SUN][TESWeather::SUNLIGHT] = 0xBED9DE;
    uiColorData[TESWeather::SUN][TESWeather::CLOUD_LAYER] = 0x9DBDFF;
    uiColorData[TESWeather::SUN][TESWeather::AMBIENT] = 0;
    uiColorData[TESWeather::SUN][TESWeather::SUN] = 0;
}

float TESWeather::GetWeatherData(WeatherData aeType, float fMax, float fMin) {
    return (float)(((((double)ucWeatherData[aeType]) * 0.003921568859368563) * (fMax - fMin)) + fMin);
}

uint32_t TESWeather::GetCloudColor(int32_t aiLayer, TESWeather::ColorTime aeColorTime) {
    uint32_t v4;
    if (((int32_t)aeColorTime) == 1) {
        v4 = 10197915;
    }
    else {
        if (((int32_t)aeColorTime) == 3) {
            v4 = 1840143;
        }
        else {
            if (aeColorTime) {
                v4 = 7100755;
            }
            else {
                v4 = 5983046;
            }
        }
    }
    if (iNumCloudLayers > 0) {
        if (aiLayer >= iNumCloudLayers) {
            aiLayer = 0;
        }
        return uiCloudColorData[aiLayer][(int32_t)aeColorTime];
    }
    return v4;
}

bool TESWeather::IsPleasant() const {
    return ((ucWeatherData[11] & 1) != 0);
}

bool TESWeather::IsCloudy() const {
    return ((ucWeatherData[11] & 2) != 0);
}

float TESWeather::GetCloudLayerSpeed(int32_t aiLayer, float fMax, float fMin) const {
    uint8_t v7 = 51;
    if (iNumCloudLayers > 0) {
        if (aiLayer >= iNumCloudLayers) {
            aiLayer = 0;
        }
        v7 = ucCloudLayerSpeed[aiLayer];
    }
    return (float)(((((double)v7) * 0.003921568859368563) * (fMax - fMin)) + fMin);
}

const TESWeather::WeatherSoundList* TESWeather::GetSoundList() const {
    return &soundList;
}

const TESTexture* TESWeather::GetCloudTexture(int32_t aiLayer) const {
    return &cloudTexture[aiLayer];
}

int32_t TESWeather::IsRainy() {
    return (int32_t)((uint8_t)((ucWeatherData[11] & 4) != 0));
}

uint32_t TESWeather::GetColor(TESWeather::ColorType aeColorType, TESWeather::ColorTime aeColorTime) const {
    return uiColorData[(uint32_t)aeColorType][(uint32_t)aeColorTime];
}
