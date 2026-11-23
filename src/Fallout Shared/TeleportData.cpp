#include "Fallout Shared/TeleportData.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

// Swaps the door ID and the six position and rotation floats read from a big-endian file; the flags are left alone.
void FILE_TELEPORT_DATA::Endian() {
    ENDIAN(iLinkedDoorID, false);
    ENDIAN(position.x, false);
    ENDIAN(position.y, false);
    ENDIAN(position.z, false);
    ENDIAN(rotation.x, false);
    ENDIAN(rotation.y, false);
    ENDIAN(rotation.z, false);
}

DoorTeleportData::DoorTeleportData()
  : pLinkedDoor(nullptr), position(3.4028135e+38f, 3.4028135e+38f, 3.4028135e+38f),
    rotation(3.4028135e+38f, 3.4028135e+38f, 3.4028135e+38f), cFlags(0) {}

DoorTeleportData::~DoorTeleportData() {
    pLinkedDoor = nullptr;
}

TESWorldSpace* DoorTeleportData::GetTeleportWorldSpace() {
    if (!pLinkedDoor) {
        return nullptr;
    }
    TESObjectCELL* pCell = pLinkedDoor->TESObjectREFR::GetParentCell();
    if (pCell) {
        return pCell->GetWorldSpace();
    }
    pCell = static_cast<TESChildCell*>(pLinkedDoor)->GetSaveParentCell();
    if (pCell) {
        return pCell->GetWorldSpace();
    }
    return nullptr;
}

TESObjectCELL* DoorTeleportData::GetTeleportCell() {
    if (pLinkedDoor) {
        return pLinkedDoor->GetParentCell();
    }
    return nullptr;
}

void DoorTeleportData::SetConnectedCellsPublic(TESObjectREFR* apDoorRef, bool abIsPublic) {
    if (apDoorRef) {
        TESObjectCELL* pTeleportCell = GetTeleportCell();
        if (pTeleportCell && pTeleportCell->GetOwner()) {
            pTeleportCell->SetTempPublic(abIsPublic);
        }
        TESObjectCELL* pParentCell = apDoorRef->GetParentCell();
        if (pParentCell && pParentCell->GetOwner()) {
            pParentCell->SetTempPublic(abIsPublic);
        }
    }
}

TESObjectREFR* DoorTeleportData::GetLinkedDoor() {
    return pLinkedDoor;
}

void DoorTeleportData::SetLinkedDoor(TESObjectREFR* apDoorRef) {
    pLinkedDoor = apDoorRef;
}

const NiPoint3& DoorTeleportData::GetTeleportPosition() {
    return position;
}

void DoorTeleportData::SetTeleportPosition(const NiPoint3& aTeleportPos) {
    position = aTeleportPos;
}

TESForm* DoorTeleportData::GetTeleportSpace() {
    if (pLinkedDoor) {
        return pLinkedDoor->GetSpace();
    }

    return nullptr;
}

// Reads an XTEL chunk (0x4C455458) into the teleport: the file's door form ID goes into pLinkedDoor (InitItem resolves
// it), then the position, the rotation and the flags; any other chunk leaves the teleport alone.
void DoorTeleportData::Load(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == 0x4C455458) {
        FILE_TELEPORT_DATA kData;
        // The local is deliberately constructed with REF_DECAL_DATA's constructor (the same layout's empty NiPoint3
        // constructors).
        ::new (&kData) REF_DECAL_DATA;
        ourMemset(&kData, 0, sizeof(FILE_TELEPORT_DATA));
        apFile->GetChunkData(&kData, sizeof(FILE_TELEPORT_DATA));
        if (apFile->QEndian()) {
            kData.Endian();
        }
        // Keeps the unresolved form ID in the pointer field until InitItem looks it up.
        pLinkedDoor = reinterpret_cast<TESObjectREFR*>(static_cast<intptr_t>(kData.iLinkedDoorID));
        position = kData.position;
        rotation = kData.rotation;
        cFlags = kData.cFlags;
    }
}

void DoorTeleportData::Copy(DoorTeleportData* apCopy) {
    pLinkedDoor = apCopy->pLinkedDoor;
    SetTeleportPosition(apCopy->position);
    SetTeleportRotation(apCopy->rotation);
    cFlags = apCopy->cFlags;
}

