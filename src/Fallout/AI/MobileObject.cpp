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
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
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
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"

#include <cstdint>
#include <xmmintrin.h>

NiPoint3 MobileObject::boundMin;
NiPoint3 MobileObject::boundMax;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fDBVoiceAttenuationIn2D("fDBVoiceAttenuationIn2D:Audio", 0.0f);
SettingT<INISettingCollection> bBackgroundLoadLipFiles("bBackgroundLoadLipFiles:BackgroundLoad", false);
SettingT<INISettingCollection> fQuadrupedPitchMult("fQuadrupedPitchMult:HAVOK", 1.0f);
SettingT<INISettingCollection> iHavokSkipFrameCount("iHavokSkipFrameCountTEST:Havok", 0);

int32_t get_iFrameCounter() {
    return Main::iFrameCounter;
}

// fUpdateTargetTimer is deliberately left uninitialised.
MobileObject::MobileObject()
  : TESObjectREFR(), pCurrentProcess(nullptr), pTalkingActivator(nullptr), pDialogueItemTarget(nullptr),
    fSayToFailVoiceTimer(0.0f), bSpeakingDone(true), bTalkingToPlayer(false), bIgnoreChangeAnimationCall(false),
    bSoundFileDone(true), bVoiceFileDone(true), bTalkingActivator(false), bInTempChangeList(false),
    bDoNotRunSayToCallBack(false), cSoundCallBackSet(-1), bSoundCallBackSuccess(false), bUseEmotion(true),
    bRunsInLow(false) {}

MobileObject::~MobileObject() {
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    if (!GetTemporary()) {
        ReferenceProcessLists.RemoveActorFromTempChangeList(this);
        if (!pDataHandler->GetClearingData()) {
            CleanUpTraps();
        }
        if (pCurrentProcess) {
            ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
        }
    }
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    ReferenceProcessLists.kUpdateLock.Unlock();
    if (bTalkingActivator) {
        pParentCell = nullptr;
    }
}

// One byte of its own, the base reference's size, then the process's; the two 16-bit values are added with truncation
// to 16 bits.
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
    SaveGameDataOLD(&ucLevel, sizeof(ucLevel));
    TESObjectREFR::SaveGame(aiFlags);
    if (pCurrentProcess) {
        pCurrentProcess->SaveGame(aiFlags, this);
    }
}

// The process level is saved as one byte, 0xFF without a process. SaveDataEndian takes the fields' addresses as raw
// bytes (char*).
void MobileObject::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    int8_t cProcessLevel = -1;
    if (pCurrentProcess) {
        cProcessLevel = static_cast<int8_t>(pCurrentProcess->GetProcessLevel());
    }
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cProcessLevel), sizeof(cProcessLevel), 0);
    TESObjectREFR::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cSoundCallBackSet), sizeof(cSoundCallBackSet), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSoundCallBackSuccess), sizeof(bSoundCallBackSuccess), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSpeakingDone), sizeof(bSpeakingDone), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bSoundFileDone), sizeof(bSoundFileDone), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bVoiceFileDone), sizeof(bVoiceFileDone), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTalkingToPlayer), sizeof(bTalkingToPlayer), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&bIgnoreChangeAnimationCall), sizeof(bIgnoreChangeAnimationCall), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bUseEmotion), sizeof(bUseEmotion), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fUpdateTargetTimer), sizeof(fUpdateTargetTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSayToFailVoiceTimer), sizeof(fSayToFailVoiceTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTalkingActivator), sizeof(bTalkingActivator), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&bDoNotRunSayToCallBack), sizeof(bDoNotRunSayToCallBack), 0);
    apSaveGameBuffer->SaveFormID(pTalkingActivator, 0);
    apSaveGameBuffer->SaveFormID(pDialogueItemTarget, 0);
    if (pCurrentProcess) {
        pCurrentProcess->SaveGame(apSaveGameBuffer);
    }
}

// Rebuilds the process from the saved level byte, drops the 3D of a cell that is not loaded, then loads the base
// reference's data and the process's.
void MobileObject::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    int8_t cLevel = 0;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cLevel), sizeof(cLevel));
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
            }
            else {
                pCurrentProcess = new HighProcess;
                ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
            }
            break;
        case 1:
            if (pCurrentProcess) {
                if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_MIDDLE_HIGH) {
                    MovetoMiddleHigh();
                }
            }
            else {
                pCurrentProcess = new MiddleHighProcess;
                ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_HIGH, false, false, nullptr);
            }
            break;
        case 2:
            if (pCurrentProcess) {
                if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_MIDDLE_LOW) {
                    MovetoMiddleLow();
                }
            }
            else {
                pCurrentProcess = new MiddleLowProcess;
                ReferenceProcessLists.AddReference(this, PROCESS_TYPE_MIDDLE_LOW, false, false, nullptr);
            }
            break;
        case 3:
            if (pCurrentProcess) {
                if (pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_LOW) {
                    MovetoLow();
                }
            }
            else {
                pCurrentProcess = new LowProcess;
                ReferenceProcessLists.AddReference(this, PROCESS_TYPE_LOW, false, false, nullptr);
            }
            break;
        default: break;
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
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucLevel), sizeof(ucLevel));
    SetProcessLevel(static_cast<PROCESS_TYPE>(static_cast<int8_t>(ucLevel)));
    TESObjectREFR::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cSoundCallBackSet), sizeof(cSoundCallBackSet));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSoundCallBackSuccess), sizeof(bSoundCallBackSuccess));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSpeakingDone), sizeof(bSpeakingDone));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bSoundFileDone), sizeof(bSoundFileDone));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bVoiceFileDone), sizeof(bVoiceFileDone));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTalkingToPlayer), sizeof(bTalkingToPlayer));
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&bIgnoreChangeAnimationCall),
        sizeof(bIgnoreChangeAnimationCall)
    );
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bUseEmotion), sizeof(bUseEmotion));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fUpdateTargetTimer), sizeof(fUpdateTargetTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSayToFailVoiceTimer), sizeof(fSayToFailVoiceTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTalkingActivator), sizeof(bTalkingActivator));
    if (apLoadGameBuffer->GetVersion() >= 8) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNotRunSayToCallBack), sizeof(bDoNotRunSayToCallBack));
    }
    apLoadGameBuffer->LoadFormID(pTalkingActivator);
    apLoadGameBuffer->LoadFormID(pDialogueItemTarget);
    if (pCurrentProcess) {
        pCurrentProcess->LoadGame(apLoadGameBuffer);
    }
}

// The guard masks both flags with 0, so the Set3D branch never runs; this is deliberate.
void MobileObject::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESObjectREFR::InitLoadGame(aiFlags, aiCurrentFlags);
    if (pCurrentProcess) {
        pCurrentProcess->InitLoadGame(aiFlags, aiCurrentFlags, this);
    }
    if (((aiFlags & 0) || (aiCurrentFlags & 0)) && (GetDisabled() || GetDelete())) {
        Set3D(nullptr, true);
    }
}

// The loaded pointers hold form IDs until here (the ID is stored in the pointer field). Get3D is called through the
// vtable first, then directly.
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
        pCurrentProcess = new LowProcess;
        SetProcessLevel(GetDesiredProcessLevel());
    }
    if (TESObjectREFR::Get3D() &&
        (!pCurrentProcess ||
         pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_LOW ||
         pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW)) {
        Set3D(nullptr, true);
    }
}

// Calls TESObjectREFR::Get3D directly rather than through the virtual; the hour is rounded to float before
// SetLastTimeProcessed.
void MobileObject::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESObjectREFR::FinishInitLoadGame(aiFlags, aiOldFlags);
    Actor* pActor = dynamic_cast<Actor*>(this);
    if (pActor &&
        (pActor->GetLifeState() == ACTOR_LIFE_STATE_DEAD || pActor->GetLifeState() == ACTOR_LIFE_STATE_DYING)) {
        DetachCharController();
    }
    else if (aiFlags & (CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE)) {
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
            delete pCurrentProcess;
            pCurrentProcess = nullptr;
        }
        else if (
            !TESObjectREFR::Get3D() &&
            this != pPlayer &&
            (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH ||
             pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH)
        ) {
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

// The equipped weapon and ammo go when the inventory changed, then the base reference's and the process's revert; a
// disabled or deleted reference that is no longer being loaded then loses its process.
void MobileObject::Revert(uint32_t aiFlags) {
    if (aiFlags & CHANGE_REFR_INVENTORY) {
        RemoveWeapon();
        RemoveAmmo();
        AttachAmmo(nullptr);
        RemoveAmmo();
    }
    TESObjectREFR::Revert(aiFlags);
    if (pCurrentProcess) {
        pCurrentProcess->Revert(aiFlags, this);
    }
    if (pSaveLoadGameOLD->IsUnk048Zero() &&
        pSaveLoadGameOLD->GetLoading() &&
        pCurrentProcess &&
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
    if (pCurrentProcess) {
        pCurrentProcess->FinishLoadGame(apLoadGameBuffer);
        if (GetDisabledOrDeleted()) {
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
            delete pCurrentProcess;
            ReferenceProcessLists.kUpdateLock.Unlock();
            pCurrentProcess = nullptr;
        }
        else if (
            !TESObjectREFR::Get3D() &&
            this != pPlayer &&
            (!pCurrentProcess->eLevel || pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH)
        ) {
            PROCESS_TYPE eDesiredProcessLevel = GetDesiredProcessLevel();
            if (eDesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW || eDesiredProcessLevel == PROCESS_TYPE_LOW) {
                if (pSaveLoadGame->GetSaveGameLoading()) {
                    if (eDesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW) {
                        MovetoMiddleLow();
                    }
                    else {
                        MovetoLow();
                    }
                    float fTime = appCalendar.GetHour() - 0.5;
                    pCurrentProcess->SetLastTimeProcessed(fTime);
                    ReferenceProcessLists.RemoveActorFromTempChangeList(this);
                }
                else {
                    ReferenceProcessLists.AddActorToTempChangeList(this);
                }
            }
        }
    }
    if (!bVoiceFileDone && !m_Extra.GetSayToExtra()) {
        // The callback's context is the form ID.
        MobileObject::DialogueConversationCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), true);
    }
    if (pSaveLoadGame->GetSaveGameLoading() && !GetDisabledOrDeleted()) {
        SetProcessLevel(GetDesiredProcessLevel());
    }
}

void MobileObject::SetDelete(bool abDeleted) {
    TESObjectREFR::SetDelete(abDeleted);
    // Every mobile object, projectiles included, is passed as the actor.
    ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(this));
    if (abDeleted) {
        const bool bMagicCaster = dynamic_cast<MagicCaster*>(this) || TESMagicCasterForm::CanBeMagicCaster(this);
        CleanUpOtherMobileObjects(bMagicCaster);
        CleanUpPointersOnDisable();
    }
}

