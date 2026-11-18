#include "Fallout Shared/PackageLocation.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESTrapListener.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

TESTrapListener* TESTrapListener_GetListener(TESObjectREFR* apRef) {
    if (apRef) {
        TESObjectCELL* pParentCell = apRef->GetParentCell();
        if (pParentCell) {
            bhkWorld* pWorld = pParentCell->GetbhkWorld();
            if (pWorld) {
                // The world holds the listener by its bhkTrapListener base.
                return static_cast<TESTrapListener*>(pWorld->pTrapListener);
            }
        }
    }
    return nullptr;
}

void PackageLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 1u);
    apLoadGameBuffer->LoadData(&iRad, 4u);
    int8_t eLocType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (eLocType <= 4) {
#if XFNV_64BIT
            // The ID fills the whole pointer slot until the object is resolved.
            apLoadGameBuffer->LoadFormID(uObject.pObject);
#else
            apLoadGameBuffer->LoadFormID((&uObject)->iFormID);
#endif
        }
        else {
            if (eLocType == 5) {
                apLoadGameBuffer->LoadData(&uObject, 4u);
            }
        }
    }
}

void PACK_LOC_FILE::Endian() {
    ENDIAN(iFormID, false);
    ENDIAN(iRadius, false);
}

PackageLocation::PackageLocation() {
    eLocType = static_cast<uint8_t>(PLT_EDITORLOC);
    iRad = 0;
    uObject.pRef = nullptr;
}

PackageLocation::PackageLocation(const PACK_LOC_FILE* apPLFile)
  : eLocType(static_cast<uint8_t>(apPLFile->eLocType)), iRad(apPLFile->iRadius) {
    switch (eLocType) {
        case 0u:
        case 4u:
        case 5u: uObject.eObjType = static_cast<PACKAGE_OBJECT_TYPE>(apPLFile->iFormID); break;
        case 1u:
            uObject.eObjType = static_cast<PACKAGE_OBJECT_TYPE>(apPLFile->iFormID);
            iRad = 0;
            break;
        case 2u:
        case 3u: uObject.eObjType = POTYPE_NONE; break;
        default: break;
    }
}

PackageLocation::~PackageLocation() {
    eLocType = static_cast<uint8_t>(-1);
    iRad = 0;
    uObject.eObjType = POTYPE_NONE;
}

PACK_LOC_TYPE PackageLocation::GetLocType() const {
    // sign-extend the stored type byte
    return static_cast<PACK_LOC_TYPE>(static_cast<int8_t>(eLocType));
}

TESObjectREFR* PackageLocation::GetLocReference() const {
    if (GetLocType() != PLT_REFERENCE) {
        return nullptr;
    }
    return uObject.pRef;
}

TESObjectCELL* PackageLocation::GetLocCell() const {
    if (GetLocType() != PLT_CELL) {
        return nullptr;
    }
    return uObject.pCell;
}

TESForm* PackageLocation::GetLocObject() const {
    if (eLocType == 4) {
        return uObject.pRef;
    }

    return nullptr;
}

PACKAGE_OBJECT_TYPE PackageLocation::GetLocObjectType() const {
    if (eLocType == 5) {
        return uObject.eObjType;
    }

    return (PACKAGE_OBJECT_TYPE)0u;
}

uint32_t PackageLocation::GetRadius() const {
    // a location with no explicit radius (untyped) or a cell has no search
    // radius to report
    if (eLocType == static_cast<uint8_t>(PLT_NONE) || eLocType == static_cast<uint8_t>(PLT_CELL)) {
        return 0;
    }
    return iRad;
}

// Reads the type twice (the test for the current location, then the switch) and calls GetLocReference and GetLinkedRef
// twice each, once to test and once to take the result, as the game does.
TESObjectREFR* PackageLocation::GetRunningPackageLocReference(MobileObject* apActor) const {
    TESObjectREFR* pRef = nullptr;
    if (GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            pRef = apActor;
        }
        return pRef;
    }

    switch (GetLocType()) {
        case PLT_REFERENCE:
            if (GetLocReference()) {
                pRef = GetLocReference();
            }
            break;
        case PLT_EDITORLOC:
            if (apActor) {
                pRef = apActor;
            }
            break;
        case PLT_OBJECT:
        case PLT_TYPE: {
            // fall back to the actor itself when the process has no location
            TESObjectREFR* pGenericLocation = apActor->GetCurrentAIProcess()->GetGenericLocation();
            if (pGenericLocation) {
                pRef = pGenericLocation;
            }
            else {
                pRef = apActor;
            }
            break;
        }
        case PLT_LINKEDREF:
            if ((apActor) && (apActor->GetLinkedRef())) {
                pRef = apActor->GetLinkedRef();
            }

            break;
        default: break;
    }
    return pRef;
}

