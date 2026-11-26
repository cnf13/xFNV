#include "Fallout Shared/PackageLocation.h"

#include "BSCore/NewOverloads.h"
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

void PackageLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&eLocType, sizeof(eLocType));
    apLoadGameBuffer->LoadData(&iRad, sizeof(iRad));
    int8_t cLocationType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (cLocationType <= 4) {
            // The ID fills the whole pointer slot until the object is resolved.
            apLoadGameBuffer->LoadFormID(uObject.pObject);
        }
        else if (cLocationType == 5) {
            apLoadGameBuffer->LoadData(&uObject, 4u);
        }
    }
}

void PACK_LOC_FILE::Endian() {
    BSEndian(iFormID, false);
    BSEndian(iRadius, false);
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

    return POTYPE_NONE;
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
// twice each, once to test and once to take the result.
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
            if (apActor && apActor->GetLinkedRef()) {
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
    TESTrapListener* pListener = TESTrapListener::GetListener(pLocReference);
    if (pListener) {
        abRefInsideTriggerOut = pListener->IsRefInTrigger(apRef, pLocReference);
    }
    return true;
}

void PackageLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&eLocType, sizeof(eLocType), 0);
    apSaveGameBuffer->SaveDataEndian(&iRad, sizeof(iRad), 0);
    int8_t cLocationType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (cLocationType <= 4) {
            apSaveGameBuffer->SaveFormID(uObject.pRef, 0);
        }
        else if (cLocationType == 5) {
            apSaveGameBuffer->SaveDataEndian(&uObject, 4u, 0);
        }
    }
}

void PackageLocation::LoadGame() {
    uint32_t uiFormID;
    pSaveLoadGameOLD->LoadGameDataOLD(&eLocType, sizeof(eLocType));
    pSaveLoadGameOLD->LoadGameDataOLD(&iRad, sizeof(iRad));
    int8_t cLocationType = static_cast<int8_t>(this->eLocType);
    if ((this->eLocType & 0x80) == 0) {
        if (cLocationType <= PLT_OBJECT) {
            pSaveLoadGameOLD->LoadNumericID(&uiFormID, 4u);
            uObject.eObjType = static_cast<PACKAGE_OBJECT_TYPE>(uiFormID);
        }
        else if (cLocationType == 5) {
            pSaveLoadGameOLD->LoadGameDataOLD(&uObject, 4u);
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
    eLocType = PLT_OBJECT;
    uObject.pObject = apObj;
}

void PackageLocation::SetLocObjectType(PACKAGE_OBJECT_TYPE aeType) {
    eLocType = PLT_TYPE;
    uObject.eObjType = aeType;
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
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, pFile);
        uint8_t ucLocationType = this->eLocType;
        if (this->eLocType) {
            if (ucLocationType == 1) {
                TESForm* pObj = TESForm::GetFormByNumericID(tempint);
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(pObj);
                if (pCell) {
                    SetLocCell(pCell);
                }
                else {
                    if (apOwner->GetFormEditorIDLength()) {
                        const char* pEditorID = apOwner->GetFormEditorID();
                        Warning(
                            "MASTERFILE: Unable to find Package Location Cell (%08X) on owner object \"%s\".",
                            tempint,
                            pEditorID
                        );
                    }
                    else {
                        Warning(
                            "MASTERFILE: Unable to find Package Location Cell (%08X) on owner object (%08X).",
                            tempint,
                            apOwner->iFormID
                        );
                    }
                    SetLocCell(nullptr);
                }
            }
            else if (ucLocationType == 4) {
                TESForm* pObj = TESForm::GetFormByNumericID(tempint);
                if (pObj) {
                    SetLocObject(pObj);
                }
                else {
                    if (apOwner->GetFormEditorIDLength()) {
                        const char* pEditorID = apOwner->GetFormEditorID();
                        Warning(
                            "MASTERFILE: Unable to find Package Location Object (%08X) on owner object \"%s\".",
                            tempint,
                            pEditorID
                        );
                    }
                    else {
                        Warning(
                            "MASTERFILE: Unable to find Package Location Object (%08X) on owner object (%08X).",
                            tempint,
                            apOwner->iFormID
                        );
                    }
                    SetLocObject(nullptr);
                }
            }
        }
        else {
            TESForm* pObj = TESForm::GetFormByNumericID(tempint);
            TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pObj);
            if (pRef) {
                SetLocReference(pRef);
            }
            else {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Package Location Reference (%08X) on owner object \"%s\".",
                        tempint,
                        pEditorID
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find Package Location Reference (%08X) on owner object (%08X).",
                        tempint,
                        apOwner->iFormID
                    );
                }
                SetLocReference(nullptr);
            }
        }
    }
}

