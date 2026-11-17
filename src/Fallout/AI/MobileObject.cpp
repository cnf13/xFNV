#include "Fallout/AI/MobileObject.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterState.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkOrientHingedBodyAction.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkOrientHingedBodyAction.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/ResponseListWrapper.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESMagicCasterForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LipSyncBackgroundManager.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"

#include <stdint.h>
#include <xmmintrin.h>

static NiPoint3 MobileObject_boundMin;
static NiPoint3 MobileObject_boundMax;

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> fDBVoiceAttenuationIn2D("fDBVoiceAttenuationIn2D:Audio", 0.0f);
SettingT<INISettingCollection> bBackgroundLoadLipFiles("bBackgroundLoadLipFiles:BackgroundLoad", false);
SettingT<INISettingCollection> fQuadrupedPitchMult("fQuadrupedPitchMult:HAVOK", 1.0f);
SettingT<INISettingCollection> gs_iHavokSkipFrameCountTEST_Havok("iHavokSkipFrameCountTEST:Havok", 0);

int32_t get_iFrameCounter() {
	return Main::iFrameCounter;
}

// iMoveDirection stays unset, as in the exe's constructor.
MoveData::MoveData()
	: fTime(0.0f), Angle(NiPoint3_ZERO), Displacement(NiPoint3_ZERO), fCurrMoveSpeed(0.0f), bVisible(true) {}

// fUpdateTargetTimer stays unset, as in the exe's constructor.
MobileObject::MobileObject()
	: TESObjectREFR(), pCurrentProcess(nullptr), pTalkingActivator(nullptr), pDialogueItemTarget(nullptr),
	  fSayToFailVoiceTimer(0.0f), bSpeakingDone(true), bTalkingToPlayer(false), bIgnoreChangeAnimationCall(false),
	  bSoundFileDone(true), bVoiceFileDone(true), bTalkingActivator(false), bInTempChangeList(false),
	  bDoNotRunSayToCallBack(false), cSoundCallBackSet(-1), bSoundCallBackSuccess(false), bUseEmotion(true),
	  bRunsInLow(false) {}

// The exe's cleanup-only C++ EH frame is left out.
MobileObject::~MobileObject() {
	ReferenceProcessLists.kUpdateLock.Lock(0);
	if (!GetTemporary()) {
		ReferenceProcessLists.RemoveActorFromTempChangeList(this);
		if (!pDataHandler->GetClearingData()) {
			CleanUpTraps();
		}
		if (pCurrentProcess) {
			PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
			ReferenceProcessLists.RemoveReference(this, ProcessLevel);
		}
	}
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	ReferenceProcessLists.kUpdateLock.Unlock();
	if (bTalkingActivator) {
		pParentCell = 0;
	}
}

// One byte of its own, the base reference's size, then the process's (the exe adds the two 16-bit values with
// truncating word stores).
uint16_t MobileObject::GetSaveSize(uint32_t aiFlags) {
	uint16_t usSize = 1;
	usSize = usSize + TESObjectREFR::GetSaveSize(aiFlags);
	if (pCurrentProcess) {
		usSize = usSize + pCurrentProcess->GetSaveSize(aiFlags, this);
	}
	return usSize;
}

// The process level byte (-1 when the object has no process), then the base reference's data, then the process's.
void MobileObject::SaveGame(uint32_t aiFlags) {
	uint8_t ucLevel = 0xFF;
	if (pCurrentProcess) {
		ucLevel = static_cast<uint8_t>(pCurrentProcess->GetProcessLevel());
	}
	SaveGameDataOLD(&ucLevel, 1);
	TESObjectREFR::SaveGame(aiFlags);
	if (pCurrentProcess) {
		pCurrentProcess->SaveGame(aiFlags, this);
	}
}

// The process level is saved as one byte, 0xFF without a process.
// SaveDataEndian takes the fields as raw bytes (char*), as the exe passes their addresses.
void MobileObject::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	int8_t cProcessLevel = -1;
	if (pCurrentProcess) {
		cProcessLevel = static_cast<int8_t>(pCurrentProcess->GetProcessLevel());
	}
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cProcessLevel), 1, 0);
	TESObjectREFR::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cSoundCallBackSet), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSoundCallBackSuccess), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSpeakingDone), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSoundFileDone), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bVoiceFileDone), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTalkingToPlayer), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bIgnoreChangeAnimationCall), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bUseEmotion), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fUpdateTargetTimer), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSayToFailVoiceTimer), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTalkingActivator), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoNotRunSayToCallBack), 1, 0);
	apSaveGameBuffer->SaveFormID(pTalkingActivator, 0);
	apSaveGameBuffer->SaveFormID(pDialogueItemTarget, 0);
	if (pCurrentProcess) {
		pCurrentProcess->SaveGame(apSaveGameBuffer);
	}
}

// Rebuilds the process from the saved level byte, drops the 3D of a cell that is not loaded, then loads the base
// reference's data and the process's. The exe's C++ EH (unwind) frame is left out: it only matters when a call in
// here throws.
void MobileObject::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	int8_t cLevel = 0;
	TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cLevel), 1);
	switch (cLevel) {
	case -1:
		if (pCurrentProcess) {
			ReferenceProcessLists.kUpdateLock.Lock(nullptr);
			ReferenceProcessLists.RemoveReference(this, pCurrentProcess->GetProcessLevel());
			delete pCurrentProcess;
			ReferenceProcessLists.kUpdateLock.Unlock();
			pCurrentProcess = nullptr;
		}
		break;
	case 0:
		if (pCurrentProcess) {
			if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_HIGH) {
				MovetoHigh();
			}
		} else {
			void* pMemory = ourMalloc(sizeof(HighProcess));
			pCurrentProcess = pMemory ? ::new (pMemory) HighProcess : nullptr;
			ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, 0, false, nullptr);
		}
		break;
	case 1:
		if (pCurrentProcess) {
			if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_MIDDLE_HIGH) {
				MovetoMiddleHigh();
			}
		} else {
			void* pMemory = ourMalloc(sizeof(MiddleHighProcess));
			pCurrentProcess = pMemory ? ::new (pMemory) MiddleHighProcess : nullptr;
			ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, 0, false, nullptr);
		}
		break;
	case 2:
		if (pCurrentProcess) {
			if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_MIDDLE_LOW) {
				MovetoMiddleLow();
			}
		} else {
			void* pMemory = ourMalloc(sizeof(MiddleLowProcess));
			pCurrentProcess = pMemory ? ::new (pMemory) MiddleLowProcess : nullptr;
			ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, 0, false, nullptr);
		}
		break;
	case 3:
		if (pCurrentProcess) {
			if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_LOW) {
				MovetoLow();
			}
		} else {
			void* pMemory = ourMalloc(sizeof(LowProcess));
			pCurrentProcess = pMemory ? ::new (pMemory) LowProcess : nullptr;
			ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, 0, false, nullptr);
		}
		break;
	default:
		break;
	}
	if (Get3D()) {
		TESObjectCELL* pCell = GetParentCell();
		if (!TES::pInstance->IsCellLoaded(pCell, false) || cLevel != 0) {
			Set3D(nullptr, true);
		}
	}
	TESObjectREFR::LoadGame(aiFlags, aiCurrentFlags);
	if (pCurrentProcess) {
		pCurrentProcess->LoadGame(aiFlags, aiCurrentFlags, this);
	}
}

// The process level byte, the base reference's data, the dialogue and sound state, then the process's own data.
void MobileObject::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint8_t ucLevel = 0xFF;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucLevel), 1);
	SetProcessLevel(static_cast<PROCESS_TYPE>(static_cast<int8_t>(ucLevel)));
	TESObjectREFR::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cSoundCallBackSet), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSoundCallBackSuccess), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeakingDone), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSoundFileDone), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bVoiceFileDone), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTalkingToPlayer), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bIgnoreChangeAnimationCall), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bUseEmotion), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fUpdateTargetTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSayToFailVoiceTimer), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTalkingActivator), 1);
	if (apLoadGameBuffer->GetVersion() >= 8) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNotRunSayToCallBack), 1);
	}
	apLoadGameBuffer->LoadFormID(pTalkingActivator);
	apLoadGameBuffer->LoadFormID(pDialogueItemTarget);
	if (pCurrentProcess) {
		pCurrentProcess->LoadGame(apLoadGameBuffer);
	}
}

// The exe's guard masks both flags with 0 (and ecx, 0), so the Set3D branch never runs; it is kept as written.
void MobileObject::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESObjectREFR::InitLoadGame(aiFlags, aiCurrentFlags);
	if (pCurrentProcess) {
		pCurrentProcess->InitLoadGame(aiFlags, aiCurrentFlags, this);
	}
	if ((aiFlags & 0) || (aiCurrentFlags & 0)) {
		if (GetDisabled() || GetDelete()) {
			Set3D(nullptr, true);
		}
	}
}

// The loaded pointers hold form IDs until here (the exe stores the ID in the pointer field). The exe's EH frame
// around the LowProcess construction (it frees the memory if the constructor throws) is left out. Get3D is called
// through the vtable first, then directly.
void MobileObject::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR::InitLoadGame(apLoadGameBuffer);
	TESObjectREFR* pActivator = nullptr;
	if (pTalkingActivator) {
		uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActivator));
		TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
		pActivator = dynamic_cast<TESObjectREFR*>(pForm);
	}
	pTalkingActivator = pActivator;
	MobileObject* pTarget = nullptr;
	if (pDialogueItemTarget) {
		uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDialogueItemTarget));
		TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
		pTarget = dynamic_cast<MobileObject*>(pForm);
	}
	pDialogueItemTarget = pTarget;
	if (pCurrentProcess) {
		pCurrentProcess->InitLoadGame(apLoadGameBuffer);
	}
	if (Get3D() && !IsProjectile() && !GetCharController()) {
		InitHavok();
	}
	if (!pCurrentProcess && !GetDisabledOrDeleted()) {
		void* pMemory = ourMalloc(sizeof(LowProcess));
		LowProcess* pProcess = pMemory ? ::new (pMemory) LowProcess : nullptr;
		pCurrentProcess = pProcess;
		SetProcessLevel(GetDesiredProcessLevel());
	}
	if (TESObjectREFR::Get3D()) {
		if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_LOW ||
			pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW) {
			Set3D(nullptr, true);
		}
	}
}

// The exe calls TESObjectREFR::Get3D directly; the hour is rounded to float before SetLastTimeProcessed.
void MobileObject::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	TESObjectREFR::FinishInitLoadGame(aiFlags, aiOldFlags);
	Actor* pActor = dynamic_cast<Actor*>(this);
	if (pActor &&
		(pActor->GetLifeState() == ACTOR_LIFE_STATE_DEAD || pActor->GetLifeState() == ACTOR_LIFE_STATE_DYING)) {
		DetachCharController();
	} else if (aiFlags & 6) {
		auto* pController = static_cast<bhkCharacterController*>(GetCharController());
		if (pController && GetParentCell()) {
			pController->SetWorld(GetParentCell()->GetbhkWorld());
		}
		if (pController && !pController->QIsFlying()) {
			NiPoint3 kPosition = *QLocationOnReference();
			pController->SetPosition(kPosition);
		}
	}
	if (pCurrentProcess) {
		ReferenceProcessLists.kUpdateLock.Lock(nullptr);
		if (GetDelete() || GetDisabled()) {
			ReferenceProcessLists.RemoveReference(this, pCurrentProcess->GetProcessLevel());
			BaseProcess* pProcess = pCurrentProcess;
			if (pProcess) {
				delete pProcess;
			}
			pCurrentProcess = nullptr;
		} else if (!TESObjectREFR::Get3D() && this != pPlayer &&
			(pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH ||
				pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH)) {
			MovetoMiddleLow();
			float fTime = appCalendar.GetHour() - 0.5;
			pCurrentProcess->SetLastTimeProcessed(fTime);
			ReferenceProcessLists.RemoveActorFromTempChangeList(static_cast<Actor*>(this));
		}
		ReferenceProcessLists.kUpdateLock.Unlock();
		if (pCurrentProcess) {
			pCurrentProcess->FinishInitLoadGame(aiFlags, aiOldFlags, this);
		}
	}
}

// The equipped weapon and ammo go when flag 0x20 is set, then the base reference's and the process's revert; a
// disabled or deleted reference that is no longer being loaded then loses its process.
void MobileObject::Revert(uint32_t aiFlags) {
	if (aiFlags & 0x20) {
		RemoveWeapon();
		RemoveAmmo();
		AttachAmmo(nullptr);
		RemoveAmmo();
	}
	TESObjectREFR::Revert(aiFlags);
	if (pCurrentProcess) {
		pCurrentProcess->Revert(aiFlags, this);
	}
	if (pSaveLoadGameOLD->IsUnk048Zero() && pSaveLoadGameOLD->GetLoading() && pCurrentProcess &&
		(GetDisabled() || GetDelete())) {
		ReferenceProcessLists.kUpdateLock.Lock(nullptr);
		ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
		delete pCurrentProcess;
		pCurrentProcess = nullptr;
		ReferenceProcessLists.kUpdateLock.Unlock();
	}
}

// The base reference's revert and the process's, then the dialogue and sound state is reset.
void MobileObject::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR::Revert(apLoadGameBuffer);
	if (pCurrentProcess) {
		pCurrentProcess->Revert(apLoadGameBuffer);
	}
	SetChaseBip(false);
	cSoundCallBackSet = -1;
	bSoundCallBackSuccess = false;
	bSpeakingDone = true;
	bSoundFileDone = true;
	bVoiceFileDone = true;
	bTalkingToPlayer = false;
	bIgnoreChangeAnimationCall = false;
	bUseEmotion = true;
	fUpdateTargetTimer = 0.0f;
	fSayToFailVoiceTimer = 0.0f;
	bDoNotRunSayToCallBack = false;
}

