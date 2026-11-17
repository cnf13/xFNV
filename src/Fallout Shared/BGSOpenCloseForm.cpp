#include "Fallout Shared/BGSOpenCloseForm.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bAnimateDoorPhysics("bAnimateDoorPhysics:General", true);

bool BGSOpenCloseForm::IsOpenCloseForm(TESForm* apForm) {
    bool v2 = false;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case 21u:
            case 23u:
            case 27u:
            case 28u: {
                v2 = true;
                break;
            }
            default: {
                return v2;
            }
        }
    }
    return v2;
}

bool BGSOpenCloseForm::SetOpenState(TESObjectREFR* apRef, bool abOpen, bool abSnap) {
    bool bplayedanims = false;
    if (apRef && BGSOpenCloseForm::IsOpenCloseForm(apRef->GetObjectReference())) {
        NiTimeController* po;
        if (AILinearTaskThreadManager::ShouldQueue3DTask() && apRef != TESObjectREFR::GetBackgroundLoadingRef()) {
            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
            Instance->QueueSetOpenState(apRef, abOpen, abSnap);
            return true;
        }
        if (abOpen) {
            apRef->SetAction(OBJECT_ACTION_OPEN);
        }
        else {
            apRef->ClearAction(OBJECT_ACTION_OPEN);
        }
        NiAVObject* ptopav = apRef->Get3D();
        if (ptopav) {
            po = ptopav->GetControllers();
        }
        else {
            po = nullptr;
        }
        NiObject* pkObject = po;
        NiControllerManager* pmanager =
            static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po));
        if (pmanager) {
            NiControllerSequence* pseqtoplay = nullptr;
            NiControllerSequence* pseqtostop = nullptr;
            NiFixedString* v5 = &FixedStrings::QOpenString();
            NiControllerSequence* popenseq = pmanager->GetSequenceByName(*v5);
            NiFixedString* v6 = &FixedStrings::QCloseString();
            NiControllerSequence* pcloseseq = pmanager->GetSequenceByName(*v6);
            if ((popenseq) && (pcloseseq)) {
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
                    (&apRef->m_Extra)->SetRagDollData(nullptr);
                    pmanager->DeactivateSequence(pseqtoplay, 0.0f);
                    pmanager->ActivateSequence(pseqtostop, 0, false, 1.0f, 0.0f, nullptr);
                    pseqtostop->SetOffset(-3.4028235e+38f);
                    float a4 = pseqtostop->GetBeginKeyTime();
                    NiUpdateData UpdateData(a4, true, false);
                    NiNode* v7 = apRef->Get3D();
                    v7->Update(UpdateData);
                    pmanager->DeactivateSequence(pseqtostop, 0.0f);
                    const char* HandleString = (&pseqtoplay->m_kName)->GetHandleString();
                    apRef->SetLastFinishedSequence(HandleString);
                    NiNode* v9 = apRef->Get3D();
                    bhkNiCollisionObject::ResetSim(v9, true);
                }
                else {
                    pmanager->DeactivateSequence(pseqtostop, 0.0f);
                    pmanager->ActivateSequence(pseqtoplay, 0, false, 1.0f, 0.0f, nullptr);
                    pseqtoplay->SetOffset(-3.4028235e+38f);
                    float a4a = pseqtoplay->GetBeginKeyTime();
                    NiUpdateData UpdateData_2(a4a, true, false);
                    NiNode* v10 = apRef->Get3D();
                    v10->Update(UpdateData_2);
                }
                bplayedanims = true;
            }
        }
        else {
            if (abOpen) {
                apRef->SetLastFinishedSequence("Open");
            }
            else {
                apRef->SetLastFinishedSequence("Close");
            }
        }
        if ((abSnap && bCutDoors.Bool() && apRef->GetObjectReference()->GetFormType() == DOOR_ID) && (apRef->Get3D()) &&
            (!apRef->GetTeleport())) {
            NavMeshObstacleManager* v11 = NavMeshObstacleManager::GetInstance();
            if (abOpen) {
                v11->OnDoorOpen(apRef);
            }
            else {
                v11->OnDoorClose(apRef);
            }
        }
    }
    return bplayedanims;
}

