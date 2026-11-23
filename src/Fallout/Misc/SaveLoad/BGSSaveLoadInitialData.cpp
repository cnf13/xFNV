#include "Fallout/Misc/SaveLoad/BGSSaveLoadInitialData.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormIDMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/InteriorCellInitialData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

INITIAL_DATA_TYPE
BGSSaveLoadInitialData::GetInitialDataType(uint32_t aiFormID, int32_t aeFormType, BGSChangeFlags aChangeFlags) {
    if (TESObjectREFR::IsReferenceFormType(aeFormType)) {
        if (pDataHandler->IsFormIDCreated(aiFormID)) {
            return INITIAL_DATA_CREATED_REFERENCE;
        }
        if (aChangeFlags.CheckFlags(CHANGE_REFR_CELL_CHANGED)) {
            return INITIAL_DATA_MOVED_REFERENCE;
        }
        if (aChangeFlags.CheckFlags(CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) {
            return INITIAL_DATA_REFERENCE_LOCATION;
        }
        return INITIAL_DATA_NONE;
    }
    if (aeFormType != CELL_ID || !aChangeFlags.CheckFlags(CHANGE_CELL_DETACHTIME)) {
        return INITIAL_DATA_NONE;
    }
    if (aChangeFlags.CheckFlags(CHANGE_CELL_EXTERIOR_CHAR)) {
        return INITIAL_DATA_EXTERIOR_CELL_CHAR;
    }
    if (aChangeFlags.CheckFlags(CHANGE_CELL_EXTERIOR_SHORT)) {
        return INITIAL_DATA_EXTERIOR_CELL_SHORT;
    }
    return INITIAL_DATA_INTERIOR_CELL;
}

INITIAL_DATA_TYPE BGSSaveLoadInitialData::GetInitialDataType(TESForm* apForm, BGSChangeFlags aiChangeFlags) {
    const ENUM_FORM_ID eFormType = apForm->GetFormType();
    uint32_t iFormID = apForm->GetFormID();
    return BGSSaveLoadInitialData::GetInitialDataType(iFormID, eFormType, aiChangeFlags);
}

uint32_t BGSSaveLoadInitialData::GetInitialDataTypeSize(INITIAL_DATA_TYPE aeType) {
    return kInitialDataTypeData[aeType].cSize;
}

bool BGSSaveLoadInitialData::IsInitialDataRefType(INITIAL_DATA_TYPE aeType) {
    return kInitialDataTypeData[aeType].bRefType;
}

uint32_t BGSSaveLoadInitialData::GetInitialDataTypeDataSize(INITIAL_DATA_TYPE aeType) {
    return BGSSaveLoadInitialData::GetInitialDataTypeSize(aeType) + 1;
}

bool BGSSaveLoadInitialData::IsInitialDataCellType(INITIAL_DATA_TYPE aeType) {
    return kInitialDataTypeData[aeType].bCellType;
}

// Writes the form's initial data by its type, then its saved Havok state when the change flags ask for it: the
// stored sub-buffer when the reference has no 3D, else the live state.
INITIAL_DATA_TYPE BGSSaveLoadInitialData::SaveInitialData(
    BGSSaveFormBuffer* apSaveGameBuffer,
    TESForm* apForm,
    BGSChangeFlags aiChangeFlags
) {
    INITIAL_DATA_TYPE eType = BGSSaveLoadInitialData::GetInitialDataType(apForm, aiChangeFlags);
    switch (eType) {
        case INITIAL_DATA_EXTERIOR_CELL_CHAR: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(apForm);
            ExteriorCellInitialDataChar kData;
            TESWorldSpace* pWorldSpace = pCell->GetWorldSpace();
            kData.sWorldSpaceIDIndex = pSaveLoadGame->pWorldspaceFormIDMap->AddFormID(pWorldSpace->iFormID);
            kData.cCellX = static_cast<char>(pCell->GetDataX());
            kData.cCellY = static_cast<char>(pCell->GetDataY());
            kData.iDetachTime = static_cast<int32_t>(pCell->GetDetachTime());
            apSaveGameBuffer->SaveDataEndian(&kData, sizeof(ExteriorCellInitialDataChar), 0);
            break;
        }
        case INITIAL_DATA_EXTERIOR_CELL_SHORT: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(apForm);
            ExteriorCellInitialDataShort kData;
            TESWorldSpace* pWorldSpace = pCell->GetWorldSpace();
            kData.sWorldSpaceIDIndex = pSaveLoadGame->pWorldspaceFormIDMap->AddFormID(pWorldSpace->iFormID);
            kData.sCellX = static_cast<int16_t>(pCell->GetDataX());
            kData.sCellY = static_cast<int16_t>(pCell->GetDataY());
            kData.iDetachTime = static_cast<int32_t>(pCell->GetDetachTime());
            apSaveGameBuffer->SaveDataEndian(&kData, sizeof(ExteriorCellInitialDataShort), 0);
            break;
        }
        case INITIAL_DATA_INTERIOR_CELL: {
            TESObjectCELL* pCell = static_cast<TESObjectCELL*>(apForm);
            uint32_t iDetachTime = pCell->GetDetachTime();
            apSaveGameBuffer->SaveDataEndian(&iDetachTime, sizeof(iDetachTime), 0);
            break;
        }
        case INITIAL_DATA_REFERENCE_LOCATION: {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
            TESForm* pLocation = pRef->GetWorldSpace();
            if (!pLocation) {
                pLocation = pRef->GetParentCell();
            }
            NiPoint3 kLocation = *pRef->QLocationOnReference();
            NiPoint3 kAngle = pRef->GetAngleOnReference();
            if (!pLocation && !pRef->GetEditorLocation(kLocation, kAngle, pLocation, nullptr)) {
                pLocation = pPlayer->GetWorldSpace();
                if (!pLocation) {
                    pLocation = pPlayer->GetParentCell();
                }
                kLocation = *pRef->QLocationOnReference();
                kAngle = pRef->GetAngleOnReference();
            }
            ReferenceLocationInitialData kRefLocationData;
            kRefLocationData.LocationIDIndex.SetNumericID(pLocation->iFormID);
            kRefLocationData.Location = kLocation;
            kRefLocationData.Angle = kAngle;
            apSaveGameBuffer->SaveDataEndian(&kRefLocationData, sizeof(ReferenceLocationInitialData), 0);
            break;
        }
        case INITIAL_DATA_CREATED_REFERENCE: {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
            TESForm* pLocation = pRef->GetWorldSpace();
            if (!pLocation) {
                pLocation = pRef->GetParentCell();
            }
            CreatedReferenceInitialData kCreatedRefData;
            kCreatedRefData.RefLocationData.LocationIDIndex.SetNumericID(pLocation->iFormID);
            kCreatedRefData.RefLocationData.Location = pRef->GetLocationOnReference();
            kCreatedRefData.RefLocationData.Angle = pRef->GetAngleOnReference();
            TESBoundObject* pObjectReference = pRef->GetObjectReference();
            // The leveled actor's original base is saved in place of its base.
            if (pRef->IsActor() && aiChangeFlags.CheckFlags(CHANGE_ACTOR_EXTRA_LEVELED_ACTOR)) {
                TESActorBase* pOriginalActorBase = pRef->m_Extra.GetLevCreaOriginalBase();
                if (pOriginalActorBase) {
                    pObjectReference = pOriginalActorBase;
                }
            }
            kCreatedRefData.BoundIDIndex.SetNumericID(pObjectReference->iFormID);
            kCreatedRefData.cCreatedReferenceFlags = 1;
            if (pRef->GetRefPersists()) {
                kCreatedRefData.cCreatedReferenceFlags |= CREATED_REFERENCE_PERSISTENT;
            }
            if (pRef->IsMobileObject() && static_cast<MobileObject*>(pRef)->IsTalkingActivator()) {
                kCreatedRefData.cCreatedReferenceFlags |= CREATED_REFERENCE_TALKING_ACTIVATOR;
            }

            apSaveGameBuffer->SaveDataEndian(&kCreatedRefData, sizeof(CreatedReferenceInitialData), 0);
            break;
        }
        case INITIAL_DATA_MOVED_REFERENCE: {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
            TESForm* pLocation = pRef->GetWorldSpace();
            if (!pLocation) {
                pLocation = pRef->GetParentCell();
            }
            NiPoint3 kLocation = *pRef->QLocationOnReference();
            NiPoint3 kAngle = pRef->GetAngleOnReference();
            if (!pLocation && !pRef->GetEditorLocation(kLocation, kAngle, pLocation, nullptr)) {
                pLocation = pPlayer->GetWorldSpace();
                if (!pLocation) {
                    pLocation = pPlayer->GetParentCell();
                }
                kLocation = *pRef->QLocationOnReference();
                kAngle = pRef->GetAngleOnReference();
            }
            MovedReferenceInitialData kMovedRefData;
            kMovedRefData.RefLocationData.LocationIDIndex.SetNumericID(pLocation->iFormID);
            kMovedRefData.RefLocationData.Location = kLocation;
            kMovedRefData.RefLocationData.Angle = kAngle;
            NiPoint3 kEditorLocation;
            NiPoint3 kEditorAngle;
            TESForm* pEditorLocationForm = nullptr;
            if (pRef->GetEditorLocation(kEditorLocation, kEditorAngle, pEditorLocationForm, nullptr)) {
                kMovedRefData.OriginalLocationIDIndex.SetNumericID(pEditorLocationForm->iFormID);
                kMovedRefData.sOriginalCellX = FloatToInt(kEditorLocation.x) >> 12;
                kMovedRefData.sOriginalCellY = FloatToInt(kEditorLocation.y) >> 12;
            }
            else {
                kMovedRefData.sOriginalCellX = 0;
                kMovedRefData.sOriginalCellY = 0;
                // The arguments are evaluated from the last to the first.
                TESBoundObject* pObjectReference = pRef->GetObjectReference();
                uint32_t iObjectID = pRef->GetObjectReference()->iFormID;
                const char* pObjectEditorID = pObjectReference->GetFormEditorID();
                uint32_t iRefID = pRef->iFormID;
                const char* pRefEditorID = pRef->GetFormEditorID();
                SaveGameWarning(
                    "EDITOR: Could not get editor location for reference '%s' (%08X) of type '%s' (%08X)",
                    pRefEditorID,
                    iRefID,
                    pObjectEditorID,
                    iObjectID
                );
            }
            apSaveGameBuffer->SaveDataEndian(&kMovedRefData, sizeof(MovedReferenceInitialData), 0);
            break;
        }
        default: break;
    }
    if (apForm->IsReference() && aiChangeFlags.CheckFlags(CHANGE_REFR_HAVOK_MOVE)) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
        BGSLoadGameSubBuffer kSavedHavokData(pRef->m_Extra.GetSavedHavokData());
        if (!pRef->Get3D() && kSavedHavokData.pBuffer) {
            kSavedHavokData.SaveGame(apSaveGameBuffer);
        }
        else {
            uint32_t iSizePosition = apSaveGameBuffer->StartVariableSizedValue();
            uint32_t iPosition1 = apSaveGameBuffer->iBufferPosition;
            pRef->SaveHavokData(apSaveGameBuffer);
            uint32_t iPosition2 = apSaveGameBuffer->iBufferPosition;
            apSaveGameBuffer->SaveVariableSizedValue(iPosition2 - iPosition1, iSizePosition);
        }
    }
    return eType;
}