void MobileObject::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR::FinishLoadGame(apLoadGameBuffer);
	if (this->pCurrentProcess) {
		this->pCurrentProcess->FinishLoadGame(apLoadGameBuffer);
		if (GetDisabledOrDeleted()) {
			ReferenceProcessLists.kUpdateLock.Lock(0);
			PROCESS_TYPE ProcessLevel = this->pCurrentProcess->eLevel;
			ReferenceProcessLists.RemoveReference(this, ProcessLevel);
			BaseProcess* pCurrentProcess = this->pCurrentProcess;
			if (pCurrentProcess) {
				delete pCurrentProcess;
			}
			ReferenceProcessLists.kUpdateLock.Unlock();
			this->pCurrentProcess = 0;
		} else {
			if (!TESObjectREFR::Get3D() && this != pPlayer &&
				(!this->pCurrentProcess->eLevel || this->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH)) {
				PROCESS_TYPE edesiredProcessLevel = GetDesiredProcessLevel();
				if (edesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW || edesiredProcessLevel == PROCESS_TYPE_LOW) {
					if (pSaveLoadGame->GetSaveGameLoading()) {
						if (edesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW) {
							MovetoMiddleLow();
						} else {
							MovetoLow();
						}
						float v5 = appCalendar.GetHour() - 0.5;
						this->pCurrentProcess->SetLastTimeProcessed(v5);
						ReferenceProcessLists.RemoveActorFromTempChangeList(this);
					} else {
						ReferenceProcessLists.AddActorToTempChangeList(this);
					}
				}
			}
		}
	}
	if (!bVoiceFileDone && !(&m_Extra)->GetSayToExtra()) {
		// The callback's context is the form ID.
		MobileObject::DialogueConversationCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), 1);
	}
	if (pSaveLoadGame->GetSaveGameLoading() && !GetDisabledOrDeleted()) {
		PROCESS_TYPE edesiredProcessLevel_2 = GetDesiredProcessLevel();
		SetProcessLevel(edesiredProcessLevel_2);
	}
}

void MobileObject::SetDelete(bool abDeleted) {
	TESObjectREFR::SetDelete(abDeleted);
	// The exe passes every mobile object, projectiles included, as the actor.
	ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(this));
	if (abDeleted) {
		const bool bMagicCaster = FNVD_DYNAMIC_CAST_VOID(MagicCaster)(this, RTTI_MobileObject, RTTI_MagicCaster) ||
			TESMagicCasterForm::CanBeMagicCaster(this);
		CleanUpOtherMobileObjects(bMagicCaster);
		CleanUpPointersOnDisable();
	}
}