// The sound callback of a say-to or of a dialogue line: finishes the say-to (checks the response ID, runs the topic
// info and clears the extra) or ends the dialogue package's listen timer, then shows the line's subtitle and starts its
// face animation.
void MobileObject::UpdateSoundCallBack() {
    if (cSoundCallBackSet || !GetSayToTopicInfo()) {
        if (cSoundCallBackSet == 1) {
            TESPackage* pPackage = GetCurrentPackage();
            DialoguePackage* pDialoguePackage = nullptr;
            if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
            }

            if (pDialoguePackage) {
                if (pDialoguePackage->pTalkingActor) {
                    pDialoguePackage->pTalkingActor->bVoiceFileDone = true;
                }
                if (!bSoundCallBackSuccess) {
                    if (bSoundFileDone) {
                        pDialoguePackage->flistentimer = 3.0f;
                    }
                    else {
                        pDialoguePackage->flistentimer = 2.0f;
                    }
                }
                else if (bSoundFileDone) {
                    pDialoguePackage->flistentimer = 0.3f;
                }
                else {
                    pDialoguePackage->flistentimer = 20.0f;
                }
                if (pDialoguePackage->GetCurrentDialogItem()) {
                    pDialoguePackage->GetCurrentDialogItem()->RunResult(TIRS_END);
                }
            }
            bVoiceFileDone = true;
        }
    }
    else {
        TESTopicInfo* pTopicInfo = GetSayToTopicInfo();
        TESTopic* pTopic = GetSayToTopic();
        uint32_t uiResponseID = GetSayToResponseID();
        bool bSayTo = true;
        if (uiResponseID && pTopicInfo) {
            ResponseListWrapper aResponseList;
            pTopicInfo->CloneResponseList(&aResponseList);
            if (uiResponseID != static_cast<uint32_t>(aResponseList.GetResponseID())) {
                bSayTo = false;
            }
        }

        if (bSayTo) {
            SetSayToTopic(nullptr);
            ExtraDataList* pExtra = GetExtra();
            pExtra->RemoveSayToInfoExtra();
        }
        if (pTopicInfo && !pTopicInfo->IsRunImmediately()) {
            pTopicInfo->RunResult(TIRS_END, this);
        }

        if (bSayTo) {
            ExtraDataList* pExtra = GetExtra();
            // 0x40000 is kEvent_SayToDone.
            Script::SetActionFlag(pTopic, pExtra, SCRIPT_ACTION_ONSAYTODONE);
        }
        if (!bSoundCallBackSuccess) {
            fSayToFailVoiceTimer = 1.0f;
        }
        if (bSayTo) {
            GetCurrentAIProcess()->SetDoingSayTo(false);
        }
        bVoiceFileDone = true;
        bSoundFileDone = true;
        if (bSayTo) {
            pPlayer->ResetPlayerGreetFlag();
        }
    }
    if (cSoundCallBackSet == 3) {
        BSFaceGenAnimationData* pFaceData = nullptr;
        uint32_t uiEmotion = GetSpeakingEmotion();
        int32_t iEmotionValue = GetSpeakingEmotionValue();
        bool bPlayer = this == pPlayer;
        double dDistance = GetDistanceFromReference(pPlayer, false, false);
        if (static_cast<double>(iSpeakSoundLipDistance.UInt()) > dDistance && (!bPlayer || pPlayer->Is3rdPerson())) {
            pFaceData = GetFaceAnimationData(nullptr);
        }
        BaseProcess* pProcess = GetCurrentAIProcess();
        if (pProcess->GetGreetingFlag()) {
            GetSayToTopic();
            BaseProcess* pSubtitleProcess = GetCurrentAIProcess();
            DialogueItem* pSubtitleItem = pSubtitleProcess->GetSubtitleItem();
            if (pFaceData && IsActor()) {
                if (BSFaceGenManager::GetUseEmotions()) {
                    if (uiEmotion) {
                        int32_t iExpression = BSFaceGenManager::GetExpressionFromEmotion(
                            static_cast<DIALOGUE_EMOTION>(uiEmotion)
                        );
                        float fAmount = static_cast<float>(iEmotionValue / 100.0);
                        pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
                    }
                    else {
                        int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(this))
                                            ->GetActorValue(ActorValue::MOOD);
                        pFaceData->SetExpressionTargetFromMood(iMood, true);
                    }
                }
                else {
                    pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
                }
            }

            if (pSubtitleItem) {
                const char* pText = pSubtitleItem->GetCurrentResponse()->cText.String();
                if ((bGeneralSubtitles.Bool() && pText) ||
                    (IsActor() && static_cast<Actor*>(this)->GetShowSubtitles())) {
                    if (Interface::GetFullHelp()) {
                        char acSubtitle[0x100];
                        std::sprintf(
                            acSubtitle,
                            "'%s' :%s",
                            GetFullName(),
                            pSubtitleItem->GetCurrentResponse()->cText.String()
                        );
                        BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                        Interface::ShowSubtitle(acSubtitle, kHandle, GetLocationOnReference(), pPlayer, false);
                    }
                    else {
                        BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                        Interface::ShowSubtitle(
                            const_cast<char*>(pSubtitleItem->GetCurrentResponse()->cText.String()),
                            kHandle,
                            GetLocationOnReference(),
                            pPlayer,
                            false
                        );
                    }
                }
                GetCurrentAIProcess()->SetSubtitleItem(nullptr);
            }
        }
        else {
            TESPackage* pPackage = GetCurrentPackage();
            BaseProcess* pPackageProcess = GetCurrentAIProcess();
            TESPackage* pProcessPackage = pPackageProcess->GetCurrentPackage();
            DialoguePackage* pDialoguePackage = nullptr;
            if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                pDialoguePackage = static_cast<DialoguePackage*>(pPackage);
            }
            else if (pProcessPackage && pProcessPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                pDialoguePackage = static_cast<DialoguePackage*>(pProcessPackage);
            }

            if (pFaceData && IsActor()) {
                if (BSFaceGenManager::GetUseEmotions()) {
                    if (uiEmotion) {
                        int32_t iExpression = BSFaceGenManager::GetExpressionFromEmotion(
                            static_cast<DIALOGUE_EMOTION>(uiEmotion)
                        );
                        float fAmount = static_cast<float>(iEmotionValue / 100.0);
                        pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
                    }
                    else {
                        int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(this))
                                            ->GetActorValue(ActorValue::MOOD);
                        pFaceData->SetExpressionTargetFromMood(iMood, true);
                    }
                }
                else {
                    pFaceData->ResetCurrentMorphs(0.0f, true, false, false, false, false);
                }
            }

            if (pDialoguePackage) {
                pPlayer->CheckForClosesConversation(
                    pDialoguePackage->GetActorStartedConversation(),
                    pDialoguePackage->GetTargetOfConversation(),
                    pDialoguePackage
                );
                if (pDialoguePackage == pPlayer->pClosestConversation || ReferenceProcessLists.bShowSubtitle) {
                    Conversation* pConversation = pDialoguePackage->GetPackConversation();
                    if (pConversation &&
                        pConversation->GetCurrentItem() &&
                        pConversation->GetCurrentItem()->GetCurrentResponse()) {
                        const char* pText = pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String();
                        if (pText &&
                            (bGeneralSubtitles.Bool() ||
                             (IsActor() && static_cast<Actor*>(this)->GetShowSubtitles()))) {
                            BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                            Interface::ShowSubtitle(
                                const_cast<char*>(pText),
                                kHandle,
                                GetLocationOnReference(),
                                pPlayer,
                                false
                            );
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

void MobileObject::InitHavok() {
    Actor* pActor = static_cast<Actor*>(this);
    RemoveFromAllWater(false);
    if (pActor->pCurrentProcess &&
        (!pActor->pCurrentProcess->eLevel || pActor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH)) {
        TESObjectCELL* pCell = pActor->GetParentCell();
        HighProcess* pBaseProc = nullptr;
        if (!pActor->pCurrentProcess->eLevel) {
            pBaseProc = static_cast<HighProcess*>(pActor->pCurrentProcess);
        }
        if (!pCell || pCell->GetAttached() || pCell->IsAttaching()) {
            bool bKeepHavokData;
            bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
            Actor* pCheckedActor = pActor->IsActor() ? pActor : nullptr;
            if (pCheckedActor) {
                ACTOR_CRITICAL_STAGE eCriticalStage = pCheckedActor->eCriticalStage;
                if (eCriticalStage == ACS_GOO_END || eCriticalStage == ACS_DISINTEGRATE_END) {
                    return;
                }
                // All flags: any reason to keep the actor's Havok data.
                bKeepHavokData = pCheckedActor->GetLifeState() == ACTOR_LIFE_STATE_DEAD ||
                    pCheckedActor->CheckShouldSaveHavokData(0xFFFFFFFFu);
                bhkNiCollisionObject::SetRigidBodyName(pActor->TESObjectREFR::Get3D());
            }
            else {
                bKeepHavokData = false;
            }
            ExtraDismemberedLimbs* pDismemberedLimbs = pActor->m_Extra.GetDismembermentExtra();
            if (pDismemberedLimbs && pActor->pCurrentProcess && pActor->Get3D()) {
                uint32_t uiLimbCount = pDismemberedLimbs->GetDismemberedLimbCount();
                for (uint32_t ui = 0; ui < uiLimbCount; ++ui) {
                    DismemberedLimb* pLimb = pDismemberedLimbs->GetDismemberedLimb(ui);
                    NiNode* pLimbRoot = pActor->pCurrentProcess->GetDamageNode(
                        static_cast<BGSBodyPart::LIMB_ENUM>(pLimb->cLimb)
                    );
                    NiNode* pLimbParent = pLimbRoot->m_pkParent;
                    NiFixedString dismemberRootName(pLimbEnumNames[pLimb->cLimb]);
                    if (!pLimbParent || !NiFixedString::HandlesEqual(pLimbParent->GetName(), dismemberRootName)) {
                        for (uint32_t i = 0; i < pLimbRoot->GetArrayCount(); ++i) {
                            NiAVObject* pChild = pLimbRoot->GetAt(i);
                            if (pChild) {
                                bhkWorld::SetDisabled(pChild, true, true, true);
                            }
                        }
                    }
                }
            }

            bhkCharacterController* pCharController = pActor->GetCharController();
            if (!pCharController) {
                bhkCharacterControllerCinfo info(true);
                info.pObject = pActor->TESObjectREFR::Get3D();
                info.SetPosition(*pActor->QLocationOnReference());
                info.SetWorld(nullptr);
                info.SetAcrobatics(pActor->GetAcrobatics());
                if (pCheckedActor && pActor->GetObjectReference() && pCheckedActor->IsCreature()) {
                    bool bTiltFrontBack = static_cast<TESCreature*>(pCheckedActor->GetObjectReference())
                                              ->TESCreature::GetTiltFrontBack();
                    info.SetPitch(bTiltFrontBack);
                    info.SetRoll(
                        static_cast<TESCreature*>(pCheckedActor->GetObjectReference())->TESCreature::GetTiltLeftRight()
                    );
                    info.SetScale(pActor->GetScale());
                }
                else {
                    info.SetUseDefaultShape(true);
                }
                pCharController = new bhkCharacterController(info);
                if (pCheckedActor && pActor->GetObjectReference()) {
                    pCharController->SetFlag(
                        bhkCharacterController::NOTPUSHABLE_PERMANENT | bhkCharacterController::NOTPUSHABLE,
                        pActor->GetActorBase()->GetNotPushable()
                    );
                }

                // The by-value NiPointer argument is released by the callee.
                pActor->pCurrentProcess->SetCharController(pCharController);
                pCharController->SetMOBObject(pActor);
                pCharController->SetScale(pActor->GetScale());
                pCharController->SetQuickSimulate(Main::GetSceneIsComplex());
                pCharController->SetFlag(bhkCharacterController::RECORD_HITS, true);
                if (pBaseProc) {
                    bhkSimpleShapePhantom* pDoorPhantom = static_cast<bhkSimpleShapePhantom*>(
                        pBaseProc->QDoorDetectionPhantom()
                    );
                    if (pDoorPhantom) {
                        CFilter kFilter;
                        CFilter kControllerFilter;
                        pDoorPhantom->GetCollisionFilter(&kFilter);
                        kFilter.SetGroup(pCharController->GetCollisionFilter(&kControllerFilter)->GetGroup());
                        pDoorPhantom->SetCollisionFilter(kFilter);
                    }
                }
            }
            if (pWorld && pWorld->IsBatchingAdds()) {
                pCharController->SetWaitForBatchAdd(true);
            }

            if (pCheckedActor && bKeepHavokData) {
                // MobileObject::GetCollisionFilter writes the filter word; the group is read from the filter it
                // returns.
                CFilter kActorFilter;
                pCheckedActor->GetCollisionFilter(&kActorFilter.iFilter);
                uint32_t uiGroup = kActorFilter.GetGroup();
                NiNode* p3D = pCheckedActor->Get3D();
                pCheckedActor->pPenetrationDetection->Initialize(p3D, static_cast<int32_t>(uiGroup));
            }

            NiAVObject* pOldAV = pCharController->GetNiObject();
            NiAVObject* pAV = pActor->TESObjectREFR::Get3D();
            if (pOldAV != pAV) {
                pCharController->SetNiObject(pAV);
                NiColorA color(0.0f, 0.0f, 0.0f, 0.0f);
                color.r = 1.0f;
                color.g = 0.0f;
                color.b = 0.0f;
                color.a = 0.25f;
                pCell = pActor->GetParentCell();
                pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
                if (pWorld && pWorld->QDebugDisplay()) {
                    pCharController->BuildDisplayGeometry(nullptr);
                    BSShaderUtil::RecursiveSetPropertyWireframe(pCharController->GetNiObject(), true);
                }

                bhkWorld::SetMotion(
                    pAV,
                    bKeepHavokData ? hkpMotion::MOTION_DYNAMIC : hkpMotion::MOTION_KEYFRAMED,
                    true,
                    false,
                    false
                );
                if (pCheckedActor && !pCheckedActor->IsDead(false)) {
                    pCheckedActor->CreateRagdollInstance(pAV, true, true, false);
                    if (pActor == pPlayer && pCheckedActor->pRagdollController) {
                        pCheckedActor->pRagdollController->SetUsePhantomFootIKCasts(false);
                        pCheckedActor->pRagdollController->SetUseFootEndCast(true);
                    }
                }
            }
            if (pActor->pCurrentProcess &&
                !pActor->pCurrentProcess->eLevel &&
                pActor != pPlayer &&
                pCheckedActor &&
                pCheckedActor->GetActorBase() &&
                !pCheckedActor->GetTurretBehavior()) {
                HighProcess* pHigh = static_cast<HighProcess*>(pActor->pCurrentProcess);
                if (!pHigh->CheckPhantom()) {
                    pHigh->SetupPhantom(pCheckedActor);
                }
            }

            pActor->MoveHavok(false);
            if (bKeepHavokData && !pActor->m_Extra.GetSavedHavokData() && !pActor->m_Extra.GetRagDollData()) {
                NiPoint3 vel(0.0f, 0.0f, -1.0f);
                TESHavokUtilities::AddVelocity(pAV, vel, true);
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
        BSBound* pBSBound = pCurrentProcess->GetBSBound();
        pBSBound->GetBoundMin(boundMin);
        return boundMin;
    }
    return TESObjectREFR::GetBoundMin();
}

NiPoint3 MobileObject::GetBoundMax() {
    if (pCurrentProcess && pCurrentProcess->GetBSBound()) {
        BSBound* pBSBound = pCurrentProcess->GetBSBound();
        pBSBound->GetBoundMax(boundMax);
        return boundMax;
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
        pCharController->SetPosition(*QLocationOnReference());
        if (pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH &&
            this != pPlayer &&
            pActor &&
            pActor->GetActorBase() &&
            !pActor->GetTurretBehavior()) {
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
    }
    else {
        bhkWorld::UpdatePosition(pObject, true, false);
    }
    // Reloads pRagdollController for every use.
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
            // Actor::DoesFly is called directly on every mobile object, projectiles included. The
            // sentinel is -FLT_MAX as a double, compared with the unrounded height; the test fails on unequal or
            // unordered values.
            if (static_cast<Actor*>(this)->DoesFly() &&
                pController->GetState() == HK_CHARACTER_USER_STATE_0 &&
                GetRelevantWaterHeight() == -3.4028234663852886e+38 &&
                pLoadedData) {
                const NiPoint3& kPosition = GetLocationOnReference();
                pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(kPosition, nullptr);
            }
        }
    }
}

bool MobileObject::MovetoHigh() {
    PROCESS_TYPE eOldLevel = pCurrentProcess->eLevel;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RemoveReference(this, eOldLevel);
    HighProcess* pHighProcess = new HighProcess;
    pHighProcess->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = pHighProcess;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

bhkCharacterController* MobileObject::Move(float afDeltaTime, NiPoint3& arDeltaMove, uint32_t aiMoveDir) {
    const hkVector4* pSupportNormal;
    const hkVector4* pZero;
    float fHeading;
    alignas(16) NiMatrix3 kChaseInverse;
    NiMatrix3 kChaseRot;
    alignas(16) float fHeightNavMesh = 0.0f;
    TESObjectCELL* pCell;
    TESWorldSpace* pWorldSpace;
    NiMatrix3 kWarpRot;
    NiPoint3 tmpMove;
    NiPoint3 tmpLoc;
    NiMatrix3 kSkipInverse;
    NiMatrix3 kSkipRot;
    NiPoint3 kZero;
    Actor* pActor = static_cast<Actor*>(this);
    Actor* pCheckedActor = IsActor() ? static_cast<Actor*>(this) : nullptr;
    bhkCharacterController* pCharController = pActor->GetCharController();
    if (pCheckedActor && pCheckedActor->GetActorValue(ActorValue::TURBO) > 0) {
        afDeltaTime = afDeltaTime / BSTimer::QGlobalTimeMultiplier();
    }

    alignas(16) NiPoint3 pos = *pActor->QLocationOnReference();
    if (!std::isnan(arDeltaMove.x) && !std::isnan(arDeltaMove.y)) {
        std::isnan(arDeltaMove.z);
    }
    float fAngleX = pActor->GetAngleOnReference().x;
    float fAngleY = pActor->GetAngleOnReference().y;
    float fAngleZ = pActor->GetAngleOnReference().z;
    if (fAngleX == 3.4028234663852886e+38 ||
        std::isnan(fAngleX) ||
        !std::isfinite(fAngleX) ||
        fAngleY == 3.4028234663852886e+38 ||
        std::isnan(fAngleY) ||
        !std::isfinite(fAngleY) ||
        fAngleZ == 3.4028234663852886e+38 ||
        std::isnan(fAngleZ) ||
        !std::isfinite(fAngleZ)) {
        int32_t iRefFormID = pActor->iFormID;
        Warning(
            "AI: MobileObject::Move called on '%s' (%08X) with invalid angle.",
            pActor->GetFormEditorID(),
            iRefFormID
        );
        NiPoint3 kAngle = *::new (&kZero) NiPoint3(0.0f, 0.0f, 0.0f);
        pActor->SetAngleOnReference(kAngle);
    }
    if (iHavokSkipFrameCount.Int()) {
        NiPoint3 kRotatedDelta;
        uint32_t uiFrameCounter = get_iFrameCounter();
        if (iLastFrameCounter != uiFrameCounter) {
            bSkipHavokThisFrame = uiFrameCounter % (iHavokSkipFrameCount.Int() + 1) != 0;
            iLastFrameCounter = uiFrameCounter;
        }
        if (bSkipHavokThisFrame && pCharController->QSupported()) {
            alignas(16) NiMatrix3 rot;
            if (pActor == pPlayer) {
                fHeading = pActor->MobileObject::GetHeading(false);
            }
            else {
                fHeading = pActor->GetHeading(false);
            }
            rot.MakeZRotation(fHeading);
            arDeltaMove = rot * arDeltaMove;
            pos = pActor->Get3D()->GetTranslate() + arDeltaMove;
            pActor->Get3D()->SetTranslate(pos);
            return pCharController;
        }
        const NiPoint3& kLocation = pActor->GetLocationOnReference();
        NiPoint3 largeDelta = pActor->Get3D()->GetTranslate() - kLocation;
        if (pActor == pPlayer) {
            fHeading = pActor->MobileObject::GetHeading(false);
        }
        else {
            fHeading = pActor->GetHeading(false);
        }
        kSkipRot.MakeZRotation(fHeading);
        NiMatrix3* pInverse = kSkipRot.Inverse(&kSkipInverse);
        arDeltaMove += *::new (&kRotatedDelta) NiPoint3(*pInverse * largeDelta);
    }
    bool bIsImmobile = pCheckedActor ? pCheckedActor->IsImmobile() : false;
    SIT_SLEEP_STATE eSitSleepState = pActor->GetSitSleepState();
    bool bSittingUnsupported = eSitSleepState &&
        pCharController &&
        (pZero = &hkVector4::getZero(),
         (pSupportNormal = &pCharController->GetSupportNormal(), pSupportNormal->equals4(*pZero, 0.001f)));
    bool bCanMove = eSitSleepState == SIT_SLEEP_NORMAL || bSittingUnsupported;
    const NiPoint3& kCameraPos = Main::WorldRootCamera()->GetWorldTranslate();
    NiPoint3 kDiff = pActor->GetLocationOnReference() - kCameraPos;
    float fDistSqr = kDiff.SqrLength();
    bool bIncapacitated = pCheckedActor &&
        (pCheckedActor->IsUnConscious() ||
         pCheckedActor->IsDead(false) ||
         pCheckedActor->IsDying() ||
         pCheckedActor->IsKnockedOut());
    if (!(bhkCharacterController::fCharControllerWarpDistSqr.Float() < static_cast<double>(fDistSqr)) ||
        pActor->IsProjectile() ||
        bIsImmobile ||
        bIncapacitated ||
        pActor == pPlayer ||
        ((tmpLoc = pos, (tmpMove = arDeltaMove, pActor != pPlayer))
             ? fHeading = pActor->GetHeading(false)
             : fHeading = pActor->MobileObject::GetHeading(false),
         (kWarpRot.MakeZRotation(fHeading),
          (tmpMove = kWarpRot * tmpMove,
           (&(tmpLoc += tmpMove),
            (pWorldSpace = pActor->GetWorldSpace(),
             (pCell = pActor->GetParentCell(),
              !Pathing::GetNavMeshZForLocation(tmpLoc, pCell, pWorldSpace, fHeightNavMesh)))))))) {
        if (bCanMove) {
            if (pActor == pPlayer) {
                pCharController->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, bIsImmobile);
            }
            if (!pCharController || pActor->IsDead(true)) {
                NiMatrix3 rot;
                if (pActor->IsProjectile()) {
                    rot.MakeXRotation(pActor->GetLooking());
                    arDeltaMove = rot * arDeltaMove;
                }
                if (pActor == pPlayer) {
                    fHeading = pActor->MobileObject::GetHeading(false);
                }
                else {
                    fHeading = pActor->GetHeading(false);
                }
                rot.MakeZRotation(fHeading);
                arDeltaMove = rot * arDeltaMove;
                pos += arDeltaMove;
            }
            else {
                MoveData md;
                md.fTime = afDeltaTime;
                md.Angle = pActor->GetAngleOnReference();
                md.Displacement = arDeltaMove;
                md.iMoveDirection = aiMoveDir;
                md.bVisible = true;
                if (pActor != pPlayer && pCheckedActor) {
                    NiAVObject* p3D = pActor->Get3D();
                    BSFadeNode* pFadeNode = p3D ? p3D->IsFadeNode() : nullptr;
                    bool bNotFadedOut = true;
                    if (pFadeNode) {
                        NiCamera* pCamera = Main::WorldRootCamera();
                        bNotFadedOut = pFadeNode->ShouldUpdate(pCamera, 0.8f);
                    }
                    md.bVisible = bNotFadedOut;
                }

                if (pCharController->QChaseBip() && !bSittingUnsupported) {
                    NiPoint3 oldPos;
                    if (pActor == pPlayer) {
                        fHeading = pActor->MobileObject::GetHeading(false);
                    }
                    else {
                        fHeading = pActor->GetHeading(false);
                    }
                    kChaseRot.MakeZRotation(fHeading);
                    arDeltaMove = kChaseRot * arDeltaMove;
                    pos += arDeltaMove;
                    pCharController->GetPosition(oldPos);
                    arDeltaMove = pos - oldPos;
                    NiMatrix3* pInverse = kChaseRot.Inverse(&kChaseInverse);
                    md.Displacement = *pInverse * arDeltaMove;
                }
                if (pCharController->QCanPitch() || pCharController->QCanRoll()) {
                    pCharController->SetStuckQuadruped(false);
                    pCharController->SetPitchMult(appTimer.fDelta * fQuadrupedPitchMult.Float());
                }
                if (pCheckedActor && (pCheckedActor->GetMoveMode() & 63) != 0) {
                    md.fCurrMoveSpeed = pCheckedActor->GetCurrentSpeed();
                }
                else {
                    md.fCurrMoveSpeed = 0.0f;
                }
                if (bIsImmobile && (pActor == pPlayer || !pCheckedActor->HasRagDoll())) {
                    pCharController->MoveImmobile(md, pActor == pPlayer);
                }
                else {
                    pCharController->Move(md);
                }
                if (pCharController->QStuckQuadruped()) {
                    alignas(16) NiPoint3 temppos;
                    pCharController->GetPosition(temppos);
                    double dRadius = HK2NI(pCharController->GetRadius());
                    float fDiameter = dRadius + dRadius;
                    temppos.z = temppos.z + fDiameter;
                    pActor->SetPosition(temppos);
                }
                if (!pCharController->QChaseBip()) {
                    pCharController->GetPosition(pos);
                    if (pCharController->QQuadruped() && !bIsImmobile) {
                        pActor->SetHeading(md.Angle.z);
                    }
                    if ((pos - pActor->GetLocationOnReference()).SqrLength() > 67108864.0) {
                        pos = pActor->GetLocationOnReference();
                        pActor->SetPosition(pos);
                    }
                    float fHeight = 0.0f;
                    if (!TES::pInstance->pInteriorCell) {
                        TES::pInstance->GetLandHeight(pos, fHeight);
                        if (fHeight - pos.z > 30.0) {
                            pos.z = fHeight;
                            pActor->SetPosition(pos);
                        }
                    }
                }
                if (pCheckedActor) {
                    pCheckedActor->ProcessDamageImpacts();
                }
            }
        }
        else if (pActor->IsDead(false) && !bIsImmobile) {
            NiMatrix3 rot;
            if (pActor == pPlayer) {
                fHeading = pActor->MobileObject::GetHeading(false);
            }
            else {
                fHeading = pActor->GetHeading(false);
            }
            rot.MakeZRotation(fHeading);
            arDeltaMove = rot * arDeltaMove;
            pos += arDeltaMove;
        }
        if (eSitSleepState || bSittingUnsupported) {
            NiMatrix3 rot;
            MoveData mdSitSleep;
            mdSitSleep.fTime = afDeltaTime;
            mdSitSleep.Angle = pActor->GetAngleOnReference();
            mdSitSleep.Displacement = arDeltaMove;
            mdSitSleep.iMoveDirection = aiMoveDir;
            mdSitSleep.bVisible = true;
            if (!bSittingUnsupported) {
                pCharController->MoveImmobile(mdSitSleep, false);
            }
            if (bSittingUnsupported) {
                pCharController->SetSupported(true);
                pCharController->SetTrueState(HK_CHARACTER_ON_GROUND);
            }
            if (pActor == pPlayer) {
                fHeading = pActor->MobileObject::GetHeading(false);
            }
            else {
                fHeading = pActor->GetHeading(false);
            }
            rot.MakeZRotation(fHeading);
            arDeltaMove = rot * arDeltaMove;
            pos += arDeltaMove;
        }
        pActor->SetLocationOnReference(pos);
        return pCharController;
    }

    if (pCharController && pCharController->GetState() == HK_CHARACTER_USER_STATE_0) {
        if (pActor->pLoadedData->fRelevantWaterHeight == -3.4028234663852886e+38) {
            const NiPoint3& kLocation = pActor->GetLocationOnReference();
            pActor->pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(kLocation, nullptr);
        }
        float fHeightWater = pActor->GetRelevantWaterHeight();
        if (fHeightWater == -3.4028234663852886e+38) {
            if (pCharController) {
                if (pActor->DoesFly()) {
                    pCharController->SetWantState(HK_CHARACTER_IN_AIR);
                }
                else {
                    pCharController->SetWantState(HK_CHARACTER_ON_GROUND);
                }
                tmpLoc.z = fHeightNavMesh;
            }
        }
        else if (fHeightWater < static_cast<double>(tmpLoc.z)) {
            tmpLoc.z = fHeightWater;
        }
        else if (fHeightNavMesh > static_cast<double>(tmpLoc.z) && fHeightWater > static_cast<double>(fHeightNavMesh)) {
            tmpLoc.z = fHeightNavMesh;
        }
    }
    else if (eSitSleepState == SIT_SLEEP_NORMAL && !bSittingUnsupported) {
        tmpLoc.z = fHeightNavMesh;
    }
    if (pCharController &&
        pCharController->GetWorld() &&
        !static_cast<uint8_t>(pCharController->QFarAway()) &&
        (pCharController->QCanPitch() || pCharController->QCanPitch() || pCharController->QQuadruped())) {
        hkVector4 supNorm;
        alignas(16) NiPoint3 hitNormal;
        PathingLocation loc(pActor);
        Pathing::GetNavmeshNormalForLocation(loc, hitNormal);
        NI2HK(supNorm, hitNormal);
        pCharController->SetSupportNormal(supNorm);
        pCharController->ResetCalculatePitchTimer();
        pCharController->InitFrontBack();
        pCharController->CalculatePitchAndRoll();
        fHeading = pActor->GetAngleOnReference().z;
        hkQuaternion hkrot;
        hkrot.setAxisAngle(pCharController->UpVec, -fHeading);
        pCharController->ApplyPitchAndRoll(hkrot);
    }

    pActor->SetPosition(tmpLoc);
    if (pCharController && pCharController->GetState() == HK_CHARACTER_IN_AIR) {
        pCharController->SetTrueState(HK_CHARACTER_ON_GROUND);
    }

    return pCharController;
}

// The jump height is fJumpHeightMin scaled by the reference's scale, times fJumpSwimmingMult when swimming.
bhkCharacterController* MobileObject::Jump() {
    bhkCharacterController* pCharController = static_cast<bhkCharacterController*>(GetCharController());
    if (pCharController) {
        float fJumpHeight = GetScale() * fJumpHeightMin.Float();
        if (pCharController->QIsSwimming()) {
            fJumpHeight = fJumpHeight * fJumpSwimmingMult.Float();
        }
        pCharController->Jump(fJumpHeight);
    }
    return pCharController;
}

void MobileObject::Process(float afDelta) {
    UpdateSoundCallBack();
    if (pCurrentProcess) {
        pCurrentProcess->Update(this);
    }
    if (IsTalkingActivator() && !IsinDialogue()) {
        TESObjectREFR* pActivator = GetTalkingActivator();
        if (pActivator) {
            pActivator->TESObjectREFR::GetExtra()->RemoveTalkingActorExtra();
        }
        ReferenceProcessLists.RemoveReference(this, PROCESS_TYPE_HIGH);
        ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(this));
    }
}

bool MobileObject::ChangeProcessLevel() {
    if (Get3D()) {
        if (TES::pInstance->IsCellLoaded(GetParentCell(), true)) {
            if (pCurrentProcess->eLevel) {
                MovetoHigh();
            }
            else {
                if (this) {
                    delete this;
                }
                return false;
            }
        }
    }
    return true;
}

void MobileObject::UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice) {
    GetAnimation();
    DialoguePackage* pDialoguePackage = nullptr;
    TESPackage* pBasePack = pCurrentProcess->GetCurrentPackage();
    if (pBasePack && pBasePack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        pDialoguePackage = static_cast<DialoguePackage*>(pBasePack);
    }

    bool bTalkFlag = false;
    if (pResponse && pCurrentProcess && bSpeakingDone) {
        BSSoundHandle kHan;
        bTalkFlag = true;
        pTalkingToPlayer = static_cast<Actor*>(this);
        bTalkingToPlayer = true;
        bool bNoSound = true;
        if (pResponse->pVoiceSound) {
            uint32_t uiSoundFileID = pResponse->pVoiceSound->iFormID;
            BSSoundHandle handle;
            if (GetTalkingActivator()) {
                handle = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundFileID, 261u);
                handle.Play(false);
            }
            else {
                handle = PlaySound(uiSoundFileID, false, 0x40000102u, true);
            }
            // The callback's context is the speaker's form ID.
            BSAudioManager::QInstance().SetCompletionCallback(
                static_cast<uint32_t>(handle.iSoundID),
                &MobileObject::SoundFileCallBack,
                reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
            );
            bNoSound = false;
            bSoundFileDone = false;
            bSpeakingDone = false;
        }
        if (bSoundFileDone && bNoSound) {
            PlayerCharacter* pTarget = pPlayer;
            TESIdleForm* pListenerIdle = pResponse->pListenIdle;
            TESIdleForm* pSpeakerIdle = pResponse->pSpeakerIdle;
            uint32_t uiLength = pResponse->cText.GetLength();
            uint32_t uiEmotionValue = pResponse->uiEmotion;
            DIALOGUE_EMOTION eEmotion = pResponse->eEmotion;
            float fDelay = SpeakSoundFunction(
                pResponse->GetVoice()->pString,
                kHan,
                eEmotion,
                uiEmotionValue,
                uiLength,
                pSpeakerIdle,
                pListenerIdle,
                pTarget,
                true,
                true,
                false,
                true,
                true
            );
            BSAudioManager::QInstance().SetCompletionCallback(
                static_cast<uint32_t>(kHan.iSoundID),
                &MobileObject::ChangeDialogueAnimation,
                reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
            );
            bVoiceFileDone = false;
            pCurrentProcess->SetSoundDelay(fDelay);
        }
    }

    if (!abChangeAnimationEndVoice) {
        UpdateAnimation(afDelta);
    }
    UpdateSoundCallBack();
    if (!abChangeAnimationEndVoice) {
        Update(afDelta);
    }
    if (bTalkFlag && pDialoguePackage) {
        pDialoguePackage->pTalkingActor = this;
    }

    TESObjectCELL* pCell = GetParentCell();
    if (pCell) {
        pCell->UpdateLights();
    }
    UpdateRefLight();
}

void MobileObject::UpdateAnimation(float afDelta) {
    if (IsTalkingActivator()) {
        TESObjectREFR* pRef = pTalkingActivator;
        if (pRef->Get3D()) {
            NiControllerManager* pManager = static_cast<NiControllerManager*>(
                pRef->Get3D()->GetController(&NiControllerManager::ms_RTTI)
            );
            if (pManager) {
                NiControllerSequence* pSeq = pManager->GetSequenceByName(FixedStrings::QTalking());
                if (pSeq) {
                    int32_t iFrame = 0;
                    uint32_t uiStartTime = pCurrentProcess->GetCreatureLipSynchStartTime();
                    if (appTimer.iLastTime > uiStartTime) {
                        uiStartTime = pCurrentProcess->GetCreatureLipSynchStartTime();
                        iFrame = (appTimer.iLastTime - uiStartTime) / 33;
                    }
                    if (pCurrentProcess->GetCreatureLipSynchAnim()) {
                        float fLipPercent = pCurrentProcess->GetCreatureLipSynchAnim()->GetPuppetValue(iFrame);
                        if (fLipPercent < 0.009999999776482582) {
                            fLipPercent = 0.01f;
                        }
                        Animation::SetLipTime(fLipPercent, pManager, pSeq);
                        if (iFrame <= pCurrentProcess->GetCreatureLipSynchAnim()->iNumFrames) {
                            Animation::SetLipTime(fLipPercent, pManager, pSeq);
                        }
                        else {
                            Animation::SetLipTime(0.0f, pManager, pSeq);
                            pManager->DeactivateSequence(pSeq, 0.0f);
                            pCurrentProcess->SetCreatureLipSynchAnim(nullptr);
                            pCurrentProcess->SetCreatureLipSynchStartTime(0);
                        }
                    }
                }
            }
            pTalkingActivator->UpdateAnimation();
            float fTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
            NiUpdateData updateData(fTime, true, false);
            pRef->Get3D()->Update(updateData);
        }
    }
}

void MobileObject::SetDialoguewithPlayer(bool abFlag) {
    if (pCurrentProcess) {
        pCurrentProcess->SetTalkingtoPC(abFlag);
        BSFaceGenNiNode* pFaceNode = pCurrentProcess->GetFaceNode();
        BSFaceGenNiNode* pFaceSkinnedNode = pCurrentProcess->GetFaceSkinnedNode();
        NiTriShape* pHeadAnims = static_cast<NiTriShape*>(pCurrentProcess->GetHeadAnims());
        if (pHeadAnims && pHeadAnims->NiAVObject::GetParent() && pFaceNode && pFaceSkinnedNode) {
            NiNode* pParent = pHeadAnims->NiAVObject::GetParent();
            pParent->SetSelectiveUpdate(true);
            pFaceNode->SetSelectiveUpdate(true);
            pFaceSkinnedNode->SetSelectiveUpdate(true);
        }

        if (abFlag) {
            pCurrentProcess->ClearAllHeadTrackTargets();
            pCurrentProcess->SetDialogHeadTrackTarget(pPlayer);
        }
    }
}

bool MobileObject::InitiateDialogue(
    MobileObject* apTarget,
    PackageLocation* pPackLoc,
    PackageLocation* pPackSecondLoc,
    bool bHeadTrack,
    bool bMove,
    bool bflag,
    TESTopic* pTopic,
    bool abTargetContinueMoving,
    bool abTargetMoveTowardStarter
) {
    if (GetCurrentProcessType() || apTarget->GetCurrentProcessType()) {
        return false;
    }
    TESPackage* pTempPack = pCurrentProcess->GetCurrentPackage();
    Conversation* pConversation = nullptr;
    if (!apTarget ||
        (apTarget->GetSitSleepState() &&
         apTarget->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
         apTarget->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING)) {
        return false;
    }
    MobileObject* pSpeakAs = this;
    MobileObject* pTalkActivator = nullptr;
    if (IsTalkingActivator()) {
        pSpeakAs = SetUpTalkingActivatorActor(apTarget, pTalkActivator);
    }
    if (apTarget != pPlayer) {
        pConversation = new Conversation(pSpeakAs, apTarget, pTopic);
    }
    StopCurrentDialogue();
    pCurrentProcess->ClearRunOncePackage();
    if (apTarget != pPlayer && !pConversation->FirstItem()) {
        return false;
    }
    pCurrentProcess->SavePackageToExtraData(this);
    pCurrentProcess->ClearActionHeadTrackTarget(true);
    pCurrentProcess->SetDialogHeadTrackTarget(apTarget);
    apTarget->pCurrentProcess->ClearActionHeadTrackTarget(true);
    apTarget->pCurrentProcess->SetDialogHeadTrackTarget(this);
    DialoguePackage* pPack = new DialoguePackage(
        pConversation,
        static_cast<Actor*>(this),
        static_cast<Actor*>(apTarget),
        static_cast<Actor*>(pTalkActivator),
        true,
        true
    );
    pPack->ptopic = pTopic;
    if (pTempPack) {
        pPack->CopyAllFlags(pTempPack);
    }
    pPack->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
    pPack->SetMustReachLocation(true);
    pPack->SetMustComplete(true);
    PackageLocation* pLoc = new PackageLocation;
    pLoc->SetLocReference(apTarget);
    pPack->SetPackageLocation(pLoc);
    if (pLoc) {
        delete pLoc;
    }
    PackageTarget* pNew = new PackageTarget;
    pPack->SetPackageTarget(pNew);
    if (pNew) {
        delete pNew;
    }
    pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
    pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
    pPack->GetPackageTarget()->SetTargReference(apTarget);
    if (GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
        pPack->GetPackageTarget()->iValue = 200u;
    }
    else {
        pPack->GetPackageTarget()->iValue = 90u;
    }
    pCurrentProcess->ComputeLastTimeProcessed();
    PutCreatedPackage(pPack, false, true);
    if (bflag) {
        pCurrentProcess->AddToProcedureIndexRunning(this, 1);
    }
    return true;
}

bool MobileObject::InitiateDialogue(TESPackage* pDialoguePackage) {
    MobileObject::GetCurrentAIProcess()->GetCurrentPackage();
    Conversation* pConversation = nullptr;
    Actor* pTarget = static_cast<Actor*>(pCurrentProcess->GetTarget());
    if (static_cast<const void*>(pTarget) != static_cast<const void*>(pPlayer)) {
        pConversation = new Conversation(this, pTarget, nullptr);
    }
    StopCurrentDialogue();
    pCurrentProcess->ClearRunOncePackage();
    if (static_cast<const void*>(pTarget) != static_cast<const void*>(pPlayer) && !pConversation->FirstItem()) {
        return false;
    }
    if (!pDialoguePackage->GetNoHeadTrack()) {
        MobileObject::GetCurrentAIProcess()->ClearActionHeadTrackTarget(true);
        MobileObject::GetCurrentAIProcess()->SetDialogHeadTrackTarget(pTarget);
        pTarget->MobileObject::GetCurrentAIProcess()->ClearActionHeadTrackTarget(true);
        pTarget->MobileObject::GetCurrentAIProcess()->SetDialogHeadTrackTarget(this);
    }
    DialoguePackage* pNewPackage = new DialoguePackage(
        pConversation,
        static_cast<Actor*>(this),
        pTarget,
        nullptr,
        false,
        pDialoguePackage->pIdleCollection == nullptr
    );
    if (pDialoguePackage) {
        pNewPackage->CopyAllFlags(pDialoguePackage);
    }
    pNewPackage->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
    pNewPackage->SetMustReachLocation(pDialoguePackage->GetMustReachLocation());
    pNewPackage->SetMustComplete(pDialoguePackage->GetMustComplete());
    pNewPackage->SetPackageLocation(pDialoguePackage->GetLocation());
    pNewPackage->SetPackageSecondLocation(pDialoguePackage->GetPackageSecondLocation());
    pNewPackage->SetPackageTarget(pDialoguePackage->GetPackageTarget());
    pNewPackage->SetProcedureType(PACKAGE_PROCEDURE_DIALOGUE);
    MobileObject::GetCurrentAIProcess()->ComputeLastTimeProcessed();
    PutCreatedPackage(pNewPackage, false, true);
    SetDialogueItemTarget(pTarget);
    pCurrentProcess->SetTarget(pTarget);
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    if (static_cast<const void*>(pTarget) == static_cast<const void*>(pPlayer) ||
        static_cast<const void*>(pTarget) == static_cast<const void*>(this)) {
        if (!pTarget->IsDead(false)) {
            MobileObject::GetCurrentAIProcess()->AddToProcedureIndexRunning(static_cast<Actor*>(this), 3u);
        }
    }
    else {
        pTarget->MobileObject::GetCurrentAIProcess()->SavePackageToExtraData(pTarget);
        pTarget->PutCreatedPackage(GetCurrentPackage(), false, true);
        pTarget->MobileObject::GetCurrentAIProcess()->SetTarget(pTarget);
        pTarget->SetDialogueItemTarget(this);
    }
    return true;
}

float MobileObject::SpeakSoundFunction(
    const char* apSoundFile,
    BSSoundHandle& aSoundHand,
    DIALOGUE_EMOTION aeEmotiontype,
    uint32_t aiEmotionValue,
    int32_t aiStringLength,
    TESIdleForm* apResponseSpeakerIdle,
    TESIdleForm* apResponseListenerIdle,
    TESObjectREFR* apTarget,
    bool abSetEmotion,
    bool ab2D,
    bool abQueue,
    bool abLip,
    bool abUseIdleManager
) {
    if (IsTalkingActivator()) {
        float fDelay = 0.0f;
        BSStringT<char> tempstring(apSoundFile);
        LipSynchAnim* pAnim = this->pCurrentProcess->GetLipAnim();
        BSFaceGenAnimationData* pAnimData = nullptr;
        this->SetSpeakingEmotion(aeEmotiontype);
        this->SetSpeakingEmotionValue(static_cast<int32_t>(aiEmotionValue));
        if (apSoundFile) {
            char acSoundFile[516];
            BSstrcpy(acSoundFile, 512, apSoundFile);
            this->StopCurrentDialogue();
            bool bIsPlayer = this == pPlayer;
            float fSoundDuration = 0.0f;
            bool b2D = this == pPlayer || ab2D;
            double dDistance = this->GetDistanceFromReference(pPlayer, false, false);
            if (static_cast<double>(iSpeakSoundLipDistance.Int()) > dDistance &&
                (!bIsPlayer || pPlayer->Is3rdPerson())) {
                pAnimData = this->GetFaceAnimationData(nullptr);
            }
            if (!pAnim) {
                if (!abLip || this->pCurrentProcess->GetLipFileFailed()) {
                    BSFaceGenAnimationData* pFaceAnimData = this->GetFaceAnimationData(nullptr);
                    if (pFaceAnimData) {
                        pFaceAnimData->AddAnimationDataFromNote("BigAah 0.9 0.2 0.1 0.2");
                    }
                    if (this->pCurrentProcess->GetLipFileFailed()) {
                        // The callback's context is the speaker's form ID.
                        MobileObject::DisplaySubTitleCallback(
                            reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)),
                            0
                        );
                        this->pCurrentProcess->SetLipFileFailed(false);
                    }
                }
                else if (LipSynchAnim::ChangeFilenameToLipSynch(tempstring)) {
                    if (abQueue) {
                        this->pCurrentProcess->SetWaitingforLipFile(true);
                        this->pCurrentProcess->SetLipQuequed(true);
                        ReferenceProcessLists.kLipSyncManager.BuildLipSynicFile(this, tempstring.pString);
                        return 0.0f;
                    }
                    pAnim = LipSynchAnim::LoadFromFile(tempstring);
                    if (!pAnim) {
                        DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
                    }
                    this->pCurrentProcess->SetWaitingforLipFile(false);
                }
            }
            if (this->GetAnimation()) {
                if (apResponseSpeakerIdle || abUseIdleManager) {
                    this->pCurrentProcess->SetupSpecialIdle(
                        this,
                        apResponseSpeakerIdle,
                        apResponseSpeakerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
                        false,
                        true,
                        true
                    );
                }
                if (apTarget && apTarget != this && (apTarget->IsMobileObject() || apTarget->IsActor())) {
                    Actor* pTargetActor = static_cast<Actor*>(apTarget);
                    bool bNoPackageIdles = true;
                    if (apTarget->IsActor()) {
                        TESPackage* pTargetPack = pTargetActor->GetCurrentPackage();
                        if (pTargetPack) {
                            bNoPackageIdles = pTargetPack->pIdleCollection == nullptr;
                        }
                    }
                    if (apResponseListenerIdle || bNoPackageIdles) {
                        pTargetActor->pCurrentProcess->SetupSpecialIdle(
                            pTargetActor,
                            apResponseListenerIdle,
                            apResponseListenerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
                            false,
                            true,
                            true
                        );
                    }
                }
            }
            if (pAnim) {
                int64_t iFrameCount = pAnim->iNumFrames;
                fSoundDuration = static_cast<double>(iFrameCount) / 30.0;
                pAnim->ApplyToNode(pAnimData, 0.2f);
            }
            if (abSetEmotion && this->pCurrentProcess) {
                this->pCurrentProcess->SetLastSpeakingEmotion(aeEmotiontype);
            }

            uint32_t uiSoundFlags = BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_VOICE;

            if (b2D) {
                uiSoundFlags |= BSGameSound::SOUND_FLAG_IS_2D;
            }
            else {
                uiSoundFlags |= BSGameSound::SOUND_FLAG_IS_3D;
            }
            aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(acSoundFile, uiSoundFlags, nullptr);
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            MobileObject* pAct = this;
            if (apTarget && apTarget->IsActor()) {
                pAct = static_cast<MobileObject*>(apTarget);
            }

            // The subtitle callback's context is the listener's (or the speaker's) form ID.
            mgr->SetPlayCallback(
                static_cast<uint32_t>(aSoundHand.iSoundID),
                &MobileObject::DisplaySubTitleCallback,
                reinterpret_cast<void*>(static_cast<uintptr_t>(pAct->iFormID))
            );
            if (b2D) {
                aSoundHand.SetStaticAttenuation(static_cast<int32_t>(fDBVoiceAttenuationIn2D.Float() * 100.0));
            }
            else {
                NiPoint3 kPosition = this->GetLocationOnReference();
                // Reads the maximum first.
                float fMaxDist = BSAudio::GetMaxDialogDist();
                float fMinDist = BSAudio::GetMinDialogDist();
                aSoundHand.SetMinMax(fMinDist, fMaxDist);
                aSoundHand.SetPosition(kPosition.x, kPosition.y, kPosition.z);
                aSoundHand.SetObjectToFollow(this->Get3D());
            }
            if (pAnim) {
                double dStartingDelay = pAnim->GetStartingDelay();
                float fPlayDelay = (BSFaceGenAnimationData::GetSpeechDelay() + dStartingDelay) * 1000.0;
                aSoundHand.PlayAfter(static_cast<int32_t>(fPlayDelay), 0);
                dStartingDelay = pAnim->GetStartingDelay();
                fDelay = BSFaceGenAnimationData::GetSpeechDelay() + dStartingDelay;
                if (fSoundDuration > 0.0) {
                    fDelay = fDelay + fSoundDuration;
                }
                else if (aSoundHand.GetDuration()) {
                    int32_t iDuration = aSoundHand.GetDuration();
                    fDelay = static_cast<double>(iDuration) / 1000.0 + fDelay;
                }
                else {
                    fDelay = fAIDefaultSpeechMult.Float() * static_cast<double>(aiStringLength) + fDelay;
                }
                this->pCurrentProcess->SetCreatureLipSynchStartTime(
                    static_cast<int32_t>(fPlayDelay) + appTimer.iLastTime
                );
                this->pCurrentProcess->SetCreatureLipSynchAnim(pAnim);
            }
            else {
                aSoundHand.PlayAfter(1u, 0);
                fDelay = fAIDefaultSpeechMult.Float() * static_cast<double>(aiStringLength);
            }
            if (this->bTalkingToPlayer) {
                this->bSpeakingDone = false;
            }
            this->pCurrentProcess->SetSoundHandle(ACTOR_SOUND_HANDLE_VOICE, aSoundHand);
            if (pAnim) {
                if (pAnim != this->pCurrentProcess->GetCreatureLipSynchAnim() && pAnim) {
                    delete pAnim;
                }

                this->pCurrentProcess->SetLipAnim(nullptr);
                this->pCurrentProcess->SetWaitingforLipFile(false);
            }
            return fDelay;
        }

        return 0.0f;
    }

    return 0.0f;
}