// The cell types only store their detach time. The reference types read their location data, then place the reference:
// a created one is made at its saved location, or moved into its saved cell, and a located or moved one is moved to its
// saved location.
INITIAL_DATA_TYPE BGSSaveLoadInitialData::LoadInitialData(
    BGSLoadFormBuffer* apLoadGameBuffer,
    TESForm* apForm,
    BGSChangeFlags aiChangeFlags
) {
    INITIAL_DATA_TYPE eType = GetInitialDataType(apForm, aiChangeFlags);
    if (IsInitialDataCellType(eType)) {
        CellInitialData kCellData;
        LoadInitialDataStruct(apLoadGameBuffer, eType, &kCellData);
        static_cast<TESObjectCELL*>(apForm)->SetDetachTime(static_cast<uint32_t>(kCellData.iDetachTime), true);
        return eType;
    }
    if (!IsInitialDataRefType(eType)) {
        if (eType != INITIAL_DATA_NONE) {
            SaveGameWarning("SAVELOAD: Unknown initial data type %i", eType);
        }
        return eType;
    }
    ReferenceInitialData kRefData;
    LoadInitialDataStruct(apLoadGameBuffer, eType, &kRefData);
    TESObjectCELL* pCell = nullptr;
    TESWorldSpace* pWorldSpace = nullptr;
    kRefData.GetLocationCellAndWorld(pCell, pWorldSpace);
    TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
    if (eType == INITIAL_DATA_CREATED_REFERENCE) {
        TESForm* pBoundForm = TESForm::GetFormByNumericID(kRefData.iBoundID);
        TESBoundObject* pBoundObject = dynamic_cast<TESBoundObject*>(pBoundForm);
        if (kRefData.cCreatedReferenceFlags & CREATED_REFERENCE_TALKING_ACTIVATOR) {
            pRef->SetLocationOnReference(kRefData.Location);
            pRef->SetAngleOnReference(kRefData.Angle);
            if (pCell) {
                pRef->SetParentCell(pCell);
            }
            else if (pWorldSpace) {
                // Reads the world space's interior cell before the reference's extra data.
                TESObjectCELL* pInterior = pWorldSpace->QPersistentCell();
                ExtraDataList* pExtra = pRef->GetExtra();
                pExtra->SetPersistentCell(pInterior);
            }
        }
        else {
            pDataHandler->CreateReferenceAtLocation(
                pBoundObject,
                kRefData.Location,
                kRefData.Angle,
                pCell,
                pWorldSpace,
                pRef,
                nullptr,
                nullptr
            );
        }
        if (pCell || pWorldSpace) {
            pRef->Update3DPosition();
            if (pRef->IsActor()) {
                pSaveLoadGame->CancelInitPackageLocations(pRef);
            }
        }
        else {
            const char* pTypeName = pBoundObject ? pBoundObject->GetFormTypeString() : "UNKNOWN";
            SaveGameWarning(
                "Created reference %08X with bound object %08X of type %s is loading into a "
                "non-existent cell/worldspace and will be deleted",
                pRef->GetFormID(),
                kRefData.iBoundID,
                pTypeName
            );
            pRef->SetStillLoading(false);
            pSaveLoadGame->DeleteForm(pRef);
            apLoadGameBuffer->SetForm(nullptr);
            return eType;
        }
    }
    else if (eType == INITIAL_DATA_REFERENCE_LOCATION || eType == INITIAL_DATA_MOVED_REFERENCE) {
        if (pRef == pPlayer) {
            if (!pSaveLoadGame->GetPlayerLocationInvalid()) {
                pRef->SetAngleOnReference(kRefData.Angle);
                pRef->SetLocationOnReference(kRefData.Location);
            }
        }
        else {
            pRef->SetAngleOnReference(kRefData.Angle);
            pRef->SetLocationOnReference(kRefData.Location);
        }
        pRef->Update3DPosition();
        if (pRef != pPlayer) {
            if (pCell && pCell->GetInterior()) {
                if (pCell != pRef->GetParentCell()) {
                    TESObjectREFR::MoveRefToNewSpace(pRef, pCell, nullptr);
                }
            }
            else if (pWorldSpace) {
                if ((pCell && (pCell == pRef->GetParentCell() || TES::pInstance->IsCellLoaded(pCell, false))) ||
                    pRef->GetRefPersists()) {
                    if (!pCell || pCell != pRef->GetParentCell()) {
                        TESObjectREFR::MoveRefToNewSpace(pRef, nullptr, pWorldSpace);
                    }
                }
                else {
                    SaveGameWarning(
                        "Trying to load non-persistent reference '%s' (%08X) into unloaded cell in "
                        "worldspace '%s' (%08X)",
                        pRef->GetFormEditorID(),
                        pRef->GetFormID(),
                        pWorldSpace->GetFormEditorID(),
                        pWorldSpace->GetFormID()
                    );
                    apLoadGameBuffer->SetCellChanged(true);
                }
            }
            else {
                SaveGameWarning(
                    "Could not find world or cell %08X for reference '%s' (%08X)",
                    kRefData.iLocationID,
                    pRef->GetFormEditorID(),
                    pRef->GetFormID()
                );
            }
            if (!pRef->Get3D() || (pCell && TES::pInstance->IsCellLoaded(pCell, false))) {
                if (pRef->Get3D() && pRef->IsActor()) {
                    pRef->InitHavok();
                }
            }
            else {
                pRef->Set3D(nullptr, false);
            }
        }
        if (pRef->IsActor()) {
            Actor* pActor = static_cast<Actor*>(pRef);
            pSaveLoadGame->CancelInitPackageLocations(pRef);
            if (eType == INITIAL_DATA_MOVED_REFERENCE && !pActor->HasEditorLocation()) {
                pActor->pEditorLocForm = TESForm::GetFormByNumericID(kRefData.iOriginalLocationID);
            }
        }
    }
    if (aiChangeFlags.CheckFlags(CHANGE_REFR_HAVOK_MOVE)) {
        pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_HAVOK_DATA, apLoadGameBuffer, nullptr);
    }
    return eType;
}