// The sound callback of a say-to or of a dialogue line: finishes the say-to (checks the response ID, runs the topic
// info and clears the extra) or ends the dialogue package's listen timer, then shows the line's subtitle and starts
// its face animation. The exe emits an empty copy-constructor call (0x6815C0) before each response-text read; src
// reads the text through BSStringT::String.
void MobileObject::UpdateSoundCallBack() {
	if (cSoundCallBackSet || !GetSayToTopicInfo()) {
		if (cSoundCallBackSet == 1) {
			TESPackage* pPackage = GetCurrentPackage();
			DialoguePackage* pDialoguePackage = nullptr;
			if (pPackage) {
				if (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
					pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
				}
			}
			if (pDialoguePackage) {
				if (pDialoguePackage->pTalkingActor) {
					pDialoguePackage->pTalkingActor->bVoiceFileDone = 1;
				}
				if (!bSoundCallBackSuccess) {
					if (bSoundFileDone) {
						pDialoguePackage->flistentimer = 3.0f;
					} else {
						pDialoguePackage->flistentimer = 2.0f;
					}
				} else if (bSoundFileDone) {
					pDialoguePackage->flistentimer = 0.3f;
				} else {
					pDialoguePackage->flistentimer = 20.0f;
				}
				if (pDialoguePackage->GetCurrentDialogItem()) {
					pDialoguePackage->GetCurrentDialogItem()->RunResult(TIRS_END);
				}
			}
			bVoiceFileDone = 1;
		}
	} else {
		TESTopicInfo* pTopicInfo = GetSayToTopicInfo();
		TESTopic* pTopic = GetSayToTopic();
		uint32_t uiResponseID = GetSayToResponseID();
		bool bSayTo = true;
		if (uiResponseID) {
			if (pTopicInfo) {
				FNVD_CONSTRUCTED_LOCAL(ResponseListWrapper, aResponseList, nullptr);
				pTopicInfo->CloneResponseList(&aResponseList);
				if (uiResponseID != static_cast<uint32_t>(aResponseList.GetResponseID())) {
					bSayTo = false;
				}
			}
		}
		if (bSayTo) {
			SetSayToTopic(nullptr);
			ExtraDataList* pExtra = GetExtra();
			pExtra->RemoveSayToInfoExtra();
		}
		if (pTopicInfo) {
			if (!pTopicInfo->IsRunImmediately()) {
				pTopicInfo->RunResult(TIRS_END, this);
			}
		}
		if (bSayTo) {
			ExtraDataList* pExtra = GetExtra();
			// kEvent_SayToDone (IDA's name; src has no enum for it).
			Script::SetActionFlag(pTopic, pExtra, 0x40000);
		}
		if (!bSoundCallBackSuccess) {
			fSayToFailVoiceTimer = 1.0f;
		}
		if (bSayTo) {
			GetCurrentAIProcess()->SetDoingSayTo(false);
		}
		bVoiceFileDone = 1;
		bSoundFileDone = 1;
		if (bSayTo) {
			pPlayer->ResetPlayerGreetFlag();
		}
	}
	if (cSoundCallBackSet == 3) {
		BSFaceGenAnimationData* pFaceData = nullptr;
		uint32_t uiEmotion = GetSpeakingEmotion();
		int32_t iEmotionValue = GetSpeakingEmotionValue();
		bool bPlayer = (this == pPlayer);
		double fDistance = GetDistanceFromReference(pPlayer, false, false);
		if (static_cast<double>(iSpeakSoundLipDistance.UInt()) > fDistance && (!bPlayer || pPlayer->Is3rdPerson())) {
			pFaceData = GetFaceAnimationData(nullptr);
		}
		BaseProcess* pProcess = GetCurrentAIProcess();
		if (pProcess->GetGreetingFlag()) {
			TESTopic* pSayToTopic = GetSayToTopic();
			BaseProcess* pSubtitleProcess = GetCurrentAIProcess();
			DialogueItem* pSubtitleItem = pSubtitleProcess->GetSubtitleItem();
			if (pFaceData) {
				if (IsActor()) {
					MobileObject* pMobile = this;
					if (BSFaceGenManager::GetUseEmotions()) {
						if (uiEmotion) {
							int32_t iExpression =
								BSFaceGenManager::GetExpressionFromEmotion(static_cast<DIALOGUE_EMOTION>(uiEmotion));
							float fAmount = static_cast<float>(iEmotionValue / 100.0);
							pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
						} else {
							int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(pMobile))
												->GetActorValue(ActorValue::MOOD);
							pFaceData->SetExpressionTargetFromMood(iMood, true);
						}
					} else {
						pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
					}
				}
			}
			if (pSubtitleItem) {
				const char* pText = pSubtitleItem->GetCurrentResponse()->cText.String();
				if ((gs_bGeneralSubtitles_Gameplay.Bool() && pText) ||
					(IsActor() && static_cast<Actor*>(this)->GetShowSubtitles())) {
					if (GetIsFullHelp()) {
						char acSubtitle[0x100];
						std::sprintf(acSubtitle, "'%s' :%s", GetFullName(),
							pSubtitleItem->GetCurrentResponse()->cText.String());
						BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
						Interface::ShowSubtitle(acSubtitle, kHandle, GetLocationOnReference(), pPlayer, false);
					} else {
						BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
						Interface::ShowSubtitle(const_cast<char*>(pSubtitleItem->GetCurrentResponse()->cText.String()),
							kHandle, GetLocationOnReference(), pPlayer, false);
					}
				}
				GetCurrentAIProcess()->SetSubtitleItem(nullptr);
			}
		} else {
			TESPackage* pPackage = GetCurrentPackage();
			BaseProcess* pPackageProcess = GetCurrentAIProcess();
			TESPackage* pProcessPackage = pPackageProcess->GetCurrentPackage();
			DialoguePackage* pDialoguePackage = nullptr;
			if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
				pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
			} else if (pProcessPackage) {
				if (pProcessPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
					pDialoguePackage = static_cast<DialoguePackage*>(pProcessPackage);
				}
			}
			if (pFaceData) {
				if (IsActor()) {
					MobileObject* pMobile = this;
					if (BSFaceGenManager::GetUseEmotions()) {
						if (uiEmotion) {
							int32_t iExpression =
								BSFaceGenManager::GetExpressionFromEmotion(static_cast<DIALOGUE_EMOTION>(uiEmotion));
							float fAmount = static_cast<float>(iEmotionValue / 100.0);
							pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
						} else {
							int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(pMobile))
												->GetActorValue(ActorValue::MOOD);
							pFaceData->SetExpressionTargetFromMood(iMood, true);
						}
					} else {
						pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
					}
				}
			}
			if (pDialoguePackage) {
				pPlayer->CheckForClosesConversation(pDialoguePackage->GetActorStartedConversation(),
					pDialoguePackage->GetTargetOfConversation(), pDialoguePackage);
				if (pDialoguePackage == pPlayer->pClosestConversation || ReferenceProcessLists.bShowSubtitle) {
					Conversation* pConversation = pDialoguePackage->GetPackConversation();
					if (pConversation) {
						if (pConversation->GetCurrentItem()) {
							if (pConversation->GetCurrentItem()->GetCurrentResponse()) {
								const char* pText =
									pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String();
								if (pText) {
									if (gs_bGeneralSubtitles_Gameplay.Bool() ||
										(IsActor() && static_cast<Actor*>(this)->GetShowSubtitles())) {
										BSSoundHandle kHandle =
											GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
										Interface::ShowSubtitle(const_cast<char*>(pText), kHandle,
											GetLocationOnReference(), pPlayer, false);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	cSoundCallBackSet = -1;
}

bool MobileObject::DetachHavok() {
	if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
		static_cast<HighProcess*>(pCurrentProcess)->RemovePhantom();
	}
	bool bDetached = static_cast<MobileObject*>(this)->DetachCharController();
	NiNode* pNode = Get3D_Body();
	if (pNode) {
		bhkWorld::RemoveObjects(pNode, true, false);
		bDetached |= true;
	}
	return bDetached;
}

// The exe's cleanup-only C++ EH frame is left out.
void MobileObject::InitHavok() {
	Actor* pactor = static_cast<Actor*>(this);
	RemoveFromAllWater(0);
	if (pactor->pCurrentProcess) {
		if (!pactor->pCurrentProcess->eLevel || pactor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH) {
			TESObjectCELL* pcell = pactor->GetParentCell();
			HighProcess* pBaseProc =
				pactor->pCurrentProcess->eLevel ? 0 : static_cast<HighProcess*>(pactor->pCurrentProcess);
			if (!pcell || (pcell->GetAttached() || pcell->IsAttaching())) {
				bool v40;
				bhkWorld* pw;
				if (pcell) {
					pw = pcell->GetbhkWorld();
				} else {
					pw = 0;
				}
				Actor* pactor_2 = pactor->IsActor() ? pactor : 0;
				if (pactor_2) {
					ACTOR_CRITICAL_STAGE CriticalStage = pactor_2->eCriticalStage;
					if (CriticalStage == ACS_GOO_END || CriticalStage == ACS_DISINTEGRATE_END) {
						return;
					}
					// All flags: any reason to keep the actor's Havok data.
					v40 = pactor_2->GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
						pactor_2->CheckShouldSaveHavokData(0xFFFFFFFFu);
					NiNode* v1 = pactor->TESObjectREFR::Get3D();
					bhkNiCollisionObject::SetRigidBodyName(v1);
				} else {
					v40 = 0;
				}
				ExtraDismemberedLimbs* pdismemberedLimbs = (&pactor->m_Extra)->GetDismembermentExtra();
				if (pdismemberedLimbs) {
					if (pactor->pCurrentProcess) {
						if (pactor->Get3D()) {
							uint32_t uilimbCount = pdismemberedLimbs->GetDismemberedLimbCount();
							for (uint32_t ui = 0; ui < uilimbCount; ++ui) {
								DismemberedLimb* plimb = pdismemberedLimbs->GetDismemberedLimb(ui);
								NiNode* plimbRoot =
									pactor->pCurrentProcess
										->GetDamageNode(static_cast<BGSBodyPart::LIMB_ENUM>(plimb->cLimb));
								NiNode* plimbParent = plimbRoot->m_pkParent;
								FNVD_INLINED_LOCAL(NiFixedString, dismemberRootName, std::nullopt, std::nullopt);
								::new (static_cast<void*>(&dismemberRootName))
									NiFixedString(static_cast<const char*>(pLimbEnumNames[plimb->cLimb]));
								if (!plimbParent ||
									!NiFixedString::HandlesEqual(plimbParent->GetName(), dismemberRootName)) {
									for (uint32_t i = 0; i < plimbRoot->GetArrayCount(); ++i) {
										NiAVObject* pchild = plimbRoot->GetAt(i);
										if (pchild) {
											bhkWorld::SetDisabled(pchild, 1, 1, 1);
										}
									}
								}
								dismemberRootName.~NiFixedString();
							}
						}
					}
				}
				bhkCharacterController* pcon = pactor->GetCharController();
				if (!pcon) {
					bhkCharacterController* pcon_2;
					FNVD_CONSTRUCTED_LOCAL(bhkCharacterControllerCinfo, info, &bhkCharacterControllerCinfo::Destruct,
						1);
					info.pObject = pactor->TESObjectREFR::Get3D();
					info.SetPosition(*pactor->QLocationOnReference());
					info.SetWorld(0);
					info.SetAcrobatics(pactor->GetAcrobatics());
					if (pactor_2 && (pactor->GetObjectReference() && pactor_2->IsCreature())) {
						bool TiltFrontBack =
							static_cast<TESCreature*>(pactor_2->GetObjectReference())->TESCreature::GetTiltFrontBack();
						info.SetPitch(TiltFrontBack);
						info.SetRoll(static_cast<TESCreature*>(pactor_2->GetObjectReference())
								->TESCreature::GetTiltLeftRight());
						info.SetScale(pactor->GetScale());
					} else {
						info.SetUseDefaultShape(1);
					}
					bhkCharacterController* v28 = static_cast<
						bhkCharacterController*>(bhkRagdollController::operator new(sizeof(bhkCharacterController)));
					if (v28) {
						pcon_2 = ::new (v28) bhkCharacterController(info);
					} else {
						pcon_2 = 0;
					}
					pcon = pcon_2;
					if (pactor_2) {
						if (pactor->GetObjectReference()) {
							bool NotPushable = pactor->GetActorBase()->GetNotPushable();
							pcon->SetFlag(bhkCharacterController::NOTPUSHABLE_PERMANENT |
									bhkCharacterController::NOTPUSHABLE,
								NotPushable);
						}
					}
					// The by-value NiPointer argument (built with 0x633C90) is released by the callee.
					pactor->pCurrentProcess->SetCharController(pcon);
					pcon->SetMOBObject(pactor);
					pcon->SetScale(pactor->GetScale());
					bool IsComplex = Main::GetSceneIsComplex();
					pcon->SetQuickSimulate(IsComplex);
					pcon->SetFlag(bhkCharacterController::RECORD_HITS, 1);
					if (pBaseProc) {
						bhkSimpleShapePhantom* pdoorPhantom =
							static_cast<bhkSimpleShapePhantom*>(pBaseProc->QDoorDetectionPhantom());
						if (pdoorPhantom) {
							CFilter ifilter;
							CFilter kControllerFilter;
							pdoorPhantom->GetCollisionFilter(&ifilter);
							CFilter* CollisionFilter = pcon->GetCollisionFilter(&kControllerFilter);
							uint32_t Group = CollisionFilter->GetGroup();
							ifilter.SetGroup(Group);
							pdoorPhantom->SetCollisionFilter(ifilter);
						}
					}
				}
				if (pw) {
					if (pw->IsBatchingAdds()) {
						pcon->SetWaitForBatchAdd(1);
					}
				}
				if (pactor_2) {
					if (v40) {
						// MobileObject::GetCollisionFilter writes the filter word; the exe reads the group from the
						// filter it returns (the same address).
						CFilter kActorFilter;
						pactor_2->GetCollisionFilter(&kActorFilter.iFilter);
						uint32_t uiGroup = kActorFilter.GetGroup();
						NiNode* v9 = pactor_2->Get3D();
						pactor_2->pPenetrationDetection->Initialize(v9, static_cast<int32_t>(uiGroup));
					}
				}
				NiAVObject* poldAV = pcon->GetNiObject();
				NiAVObject* pav = pactor->TESObjectREFR::Get3D();
				if (poldAV != pav) {
					bhkWorld* pw_3;
					pcon->SetNiObject(pav);
					FNVD_CONSTRUCTED_LOCAL(NiColorA, color, nullptr, 0.0f, 0.0f, 0.0f, 0.0f);
					color.r = 1.0f;
					color.g = 0.0f;
					color.b = 0.0f;
					color.a = 0.25f;
					TESObjectCELL* pcell_2 = pactor->GetParentCell();
					if (pcell_2) {
						pw_3 = pcell_2->GetbhkWorld();
					} else {
						pw_3 = 0;
					}
					if (pw_3) {
						if (pw_3->QDebugDisplay()) {
							pcon->BuildDisplayGeometry(0);
							NiAVObject* poldAV_2 = pcon->GetNiObject();
							BSShaderUtil::RecursiveSetPropertyWireframe(poldAV_2, true);
						}
					}
					bhkWorld::SetMotion(pav, v40 ? hkpMotion::MOTION_DYNAMIC : hkpMotion::MOTION_KEYFRAMED, 1, 0, 0);
					if (pactor_2) {
						if (!pactor_2->IsDead(0)) {
							pactor_2->CreateRagdollInstance(pav, 1, 1, 0);
							if (pactor == pPlayer) {
								if (pactor_2->pRagdollController) {
									pactor_2->pRagdollController->SetUsePhantomFootIKCasts(0);
									pactor_2->pRagdollController->SetUseFootEndCast(1);
								}
							}
						}
					}
				}
				if (pactor->pCurrentProcess) {
					if (!pactor->pCurrentProcess->eLevel && pactor != pPlayer) {
						if (pactor_2) {
							if (pactor_2->GetActorBase()) {
								if (!pactor_2->GetTurretBehavior()) {
									HighProcess* phigh = static_cast<HighProcess*>(pactor->pCurrentProcess);
									if (!phigh->CheckPhantom()) {
										phigh->SetupPhantom(pactor_2);
									}
								}
							}
						}
					}
				}
				pactor->MoveHavok(0);
				if (v40 && !(&pactor->m_Extra)->GetSavedHavokData() && !(&pactor->m_Extra)->GetRagDollData()) {
					NiPoint3 vel(0.0f, 0.0f, -1.0f);
					TESHavokUtilities::AddVelocity(pav, vel, 1);
				}
			}
		}
	}
}

NiAVObject* MobileObject::Load3D(bool abBackgroundLoading) {
	NiAVObject* pObject = TESObjectREFR::Load3D(abBackgroundLoading);
	HandleTextureUpgrade(false);
	if (pObject && pCurrentProcess) {
		NiExtraData* pExtra = pObject->GetExtraData(BSBound::Tag());
		if (pExtra) {
			pCurrentProcess->SetBSBound(static_cast<BSBound*>(pExtra));
		}
	}
	return pObject;
}

NiPoint3 MobileObject::GetBoundMin() {
	if (pCurrentProcess && pCurrentProcess->GetBSBound()) {
		BSBound* pbsBound = pCurrentProcess->GetBSBound();
		pbsBound->GetBoundMin(MobileObject_boundMin);
		return MobileObject_boundMin;
	}
	return TESObjectREFR::GetBoundMin();
}

NiPoint3 MobileObject::GetBoundMax() {
	if (pCurrentProcess && pCurrentProcess->GetBSBound()) {
		BSBound* pbsBound = pCurrentProcess->GetBSBound();
		pbsBound->GetBoundMax(MobileObject_boundMax);
		return MobileObject_boundMax;
	}
	return TESObjectREFR::GetBoundMax();
}

void MobileObject::SetRunsInLow(bool abFlag) {
	bRunsInLow = abFlag;
}

void MobileObject::MoveHavok(bool abForceRec) {
	auto* pCharController = static_cast<bhkCharacterController*>(GetCharController());
	if (!pCharController) {
		return;
	}
	Actor* pActor = IsActor() ? static_cast<Actor*>(this) : nullptr;
	bool bIsDead = pActor && pActor->GetLifeState() == ACTOR_LIFE_STATE_DEAD;
	TESObjectCELL* pParentCell = GetParentCell();
	bhkWorld* pParentHavokWorld = pParentCell ? pParentCell->GetbhkWorld() : nullptr;
	bhkWorld* pCharControllerWorld = pCharController->GetWorld();
	if (pParentHavokWorld != pCharControllerWorld) {
		pCharController->SetWantState(HK_CHARACTER_ON_GROUND);
		float fWaterHeight = pParentCell ? pParentCell->GetWaterHeight() : 0.0f;
		pCharController->SetWaterHeight(fWaterHeight);
		if (!bIsDead) {
			pCharController->SetWorld(pParentHavokWorld);
		}
		// The exe calls the shared getter 0x436AA0 (TESObjectREFR::GetLocationOnReference's code) directly.
		pCharController->SetPosition(*QLocationOnReference());
		if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH && this != pPlayer && pActor &&
			pActor->GetActorBase() && !pActor->GetTurretBehavior()) {
			static_cast<HighProcess*>(pCurrentProcess)->SetupPhantom(pActor);
		}
	}
	bool bWorldChanged = false;
	NiAVObject* pNiObject = pCharController->GetNiObject();
	NiNode* pObject = TESObjectREFR::Get3D();
	if (pNiObject != pObject) {
		pCharController->SetNiObject(pObject);
	}
	if (pCharControllerWorld != pParentHavokWorld) {
		if (pCharControllerWorld) {
			bhkWorld::RemoveObjects(pObject, true, false);
		}
		bWorldChanged = true;
	}
	if (!pParentHavokWorld) {
		return;
	}
	if (bWorldChanged) {
		CFilter kFilter;
		pCharController->GetCollisionFilter(&kFilter);
		uint32_t uiGroup = kFilter.GetGroup();
		bool bIsBatchingAdds = pParentHavokWorld->IsBatchingAdds();
		if (!bIsBatchingAdds) {
			pParentHavokWorld->StartBatchingAdds();
		}
		pParentHavokWorld->AddObjects(pObject, true, false, uiGroup, false);
		if (!bIsBatchingAdds) {
			pParentHavokWorld->StopBatchingAdds(false);
		}
	} else {
		bhkWorld::UpdatePosition(pObject, true, false);
	}
	// The exe reloads pRagdollController for every use.
	if (pActor && pActor->pRagdollController && pActor->pRagdollController->QRagdollAnimInit()) {
		if (!IsDead(false)) {
			pActor->pRagdollController->SyncRagdollTo3D();
		}
		if (pActor->GetTurretBehavior()) {
			pActor->pRagdollController->DisableRagdollAnim(true);
		}
	}
	if (GetSitSleepState() && GetCurrentAIProcess()) {
		BaseProcess* pMarkerProcess = GetCurrentAIProcess();
		BaseProcess* pFurnitureProcess = GetCurrentAIProcess();
		BaseProcess* pProcess = GetCurrentAIProcess();
		int32_t iFurnitureNum = pMarkerProcess->GetFurnitureMarker()->cNumber;
		TESObjectREFR* pFurniture = pFurnitureProcess->GetCurrentFurniture();
		if (pProcess->ShouldDisableCharController(pFurniture, iFurnitureNum)) {
			SetSize(0.1f);
			SetChaseBip(true);
			pCharController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, true);
		}
	}
}

void MobileObject::SetParentCell(TESObjectCELL* apCell) {
	if (apCell == GetParentCell()) {
		return;
	}
	TESObjectREFR::SetParentCell(apCell);
	if (apCell && !IsTalkingActivator()) {
		auto* pController = static_cast<bhkCharacterController*>(GetCharController());
		if (pController) {
			pController->SetWaterHeight(GetRelevantWaterHeight());
			// The exe calls Actor::DoesFly directly on every mobile object, projectiles included. The sentinel is
			// the qword -FLT_MAX at 0x10241B0, compared with the unrounded height: test ah,0x44 / jp skips on
			// unequal or unordered.
			if (static_cast<Actor*>(this)->DoesFly() && pController->GetState() == HK_CHARACTER_USER_STATE_0 &&
				GetRelevantWaterHeight() == -3.4028234663852886e+38 && pLoadedData) {
				const NiPoint3* pPosition = &GetLocationOnReference();
				pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(*pPosition, nullptr);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool MobileObject::MovetoHigh() {
	HighProcess* v2;
	PROCESS_TYPE aWhichList = pCurrentProcess->eLevel;
	ReferenceProcessLists.kUpdateLock.Lock(0);
	ReferenceProcessLists.RemoveReference(this, aWhichList);
	HighProcess* apMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
	if (apMemory) {
		v2 = ::new (apMemory) HighProcess;
	} else {
		v2 = 0;
	}
	v2->Copy(pCurrentProcess);
	if (pCurrentProcess) {
		delete pCurrentProcess;
	}
	pCurrentProcess = v2;
	ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, 0, 0, 0);
	ReferenceProcessLists.kUpdateLock.Unlock();
	MoveHavok(0);
	pCurrentProcess->SetLastSeenLocation();
	return 1;
}

// The exe calls it directly as well as through the vtable.
// The exe's cleanup-only C++ EH frame is left out.
bhkCharacterController* MobileObject::Move(float afDeltaTime, NiPoint3& arDeltaMove, uint32_t aiMoveDir) {
	const hkVector4* SupportNormal;
	const hkVector4* X_4;
	float v25;
	MoveData md_2;
	alignas(16) NiMatrix3 v46;
	NiMatrix3 rot_4;
	MoveData md;
	hkQuaternion hkrot;
	alignas(16) float fheightNavMesh;
	TESObjectCELL* pc;
	TESWorldSpace* pws;
	NiMatrix3 rot_3;
	NiPoint3 tmpMove;
	NiPoint3 tmpLoc;
	int32_t v71;
	bool bisImmobile;
	NiMatrix3 v82;
	NiMatrix3 rot_2;
	NiPoint3 v90;
	Actor* pactor_2 = static_cast<Actor*>(this);
	Actor* pactor = IsActor() ? static_cast<Actor*>(this) : 0;
	bhkCharacterController* pcon = pactor_2->GetCharController();
	bool bIsProjectile = pactor_2->IsProjectile();
	if (pactor) {
		if (pactor->GetActorValue(ActorValue::TURBO) > 0) {
			afDeltaTime = afDeltaTime / BSTimer::QGlobalTimeMultiplier();
		}
	}
	alignas(16) NiPoint3 pos = *pactor_2->QLocationOnReference();
	if (!std::isnan(arDeltaMove.x) && !std::isnan(arDeltaMove.y)) {
		std::isnan(arDeltaMove.z);
	}
	float fanglex = (&pactor_2->GetAngleOnReference())->x;
	float fangley = (&pactor_2->GetAngleOnReference())->y;
	float fanglez = (&pactor_2->GetAngleOnReference())->z;
	if (fanglex == 3.4028234663852886e+38 ||
		(std::isnan(fanglex) ||
			(!std::isfinite(fanglex) ||
				(fangley == 3.4028234663852886e+38 ||
					(std::isnan(fangley) ||
						(!std::isfinite(fangley) ||
							(fanglez == 3.4028234663852886e+38 ||
								(std::isnan(fanglez) || !std::isfinite(fanglez))))))))) {
		int32_t CastingType = pactor_2->iFormID;
		Warning("AI: MobileObject::Move called on '%s' (%08X) with invalid angle.", pactor_2->GetFormEditorID(),
			CastingType);
		NiPoint3 X = *::new (&v90) NiPoint3(0.0f, 0.0f, 0.0f);
		pactor_2->SetAngleOnReference(X);
	}
	if (gs_iHavokSkipFrameCountTEST_Havok.Int()) {
		float v23;
		NiPoint3 largeDelta_2;
		uint32_t iFrameCounter = get_iFrameCounter();
		if (iLastFrameCounter != iFrameCounter) {
			bSkipHavokThisFrame = iFrameCounter % (gs_iHavokSkipFrameCountTEST_Havok.Int() + 1) != 0;
			iLastFrameCounter = iFrameCounter;
		}
		if (bSkipHavokThisFrame && pcon->QSupported()) {
			float Heading;
			alignas(16) NiMatrix3 rot;
			if (pactor_2 == pPlayer) {
				Heading = pactor_2->MobileObject::GetHeading(0);
			} else {
				Heading = pactor_2->GetHeading(0);
			}
			rot.MakeZRotation(Heading);
			arDeltaMove = rot * arDeltaMove;
			NiNode* v4 = pactor_2->Get3D();
			pos = v4->GetTranslate() + arDeltaMove;
			NiNode* v5 = pactor_2->Get3D();
			v5->SetTranslate(pos);
			return pcon;
		}
		const NiPoint3* v22 = &pactor_2->GetLocationOnReference();
		NiNode* v7 = pactor_2->Get3D();
		NiPoint3 largeDelta = v7->GetTranslate() - *v22;
		if (pactor_2 == pPlayer) {
			v23 = pactor_2->MobileObject::GetHeading(0);
		} else {
			v23 = pactor_2->GetHeading(0);
		}
		rot_2.MakeZRotation(v23);
		NiMatrix3* v8 = rot_2.Inverse(&v82);
		arDeltaMove.operator+=(*::new (&largeDelta_2) NiPoint3(*v8 * largeDelta));
	}
	if (pactor) {
		bisImmobile = pactor->IsImmobile();
	} else {
		bisImmobile = 0;
	}
	SIT_SLEEP_STATE eSitSleepState = pactor_2->GetSitSleepState();
	SIT_SLEEP_STATE v78 = static_cast<SIT_SLEEP_STATE>(eSitSleepState &&
		(pcon &&
			(X_4 = &hkVector4::getZero(),
				(SupportNormal = &pcon->GetSupportNormal(), SupportNormal->equals4(*X_4, 0.001f)))));
	bool v77 = v78;
	int32_t v76 = eSitSleepState == SIT_SLEEP_NORMAL || v77;
	bool v75 = v76;
	const NiPoint3* WorldTranslate = &Main::WorldRootCamera()->GetWorldTranslate();
	const NiPoint3* v10 = &pactor_2->GetLocationOnReference();
	NiPoint3 diff = *v10 - *WorldTranslate;
	float v73 = diff.SqrLength();
	if (pactor) {
		int32_t v72 = pactor->IsUnConscious() || (pactor->IsDead(0) || (pactor->IsDying() || pactor->IsKnockedOut()));
		v71 = v72;
	} else {
		v71 = 0;
	}
	bool v70 = v71;
	if (!(gs_fCharControllerWarpDistSqr_HAVOK.Float() < static_cast<double>(v73)) ||
		(pactor_2->IsProjectile() ||
			(bisImmobile ||
				(v70 ||
					(pactor_2 == pPlayer ||
						((tmpLoc = pos, (tmpMove = arDeltaMove, pactor_2 != pPlayer))
								? v25 = pactor_2->GetHeading(0)
								: v25 = pactor_2->MobileObject::GetHeading(0),
							(rot_3.MakeZRotation(v25),
								(tmpMove = rot_3 * tmpMove,
									(&tmpLoc.operator+=(tmpMove),
										(pws = pactor_2->GetWorldSpace(),
											(pc = pactor_2->GetParentCell(),
												!Pathing::GetNavMeshZForLocation(tmpLoc, pc, pws,
													fheightNavMesh)))))))))))) {
		if (v75) {
			if (pactor_2 == pPlayer) {
				pcon->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, bisImmobile);
			}
			if (!pcon || pactor_2->IsDead(1)) {
				float v30;
				NiMatrix3 rot_5;
				if (pactor_2->IsProjectile()) {
					float Looking = pactor_2->GetLooking();
					rot_5.MakeXRotation(Looking);
					arDeltaMove = rot_5 * arDeltaMove;
				}
				if (pactor_2 == pPlayer) {
					v30 = pactor_2->MobileObject::GetHeading(0);
				} else {
					v30 = pactor_2->GetHeading(0);
				}
				rot_5.MakeZRotation(v30);
				arDeltaMove = rot_5 * arDeltaMove;
				pos.operator+=(arDeltaMove);
			} else {
				md.~MoveData();
				::new (&md) MoveData;
				md.fTime = afDeltaTime;
				md.Angle = pactor_2->GetAngleOnReference();
				md.Displacement = arDeltaMove;
				md.iMoveDirection = aiMoveDir;
				md.bVisible = 1;
				if (pactor_2 != pPlayer) {
					if (pactor) {
						bool isNotFadedOut;
						BSFadeNode* pfadeNode;
						NiAVObject* p3d = (NiAVObject*)pactor_2->Get3D();
						if (p3d) {
							pfadeNode = p3d->IsFadeNode();
						} else {
							pfadeNode = 0;
						}
						if (pfadeNode) {
							NiCamera* v13 = Main::WorldRootCamera();
							isNotFadedOut = pfadeNode->ShouldUpdate(v13, 0.8f);
						} else {
							isNotFadedOut = 1;
						}
						md.bVisible = isNotFadedOut;
					}
				}
				if (pcon->QChaseBip() && !v77) {
					float v27;
					NiPoint3 oldPos;
					if (pactor_2 == pPlayer) {
						v27 = pactor_2->MobileObject::GetHeading(0);
					} else {
						v27 = pactor_2->GetHeading(0);
					}
					rot_4.MakeZRotation(v27);
					arDeltaMove = rot_4 * arDeltaMove;
					pos.operator+=(arDeltaMove);
					pcon->GetPosition(oldPos);
					arDeltaMove = pos - oldPos;
					NiMatrix3* v14 = rot_4.Inverse(&v46);
					md.Displacement = *v14 * arDeltaMove;
				}
				if (pcon->QCanPitch() || pcon->QCanRoll()) {
					pcon->SetStuckQuadruped(0);
					float* v15 = &fQuadrupedPitchMult.Float();
					float v45 = appTimer.fDelta * *v15;
					pcon->SetPitchMult(v45);
				}
				if (pactor && (pactor->GetMoveMode() & 63) != 0) {
					md.fCurrMoveSpeed = pactor->GetCurrentSpeed();
				} else {
					md.fCurrMoveSpeed = 0.0f;
				}
				if (bisImmobile && (pactor_2 == pPlayer || !pactor->HasRagDoll())) {
					pcon->MoveImmobile(md, pactor_2 == pPlayer);
				} else {
					pcon->Move(md);
				}
				if (pcon->QStuckQuadruped()) {
					alignas(16) NiPoint3 temppos;
					pcon->GetPosition(temppos);
					float Radius = pcon->GetRadius();
					double v16 = HK2NI(Radius);
					float v43 = v16 + v16;
					temppos.z = temppos.z + v43;
					pactor_2->SetPosition(temppos);
				}
				if (!pcon->QChaseBip()) {
					pcon->GetPosition(pos);
					if (pcon->QQuadruped() && !bisImmobile) {
						pactor_2->SetHeading(md.Angle.z);
					}
					const NiPoint3* v17 = &pactor_2->GetLocationOnReference();
					if ((pos - *v17).SqrLength() > 67108864.0) {
						pos = pactor_2->GetLocationOnReference();
						pactor_2->SetPosition(pos);
					}
					float fheight = 0.0f;
					if (!TES::pInstance->pInteriorCell) {
						TES::pInstance->GetLandHeight(pos, fheight);
						if (fheight - pos.z > 30.0) {
							pos.z = fheight;
							pactor_2->SetPosition(pos);
						}
					}
				}
				if (pactor) {
					pactor->ProcessDamageImpacts();
				}
			}
		} else {
			if (pactor_2->IsDead(0) && !bisImmobile) {
				float v31;
				NiMatrix3 rot_6;
				if (pactor_2 == pPlayer) {
					v31 = pactor_2->MobileObject::GetHeading(0);
				} else {
					v31 = pactor_2->GetHeading(0);
				}
				rot_6.MakeZRotation(v31);
				arDeltaMove = rot_6 * arDeltaMove;
				pos.operator+=(arDeltaMove);
			}
		}
		if (eSitSleepState || v77) {
			float v32;
			NiMatrix3 rot_7;
			md_2.~MoveData();
			::new (&md_2) MoveData;
			md_2.fTime = afDeltaTime;
			md_2.Angle = pactor_2->GetAngleOnReference();
			md_2.Displacement = arDeltaMove;
			md_2.iMoveDirection = aiMoveDir;
			md_2.bVisible = 1;
			if (!v77) {
				pcon->MoveImmobile(md_2, 0);
			}
			if (v77) {
				pcon->SetSupported(1);
				pcon->SetTrueState(HK_CHARACTER_ON_GROUND);
			}
			if (pactor_2 == pPlayer) {
				v32 = pactor_2->MobileObject::GetHeading(0);
			} else {
				v32 = pactor_2->GetHeading(0);
			}
			rot_7.MakeZRotation(v32);
			arDeltaMove = rot_7 * arDeltaMove;
			pos.operator+=(arDeltaMove);
		}
		pactor_2->SetLocationOnReference(pos);
		return pcon;
	} else {
		if (pcon && pcon->GetState() == HK_CHARACTER_USER_STATE_0) {
			if (pactor_2->pLoadedData->fRelevantWaterHeight == -3.4028234663852886e+38) {
				const NiPoint3* v11 = &pactor_2->GetLocationOnReference();
				double WaterHeight = TES::pInstance->GetWaterHeight(*v11, 0);
				pactor_2->pLoadedData->fRelevantWaterHeight = WaterHeight;
			}
			float fheightWater = pactor_2->GetRelevantWaterHeight();
			if (fheightWater == -3.4028234663852886e+38) {
				if (pcon) {
					if (pactor_2->DoesFly()) {
						pcon->SetWantState(HK_CHARACTER_IN_AIR);
					} else {
						pcon->SetWantState(HK_CHARACTER_ON_GROUND);
					}
					tmpLoc.z = fheightNavMesh;
				}
			} else {
				if (!(fheightWater < static_cast<double>(tmpLoc.z))) {
					if (fheightNavMesh > static_cast<double>(tmpLoc.z) &&
						fheightWater > static_cast<double>(fheightNavMesh)) {
						tmpLoc.z = fheightNavMesh;
					}
				} else {
					tmpLoc.z = fheightWater;
				}
			}
		} else {
			if (eSitSleepState == SIT_SLEEP_NORMAL && !v77) {
				tmpLoc.z = fheightNavMesh;
			}
		}
		if (pcon) {
			if (pcon->GetWorld()) {
				if (!static_cast<uint8_t>(pcon->QFarAway()) &&
					(pcon->QCanPitch() || (pcon->QCanPitch() || pcon->QQuadruped()))) {
					hkVector4 supNorm;
					alignas(16) NiPoint3 hitNormal;
					FNVD_CONSTRUCTED_LOCAL(PathingLocation, loc, &PathingLocation::Destruct, pactor_2);
					Pathing::GetNavmeshNormalForLocation(loc, hitNormal);
					NI2HK(supNorm, hitNormal);
					pcon->SetSupportNormal(supNorm);
					pcon->ResetCalculatePitchTimer();
					pcon->InitFrontBack();
					pcon->CalculatePitchAndRoll();
					float v58 = (&pactor_2->GetAngleOnReference())->z;
					hkrot.~hkQuaternion();
					::new (&hkrot) hkQuaternion;
					hkrot.setAxisAngle(pcon->UpVec, -v58);
					pcon->ApplyPitchAndRoll(hkrot);
				}
			}
		}
		pactor_2->SetPosition(tmpLoc);
		if (pcon) {
			if (pcon->GetState() == HK_CHARACTER_IN_AIR) {
				pcon->SetTrueState(HK_CHARACTER_ON_GROUND);
			}
		}
		return pcon;
	}
}

// The jump height is fJumpHeightMin scaled by the reference's scale, times fJumpSwimmingMult when swimming. The
// exe looks the setting up before it reads the scale.
bhkCharacterController* MobileObject::Jump() {
	bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
	if (pCharController) {
		float* pfJumpHeightMin = &fJumpHeightMin.Float();
		float fJumpHeight = GetScale() * *pfJumpHeightMin;
		if (pCharController->QIsSwimming()) {
			fJumpHeight = fJumpHeight * fJumpSwimmingMult.Float();
		}
		pCharController->Jump(fJumpHeight);
	}
	return pCharController;
}

void MobileObject::Process(float afDelta) {
	ExtraDataList* extra;
	TESObjectREFR* talkingActivator;
	UpdateSoundCallBack();
	if (pCurrentProcess) {
		pCurrentProcess->Update(this);
	}
	if (IsTalkingActivator() && (!IsinDialogue())) {
		talkingActivator = GetTalkingActivator();
		if (talkingActivator) {
			extra = talkingActivator->TESObjectREFR::GetExtra();
			extra->RemoveTalkingActorExtra();
		}
		(&ReferenceProcessLists)->RemoveReference(this, PROCESS_TYPE_HIGH);
		(&ReferenceProcessLists)->AddActorToTempChangeList(static_cast<Actor*>(this));
	}
}

bool MobileObject::ChangeProcessLevel() {
	bool v4 = 1;
	if (Get3D()) {
		TESObjectCELL* ParentCell = GetParentCell();
		if (TES::pInstance->IsCellLoaded(ParentCell, 1)) {
			if (pCurrentProcess->eLevel) {
				MovetoHigh();
			} else {
				if (this) {
					delete this;
				}
				return 0;
			}
		}
	}
	return v4;
}

// The exe's cleanup-only C++ EH frame is left out.
void MobileObject::UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, handle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, phan, std::nullopt, std::nullopt);
	Animation* panim = GetAnimation();
	DialoguePackage* pdia = 0;
	BaseProcess* CurrentAIProcess = pCurrentProcess;
	TESPackage* pbasepack = CurrentAIProcess->GetCurrentPackage();
	if (pbasepack) {
		if (pbasepack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
			pdia = static_cast<DialoguePackage*>(pbasepack);
		}
	}
	bool bPlayIdle = 0;
	bool btalkflag = 0;
	if (pResponse) {
		if (pCurrentProcess) {
			if (bSpeakingDone) {
				::new (&phan) BSSoundHandle;
				btalkflag = 1;
				pTalkingToPlayer = static_cast<Actor*>(this);
				bTalkingToPlayer = 1;
				bool bnosound = 1;
				if (pResponse->pVoiceSound) {
					uint32_t isoundFileID = pResponse->pVoiceSound->iFormID;
					::new (&handle) BSSoundHandle;
					if (GetTalkingActivator()) {
						TESObjectREFR* pref = GetTalkingActivator();
						BSAudio* v4 = &BSAudio::QInstance();
						BSSoundHandle* arHandle = v4->GetSoundHandleByNumericID(&a1, isoundFileID, 261u);
						handle.operator=(*arHandle);
						handle.Play(0);
					} else {
						BSSoundHandle* v17 = PlaySound(&a2, isoundFileID, 0, 0x40000102u, 1);
						handle.operator=(*v17);
					}
					BSAudioManager* mgr = &BSAudioManager::QInstance();
					// The callback's context is the speaker's form ID.
					mgr->SetCompletionCallback(static_cast<uint32_t>(handle.iSoundID), &MobileObject::SoundFileCallBack,
						reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)));
					bnosound = 0;
					bSoundFileDone = 0;
					bSpeakingDone = 0;
				}
				if (bSoundFileDone && bnosound) {
					PlayerCharacter* v10 = pPlayer;
					TESIdleForm* ListenerIdle = pResponse->pListenIdle;
					TESIdleForm* SpeakerIdle = pResponse->pSpeakerIdle;
					uint32_t Length = pResponse->cText.GetLength();
					uint32_t v6 = pResponse->uiEmotion;
					DIALOGUE_EMOTION FormFlags = pResponse->eEmotion;
					float fdelay = SpeakSoundFunction(pResponse->GetVoice()->pString, phan, FormFlags, v6, Length,
						SpeakerIdle, ListenerIdle, v10, 1, 1, 0, 1, 1);
					BSAudioManager* mgr_2 = &BSAudioManager::QInstance();
					mgr_2->SetCompletionCallback(static_cast<uint32_t>(phan.iSoundID),
						&MobileObject::ChangeDialogueAnimation,
						reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)));
					bVoiceFileDone = 0;
					BaseProcess* v16 = pCurrentProcess;
					v16->SetSoundDelay(fdelay);
				}
			}
		}
	}
	if (!abChangeAnimationEndVoice) {
		UpdateAnimation(afDelta);
	}
	UpdateSoundCallBack();
	if (!abChangeAnimationEndVoice) {
		Update(afDelta);
	}
	if (btalkflag) {
		if (pdia) {
			pdia->pTalkingActor = this;
		}
	}
	TESObjectCELL* pcell = GetParentCell();
	if (pcell) {
		pcell->UpdateLights();
	}
	UpdateRefLight();
}

