#include "Fallout Shared/BGSExplosion.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// Reads the explosion's chunks. ENAM/EITM and ANAM/EAMT are the enchantment's form ID and amount, MNAM its image
// space modifier, INAM the placed object; the DATA chunk holds the explosion data, which a big-endian file stores
// swapped.
bool BGSExplosion::Load(TESFile* apFile) {
    // Only the form type byte is compared, not the whole return value.
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != EXPL_ID) {
        return false;
    }
    static_cast<TESForm*>(this)->ClearData();
    this->LoadForm(apFile);
    this->SetInitialized(false);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case DATA_ID:
                this->LoadData(apFile, reinterpret_cast<char*>(&Data), 0x34u);
                // The file's record is 0x34 bytes, with a 4-byte form ID in each pointer field.
                TESFile::WidenFormIDRecords(
                    &Data,
                    1,
                    0x34,
                    sizeof(Data),
                    {{0xC, offsetof(BGSExplosionData, pLight)},
                     {0x10, offsetof(BGSExplosionData, pSound1)},
                     {0x1C, offsetof(BGSExplosionData, pImpactDataSet)},
                     {0x20, offsetof(BGSExplosionData, pSound2)}}
                );
                break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case ENAM_ID:
            case EITM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchanting(
                    reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case ANAM_ID:
            case EAMT_ID: {
                int16_t sAmount = 0;
                apFile->LoadWord(&sAmount);
                static_cast<TESEnchantableForm*>(this)->SetFormEnchantmentAmount(static_cast<uint16_t>(sAmount));
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case INAM_ID: {
                // The chunk holds a form ID, which is stored as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                pImpactPlacedObject = reinterpret_cast<TESBoundObject*>(static_cast<uintptr_t>(uiFormID));
                break;
            }
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case MNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<TESImageSpaceModifiableForm*>(this)->SetFormImageSpaceModifying(
                    reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
        if (eChunk == DATA_ID && apFile->QEndian()) {
            Data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

BGSExplosionData::BGSExplosionData() {
    Initialize();
}

// Swaps all thirteen dwords read from a big-endian file, in field order. CASINO_DATA::Endian shares the code.
void BGSExplosionData::Endian() {
    ENDIAN(fForce, false);
    ENDIAN(fDamage, false);
    ENDIAN(fRadius, false);
    ENDIAN(pLight, false);
    ENDIAN(pSound1, false);
    ENDIAN(iFlags, false);
    ENDIAN(fImageSpaceRadius, false);
    ENDIAN(pImpactDataSet, false);
    ENDIAN(pSound2, false);
    ENDIAN(fRadiationRads, false);
    ENDIAN(fRadiationDissipationTime, false);
    ENDIAN(fRadiationRadius, false);
    ENDIAN(eSoundLevel, false);
}

void BGSExplosionData::Initialize() {
    fForce = 0.0f;
    fDamage = 0.0f;
    fRadius = 0.0f;
    fRadiationRads = 0.0f;
    fRadiationDissipationTime = 0.0f;
    fRadiationRadius = 0.0f;
    pLight = nullptr;
    pSound1 = nullptr;
    iFlags = 0;
    fImageSpaceRadius = 0.0f;
    pImpactDataSet = nullptr;
    pSound2 = nullptr;
    eSoundLevel = SOUND_LEVEL_NORMAL;
}

BGSExplosion::BGSExplosion()
  : TESBoundObject(), TESFullName(), TESModel(), TESEnchantableForm(), BGSPreloadable(), TESImageSpaceModifiableForm(),
    Data() {
    cFormType = 81;
    BGSExplosion::InitializeData();
    SetCastingType(MagicSystem::WHEN_STRIKES);
}

BGSExplosion::~BGSExplosion() {}

void BGSExplosion::InitializeData() {
    pImpactPlacedObject = nullptr;
    GetData()->Initialize();
}

// Data is written with each form pointer replaced by its form ID, stored raw in the pointer field, then the pointers
// are put back.
void BGSExplosion::Save() {
    StartForm();
    TESFullName::Save();
    TESModel::Save(MODL_ID, MODT_ID);
    SaveSubRecord();
    TESImageSpaceModifiableForm::Save();
    TESObjectLIGH* pSaveLight = Data.pLight;
    Data.pLight = reinterpret_cast<TESObjectLIGH*>(static_cast<uintptr_t>(Data.pLight ? Data.pLight->iFormID : 0));
    TESSound* pSaveSound1 = Data.pSound1;
    Data.pSound1 = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pSound1 ? Data.pSound1->iFormID : 0));
    TESSound* pSaveSound2 = Data.pSound2;
    Data.pSound2 = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(Data.pSound2 ? Data.pSound2->iFormID : 0));
    BGSImpactDataSet* pSaveImpactDataSet = Data.pImpactDataSet;
    Data.pImpactDataSet = reinterpret_cast<BGSImpactDataSet*>(
        static_cast<uintptr_t>(Data.pImpactDataSet ? Data.pImpactDataSet->iFormID : 0)
    );
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    // The file keeps the form IDs as 4 bytes; pack the record to its layout for the write and restore it after.
    TESFile::NarrowFormIDRecords(
        &Data,
        1,
        0x34,
        sizeof(Data),
        {{0xC, offsetof(BGSExplosionData, pLight)},
         {0x10, offsetof(BGSExplosionData, pSound1)},
         {0x1C, offsetof(BGSExplosionData, pImpactDataSet)},
         {0x20, offsetof(BGSExplosionData, pSound2)}}
    );
    __SaveData(&Data, 52u);
    TESFile::WidenFormIDRecords(
        &Data,
        1,
        0x34,
        sizeof(Data),
        {{0xC, offsetof(BGSExplosionData, pLight)},
         {0x10, offsetof(BGSExplosionData, pSound1)},
         {0x1C, offsetof(BGSExplosionData, pImpactDataSet)},
         {0x20, offsetof(BGSExplosionData, pSound2)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    Data.pLight = pSaveLight;
    Data.pSound1 = pSaveSound1;
    Data.pSound2 = pSaveSound2;
    Data.pImpactDataSet = pSaveImpactDataSet;
    if (QImpactPlacedObject()) {
        TESForm::AddChunk(INAM_ID, QImpactPlacedObject()->iFormID);
    }
    CloseForm();
}

// Before InitItem the form pointers in Data and pImpactPlacedObject hold the form IDs the file stored, and are treated
// as raw form IDs until they are resolved.
void BGSExplosion::InitItem() {
    if (!GetInitialized()) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pLight));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            Data.pLight = dynamic_cast<TESObjectLIGH*>(pForm);
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pSound1));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            Data.pSound1 = dynamic_cast<TESSound*>(pForm);
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pSound2));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            Data.pSound2 = dynamic_cast<TESSound*>(pForm);
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pImpactDataSet));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            Data.pImpactDataSet = dynamic_cast<BGSImpactDataSet*>(pForm);
            if (!Data.pImpactDataSet) {
                int32_t iExplosionFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find impact data set (%08X) for explosion '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iExplosionFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pImpactPlacedObject));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pImpactPlacedObject = dynamic_cast<TESBoundObject*>(pForm);
            if (!pImpactPlacedObject) {
                int32_t iExplosionFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find impact placed object (%08X) for explosion '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iExplosionFormID
                );
            }
        }
        TESImageSpaceModifiableForm::InitItem(this);
        TESEnchantableForm::InitItem(this);
        SetInitialized(true);
    }
}