uint32_t BGSSaveLoadInitialData::LoadInitialDataStruct(
    BGSLoadFormBuffer* apLoadGameBuffer,
    INITIAL_DATA_TYPE aeType,
    void* apData
) {
    if (aeType == INITIAL_DATA_NONE) {
        return 0;
    }
    char apDataa[128];
    uint32_t aiSize = BGSSaveLoadInitialData::GetInitialDataTypeSize(aeType);
    apLoadGameBuffer->LoadData(apDataa, aiSize);
    return BGSSaveLoadInitialData::LoadInitialDataStruct(apDataa, aeType, apData);
}

uint32_t BGSSaveLoadInitialData::LoadInitialDataStruct(
    BGSSaveLoadFile* apFile,
    INITIAL_DATA_TYPE aeType,
    void* apInitialDataBuffer
) {
    char apBuffer[128];
    if (aeType == INITIAL_DATA_NONE) {
        return 0;
    }
    uint32_t aiBytes = BGSSaveLoadInitialData::GetInitialDataTypeDataSize(aeType);
    apFile->Read(apBuffer, aiBytes);
    return BGSSaveLoadInitialData::LoadInitialDataStruct(apBuffer, aeType, apInitialDataBuffer);
}

uint32_t
BGSSaveLoadInitialData::LoadInitialDataStruct(char* apBuffer, INITIAL_DATA_TYPE aeType, void* apInitialDataBuffer) {
    InteriorCellInitialData kInteriorData;
    ExteriorCellInitialDataShort kShortData;
    ExteriorCellInitialDataChar kCharData;
    uint32_t uiSize = BGSSaveLoadInitialData::GetInitialDataTypeSize(aeType);
    switch (aeType) {
        case INITIAL_DATA_EXTERIOR_CELL_CHAR: {
            ourMemcpy(&kCharData, apBuffer, uiSize);
            static_cast<CellInitialData*>(apInitialDataBuffer)->Initialize_CHAR(&kCharData);
            break;
        }
        case INITIAL_DATA_EXTERIOR_CELL_SHORT: {
            ourMemcpy(&kShortData, apBuffer, uiSize);
            static_cast<CellInitialData*>(apInitialDataBuffer)->Initialize_SHORT(&kShortData);
            break;
        }
        case INITIAL_DATA_INTERIOR_CELL: {
            ourMemcpy(&kInteriorData, apBuffer, uiSize);
            static_cast<CellInitialData*>(apInitialDataBuffer)->Initialize_INTERIOR(&kInteriorData);
            break;
        }
        case INITIAL_DATA_REFERENCE_LOCATION: {
            ReferenceLocationInitialData kLocationData;
            ourMemcpy(&kLocationData, apBuffer, uiSize);
            static_cast<ReferenceInitialData*>(apInitialDataBuffer)->Initialize_LOCATION(&kLocationData);
            break;
        }
        case INITIAL_DATA_CREATED_REFERENCE: {
            CreatedReferenceInitialData kCreatedData;
            ourMemcpy(&kCreatedData, apBuffer, uiSize);
            static_cast<ReferenceInitialData*>(apInitialDataBuffer)->Initialize_CREATED(&kCreatedData);
            break;
        }
        case INITIAL_DATA_MOVED_REFERENCE: {
            MovedReferenceInitialData kMovedData;
            ourMemcpy(&kMovedData, apBuffer, uiSize);
            static_cast<ReferenceInitialData*>(apInitialDataBuffer)->Initialize_MOVED(&kMovedData);
            break;
        }
        default: {
            break;
        }
    }
    return BGSSaveLoadInitialData::GetInitialDataTypeDataSize(aeType);
}