void MobileObject::UpdateAnimation(float afDelta) {
	if (IsTalkingActivator()) {
		TESObjectREFR* pref = pTalkingActivator;
		if (pref->Get3D()) {
			NiNode* v2 = pref->Get3D();
			NiControllerManager* pmanager =
				static_cast<NiControllerManager*>(v2->GetController(&NiControllerManager::ms_RTTI));
			if (pmanager) {
				NiFixedString* v3 = &FixedStrings::QTalking();
				NiControllerSequence* pseq = pmanager->GetSequenceByName(*v3);
				if (pseq) {
					int32_t iframe = 0;
					BaseProcess* CurrentAIProcess = pCurrentProcess;
					uint32_t v4 = CurrentAIProcess->GetCreatureLipSynchStartTime();
					if (appTimer.iLastTime > v4) {
						BaseProcess* v14 = pCurrentProcess;
						uint32_t v5 = v14->GetCreatureLipSynchStartTime();
						iframe = (appTimer.iLastTime - v5) / 33;
					}
					BaseProcess* v13 = pCurrentProcess;
					if (v13->GetCreatureLipSynchAnim()) {
						BaseProcess* v12 = pCurrentProcess;
						LipSynchAnim* v6 = v12->GetCreatureLipSynchAnim();
						float flipPercent = v6->GetPuppetValue(iframe);
						if (flipPercent < 0.009999999776482582) {
							flipPercent = 0.01f;
						}
						Animation::SetLipTime(flipPercent, pmanager, pseq);
						BaseProcess* v11 = pCurrentProcess;
						if (iframe <= v11->GetCreatureLipSynchAnim()->iNumFrames) {
							Animation::SetLipTime(flipPercent, pmanager, pseq);
						} else {
							Animation::SetLipTime(0.0f, pmanager, pseq);
							pmanager->DeactivateSequence(pseq, 0.0f);
							BaseProcess* v10 = pCurrentProcess;
							v10->SetCreatureLipSynchAnim(0);
							BaseProcess* v9 = pCurrentProcess;
							v9->SetCreatureLipSynchStartTime(0);
						}
					}
				}
			}
			pTalkingActivator->UpdateAnimation();
			float afTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, updateData, nullptr, afTime, 1, 0);
			NiNode* v7 = pref->Get3D();
			v7->Update(updateData);
		}
	}
}

