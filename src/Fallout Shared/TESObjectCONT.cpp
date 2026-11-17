#include "Fallout Shared/TESObjectCONT.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

// The container entry the last CNTO chunk added; the COED chunk that follows gives it its
// per-item extra data.
extern ContainerObject* TESObjectCONT_plastloaded;
ContainerObject* TESObjectCONT_plastloaded;

TESObjectCONT::TESObjectCONT() {
	SetFormType(27u);
	TESObjectCONT::InitializeData();
}

TESObjectCONT::~TESObjectCONT() {}

void TESObjectCONT::InitializeData() {
	Data.cContFlags = 0;
	pOpenSound = nullptr;
	pCloseSound = nullptr;
	pLoopingSound = nullptr;
}

// Reads the container's chunks. COED and CNTO share one handler: it reads the chunk code a second time and either
// gives the last entry its extra data (COED) or adds a new entry (CNTO). The editor ID buffer is emptied before
// the chunk is read: GetChunkData writes at most the chunk's bytes, and a zero-size chunk leaves the slot
// unwritten, so the exe would read its uninitialised stack tail as the editor ID.
bool TESObjectCONT::Load(TESFile* apFile) {
	if ((uint8_t)apFile->GetTESForm() != 27) {
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
			// the exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			// the empty string, for a chunk that writes nothing
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case MODL_ID:
		case MODD_ID:
		case MOSD_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile);
			break;
		case MODS_ID: {
			uint32_t uiSize = apFile->GetChunkSize();
			// the exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
			apFile->GetChunkData(pBuffer, uiSize);
			uint16_t usFormVersion = apFile->GetFormVersion();
			static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
			break;
		}
		case SCRI_ID: {
			uint32_t uiScriptID = 0;
			apFile->GetChunkData(uiScriptID);
			TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
			pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
			pScriptableForm->InitItem(this);
			break;
		}
		case COED_ID:
		case CNTO_ID: {
			uint32_t uiSubChunk = apFile->GetTESChunk();
			if (uiSubChunk == COED_ID) {
				if (TESObjectCONT_plastloaded) {
					ContainerItemExtra* pItemExtra = nullptr;
					void* pMemory = ourMalloc(sizeof(ContainerItemExtra));
					if (pMemory) {
						pItemExtra = ::new (pMemory) ContainerItemExtra;
					}
					TESObjectCONT_plastloaded->pItemExtra = pItemExtra;
					TESObjectCONT_plastloaded->pItemExtra->Load(apFile);
					TESObjectCONT_plastloaded = nullptr;
				}
			} else if (uiSubChunk == CNTO_ID) {
				CONTAINER_OBJECT_FILE fileEntry;
				ourMemset(&fileEntry, 0, sizeof(fileEntry));
				apFile->GetChunkData(&fileEntry, 8u);
				if (apFile->QEndian()) {
					fileEntry.Endian();
				}
				TESObjectCONT_plastloaded = static_cast<TESContainer*>(this)->AddObject(&fileEntry);
			}
			break;
		}
		case DATA_ID:
			LoadData(apFile, reinterpret_cast<char*>(&Data), 1u);
			break;
		case DSTD_ID:
		case DEST_ID:
			BGSDestructibleObjectForm::LoadChunk(this, apFile);
			break;
		case SNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData(uiSoundID);
			SetOpenSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case QNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData(uiSoundID);
			SetCloseSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case RNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData(uiSoundID);
			SetLoopingSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESObjectCONT::Save() {
	StartForm();
	TESFullName::Save();
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESScriptableForm::Save();
	TESContainer::Save();
	BGSDestructibleObjectForm::Save();
	if (TESForm::QEndianSwapOnSave()) {
	}
	__SaveData(&Data, 1u);
	if (TESForm::QEndianSwapOnSave()) {
	}
	if (this->GetOpenSound()) {
		TESForm::AddChunk(SNAM_ID, this->GetOpenSound()->iFormID);
	}
	if (this->GetCloseSound()) {
		TESForm::AddChunk(QNAM_ID, this->GetCloseSound()->iFormID);
	}
	if (pLoopingSound) {
		TESForm::AddChunk(RNAM_ID, pLoopingSound->iFormID);
	}
	CloseForm();
}

void TESObjectCONT::InitItem() {
	if (!GetInitialized()) {
		TESScriptableForm::InitItem(this);
		TESContainer::InitItem(this);
		BGSDestructibleObjectForm::InitItem(this);
		TESModelTextureSwap::InitItem(this);
		uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetOpenSound()));
		if (itempint) {
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempint, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
			pOpenSound = dynamic_cast<TESSound*>(FormByNumericID);
			if (!this->GetOpenSound()) {
				int32_t CastingType = iFormID;
				const char* v7 = GetFormEditorID();
				Warning("MASTERFILE: Could not find open sound (%08X) for container '%s' (%08X).", itempint, v7,
					CastingType);
			}
		}
		itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this->GetCloseSound()));
		if (itempint) {
			TESFile* v3 = GetFile(-1);
			TESForm::AddCompileIndex(itempint, v3);
			TESForm* v4 = TESForm::GetFormByNumericID(itempint);
			pCloseSound = dynamic_cast<TESSound*>(v4);
			if (!this->GetCloseSound()) {
				int32_t v11 = iFormID;
				const char* v8 = GetFormEditorID();
				Warning("MASTERFILE: Could not find open sound (%08X) for container '%s' (%08X).", itempint, v8, v11);
			}
		}
		itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLoopingSound));
		if (itempint) {
			TESFile* v5 = GetFile(-1);
			TESForm::AddCompileIndex(itempint, v5);
			TESForm* v6 = TESForm::GetFormByNumericID(itempint);
			pLoopingSound = dynamic_cast<TESSound*>(v6);
			if (!pLoopingSound) {
				int32_t v12 = iFormID;
				const char* v9 = GetFormEditorID();
				Warning("MASTERFILE: Could not find pLoopingSound (%08X) for container '%s' (%08X).", itempint, v9,
					v12);
			}
		}
		SetInitialized(1);
	}
}

