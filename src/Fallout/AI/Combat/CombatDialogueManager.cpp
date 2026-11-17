#include "Fallout/AI/Combat/CombatDialogueManager.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> bDisableCombatDialogue("bDisableCombatDialogue:Combat", false);

// Gets the target before the attacker.
bool CombatDialogueManager::StartDialogue(
    CombatController* apCombatController,
    DIALOGUE_TYPE aeDialogueType,
    int32_t aiDialogueTopic,
    bool abInterrupt
) {
    Actor* pTarget = apCombatController->GetTarget();
    Actor* pSpeaker = apCombatController->GetAttacker();
    return StartDialogue(pSpeaker, pTarget, aeDialogueType, aiDialogueTopic, abInterrupt, apCombatController);
}

bool CombatDialogueManager::StartDialogue(
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    DIALOGUE_TYPE aeDialogueType,
    int32_t aiDialogueTopic,
    bool abInterrupt,
    CombatController* apCombatController
) {
    BGSWorldLocation memberWorldLoc;
    BGSWorldLocation speakerWorldLoc;
    if (bDisableCombatDialogue.Bool()) {
        return false;
    }
    if (!apSpeaker->IsActor()) {
        return false;
    }
    uint32_t buseDialoguePackage = BSGetTickCount();
    if (buseDialoguePackage - CombatDialogueManager::uiLastCombatAttackSoundTime < 1500) {
        return false;
    }
    CombatDialogueManager::uiLastCombatAttackSoundTime = buseDialoguePackage;
    Actor* pspeaker = static_cast<Actor*>(apSpeaker);
    if (!apCombatController) {
        apCombatController = pspeaker->GetCombatController();
    }
    CombatTimer* ptimer = nullptr;
    if (apCombatController) {
        CombatState* pcombatState = apCombatController->pCombatState;
        CombatGroup* pcombatGroup = apCombatController->pCombatGroup;
        if (aeDialogueType == DT_COMBAT) {
            switch (aiDialogueTopic) {
                case 0:
                case 1: {
                    ptimer = &pcombatState->AttackDialogueTimer;
                    break;
                }
                case 3: {
                    ptimer = &pcombatState->FleeDialogueTimer;
                    break;
                }
                case 4: {
                    ptimer = &pcombatState->FireExplosiveDialogueTimer;
                    break;
                }
                case 5: {
                    ptimer = &pcombatGroup->AvoidThreatDialogueTimer;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else {
            if (aeDialogueType == DT_DETECTION) {
                ptimer = pcombatGroup->GetDetectionDialogueTimer(static_cast<uint32_t>(aiDialogueTopic));
            }
            else {
                Warning("COMBAT: Unknown dialogue type %i", aeDialogueType);
            }
        }
    }
    if (ptimer && !ptimer->Check()) {
        return false;
    }
    if (pspeaker->IsUnConscious() || pspeaker->GetCurrentProcessType()) {
        return false;
    }
    bool buseDialoguePackage_2 = true;
    if (abInterrupt) {
        if (pspeaker->GetSitSleepState()) {
            buseDialoguePackage_2 = false;
        }
        else {
            BaseProcess* CurrentAIProcess = pspeaker->pCurrentProcess;
            TESPackage* ppack_3 = CurrentAIProcess->GetRunOncePackage();
            if (ppack_3) {
                buseDialoguePackage_2 = ppack_3->GetPackType() != PACKAGE_TYPE_GET_UP;
            }
        }
    }
    else {
        BaseProcess* v31 = pspeaker->pCurrentProcess;
        if (v31->GetGreetingFlag()) {
            return false;
        }
        BaseProcess* v30 = pspeaker->pCurrentProcess;
        if (v30->GetRunOncePackage()) {
            return false;
        }
        BaseProcess* v29 = pspeaker->pCurrentProcess;
        TESPackage* ppack = v29->GetCurrentPackage();
        if (ppack && ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            return false;
        }
        BaseProcess* v28 = pspeaker->pCurrentProcess;
        TESPackage* ppack_2 = v28->GetRunOncePackage();
        if (ppack_2 &&
            (ppack_2->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE ||
             ppack_2->GetPackType() == PACKAGE_TYPE_GET_UP)) {
            return false;
        }
        if (pspeaker == pPlayer && pPlayer->IsPlayingCombatAttackSound()) {
            return false;
        }
    }
    if (ptimer) {
        float ftime = 0.0f;
        if (aeDialogueType == DT_COMBAT) {
            switch (aiDialogueTopic) {
                case 0:
                case 1: {
                    float a2a = fCombatDialogueAttackMaxElapsedTime.Float();
                    float* v8 = &fCombatDialogueAttackMinElapsedTime.Float();
                    ftime = RandomFloat(*v8, a2a);
                    break;
                }
                case 3: {
                    float a2d = fCombatDialogueFleeMaxElapsedTime.Float();
                    float* v11 = &fCombatDialogueFleeMinElapsedTime.Float();
                    ftime = RandomFloat(*v11, a2d);
                    break;
                }
                case 4: {
                    float a2b = fCombatDialogueFireExplosiveMaxElapsedTime.Float();
                    float* v9 = &fCombatDialogueFireExplosiveMinElapsedTime.Float();
                    ftime = RandomFloat(*v9, a2b);
                    break;
                }
                case 5: {
                    float a2c = fCombatDialogueAvoidThreatMaxElapsedTime.Float();
                    float* v10 = &fCombatDialogueAvoidThreatMinElapsedTime.Float();
                    ftime = RandomFloat(*v10, a2c);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else {
            if (aeDialogueType == DT_DETECTION) {
                float* v12;
                float a2;
                // The topic is tested signed against 0 and 1.
                switch (aiDialogueTopic) {
                    case 0:
                    case 1: {
                        a2 = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
                        v12 = &fCombatDetectionDialogueIdleMinElapsedTime.Float();
                        break;
                    }
                    default: {
                        a2 = fCombatDetectionDialogueMaxElapsedTime.Float();
                        v12 = &fCombatDetectionDialogueMinElapsedTime.Float();
                        break;
                    }
                }
                ftime = RandomFloat(*v12, a2);
            }
            else {
                Warning("COMBAT: Unknown dialogue type %i", aeDialogueType);
            }
        }
        ptimer->Reset(ftime);
    }
    Actor* pdialogueTarget = nullptr;
    if (buseDialoguePackage_2) {
        if (apCombatController) {
            CombatGroup* pgroup = apCombatController->pCombatGroup;
            float fmaxDistanceSquared = 1048576.0f;
            uint32_t imemberCount = pgroup->GetMemberCount();
            pspeaker->GetWorldLocation(&speakerWorldLoc);
            for (uint32_t i = 0; i < imemberCount; ++i) {
                Actor* pmember = pgroup->GetMember(i);
                if (pmember != pspeaker && pmember != pPlayer) {
                    BaseProcess* v26 = pmember->pCurrentProcess;
                    if (!v26->GetGreetingFlag()) {
                        BaseProcess* v25 = pmember->pCurrentProcess;
                        TESPackage* ppack_4 = v25->GetCurrentPackage();
                        if (!ppack_4 || ppack_4->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                            BaseProcess* v24 = pmember->pCurrentProcess;
                            ppack_4 = v24->GetRunOncePackage();
                            if (!ppack_4 || ppack_4->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                                pmember->GetWorldLocation(&memberWorldLoc);
                                float fdistanceSquared = memberWorldLoc.GetDistanceSquared(speakerWorldLoc);
                                if (fmaxDistanceSquared > static_cast<double>(fdistanceSquared)) {
                                    pdialogueTarget = pmember;
                                    fmaxDistanceSquared = fdistanceSquared;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (!pdialogueTarget) {
        pdialogueTarget = pspeaker;
    }
    TESTopic* ptopic = TESTopic::GetTopic(aeDialogueType, aiDialogueTopic);
    if (!ptopic) {
        return false;
    }
    if (pspeaker == pPlayer) {
        DialogueItem* pvoice = ptopic->CreateDialogueItem(apSpeaker, apTarget, ptopic, nullptr, true);
        if (pvoice) {
            if (pvoice->FirstResponse()) {
                DialogueResponse* presponse = pvoice->GetCurrentResponse();
                if (presponse) {
                    pPlayer->SetSpeaking(true);
                    BaseProcess* v23 = pspeaker->pCurrentProcess;
                    BSSoundHandle pSoundHandlePointer = v23->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
                    uint32_t Length = presponse->cText.GetLength();
                    uint32_t CastingType = presponse->uiEmotion;
                    DIALOGUE_EMOTION FormFlags = presponse->eEmotion;
                    char* v13 = presponse->GetVoice()->pString;
                    pspeaker->SpeakSoundFunction(
                        v13, pSoundHandlePointer, FormFlags, CastingType, Length, nullptr, nullptr, nullptr, false,
                        false, false, false, true
                    );
                    pSoundHandlePointer.SetCompletionCallback(&PlayerCharacter::SpeakDoneCallback, nullptr);
                    pvoice->RunResult(TIRS_BEGIN);
                }
            }
            if (pvoice) {
                delete pvoice;
            }
        }
    }
    else {
        if (!buseDialoguePackage_2) {
            BaseProcess* v22 = pspeaker->pCurrentProcess;
            v22->ProcessGreet(pspeaker, ptopic, false, false, false, false);
            return true;
        }
        if (pspeaker->InitiateDialogue(pdialogueTarget, nullptr, nullptr, false, true, true, ptopic, true, false)) {
            return true;
        }
    }
    return false;
}

void CombatDialogueManager::Create() {
    if (!pCombatDialogueManager) {
        pCombatDialogueManager = static_cast<CombatDialogueManager*>(ourMalloc(sizeof(CombatDialogueManager)));
    }
}

void CombatDialogueManager::Destroy() {
    ourFree(pCombatDialogueManager);
    pCombatDialogueManager = nullptr;
}