void MobileObject::SetDialoguewithPlayer(bool abFlag) {
	NiNode* parent;
	BSFaceGenNiNode* v4;
	NiTriShape* v5;
	BSFaceGenNiNode* v6;
	if (pCurrentProcess) {
		pCurrentProcess->SetTalkingtoPC(abFlag);
		v6 = pCurrentProcess->GetFaceNode();
		v4 = pCurrentProcess->GetFaceSkinnedNode();
		v5 = static_cast<NiTriShape*>(pCurrentProcess->GetHeadAnims());
		if (v5) {
			if (v5->NiAVObject::GetParent()) {
				if (v6) {
					if (v4) {
						parent = v5->NiAVObject::GetParent();
						parent->SetSelectiveUpdate(1);
						v6->SetSelectiveUpdate(1);
						v4->SetSelectiveUpdate(1);
					}
				}
			}
		}
		if (abFlag) {
			pCurrentProcess->ClearAllHeadTrackTargets();
			pCurrentProcess->SetDialogHeadTrackTarget(pPlayer);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool MobileObject::InitiateDialogue(MobileObject* apTarget, PackageLocation* pPackLoc, PackageLocation* pPackSecondLoc,
	bool bHeadTrack, bool bMove, bool bflag, TESTopic* pTopic, bool abTargetContinueMoving,
	bool abTargetMoveTowardStarter) {
	PackageTarget* pnew;
	PackageLocation* ploc;
	DialoguePackage* ppack;
	MobileObject* ptalkactivator;
	if (GetCurrentProcessType() || apTarget->GetCurrentProcessType()) {
		return 0;
	}
	BaseProcess* CurrentAIProcess = pCurrentProcess;
	TESPackage* pTempPack = CurrentAIProcess->GetCurrentPackage();
	Conversation* pConversation = 0;
	if (!apTarget ||
		(apTarget->GetSitSleepState() &&
			(apTarget->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
				apTarget->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING))) {
		return 0;
	}
	MobileObject* pSpeakAs = this;
	ptalkactivator = 0;
	if (IsTalkingActivator()) {
		pSpeakAs = SetUpTalkingActivatorActor(apTarget, ptalkactivator);
	}
	if (apTarget != pPlayer) {
		Conversation* pConversation_2;
		Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
		if (apMemory) {
			pConversation_2 = ::new (apMemory) Conversation(pSpeakAs, apTarget, pTopic);
		} else {
			pConversation_2 = 0;
		}
		pConversation = pConversation_2;
	}
	StopCurrentDialogue();
	pCurrentProcess->ClearRunOncePackage();
	if (apTarget != pPlayer && !pConversation->FirstItem()) {
		return 0;
	}
	BaseProcess* v20 = pCurrentProcess;
	v20->SavePackageToExtraData(this);
	BaseProcess* v19 = pCurrentProcess;
	v19->ClearActionHeadTrackTarget(1);
	BaseProcess* v18 = pCurrentProcess;
	v18->SetDialogHeadTrackTarget(apTarget);
	BaseProcess* v17 = apTarget->pCurrentProcess;
	v17->ClearActionHeadTrackTarget(1);
	BaseProcess* v16 = apTarget->pCurrentProcess;
	v16->SetDialogHeadTrackTarget(this);
	DialoguePackage* v26 = static_cast<DialoguePackage*>(ourMalloc(sizeof(DialoguePackage)));
	if (v26) {
		ppack = ::new (v26) DialoguePackage(pConversation, static_cast<Actor*>(this), static_cast<Actor*>(apTarget),
			static_cast<Actor*>(ptalkactivator), 1, 1);
	} else {
		ppack = 0;
	}
	ppack->ptopic = pTopic;
	if (pTempPack) {
		ppack->CopyAllFlags(pTempPack);
	}
	ppack->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
	ppack->SetMustReachLocation(1);
	ppack->SetMustComplete(1);
	PackageLocation* v25 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
	if (v25) {
		ploc = ::new (v25) PackageLocation;
	} else {
		ploc = 0;
	}
	ploc->SetLocReference(apTarget);
	ppack->SetPackageLocation(ploc);
	if (ploc) {
		delete ploc;
	}
	PackageTarget* v24 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
	if (v24) {
		pnew = ::new (v24) PackageTarget;
	} else {
		pnew = 0;
	}
	ppack->SetPackageTarget(pnew);
	if (pnew) {
		delete pnew;
	}
	ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
	ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
	ppack->GetPackageTarget()->SetTargReference(apTarget);
	if (GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
		ppack->GetPackageTarget()->iValue = 200u;
	} else {
		ppack->GetPackageTarget()->iValue = 90u;
	}
	BaseProcess* v12 = pCurrentProcess;
	v12->ComputeLastTimeProcessed();
	PutCreatedPackage(ppack, 0, 1);
	if (bflag) {
		BaseProcess* v11 = pCurrentProcess;
		v11->AddToProcedureIndexRunning(this, 1);
	}
	return 1;
}

bool MobileObject::InitiateDialogue(TESPackage* pDialoguePackage) {
	BGSIdleCollection* pIdles;
	TESPackage* currentPackage;
	BaseProcess* v9;
	BaseProcess* v11;
	BaseProcess* v12;
	DialoguePackage* pNewPackage;
	BaseProcess* v15;
	BaseProcess* v16;
	BaseProcess* v17;
	BaseProcess* v18;
	Conversation* v19;
	void* v23;
	BaseProcess* currentAIProcess = MobileObject::GetCurrentAIProcess();
	currentAIProcess->GetCurrentPackage();
	Conversation* a2 = nullptr;
	Actor* pTarget = static_cast<Actor*>(pCurrentProcess->GetTarget());
	if (static_cast<const void*>(pTarget) != static_cast<const void*>(pPlayer)) {
		v23 = ourMalloc(sizeof(Conversation));
		if (v23) {
			v19 = ::new (v23) Conversation(this, pTarget, nullptr);
		} else {
			v19 = nullptr;
		}
		a2 = v19;
	}
	StopCurrentDialogue();
	pCurrentProcess->ClearRunOncePackage();
	if ((static_cast<const void*>(pTarget) != static_cast<const void*>(pPlayer)) && (!a2->FirstItem())) {
		return (bool)0;
	}
	if (!pDialoguePackage->GetNoHeadTrack()) {
		v18 = MobileObject::GetCurrentAIProcess();
		v18->ClearActionHeadTrackTarget((bool)1);
		v17 = MobileObject::GetCurrentAIProcess();
		v17->SetDialogHeadTrackTarget(pTarget);
		v16 = pTarget->MobileObject::GetCurrentAIProcess();
		v16->ClearActionHeadTrackTarget((bool)1);
		v15 = pTarget->MobileObject::GetCurrentAIProcess();
		v15->SetDialogHeadTrackTarget(this);
	}
	void* v22 = ourMalloc(sizeof(DialoguePackage));
	if (v22) {
		pIdles = pDialoguePackage->pIdleCollection;
		pNewPackage =
			::new (v22) DialoguePackage(a2, static_cast<Actor*>(this), pTarget, nullptr, 0, (uint8_t)(pIdles == 0));
	} else {
		pNewPackage = nullptr;
	}
	if (pDialoguePackage) {
		pNewPackage->CopyAllFlags(pDialoguePackage);
	}
	pNewPackage->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
	uint8_t mustReachLocation = (uint8_t)pDialoguePackage->GetMustReachLocation();
	pNewPackage->SetMustReachLocation(mustReachLocation);
	uint8_t mustComplete = (uint8_t)pDialoguePackage->GetMustComplete();
	pNewPackage->SetMustComplete(mustComplete);
	PackageLocation* location = pDialoguePackage->GetLocation();
	pNewPackage->SetPackageLocation(location);
	PackageLocation* packageSecondLocation = pDialoguePackage->GetPackageSecondLocation();
	pNewPackage->SetPackageSecondLocation(packageSecondLocation);
	PackageTarget* packageTarget = pDialoguePackage->GetPackageTarget();
	pNewPackage->SetPackageTarget(packageTarget);
	pNewPackage->SetProcedureType(PACKAGE_PROCEDURE_DIALOGUE);
	BaseProcess* v13 = MobileObject::GetCurrentAIProcess();
	v13->ComputeLastTimeProcessed();
	PutCreatedPackage(pNewPackage, (bool)0, (bool)1);
	SetDialogueItemTarget(pTarget);
	pCurrentProcess->SetTarget(pTarget);
	pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
	pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
	if ((static_cast<const void*>(pTarget) == static_cast<const void*>(pPlayer)) ||
		(static_cast<const void*>(pTarget) == static_cast<const void*>(this))) {
		if (!pTarget->IsDead((bool)0)) {
			v11 = MobileObject::GetCurrentAIProcess();
			v11->AddToProcedureIndexRunning(static_cast<Actor*>(this), 3u);
		}
	} else {
		v12 = pTarget->MobileObject::GetCurrentAIProcess();
		v12->SavePackageToExtraData(pTarget);
		currentPackage = GetCurrentPackage();
		pTarget->PutCreatedPackage(currentPackage, (bool)0, (bool)1);
		v9 = pTarget->MobileObject::GetCurrentAIProcess();
		v9->SetTarget(pTarget);
		pTarget->SetDialogueItemTarget(this);
	}
	return (bool)1;
}

// The exe's cleanup-only C++ EH frame is left out.
float MobileObject::SpeakSoundFunction(const char* apSoundFile, BSSoundHandle& aSoundHand,
	DIALOGUE_EMOTION aeEmotiontype, uint32_t aiEmotionValue, int32_t aiStringLength, TESIdleForm* apResponseSpeakerIdle,
	TESIdleForm* apResponseListenerIdle, TESObjectREFR* apTarget, bool abSetEmotion, bool ab2D, bool abQueue,
	bool abLip, bool abUseIdleManager) {
	FNVD_INLINED_LOCAL(BSSoundHandle, kVoiceHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, tempstring, std::nullopt, std::nullopt);
	if (IsTalkingActivator()) {
		float fdelay = 0.0f;
		::new (&tempstring) BSStringT<char>(apSoundFile);
		BaseProcess* CurrentAIProcess = this->pCurrentProcess;
		LipSynchAnim* panim = CurrentAIProcess->GetLipAnim();
		BSFaceGenAnimationData* panimdata = 0;
		this->SetSpeakingEmotion(aeEmotiontype);
		this->SetSpeakingEmotionValue(static_cast<int32_t>(aiEmotionValue));
		if (apSoundFile) {
			char Dst[516];
			BSstrcpy(Dst, 512, apSoundFile);
			this->StopCurrentDialogue();
			bool bisPlayer = this == pPlayer;
			float fSoundDuration = 0.0f;
			bool v50 = this == pPlayer || ab2D;
			double DistanceFromReference = this->GetDistanceFromReference(pPlayer, 0, 0);
			if (static_cast<double>(iSpeakSoundLipDistance.Int()) > DistanceFromReference &&
				(!bisPlayer || pPlayer->Is3rdPerson())) {
				panimdata = this->GetFaceAnimationData(0);
			}
			if (!panim) {
				BaseProcess* v48;
				if (!abLip || (v48 = this->pCurrentProcess, v48->GetLipFileFailed())) {
					BSFaceGenAnimationData* panimdata_2 = this->GetFaceAnimationData(0);
					if (panimdata_2) {
						panimdata_2->AddAnimationDataFromNote("BigAah 0.9 0.2 0.1 0.2");
					}
					BaseProcess* v44 = this->pCurrentProcess;
					if (v44->GetLipFileFailed()) {
						// The callback's context is the speaker's form ID.
						MobileObject::DisplaySubTitleCallback(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)),
							0);
						BaseProcess* v43 = this->pCurrentProcess;
						v43->SetLipFileFailed(0);
					}
				} else {
					if (LipSynchAnim::ChangeFilenameToLipSynch(tempstring)) {
						if (abQueue) {
							BaseProcess* v47 = this->pCurrentProcess;
							v47->SetWaitingforLipFile(1);
							BaseProcess* v46 = this->pCurrentProcess;
							v46->SetLipQuequed(1);
							ReferenceProcessLists.kLipSyncManager.BuildLipSynicFile(this, tempstring.pString);
							tempstring.~BSStringT<char>();
							return 0.0f;
						}
						panim = LipSynchAnim::LoadFromFile(tempstring);
						if (!panim) {
							DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
						}
						BaseProcess* v45 = this->pCurrentProcess;
						v45->SetWaitingforLipFile(0);
					}
				}
			}
			if (this->GetAnimation()) {
				if (apResponseSpeakerIdle || abUseIdleManager) {
					this->pCurrentProcess->SetupSpecialIdle(this, apResponseSpeakerIdle,
						apResponseSpeakerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO, 0, 1, 1);
				}
				if (apTarget) {
					if (apTarget != this && (apTarget->IsMobileObject() || apTarget->IsActor())) {
						Actor* ptargact = static_cast<Actor*>(apTarget);
						bool v62 = 1;
						if (apTarget->IsActor()) {
							TESPackage* ptargpack = ptargact->GetCurrentPackage();
							if (ptargpack) {
								v62 = ptargpack->pIdleCollection == 0;
							}
						}
						if (apResponseListenerIdle || v62) {
							BaseProcess* v42 = ptargact->pCurrentProcess;
							v42->SetupSpecialIdle(ptargact, apResponseListenerIdle,
								apResponseListenerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
								0, 1, 1);
						}
					}
				}
			}
			if (panim) {
				int64_t v41 = panim->iNumFrames;
				fSoundDuration = static_cast<double>(v41) / 30.0;
				panim->ApplyToNode(panimdata, 0.2f);
			}
			if (abSetEmotion) {
				if (this->pCurrentProcess) {
					BaseProcess* v40 = this->pCurrentProcess;
					v40->SetLastSpeakingEmotion(aeEmotiontype);
				}
			}
			uint32_t isoundFlags = BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_VOICE;
			TESObjectREFR* ptalkingactivator = this->GetTalkingActivator();
			if (ptalkingactivator) {
				if (ptalkingactivator->GetObjectReference()) {
					[[maybe_unused]] BGSTalkingActivator* ptalker =
						static_cast<BGSTalkingActivator*>(ptalkingactivator->GetObjectReference());
				}
			}
			if (v50) {
				isoundFlags |= BSGameSound::SOUND_FLAG_IS_2D;
			} else {
				isoundFlags |= BSGameSound::SOUND_FLAG_IS_3D;
			}
			BSAudio* v16 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByFilename = v16->GetSoundHandleByFilename(&out, Dst, isoundFlags, 0);
			aSoundHand.operator=(*SoundHandleByFilename);
			BSAudioManager* mgr = &BSAudioManager::QInstance();
			MobileObject* pact = this;
			if (apTarget) {
				if (apTarget->IsActor()) {
					pact = static_cast<MobileObject*>(apTarget);
				}
			}
			// The subtitle callback's context is the listener's (or the speaker's) form ID.
			mgr->SetPlayCallback(static_cast<uint32_t>(aSoundHand.iSoundID), &MobileObject::DisplaySubTitleCallback,
				reinterpret_cast<void*>(static_cast<uintptr_t>(pact->iFormID)));
			if (v50) {
				double v18 = fDBVoiceAttenuationIn2D.Float() * 100.0;
				aSoundHand.SetStaticAttenuation(static_cast<int32_t>(v18));
			} else {
				NiPoint3 afZ = this->GetLocationOnReference();
				// The exe reads the maximum first.
				float fMaxDist = BSAudio::GetMaxDialogDist();
				float fMinDist = BSAudio::GetMinDialogDist();
				aSoundHand.SetMinMax(fMinDist, fMaxDist);
				aSoundHand.SetPosition(afZ.x, afZ.y, afZ.z);
				NiNode* v17 = this->Get3D();
				aSoundHand.SetObjectToFollow(v17);
			}
			if (panim) {
				double StartingDelay = panim->GetStartingDelay();
				float fplaydelay = (BSFaceGenAnimationData::GetSpeechDelay() + StartingDelay) * 1000.0;
				aSoundHand.PlayAfter(static_cast<int32_t>(fplaydelay), 0);
				double v36 = panim->GetStartingDelay();
				fdelay = BSFaceGenAnimationData::GetSpeechDelay() + v36;
				if (!(fSoundDuration > 0.0)) {
					if (aSoundHand.GetDuration()) {
						int32_t Duration = aSoundHand.GetDuration();
						fdelay = static_cast<double>(Duration) / 1000.0 + fdelay;
					} else {
						double v34 = static_cast<double>(aiStringLength);
						float* v19 = &fAIDefaultSpeechMult.Float();
						fdelay = *v19 * v34 + fdelay;
					}
				} else {
					fdelay = fdelay + fSoundDuration;
				}
				BaseProcess* v33 = this->pCurrentProcess;
				int32_t v20 = static_cast<int32_t>(fplaydelay);
				uint32_t Time = appTimer.iLastTime;
				v33->SetCreatureLipSynchStartTime(v20 + Time);
				BaseProcess* v32 = this->pCurrentProcess;
				v32->SetCreatureLipSynchAnim(panim);
			} else {
				aSoundHand.PlayAfter(1u, 0);
				double v31 = static_cast<double>(aiStringLength);
				fdelay = fAIDefaultSpeechMult.Float() * v31;
			}
			if (this->bTalkingToPlayer) {
				this->bSpeakingDone = 0;
			}
			BaseProcess* v30 = this->pCurrentProcess;
			// The exe builds the by-value handle with operator= (0x418900).
			kVoiceHandle.operator=(aSoundHand);
			v30->SetSoundHandle(ACTOR_SOUND_HANDLE_VOICE, kVoiceHandle);
			if (panim) {
				BaseProcess* v28 = this->pCurrentProcess;
				LipSynchAnim* v22 = v28->GetCreatureLipSynchAnim();
				if (panim != v22) {
					if (panim) {
						delete panim;
					}
				}
				BaseProcess* v26 = this->pCurrentProcess;
				v26->SetLipAnim(0);
				BaseProcess* v25 = this->pCurrentProcess;
				v25->SetWaitingforLipFile(0);
			}
			tempstring.~BSStringT<char>();
			return fdelay;
		} else {
			tempstring.~BSStringT<char>();
			return 0.0f;
		}
	} else {
		return 0.0f;
	}
}

void MobileObject::EndDialogue() {
	if (pCurrentProcess) {
		TESPackage* v1;
		pCurrentProcess->SetSubtitleItem(0);
		if (pCurrentProcess->GetCurrentPackage() &&
			(v1 = pCurrentProcess->GetCurrentPackage(), v1->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
			TESPackage* v2 = pCurrentProcess->GetCurrentPackage();
			DialoguePackage* pDialoguePackage = dynamic_cast<DialoguePackage*>(v2);
			if (pDialoguePackage) {
				if (pDialoguePackage->handle.IsValid()) {
					pDialoguePackage->DeleteSoundHandle();
				}
				Actor* pSubject = static_cast<Actor*>(pDialoguePackage->GetActorStartedConversation());
				Actor* pTarget = static_cast<Actor*>(pDialoguePackage->pTargetActor);
				if (pSubject) {
					if (pSubject->IsActor()) {
						ReferenceProcessLists.kLipSyncManager.CancelLipFile(pSubject);
					}
					BaseProcess* CurrentAIProcess = pSubject->pCurrentProcess;
					CurrentAIProcess->ClearDialogHeadTrackTarget(1);
					pSubject->bVoiceFileDone = 1;
				}
				if (pTarget) {
					if (pTarget->IsActor()) {
						ReferenceProcessLists.kLipSyncManager.CancelLipFile(pTarget);
					}
					BaseProcess* v17 = pTarget->pCurrentProcess;
					v17->ClearDialogHeadTrackTarget(1);
					pTarget->bVoiceFileDone = 1;
				}
				if (pSubject) {
					if (pTarget) {
						BaseProcess* v16 = pSubject->pCurrentProcess;
						v16->SetDefaultHeadTrackTarget(pTarget);
						BaseProcess* v15 = pTarget->pCurrentProcess;
						v15->SetDefaultHeadTrackTarget(pSubject);
						pSubject->fUpdateTargetTimer = 1.0f;
						pTarget->fUpdateTargetTimer = 1.0f;
						BaseProcess* v14 = pSubject->pCurrentProcess;
						v14->SetLipFileFailed(0);
						BaseProcess* v13 = pTarget->pCurrentProcess;
						v13->SetLipFileFailed(0);
					}
				}
				Animation* v24 = pSubject->GetAnimation();
				if (v24) {
					v24->SpecialIdleFree(1, 0);
				}
				Animation* v25 = pTarget->GetAnimation();
				if (v25) {
					v25->SpecialIdleFree(1, 0);
				}
				if (pSubject) {
					if (pSubject->pCurrentProcess) {
						BaseProcess* v12 = pSubject->pCurrentProcess;
						v12->SetLastSpeakingEmotion(DE_NEUTRAL);
					}
				}
				if (pTarget) {
					if (pTarget->pCurrentProcess) {
						BaseProcess* v11 = pTarget->pCurrentProcess;
						v11->SetLastSpeakingEmotion(DE_NEUTRAL);
					}
				}
				pSubject->GetCurrentPackage();
				pTarget->GetCurrentPackage();
				BaseProcess* v10 = pSubject->pCurrentProcess;
				v10->LoadPackageFromExtraData(pSubject);
				if (pSubject->IsActor()) {
					BaseProcess* v9 = pSubject->pCurrentProcess;
					if (!v9->GetCurrentPackage() && !pDataHandler->GetClearingData()) {
						BaseProcess* v8 = pSubject->pCurrentProcess;
						v8->CheckforNewPackage(pSubject, 1);
					}
				}
				if (pTarget) {
					if (pTarget->IsinDialogue()) {
						if (pTarget->pCurrentProcess) {
							BaseProcess* v7 = pTarget->pCurrentProcess;
							v7->LoadPackageFromExtraData(pTarget);
							if (!pDataHandler->GetClearingData() && pTarget->IsActor()) {
								BaseProcess* v6 = pTarget->pCurrentProcess;
								v6->CheckforNewPackage(pTarget, 1);
							}
						}
					}
				}
				MobileObject* TalkingActivator = pDialoguePackage->pTalkingActivator;
				if (TalkingActivator) {
					(&TalkingActivator->GetTalkingActivator()->m_Extra)->RemoveTalkingActorExtra();
					(&TalkingActivator->GetTalkingActivator()->m_Extra)->RemoveTalkingActorExtra();
					ReferenceProcessLists.RemoveReference(TalkingActivator, PROCESS_TYPE_HIGH);
					ReferenceProcessLists.AddActorToTempChangeList(TalkingActivator);
				}
				delete pDialoguePackage;
			}
		} else {
			BaseProcess* v5 = pCurrentProcess;
			TESObjectREFR* v20 = v5->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_DIALOG);
			BaseProcess* v4 = pCurrentProcess;
			v4->ClearDialogHeadTrackTarget(1);
			BaseProcess* v3 = pCurrentProcess;
			v3->SetDefaultHeadTrackTarget(v20);
			fUpdateTargetTimer = 1.0f;
		}
	}
}

// A mobile object has no editor location of its own: zero.
NiPoint3 MobileObject::GetEditorLocationCoord() {
	return NiPoint3_ZERO;
}

bool MobileObject::DetachCharController() {
	auto* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		pController->RemoveCollisionDebug();
		pController->SetWorld(nullptr);
		return true;
	}
	return false;
}