void MobileObject::EndDialogue() {
    if (pCurrentProcess) {
        pCurrentProcess->SetSubtitleItem(nullptr);
        if (pCurrentProcess->GetCurrentPackage() &&
            pCurrentProcess->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            DialoguePackage* pDialoguePackage = dynamic_cast<DialoguePackage*>(pCurrentProcess->GetCurrentPackage());
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
                    pSubject->pCurrentProcess->ClearDialogHeadTrackTarget(true);
                    pSubject->bVoiceFileDone = true;
                }
                if (pTarget) {
                    if (pTarget->IsActor()) {
                        ReferenceProcessLists.kLipSyncManager.CancelLipFile(pTarget);
                    }
                    pTarget->pCurrentProcess->ClearDialogHeadTrackTarget(true);
                    pTarget->bVoiceFileDone = true;
                }
                if (pSubject && pTarget) {
                    pSubject->pCurrentProcess->SetDefaultHeadTrackTarget(pTarget);
                    pTarget->pCurrentProcess->SetDefaultHeadTrackTarget(pSubject);
                    pSubject->fUpdateTargetTimer = 1.0f;
                    pTarget->fUpdateTargetTimer = 1.0f;
                    pSubject->pCurrentProcess->SetLipFileFailed(false);
                    pTarget->pCurrentProcess->SetLipFileFailed(false);
                }

                Animation* pAnimation = pSubject->GetAnimation();
                if (pAnimation) {
                    pAnimation->SpecialIdleFree(true, false);
                }
                pAnimation = pTarget->GetAnimation();
                if (pAnimation) {
                    pAnimation->SpecialIdleFree(true, false);
                }
                if (pSubject && pSubject->pCurrentProcess) {
                    pSubject->pCurrentProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
                }

                if (pTarget && pTarget->pCurrentProcess) {
                    pTarget->pCurrentProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
                }

                pSubject->GetCurrentPackage();
                pTarget->GetCurrentPackage();
                pSubject->pCurrentProcess->LoadPackageFromExtraData(pSubject);
                if (pSubject->IsActor()) {
                    if (!pSubject->pCurrentProcess->GetCurrentPackage() && !pDataHandler->GetClearingData()) {
                        pSubject->pCurrentProcess->CheckforNewPackage(pSubject, true);
                    }
                }
                if (pTarget && pTarget->IsinDialogue() && pTarget->pCurrentProcess) {
                    pTarget->pCurrentProcess->LoadPackageFromExtraData(pTarget);
                    if (!pDataHandler->GetClearingData() && pTarget->IsActor()) {
                        pTarget->pCurrentProcess->CheckforNewPackage(pTarget, true);
                    }
                }

                MobileObject* pTalker = pDialoguePackage->pTalkingActivator;
                if (pTalker) {
                    pTalker->GetTalkingActivator()->m_Extra.RemoveTalkingActorExtra();
                    pTalker->GetTalkingActivator()->m_Extra.RemoveTalkingActorExtra();
                    ReferenceProcessLists.RemoveReference(pTalker, PROCESS_TYPE_HIGH);
                    ReferenceProcessLists.AddActorToTempChangeList(pTalker);
                }
                delete pDialoguePackage;
            }
        }
        else {
            TESObjectREFR* pHeadTrackTarget = pCurrentProcess->GetCurrentHeadTrackTarget(
                BaseProcess::HEAD_TRACK_TYPE_DIALOG
            );
            pCurrentProcess->ClearDialogHeadTrackTarget(true);
            pCurrentProcess->SetDefaultHeadTrackTarget(pHeadTrackTarget);
            fUpdateTargetTimer = 1.0f;
        }
    }
}