bool PackageLocation::IsRefAtLocationTrigger(
    MobileObject* apPackOwner,
    TESObjectREFR* apRef,
    bool& abRefInsideTriggerOut
) const {
    abRefInsideTriggerOut = false;

    TESObjectREFR* pLocReference = GetRunningPackageLocReference(apPackOwner);
    if (!pLocReference) {
        return false;
    }

    // only traps can act as a location trigger
    if (pLocReference->GetObjectReference()->GetFormType() != ACTI_ID) {
        return false;
    }

    // a trap without a listener reports every reference as outside
    TESTrapListener* pListener = TESTrapListener_GetListener(pLocReference);
    if (pListener) {
        abRefInsideTriggerOut = pListener->IsRefInTrigger(apRef, pLocReference);
    }
    return true;
}

void PackageLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iRad, 4u, 0);
    int8_t eLocType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (eLocType <= 4) {
            apSaveGameBuffer->SaveFormID(uObject.pRef, 0);
        }
        else {
            if (eLocType == 5) {
                apSaveGameBuffer->SaveDataEndian(&uObject, 4u, 0);
            }
        }
    }
}

void PackageLocation::LoadGame() {
    int8_t eLocType;
    uint32_t apFormID;
    pSaveLoadGameOLD->LoadGameDataOLD(this, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&iRad, 4u);
    eLocType = (int8_t)this->eLocType;
    if ((this->eLocType & 0x80) == 0) {
        if (((int32_t)eLocType) <= 4) {
            pSaveLoadGameOLD->LoadNumericID(&apFormID, 4u);
            uObject.eObjType = static_cast<PACKAGE_OBJECT_TYPE>(apFormID);
        }
        else {
            if (eLocType == 5) {
                pSaveLoadGameOLD->LoadGameDataOLD(&uObject, 4u);
            }
        }
    }
}

void PackageLocation::SetLocType(PACK_LOC_TYPE aeLoctype) {
    if (aeLoctype != GetLocType()) {
        eLocType = static_cast<uint8_t>(aeLoctype);
        switch (GetLocType()) {
            case PLT_CELL:
                // a cell location stores the cell, not an object
                uObject.pRef = nullptr;
                iRad = 0;
                break;
            case PLT_REFERENCE:
            case PLT_CURRLOC:
            case PLT_EDITORLOC:
            case PLT_OBJECT:
            case PLT_TYPE:
            case PLT_LINKEDREF: uObject.pRef = nullptr; break;
            default: break;
        }
    }
}

void PackageLocation::SetLocReference(TESObjectREFR* apRef) {
    eLocType = static_cast<uint8_t>(PLT_REFERENCE);
    uObject.pRef = apRef;
}

void PackageLocation::SetLocCell(TESObjectCELL* apCell) {
    eLocType = static_cast<uint8_t>(PLT_CELL);
    uObject.pCell = apCell;
}

void PackageLocation::SetLocObject(TESForm* apObj) {
    eLocType = (uint8_t)4;
    uObject.pObject = apObj;
}

void PackageLocation::SetLocObjectType(PACKAGE_OBJECT_TYPE aeType) {
    eLocType = (uint8_t)5;
    uObject.eObjType = (PACKAGE_OBJECT_TYPE)((uint32_t)aeType);
}

void PackageLocation::SetRadius(uint32_t aiRad) {
    // locations without an object search (untyped or cell) keep no radius
    if (eLocType != static_cast<uint8_t>(PLT_NONE) && eLocType != static_cast<uint8_t>(PLT_CELL)) {
        iRad = aiRad;
    }
}

void PackageLocation::Copy(PackageLocation* apCopy, bool abNoneOK) {
    if (!apCopy) {
        return;
    }
    if (!abNoneOK && apCopy->GetLocType() == PLT_NONE) {
        return;
    }

    SetLocType(apCopy->GetLocType());
    switch (GetLocType()) {
        case PLT_REFERENCE:
            SetRadius(apCopy->GetRadius());
            SetLocReference(apCopy->GetLocReference());
            break;
        case PLT_CELL: SetLocCell(apCopy->GetLocCell()); break;
        case PLT_OBJECT:
            SetRadius(apCopy->GetRadius());
            SetLocObject(apCopy->GetLocObject());
            break;
        case PLT_TYPE:
            SetRadius(apCopy->GetRadius());
            SetLocObjectType(apCopy->GetLocObjectType());
            break;
        case PLT_CURRLOC:
        case PLT_EDITORLOC:
        case PLT_LINKEDREF:
        case PLT_FIRSTLOC: SetRadius(apCopy->GetRadius()); break;
        default: return;
    }
}