TESObjectREFR* BGSSaveLoadInitialData::CreateReference(
    uint32_t aiFormID,
    ReferenceInitialData* apReferenceInitialData,
    TESForm* apPreExistingForm
) {
    TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apPreExistingForm);
    if (!apPreExistingForm) {
        pRef = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(aiFormID));
    }
    if (pDataHandler->IsFormIDCreated(apReferenceInitialData->iBoundID)) {
        apReferenceInitialData->iBoundID = 0;
    }
    TESForm* pBoundForm = TESForm::GetFormByNumericID(apReferenceInitialData->iBoundID);
    TESBoundObject* pBoundObject = dynamic_cast<TESBoundObject*>(pBoundForm);
    if (!pBoundObject) {
        SaveGameWarning(
            "SAVELOAD: Bound object %08X no longer exists.  Reference %08X will not be created.",
            apReferenceInitialData->iBoundID,
            aiFormID
        );
        return nullptr;
    }
    if (pRef) {
        bool bClear = false;
        if (pRef->IsReference()) {
            if (pRef->GetObjectReference() != pBoundObject &&
                (!pRef->IsActor() || pRef->m_Extra.GetLevCreaOriginalBase() != pBoundObject)) {
                bClear = true;
            }
        }
        else {
            bClear = true;
        }
        if (bClear) {
            pSaveLoadGame->ClearForm(pRef);
            pRef = nullptr;
        }
    }
    uint8_t cCreatedType = apReferenceInitialData->cCreatedReferenceFlags;
    if (!pRef) {
        TESObjectREFR* pNewRef = nullptr;
        ENUM_FORM_ID eRefFormType = REFR_ID;
        switch (pBoundObject->GetFormType()) {
            case NPC__ID: eRefFormType = ACHR_ID; break;
            case CREA_ID: eRefFormType = ACRE_ID; break;
            case PROJ_ID: eRefFormType = static_cast<BGSProjectile*>(pBoundObject)->GetProjectileRefFormType(); break;
            case EXPL_ID: {
                pNewRef = new Explosion;
                break;
            }
            default: break;
        }
        if (cCreatedType & CREATED_REFERENCE_TALKING_ACTIVATOR) {
            eRefFormType = TACT_ID;
        }
        if (cCreatedType & 3) {
            if (!pNewRef) {
                pNewRef = TESObjectREFR::CreateReference(static_cast<uint8_t>(eRefFormType), true);
            }
            pNewRef->SetObjectReference(pBoundObject);
            pNewRef->SetFormID(aiFormID, true);
            pRef = pNewRef;
        }
        else {
            SaveGameWarning(
                "SAVELOAD: Invalid created reference type %i for form %08X with bound object %08X in "
                "location %08X",
                cCreatedType,
                aiFormID,
                apReferenceInitialData->iBoundID,
                apReferenceInitialData->iLocationID
            );
        }
    }
    if (pRef && (cCreatedType & CREATED_REFERENCE_PERSISTENT)) {
        pRef->SetRefPersists(true);
    }
    if (pRef) {
        pRef->SetStillLoading(true);
    }
    return pRef;
}