// A mobile object has no editor location of its own: zero.
NiPoint3 MobileObject::GetEditorLocationCoord() {
    return NiPoint3::ZERO;
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
    bool bRes = DetachCharController();
    if (this->pCurrentProcess) {
        this->pCurrentProcess->SetCharController(
            NiPointer<bhkCharacterController>(static_cast<bhkCharacterController*>(nullptr))
        );
    }
    return bRes;
}

void MobileObject::SetPosition(const NiPoint3& arPos) {
    SetLocationOnReference(arPos);
    auto* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (pController) {
        pController->SetPosition(arPos);
        pController->SetLinearVelocity(hkVector4::getZero());
        // Both getters are called and their results discarded.
        pController->QFallStartHeight();
        pController->QHitDamage();
        pController->ClearManifold();
    }
    if (pLoadedData && pLoadedData->m_spData3D) {
        pLoadedData->m_spData3D->SetTranslate(arPos);
    }
}

void MobileObject::GetLinearVelocity(NiPoint3& arVel) {
    bhkCharacterController* pCharController = GetCharController();
    if (pCharController) {
        pCharController->GetLinearVelocity(arVel);
    }
    else {
        arVel = NiPoint3::ZERO;
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
            default: break;
        }
    }
    afAngle = ClampAngle(afAngle);
    if (IsActor() && static_cast<Actor*>(this)->GetTurretBehavior()) {
        // The angle is rounded to float after each step.
        afAngle = afAngle - data.kRotation.z;
        while (afAngle < -3.1415927410125732) {
            afAngle = afAngle + 6.2831854820251465;
        }
        while (afAngle >= 3.1415927410125732) {
            afAngle = afAngle - 6.2831854820251465;
        }
        SetAngleOnReferenceY(afAngle);
    }
    else {
        SetAngleOnReferenceZ(afAngle);
    }
}

