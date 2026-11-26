#include "Fallout Shared/BGSOpenCloseForm.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bAnimateDoorPhysics("bAnimateDoorPhysics:General", true);

bool BGSOpenCloseForm::IsOpenCloseForm(TESForm* apForm) {
    bool bIsOpenClose = false;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case 21u:
            case 23u:
            case 27u:
            case 28u: {
                bIsOpenClose = true;
                break;
            }
            default: {
                break;
            }
        }
    }
    return bIsOpenClose;
}

bool BGSOpenCloseForm::SetOpenState(TESObjectREFR* apRef, bool abOpen, bool abSnap) {
    bool bplayedanims = false;
    if (apRef && BGSOpenCloseForm::IsOpenCloseForm(apRef->GetObjectReference())) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask() && apRef != TESObjectREFR::GetBackgroundLoadingRef()) {
            TaskQueueInterface::GetInstance()->QueueSetOpenState(apRef, abOpen, abSnap);
            return true;
        }
        if (abOpen) {
            apRef->SetAction(OBJECT_ACTION_OPEN);
        }
        else {
            apRef->ClearAction(OBJECT_ACTION_OPEN);
        }
        NiAVObject* ptopav = apRef->Get3D();
        NiTimeController* po = ptopav ? ptopav->GetControllers() : nullptr;
        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
        );
        if (pmanager) {
            NiControllerSequence* pseqtoplay = nullptr;
            NiControllerSequence* pseqtostop = nullptr;
            NiControllerSequence* popenseq = pmanager->GetSequenceByName(FixedStrings::QOpenString());
            NiControllerSequence* pcloseseq = pmanager->GetSequenceByName(FixedStrings::QCloseString());
            if (popenseq && pcloseseq) {
                if (abOpen) {
                    pseqtoplay = popenseq;
                    pseqtostop = pcloseseq;
                }
                else {
                    pseqtoplay = pcloseseq;
                    pseqtostop = popenseq;
                }
                pmanager->SetActive(true);
                if (abSnap) {
                    apRef->m_Extra.SetRagDollData(nullptr);
                    pmanager->DeactivateSequence(pseqtoplay, 0.0f);
                    pmanager->ActivateSequence(pseqtostop, 0, false, 1.0f, 0.0f, nullptr);
                    pseqtostop->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginKeyTime = pseqtostop->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginKeyTime, true, false);
                    NiNode* p3D = apRef->Get3D();
                    p3D->Update(kUpdateData);
                    pmanager->DeactivateSequence(pseqtostop, 0.0f);
                    const char* pcHandleString = pseqtoplay->m_kName.GetHandleString();
                    apRef->SetLastFinishedSequence(pcHandleString);
                    p3D = apRef->Get3D();
                    bhkNiCollisionObject::ResetSim(p3D, true);
                }
                else {
                    pmanager->DeactivateSequence(pseqtostop, 0.0f);
                    pmanager->ActivateSequence(pseqtoplay, 0, false, 1.0f, 0.0f, nullptr);
                    pseqtoplay->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginKeyTime = pseqtoplay->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginKeyTime, true, false);
                    NiNode* p3D = apRef->Get3D();
                    p3D->Update(kUpdateData);
                }
                bplayedanims = true;
            }
        }
        else if (abOpen) {
            apRef->SetLastFinishedSequence("Open");
        }
        else {
            apRef->SetLastFinishedSequence("Close");
        }
        if (abSnap &&
            bCutDoors.Bool() &&
            apRef->GetObjectReference()->GetFormType() == DOOR_ID &&
            apRef->Get3D() &&
            !apRef->GetTeleport()) {
            NavMeshObstacleManager* pObstacleManager = NavMeshObstacleManager::GetInstance();
            if (abOpen) {
                pObstacleManager->OnDoorOpen(apRef);
            }
            else {
                pObstacleManager->OnDoorClose(apRef);
            }
        }
    }
    return bplayedanims;
}