// The other's data is fetched before this one's.
void BGSExplosion::Copy(TESForm* apCopy) {
    BGSExplosion* pCopy = dynamic_cast<BGSExplosion*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        BGSExplosionData* pSource = pCopy->GetData();
        ourMemcpy(GetData(), pSource, sizeof(BGSExplosionData));
        SetImpactPlacedObject(pCopy->QImpactPlacedObject());
    }
}

// True when the forms differ.
bool BGSExplosion::Compare(TESForm* apCompare) {
    BGSExplosion* pCompare = dynamic_cast<BGSExplosion*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (CompareAllComponents(pCompare)) {
        return true;
    }
    if (std::memcmp(&Data, pCompare->GetData(), sizeof(BGSExplosionData))) {
        return true;
    }
    TESForm* pPlaced = QImpactPlacedObject();
    return pPlaced != pCompare->QImpactPlacedObject();
}

NiAVObject* BGSExplosion::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
    NiAVObject* pAV = nullptr;
    Explosion* pExplosion = static_cast<Explosion*>(pRequester);
    if (!pExplosion->Explosion::GetUnderWater()) {
        return TESBoundObject::Clone3D(pRequester, deepCopy);
    }
    const char* pFilename = pWaterExplosion->GetModel();
    NiAVObject* pLoadedArt = pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, true, false, false, false);
    BSCloningProcess cloneData(1.0f);
    if (pLoadedArt) {
        pAV = static_cast<NiAVObject*>(pLoadedArt->Clone(cloneData));
    }
    return pAV;
}