void MobileObject::SetHeading(TESObjectREFR* apTargetReference) {
    const NiPoint3& kLocation = GetLocationOnReference();
    NiPoint3 movedir = apTargetReference->GetLocationOnReference() - kLocation;
    float fTargetAngle = GetZAngleFromVector(movedir);
    SetHeading(fTargetAngle);
}

TESObjectREFR* MobileObject::GetCurrentTarget() {
    return pCurrentProcess->GetTarget();
}

bool MobileObject::IsAtPoint(const NiPoint3& aPoint, float afRadius, bool abExpandRadius, bool abAlwaysTestHeight) {
    NiPoint3 kLocation = GetLocationOnReference();
    NiPoint3 kDiff = kLocation - aPoint;
    float fMinZ = static_cast<float>(kLocation.z - 30.0);
    float fMaxZ = static_cast<float>(kLocation.z + 128.0f + 30.0);
    if (fMinZ < static_cast<double>(aPoint.z) && fMaxZ > static_cast<double>(aPoint.z)) {
        kDiff.z = 0.0f;
    }
    if (abExpandRadius) {
        afRadius = afRadius + 32.0f;
    }
    return NiPoint3::IsMagnitudeLongerThanRadius(&kDiff, afRadius) <= 0;
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
            pCharacterController->BuildDisplayGeometry(nullptr);
            BSShaderUtil::RecursiveSetPropertyWireframe(pCharacterController->GetNiObject(), 1);
        }
        else {
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
    BGSTalkingActivator* pActiTalk = static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference());
    if (pActiTalk->GetTempRef()) {
        return pActiTalk->GetTempRef();
    }

    return this;
}