bool PackageLocation::IsLocationGeneric() {
    return GetLocType() > PLT_EDITORLOC;
}

bool PackageLocation::IsRadiusNonZero() {
    return iRad != 0;
}

uint16_t PackageLocation::GetSaveSize() {
    int8_t cLocationType = static_cast<int8_t>(eLocType);
    if ((eLocType & 0x80) != 0) {
        return 5;
    }
    if (cLocationType <= PLT_OBJECT) {
        return 9;
    }

    if (cLocationType == 5) {
        return 9;
    }

    return 5;
}

void PackageLocation::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    // The saved form ID in uObject becomes the form.
    if (eLocType <= 4u) {
        uObject.pObject = uObject.iFormID ? TESForm::GetFormByNumericID(uObject.iFormID) : nullptr;
    }
}

void PackageLocation::InitLoadGame() {
    // The saved game left the object's form ID in the union; it is resolved to the form here.
    if (eLocType <= 4u) {
        uObject.pObject = TESForm::GetFormByNumericID(uObject.iFormID);
    }
}

void PackageLocation::Save(CHUNK_ID aeChunk) {
    PACK_LOC_FILE kPLFile;
    ourMemset(&kPLFile, 0, sizeof(kPLFile));
    kPLFile.eLocType = static_cast<int8_t>(GetLocType());
    if (GetLocType() == PLT_TYPE) {
        kPLFile.iFormID = static_cast<uint32_t>(GetLocObjectType());
    }
    // The pointer itself, not the 4-byte eObjType over its low half.
    else if (uObject.pRef) {
        kPLFile.iFormID = uObject.pRef->TESForm::GetFormID();
    }
    kPLFile.iRadius = GetRadius();
    if (TESForm::QEndianSwapOnSave()) {
        kPLFile.Endian();
    }
    TESForm::__AddChunkData(aeChunk, &kPLFile, 0xCu);
    if (TESForm::QEndianSwapOnSave()) {
        kPLFile.Endian();
    }
}

bool PackageLocation::Compare(PackageLocation* apCompare) {
    if (!apCompare) {
        return true;
    }
    int32_t iLocType = GetLocType();
    if (iLocType != apCompare->GetLocType()) {
        return true;
    }
    uint32_t uiRadius = GetRadius();
    if (uiRadius != apCompare->GetRadius()) {
        return true;
    }
    TESObjectREFR* pLocReference = GetLocReference();
    if (pLocReference != apCompare->GetLocReference()) {
        return true;
    }
    TESObjectCELL* pLocCell = GetLocCell();
    if (pLocCell != apCompare->GetLocCell()) {
        return true;
    }
    TESForm* pLocObject = GetLocObject();
    if (pLocObject != apCompare->GetLocObject()) {
        return true;
    }
    uint32_t uiObjectType = static_cast<uint32_t>(GetLocObjectType());
    return uiObjectType != static_cast<uint32_t>(apCompare->GetLocObjectType());
}

void PackageLocation::SaveGame() {
    pSaveLoadGameOLD->SaveGameDataOLD(&eLocType, sizeof(eLocType));
    pSaveLoadGameOLD->SaveGameDataOLD(&iRad, sizeof(iRad));
    int8_t cLocationType = this->eLocType;
    if ((this->eLocType & 128) == 0) {
        if (cLocationType <= 4) {
            int32_t iFormID = 0;
            if (uObject.pRef) {
                iFormID = uObject.pRef->iFormID;
            }
            pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&iFormID), 4u);
        }
        else if (cLocationType == 5) {
            pSaveLoadGameOLD->SaveGameDataOLD(&uObject, 4u);
        }
    }
}

void PackageLocation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
