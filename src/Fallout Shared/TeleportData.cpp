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
	int32_t v4 = 0;
	if (!pLinkedDoor) {
		return reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(v4));
	}
	TESObjectCELL* parentCell = pLinkedDoor->TESObjectREFR::GetParentCell();
	if (parentCell) {
		return parentCell->GetWorldSpace();
	}
	parentCell = static_cast<TESChildCell*>(pLinkedDoor)->GetSaveParentCell();
	if (parentCell) {
		return parentCell->GetWorldSpace();
	}
	return reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(v4));
}

TESObjectCELL* DoorTeleportData::GetTeleportCell() {
	if (pLinkedDoor) {
		return pLinkedDoor->GetParentCell();
	}
	return nullptr;
}

void DoorTeleportData::SetConnectedCellsPublic(TESObjectREFR* apDoorRef, bool abIsPublic) {
	if (apDoorRef) {
		TESObjectCELL* TeleportCell = GetTeleportCell();
		if (TeleportCell && TeleportCell->GetOwner()) {
			TeleportCell->SetTempPublic(abIsPublic);
		}
		TESObjectCELL* ParentCell = apDoorRef->GetParentCell();
		if (ParentCell && ParentCell->GetOwner()) {
			ParentCell->SetTempPublic(abIsPublic);
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
	} else {
		return nullptr;
	}
}

// Reads an XTEL chunk (0x4C455458) into the teleport: the file's door form ID goes into pLinkedDoor (InitItem resolves
// it), then the position, the rotation and the flags; any other chunk leaves the teleport alone.
void DoorTeleportData::Load(TESFile* apFile) {
	if (apFile && apFile->GetTESChunk() == 0x4C455458) {
		FILE_TELEPORT_DATA kData;
		// The exe constructs the local with REF_DECAL_DATA's constructor (the same layout's empty NiPoint3
		// constructors).
		::new (&kData) REF_DECAL_DATA;
		ourMemset(&kData, 0, sizeof(FILE_TELEPORT_DATA));
		apFile->GetChunkData(&kData, sizeof(FILE_TELEPORT_DATA));
		if (apFile->QEndian()) {
			kData.Endian();
		}
		// The exe keeps the unresolved form ID in the pointer field until InitItem looks it up.
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
	} else {
		aOutMapName.Set("", 0);
	}
}

void DoorTeleportData::SetTeleportRotation(const NiPoint3& aTeleportRot) {
	rotation = aTeleportRot;
}

void DoorTeleportData::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&position), 0xC);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&rotation), 0xC);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cFlags), 1);
	apLoadGameBuffer->LoadFormID(pLinkedDoor);
}

// The door's form ID goes through a local and is stored as is in pLinkedDoor (the exe keeps the raw dword in the
// pointer slot); it is resolved later.
void DoorTeleportData::LoadGameTES() {
	pSaveLoadGameOLD->LoadGameDataOLD(&position, 0xC);
	pSaveLoadGameOLD->LoadGameDataOLD(&rotation, 0xC);
	uint32_t uiDoorID;
	pSaveLoadGameOLD->LoadNumericID(&uiDoorID, 4);
	pLinkedDoor = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiDoorID));
}

BGSWorldLocation* DoorTeleportData::GetTeleportWorldLocation(BGSWorldLocation* apResult) {
	TESForm* TeleportSpace = GetTeleportSpace();
	NiPoint3* ActorPackageThatIsRunning = &position;
	::new (apResult) BGSWorldLocation(*ActorPackageThatIsRunning, TeleportSpace);
	return apResult;
}

bool DoorTeleportData::Compare(DoorTeleportData* apCompare) {
	TESObjectREFR* LinkedDoor = pLinkedDoor;
	if (LinkedDoor != apCompare->pLinkedDoor) {
		return 1;
	}
	if ((&position)->operator!=(*&apCompare->position)) {
		return 1;
	}
	if ((&rotation)->operator!=(*&apCompare->rotation)) {
		return 1;
	}
	return cFlags != apCompare->cFlags;
}

// PDB DoorTeleportData::GetSaveSize (unsigned short): the position, the rotation and the door's form ID.
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

// Resolves the linked door's saved form ID, kept in pLinkedDoor, to its reference. No return value: eax is left
// holding whatever the last step put there, and its one caller (ExtraDataList::InitLoadGameTES 0x426020) does not
// read it.
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
		// The exe constructs the local with REF_DECAL_DATA's constructor (the same layout's empty NiPoint3
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
	int32_t a2;
	pSaveLoadGameOLD->SaveGameDataOLD(&position, 12u);
	pSaveLoadGameOLD->SaveGameDataOLD(&rotation, 12u);
	a2 = 0;
	if (pLinkedDoor) {
		a2 = pLinkedDoor->iFormID;
	}
	pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&a2), 4u);
}

void DoorTeleportData::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(&position, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&rotation, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&cFlags, 1u, 0);
	apSaveGameBuffer->SaveFormID(pLinkedDoor, 0);
}

void DoorTeleportData::operator delete(void* apMemory) {
	ourFree(apMemory);
}
