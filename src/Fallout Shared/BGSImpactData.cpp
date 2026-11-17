#include "Fallout Shared/BGSImpactData.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

// The decal sizes first, then the impact fields, then the rest of the decal data and the sound level, in the game's
// order. The flag and parallax pass bytes need no swap.
void BGSImpactData::OLD_IMPACT_DATA_DATA::Endian() {
    ENDIAN(fDecalMinWidth, false);
    ENDIAN(fDecalMaxWidth, false);
    ENDIAN(fDecalMinHeight, false);
    ENDIAN(fDecalMaxHeight, false);
    ENDIAN(fEffectDuration, false);
    ENDIAN(fParallaxScale, false);
    ENDIAN(eOrient, false);
    ENDIAN(fDepth, false);
    ENDIAN(fShininess, false);
    ENDIAN(fAngleThreshold, false);
    ENDIAN(fPlacementRadius, false);
    ENDIAN(iColor, false);
    ENDIAN(eSoundLevel, false);
}

// Swaps the five dwords read from a big-endian file, in field order; the flag bytes need no swap.
void BGSImpactData::IMPACT_DATA_DATA::Endian() {
    ENDIAN(fEffectDuration, false);
    ENDIAN(eOrient, false);
    ENDIAN(fAngleThreshold, false);
    ENDIAN(fPlacementRadius, false);
    ENDIAN(eSoundLevel, false);
}

BGSImpactData::BGSImpactData() {
    SetFormType(94u);
    BGSImpactData::InitializeData();
}

BGSImpactData::~BGSImpactData() {}

void BGSImpactData::InitializeData() {
    ourMemset(&Data, 0, sizeof(Data));
    Data.fEffectDuration = 0x1p-2f;
    Data.eOrient = BGSImpactData::ORIENT_PROJ_REFLECT;
    Data.fAngleThreshold = 15.0f;
    Data.fPlacementRadius = 0x1p+4f;
    pDecalTextureSet = nullptr;
    pSound1 = nullptr;
    pSound2 = nullptr;
    Data.eSoundLevel = SOUND_LEVEL_NORMAL;
}

// Reads the impact data's chunks. DATA comes in two layouts: the current one, which is the impact data itself, and the
// older one, which also carries the decal data and which is copied field by field into Data and DData.
bool BGSImpactData::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != IPCT_ID) {
        return false;
    }
    this->LoadForm(apFile);
    if (apFile->QEndian()) {
        Data.Endian();
    }
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case DATA_ID: {
                if (apFile->GetFormVersion() >= 10) {
                    ourMemset(&Data, 0, sizeof(Data));
                    this->LoadData(apFile, reinterpret_cast<char*>(&Data), 0x18u);
                    if (apFile->QEndian()) {
                        Data.Endian();
                    }
                }
                else {
                    OLD_IMPACT_DATA_DATA oldData;
                    ourMemset(&oldData, 0, sizeof(oldData));
                    this->LoadData(apFile, reinterpret_cast<char*>(&oldData), 0x38u);
                    if (apFile->QEndian()) {
                        oldData.Endian();
                    }
                    ourMemset(&Data, 0, sizeof(Data));
                    Data.fEffectDuration = oldData.fEffectDuration;
                    Data.eOrient = oldData.eOrient;
                    Data.fAngleThreshold = oldData.fAngleThreshold;
                    Data.fPlacementRadius = oldData.fPlacementRadius;
                    Data.eSoundLevel = oldData.eSoundLevel;
                    DData.fDecalMinWidth = oldData.fDecalMinWidth;
                    DData.fDecalMaxWidth = oldData.fDecalMaxWidth;
                    DData.fDecalMinHeight = oldData.fDecalMinHeight;
                    DData.fDecalMaxHeight = oldData.fDecalMaxHeight;
                    DData.fDepth = oldData.fDepth;
                    DData.fShininess = oldData.fShininess;
                    DData.fParallaxScale = oldData.fParallaxScale;
                    DData.ucParallaxPasses = oldData.cParallaxPasses;
                    DData.ucFlags = oldData.cFlags;
                    DData.uiColor = oldData.iColor;
                }
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case NAM1_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                this->SetSound2(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case DNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                this->SetDecalTextureSet(reinterpret_cast<BGSTextureSet*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case SNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                this->SetSound1(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case DODT_ID: DData.Load(apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSImpactData::Save() {
    uint32_t v2;
    uint32_t v3;
    uint32_t v4;
    TESForm::StartForm();
    static_cast<TESModel*>(this)->Save(MODL_ID, MODT_ID);
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    TESForm::__SaveData(&Data, 0x18u);
    if (TESForm::QEndianSwapOnSave()) {
        (&Data)->Endian();
    }
    if (!GetUseTextureSetDecalData()) {
        (&DData)->Save(DODT_ID);
    }
    if (pDecalTextureSet) {
        v4 = pDecalTextureSet->TESForm::GetFormID();
        TESForm::AddChunk(DNAM_ID, v4);
    }
    if (pSound1) {
        v3 = pSound1->TESForm::GetFormID();
        TESForm::AddChunk(SNAM_ID, v3);
    }
    if (pSound2) {
        v2 = pSound2->TESForm::GetFormID();
        TESForm::AddChunk(NAM1_ID, v2);
    }
    TESForm::CloseForm();
}

void BGSImpactData::InitItem() {
    if (!GetInitialized()) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDecalTextureSet));
        if (itempint) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            pDecalTextureSet = dynamic_cast<BGSTextureSet*>(FormByNumericID);
            if (!pDecalTextureSet) {
                int32_t CastingType = iFormID;
                const char* v7 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find texture set (%08X) on impact data %s (%08X).", itempint, v7, CastingType
                );
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSound1));
        if (itempint) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempint);
            pSound1 = dynamic_cast<TESSound*>(v4);
            if (!pSound1) {
                int32_t v11 = iFormID;
                const char* v8 = GetFormEditorID();
                Warning("MASTERFILE: Unable to find sound (%08X) on impact data %s (%08X).", itempint, v8, v11);
            }
        }
        itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSound2));
        if (itempint) {
            TESFile* v5 = GetFile(-1);
            TESForm::AddCompileIndex(itempint, v5);
            TESForm* v6 = TESForm::GetFormByNumericID(itempint);
            pSound2 = dynamic_cast<TESSound*>(v6);
            if (!pSound2) {
                int32_t v12 = iFormID;
                const char* v9 = GetFormEditorID();
                Warning("MASTERFILE: Unable to find sound (%08X) on impact data %s (%08X).", itempint, v9, v12);
            }
        }
        CopyDecalDataFromTextureSet();
        SetInitialized(true);
    }
}