void MobileObject::AddSayOnceTopicInfo(TESTopicInfo* apTopicInfo) {
    uint32_t uiDate = iGameYear << 13 | (LowProcess::cCurrentGameDay | LowProcess::eCurrentGameMonth << 9);
    SayOnceTopicInfos* pInfos = new SayOnceTopicInfos(apTopicInfo, uiDate, fGameHour);
    MobileObject* pSpeaker = this;
    if (IsTalkingActivator()) {
        pSpeaker = static_cast<MobileObject*>(GetTalkingActivator());
    }
    pSpeaker->m_Extra.SetExtraSayTopicInfoOnceADayExtra(pInfos);
    pSpeaker->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

void MobileObject::StopCurrentDialogue() {
    BaseProcess* pProc = pCurrentProcess;
    bool bPlaying = false;
    if (pProc) {
        BSSoundHandle kHandle = pProc->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        if (kHandle.IsValid()) {
            bPlaying = true;
        }
    }
    if (bPlaying) {
        TESTopicInfo* pInfo = GetSayToTopicInfo();
        TESTopic* pTopic = GetSayToTopic();
        SetSayToTopic(nullptr);
        m_Extra.RemoveSayToInfoExtra();
        if (pInfo) {
            if (!pInfo->IsRunImmediately()) {
                pInfo->RunResult(TIRS_END, this);
            }
            Script::SetActionFlag(pTopic, &m_Extra, SCRIPT_ACTION_ONSAYTODONE);
            bDoNotRunSayToCallBack = true;
            pCurrentProcess->SetDoingSayTo(false);
        }
        cSoundCallBackSet = -1;
        pProc->StopSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        pProc->SetGreetingFlag(false);
        pPlayer->ResetPlayerGreetFlag();
    }
    bVoiceFileDone = true;
    bSoundFileDone = true;
    if (!IsTalkingActivator() && Get3D()) {
        static_cast<Actor*>(this)->Actor::ResetFaceData();
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
    const NiPoint3& kPosition = static_cast<TESObjectREFR*>(this)->GetLocationOnReference();
    TESObjectCELL* pCell = GetParentCell();
    TESWorldSpace* pWorld = GetWorldSpace();
    SetPackageStartLocation(pWorld, pCell, kPosition, fZAngle);
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

// Calls GetCurrentState twice: once for the null test and once for the state it asks.
bool MobileObject::IsInAir() {
    bool bInAir = false;
    bhkCharacterController* pController = GetCharController();
    if (!pController || !pController->GetCurrentState()) {
        return bInAir;
    }
    hkpCharacterState* pState = pController->GetCurrentState();
    if (pState->getType() == HK_CHARACTER_IN_AIR) {
        bInAir = true;
    }
    return bInAir;
}

BSSoundHandle
MobileObject::PlaySoundByEditorName(const char* apEditorName, bool abLoop, uint32_t aiSoundType, bool abAllowCopy) {
    BSSoundHandle soundHand;
    if (Get3D()) {
        TESSound* pSound = pDataHandler->GetSound(apEditorName);
        if (pSound) {
            uint32_t uiFormID = pSound->iFormID;
            soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiFormID, aiSoundType);
            if ((aiSoundType & 2) != 0) {
                NiPoint3 kPosition = GetLocationOnReference();
                soundHand.SetPosition(kPosition.x, kPosition.y, kPosition.z);
                soundHand.SetObjectToFollow(Get3D());
            }
            soundHand.SetVolume(1.0f);
            soundHand.Play(abLoop);
        }
    }
    return soundHand;
}

uint32_t* MobileObject::GetCollisionFilter(uint32_t* apResult) {
    CFilter kControllerFilter;
    CFilter kEmptyFilter;
    const uint32_t* pFilter;
    auto* pController = GetCharController();
    if (pController) {
        pFilter = &pController->GetCollisionFilter(&kControllerFilter)->iFilter;
    }
    else {
        kEmptyFilter = CFilter(0);
        pFilter = &kEmptyFilter.iFilter;
    }
    *apResult = *pFilter;
    return apResult;
}

bool MobileObject::GetRunsInLow() {
    return bRunsInLow;
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
    return pCurrentProcess->IsTalking(static_cast<Actor*>(this));
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
    }
    else {
        pExtraData = GetExtra()->GetExtraSayTopicInfoOnceADayExtra();
    }
    if (!pExtraData) {
        return;
    }
    BSSimpleList<SayOnceTopicInfos*>* pIter = pExtraData->pListofSaidOnceTopicInfos;
    // GetItem is called once for the test and again for the item.
    while (pIter && pIter->GetItem()) {
        SayOnceTopicInfos* pTopicInfoData = pIter->GetItem();
        bool bRemove = false;
        float fHour = pTopicInfoData->fHourStamp + fSayOncePerDayInfoTimer.Float();
        int32_t iDay = pTopicInfoData->iDateStamp & 0x1FF;
        int32_t iMonth = (pTopicInfoData->iDateStamp & 0x1E00) >> 9;
        int32_t iYear = (pTopicInfoData->iDateStamp & 0x7FFE000) >> 13;
        int32_t iCurrMonth = LowProcess::eCurrentGameMonth;
        int32_t iCurrDay = LowProcess::cCurrentGameDay;
        // The year is compared signed, though iGameYear is unsigned.
        int32_t iCurrYear = static_cast<int32_t>(iGameYear);
        float fCurrHour = fGameHour;
        // NaN hours take the "not past 23" paths.
        if (!(fHour > 23.0) && iCurrDay <= iDay && iCurrMonth <= iMonth && iCurrYear <= iYear) {
            if (fHour <= fCurrHour) {
                bRemove = true;
            }
        }
        else if (fHour > 23.0) {
            fHour = fHour - 23.0;
            if (fHour <= fCurrHour && (iCurrDay > iDay || iCurrMonth > iMonth || iCurrYear > iYear)) {
                bRemove = true;
            }
        }
        else if (fHour >= fCurrHour) {
            bRemove = true;
        }
        if (bRemove) {
            pIter->Remove(pTopicInfoData);
            ourFree(pTopicInfoData);
            pIter = pExtraData->pListofSaidOnceTopicInfos;
        }
        else {
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
    SetLooking(GetLooking() + afModAngle);
}

void MobileObject::DeleteCurrentProcess() {
    BaseProcess* lpProcess = pCurrentProcess;
    if (lpProcess) {
        delete lpProcess;
    }
    pCurrentProcess = nullptr;
}

void MobileObject::SetChaseBip(bool abTF) {
    auto* pController = GetCharController();
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
    if (pNode && (!pNode->GetHighDetail() || abForceRecheck) && CheckHighDetail()) {
        pModelLoader->UpgradeTextures(pNode);
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
        // The state is compared signed though SIT_SLEEP_STATE is unsigned.
        const int32_t iSitSleepState = static_cast<int32_t>(GetSitSleepState());
        if (iSitSleepState > static_cast<int32_t>(SIT_SLEEP_NORMAL) &&
            iSitSleepState <= static_cast<int32_t>(SIT_SLEEP_WANT_TO_STAND)) {
            fMaxLookDown = fSittingMaxLookingDown.Float() * 0.01745329238474369;
        }
    }
    // The lower limit -1.5533430576324463 is compared as a double and stored as a float; both compares are a strict
    // "less than".
    if (afAngle < -1.5533430576324463) {
        SetAngleOnReferenceX(-1.5533430576324463f);
    }
    else if (fMaxLookDown < afAngle) {
        SetAngleOnReferenceX(fMaxLookDown);
    }
    else {
        SetAngleOnReferenceX(afAngle);
    }
}

void MobileObject::CleanUpOtherMobileObjects(bool abMagicCaster) {
    ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
    uint32_t uiIndex = 0;
    for (uint32_t i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); uiIndex < i;
         i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
        MobileObject* pMobile = pMobArray->GetItem(uiIndex);
        if (pMobile != this) {
            if (pMobile->IsActor()) {
                pMobile->pCurrentProcess->RemoveHeadTrackTarget(this);
                if (abMagicCaster) {
                    static_cast<Actor*>(pMobile)->Actor::CheckDeletedCaster(GetMagicCaster());
                }
                MagicTarget* pDesiredTarget = pMobile->pCurrentProcess->GetDesiredTarget();
                if (pDesiredTarget == GetMagicTarget()) {
                    pMobile->pCurrentProcess->SetDesiredTarget(nullptr);
                }
            }
            else if (pMobile->IsProjectile()) {
                static_cast<Projectile*>(pMobile)->Projectile::CheckRefDeleted(this);
            }
            else if (pMobile->IsExplosion()) {
                static_cast<Explosion*>(pMobile)->Explosion::CheckRefDeleted(this);
            }
            else if (abMagicCaster) {
                MagicProjectile* pProjectile = dynamic_cast<MagicProjectile*>(pMobile);
                if (pProjectile) {
                    pProjectile->CheckDeletedCaster(GetMagicCaster());
                }
            }
        }
        ++uiIndex;
    }
    for (uint32_t i = pMobArray->GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
         i < pMobArray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         ++i) {
        MobileObject* pMob = pMobArray->GetItem(i);
        if (pMob != this) {
            Actor* pActor = dynamic_cast<Actor*>(pMob);
            if (pActor) {
                if (abMagicCaster) {
                    pActor->CheckDeletedCaster(GetMagicCaster());
                }
                MagicTarget* pDesiredTarget = pActor->pCurrentProcess->GetDesiredTarget();
                if (pDesiredTarget == GetMagicTarget()) {
                    pActor->pCurrentProcess->SetDesiredTarget(nullptr);
                }
            }
            else if (abMagicCaster) {
                MagicProjectile* pProjectile = dynamic_cast<MagicProjectile*>(pMob);
                if (pProjectile) {
                    pProjectile->CheckDeletedCaster(GetMagicCaster());
                }
            }
        }
    }
    if (pPlayer) {
        if (pPlayer->pCurrentProcess) {
            pPlayer->pCurrentProcess->RemoveHeadTrackTarget(this);
        }
        if (abMagicCaster) {
            MagicCaster* pCaster = GetMagicCaster();
            pPlayer->CheckDeletedCaster(pCaster);
        }
        if (pPlayer->pCurrentProcess) {
            MagicTarget* pDesiredTarget = pPlayer->pCurrentProcess->GetDesiredTarget();
            if (pDesiredTarget == GetMagicTarget()) {
                pPlayer->pCurrentProcess->SetDesiredTarget(nullptr);
            }
        }
        if (pPlayer && pPlayer->GetAutoAimActorSimple() == this) {
            pPlayer->ClearAutoAim();
        }
    }
}

void MobileObject::SetSoundCallBack(char acCallBack) {
    cSoundCallBackSet = acCallBack;
}

bool MobileObject::GetShouldUseEmotionAnimation() {
    return bUseEmotion;
}

void MobileObject::SetTalkingActivator(bool abFlag) {
    bTalkingActivator = abFlag;
}

void MobileObject::SetShouldUseEmotionAnimation(bool abFlag) {
    bUseEmotion = abFlag;
}

bool MobileObject::IsinDialogue() {
    TESPackage* pPackage = GetCurrentPackage();
    return pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE;
}

bool MobileObject::SetProcessLevel(PROCESS_TYPE aeLevel) {
    bool bChanged = false;
    if (aeLevel == PROCESS_TYPE_NONE) {
        ReferenceProcessLists.kUpdateLock.Lock(nullptr);
        if (pCurrentProcess) {
            bChanged = true;
            ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
            if (pCurrentProcess) {
                delete pCurrentProcess;
            }
            pCurrentProcess = nullptr;
        }
        ReferenceProcessLists.kUpdateLock.Unlock();
    }
    else {
        if (!pCurrentProcess) {
            pCurrentProcess = new LowProcess;
        }
        if (GetCurrentProcessType() != aeLevel) {
            switch (aeLevel) {
                case PROCESS_TYPE_HIGH: {
                    bChanged = MovetoHigh();
                    break;
                }
                case PROCESS_TYPE_MIDDLE_HIGH: {
                    bChanged = MovetoMiddleHigh();
                    break;
                }
                case PROCESS_TYPE_MIDDLE_LOW: {
                    bChanged = MovetoMiddleLow();
                    break;
                }
                case PROCESS_TYPE_LOW: {
                    bChanged = MovetoLow();
                    break;
                }
                default: break;
            }
        }
    }
    return bChanged;
}

// abAllowCopy is unused.
BSSoundHandle MobileObject::PlaySound(uint32_t aiSoundFormID, bool abLoop, uint32_t aiSoundType, bool abAllowCopy) {
    BSSoundHandle kSound;
    kSound = BSAudio::QInstance().GetSoundHandleByNumericID(aiSoundFormID, aiSoundType);
    if (TESObjectREFR::Get3D() && (aiSoundType & 2)) {
        NiPoint3 kPosition = GetLocationOnReference();
        kSound.SetPosition(kPosition.x, kPosition.y, kPosition.z);
        kSound.SetObjectToFollow(TESObjectREFR::Get3D());
    }
    kSound.Play(abLoop);
    return kSound;
}

BSSoundHandle MobileObject::PlaySound_0(char* apSoundFileName, bool abLoop, uint32_t aiSoundType) {
    BSSoundHandle soundHand;
    soundHand = BSAudio::QInstance().GetSoundHandleByFilename(apSoundFileName, aiSoundType, nullptr);
    if (Get3D() && (aiSoundType & 2) != 0) {
        NiPoint3 kPosition = GetLocationOnReference();
        soundHand.SetPosition(kPosition.x, kPosition.y, kPosition.z);
        soundHand.SetObjectToFollow(Get3D());
    }

    soundHand.Play(abLoop);
    return soundHand;
}

float MobileObject::GetActualHeading() {
    float fHeading = GetHeading(false);
    if (IsActor() && static_cast<Actor*>(this)->Actor::GetTurretBehavior()) {
        NiAVObject* p3D = Get3D();
        if (p3D) {
            NiAVObject* pYawBone = p3D->GetObjectByName(FixedStrings::QBip01Yaw());
            if (pYawBone) {
                bhkBlendCollisionObject* pBlendCol = static_cast<bhkBlendCollisionObject*>(
                    NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, pYawBone->GetCollisionObject())
                );
                if (pBlendCol) {
                    BSSimpleList<NiPointer<bhkSerializable>>* pActCon = &pBlendCol->GetRigidBody()->kActionConstrants;
                    while (pActCon && pActCon->m_item) {
                        bhkOrientHingedBodyAction* pOrientAction = static_cast<bhkOrientHingedBodyAction*>(
                            NiObject::DynamicCastStatic(&bhkOrientHingedBodyAction::ms_RTTI, pActCon->m_item)
                        );
                        pActCon = pActCon->m_pkNext;
                        if (pOrientAction) {
                            // The action's Havok object is its hkOrientHingedBodyAction.
                            hkOrientHingedBodyAction* pHkAction = static_cast<hkOrientHingedBodyAction*>(
                                pOrientAction->GethkObject()
                            );
                            fHeading = fHeading - pHkAction->fAngleDiff;
                            return ClampAngle(fHeading);
                        }
                    }
                }
            }
        }
    }
    return fHeading;
}

void MobileObject::SayToCallBack(void* apContext, bool abSucceeded) {
    // The context is the speaker's form ID.
    MobileObject* pActor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if (pActor && pActor->IsMobileObject()) {
        if (pActor->bDoNotRunSayToCallBack) {
            pActor->bDoNotRunSayToCallBack = false;
        }
        else {
            pActor->SetSoundCallBack(0);
            pActor->SetSoundCallBackSuccess(abSucceeded);
        }
    }
}

void MobileObject::DisplaySubTitleCallback(void* apContext, int32_t aiDuration) {
    TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)));
    if (pForm && pForm->IsMobileObject()) {
        MobileObject* pMobile = static_cast<MobileObject*>(pForm);
        if (pMobile) {
            pMobile->SetSoundCallBack(3);
        }
    }
}