void TESObjectCONT::Copy(TESForm* apCopy) {
	TESObjectCONT* pCopy = dynamic_cast<TESObjectCONT*>(apCopy);
	if (pCopy) {
		CopyAllComponents(apCopy);
		Data.cContFlags = pCopy->Data.cContFlags;
		SetOpenSound(pCopy->GetOpenSound());
		SetCloseSound(pCopy->GetCloseSound());
		SetLoopingSound(pCopy->GetLoopingSound());
	}
}

bool TESObjectCONT::Compare(TESForm* apCompare) {
	TESObjectCONT* pComp = dynamic_cast<TESObjectCONT*>(apCompare);
	if (!pComp) {
		return 1;
	}
	if (CompareAllComponents(pComp)) {
		return 1;
	}
	int32_t cContFlags = Data.cContFlags;
	if (cContFlags != (&pComp->Data)->cContFlags) {
		return 1;
	}
	TESSound* OpenSound = GetOpenSound();
	if (OpenSound != pComp->GetOpenSound()) {
		return 1;
	}
	TESSound* CloseSound = GetCloseSound();
	if (CloseSound != pComp->GetCloseSound()) {
		return 1;
	}
	TESSound* LoopingSound = GetLoopingSound();
	return LoopingSound != pComp->GetLoopingSound();
}