bool MobileObject::RemoveCharController() {
	bool bres = DetachCharController();
	if (this->pCurrentProcess) {
		// The exe builds the null NiPointer argument in its outgoing slot (0x633C90, NiPointer's constructor).
		this->pCurrentProcess
			->SetCharController(NiPointer<bhkCharacterController>(static_cast<bhkCharacterController*>(0)));
	}
	return bres;
}

void MobileObject::SetPosition(const NiPoint3& arPos) {
	SetLocationOnReference(arPos);
	auto* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		pController->SetPosition(arPos);
		pController->SetLinearVelocity(hkVector4::getZero());
		// The exe calls both getters and discards their results (fstp st(0) after the first).
		pController->QFallStartHeight();
		pController->QHitDamage();
		pController->ClearManifold();
	}
	// The exe calls NiPointer::operator-> (0x559450) out of line for both reads.
	if (pLoadedData && pLoadedData->m_spData3D.operator->()) {
		pLoadedData->m_spData3D.operator->()->SetTranslate(arPos);
	}
}

void MobileObject::GetLinearVelocity(NiPoint3& arVel) {
	bhkCharacterController* CharController = GetCharController();
	if (CharController) {
		CharController->GetLinearVelocity(arVel);
	} else {
		arVel = NiPoint3_ZERO;
	}
}

float MobileObject::GetHeading(bool abIgnoreImmobile) {
	return TESObjectREFR::GetAngleOnReference().z;
}

void MobileObject::SetHeading(float afAngle) {
	if (pCurrentProcess) {
		if (IsKnockedOut()) {
			return;
		}
		switch (GetSitSleepState()) {
		case SIT_SLEEP_IS_SITTING:
		case SIT_SLEEP_WANT_TO_STAND:
		case SIT_SLEEP_IS_SLEEPING:
		case SIT_SLEEP_WANT_TO_WAKE:
			if (pCurrentProcess->GetCurrentFurniture() && IsActor()) {
				return;
			}
			break;
		default:
			break;
		}
	}
	afAngle = ClampAngle(afAngle);
	// The exe calls Actor::GetTurretBehavior directly once IsActor holds.
	if (IsActor() && static_cast<Actor*>(this)->GetTurretBehavior()) {
		// The angle is rounded to float after each step; the bounds are the qword constants at 0x101FF58,
		// 0x101FF48 and 0x101FF40.
		afAngle = afAngle - data.kRotation.z;
		while (afAngle < -3.1415927410125732) {
			afAngle = afAngle + 6.2831854820251465;
		}
		while (afAngle >= 3.1415927410125732) {
			afAngle = afAngle - 6.2831854820251465;
		}
		SetAngleOnReferenceY(afAngle);
	} else {
		SetAngleOnReferenceZ(afAngle);
	}
}