void MobileObject::ChangeDialogueAnimation(void* apContext, bool abSucceeded) {
    // The callback's context holds the actor's form ID, cast from void* to uint32_t.
    MobileObject* pActor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if (pActor && pActor->IsMobileObject()) {
        if (pTalkingToPlayer) {
            pTalkingToPlayer->bTalkingToPlayer = false;
            pTalkingToPlayer->bSpeakingDone = true;
            if (pActor->IsTalkingActivator()) {
                TESObjectREFR* pRef = pActor->GetTalkingActivator();
                NiControllerManager* pManager = static_cast<NiControllerManager*>(
                    pRef->Get3D()->GetController(&NiControllerManager::ms_RTTI)
                );
                if (pManager) {
                    NiControllerSequence* pSeq = pManager->GetSequenceByName(FixedStrings::QTalking());
                    if (pSeq) {
                        Animation::SetLipTime(0.0f, pManager, pSeq);
                        pManager->DeactivateSequence(pSeq, 0.0f);
                    }
                }
                pActor->pCurrentProcess->SetCreatureLipSynchAnim(nullptr);
                pActor->pCurrentProcess->SetCreatureLipSynchStartTime(0);
            }
        }
        if (!abSucceeded) {
            DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
        }
    }

    pTalkingToPlayer = nullptr;
}

