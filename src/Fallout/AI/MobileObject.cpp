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

#include <cstdint>
#include <xmmintrin.h>

static NiPoint3 MobileObject_boundMin;
static NiPoint3 MobileObject_boundMax;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fDBVoiceAttenuationIn2D("fDBVoiceAttenuationIn2D:Audio", 0.0f);
SettingT<INISettingCollection> bBackgroundLoadLipFiles("bBackgroundLoadLipFiles:BackgroundLoad", false);
SettingT<INISettingCollection> fQuadrupedPitchMult("fQuadrupedPitchMult:HAVOK", 1.0f);
SettingT<INISettingCollection> gs_iHavokSkipFrameCountTEST_Havok("iHavokSkipFrameCountTEST:Havok", 0);

int32_t get_iFrameCounter() {
    return Main::iFrameCounter;
}

// iMoveDirection is deliberately left uninitialised, as the game does.
MoveData::MoveData()
  : fTime(0.0f), Angle(NiPoint3_ZERO), Displacement(NiPoint3_ZERO), fCurrMoveSpeed(0.0f), bVisible(true) {}

// fUpdateTargetTimer is deliberately left uninitialised, as the game does.
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
            PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
            ReferenceProcessLists.RemoveReference(this, ProcessLevel);
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
    SaveGameDataOLD(&ucLevel, 1);
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
// reference's data and the process's.
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
            }
            else {
                void* pMemory = ourMalloc(sizeof(HighProcess));
                pCurrentProcess = pMemory ? ::new (pMemory) HighProcess : nullptr;
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
                void* pMemory = ourMalloc(sizeof(MiddleHighProcess));
                pCurrentProcess = pMemory ? ::new (pMemory) MiddleHighProcess : nullptr;
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
                void* pMemory = ourMalloc(sizeof(MiddleLowProcess));
                pCurrentProcess = pMemory ? ::new (pMemory) MiddleLowProcess : nullptr;
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
                void* pMemory = ourMalloc(sizeof(LowProcess));
                pCurrentProcess = pMemory ? ::new (pMemory) LowProcess : nullptr;
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

// The guard masks both flags with 0, so the Set3D branch never runs; this is deliberate, as the game does.
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
        void* pMemory = ourMalloc(sizeof(LowProcess));
        LowProcess* pProcess = pMemory ? ::new (pMemory) LowProcess : nullptr;
        pCurrentProcess = pProcess;
        SetProcessLevel(GetDesiredProcessLevel());
    }
    if ((TESObjectREFR::Get3D()) &&
        (!pCurrentProcess || pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_LOW ||
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
    else if (aiFlags & 6) {
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
        }
        else if (
            !TESObjectREFR::Get3D() && this != pPlayer &&
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
            ReferenceProcessLists.kUpdateLock.Lock(nullptr);
            PROCESS_TYPE ProcessLevel = this->pCurrentProcess->eLevel;
            ReferenceProcessLists.RemoveReference(this, ProcessLevel);
            BaseProcess* pCurrentProcess = this->pCurrentProcess;
            if (pCurrentProcess) {
                delete pCurrentProcess;
            }
            ReferenceProcessLists.kUpdateLock.Unlock();
            this->pCurrentProcess = nullptr;
        }
        else {
            if (!TESObjectREFR::Get3D() && this != pPlayer &&
                (!this->pCurrentProcess->eLevel || this->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH)) {
                PROCESS_TYPE edesiredProcessLevel = GetDesiredProcessLevel();
                if (edesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW || edesiredProcessLevel == PROCESS_TYPE_LOW) {
                    if (pSaveLoadGame->GetSaveGameLoading()) {
                        if (edesiredProcessLevel == PROCESS_TYPE_MIDDLE_LOW) {
                            MovetoMiddleLow();
                        }
                        else {
                            MovetoLow();
                        }
                        float v5 = appCalendar.GetHour() - 0.5;
                        this->pCurrentProcess->SetLastTimeProcessed(v5);
                        ReferenceProcessLists.RemoveActorFromTempChangeList(this);
                    }
                    else {
                        ReferenceProcessLists.AddActorToTempChangeList(this);
                    }
                }
            }
        }
    }
    if (!bVoiceFileDone && !(&m_Extra)->GetSayToExtra()) {
        // The callback's context is the form ID.
        MobileObject::DialogueConversationCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), true);
    }
    if (pSaveLoadGame->GetSaveGameLoading() && !GetDisabledOrDeleted()) {
        PROCESS_TYPE edesiredProcessLevel_2 = GetDesiredProcessLevel();
        SetProcessLevel(edesiredProcessLevel_2);
    }
}

