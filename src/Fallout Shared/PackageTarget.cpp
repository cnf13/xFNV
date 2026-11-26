#include "Fallout Shared/PackageTarget.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

void PackageTarget::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&eTargType, sizeof(eTargType));
    apLoadGameBuffer->LoadData(&iValue, sizeof(iValue));
    apLoadGameBuffer->LoadData(&fAcquireRadius, sizeof(fAcquireRadius));
    if (eTargType <= 1u) {
        // The ID fills the whole pointer slot until the object is resolved.
        apLoadGameBuffer->LoadFormID(uObject.pObject);
    }
    else if (eTargType == 2) {
        apLoadGameBuffer->LoadData(&uObject, 4u);
    }
}

void PACK_TARG_FILE::Endian() {
    BSEndian(iFormID, false);
    BSEndian(iValue, false);
    BSEndian(fAcquireRadius, false);
}

PackageTarget::PackageTarget() {
    eTargType = static_cast<uint8_t>(PTT_TYPE);
    uObject.pRef = nullptr;
    iValue = 0;
    fAcquireRadius = 0.0f;
}

// The type through SetTargType, the value and the radius, then the file's form ID for reference, object and
// object-type targets (resolved later by InitItem).
PackageTarget::PackageTarget(const PACK_TARG_FILE* apPTFile) : iValue(apPTFile->iValue) {
    SetTargType(static_cast<PACK_TARG_TYPE>(apPTFile->eTargType));
    SetTargAcquireRadius(apPTFile->fAcquireRadius);
    if (eTargType <= static_cast<uint8_t>(PTT_TYPE)) {
        uObject.iFormID = apPTFile->iFormID;
    }
}

PACK_TARG_TYPE PackageTarget::GetTargType() const {
    // the stored type byte is zero-extended
    return static_cast<PACK_TARG_TYPE>(eTargType);
}

TESObjectREFR* PackageTarget::GetTargReference() const {
    if (GetTargType() != PTT_REFERENCE) {
        return nullptr;
    }
    return uObject.pRef;
}

TESForm* PackageTarget::GetTargObject() const {
    if (GetTargType() != PTT_OBJECT) {
        return nullptr;
    }
    return uObject.pObject;
}

PACKAGE_OBJECT_TYPE PackageTarget::GetTargObjectType() const {
    if (GetTargType() != PTT_TYPE) {
        return POTYPE_NONE;
    }
    return uObject.eObjType;
}

void PackageTarget::SetTargType(PACK_TARG_TYPE aeType) {
    // Compares the zero-extended type byte with the whole argument, then stores its low byte.
    if (static_cast<int32_t>(eTargType) != aeType) {
        eTargType = static_cast<uint8_t>(aeType);
        // object-based target types lose their object when the type changes
        switch (eTargType) {
            case PTT_REFERENCE:
            case PTT_OBJECT:
            case PTT_TYPE: uObject.pRef = nullptr; break;
            default: break;
        }
    }
}

void PackageTarget::SetTargReference(TESObjectREFR* apRef) {
    if (GetTargType() == PTT_REFERENCE) {
        uObject.pRef = apRef;
    }
}

void PackageTarget::SetTargObject(TESForm* apObj) {
    if (GetTargType() == PTT_OBJECT) {
        uObject.pObject = apObj;
    }
}

void PackageTarget::SetTargObjectType(PACKAGE_OBJECT_TYPE aeType) {
    if (GetTargType() == PTT_TYPE) {
        uObject.eObjType = aeType;
    }
}

void PackageTarget::SetTargAcquireRadius(float afAcquireRadius) {
    fAcquireRadius = afAcquireRadius;
}

void PackageTarget::Copy(PackageTarget* apCopy) {
    if (!apCopy) {
        return;
    }
    SetTargType(apCopy->GetTargType());
    SetTargReference(apCopy->GetTargReference());
    SetTargObject(apCopy->GetTargObject());
    SetTargObjectType(apCopy->GetTargObjectType());
    iValue = apCopy->iValue;
    SetTargAcquireRadius(apCopy->fAcquireRadius);
}

// The plugin file stores the target's form ID in uObject: it is made an index of the owner's file, then resolved
// to the reference (checked by DYNAMIC_CAST) or the object.
void PackageTarget::InitItem(TESForm* apOwner) {
    uint32_t uiTempInt = uObject.iFormID;
    TESFile* pFile = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(uiTempInt, pFile);
    if (eTargType) {
        if (eTargType == 1) {
            uObject.pObject = TESForm::GetFormByNumericID(uiTempInt);
            if (!uObject.pObject) {
                if (apOwner) {
                    if (apOwner->GetFormEditorIDLength()) {
                        const char* pEditorID = apOwner->GetFormEditorID();
                        Warning(
                            "MASTERFILE: Unable to find Package Target Object (%08X) on owner object \"%s\".",
                            uiTempInt,
                            pEditorID
                        );
                    }
                    else {
                        Warning(
                            "MASTERFILE: Unable to find Package Target Object (%08X) on owner object (%08X).",
                            uiTempInt,
                            apOwner->iFormID
                        );
                    }
                }
                else {
                    Warning("MASTERFILE: Unable to find Package Target Object (%08X) on unknown owner.", uiTempInt);
                }
            }
        }
    }
    else {
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        uObject.pRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (!uObject.pRef) {
            if (apOwner) {
                if (apOwner->GetFormEditorIDLength()) {
                    const char* pEditorID = apOwner->GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Package Target Reference (%08X) on owner object \"%s\".",
                        uiTempInt,
                        pEditorID
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find Package Target Reference (%08X) on owner object (%08X).",
                        uiTempInt,
                        apOwner->iFormID
                    );
                }
            }
            else {
                Warning("MASTERFILE: Unable to find Package Target Reference (%08X) on unknown owner.", uiTempInt);
            }
        }
    }
}

