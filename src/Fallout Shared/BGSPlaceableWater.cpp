#include "Fallout Shared/BGSPlaceableWater.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "SDK/MSVC/rtti.h"

// Reads the model chunks, the DATA record (8 bytes: the flags and the water type's form ID, of which only the water
// type is kept, the flags are cleared) and the DNAM record. GetTESForm's result is stored as a byte and compared
// zero-extended, hence the uint8_t.
bool BGSPlaceableWater::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != PWAT_ID) {
        return false;
    }
    ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        if (eChunk == MODL_ID || eChunk == MODT_ID) {
            TESModel* pModel = static_cast<TESModel*>(this);
            TESModel::LoadModelChunk(pModel, apFile);
        }
        else if (eChunk == DATA_ID) {
            BGSPlaceableWaterData kData;
            kData.Initialize();
            // LoadData takes the record as raw bytes; the chunk is read straight into the local copy.
            LoadData(apFile, reinterpret_cast<char*>(&kData), 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the water type's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(
                &kData, 1, 8, sizeof(kData), {{4, offsetof(BGSPlaceableWaterData, pWaterType)}}
            );
#endif
            if (apFile->QEndian()) {
                kData.EndianFromFile();
            }
            Data.pWaterType = kData.pWaterType;
            Data.ulFlags = 0;
            SetReflectionEnabled(true);
            SetReflectsLand(true);
            SetReflectsLODLand(true);
            SetReflectsSky(true);
            SetRefractionEnabled(true);
            SetRefractsLand(true);
            SetAutomaticWater(false);
        }
        else if (eChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            uint32_t uiChunkSize = apFile->GetChunkSize();
            char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
            // An empty chunk leaves the buffer unwritten (see above).
            pEditorID[0] = '\0';
            apFile->GetChunkData(pEditorID, 0x200);
            static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
        }
        else if (eChunk == OBND_ID) {
            static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile);
        }
        else if (eChunk == DNAM_ID) {
            apFile->GetChunkData(&Data, 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the water type's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{4, offsetof(BGSPlaceableWaterData, pWaterType)}});
#endif
            // Before form version 9 the flags are not in the record: automatic water is off.
            if (apFile->GetFormVersion() < 9) {
                SetAutomaticWater(false);
            }
            if (apFile->QEndian()) {
                Data.Endian();
            }
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

BGSPlaceableWaterData::BGSPlaceableWaterData() {
    Clear();
}

void BGSPlaceableWaterData::Clear() {
    ulFlags = 0;
    pWaterType = nullptr;
}

void BGSPlaceableWaterData::Endian() {
    ENDIAN(ulFlags, false);
    ENDIAN(pWaterType, false);
}

void BGSPlaceableWaterData::Initialize() {
    ulFlags = static_cast<uint32_t>((ulFlags & 0xFFFF0000u) | (static_cast<uint32_t>(static_cast<uint16_t>(0))));
    pWaterType = nullptr;
}

void BGSPlaceableWaterData::EndianFromFile() {
    // Only the low 16 bits of the flags are swapped.
    ENDIAN(usFlags, false);
    ENDIAN(pWaterType, false);
}

BGSPlaceableWater::BGSPlaceableWater() {
    SetFormType(PWAT_ID);
    BGSPlaceableWater::InitializeData();
}

BGSPlaceableWater::~BGSPlaceableWater() {}

void BGSPlaceableWater::InitializeData() {
    Data.Clear();
    SetReflectionEnabled(true);
    SetReflectsLand(true);
    SetReflectsLODLand(true);
    SetReflectsSky(true);
    SetRefractionEnabled(true);
    SetRefractsLand(true);
    SetAutomaticWater(false);
    SetObjectTexCoordsEnabled(false);
    SetDepthEnabled(true);
    EnableUnderwaterFoggingPass(true);
}

// Writes the model chunks, then DATA with the water type replaced by its form ID (0 for none).
void BGSPlaceableWater::Save() {
    StartForm();
    TESModel::Save(MODL_ID, MODT_ID);
    BGSPlaceableWaterData kData;
    kData.~BGSPlaceableWaterData();
    ::new (&kData) BGSPlaceableWaterData;
    ourMemcpy(&kData, &Data, sizeof(BGSPlaceableWaterData));
    uint32_t uiWaterTypeID;
    if (Data.pWaterType) {
        uiWaterTypeID = Data.pWaterType->GetFormID();
    }
    else {
        uiWaterTypeID = 0;
    }
    kData.pWaterType = reinterpret_cast<TESWaterForm*>(static_cast<uintptr_t>(uiWaterTypeID));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &kData, sizeof(BGSPlaceableWaterData));
    if (TESForm::QEndianSwapOnSave()) {
        kData.Endian();
    }
    CloseForm();
}

void BGSPlaceableWater::InitItem() {
    if (!GetInitialized()) {
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pWaterType));
        if (itempInt) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            Data.pWaterType = dynamic_cast<TESWaterForm*>(FormByNumericID);
        }
        if (!Data.pWaterType) {
            Data.pWaterType = pDefaultWater;
        }
        SetInitialized(true);
    }
}