void MobileObject::SetDelete(bool abDeleted) {
    TESObjectREFR::SetDelete(abDeleted);
    // Every mobile object, projectiles included, is passed as the actor, as the game does.
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
            if ((pPackage) && (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
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
        if ((uiResponseID) && (pTopicInfo)) {
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
        if ((pTopicInfo) && (!pTopicInfo->IsRunImmediately())) {
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
            if ((pFaceData) && (IsActor())) {
                MobileObject* pMobile = this;
                if (BSFaceGenManager::GetUseEmotions()) {
                    if (uiEmotion) {
                        int32_t iExpression =
                            BSFaceGenManager::GetExpressionFromEmotion(static_cast<DIALOGUE_EMOTION>(uiEmotion));
                        float fAmount = static_cast<float>(iEmotionValue / 100.0);
                        pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
                    }
                    else {
                        int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(pMobile))
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
                if ((gs_bGeneralSubtitles_Gameplay.Bool() && pText) ||
                    (IsActor() && static_cast<Actor*>(this)->GetShowSubtitles())) {
                    if (GetIsFullHelp()) {
                        char acSubtitle[0x100];
                        std::sprintf(
                            acSubtitle, "'%s' :%s", GetFullName(), pSubtitleItem->GetCurrentResponse()->cText.String()
                        );
                        BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                        Interface::ShowSubtitle(acSubtitle, kHandle, GetLocationOnReference(), pPlayer, false);
                    }
                    else {
                        BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                        Interface::ShowSubtitle(
                            const_cast<char*>(pSubtitleItem->GetCurrentResponse()->cText.String()), kHandle,
                            GetLocationOnReference(), pPlayer, false
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
            else if ((pProcessPackage) && (pProcessPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
                pDialoguePackage = static_cast<DialoguePackage*>(pProcessPackage);
            }

            if ((pFaceData) && (IsActor())) {
                MobileObject* pMobile = this;
                if (BSFaceGenManager::GetUseEmotions()) {
                    if (uiEmotion) {
                        int32_t iExpression =
                            BSFaceGenManager::GetExpressionFromEmotion(static_cast<DIALOGUE_EMOTION>(uiEmotion));
                        float fAmount = static_cast<float>(iEmotionValue / 100.0);
                        pFaceData->SetExpressionTarget(static_cast<BS_FACEGEN_EXPRESSION>(iExpression), fAmount);
                    }
                    else {
                        int32_t iMood = static_cast<ActorValueOwner*>(static_cast<Actor*>(pMobile))
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
                    pDialoguePackage->GetActorStartedConversation(), pDialoguePackage->GetTargetOfConversation(),
                    pDialoguePackage
                );
                if (pDialoguePackage == pPlayer->pClosestConversation || ReferenceProcessLists.bShowSubtitle) {
                    Conversation* pConversation = pDialoguePackage->GetPackConversation();
                    if ((pConversation) && (pConversation->GetCurrentItem()) &&
                        (pConversation->GetCurrentItem()->GetCurrentResponse())) {
                        const char* pText = pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String();
                        if ((pText) &&
                            (gs_bGeneralSubtitles_Gameplay.Bool() ||
                             (IsActor() && static_cast<Actor*>(this)->GetShowSubtitles()))) {
                            BSSoundHandle kHandle = GetCurrentAIProcess()->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                            Interface::ShowSubtitle(
                                const_cast<char*>(pText), kHandle, GetLocationOnReference(), pPlayer, false
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
    Actor* pactor = static_cast<Actor*>(this);
    RemoveFromAllWater(false);
    if ((pactor->pCurrentProcess) &&
        (!pactor->pCurrentProcess->eLevel || pactor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_HIGH)) {
        TESObjectCELL* pcell = pactor->GetParentCell();
        HighProcess* pBaseProc =
            pactor->pCurrentProcess->eLevel ? nullptr : static_cast<HighProcess*>(pactor->pCurrentProcess);
        if (!pcell || (pcell->GetAttached() || pcell->IsAttaching())) {
            bool v40;
            bhkWorld* pw;
            if (pcell) {
                pw = pcell->GetbhkWorld();
            }
            else {
                pw = nullptr;
            }
            Actor* pactor_2 = pactor->IsActor() ? pactor : nullptr;
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
            }
            else {
                v40 = false;
            }
            ExtraDismemberedLimbs* pdismemberedLimbs = (&pactor->m_Extra)->GetDismembermentExtra();
            if ((pdismemberedLimbs) && (pactor->pCurrentProcess) && (pactor->Get3D())) {
                uint32_t uilimbCount = pdismemberedLimbs->GetDismemberedLimbCount();
                for (uint32_t ui = 0; ui < uilimbCount; ++ui) {
                    DismemberedLimb* plimb = pdismemberedLimbs->GetDismemberedLimb(ui);
                    NiNode* plimbRoot =
                        pactor->pCurrentProcess->GetDamageNode(static_cast<BGSBodyPart::LIMB_ENUM>(plimb->cLimb));
                    NiNode* plimbParent = plimbRoot->m_pkParent;
                    NiFixedString dismemberRootName(pLimbEnumNames[plimb->cLimb]);
                    if (!plimbParent || !NiFixedString::HandlesEqual(plimbParent->GetName(), dismemberRootName)) {
                        for (uint32_t i = 0; i < plimbRoot->GetArrayCount(); ++i) {
                            NiAVObject* pchild = plimbRoot->GetAt(i);
                            if (pchild) {
                                bhkWorld::SetDisabled(pchild, true, true, true);
                            }
                        }
                    }
                }
            }

            bhkCharacterController* pcon = pactor->GetCharController();
            if (!pcon) {
                bhkCharacterController* pcon_2;
                bhkCharacterControllerCinfo info(true);
                info.pObject = pactor->TESObjectREFR::Get3D();
                info.SetPosition(*pactor->QLocationOnReference());
                info.SetWorld(nullptr);
                info.SetAcrobatics(pactor->GetAcrobatics());
                if (pactor_2 && (pactor->GetObjectReference() && pactor_2->IsCreature())) {
                    bool TiltFrontBack =
                        static_cast<TESCreature*>(pactor_2->GetObjectReference())->TESCreature::GetTiltFrontBack();
                    info.SetPitch(TiltFrontBack);
                    info.SetRoll(
                        static_cast<TESCreature*>(pactor_2->GetObjectReference())->TESCreature::GetTiltLeftRight()
                    );
                    info.SetScale(pactor->GetScale());
                }
                else {
                    info.SetUseDefaultShape(true);
                }
                bhkCharacterController* v28 = static_cast<bhkCharacterController*>(
                    bhkRagdollController::operator new(sizeof(bhkCharacterController))
                );
                if (v28) {
                    pcon_2 = ::new (v28) bhkCharacterController(info);
                }
                else {
                    pcon_2 = nullptr;
                }
                pcon = pcon_2;
                if ((pactor_2) && (pactor->GetObjectReference())) {
                    bool NotPushable = pactor->GetActorBase()->GetNotPushable();
                    pcon->SetFlag(
                        bhkCharacterController::NOTPUSHABLE_PERMANENT | bhkCharacterController::NOTPUSHABLE, NotPushable
                    );
                }

                // The by-value NiPointer argument is released by the callee.
                pactor->pCurrentProcess->SetCharController(pcon);
                pcon->SetMOBObject(pactor);
                pcon->SetScale(pactor->GetScale());
                bool IsComplex = Main::GetSceneIsComplex();
                pcon->SetQuickSimulate(IsComplex);
                pcon->SetFlag(bhkCharacterController::RECORD_HITS, true);
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
            if ((pw) && (pw->IsBatchingAdds())) {
                pcon->SetWaitForBatchAdd(true);
            }

            if ((pactor_2) && (v40)) {
                // MobileObject::GetCollisionFilter writes the filter word; the group is read from the filter it
                // returns.
                CFilter kActorFilter;
                pactor_2->GetCollisionFilter(&kActorFilter.iFilter);
                uint32_t uiGroup = kActorFilter.GetGroup();
                NiNode* v9 = pactor_2->Get3D();
                pactor_2->pPenetrationDetection->Initialize(v9, static_cast<int32_t>(uiGroup));
            }

            NiAVObject* poldAV = pcon->GetNiObject();
            NiAVObject* pav = pactor->TESObjectREFR::Get3D();
            if (poldAV != pav) {
                bhkWorld* pw_3;
                pcon->SetNiObject(pav);
                NiColorA color(0.0f, 0.0f, 0.0f, 0.0f);
                color.r = 1.0f;
                color.g = 0.0f;
                color.b = 0.0f;
                color.a = 0.25f;
                TESObjectCELL* pcell_2 = pactor->GetParentCell();
                if (pcell_2) {
                    pw_3 = pcell_2->GetbhkWorld();
                }
                else {
                    pw_3 = nullptr;
                }
                if ((pw_3) && (pw_3->QDebugDisplay())) {
                    pcon->BuildDisplayGeometry(nullptr);
                    NiAVObject* poldAV_2 = pcon->GetNiObject();
                    BSShaderUtil::RecursiveSetPropertyWireframe(poldAV_2, true);
                }

                bhkWorld::SetMotion(
                    pav, v40 ? hkpMotion::MOTION_DYNAMIC : hkpMotion::MOTION_KEYFRAMED, true, false, false
                );
                if ((pactor_2) && (!pactor_2->IsDead(false))) {
                    pactor_2->CreateRagdollInstance(pav, true, true, false);
                    if ((pactor == pPlayer) && (pactor_2->pRagdollController)) {
                        pactor_2->pRagdollController->SetUsePhantomFootIKCasts(false);
                        pactor_2->pRagdollController->SetUseFootEndCast(true);
                    }
                }
            }
            if ((pactor->pCurrentProcess) && (!pactor->pCurrentProcess->eLevel && pactor != pPlayer) && (pactor_2) &&
                (pactor_2->GetActorBase()) && (!pactor_2->GetTurretBehavior())) {
                HighProcess* phigh = static_cast<HighProcess*>(pactor->pCurrentProcess);
                if (!phigh->CheckPhantom()) {
                    phigh->SetupPhantom(pactor_2);
                }
            }

            pactor->MoveHavok(false);
            if (v40 && !(&pactor->m_Extra)->GetSavedHavokData() && !(&pactor->m_Extra)->GetRagDollData()) {
                NiPoint3 vel(0.0f, 0.0f, -1.0f);
                TESHavokUtilities::AddVelocity(pav, vel, true);
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
            // Actor::DoesFly is called directly on every mobile object, projectiles included, as the game does. The
            // sentinel is -FLT_MAX as a double, compared with the unrounded height; the test fails on unequal or
            // unordered values.
            if (static_cast<Actor*>(this)->DoesFly() && pController->GetState() == HK_CHARACTER_USER_STATE_0 &&
                GetRelevantWaterHeight() == -3.4028234663852886e+38 && pLoadedData) {
                const NiPoint3* pPosition = &GetLocationOnReference();
                pLoadedData->fRelevantWaterHeight = TES::pInstance->GetWaterHeight(*pPosition, nullptr);
            }
        }
    }
}

bool MobileObject::MovetoHigh() {
    HighProcess* v2;
    PROCESS_TYPE aWhichList = pCurrentProcess->eLevel;
    ReferenceProcessLists.kUpdateLock.Lock(nullptr);
    ReferenceProcessLists.RemoveReference(this, aWhichList);
    HighProcess* apMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
    if (apMemory) {
        v2 = ::new (apMemory) HighProcess;
    }
    else {
        v2 = nullptr;
    }
    v2->Copy(pCurrentProcess);
    if (pCurrentProcess) {
        delete pCurrentProcess;
    }
    pCurrentProcess = v2;
    ReferenceProcessLists.AddReference(this, PROCESS_TYPE_HIGH, false, false, nullptr);
    ReferenceProcessLists.kUpdateLock.Unlock();
    MoveHavok(false);
    pCurrentProcess->SetLastSeenLocation();
    return true;
}

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
    Actor* pactor = IsActor() ? static_cast<Actor*>(this) : nullptr;
    bhkCharacterController* pcon = pactor_2->GetCharController();
    bool bIsProjectile = pactor_2->IsProjectile();
    if ((pactor) && (pactor->GetActorValue(ActorValue::TURBO) > 0)) {
        afDeltaTime = afDeltaTime / BSTimer::QGlobalTimeMultiplier();
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
             (fanglez == 3.4028234663852886e+38 || (std::isnan(fanglez) || !std::isfinite(fanglez))))))))) {
        int32_t CastingType = pactor_2->iFormID;
        Warning(
            "AI: MobileObject::Move called on '%s' (%08X) with invalid angle.", pactor_2->GetFormEditorID(), CastingType
        );
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
                Heading = pactor_2->MobileObject::GetHeading(false);
            }
            else {
                Heading = pactor_2->GetHeading(false);
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
            v23 = pactor_2->MobileObject::GetHeading(false);
        }
        else {
            v23 = pactor_2->GetHeading(false);
        }
        rot_2.MakeZRotation(v23);
        NiMatrix3* v8 = rot_2.Inverse(&v82);
        arDeltaMove.operator+=(*::new (&largeDelta_2) NiPoint3(*v8 * largeDelta));
    }
    if (pactor) {
        bisImmobile = pactor->IsImmobile();
    }
    else {
        bisImmobile = false;
    }
    SIT_SLEEP_STATE eSitSleepState = pactor_2->GetSitSleepState();
    SIT_SLEEP_STATE v78 = static_cast<SIT_SLEEP_STATE>(
        eSitSleepState &&
        (pcon &&
         (X_4 = &hkVector4::getZero(),
          (SupportNormal = &pcon->GetSupportNormal(), SupportNormal->equals4(*X_4, 0.001f))))
    );
    bool v77 = v78;
    int32_t v76 = eSitSleepState == SIT_SLEEP_NORMAL || v77;
    bool v75 = v76;
    const NiPoint3* WorldTranslate = &Main::WorldRootCamera()->GetWorldTranslate();
    const NiPoint3* v10 = &pactor_2->GetLocationOnReference();
    NiPoint3 diff = *v10 - *WorldTranslate;
    float v73 = diff.SqrLength();
    if (pactor) {
        int32_t v72 =
            pactor->IsUnConscious() || (pactor->IsDead(false) || (pactor->IsDying() || pactor->IsKnockedOut()));
        v71 = v72;
    }
    else {
        v71 = 0;
    }
    bool v70 = v71;
    if (!(gs_fCharControllerWarpDistSqr_HAVOK.Float() < static_cast<double>(v73)) ||
        (pactor_2->IsProjectile() ||
         (bisImmobile ||
          (v70 ||
           (pactor_2 == pPlayer ||
            ((tmpLoc = pos, (tmpMove = arDeltaMove, pactor_2 != pPlayer))
                 ? v25 = pactor_2->GetHeading(false)
                 : v25 = pactor_2->MobileObject::GetHeading(false),
             (rot_3.MakeZRotation(v25),
              (tmpMove = rot_3 * tmpMove,
               (&tmpLoc.operator+=(tmpMove),
                (pws = pactor_2->GetWorldSpace(),
                 (pc = pactor_2->GetParentCell(),
                  !Pathing::GetNavMeshZForLocation(tmpLoc, pc, pws, fheightNavMesh)))))))))))) {
        if (v75) {
            if (pactor_2 == pPlayer) {
                pcon->SetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS, bisImmobile);
            }
            if (!pcon || pactor_2->IsDead(true)) {
                float v30;
                NiMatrix3 rot_5;
                if (pactor_2->IsProjectile()) {
                    float Looking = pactor_2->GetLooking();
                    rot_5.MakeXRotation(Looking);
                    arDeltaMove = rot_5 * arDeltaMove;
                }
                if (pactor_2 == pPlayer) {
                    v30 = pactor_2->MobileObject::GetHeading(false);
                }
                else {
                    v30 = pactor_2->GetHeading(false);
                }
                rot_5.MakeZRotation(v30);
                arDeltaMove = rot_5 * arDeltaMove;
                pos.operator+=(arDeltaMove);
            }
            else {
                md.~MoveData();
                ::new (&md) MoveData;
                md.fTime = afDeltaTime;
                md.Angle = pactor_2->GetAngleOnReference();
                md.Displacement = arDeltaMove;
                md.iMoveDirection = aiMoveDir;
                md.bVisible = true;
                if ((pactor_2 != pPlayer) && (pactor)) {
                    bool isNotFadedOut;
                    BSFadeNode* pfadeNode;
                    NiAVObject* p3d = (NiAVObject*)pactor_2->Get3D();
                    if (p3d) {
                        pfadeNode = p3d->IsFadeNode();
                    }
                    else {
                        pfadeNode = nullptr;
                    }
                    if (pfadeNode) {
                        NiCamera* v13 = Main::WorldRootCamera();
                        isNotFadedOut = pfadeNode->ShouldUpdate(v13, 0.8f);
                    }
                    else {
                        isNotFadedOut = true;
                    }
                    md.bVisible = isNotFadedOut;
                }

                if (pcon->QChaseBip() && !v77) {
                    float v27;
                    NiPoint3 oldPos;
                    if (pactor_2 == pPlayer) {
                        v27 = pactor_2->MobileObject::GetHeading(false);
                    }
                    else {
                        v27 = pactor_2->GetHeading(false);
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
                    pcon->SetStuckQuadruped(false);
                    float* v15 = &fQuadrupedPitchMult.Float();
                    float v45 = appTimer.fDelta * *v15;
                    pcon->SetPitchMult(v45);
                }
                if (pactor && (pactor->GetMoveMode() & 63) != 0) {
                    md.fCurrMoveSpeed = pactor->GetCurrentSpeed();
                }
                else {
                    md.fCurrMoveSpeed = 0.0f;
                }
                if (bisImmobile && (pactor_2 == pPlayer || !pactor->HasRagDoll())) {
                    pcon->MoveImmobile(md, pactor_2 == pPlayer);
                }
                else {
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
        }
        else {
            if (pactor_2->IsDead(false) && !bisImmobile) {
                float v31;
                NiMatrix3 rot_6;
                if (pactor_2 == pPlayer) {
                    v31 = pactor_2->MobileObject::GetHeading(false);
                }
                else {
                    v31 = pactor_2->GetHeading(false);
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
            md_2.bVisible = true;
            if (!v77) {
                pcon->MoveImmobile(md_2, false);
            }
            if (v77) {
                pcon->SetSupported(true);
                pcon->SetTrueState(HK_CHARACTER_ON_GROUND);
            }
            if (pactor_2 == pPlayer) {
                v32 = pactor_2->MobileObject::GetHeading(false);
            }
            else {
                v32 = pactor_2->GetHeading(false);
            }
            rot_7.MakeZRotation(v32);
            arDeltaMove = rot_7 * arDeltaMove;
            pos.operator+=(arDeltaMove);
        }
        pactor_2->SetLocationOnReference(pos);
        return pcon;
    }

    if (pcon && pcon->GetState() == HK_CHARACTER_USER_STATE_0) {
        if (pactor_2->pLoadedData->fRelevantWaterHeight == -3.4028234663852886e+38) {
            const NiPoint3* v11 = &pactor_2->GetLocationOnReference();
            double WaterHeight = TES::pInstance->GetWaterHeight(*v11, nullptr);
            pactor_2->pLoadedData->fRelevantWaterHeight = WaterHeight;
        }
        float fheightWater = pactor_2->GetRelevantWaterHeight();
        if (fheightWater == -3.4028234663852886e+38) {
            if (pcon) {
                if (pactor_2->DoesFly()) {
                    pcon->SetWantState(HK_CHARACTER_IN_AIR);
                }
                else {
                    pcon->SetWantState(HK_CHARACTER_ON_GROUND);
                }
                tmpLoc.z = fheightNavMesh;
            }
        }
        else {
            if (!(fheightWater < static_cast<double>(tmpLoc.z))) {
                if (fheightNavMesh > static_cast<double>(tmpLoc.z) &&
                    fheightWater > static_cast<double>(fheightNavMesh)) {
                    tmpLoc.z = fheightNavMesh;
                }
            }
            else {
                tmpLoc.z = fheightWater;
            }
        }
    }
    else {
        if (eSitSleepState == SIT_SLEEP_NORMAL && !v77) {
            tmpLoc.z = fheightNavMesh;
        }
    }
    if ((pcon) && (pcon->GetWorld()) &&
        (!static_cast<uint8_t>(pcon->QFarAway()) && (pcon->QCanPitch() || (pcon->QCanPitch() || pcon->QQuadruped())))) {
        hkVector4 supNorm;
        alignas(16) NiPoint3 hitNormal;
        PathingLocation loc(pactor_2);
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

    pactor_2->SetPosition(tmpLoc);
    if ((pcon) && (pcon->GetState() == HK_CHARACTER_IN_AIR)) {
        pcon->SetTrueState(HK_CHARACTER_ON_GROUND);
    }

    return pcon;
}

// The jump height is fJumpHeightMin scaled by the reference's scale, times fJumpSwimmingMult when swimming. The setting
// is looked up before the scale is read.
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
    bool v4 = true;
    if (Get3D()) {
        TESObjectCELL* ParentCell = GetParentCell();
        if (TES::pInstance->IsCellLoaded(ParentCell, true)) {
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
    return v4;
}

void MobileObject::UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    Animation* panim = GetAnimation();
    DialoguePackage* pdia = nullptr;
    BaseProcess* CurrentAIProcess = pCurrentProcess;
    TESPackage* pbasepack = CurrentAIProcess->GetCurrentPackage();
    if ((pbasepack) && (pbasepack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
        pdia = static_cast<DialoguePackage*>(pbasepack);
    }

    bool bPlayIdle = false;
    bool btalkflag = false;
    if ((pResponse) && (pCurrentProcess) && (bSpeakingDone)) {
        BSSoundHandle phan;
        btalkflag = true;
        pTalkingToPlayer = static_cast<Actor*>(this);
        bTalkingToPlayer = true;
        bool bnosound = true;
        if (pResponse->pVoiceSound) {
            uint32_t isoundFileID = pResponse->pVoiceSound->iFormID;
            BSSoundHandle handle;
            if (GetTalkingActivator()) {
                TESObjectREFR* pref = GetTalkingActivator();
                BSAudio* v4 = &BSAudio::QInstance();
                BSSoundHandle* arHandle = v4->GetSoundHandleByNumericID(&a1, isoundFileID, 261u);
                handle.operator=(*arHandle);
                handle.Play(false);
            }
            else {
                BSSoundHandle* v17 = PlaySound(&a2, isoundFileID, false, 0x40000102u, true);
                handle.operator=(*v17);
            }
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            // The callback's context is the speaker's form ID.
            mgr->SetCompletionCallback(
                static_cast<uint32_t>(handle.iSoundID), &MobileObject::SoundFileCallBack,
                reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
            );
            bnosound = false;
            bSoundFileDone = false;
            bSpeakingDone = false;
        }
        if (bSoundFileDone && bnosound) {
            PlayerCharacter* v10 = pPlayer;
            TESIdleForm* ListenerIdle = pResponse->pListenIdle;
            TESIdleForm* SpeakerIdle = pResponse->pSpeakerIdle;
            uint32_t Length = pResponse->cText.GetLength();
            uint32_t v6 = pResponse->uiEmotion;
            DIALOGUE_EMOTION FormFlags = pResponse->eEmotion;
            float fdelay = SpeakSoundFunction(
                pResponse->GetVoice()->pString, phan, FormFlags, v6, Length, SpeakerIdle, ListenerIdle, v10, true, true,
                false, true, true
            );
            BSAudioManager* mgr_2 = &BSAudioManager::QInstance();
            mgr_2->SetCompletionCallback(
                static_cast<uint32_t>(phan.iSoundID), &MobileObject::ChangeDialogueAnimation,
                reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
            );
            bVoiceFileDone = false;
            BaseProcess* v16 = pCurrentProcess;
            v16->SetSoundDelay(fdelay);
        }
    }

    if (!abChangeAnimationEndVoice) {
        UpdateAnimation(afDelta);
    }
    UpdateSoundCallBack();
    if (!abChangeAnimationEndVoice) {
        Update(afDelta);
    }
    if ((btalkflag) && (pdia)) {
        pdia->pTalkingActor = this;
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
                        }
                        else {
                            Animation::SetLipTime(0.0f, pmanager, pseq);
                            pmanager->DeactivateSequence(pseq, 0.0f);
                            BaseProcess* v10 = pCurrentProcess;
                            v10->SetCreatureLipSynchAnim(nullptr);
                            BaseProcess* v9 = pCurrentProcess;
                            v9->SetCreatureLipSynchStartTime(0);
                        }
                    }
                }
            }
            pTalkingActivator->UpdateAnimation();
            float afTime = static_cast<double>(appTimer.iLastTime) / 1000.0;
            NiUpdateData updateData(afTime, true, false);
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
        if ((v5) && (v5->NiAVObject::GetParent()) && (v6) && (v4)) {
            parent = v5->NiAVObject::GetParent();
            parent->SetSelectiveUpdate(true);
            v6->SetSelectiveUpdate(true);
            v4->SetSelectiveUpdate(true);
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
    PackageTarget* pnew;
    PackageLocation* ploc;
    DialoguePackage* ppack;
    MobileObject* ptalkactivator;
    if (GetCurrentProcessType() || apTarget->GetCurrentProcessType()) {
        return false;
    }
    BaseProcess* CurrentAIProcess = pCurrentProcess;
    TESPackage* pTempPack = CurrentAIProcess->GetCurrentPackage();
    Conversation* pConversation = nullptr;
    if (!apTarget ||
        (apTarget->GetSitSleepState() &&
         (apTarget->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
          apTarget->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING))) {
        return false;
    }
    MobileObject* pSpeakAs = this;
    ptalkactivator = nullptr;
    if (IsTalkingActivator()) {
        pSpeakAs = SetUpTalkingActivatorActor(apTarget, ptalkactivator);
    }
    if (apTarget != pPlayer) {
        Conversation* pConversation_2;
        Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
        if (apMemory) {
            pConversation_2 = ::new (apMemory) Conversation(pSpeakAs, apTarget, pTopic);
        }
        else {
            pConversation_2 = nullptr;
        }
        pConversation = pConversation_2;
    }
    StopCurrentDialogue();
    pCurrentProcess->ClearRunOncePackage();
    if (apTarget != pPlayer && !pConversation->FirstItem()) {
        return false;
    }
    BaseProcess* v20 = pCurrentProcess;
    v20->SavePackageToExtraData(this);
    BaseProcess* v19 = pCurrentProcess;
    v19->ClearActionHeadTrackTarget(true);
    BaseProcess* v18 = pCurrentProcess;
    v18->SetDialogHeadTrackTarget(apTarget);
    BaseProcess* v17 = apTarget->pCurrentProcess;
    v17->ClearActionHeadTrackTarget(true);
    BaseProcess* v16 = apTarget->pCurrentProcess;
    v16->SetDialogHeadTrackTarget(this);
    DialoguePackage* v26 = static_cast<DialoguePackage*>(ourMalloc(sizeof(DialoguePackage)));
    if (v26) {
        ppack = ::new (v26) DialoguePackage(
            pConversation, static_cast<Actor*>(this), static_cast<Actor*>(apTarget),
            static_cast<Actor*>(ptalkactivator), true, true
        );
    }
    else {
        ppack = nullptr;
    }
    ppack->ptopic = pTopic;
    if (pTempPack) {
        ppack->CopyAllFlags(pTempPack);
    }
    ppack->SetPackType(PACKAGE_TYPE_IN_GAME_DIALOGUE);
    ppack->SetMustReachLocation(true);
    ppack->SetMustComplete(true);
    PackageLocation* v25 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
    if (v25) {
        ploc = ::new (v25) PackageLocation;
    }
    else {
        ploc = nullptr;
    }
    ploc->SetLocReference(apTarget);
    ppack->SetPackageLocation(ploc);
    if (ploc) {
        delete ploc;
    }
    PackageTarget* v24 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
    if (v24) {
        pnew = ::new (v24) PackageTarget;
    }
    else {
        pnew = nullptr;
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
    }
    else {
        ppack->GetPackageTarget()->iValue = 90u;
    }
    BaseProcess* v12 = pCurrentProcess;
    v12->ComputeLastTimeProcessed();
    PutCreatedPackage(ppack, false, true);
    if (bflag) {
        BaseProcess* v11 = pCurrentProcess;
        v11->AddToProcedureIndexRunning(this, 1);
    }
    return true;
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
        }
        else {
            v19 = nullptr;
        }
        a2 = v19;
    }
    StopCurrentDialogue();
    pCurrentProcess->ClearRunOncePackage();
    if ((static_cast<const void*>(pTarget) != static_cast<const void*>(pPlayer)) && (!a2->FirstItem())) {
        return false;
    }
    if (!pDialoguePackage->GetNoHeadTrack()) {
        v18 = MobileObject::GetCurrentAIProcess();
        v18->ClearActionHeadTrackTarget(true);
        v17 = MobileObject::GetCurrentAIProcess();
        v17->SetDialogHeadTrackTarget(pTarget);
        v16 = pTarget->MobileObject::GetCurrentAIProcess();
        v16->ClearActionHeadTrackTarget(true);
        v15 = pTarget->MobileObject::GetCurrentAIProcess();
        v15->SetDialogHeadTrackTarget(this);
    }
    void* v22 = ourMalloc(sizeof(DialoguePackage));
    if (v22) {
        pIdles = pDialoguePackage->pIdleCollection;
        pNewPackage = ::new (v22)
            DialoguePackage(a2, static_cast<Actor*>(this), pTarget, nullptr, false, (uint8_t)(pIdles == nullptr));
    }
    else {
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
    PutCreatedPackage(pNewPackage, false, true);
    SetDialogueItemTarget(pTarget);
    pCurrentProcess->SetTarget(pTarget);
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    pCurrentProcess->BaseProcess::ComputeLastTimeProcessed();
    if ((static_cast<const void*>(pTarget) == static_cast<const void*>(pPlayer)) ||
        (static_cast<const void*>(pTarget) == static_cast<const void*>(this))) {
        if (!pTarget->IsDead(false)) {
            v11 = MobileObject::GetCurrentAIProcess();
            v11->AddToProcedureIndexRunning(static_cast<Actor*>(this), 3u);
        }
    }
    else {
        v12 = pTarget->MobileObject::GetCurrentAIProcess();
        v12->SavePackageToExtraData(pTarget);
        currentPackage = GetCurrentPackage();
        pTarget->PutCreatedPackage(currentPackage, false, true);
        v9 = pTarget->MobileObject::GetCurrentAIProcess();
        v9->SetTarget(pTarget);
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
    FNVD_INLINED_LOCAL(BSSoundHandle, kVoiceHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    if (IsTalkingActivator()) {
        float fdelay = 0.0f;
        BSStringT<char> tempstring(apSoundFile);
        BaseProcess* CurrentAIProcess = this->pCurrentProcess;
        LipSynchAnim* panim = CurrentAIProcess->GetLipAnim();
        BSFaceGenAnimationData* panimdata = nullptr;
        this->SetSpeakingEmotion(aeEmotiontype);
        this->SetSpeakingEmotionValue(static_cast<int32_t>(aiEmotionValue));
        if (apSoundFile) {
            char Dst[516];
            BSstrcpy(Dst, 512, apSoundFile);
            this->StopCurrentDialogue();
            bool bisPlayer = this == pPlayer;
            float fSoundDuration = 0.0f;
            bool v50 = this == pPlayer || ab2D;
            double DistanceFromReference = this->GetDistanceFromReference(pPlayer, false, false);
            if (static_cast<double>(iSpeakSoundLipDistance.Int()) > DistanceFromReference &&
                (!bisPlayer || pPlayer->Is3rdPerson())) {
                panimdata = this->GetFaceAnimationData(nullptr);
            }
            if (!panim) {
                BaseProcess* v48;
                if (!abLip || (v48 = this->pCurrentProcess, v48->GetLipFileFailed())) {
                    BSFaceGenAnimationData* panimdata_2 = this->GetFaceAnimationData(nullptr);
                    if (panimdata_2) {
                        panimdata_2->AddAnimationDataFromNote("BigAah 0.9 0.2 0.1 0.2");
                    }
                    BaseProcess* v44 = this->pCurrentProcess;
                    if (v44->GetLipFileFailed()) {
                        // The callback's context is the speaker's form ID.
                        MobileObject::DisplaySubTitleCallback(
                            reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), 0
                        );
                        BaseProcess* v43 = this->pCurrentProcess;
                        v43->SetLipFileFailed(false);
                    }
                }
                else {
                    if (LipSynchAnim::ChangeFilenameToLipSynch(tempstring)) {
                        if (abQueue) {
                            BaseProcess* v47 = this->pCurrentProcess;
                            v47->SetWaitingforLipFile(true);
                            BaseProcess* v46 = this->pCurrentProcess;
                            v46->SetLipQuequed(true);
                            ReferenceProcessLists.kLipSyncManager.BuildLipSynicFile(this, tempstring.pString);
                            return 0.0f;
                        }
                        panim = LipSynchAnim::LoadFromFile(tempstring);
                        if (!panim) {
                            DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
                        }
                        BaseProcess* v45 = this->pCurrentProcess;
                        v45->SetWaitingforLipFile(false);
                    }
                }
            }
            if (this->GetAnimation()) {
                if (apResponseSpeakerIdle || abUseIdleManager) {
                    this->pCurrentProcess->SetupSpecialIdle(
                        this, apResponseSpeakerIdle,
                        apResponseSpeakerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO, false,
                        true, true
                    );
                }
                if ((apTarget) && (apTarget != this && (apTarget->IsMobileObject() || apTarget->IsActor()))) {
                    Actor* ptargact = static_cast<Actor*>(apTarget);
                    bool v62 = true;
                    if (apTarget->IsActor()) {
                        TESPackage* ptargpack = ptargact->GetCurrentPackage();
                        if (ptargpack) {
                            v62 = ptargpack->pIdleCollection == nullptr;
                        }
                    }
                    if (apResponseListenerIdle || v62) {
                        BaseProcess* v42 = ptargact->pCurrentProcess;
                        v42->SetupSpecialIdle(
                            ptargact, apResponseListenerIdle,
                            apResponseListenerIdle ? AnimIdle::PLAY_TYPE_AUTO_EXCLUSIVE : AnimIdle::PLAY_TYPE_AUTO,
                            false, true, true
                        );
                    }
                }
            }
            if (panim) {
                int64_t v41 = panim->iNumFrames;
                fSoundDuration = static_cast<double>(v41) / 30.0;
                panim->ApplyToNode(panimdata, 0.2f);
            }
            if ((abSetEmotion) && (this->pCurrentProcess)) {
                BaseProcess* v40 = this->pCurrentProcess;
                v40->SetLastSpeakingEmotion(aeEmotiontype);
            }

            uint32_t isoundFlags = BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_VOICE;
            TESObjectREFR* ptalkingactivator = this->GetTalkingActivator();
            if ((ptalkingactivator) && (ptalkingactivator->GetObjectReference())) {
                [[maybe_unused]] BGSTalkingActivator* ptalker =
                    static_cast<BGSTalkingActivator*>(ptalkingactivator->GetObjectReference());
            }

            if (v50) {
                isoundFlags |= BSGameSound::SOUND_FLAG_IS_2D;
            }
            else {
                isoundFlags |= BSGameSound::SOUND_FLAG_IS_3D;
            }
            BSAudio* v16 = &BSAudio::QInstance();
            BSSoundHandle* SoundHandleByFilename = v16->GetSoundHandleByFilename(&out, Dst, isoundFlags, nullptr);
            aSoundHand.operator=(*SoundHandleByFilename);
            BSAudioManager* mgr = &BSAudioManager::QInstance();
            MobileObject* pact = this;
            if ((apTarget) && (apTarget->IsActor())) {
                pact = static_cast<MobileObject*>(apTarget);
            }

            // The subtitle callback's context is the listener's (or the speaker's) form ID.
            mgr->SetPlayCallback(
                static_cast<uint32_t>(aSoundHand.iSoundID), &MobileObject::DisplaySubTitleCallback,
                reinterpret_cast<void*>(static_cast<uintptr_t>(pact->iFormID))
            );
            if (v50) {
                double v18 = fDBVoiceAttenuationIn2D.Float() * 100.0;
                aSoundHand.SetStaticAttenuation(static_cast<int32_t>(v18));
            }
            else {
                NiPoint3 afZ = this->GetLocationOnReference();
                // Reads the maximum first.
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
                    }
                    else {
                        double v34 = static_cast<double>(aiStringLength);
                        float* v19 = &fAIDefaultSpeechMult.Float();
                        fdelay = *v19 * v34 + fdelay;
                    }
                }
                else {
                    fdelay = fdelay + fSoundDuration;
                }
                BaseProcess* v33 = this->pCurrentProcess;
                int32_t v20 = static_cast<int32_t>(fplaydelay);
                uint32_t Time = appTimer.iLastTime;
                v33->SetCreatureLipSynchStartTime(v20 + Time);
                BaseProcess* v32 = this->pCurrentProcess;
                v32->SetCreatureLipSynchAnim(panim);
            }
            else {
                aSoundHand.PlayAfter(1u, 0);
                double v31 = static_cast<double>(aiStringLength);
                fdelay = fAIDefaultSpeechMult.Float() * v31;
            }
            if (this->bTalkingToPlayer) {
                this->bSpeakingDone = false;
            }
            BaseProcess* v30 = this->pCurrentProcess;
            kVoiceHandle.operator=(aSoundHand);
            v30->SetSoundHandle(ACTOR_SOUND_HANDLE_VOICE, kVoiceHandle);
            if (panim) {
                BaseProcess* v28 = this->pCurrentProcess;
                LipSynchAnim* v22 = v28->GetCreatureLipSynchAnim();
                if ((panim != v22) && (panim)) {
                    delete panim;
                }

                BaseProcess* v26 = this->pCurrentProcess;
                v26->SetLipAnim(nullptr);
                BaseProcess* v25 = this->pCurrentProcess;
                v25->SetWaitingforLipFile(false);
            }
            return fdelay;
        }

        return 0.0f;
    }

    return 0.0f;
}

void MobileObject::EndDialogue() {
    if (pCurrentProcess) {
        TESPackage* v1;
        pCurrentProcess->SetSubtitleItem(nullptr);
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
                    CurrentAIProcess->ClearDialogHeadTrackTarget(true);
                    pSubject->bVoiceFileDone = true;
                }
                if (pTarget) {
                    if (pTarget->IsActor()) {
                        ReferenceProcessLists.kLipSyncManager.CancelLipFile(pTarget);
                    }
                    BaseProcess* v17 = pTarget->pCurrentProcess;
                    v17->ClearDialogHeadTrackTarget(true);
                    pTarget->bVoiceFileDone = true;
                }
                if ((pSubject) && (pTarget)) {
                    BaseProcess* v16 = pSubject->pCurrentProcess;
                    v16->SetDefaultHeadTrackTarget(pTarget);
                    BaseProcess* v15 = pTarget->pCurrentProcess;
                    v15->SetDefaultHeadTrackTarget(pSubject);
                    pSubject->fUpdateTargetTimer = 1.0f;
                    pTarget->fUpdateTargetTimer = 1.0f;
                    BaseProcess* v14 = pSubject->pCurrentProcess;
                    v14->SetLipFileFailed(false);
                    BaseProcess* v13 = pTarget->pCurrentProcess;
                    v13->SetLipFileFailed(false);
                }

                Animation* v24 = pSubject->GetAnimation();
                if (v24) {
                    v24->SpecialIdleFree(true, false);
                }
                Animation* v25 = pTarget->GetAnimation();
                if (v25) {
                    v25->SpecialIdleFree(true, false);
                }
                if ((pSubject) && (pSubject->pCurrentProcess)) {
                    BaseProcess* v12 = pSubject->pCurrentProcess;
                    v12->SetLastSpeakingEmotion(DE_NEUTRAL);
                }

                if ((pTarget) && (pTarget->pCurrentProcess)) {
                    BaseProcess* v11 = pTarget->pCurrentProcess;
                    v11->SetLastSpeakingEmotion(DE_NEUTRAL);
                }

                pSubject->GetCurrentPackage();
                pTarget->GetCurrentPackage();
                BaseProcess* v10 = pSubject->pCurrentProcess;
                v10->LoadPackageFromExtraData(pSubject);
                if (pSubject->IsActor()) {
                    BaseProcess* v9 = pSubject->pCurrentProcess;
                    if (!v9->GetCurrentPackage() && !pDataHandler->GetClearingData()) {
                        BaseProcess* v8 = pSubject->pCurrentProcess;
                        v8->CheckforNewPackage(pSubject, true);
                    }
                }
                if ((pTarget) && (pTarget->IsinDialogue()) && (pTarget->pCurrentProcess)) {
                    BaseProcess* v7 = pTarget->pCurrentProcess;
                    v7->LoadPackageFromExtraData(pTarget);
                    if (!pDataHandler->GetClearingData() && pTarget->IsActor()) {
                        BaseProcess* v6 = pTarget->pCurrentProcess;
                        v6->CheckforNewPackage(pTarget, true);
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
        }
        else {
            BaseProcess* v5 = pCurrentProcess;
            TESObjectREFR* v20 = v5->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_DIALOG);
            BaseProcess* v4 = pCurrentProcess;
            v4->ClearDialogHeadTrackTarget(true);
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
        this->pCurrentProcess->SetCharController(
            NiPointer<bhkCharacterController>(static_cast<bhkCharacterController*>(nullptr))
        );
    }
    return bres;
}

void MobileObject::SetPosition(const NiPoint3& arPos) {
    SetLocationOnReference(arPos);
    auto* pController = static_cast<bhkCharacterController*>(GetCharController());
    if (pController) {
        pController->SetPosition(arPos);
        pController->SetLinearVelocity(hkVector4::getZero());
        // Both getters are called and their results discarded, as the game does.
        pController->QFallStartHeight();
        pController->QHitDamage();
        pController->ClearManifold();
    }
    if (pLoadedData && pLoadedData->m_spData3D.operator->()) {
        pLoadedData->m_spData3D.operator->()->SetTranslate(arPos);
    }
}

void MobileObject::GetLinearVelocity(NiPoint3& arVel) {
    bhkCharacterController* CharController = GetCharController();
    if (CharController) {
        CharController->GetLinearVelocity(arVel);
    }
    else {
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
    float v8 = (float)((v7.z + 0x1p+7f) + 30.0);
    if ((v9 < ((double)(&aPoint)->z)) && (v8 > ((double)(&aPoint)->z))) {
        v6.z = (float)0.0;
    }
    if (abExpandRadius) {
        afRadius = (afRadius + 0x1p+5f);
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
            pCharacterController->BuildDisplayGeometry(nullptr);
            NiAVObject* NiObject_ = pCharacterController->GetNiObject();
            BSShaderUtil::RecursiveSetPropertyWireframe(NiObject_, 1);
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
    BGSTalkingActivator* pactitalk = static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference());
    if (pactitalk->GetTempRef()) {
        return pactitalk->GetTempRef();
    }

    return this;
}

void MobileObject::AddSayOnceTopicInfo(TESTopicInfo* apTopicInfo) {
    SayOnceTopicInfos* v2;
    uint32_t aiDate = iGameYear << 13 | (LowProcess::cCurrentGameDay | LowProcess::eCurrentGameMonth << 9);
    SayOnceTopicInfos* apMemory = static_cast<SayOnceTopicInfos*>(ourMalloc(sizeof(SayOnceTopicInfos)));
    if (apMemory) {
        v2 = ::new (apMemory) SayOnceTopicInfos(apTopicInfo, aiDate, fGameHour);
    }
    else {
        v2 = nullptr;
    }
    MobileObject* TalkingActivator = this;
    if (IsTalkingActivator()) {
        TalkingActivator = static_cast<MobileObject*>(GetTalkingActivator());
    }
    (&TalkingActivator->m_Extra)->SetExtraSayTopicInfoOnceADayExtra(v2);
    TalkingActivator->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

void MobileObject::StopCurrentDialogue() {
    char v6 = 0;
    BaseProcess* pproc = pCurrentProcess;
    bool v3 = false;
    if (pproc) {
        BSSoundHandle th_s = pproc->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        v6 = 1;
        if (th_s.IsValid()) {
            v3 = true;
        }
    }
    if ((v6 & 1) != 0) {}
    if (v3) {
        TESTopicInfo* pinfo = GetSayToTopicInfo();
        TESTopic* ptopic = GetSayToTopic();
        SetSayToTopic(nullptr);
        (&m_Extra)->RemoveSayToInfoExtra();
        if (pinfo) {
            if (!pinfo->IsRunImmediately()) {
                pinfo->RunResult(TIRS_END, this);
            }
            ExtraDataList* Extra = &m_Extra;
            Script::SetActionFlag(ptopic, Extra, SCRIPT_ACTION_ONSAYTODONE);
            bDoNotRunSayToCallBack = true;
            BaseProcess* pproc_2 = pCurrentProcess;
            pproc_2->SetDoingSayTo(false);
        }
        cSoundCallBackSet = -1;
        pproc->StopSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        pproc->SetGreetingFlag(false);
        pPlayer->ResetPlayerGreetFlag();
    }
    bVoiceFileDone = true;
    bSoundFileDone = true;
    if ((!IsTalkingActivator()) && (Get3D())) {
        Actor* pactor = static_cast<Actor*>(this);
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

BSSoundHandle* MobileObject::PlaySoundByEditorName(
    BSSoundHandle* apResult,
    const char* apEditorName,
    bool abLoop,
    uint32_t aiSoundType,
    bool abAllowCopy
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSSoundHandle soundHand;
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
    auto* pController = GetCharController();
    if (pController) {
        pFilter = &pController->GetCollisionFilter(&kControllerFilter)->iFilter;
    }
    else {
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
    }
    else {
        pExtraData = GetExtra()->GetExtraSayTopicInfoOnceADayExtra();
    }
    if (!pExtraData) {
        return;
    }
    BSSimpleList<SayOnceTopicInfos*>* pIter = pExtraData->pListofSaidOnceTopicInfos;
    // GetItem is called once for the test and again for the item, as the game does.
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
        // NaN hours take the "not past 23" paths, as the game does.
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
    if ((pNode && (!pNode->GetHighDetail() || abForceRecheck)) && (CheckHighDetail())) {
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
                    v16->SetDesiredTarget(nullptr);
                }
            }
            else {
                if (pprojectile__pexplosion__pactor->IsProjectile()) {
                    static_cast<Projectile*>(pprojectile__pexplosion__pactor)->Projectile::CheckRefDeleted(this);
                }
                else {
                    if (pprojectile__pexplosion__pactor->IsExplosion()) {
                        static_cast<Explosion*>(pprojectile__pexplosion__pactor)->Explosion::CheckRefDeleted(this);
                    }
                    else {
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
                    v14->SetDesiredTarget(nullptr);
                }
            }
            else {
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
                v11->SetDesiredTarget(nullptr);
            }
        }
        if ((pPlayer) && (pPlayer->GetAutoAimActorSimple() == this)) {
            pPlayer->ClearAutoAim();
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

bool MobileObject::SetProcessLevel(PROCESS_TYPE aeLevel) {
    bool v7 = false;
    if (aeLevel == PROCESS_TYPE_NONE) {
        ReferenceProcessLists.kUpdateLock.Lock(nullptr);
        if (pCurrentProcess) {
            v7 = true;
            PROCESS_TYPE ProcessLevel = pCurrentProcess->eLevel;
            ReferenceProcessLists.RemoveReference(this, ProcessLevel);
            if (pCurrentProcess) {
                delete pCurrentProcess;
            }
            pCurrentProcess = nullptr;
        }
        ReferenceProcessLists.kUpdateLock.Unlock();
    }
    else {
        if (!pCurrentProcess) {
            LowProcess* v4;
            LowProcess* apMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
            if (apMemory) {
                v4 = ::new (apMemory) LowProcess;
            }
            else {
                v4 = nullptr;
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

// The handle GetSoundHandleByNumericID fills is a temporary constructed in place (no constructor call), then assigned
// and destroyed; the result is assigned into the caller's memory. abAllowCopy is unused.
BSSoundHandle* MobileObject::PlaySound(
    BSSoundHandle* apResult,
    uint32_t aiSoundFormID,
    bool abLoop,
    uint32_t aiSoundType,
    bool abAllowCopy
) {
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

BSSoundHandle*
MobileObject::PlaySound_0(BSSoundHandle* apResult, char* apSoundFileName, bool abLoop, uint32_t aiSoundType) {
    FNVD_INLINED_LOCAL(BSSoundHandle, out, std::nullopt, std::nullopt);
    BSSoundHandle soundHand;
    BSAudio* v5 = &BSAudio::QInstance();
    BSSoundHandle* arHandle = v5->GetSoundHandleByFilename(&out, apSoundFileName, aiSoundType, nullptr);
    soundHand.operator=(*arHandle);
    if ((Get3D()) && ((aiSoundType & 2) != 0)) {
        NiPoint3 v11 = GetLocationOnReference();
        soundHand.SetPosition(v11.x, v11.y, v11.z);
        NiNode* v6 = Get3D();
        soundHand.SetObjectToFollow(v6);
    }

    soundHand.Play(abLoop);
    apResult->operator=(soundHand);
    return apResult;
}

float MobileObject::GetActualHeading() {
    float fheading = GetHeading(false);
    if (IsActor() && static_cast<Actor*>(this)->Actor::GetTurretBehavior()) {
        NiAVObject* p3d = Get3D();
        if (p3d) {
            NiAVObject* pyawBone = p3d->GetObjectByName(FixedStrings::QBip01Yaw());
            if (pyawBone) {
                bhkBlendCollisionObject* pblendCol = static_cast<bhkBlendCollisionObject*>(
                    NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, pyawBone->GetCollisionObject())
                );
                if (pblendCol) {
                    BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &pblendCol->GetRigidBody()->kActionConstrants;
                    while (pactCon && pactCon->m_item) {
                        bhkOrientHingedBodyAction* porientAction = static_cast<bhkOrientHingedBodyAction*>(
                            NiObject::DynamicCastStatic(&bhkOrientHingedBodyAction::ms_RTTI, pactCon->m_item)
                        );
                        pactCon = pactCon->m_pkNext;
                        if (porientAction) {
                            // The action's Havok object is its hkOrientHingedBodyAction.
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
    MobileObject* pactor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if ((pactor) && (pactor->IsMobileObject())) {
        if (pactor->bDoNotRunSayToCallBack) {
            pactor->bDoNotRunSayToCallBack = false;
        }
        else {
            pactor->SetSoundCallBack(0);
            pactor->SetSoundCallBackSuccess(abSucceeded);
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
    MobileObject* pactor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if ((pactor) && (pactor->IsMobileObject())) {
        if (pTalkingToPlayer) {
            pTalkingToPlayer->bTalkingToPlayer = false;
            pTalkingToPlayer->bSpeakingDone = true;
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
                CurrentAIProcess->SetCreatureLipSynchAnim(nullptr);
                BaseProcess* v4 = pactor->pCurrentProcess;
                v4->SetCreatureLipSynchStartTime(0);
            }
        }
        if (!abSucceeded) {
            DialogMenu::fUseTimerNoVoice = fDialogSpeechDelaySeconds.Float();
        }
    }

    pTalkingToPlayer = nullptr;
}

void MobileObject::DialogueConversationCallBack(void* apContext, bool abSucceeded) {
    MobileObject* pactor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if ((pactor) && (pactor->IsMobileObject())) {
        pactor->SetSoundCallBack(1);
        pactor->SetSoundCallBackSuccess(abSucceeded);
    }
}

void MobileObject::SoundFileCallBack(void* apContext, bool abSucceeded) {
    MobileObject* pactor = static_cast<MobileObject*>(
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apContext)))
    );
    if ((pactor) && (pactor->IsMobileObject())) {
        pactor->bSoundFileDone = true;
        if (pactor->bTalkingToPlayer) {
            pactor->bSpeakingDone = true;
        }
        TESPackage* ppack = pactor->GetCurrentPackage();
        DialoguePackage* pdial = nullptr;
        if ((ppack) && (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
            pdial = static_cast<DialoguePackage*>(ppack);
        }

        if (pdial) {
            if (pactor->bSoundFileDone) {
                pdial->flistentimer = 0.3f;
            }
            else {
                pdial->flistentimer = 20.0f;
            }
        }
    }
}

SayOnceTopicInfos::SayOnceTopicInfos() : pInfo(nullptr), iDateStamp(-1), fHourStamp(-1.0f) {}

SayOnceTopicInfos::SayOnceTopicInfos(TESTopicInfo* apInfo, int32_t aiDate, float afTime)
  : pInfo(apInfo), iDateStamp(aiDate), fHourStamp(afTime) {}

bool TESMagicCasterForm::CanBeMagicCaster(TESForm* apForm) {
    return (dynamic_cast<TESMagicCasterForm*>(apForm) != nullptr);
}

DialogueItem* DialoguePackage::GetCurrentDialogItem() {
    return pCurrentItem;
}

NiFixedString& FixedStrings::QTalking() {
    return *FixedStrings::pTalking;
}