uint16_t PackageTarget::GetSaveSize() {
    if (static_cast<uint32_t>(eTargType) <= 1u) {
        return 13;
    }
    if (eTargType == 2) {
        return 13;
    }
    return 9;
}

// Reference and object targets: the saved form ID in uObject is resolved, a zero ID to null without a lookup.
void PackageTarget::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (eTargType <= 1u) {
        uObject.pObject = uObject.iFormID ? TESForm::GetFormByNumericID(uObject.iFormID) : nullptr;
    }
}

// Reference and object targets: the form ID an old-format save left in uObject is resolved.
void PackageTarget::InitLoadGame() {
    if (eTargType <= 1u) {
        uObject.pObject = TESForm::GetFormByNumericID(uObject.iFormID);
    }
}

void PackageTarget::LoadGame() {
    uint32_t uiFormID;
    pSaveLoadGameOLD->LoadGameDataOLD(&eTargType, sizeof(eTargType));
    pSaveLoadGameOLD->LoadGameDataOLD(&iValue, sizeof(iValue));
    pSaveLoadGameOLD->LoadGameDataOLD(&fAcquireRadius, sizeof(fAcquireRadius));
    if (static_cast<uint32_t>(eTargType) <= 1u) {
        pSaveLoadGameOLD->LoadNumericID(&uiFormID, 4u);
        uObject.eObjType = static_cast<PACKAGE_OBJECT_TYPE>(uiFormID);
    }
    else if (eTargType == 2) {
        pSaveLoadGameOLD->LoadGameDataOLD(&uObject, 4u);
    }
}

void PackageTarget::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&eTargType, sizeof(eTargType), 0);
    apSaveGameBuffer->SaveDataEndian(&iValue, sizeof(iValue), 0);
    apSaveGameBuffer->SaveDataEndian(&fAcquireRadius, sizeof(fAcquireRadius), 0);
    if (eTargType <= 1u) {
        apSaveGameBuffer->SaveFormID(uObject.pRef, 0);
    }
    else if (eTargType == 2) {
        apSaveGameBuffer->SaveDataEndian(&uObject, 4u, 0);
    }
}

bool PackageTarget::Compare(PackageTarget* apCompare) {
    if (!apCompare) {
        return true;
    }
    int32_t iTargType = GetTargType();
    if (iTargType != apCompare->GetTargType()) {
        return true;
    }
    TESObjectREFR* pTargReference = GetTargReference();
    if (pTargReference != apCompare->GetTargReference()) {
        return true;
    }
    TESForm* pTargObject = GetTargObject();
    if (pTargObject != apCompare->GetTargObject()) {
        return true;
    }
    uint32_t uiTargObjectType = static_cast<uint32_t>(GetTargObjectType());
    if (uiTargObjectType != static_cast<uint32_t>(apCompare->GetTargObjectType())) {
        return true;
    }
    uint32_t uiValue = iValue;
    if (uiValue != static_cast<uint32_t>(apCompare->iValue)) {
        return true;
    }
    double dAcquireRadius = GetTargAcquireRadius();
    return dAcquireRadius != apCompare->GetTargAcquireRadius();
}

// The saved target is the plugin file's form of it (PACK_TARG_FILE): the reference's or object's form ID, or the
// object type.
void PackageTarget::Save(CHUNK_ID aeChunk) {
    PACK_TARG_FILE kFile;
    ourMemset(&kFile, 0, sizeof(PACK_TARG_FILE));
    kFile.iValue = iValue;
    kFile.fAcquireRadius = fAcquireRadius;
    kFile.eTargType = eTargType;
    if (eTargType <= 1u) {
        // The pointer itself, not the 4-byte eObjType over its low half.
        if (uObject.pRef) {
            kFile.iFormID = uObject.pRef->iFormID;
        }
    }
    else if (eTargType == 2) {
        kFile.iFormID = static_cast<uint32_t>(GetTargObjectType());
    }
    if (TESForm::QEndianSwapOnSave()) {
        kFile.Endian();
    }
    TESForm::__AddChunkData(aeChunk, &kFile, sizeof(PACK_TARG_FILE));
    if (TESForm::QEndianSwapOnSave()) {
        kFile.Endian();
    }
}

void PackageTarget::SaveGame() {
    pSaveLoadGameOLD->SaveGameDataOLD(&eTargType, sizeof(eTargType));
    pSaveLoadGameOLD->SaveGameDataOLD(&iValue, sizeof(iValue));
    pSaveLoadGameOLD->SaveGameDataOLD(&fAcquireRadius, sizeof(fAcquireRadius));
    if (eTargType <= 1u) {
        int32_t iFormID = 0;
        if (uObject.pRef) {
            iFormID = uObject.pRef->iFormID;
        }
        pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&iFormID), 4u);
    }
    else if (eTargType == 2) {
        pSaveLoadGameOLD->SaveGameDataOLD(&uObject, 4u);
    }
}

void PackageTarget::operator delete(void* apMemory) {
    ourFree(apMemory);
}