void BGSPlaceableWater::Copy(TESForm* apCopy) {
    BGSPlaceableWater* v5 = dynamic_cast<BGSPlaceableWater*>(apCopy);
    if (v5) {
        TESForm::CopyAllComponents(v5);
        ourMemcpy(&Data, &v5->Data, sizeof(Data));
    }
}

bool BGSPlaceableWater::Compare(TESForm* apCompare) {
    BGSPlaceableWater* v5 = dynamic_cast<BGSPlaceableWater*>(apCompare);
    if (!v5) {
        return true;
    }
    if (TESForm::CompareAllComponents(v5)) {
        return true;
    }
    return (std::memcmp(&Data, &v5->Data, sizeof(Data)) != 0);
}

bool BGSPlaceableWater::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    TESWaterForm* pwaterForm = GetWaterType();
    if (!pwaterForm) {
        return false;
    }
    if (apActionRef && apActionRef->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apActionRef);
        TESIdleManager::SetUsedItem(this);
        TESIdleManager::SetUsedItemActivate(true);
        double EyeLevel = pActor->Actor::GetEyeLevel();
        double v10 = (&pActor->GetLocationOnReference())->z + EyeLevel;
        float v15 = v10 - pActor->GetReach();
        float z = (&apItemActivated->GetLocationOnReference())->z;
        int32_t ilevel = apItemActivated->IsNpc() || v15 < static_cast<double>(z);
        TESIdleManager::SetUsedItemLevel(ilevel);
        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
        char banim = pProcess->SetupSpecialIdle(pActor, nullptr, AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE, true, false, true);
        TESIdleManager::SetUsedItem(nullptr);
        TESIdleManager::SetUsedItemActivate(false);
        TESIdleManager::SetUsedItemLevel(-1);
        if (!banim) {
            return false;
        }
        if (apActionRef == pPlayer) {
            pCurrentWater = pwaterForm;
            pPlayer->SetIsDrinkingPlacedWater(true);
            Animation* Animation_0 = pPlayer->GetAnimation(true);
            Animation_0->ClearGroup(AGS_MOVEMENT, 0.0f);
            Animation* v8 = pPlayer->GetAnimation(false);
            v8->ClearGroup(AGS_MOVEMENT, 0.0f);
        }
    }
    return pwaterForm->Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aiCount);
}

TESWaterForm* BGSPlaceableWater::GetWaterType() {
    if (Data.pWaterType && (GetInitialized() && Data.pWaterType->pRemappedWaterForm)) {
        return Data.pWaterType->pRemappedWaterForm;
    }

    return Data.pWaterType;
}

bool BGSPlaceableWater::IsAutomaticWater() {
    return (bool)((uint8_t)GetFlag(1073741824));
}