void MobileObject::SetHeading(TESObjectREFR* apTargetReference) {
	const NiPoint3* v3 = &GetLocationOnReference();
	const NiPoint3* v2 = &apTargetReference->GetLocationOnReference();
	NiPoint3 movedir = *v2 - *v3;
	float ftargetangle = GetZAngleFromVector(movedir);
	SetHeading(ftargetangle);
}

TESObjectREFR* MobileObject::GetCurrentTarget() {
	return pCurrentProcess->GetTarget();
}

bool MobileObject::IsAtPoint(const NiPoint3& aPoint, float afRadius, bool abExpandRadius, bool abAlwaysTestHeight) {
	NiPoint3 v6;
	NiPoint3 v7 = GetLocationOnReference();
	(v6 = (&v7)->operator-(aPoint));
	float v10 = 0x1p+7f;
	float v9 = (float)(v7.z - 30.0);
	float v8 = (float)((v7.z + ((float)0x1p+7f)) + 30.0);
	if ((v9 < ((double)(&aPoint)->z)) && (v8 > ((double)(&aPoint)->z))) {
		v6.z = (float)0.0;
	}
	if (abExpandRadius) {
		afRadius = (float)(afRadius + ((float)0x1p+5f));
	}
	return (bool)((uint8_t)(NiPoint3::IsMagnitudeLongerThanRadius(&v6, afRadius) <= 0));
}

void MobileObject::SetSize(float afRadius) {
	auto* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		pController->SetSize(afRadius);
	}
}

void MobileObject::SetDisplayGeometry(bool abTF) {
	bhkCharacterController* pCharacterController = GetCharController();
	if (pCharacterController) {
		if (abTF) {
			pCharacterController->BuildDisplayGeometry(0);
			NiAVObject* NiObject_ = pCharacterController->GetNiObject();
			BSShaderUtil::RecursiveSetPropertyWireframe(NiObject_, 1);
		} else {
			pCharacterController->RemoveCollisionDebug();
		}
	}
}

void MobileObject::PutCreatedPackage(TESPackage* apPack, bool abTempPack, bool abIsaCreatedPackage) {
	pCurrentProcess->SetCurrentPackage(apPack, this);
	pCurrentProcess->SetupNewPackage();
}

MobileObject* MobileObject::SetUpTalkingActivatorActor(MobileObject* apTarget, MobileObject*& aptalkingactivator) {
	aptalkingactivator = this;
	BGSTalkingActivator* pactitalk = static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference());
	if (pactitalk->GetTempRef()) {
		return pactitalk->GetTempRef();
	} else {
		return this;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void MobileObject::AddSayOnceTopicInfo(TESTopicInfo* apTopicInfo) {
	SayOnceTopicInfos* v2;
	uint32_t aiDate = iGameYear << 13 | (LowProcess::cCurrentGameDay | LowProcess::eCurrentGameMonth << 9);
	SayOnceTopicInfos* apMemory = static_cast<SayOnceTopicInfos*>(ourMalloc(sizeof(SayOnceTopicInfos)));
	if (apMemory) {
		v2 = ::new (apMemory) SayOnceTopicInfos(apTopicInfo, aiDate, fGameHour);
	} else {
		v2 = 0;
	}
	MobileObject* TalkingActivator = this;
	if (IsTalkingActivator()) {
		TalkingActivator = static_cast<MobileObject*>(GetTalkingActivator());
	}
	(&TalkingActivator->m_Extra)->SetExtraSayTopicInfoOnceADayExtra(v2);
	TalkingActivator->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

// The exe's cleanup-only C++ EH frame is left out.
void MobileObject::StopCurrentDialogue() {
	char v6 = 0;
	BaseProcess* pproc = pCurrentProcess;
	bool v3 = false;
	if (pproc) {
		// The exe builds the returned handle in th_s; its destructor is inlined to nothing.
		BSSoundHandle th_s = pproc->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
		v6 = 1;
		if (th_s.IsValid()) {
			v3 = true;
		}
	}
	if ((v6 & 1) != 0) {
	}
	if (v3) {
		TESTopicInfo* pinfo = GetSayToTopicInfo();
		TESTopic* ptopic = GetSayToTopic();
		SetSayToTopic(0);
		(&m_Extra)->RemoveSayToInfoExtra();
		if (pinfo) {
			if (!pinfo->IsRunImmediately()) {
				pinfo->RunResult(TIRS_END, this);
			}
			ExtraDataList* Extra = &m_Extra;
			Script::SetActionFlag(ptopic, Extra, 0x40000u);
			bDoNotRunSayToCallBack = 1;
			BaseProcess* pproc_2 = pCurrentProcess;
			pproc_2->SetDoingSayTo(0);
		}
		cSoundCallBackSet = -1;
		pproc->StopSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
		pproc->SetGreetingFlag(0);
		pPlayer->ResetPlayerGreetFlag();
	}
	bVoiceFileDone = 1;
	bSoundFileDone = 1;
	if (!IsTalkingActivator()) {
		if (Get3D()) {
			Actor* pactor = static_cast<Actor*>(this);
			static_cast<Actor*>(this)->Actor::ResetFaceData();
		}
	}
}

BaseProcess* MobileObject::GetCurrentAIProcess() {
	return pCurrentProcess;
}

PROCESS_TYPE MobileObject::GetCurrentProcessType() {
	if (pCurrentProcess) {
		return pCurrentProcess->GetProcessLevel();
	}
	return PROCESS_TYPE_NONE;
}

void MobileObject::SetPackageStartLocationFromCurrentLocation() {
	float fZAngle = GetAngleOnReference().z;
	const NiPoint3* pPosition = &static_cast<TESObjectREFR*>(this)->GetLocationOnReference();
	TESObjectCELL* pCell = GetParentCell();
	TESWorldSpace* pWorld = GetWorldSpace();
	SetPackageStartLocation(pWorld, pCell, *pPosition, fZAngle);
}

TESPackage* MobileObject::GetCurrentPackage() {
	if (pCurrentProcess) {
		return pCurrentProcess->GetPackageThatIsRunning();
	}
	return nullptr;
}

bhkCharacterController* MobileObject::GetCharController() {
	if (pCurrentProcess) {
		return pCurrentProcess->GetCharController();
	}
	return nullptr;
}

// The exe calls GetCurrentState twice: once for the null test and once for the state it asks.
bool MobileObject::IsInAir() {
	bool bInAir = false;
	bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (!pController || !pController->GetCurrentState())
		return bInAir;
	hkpCharacterState* pState = pController->GetCurrentState();
	if (pState->getType() == HK_CHARACTER_IN_AIR)
		bInAir = true;
	return bInAir;
}

// The exe's cleanup-only C++ EH frame is left out.
BSSoundHandle* MobileObject::PlaySoundByEditorName(BSSoundHandle* apResult, const char* apEditorName, bool abLoop,
	uint32_t aiSoundType, bool abAllowCopy) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	::new (&soundHand) BSSoundHandle;
	if (Get3D()) {
		TESSound* psnd = pDataHandler->GetSound(apEditorName);
		if (psnd) {
			uint32_t a3 = psnd->iFormID;
			BSAudio* v6 = &BSAudio::QInstance();
			BSSoundHandle* arHandle = v6->GetSoundHandleByNumericID(&a1, a3, aiSoundType);
			soundHand.operator=(*arHandle);
			if ((aiSoundType & 2) != 0) {
				NiPoint3 v13 = GetLocationOnReference();
				soundHand.SetPosition(v13.x, v13.y, v13.z);
				NiNode* v7 = Get3D();
				soundHand.SetObjectToFollow(v7);
			}
			soundHand.SetVolume(1.0f);
			soundHand.Play(abLoop);
		}
	}
	apResult->operator=(soundHand);
	return apResult;
}

uint32_t* MobileObject::GetCollisionFilter(uint32_t* apResult) {
	CFilter kControllerFilter;
	CFilter kEmptyFilter;
	const uint32_t* pFilter;
	auto* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		pFilter = &pController->GetCollisionFilter(&kControllerFilter)->iFilter;
	} else {
		pFilter = &FNVD_RECONSTRUCT(CFilter)(&kEmptyFilter, 0)->iFilter;
	}
	*apResult = *pFilter;
	return apResult;
}

bool MobileObject::GetRunsInLow() {
	return (bool)((uint8_t)bRunsInLow);
}

TESObjectREFR* MobileObject::GetTalkingActivator() {
	return pTalkingActivator;
}

void MobileObject::SetTalkingActivatorRef(TESObjectREFR* apRef) {
	pTalkingActivator = apRef;
}

bool MobileObject::IsTalkingActivator() {
	return bTalkingActivator;
}

bool MobileObject::IsTalking() {
	return (bool)((uint8_t)pCurrentProcess->IsTalking(static_cast<Actor*>(this)));
}

void MobileObject::InitLastTimeProcessed() {
	if (pCurrentProcess) {
		pCurrentProcess->ComputeLastTimeProcessed();
	}
}

void MobileObject::SetSoundCallBackSuccess(bool abSuccess) {
	bSoundCallBackSuccess = abSuccess;
}

void MobileObject::UpdateTopicInfoSayOnce() {
	ExtraSayTopicInfoOnceADay* pExtraData = nullptr;
	if (IsTalkingActivator()) {
		pExtraData = GetTalkingActivator()->GetExtra()->GetExtraSayTopicInfoOnceADayExtra();
	} else {
		pExtraData = GetExtra()->GetExtraSayTopicInfoOnceADayExtra();
	}
	if (!pExtraData) {
		return;
	}
	BSSimpleList<SayOnceTopicInfos*>* pIter = pExtraData->pListofSaidOnceTopicInfos;
	// The exe calls GetItem (0x6815C0) for the test and again for the item.
	while (pIter && pIter->GetItem()) {
		SayOnceTopicInfos* pTopicInfoData = pIter->GetItem();
		bool bRemove = false;
		float fHour = pTopicInfoData->fHourStamp + fSayOncePerDayInfoTimer.Float();
		int32_t iDay = pTopicInfoData->iDateStamp & 0x1FF;
		int32_t iMonth = (pTopicInfoData->iDateStamp & 0x1E00) >> 9;
		int32_t iYear = (pTopicInfoData->iDateStamp & 0x7FFE000) >> 13;
		int32_t iCurrMonth = LowProcess::eCurrentGameMonth;
		int32_t iCurrDay = LowProcess::cCurrentGameDay;
		// The asm compares the year signed (jle) though iGameYear is unsigned.
		int32_t iCurrYear = static_cast<int32_t>(iGameYear);
		float fCurrHour = fGameHour;
		// 23.0 is the qword at 0x1035838; NaN hours take the "not past 23" paths, as the exe's test ah,0x41 does.
		if (!(fHour > 23.0) && iCurrDay <= iDay && iCurrMonth <= iMonth && iCurrYear <= iYear) {
			if (fHour <= fCurrHour) {
				bRemove = true;
			}
		} else if (fHour > 23.0) {
			fHour = fHour - 23.0;
			if (fHour <= fCurrHour && (iCurrDay > iDay || iCurrMonth > iMonth || iCurrYear > iYear)) {
				bRemove = true;
			}
		} else if (fHour >= fCurrHour) {
			bRemove = true;
		}
		if (bRemove) {
			pIter->Remove(pTopicInfoData);
			ourFree(pTopicInfoData);
			pIter = pExtraData->pListofSaidOnceTopicInfos;
		} else {
			pIter = pIter->GetNext();
		}
	}
}

float MobileObject::GetLooking() {
	return GetAngleOnReference().x;
}

void MobileObject::ModHeading(float afModAngle) {
	float lfHeading = GetHeading(false) + afModAngle;
	SetHeading(lfHeading);
}

void MobileObject::ModLooking(float afModAngle) {
	float v2 = GetLooking() + afModAngle;
	SetLooking(v2);
}

void MobileObject::DeleteCurrentProcess() {
	BaseProcess* lpProcess = pCurrentProcess;
	if (lpProcess) {
		delete lpProcess;
	}
	pCurrentProcess = nullptr;
}

void MobileObject::SetChaseBip(bool abTF) {
	auto* pController = static_cast<bhkCharacterController*>(GetCharController());
	if (pController) {
		pController->SetFlag(bhkCharacterController::CHASEBIP, abTF);
	}
}

PROCESS_TYPE MobileObject::GetDesiredProcessLevel() {
	if (!pSaveLoadGameOLD->GetLoading() && pPlayer->IsSleepingorResting()) {
		return GetCurrentProcessType();
	}
	TESObjectCELL* pCell = GetParentCell();
	if (pSaveLoadGame->GetSaveGameLoading() || Get3D_Body() || pModelLoader->IsReferenceQueued(this)) {
		if (TES::pInstance->IsCellLoaded(pCell, true)) {
			return PROCESS_TYPE_HIGH;
		}
		if (TES::pInstance->IsCellLoaded(pCell, false)) {
			return PROCESS_TYPE_MIDDLE_HIGH;
		}
	}
	if ((pCell && pCell->GetLowestProcessMiddleLow()) || TES::pInstance->IsCellLoaded(pCell, false)) {
		return PROCESS_TYPE_MIDDLE_LOW;
	}
	return PROCESS_TYPE_LOW;
}

bool MobileObject::IsInDialoguewithPlayer() {
	return Interface::InDialog() && pCurrentProcess && pCurrentProcess->GetTalkingtoPC();
}

void MobileObject::HandleTextureUpgrade(bool abForceRecheck) {
	NiNode* pNode = static_cast<TESObjectREFR*>(this)->Get3D();
	if (pNode && (!pNode->GetHighDetail() || abForceRecheck)) {
		if (CheckHighDetail()) {
			pModelLoader->UpgradeTextures(pNode);
		}
	}
}

uint32_t MobileObject::CheckHighDetail() {
	// 1 when the reference is within fAIMaxHeadTrackDistanceFromPC of the player.
	float fDistance = GetDistanceFromPoint(pPlayer->GetLocationOnReference());
	return fAIMaxHeadTrackDistanceFromPC.Float() >= fDistance;
}

MobileObject* MobileObject::GetDialogueItemTarget() {
	return pDialogueItemTarget;
}

void MobileObject::SetDialogueItemTarget(MobileObject* pref) {
	pDialogueItemTarget = pref;
}