void MobileObject::DialogueConversationCallBack(void* apContext, bool abSucceeded) {
    MobileObject* pActor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if (pActor && pActor->IsMobileObject()) {
        pActor->SetSoundCallBack(1);
        pActor->SetSoundCallBackSuccess(abSucceeded);
    }
}

void MobileObject::SoundFileCallBack(void* apContext, bool abSucceeded) {
    MobileObject* pActor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if (pActor && pActor->IsMobileObject()) {
        pActor->bSoundFileDone = true;
        if (pActor->bTalkingToPlayer) {
            pActor->bSpeakingDone = true;
        }
        TESPackage* pPack = pActor->GetCurrentPackage();
        DialoguePackage* pDial = nullptr;
        if (pPack && pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            pDial = static_cast<DialoguePackage*>(pPack);
        }

        if (pDial) {
            if (pActor->bSoundFileDone) {
                pDial->flistentimer = 0.3f;
            }
            else {
                pDial->flistentimer = 20.0f;
            }
        }
    }
}

SayOnceTopicInfos::SayOnceTopicInfos() : pInfo(nullptr), iDateStamp(-1), fHourStamp(-1.0f) {}

SayOnceTopicInfos::SayOnceTopicInfos(TESTopicInfo* apInfo, int32_t aiDate, float afTime)
  : pInfo(apInfo), iDateStamp(aiDate), fHourStamp(afTime) {}

bool TESMagicCasterForm::CanBeMagicCaster(TESForm* apForm) {
    return dynamic_cast<TESMagicCasterForm*>(apForm) != nullptr;
}

DialogueItem* DialoguePackage::GetCurrentDialogItem() {
    return pCurrentItem;
}

NiFixedString& FixedStrings::QTalking() {
    return *FixedStrings::pTalking;
}