void BGSOpenCloseForm::HandleAnimEnd(TESObjectREFR* apAnimRef, NiControllerSequence* apEndSeq) {
    if (apAnimRef && apEndSeq) {
        BGSOpenCloseForm::HandleAnimEndHavok(apAnimRef, apEndSeq);
        const char* pcHandleString = apEndSeq->m_kName.GetHandleString();
        if (!std::strcmp(pcHandleString, "Open")) {
            Script::SetActionFlag(apAnimRef, &apAnimRef->m_Extra, SCRIPT_ACTION_ONOPEN);
            TESObjectREFR* pactionref = apAnimRef->m_Extra.GetOpenCloseActivateRef();
            if (pactionref) {
                BGSOpenCloseForm* pocform = BGSOpenCloseForm::GetAsOpenCloseForm(apAnimRef->GetObjectReference());
                if (pocform) {
                    pocform->HandleOpenFinish(apAnimRef, pactionref);
                }
                apAnimRef->m_Extra.SetOpenCloseActivateRef(nullptr);
                if (pactionref == pPlayer && pocform && pocform->GetBlockActivatesWhileOpening()) {
                    pPlayer->SetBlockActivate(false);
                }
            }
            if (bCutDoors.Bool() && apAnimRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueObstacleDoorOpen(apAnimRef);
                }
                else {
                    NavMeshObstacleManager::GetInstance()->OnDoorOpen(apAnimRef);
                }
            }
        }
        else {
            pcHandleString = apEndSeq->m_kName.GetHandleString();
            if (!std::strcmp(pcHandleString, "Close")) {
                Script::SetActionFlag(apAnimRef, &apAnimRef->m_Extra, SCRIPT_ACTION_ONCLOSE);
                TESObjectREFR* pactionref = apAnimRef->m_Extra.GetOpenCloseActivateRef();
                if (pactionref) {
                    BGSOpenCloseForm* pocform = BGSOpenCloseForm::GetAsOpenCloseForm(apAnimRef->GetObjectReference());
                    if (pocform) {
                        pocform->HandleCloseFinish(apAnimRef, pactionref);
                    }
                    apAnimRef->m_Extra.SetOpenCloseActivateRef(nullptr);
                }
                if (bCutDoors.Bool() && apAnimRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        TaskQueueInterface::GetInstance()->QueueObstacleDoorClose(apAnimRef);
                    }
                    else {
                        NavMeshObstacleManager::GetInstance()->OnDoorClose(apAnimRef);
                    }
                }
            }
        }
    }
}

// Handles the end of the open or close animation apEndSeq of apAnimRef. Reads the door physics setting twice and
// returns whether the sequence was Open or Close.
bool BGSOpenCloseForm::HandleAnimEndHavok(TESObjectREFR* apAnimRef, NiControllerSequence* apEndSeq) {
    bool bIsOpenOrClose = false;
    if (apAnimRef && apEndSeq) {
        if (!std::strcmp(apEndSeq->GetName().GetHandleString(), "Open") ||
            !std::strcmp(apEndSeq->GetName().GetHandleString(), "Close")) {
            bIsOpenOrClose = true;
        }
        if (!bAnimateDoorPhysics.Bool() && bIsOpenOrClose && !bAnimateDoorPhysics.Bool()) {
            NiNode* pNode = apAnimRef->Get3D();
            if (pNode) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueHavokAdd(pNode);
                }
                else {
                    TESObjectCELL* pParentCell = apAnimRef->GetParentCell();
                    bhkWorld* pWorld = pParentCell ? pParentCell->GetbhkWorld() : nullptr;
                    if (pWorld) {
                        pWorld->AddObjects(pNode, true, true, 0, false);
                    }
                }
            }
        }
    }

    return bIsOpenOrClose;
}

void BGSOpenCloseForm::SnapHavokTo3D(TESObjectREFR* apRef) {
    NiPointer<NiAVObject> sp3D(apRef->Get3D());
    if (sp3D.m_pObject) {
        bhkCollisionObject* pcolObj = bhkUtilFunctions::FindNextCollisionObject(sp3D.m_pObject);
        bool bprevMovementVel = pcolObj ? pcolObj->GetUseVelocity() : false;
        if (bprevMovementVel) {
            bhkUtilFunctions::SetUseVelocityRecurse(false, sp3D.m_pObject);
        }
        NiUpdateData updateData(0.0f, false, false);
        sp3D->Update(updateData);
        if (bprevMovementVel) {
            bhkUtilFunctions::SetUseVelocityRecurse(bprevMovementVel, sp3D.m_pObject);
        }
    }
}