ReferenceLocationInitialData::ReferenceLocationInitialData() {}

void ReferenceInitialData::Initialize_LOCATION(ReferenceLocationInitialData* apData) {
    iLocationID = apData->LocationIDIndex.GetNumericID();
    iBoundID = 0;
    iOriginalLocationID = 0;
    iOriginalCellX = 0;
    iOriginalCellY = 0;
    Location = apData->Location;
    Angle = apData->Angle;
    cCreatedReferenceFlags = 0;
}

bool ReferenceInitialData::GetLocationCellAndWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    TESWorldSpace* pLocation = static_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(iLocationID));
    bool bFound = GetLocationCellOrWorld(pLocation, arpCell, arpWorld);
    if (arpWorld) {
        int32_t iX = FloatToInt(Location.x) >> 12;
        int32_t iY = FloatToInt(Location.y) >> 12;
        arpCell = arpWorld->GetCellFromCellCoord(iX, iY);
    }
    return bFound;
}

bool ReferenceInitialData::GetOriginalLocationCellAndWorld(TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    TESWorldSpace* pLocation = static_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(iOriginalLocationID));
    bool bFound = GetLocationCellOrWorld(pLocation, arpCell, arpWorld);
    if (arpWorld) {
        arpCell = arpWorld->GetCellFromCellCoord(iOriginalCellX, iOriginalCellY);
    }
    return bFound;
}