bool BGSPlaceableWater::IsLODWater() {
    return (bool)((uint8_t)GetFlag(134217728));
}

void BGSPlaceableWater::SetFlag(BGSPlaceableWaterFlags aeFlag, bool abValue) {
    if (abValue) {
        Data.ulFlags |= aeFlag;
    }
    else {
        Data.ulFlags &= ~aeFlag;
    }
}

void BGSPlaceableWater::EnableUnderwaterFoggingPass(bool abValue) {
    SetFlag(DISABLE_FOGGING_PASS, abValue == 0);
}

bool BGSPlaceableWater::GetFlag(uint32_t aiValue) {
    return ((aiValue & Data.ulFlags) != 0);
}

bool BGSPlaceableWater::IsDepthEnabled() {
    return (bool)((uint8_t)GetFlag(268435456));
}

void BGSPlaceableWater::SetReflectionEnabled(bool abValue) {
    SetFlag(REFLECTION_ENABLED, abValue);
}

void BGSPlaceableWater::SetReflectsLand(bool abValue) {
    SetFlag(REFLECTS_LAND, abValue);
}

void BGSPlaceableWater::SetReflectsLODLand(bool abValue) {
    SetFlag(REFLECTS_LOD_LAND, abValue);
}

void BGSPlaceableWater::SetReflectsSky(bool abValue) {
    SetFlag(REFLECTS_SKY, abValue);
}

void BGSPlaceableWater::SetRefractionEnabled(bool abValue) {
    SetFlag(REFRACTION_ENABLED, abValue);
}

void BGSPlaceableWater::SetRefractsLand(bool abValue) {
    SetFlag(REFRACTS_LAND, abValue);
}

void BGSPlaceableWater::SetAutomaticWater(bool abValue) {
    SetFlag(AUTOMATIC_WATER, abValue);
}

void BGSPlaceableWater::SetObjectTexCoordsEnabled(bool abValue) {
    SetFlag(ENABLE_OBJECT_TEXCOORD, abValue);
}

void BGSPlaceableWater::SetDepthEnabled(bool abValue) {
    SetFlag(ENABLE_DEPTH, abValue);
}

void BGSPlaceableWater::SetReflectsLODBuildings(bool abValue) {
    SetFlag(REFLECTS_LOD_BUILDINGS, abValue);
}

void BGSPlaceableWater::SetReflectsLODTrees(bool abValue) {
    SetFlag(REFLECTS_LOD_TREES, abValue);
}

void BGSPlaceableWater::SetRefractsActors(bool abValue) {
    SetFlag(REFRACTS_ACTORS, abValue);
}

void BGSPlaceableWater::SetReflectsActors(bool abValue) {
    SetFlag(REFLECTS_ACTORS, abValue);
}

void BGSPlaceableWater::SetReflectsNoReference(bool abValue) {
    SetFlag(REFLECTS_NO_REFERENCE, abValue);
}

void BGSPlaceableWater::SetReflectsDeadActors(bool abValue) {
    SetFlag(REFLECTS_DEAD_ACTORS, abValue);
}

void BGSPlaceableWater::SetLODWater(bool abValue) {
    SetFlag(LOD_WATER, abValue);
}

int32_t BGSPlaceableWater::GetUnderwaterFog() {
    return (int32_t)((uint8_t)(!GetFlag((-2147483647 - 1))));
}

bool BGSPlaceableWater::GetObjectTexCoordsEnabled() {
    return (bool)((uint8_t)GetFlag(536870912));
}

TESWaterListener* BGSPlaceableWater::GetWaterListener(TESObjectREFR* apRef) {
    bhkWorld* v2;
    TESObjectCELL* parentCell;
    TESWaterListener* v4 = nullptr;
    if (apRef) {
        parentCell = apRef->TESObjectREFR::GetParentCell();
        if (parentCell) {
            v2 = parentCell->GetbhkWorld();
            if (v2) {
                return static_cast<TESWaterListener*>(v2->GetWaterListener());
            }
        }
    }
    return v4;
}