BGSOpenCloseForm::OPEN_STATE BGSOpenCloseForm::GetOpenState(TESObjectREFR* apRef) {
    int32_t eState = 0;
    if (apRef) {
        TESBoundObject* pObjectReference = apRef->TESObjectREFR::GetObjectReference();
        if (BGSOpenCloseForm::IsOpenCloseForm(pObjectReference)) {
            if (apRef->GetAction(OBJECT_ACTION_OPEN)) {
                eState = 1;
            }
            else {
                eState = 3;
            }
            NiNode* p3D = apRef->Get3D();
            NiTimeController* pControllers = p3D ? p3D->NiObjectNET::GetControllers() : nullptr;
            NiControllerManager* pManager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
            );
            if (pManager) {
                BSAnimGroupSequence* pOpenSequence = static_cast<BSAnimGroupSequence*>(
                    pManager->GetSequenceByName(FixedStrings::QOpenString())
                );
                BSAnimGroupSequence* pCloseSequence = static_cast<BSAnimGroupSequence*>(
                    pManager->GetSequenceByName(FixedStrings::QCloseString())
                );
                if (pOpenSequence && pCloseSequence) {
                    if (pOpenSequence->NiControllerSequence::GetState() == NiControllerSequence::kAnimating) {
                        return BGSOpenCloseForm::OBJECT_OPENING;
                    }

                    if (pCloseSequence->NiControllerSequence::GetState() == NiControllerSequence::kAnimating) {
                        return BGSOpenCloseForm::OBJECT_CLOSING;
                    }
                }
            }
        }
    }
    return static_cast<BGSOpenCloseForm::OPEN_STATE>(eState);
}