void BGSOpenCloseForm::HandleAnimEnd(TESObjectREFR* apAnimRef, NiControllerSequence* apEndSeq) {
    if ((apAnimRef) && (apEndSeq)) {
        BGSOpenCloseForm::HandleAnimEndHavok(apAnimRef, apEndSeq);
        const char* HandleString = (&apEndSeq->m_kName)->GetHandleString();
        if (!std::strcmp(HandleString, "Open")) {
            Script::SetActionFlag(apAnimRef, &apAnimRef->m_Extra, SCRIPT_ACTION_ONOPEN);
            TESObjectREFR* pactionref = (&apAnimRef->m_Extra)->GetOpenCloseActivateRef();
            if (pactionref) {
                BGSOpenCloseForm* pocform = BGSOpenCloseForm::GetAsOpenCloseForm(apAnimRef->GetObjectReference());
                if (pocform) {
                    pocform->HandleOpenFinish(apAnimRef, pactionref);
                }
                (&apAnimRef->m_Extra)->SetOpenCloseActivateRef(nullptr);
                if ((pactionref == pPlayer) && (pocform) && (pocform->GetBlockActivatesWhileOpening())) {
                    pPlayer->SetBlockActivate(false);
                }
            }
            if (bCutDoors.Bool() && apAnimRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                    Instance->QueueObstacleDoorOpen(apAnimRef);
                }
                else {
                    NavMeshObstacleManager* v4 = NavMeshObstacleManager::GetInstance();
                    v4->OnDoorOpen(apAnimRef);
                }
            }
        }
        else {
            const char* v5 = (&apEndSeq->m_kName)->GetHandleString();
            if (!std::strcmp(v5, "Close")) {
                Script::SetActionFlag(apAnimRef, &apAnimRef->m_Extra, SCRIPT_ACTION_ONCLOSE);
                TESObjectREFR* pactionref_2 = (&apAnimRef->m_Extra)->GetOpenCloseActivateRef();
                if (pactionref_2) {
                    BGSOpenCloseForm* pocform_2 = BGSOpenCloseForm::GetAsOpenCloseForm(apAnimRef->GetObjectReference());
                    if (pocform_2) {
                        pocform_2->HandleCloseFinish(apAnimRef, pactionref_2);
                    }
                    (&apAnimRef->m_Extra)->SetOpenCloseActivateRef(nullptr);
                }
                if (bCutDoors.Bool() && apAnimRef->GetObjectReference()->GetFormType() == DOOR_ID) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        TaskQueueInterface* v6 = TaskQueueInterface::GetInstance();
                        v6->QueueObstacleDoorClose(apAnimRef);
                    }
                    else {
                        NavMeshObstacleManager* v7 = NavMeshObstacleManager::GetInstance();
                        v7->OnDoorClose(apAnimRef);
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
    if ((apAnimRef) && (apEndSeq)) {
        if (!std::strcmp(apEndSeq->GetName().GetHandleString(), "Open") ||
            !std::strcmp(apEndSeq->GetName().GetHandleString(), "Close")) {
            bIsOpenOrClose = true;
        }
        if ((!(&bAnimateDoorPhysics)->Bool()) && (bIsOpenOrClose) && (!(&bAnimateDoorPhysics)->Bool())) {
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
        bool bprevMovementVel;
        bhkCollisionObject* pcolObj = bhkUtilFunctions::FindNextCollisionObject(sp3D.m_pObject);
        if (pcolObj) {
            bprevMovementVel = pcolObj->GetUseVelocity();
        }
        else {
            bprevMovementVel = false;
        }
        if (bprevMovementVel) {
            bhkUtilFunctions::SetUseVelocityRecurse(false, sp3D.m_pObject);
        }
        NiUpdateData updateData(0.0f, false, false);
        sp3D.m_pObject->Update(updateData);
        if (bprevMovementVel) {
            bhkUtilFunctions::SetUseVelocityRecurse(bprevMovementVel, sp3D.m_pObject);
        }
    }
}

BGSOpenCloseForm::OPEN_STATE BGSOpenCloseForm::GetOpenState(TESObjectREFR* apRef) {
    TESBoundObject* objectReference;
    NiFixedString* g_Open;
    NiFixedString* g_Close;
    NiTimeController* controllers;
    BSAnimGroupSequence* pOpenSequence;
    BSAnimGroupSequence* pCloseSequence;
    NiNode* v8;
    NiControllerManager* v9;
    int32_t eState = 0;
    if (apRef) {
        objectReference = apRef->TESObjectREFR::GetObjectReference();
        if (BGSOpenCloseForm::IsOpenCloseForm(objectReference)) {
            if (apRef->GetAction(OBJECT_ACTION_OPEN)) {
                eState = 1;
            }
            else {
                eState = 3;
            }
            v8 = apRef->Get3D();
            if (v8) {
                controllers = v8->NiObjectNET::GetControllers();
            }
            else {
                controllers = nullptr;
            }
            v9 = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, controllers)
            );
            if (v9) {
                g_Open = &FixedStrings::QOpenString();
                pOpenSequence = static_cast<BSAnimGroupSequence*>(v9->GetSequenceByName(*g_Open));
                g_Close = &FixedStrings::QCloseString();
                pCloseSequence = static_cast<BSAnimGroupSequence*>(v9->GetSequenceByName(*g_Close));
                if ((pOpenSequence) && (pCloseSequence)) {
                    if (((int32_t)pOpenSequence->NiControllerSequence::GetState()) == 1) {
                        return BGSOpenCloseForm::OBJECT_OPENING;
                    }

                    if (((int32_t)pCloseSequence->NiControllerSequence::GetState()) == 1) {
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
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    if (apItemActivated) {
        bool bhasanims = false;
        if ((apItemActivated->Get3D()) && (apItemActivated->Get3D()->GetControllers())) {
            NiTimeController* po = apItemActivated->Get3D()->GetControllers();
            NiControllerManager* pmanager =
                static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po));
            if (pmanager) {
                NiFixedString* v3 = &FixedStrings::QOpenString();
                NiControllerSequence* popenSeq = pmanager->GetSequenceByName(*v3);
                NiFixedString* v4 = &FixedStrings::QCloseString();
                NiControllerSequence* pcloseSeq = pmanager->GetSequenceByName(*v4);
                if ((popenSeq) && (pcloseSeq)) {
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
                        if ((!bAnimateDoorPhysics.Bool()) && (apActionRef) &&
                            (apActionRef->IsActor() && apActionRef != pPlayer) && (apItemActivated->Get3D())) {
                            bool bremovehavok = true;
                            if (apItemActivated->GetObjectReference()->GetFormType() == DOOR_ID) {
                                TESObjectDOOR* pdoor =
                                    static_cast<TESObjectDOOR*>(apItemActivated->GetObjectReference());
                                bremovehavok = !pdoor->IsSlidingDoor();
                            }
                            if (bremovehavok) {
                                NiAVObject* pav = apItemActivated->Get3D();
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                                    Instance->QueueHavokRemove(pav);
                                }
                                else {
                                    bhkWorld::Activate(pav, true, true, false);
                                    bhkWorld::RemoveObjects(pav, true, true);
                                }
                            }
                        }

                        pmanager->SetActive(true);
                        pmanager->ActivateSequence(pplayThisSeq, 0, false, 1.0f, 0.0f, nullptr);
                        pplayThisSeq->SetOffset(-3.4028235e+38f);
                        if (soundThisSeq) {
                            BSSoundHandle SoundHandle;
                            if (apActionRef == pPlayer) {
                                uint32_t a3 = soundThisSeq->iFormID;
                                BSAudio* v6 = &BSAudio::QInstance();
                                BSSoundHandle* arHandle = v6->GetSoundHandleByNumericID(
                                    &apHandle, a3,
                                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                        BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_UNKBIT31
                                );
                                SoundHandle.operator=(*arHandle);
                            }
                            else {
                                uint32_t a3a = soundThisSeq->iFormID;
                                BSAudio* v7 = &BSAudio::QInstance();
                                BSSoundHandle* SoundHandleByNumericID = v7->GetSoundHandleByNumericID(&th_s, a3a, 258u);
                                SoundHandle.operator=(*SoundHandleByNumericID);
                                NiPoint3 v19 = apItemActivated->GetLocationOnReference();
                                SoundHandle.SetPosition(v19.x, v19.y, v19.z);
                            }
                            SoundHandle.Play(false);
                        }
                        (&apItemActivated->m_Extra)->SetOpenCloseActivateRef(apActionRef);
                        apItemActivated->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                        if (pplayThisSeq == popenSeq && apActionRef == pPlayer) {
                            BGSOpenCloseForm* pocform =
                                BGSOpenCloseForm::GetAsOpenCloseForm(apItemActivated->GetObjectReference());
                            if ((pocform) && (pocform->GetBlockActivatesWhileOpening())) {
                                pPlayer->SetBlockActivate(true);
                            }
                        }
                    }
                }
            }
        }

        if (!bhasanims) {
            bool bopen = false;
            BGSOpenCloseForm::OPEN_STATE OpenState = BGSOpenCloseForm::GetOpenState(apItemActivated);
            if (OpenState <= BGSOpenCloseForm::OBJECT_NONE || OpenState > BGSOpenCloseForm::OBJECT_OPENING) {
                bopen = true;
                apItemActivated->SetAction(OBJECT_ACTION_OPEN);
            }
            else {
                apItemActivated->ClearAction(OBJECT_ACTION_OPEN);
            }
            BGSOpenCloseForm* pocform_2 = BGSOpenCloseForm::GetAsOpenCloseForm(apItemActivated->GetObjectReference());
            if (pocform_2) {
                if (bopen) {
                    pocform_2->HandleOpenFinish(apItemActivated, apActionRef);
                }
                else {
                    pocform_2->HandleCloseFinish(apItemActivated, apActionRef);
                }
            }
        }
    }
}

BGSOpenCloseForm* BGSOpenCloseForm::GetAsOpenCloseForm(TESForm* apForm) {
    BGSOpenCloseForm* pretval = nullptr;
    if ((apForm) && (BGSOpenCloseForm::IsOpenCloseForm(apForm))) {
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

NiControllerSequence* NiControllerManager::GetSequenceByName(const NiFixedString& kName) {
    NiControllerSequence* pkSequence;
    if (m_kStringMap.GetAt(kName.GetHandleString(), pkSequence)) {
        return pkSequence;
    }
    return nullptr;
}

bool NiControllerManager::ActivateSequence(
    NiControllerSequence* pkSequence,
    int32_t iPriority,
    bool bStartOver,
    float fWeight,
    float fEaseInTime,
    NiControllerSequence* pkTimeSyncSeq
) {
    // The priority is passed as its low byte, zero-extended.
    return pkSequence->Activate(
        static_cast<uint8_t>(iPriority), bStartOver, fWeight, fEaseInTime, pkTimeSyncSeq, false
    );
}

// Returns the result of Deactivate unchanged as a bool rather than an int, deliberately, as the game does.
bool NiControllerManager::DeactivateSequence(NiControllerSequence* pkSequence, float fEaseOutTime) {
    return pkSequence->Deactivate(fEaseOutTime, false);
}

void NiControllerSequence::SetOffset(float fOffset) {
    m_fOffset = fOffset;
}