void BGSImpactData::Copy(TESForm* apCopy) {
    BGSImpactData* v3 = dynamic_cast<BGSImpactData*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(v3);
        ourMemcpy(&Data, &v3->Data, sizeof(Data));
        (&DData)->Copy(&v3->DData);
        pDecalTextureSet = v3->pDecalTextureSet;
        pSound1 = v3->pSound1;
        pSound2 = v3->pSound2;
    }
}

bool BGSImpactData::Compare(TESForm* apCompare) {
    BGSImpactData* v4 = dynamic_cast<BGSImpactData*>(apCompare);
    if (!v4) {
        return true;
    }
    if (TESForm::CompareAllComponents(v4)) {
        return true;
    }
    if (std::memcmp(&Data, &v4->Data, sizeof(Data))) {
        return true;
    }
    if ((!GetUseTextureSetDecalData()) && (&DData)->Compare(&v4->DData)) {
        return true;
    }
    if (pDecalTextureSet != v4->pDecalTextureSet) {
        return true;
    }
    if (pSound1 == v4->pSound1) {
        return (bool)((uint8_t)(pSound2 != v4->pSound2));
    }
    return true;
}

bool BGSImpactData::GetUseTextureSetDecalData() {
    return ((Data.cFlags & 1) != 0);
}

float BGSImpactData::GetDecalShininess() {
    return (&DData)->DecalData::GetDecalShininess();
}

float BGSImpactData::GetDecalParallaxScale() {
    return (&DData)->DecalData::GetDecalParallaxScale();
}

uint32_t BGSImpactData::GetDecalColor() {
    return (&DData)->DecalData::GetDecalColor();
}

float BGSImpactData::GetDecalDepth() {
    return (&DData)->GetDecalDepth();
}

bool BGSImpactData::GetIsAlphaBlend() {
    return (bool)((uint8_t)(&DData)->GetAlphaBlending());
}

uint8_t BGSImpactData::GetParallaxPasses() {
    return (&DData)->GetDecalParallaxPasses();
}

int32_t BGSImpactData::GetSoundLevelValue() {
    int32_t iValue = 0;
    switch (Data.eSoundLevel) {
        case SOUND_LEVEL_LOUD: iValue = iSoundLevelLoud.UInt(); break;
        case SOUND_LEVEL_NORMAL: iValue = iSoundLevelNormal.UInt(); break;
        case SOUND_LEVEL_SILENT: iValue = iSoundLevelSilent.UInt(); break;
    }
    return iValue;
}

void BGSImpactData::CopyDecalDataFromTextureSet() {
    BGSTextureSet* combatStyle;
    DecalData* decalData;
    if (GetUseTextureSetDecalData()) {
        if (BGSImpactData::GetDecalTextureSet()) {
            combatStyle = BGSImpactData::GetDecalTextureSet();
            decalData = combatStyle->GetDecalData();
        }
        else {
            decalData = DecalData::GetDefaultDecalData();
        }
        (&DData)->Copy(decalData);
    }
}

double BGSImpactData::GetMinWidth() {
    return (&DData)->GetDecalMinWidth();
}

bool BGSImpactData::GetIsParallax() {
    return (bool)((uint8_t)(&DData)->IsParallax());
}

bool BGSImpactData::GetIsAlphaTest() {
    return (bool)((uint8_t)(&DData)->GetAlphaTesting());
}

void BGSImpactData::SetDecalTextureSet(BGSTextureSet* apSet) {
    pDecalTextureSet = apSet;
}

void BGSImpactData::SetSound1(TESSound* apSound) {
    pSound1 = apSound;
}

void BGSImpactData::SetSound2(TESSound* apSound) {
    pSound2 = apSound;
}