void BGSOpenCloseForm::HandleActivate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    BGSOpenCloseForm* apOpenCloseForm
) {
    if (apItemActivated) {
        bool bhasanims = false;
        if (apItemActivated->Get3D() && apItemActivated->Get3D()->GetControllers()) {
            NiTimeController* po = apItemActivated->Get3D()->GetControllers();
            NiControllerManager* pmanager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
            );
            if (pmanager) {
                NiControllerSequence* popenSeq = pmanager->GetSequenceByName(FixedStrings::QOpenString());
                NiControllerSequence* pcloseSeq = pmanager->GetSequenceByName(FixedStrings::QCloseString());
                if (popenSeq && pcloseSeq) {
                    bhasanims = true;
                    if (popenSeq->GetState() != NiControllerSequence::kAnimating &&
                        pcloseSeq->GetState() != NiControllerSequence::kAnimating) {
                        NiControllerSequence* pplayThisSeq;
                        TESSound* soundThisSeq = nullptr;
                        TESObjectDOOR* pDoor = dynamic_cast<TESObjectDOOR*>(apOpenCloseForm);
                        if (apItemActivated->GetAction(OBJECT_ACTION_OPEN)) {
                            if (pDoor) {
                                soundThisSeq = pDoor->pCloseSound;
                            }
                            pplayThisSeq = pcloseSeq;
                            apItemActivated->ClearAction(OBJECT_ACTION_OPEN);
                        }
                        else {
                            if (pDoor) {
                                soundThisSeq = pDoor->pOpenSound;
                            }
                            pplayThisSeq = popenSeq;
                            apItemActivated->SetAction(OBJECT_ACTION_OPEN);
                        }
                        if (!bAnimateDoorPhysics.Bool() &&
                            apActionRef &&
                            apActionRef->IsActor() &&
                            apActionRef != pPlayer &&
                            apItemActivated->Get3D()) {
                            bool bremovehavok = true;
                            if (apItemActivated->GetObjectReference()->GetFormType() == DOOR_ID) {
                                TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(
                                    apItemActivated->GetObjectReference()
                                );
                                bremovehavok = !pdoor->IsSlidingDoor();
                            }
                            if (bremovehavok) {
                                NiAVObject* pav = apItemActivated->Get3D();
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    TaskQueueInterface* pTaskQueue = TaskQueueInterface::GetInstance();
                                    pTaskQueue->QueueHavokRemove(pav);
                                }
                                else {
                                    bhkWorld::Activate(pav, true, true, false);
                                    bhkWorld::RemoveObjects(pav, true, true);
                                }
                            }
                        }

                        pmanager->SetActive(true);
                        pmanager->ActivateSequence(pplayThisSeq, 0, false, 1.0f, 0.0f, nullptr);
                        pplayThisSeq->SetOffset(-std::numeric_limits<float>::max());
                        if (soundThisSeq) {
                            BSSoundHandle SoundHandle;
                            if (apActionRef == pPlayer) {
                                uint32_t uiSoundID = soundThisSeq->iFormID;
                                BSAudio* pAudio = &BSAudio::QInstance();
                                SoundHandle = pAudio->GetSoundHandleByNumericID(
                                    uiSoundID,
                                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                        BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_UNKBIT31
                                );
                            }
                            else {
                                uint32_t uiSoundID = soundThisSeq->iFormID;
                                BSAudio* pAudio = &BSAudio::QInstance();
                                SoundHandle = pAudio->GetSoundHandleByNumericID(uiSoundID, 258u);
                                NiPoint3 kLocation = apItemActivated->GetLocationOnReference();
                                SoundHandle.SetPosition(kLocation.x, kLocation.y, kLocation.z);
                            }
                            SoundHandle.Play(false);
                        }
                        apItemActivated->m_Extra.SetOpenCloseActivateRef(apActionRef);
                        apItemActivated->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                        if (pplayThisSeq == popenSeq && apActionRef == pPlayer) {
                            BGSOpenCloseForm* pocform = BGSOpenCloseForm::GetAsOpenCloseForm(
                                apItemActivated->GetObjectReference()
                            );
                            if (pocform && pocform->GetBlockActivatesWhileOpening()) {
                                pPlayer->SetBlockActivate(true);
                            }
                        }
                    }
                }
            }
        }

        if (!bhasanims) {
            bool bopen = false;
            BGSOpenCloseForm::OPEN_STATE eOpenState = BGSOpenCloseForm::GetOpenState(apItemActivated);
            if (eOpenState <= BGSOpenCloseForm::OBJECT_NONE || eOpenState > BGSOpenCloseForm::OBJECT_OPENING) {
                bopen = true;
                apItemActivated->SetAction(OBJECT_ACTION_OPEN);
            }
            else {
                apItemActivated->ClearAction(OBJECT_ACTION_OPEN);
            }
            BGSOpenCloseForm* pocform = BGSOpenCloseForm::GetAsOpenCloseForm(apItemActivated->GetObjectReference());
            if (pocform) {
                if (bopen) {
                    pocform->HandleOpenFinish(apItemActivated, apActionRef);
                }
                else {
                    pocform->HandleCloseFinish(apItemActivated, apActionRef);
                }
            }
        }
    }
}

BGSOpenCloseForm* BGSOpenCloseForm::GetAsOpenCloseForm(TESForm* apForm) {
    BGSOpenCloseForm* pretval = nullptr;
    if (apForm && BGSOpenCloseForm::IsOpenCloseForm(apForm)) {
        switch (apForm->GetFormType()) {
            case ACTI_ID:
            case TERM_ID: pretval = static_cast<TESObjectACTI*>(apForm); break;
            case CONT_ID: pretval = static_cast<TESObjectCONT*>(apForm); break;
            case DOOR_ID: pretval = static_cast<TESObjectDOOR*>(apForm); break;
            default: break;
        }
    }

    return pretval;
}

NiFixedString& FixedStrings::QOpenString() {
    return *FixedStrings::Open;
}

NiFixedString& FixedStrings::QCloseString() {
    return *FixedStrings::Close;
}