void ReferenceInitialData::Initialize_CREATED(CreatedReferenceInitialData* apData) {
    iLocationID = apData->RefLocationData.LocationIDIndex.GetNumericID();
    iBoundID = apData->BoundIDIndex.GetNumericID();
    iOriginalLocationID = 0;
    iOriginalCellX = 0;
    iOriginalCellY = 0;
    Location = apData->RefLocationData.Location;
    Angle = apData->RefLocationData.Angle;
    cCreatedReferenceFlags = apData->cCreatedReferenceFlags;
}

void ReferenceInitialData::Initialize_MOVED(MovedReferenceInitialData* apData) {
    iLocationID = apData->RefLocationData.LocationIDIndex.GetNumericID();
    iBoundID = 0;
    iOriginalLocationID = apData->OriginalLocationIDIndex.GetNumericID();
    iOriginalCellX = apData->sOriginalCellX;
    iOriginalCellY = apData->sOriginalCellY;
    Location = apData->RefLocationData.Location;
    Angle = apData->RefLocationData.Angle;
    cCreatedReferenceFlags = 0;
}

void CellInitialData::Initialize_INTERIOR(InteriorCellInitialData* apData) {
    iCellX = 0;
    iCellY = 0;
    iDetachTime = apData->iDetachTime;
    iWorldSpaceID = 0;
}

void CellInitialData::Initialize_CHAR(ExteriorCellInitialDataChar* apData) {
    iCellX = apData->cCellX;
    iCellY = apData->cCellY;
    iDetachTime = apData->iDetachTime;
    uint32_t uiWorldSpaceIDIndex = apData->sWorldSpaceIDIndex;
    iWorldSpaceID = pSaveLoadGame->pWorldspaceFormIDMap->GetFormID(uiWorldSpaceIDIndex);
}

void CellInitialData::Initialize_SHORT(ExteriorCellInitialDataShort* apData) {
    iCellX = apData->sCellX;
    iCellY = apData->sCellY;
    iDetachTime = apData->iDetachTime;
    uint32_t uiWorldSpaceIDIndex = apData->sWorldSpaceIDIndex;
    iWorldSpaceID = pSaveLoadGame->pWorldspaceFormIDMap->GetFormID(uiWorldSpaceIDIndex);
}

CreatedReferenceInitialData::CreatedReferenceInitialData() {}

MovedReferenceInitialData::MovedReferenceInitialData() {}

BGSNumericIDIndex::BGSNumericIDIndex() : cData1(0), cData2(0), cData3(0) {}