void MobileObject::SetLooking(float afAngle) {
	float fMaxLookDown = 1.5533430576324463f;
	if (IsActor()) {
		// The asm compares the state signed (jle) though SIT_SLEEP_STATE is unsigned.
		const int32_t iSitSleepState = static_cast<int32_t>(GetSitSleepState());
		if (iSitSleepState > static_cast<int32_t>(SIT_SLEEP_NORMAL) &&
			iSitSleepState <= static_cast<int32_t>(SIT_SLEEP_WANT_TO_STAND)) {
			fMaxLookDown = fSittingMaxLookingDown.Float() * 0.01745329238474369;
		}
	}
	// The lower limit is the qword -1.5533430576324463 at 0x108A7F0 for the compare and the dword at 0x108A7EC for
	// the store; both compares fall through only on a strict "less than" (test ah,5 / jp).
	if (afAngle < -1.5533430576324463) {
		SetAngleOnReferenceX(-1.5533430576324463f);
	} else if (fMaxLookDown < afAngle) {
		SetAngleOnReferenceX(fMaxLookDown);
	} else {
		SetAngleOnReferenceX(afAngle);
	}
}

void MobileObject::CleanUpOtherMobileObjects(bool abMagicCaster) {
	ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
	uint32_t uiIndex = 0;
	for (uint32_t i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); uiIndex < i;
		i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
		MobileObject* pprojectile__pexplosion__pactor = pMobArray->GetItem(uiIndex);
		if (pprojectile__pexplosion__pactor != this) {
			if (pprojectile__pexplosion__pactor->IsActor()) {
				BaseProcess* pBaseProcess = pprojectile__pexplosion__pactor->pCurrentProcess;
				pBaseProcess->RemoveHeadTrackTarget(this);
				if (abMagicCaster) {
					MagicCaster* v3 = GetMagicCaster();
					static_cast<Actor*>(pprojectile__pexplosion__pactor)->Actor::CheckDeletedCaster(v3);
				}
				BaseProcess* CurrentAIProcess = pprojectile__pexplosion__pactor->pCurrentProcess;
				MagicTarget* v4 = CurrentAIProcess->GetDesiredTarget();
				if (v4 == GetMagicTarget()) {
					BaseProcess* v16 = pprojectile__pexplosion__pactor->pCurrentProcess;
					v16->SetDesiredTarget(0);
				}
			} else {
				if (pprojectile__pexplosion__pactor->IsProjectile()) {
					static_cast<Projectile*>(pprojectile__pexplosion__pactor)->Projectile::CheckRefDeleted(this);
				} else {
					if (pprojectile__pexplosion__pactor->IsExplosion()) {
						static_cast<Explosion*>(pprojectile__pexplosion__pactor)->Explosion::CheckRefDeleted(this);
					} else {
						if (abMagicCaster) {
							MagicProjectile* pprojectile =
								dynamic_cast<MagicProjectile*>(pprojectile__pexplosion__pactor);
							if (pprojectile) {
								MagicCaster* v5 = GetMagicCaster();
								pprojectile->CheckDeletedCaster(v5);
							}
						}
					}
				}
			}
		}
		++uiIndex;
	}
	for (uint32_t i_2 = pMobArray->GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
		i_2 < pMobArray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH); ++i_2) {
		MobileObject* pmob = pMobArray->GetItem(i_2);
		if (pmob != this) {
			Actor* pactor = dynamic_cast<Actor*>(pmob);
			if (pactor) {
				if (abMagicCaster) {
					MagicCaster* v6 = GetMagicCaster();
					pactor->CheckDeletedCaster(v6);
				}
				BaseProcess* v15 = pactor->pCurrentProcess;
				MagicTarget* v7 = v15->GetDesiredTarget();
				if (v7 == GetMagicTarget()) {
					BaseProcess* v14 = pactor->pCurrentProcess;
					v14->SetDesiredTarget(0);
				}
			} else {
				if (abMagicCaster) {
					MagicProjectile* pprojectile_2 = dynamic_cast<MagicProjectile*>(pmob);
					if (pprojectile_2) {
						MagicCaster* v8 = GetMagicCaster();
						pprojectile_2->CheckDeletedCaster(v8);
					}
				}
			}
		}
	}
	if (pPlayer) {
		if (pPlayer->pCurrentProcess) {
			BaseProcess* v13 = pPlayer->pCurrentProcess;
			v13->RemoveHeadTrackTarget(this);
		}
		if (abMagicCaster) {
			MagicCaster* v9 = GetMagicCaster();
			pPlayer->CheckDeletedCaster(v9);
		}
		if (pPlayer->pCurrentProcess) {
			BaseProcess* v12 = pPlayer->pCurrentProcess;
			MagicTarget* v10 = v12->GetDesiredTarget();
			if (v10 == GetMagicTarget()) {
				BaseProcess* v11 = pPlayer->pCurrentProcess;
				v11->SetDesiredTarget(0);
			}
		}
		if (pPlayer) {
			if (pPlayer->GetAutoAimActorSimple() == this) {
				pPlayer->ClearAutoAim();
			}
		}
	}
}

void MobileObject::SetSoundCallBack(char acCallBack) {
	cSoundCallBackSet = acCallBack;
}

bool MobileObject::GetShouldUseEmotionAnimation() {
	return (bool)((uint8_t)bUseEmotion);
}

void MobileObject::SetTalkingActivator(bool abFlag) {
	bTalkingActivator = (bool)((uint8_t)abFlag);
}

void MobileObject::SetShouldUseEmotionAnimation(bool abFlag) {
	bUseEmotion = (bool)((uint8_t)abFlag);
}

bool MobileObject::IsinDialogue() {
	TESPackage* currentPackage = GetCurrentPackage();
	return (bool)((uint8_t)(currentPackage && (((int32_t)currentPackage->GetPackType()) == 28)));
}

// The exe's cleanup-only C++ EH frame is left out.
bool MobileObject::SetProcessLevel(PROCESS_TYPE aeLevel) {
	bool v7 = 0;
	if (aeLevel == PROCESS_TYPE_NONE) {
		ReferenceProcessLists.kUpdateLock.Lock(0);
		if (pCurrentProcess) {
			v7 = 1;
			PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
			ReferenceProcessLists.RemoveReference(this, ProcessLevel);
			if (pCurrentProcess) {
				delete pCurrentProcess;
			}
			pCurrentProcess = 0;
		}
		ReferenceProcessLists.kUpdateLock.Unlock();
	} else {
		if (!pCurrentProcess) {
			LowProcess* v4;
			LowProcess* apMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
			if (apMemory) {
				v4 = ::new (apMemory) LowProcess;
			} else {
				v4 = 0;
			}
			pCurrentProcess = v4;
		}
		if (GetCurrentProcessType() != aeLevel) {
			switch (aeLevel) {
			case PROCESS_TYPE_HIGH: {
				v7 = MovetoHigh();
				break;
			}
			case PROCESS_TYPE_MIDDLE_HIGH: {
				v7 = MovetoMiddleHigh();
				break;
			}
			case PROCESS_TYPE_MIDDLE_LOW: {
				v7 = MovetoMiddleLow();
				break;
			}
			case PROCESS_TYPE_LOW: {
				v7 = MovetoLow();
				break;
			}
			default: {
				return v7;
			}
			}
		}
	}
	return v7;
}

// The handle GetSoundHandleByNumericID fills is a temporary the exe constructs in place (no constructor call), then
// assigns and destroys; the result is assigned into the caller's memory. abAllowCopy is unused. The exe's
// cleanup-only EH frame is left out.
BSSoundHandle* MobileObject::PlaySound(BSSoundHandle* apResult, uint32_t aiSoundFormID, bool abLoop,
	uint32_t aiSoundType, bool abAllowCopy) {
	BSSoundHandle kSound;
	// Raw storage: the callee constructs the handle there.
	alignas(BSSoundHandle) uint8_t aucTemp[sizeof(BSSoundHandle)];
	BSSoundHandle* pTemp = reinterpret_cast<BSSoundHandle*>(aucTemp);
	BSAudio* pAudio = &BSAudio::QInstance();
	BSSoundHandle* pFound = pAudio->GetSoundHandleByNumericID(pTemp, aiSoundFormID, aiSoundType);
	kSound = *pFound;
	pTemp->~BSSoundHandle();
	if (TESObjectREFR::Get3D() && (aiSoundType & 2)) {
		NiPoint3 kPosition = GetLocationOnReference();
		kSound.SetPosition(kPosition.x, kPosition.y, kPosition.z);
		kSound.SetObjectToFollow(TESObjectREFR::Get3D());
	}
	kSound.Play(abLoop);
	*apResult = kSound;
	return apResult;
}

// The exe's cleanup-only C++ EH frame is left out.
BSSoundHandle* MobileObject::PlaySound_0(BSSoundHandle* apResult, char* apSoundFileName, bool abLoop,
	uint32_t aiSoundType) {
	FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	::new (&soundHand) BSSoundHandle;
	BSAudio* v5 = &BSAudio::QInstance();
	BSSoundHandle* arHandle = v5->GetSoundHandleByFilename(&out, apSoundFileName, aiSoundType, 0);
	soundHand.operator=(*arHandle);
	if (Get3D()) {
		if ((aiSoundType & 2) != 0) {
			NiPoint3 v11 = GetLocationOnReference();
			soundHand.SetPosition(v11.x, v11.y, v11.z);
			NiNode* v6 = Get3D();
			soundHand.SetObjectToFollow(v6);
		}
	}
	soundHand.Play(abLoop);
	apResult->operator=(soundHand);
	return apResult;
}

float MobileObject::GetActualHeading() {
	float fheading = GetHeading(0);
	if (IsActor() && static_cast<Actor*>(this)->Actor::GetTurretBehavior()) {
		NiAVObject* p3d = Get3D();
		if (p3d) {
			NiAVObject* pyawBone = p3d->GetObjectByName(FixedStrings::QBip01Yaw());
			if (pyawBone) {
				bhkBlendCollisionObject* pblendCol =
					static_cast<bhkBlendCollisionObject*>(NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI,
						pyawBone->GetCollisionObject()));
				if (pblendCol) {
					BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &pblendCol->GetRigidBody()->kActionConstrants;
					while (pactCon && pactCon->m_item) {
						bhkOrientHingedBodyAction* porientAction = static_cast<
							bhkOrientHingedBodyAction*>(NiObject::DynamicCastStatic(&bhkOrientHingedBodyAction::ms_RTTI,
							pactCon->m_item));
						pactCon = pactCon->m_pkNext;
						if (porientAction) {
							// The action's Havok object (the shared getter 0x620B80) is its hkOrientHingedBodyAction.
							hkOrientHingedBodyAction* phkAction =
								static_cast<hkOrientHingedBodyAction*>(porientAction->GethkObject());
							fheading = fheading - phkAction->fAngleDiff;
							return ClampAngle(fheading);
						}
					}
				}
			}
		}
	}
	return fheading;
}

void MobileObject::SayToCallBack(void* apContext, bool abSucceeded) {
	// The context is the speaker's form ID.
	MobileObject* pactor = static_cast<
		MobileObject*>(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext))));
	if (pactor) {
		if (pactor->IsMobileObject()) {
			if (pactor->bDoNotRunSayToCallBack) {
				pactor->bDoNotRunSayToCallBack = 0;
			} else {
				pactor->SetSoundCallBack(0);
				pactor->SetSoundCallBackSuccess(abSucceeded);
			}
		}
	}
}

// The exe tests the cast pointer for null again (0x935F3E) after it called IsMobileObject through it.
void MobileObject::DisplaySubTitleCallback(void* apContext, int32_t aiDuration) {
	TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)));
	if (pForm && pForm->IsMobileObject()) {
		MobileObject* pMobile = static_cast<MobileObject*>(pForm);
		if (pMobile)
			pMobile->SetSoundCallBack(3);
	}
}

void MobileObject::ChangeDialogueAnimation(void* apContext, bool abSucceeded) {
	// EXE-CAST: void* as uint32_t (the callback's context holds the actor's form ID)
	MobileObject* pactor = static_cast<
		MobileObject*>(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext))));
	if (pactor) {
		if (pactor->IsMobileObject()) {
			if (pTalkingToPlayer) {
				pTalkingToPlayer->bTalkingToPlayer = 0;
				pTalkingToPlayer->bSpeakingDone = 1;
				if (pactor->IsTalkingActivator()) {
					TESObjectREFR* pref = pactor->GetTalkingActivator();
					NiNode* v2 = pref->Get3D();
					NiControllerManager* pmanager =
						static_cast<NiControllerManager*>(v2->GetController(&NiControllerManager::ms_RTTI));
					if (pmanager) {
						NiFixedString* v3 = &FixedStrings::QTalking();
						NiControllerSequence* pseq = pmanager->GetSequenceByName(*v3);
						if (pseq) {
							Animation::SetLipTime(0.0f, pmanager, pseq);
							pmanager->DeactivateSequence(pseq, 0.0f);
						}
					}
					BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
					CurrentAIProcess->SetCreatureLipSynchAnim(0);
					BaseProcess* v4 = pactor->pCurrentProcess;
					v4->SetCreatureLipSynchStartTime(0);
				}
			}
			if (!abSucceeded) {
				DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
			}
		}
	}
	pTalkingToPlayer = 0;
}

void MobileObject::DialogueConversationCallBack(void* apContext, bool abSucceeded) {
	MobileObject* pactor = static_cast<
		MobileObject*>(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext))));
	if (pactor) {
		if (pactor->IsMobileObject()) {
			pactor->SetSoundCallBack(1);
			pactor->SetSoundCallBackSuccess(abSucceeded);
		}
	}
}

void MobileObject::SoundFileCallBack(void* apContext, bool abSucceeded) {
	MobileObject* pactor = static_cast<
		MobileObject*>(TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext))));
	if (pactor) {
		if (pactor->IsMobileObject()) {
			pactor->bSoundFileDone = 1;
			if (pactor->bTalkingToPlayer) {
				pactor->bSpeakingDone = 1;
			}
			TESPackage* ppack = pactor->GetCurrentPackage();
			DialoguePackage* pdial = 0;
			if (ppack) {
				if (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
					pdial = static_cast<DialoguePackage*>(ppack);
				}
			}
			if (pdial) {
				if (pactor->bSoundFileDone) {
					pdial->flistentimer = 0.3f;
				} else {
					pdial->flistentimer = 20.0f;
				}
			}
		}
	}
}

SayOnceTopicInfos::SayOnceTopicInfos() : pInfo(0), iDateStamp(-1), fHourStamp(-1.0f) {}

SayOnceTopicInfos::SayOnceTopicInfos(TESTopicInfo* apInfo, int32_t aiDate, float afTime)
	: pInfo(apInfo), iDateStamp(aiDate), fHourStamp(afTime) {}

bool TESMagicCasterForm::CanBeMagicCaster(TESForm* apForm) {
	return (dynamic_cast<TESMagicCasterForm*>(apForm) != 0);
}

DialogueItem* DialoguePackage::GetCurrentDialogItem() {
	return pCurrentItem;
}

NiFixedString& FixedStrings::QTalking() {
	return *FixedStrings::pTalking;
}