void PackageLocation::InitItem(TESForm* apOwner) {
    if (GetLocType() != PLT_TYPE) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(uObject.pRef));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, File);
        uint8_t eLocType = this->eLocType;
        if (this->eLocType) {
            if (eLocType == 1) {
                TESForm* pobj = TESForm::GetFormByNumericID(tempint);
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pobj);
                if (pCell) {
                    SetLocCell(pCell);
                }
                else {
                    if (apOwner->GetFormEditorIDLength()) {
                        const char* v5 = apOwner->GetFormEditorID();
                        Warning(
                            "MASTERFILE: Unable to find Package Location Cell (%08X) on owner object \"%s\".", tempint,
                            v5
                        );
                    }
                    else {
                        int32_t CastingType = apOwner->iFormID;
                        Warning(
                            "MASTERFILE: Unable to find Package Location Cell (%08X) on owner object (%08X).", tempint,
                            CastingType
                        );
                    }
                    SetLocCell(nullptr);
                }
            }
            else {
                if (eLocType == 4) {
                    TESForm* pobj_2 = TESForm::GetFormByNumericID(tempint);
                    if (pobj_2) {
                        SetLocObject(pobj_2);
                    }
                    else {
                        if (apOwner->GetFormEditorIDLength()) {
                            const char* v7 = apOwner->GetFormEditorID();
                            Warning(
                                "MASTERFILE: Unable to find Package Location Object (%08X) on owner object \"%s\".",
                                tempint, v7
                            );
                        }
                        else {
                            int32_t v8 = apOwner->iFormID;
                            Warning(
                                "MASTERFILE: Unable to find Package Location Object (%08X) on owner object (%08X).",
                                tempint, v8
                            );
                        }
                        SetLocObject(nullptr);
                    }
                }
            }
        }
        else {
            TESForm* pobj_3 = TESForm::GetFormByNumericID(tempint);
            TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pobj_3);
            if (pRef) {
                SetLocReference(pRef);
            }
            else {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* v9 = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Package Location Reference (%08X) on owner object \"%s\".", tempint,
                        v9
                    );
                }
                else {
                    int32_t v10 = apOwner->iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Package Location Reference (%08X) on owner object (%08X).", tempint,
                        v10
                    );
                }
                SetLocReference(nullptr);
            }
        }
    }
}

bool PackageLocation::IsLocationGeneric() {
    return (bool)((uint8_t)(((int32_t)GetLocType()) > 3));
}

bool PackageLocation::IsRadiusNonZero() {
    return iRad != 0;
}

uint16_t PackageLocation::GetSaveSize() {
    int16_t v3 = 5;
    int8_t ucLocationType = (int8_t)eLocType;
    if ((eLocType & 0x80) != 0) {
        return (uint16_t)v3;
    }
    if (((int32_t)ucLocationType) <= 4) {
        return (uint16_t)9;
    }

    if (ucLocationType == 5) {
        return (uint16_t)9;
    }

    return (uint16_t)v3;
}

void PackageLocation::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    // The saved form ID in uObject becomes the form.
    if (eLocType <= 4u) {
        TESForm* FormByNumericID;
        if (uObject.iFormID) {
            FormByNumericID = TESForm::GetFormByNumericID(uObject.iFormID);
        }
        else {
            FormByNumericID = nullptr;
        }
        uObject.pObject = FormByNumericID;
    }
}

void PackageLocation::InitLoadGame() {
    // The saved game left the object's form ID in the union; it is resolved to the form here.
    if (eLocType <= 4u) {
        uObject.pObject = TESForm::GetFormByNumericID(uObject.iFormID);
    }
}

void PackageLocation::Save(CHUNK_ID aeChunk) {
    PACK_LOC_FILE v3;
    ourMemset(&v3, 0, sizeof(v3));
    v3.eLocType = (int8_t)((int32_t)GetLocType());
    if (((int32_t)GetLocType()) == 5) {
        v3.iFormID = (uint32_t)GetLocObjectType();
    }
    else {
        if (uObject.eObjType) {
            v3.iFormID = uObject.pRef->TESForm::GetFormID();
        }
    }
    v3.iRadius = GetRadius();
    if (TESForm::QEndianSwapOnSave()) {
        (&v3)->Endian();
    }
    TESForm::__AddChunkData(aeChunk, &v3, 0xCu);
    if (TESForm::QEndianSwapOnSave()) {
        (&v3)->Endian();
    }
}

bool PackageLocation::Compare(PackageLocation* apCompare) {
    if (!apCompare) {
        return true;
    }
    int32_t locType = (int32_t)GetLocType();
    if (locType != ((int32_t)apCompare->GetLocType())) {
        return true;
    }
    uint32_t radius = GetRadius();
    if (radius != apCompare->GetRadius()) {
        return true;
    }
    TESObjectREFR* locReference = GetLocReference();
    if (locReference != apCompare->GetLocReference()) {
        return true;
    }
    TESObjectCELL* locCell = GetLocCell();
    if (locCell != apCompare->GetLocCell()) {
        return true;
    }
    TESForm* locObject = GetLocObject();
    if (locObject != apCompare->GetLocObject()) {
        return true;
    }
    uint32_t objectType = (uint32_t)GetLocObjectType();
    return (bool)((uint8_t)(objectType != ((uint32_t)apCompare->GetLocObjectType())));
}

void PackageLocation::SaveGame() {
    int32_t a2;
    pSaveLoadGameOLD->SaveGameDataOLD(this, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&iRad, 4u);
    int8_t eLocType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (eLocType <= 4) {
            a2 = 0;
            if (uObject.eObjType) {
                a2 = uObject.pRef->iFormID;
            }
            pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&a2), 4u);
        }
        else {
            if (eLocType == 5) {
                pSaveLoadGameOLD->SaveGameDataOLD(&uObject, 4u);
            }
        }
    }
}

void PackageLocation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