void BGSExplosion::UnClone3D(TESObjectREFR* apRequester) {
    if (static_cast<Explosion*>(apRequester)->Explosion::GetUnderWater()) {
        const char* pFilename = pWaterExplosion->GetModel();
        pModelLoader->ReleaseFile(pFilename);
    }
    else {
        TESObject::UnClone3D(apRequester);
    }
}

void BGSExplosion::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (GetModelLength()) {
        TESModel* pModel = nullptr;
        if (this) {
            pModel = static_cast<TESModel*>(this);
        }
        ENUM_LOD_MULT eLODMult = TES::GetLODMult(this);
        pModelLoader->QueueModel(pModel, aePriority, apParent, eLODMult, true, false, false);
    }
    if (Data.pImpactDataSet) {
        Data.pImpactDataSet->QueueFiles(aePriority, apParent);
    }
    if (pWaterExplosion && pWaterExplosion->GetModelLength()) {
        TESModel* pWaterModel = pWaterExplosion;
        ENUM_LOD_MULT eLODMult = TES::GetLODMult(this);
        pModelLoader->QueueModel(pWaterModel, aePriority, apParent, eLODMult, true, false, false);
    }
}

float BGSExplosion::GetRadiusBSUnits() {
    if (GetFlag(FLAG_RADII_IN_GAME_UNITS)) {
        return Data.fRadius;
    }

    return Data.fRadius * fBSUnitsPerFoot.Float();
}

bool BGSExplosion::GetFlag(int32_t aiFlag) {
    return (aiFlag & GetData()->iFlags) != 0;
}

double BGSExplosion::GetDissipationTime() {
    return Data.fRadiationDissipationTime;
}

double BGSExplosion::GetISRadius() {
    if (GetFlag(FLAG_RADII_IN_GAME_UNITS)) {
        return Data.fImageSpaceRadius;
    }

    return Data.fImageSpaceRadius * fBSUnitsPerFoot.Float();
}

double BGSExplosion::GetRADRadius() {
    if (GetFlag(FLAG_RADII_IN_GAME_UNITS)) {
        return Data.fRadiationRadius;
    }

    return Data.fRadiationRadius * fBSUnitsPerFoot.Float();
}

BGSExplosionData* BGSExplosion::GetData() {
    return &Data;
}

TESBoundObject* BGSExplosion::QImpactPlacedObject() {
    return pImpactPlacedObject;
}

void BGSExplosion::SetImpactPlacedObject(TESBoundObject* apObj) {
    pImpactPlacedObject = apObj;
}

void BGSExplosion::SetForce(float afValue) {
    Data.fForce = afValue;
}

BGSImpactData* BGSExplosion::GetImpactData(HavokMaterialType aehkMatType) {
    BGSImpactData* pImpactData = nullptr;
    if (GetImpactDataSet()) {
        pImpactData = GetImpactDataSet()->GetImpactDataForMaterial(aehkMatType);
    }
    return pImpactData;
}

int32_t BGSExplosion::GetSoundLevelValue() {
    uint32_t uiValue = 0;
    switch (Data.eSoundLevel) {
        case SOUND_LEVEL_LOUD: uiValue = iSoundLevelLoud.UInt(); break;
        case SOUND_LEVEL_NORMAL: uiValue = iSoundLevelNormal.UInt(); break;
        case SOUND_LEVEL_SILENT: uiValue = iSoundLevelSilent.UInt(); break;
    }
    return uiValue;
}

bool Explosion::GetUnderWater() {
    return bUnderWater;
}