// The exe's cleanup-only C++ EH frame is left out.
bool TESObjectCONT::Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
	TESBoundObject* apObjecttoGet, int32_t aicount) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	int32_t fdoPunishment;
	Actor* pActionActor = dynamic_cast<Actor*>(apActionRef);
	TESForm* pownership = 0;
	PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
	if (!pActionActor) {
		return 0;
	}
	bool bignorelock = 0;
	if (apActionRef != pPlayer) {
		bignorelock = apItemActivated->IsAnOwner(pActionActor, 1);
	}
	LockData* pLock = apItemActivated->GetLock();
	if (pLock && (pLock->IsLocked() && !bignorelock)) {
		bool blockpick = pActionActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eUseItem);
		if (pLock->pKey && pActionActor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem)) {
			if (!pstaticcontainer && pActionActor == pPlayer) {
				char msg[256];
				pstaticcontainer = static_cast<MobileObject*>(apItemActivated);
				ExtraLock* pextralock = apItemActivated->GetExtraLock();
				if (pextralock) {
					pextralock->UnLockDoor();
					apItemActivated->AddLockChange();
				}
				BSAudio* v6 = &BSAudio::QInstance();
				BSSoundHandle* SoundHandleByName = v6->GetSoundHandleByName(&a1, "UILockpickingUnlock", 289u);
				SoundHandleByName->Play(0);
				BSsprintf(msg, 255u, sOpenWithKey.String(), pLock->pKey->GetFullName());
				Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_key.dds", 0, 2.0f, 0);
				pstaticcontainer->Activate(pPlayer, 0, 0, 1);
				pstaticcontainer = 0;
				return 1;
			}
		} else {
			if (!pstaticcontainer) {
				if (pActionActor == pPlayer) {
					ExtraLock* pextralock_2 = apItemActivated->GetExtraLock();
					*reinterpret_cast<float*>(&fdoPunishment) = 0.0f;
					BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment);
					if (pextralock_2 &&
						(pextralock_2->pLock->GetLockLevel(apItemActivated) == 5 || pextralock_2->pLock->IsBroken())) {
						const char* v8 = sImpossibleLock.String();
						Interface::ShowMessage(v8, 0, "Interface\\Icons\\Message Icons\\glow_message_padlock.dds", 0,
							2.0f, 0);
					} else {
						_ShowLockPickMenu(apItemActivated, 0, 0);
					}
				} else {
					Animation* panim = pActionActor->GetAnimation();
					if (panim) {
						panim->SpecialIdleFree(1, 0);
					}
					TESIdleManager::SetUsedItem(pLockpick);
					TESIdleManager::SetUsedItemActivate(1);
					TESIdleManager::SetUsedItemLevel(1);
					BaseProcess* CurrentAIProcess = pActionActor->pCurrentProcess;
					CurrentAIProcess->SetupSpecialIdle(pActionActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
					TESIdleManager::SetUsedItem(0);
					TESIdleManager::SetUsedItemActivate(0);
					TESIdleManager::SetUsedItemLevel(-1);
					ExtraLock* pextralock_3 = apItemActivated->GetExtraLock();
					if (pextralock_3) {
						pextralock_3->UnLockDoor();
						apItemActivated->AddLockChange();
					}
				}
			}
		}
		return 0;
	}
	if (pActionActor != pPlayer) {
		if (GetOpenSound()) {
			if (apItemActivated) {
				if (apItemActivated->Get3D()) {
					TESSound* OpenSound = GetOpenSound();
					uint32_t time = OpenSound->iFormID;
					BSAudio* v10 = &BSAudio::QInstance();
					v10->GetSoundHandleByNumericID(&soundHand, time, 258u);
					NiPoint3 v17 = apItemActivated->GetLocationOnReference();
					soundHand.SetPosition(v17.x, v17.y, v17.z);
					soundHand.Play(0);
				}
			}
		}
	}
	if (pActionActor == pPlayer) {
		if (BGSOpenCloseForm::GetOpenState(apItemActivated) == BGSOpenCloseForm::OBJECT_OPEN) {
			HandleOpenFinish(apItemActivated, pActionActor);
		} else {
			BGSOpenCloseForm::HandleActivate(apItemActivated, pActionActor, 0);
		}
		return 1;
	} else {
		if (apObjecttoGet) {
			apItemActivated->RemoveItem(apObjecttoGet, 0, aicount, 0, 0, pActionActor, 0, 0, 1, 0);
		}
		if (pActionActor->IsNpc() && !apItemActivated->IsAnOwner(pActionActor, 1)) {
			if (apItemActivated->GetOwner()) {
				pActionActor->StealAlarm(apItemActivated, apObjecttoGet, aicount, 1, pownership);
			}
		}
		return 1;
	}
}

// Once the player's open animation finishes, opens the container menu and runs the activation.
void TESObjectCONT::HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) {
	if (apItemActivated && apActionRef == pPlayer) {
		Interface::QueueMenuCreate(Interface::OPEN_CONTAINER, apItemActivated, nullptr, nullptr,
			Interface::CMM_STANDARD, false);
		BGSOpenCloseForm::HandleActivate(apItemActivated, apActionRef, nullptr);
	}
}

TESSound* TESObjectCONT::GetLoopingSound() {
	return pLoopingSound;
}

void TESObjectCONT::SetOpenSound(TESSound* apSound) {
	pOpenSound = apSound;
}

void TESObjectCONT::SetCloseSound(TESSound* apSound) {
	pCloseSound = apSound;
}

void TESObjectCONT::SetLoopingSound(TESSound* apSound) {
	pLoopingSound = apSound;
}

bool TESObjectCONT::CanActivate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abAllowLockpicking) {
	if (!apActionRef || !apActionRef->IsActor()) {
		return 0;
	}
	LockData* Lock = apItemActivated->GetLock();
	if (!Lock || !Lock->IsLocked()) {
		return 1;
	}
	if (apActionRef != pPlayer && apItemActivated->IsAnOwner(static_cast<Actor*>(apActionRef), 1)) {
		return 1;
	}
	PACKAGE_OBJECT_TYPE aeMatchId = POTYPE_NONE;
	if (Lock->pKey && static_cast<Actor*>(apActionRef)->Actor::HasObjects(Lock->pKey, POTYPE_NONE, 1, 0, aeMatchId)) {
		return 1;
	}
	return abAllowLockpicking &&
		static_cast<Actor*>(apActionRef)->Actor::HasObjects(pLockpick, POTYPE_NONE, 1, 0, aeMatchId);
}