void DoorTeleportData::GetTeleportCellMapName(BSStringT<char>& aOutMapName) {
    if (pLinkedDoor) {
        pLinkedDoor->GetMapNameForLocation(aOutMapName);
    }
    else {
        aOutMapName.Set("", 0);
    }
}

void DoorTeleportData::SetTeleportRotation(const NiPoint3& aTeleportRot) {
    rotation = aTeleportRot;
}

void DoorTeleportData::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&position), sizeof(position));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&rotation), sizeof(rotation));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cFlags), sizeof(cFlags));
    apLoadGameBuffer->LoadFormID(pLinkedDoor);
}

// The door's form ID goes through a local and is stored as is in pLinkedDoor; it is resolved later.
void DoorTeleportData::LoadGameTES() {
    pSaveLoadGameOLD->LoadGameDataOLD(&position, sizeof(position));
    pSaveLoadGameOLD->LoadGameDataOLD(&rotation, sizeof(rotation));
    uint32_t uiDoorID;
    pSaveLoadGameOLD->LoadNumericID(&uiDoorID, 4);
    pLinkedDoor = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiDoorID));
}

BGSWorldLocation* DoorTeleportData::GetTeleportWorldLocation(BGSWorldLocation* apResult) {
    TESForm* pSpace = GetTeleportSpace();
    ::new (apResult) BGSWorldLocation(position, pSpace);
    return apResult;
}

bool DoorTeleportData::Compare(DoorTeleportData* apCompare) {
    if (pLinkedDoor != apCompare->pLinkedDoor) {
        return true;
    }
    if (position != apCompare->position) {
        return true;
    }
    if (rotation != apCompare->rotation) {
        return true;
    }
    return cFlags != apCompare->cFlags;
}

// The save size covers the position, the rotation and the door's form ID.
uint16_t DoorTeleportData::GetSaveSize() {
    uint16_t usSize = 0;
    usSize += sizeof(NiPoint3);
    usSize += sizeof(NiPoint3);
    usSize += sizeof(uint32_t);
    return usSize;
}

void DoorTeleportData::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pDoor = nullptr;
    // The load left the door's form ID in the pointer field.
    if (pLinkedDoor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLinkedDoor)));
        pDoor = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pLinkedDoor = pDoor;
}

// Resolves the linked door's saved form ID, kept in pLinkedDoor, to its reference.
void DoorTeleportData::FinishInitLoadGame() {
    if (pLinkedDoor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLinkedDoor)));
        pLinkedDoor = dynamic_cast<TESObjectREFR*>(pForm);
    }
}

// Writes the teleport as an XTEL chunk: the linked door's form ID, the position, the rotation and the flags.
void DoorTeleportData::Save() {
    if (pLinkedDoor) {
        FILE_TELEPORT_DATA kData;
        // The local is deliberately constructed with REF_DECAL_DATA's constructor (the same layout's empty NiPoint3
        // constructors).
        ::new (&kData) REF_DECAL_DATA;
        ourMemset(&kData, 0, sizeof(FILE_TELEPORT_DATA));
        kData.iLinkedDoorID = static_cast<int32_t>(pLinkedDoor->iFormID);
        kData.position = position;
        kData.rotation = rotation;
        kData.cFlags = cFlags;
        if (TESForm::QEndianSwapOnSave()) {
            kData.Endian();
        }
        TESForm::__AddChunkData(XTEL_ID, &kData, sizeof(FILE_TELEPORT_DATA));
        if (TESForm::QEndianSwapOnSave()) {
            kData.Endian();
        }
    }
}

void DoorTeleportData::SaveGame() {
    pSaveLoadGameOLD->SaveGameDataOLD(&position, sizeof(position));
    pSaveLoadGameOLD->SaveGameDataOLD(&rotation, sizeof(rotation));
    int32_t iDoorID = 0;
    if (pLinkedDoor) {
        iDoorID = pLinkedDoor->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&iDoorID), 4u);
}

void DoorTeleportData::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&position, sizeof(position), 0);
    apSaveGameBuffer->SaveDataEndian(&rotation, sizeof(rotation), 0);
    apSaveGameBuffer->SaveDataEndian(&cFlags, sizeof(cFlags), 0);
    apSaveGameBuffer->SaveFormID(pLinkedDoor, 0);
}

void DoorTeleportData::operator delete(void* apMemory) {
    ourFree(apMemory);
}
