#include "Fallout/AI/Combat/CombatDialogueManager.h"

#include "BSCore/NewOverloads.h"
#include "BSMain/XINISetting.h"
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
    BGSWorldLocation kMemberWorldLoc;
    BGSWorldLocation kSpeakerWorldLoc;
    if (bDisableCombatDialogue.Bool()) {
        return false;
    }
    if (!apSpeaker->IsActor()) {
        return false;
    }
    uint32_t uiTickCount = BSGetTickCount();
    if (uiTickCount - CombatDialogueManager::uiLastCombatAttackSoundTime < 1500) {
        return false;
    }
    CombatDialogueManager::uiLastCombatAttackSoundTime = uiTickCount;
    Actor* pSpeaker = static_cast<Actor*>(apSpeaker);
    if (!apCombatController) {
        apCombatController = pSpeaker->GetCombatController();
    }
    CombatTimer* pTimer = nullptr;
    if (apCombatController) {
        CombatState* pCombatState = apCombatController->pCombatState;
        CombatGroup* pCombatGroup = apCombatController->pCombatGroup;
        if (aeDialogueType == DT_COMBAT) {
            switch (aiDialogueTopic) {
                case 0:
                case 1: {
                    pTimer = &pCombatState->AttackDialogueTimer;
                    break;
                }
                case 3: {
                    pTimer = &pCombatState->FleeDialogueTimer;
                    break;
                }
                case 4: {
                    pTimer = &pCombatState->FireExplosiveDialogueTimer;
                    break;
                }
                case 5: {
                    pTimer = &pCombatGroup->AvoidThreatDialogueTimer;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else if (aeDialogueType == DT_DETECTION) {
            pTimer = pCombatGroup->GetDetectionDialogueTimer(static_cast<uint32_t>(aiDialogueTopic));
        }
        else {
            Warning("COMBAT: Unknown dialogue type %i", aeDialogueType);
        }
    }
    if (pTimer && !pTimer->Check()) {
        return false;
    }
    if (pSpeaker->IsUnConscious() || pSpeaker->GetCurrentProcessType()) {
        return false;
    }
    bool bUseDialoguePackage = true;
    if (abInterrupt) {
        if (pSpeaker->GetSitSleepState()) {
            bUseDialoguePackage = false;
        }
        else {
            TESPackage* pPackage = pSpeaker->pCurrentProcess->GetRunOncePackage();
            if (pPackage) {
                bUseDialoguePackage = pPackage->GetPackType() != PACKAGE_TYPE_GET_UP;
            }
        }
    }
    else {
        if (pSpeaker->pCurrentProcess->GetGreetingFlag()) {
            return false;
        }
        if (pSpeaker->pCurrentProcess->GetRunOncePackage()) {
            return false;
        }
        TESPackage* pPackage = pSpeaker->pCurrentProcess->GetCurrentPackage();
        if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
            return false;
        }
        pPackage = pSpeaker->pCurrentProcess->GetRunOncePackage();
        if (pPackage &&
            (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE ||
             pPackage->GetPackType() == PACKAGE_TYPE_GET_UP)) {
            return false;
        }
        if (pSpeaker == pPlayer && pPlayer->IsPlayingCombatAttackSound()) {
            return false;
        }
    }
    if (pTimer) {
        float fTime = 0.0f;
        if (aeDialogueType == DT_COMBAT) {
            switch (aiDialogueTopic) {
                case 0:
                case 1: {
                    float fMaxTime = fCombatDialogueAttackMaxElapsedTime.Float();
                    fTime = RandomFloat(fCombatDialogueAttackMinElapsedTime.Float(), fMaxTime);
                    break;
                }
                case 3: {
                    float fMaxTime = fCombatDialogueFleeMaxElapsedTime.Float();
                    fTime = RandomFloat(fCombatDialogueFleeMinElapsedTime.Float(), fMaxTime);
                    break;
                }
                case 4: {
                    float fMaxTime = fCombatDialogueFireExplosiveMaxElapsedTime.Float();
                    fTime = RandomFloat(fCombatDialogueFireExplosiveMinElapsedTime.Float(), fMaxTime);
                    break;
                }
                case 5: {
                    float fMaxTime = fCombatDialogueAvoidThreatMaxElapsedTime.Float();
                    fTime = RandomFloat(fCombatDialogueAvoidThreatMinElapsedTime.Float(), fMaxTime);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else if (aeDialogueType == DT_DETECTION) {
            float fMinTime;
            float fMaxTime;
            // The topic is tested signed against 0 and 1.
            switch (aiDialogueTopic) {
                case 0:
                case 1: {
                    fMaxTime = fCombatDetectionDialogueIdleMaxElapsedTime.Float();
                    fMinTime = fCombatDetectionDialogueIdleMinElapsedTime.Float();
                    break;
                }
                default: {
                    fMaxTime = fCombatDetectionDialogueMaxElapsedTime.Float();
                    fMinTime = fCombatDetectionDialogueMinElapsedTime.Float();
                    break;
                }
            }
            fTime = RandomFloat(fMinTime, fMaxTime);
        }
        else {
            Warning("COMBAT: Unknown dialogue type %i", aeDialogueType);
        }
        pTimer->Reset(fTime);
    }
    Actor* pDialogueTarget = nullptr;
    if (bUseDialoguePackage && apCombatController) {
        CombatGroup* pGroup = apCombatController->pCombatGroup;
        float fMaxDistanceSquared = 1048576.0f;
        uint32_t uiMemberCount = pGroup->GetMemberCount();
        pSpeaker->GetWorldLocation(&kSpeakerWorldLoc);
        for (uint32_t i = 0; i < uiMemberCount; ++i) {
            Actor* pMember = pGroup->GetMember(i);
            if (pMember != pSpeaker && pMember != pPlayer) {
                if (!pMember->pCurrentProcess->GetGreetingFlag()) {
                    TESPackage* pPackage = pMember->pCurrentProcess->GetCurrentPackage();
                    if (!pPackage || pPackage->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                        pPackage = pMember->pCurrentProcess->GetRunOncePackage();
                        if (!pPackage || pPackage->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                            pMember->GetWorldLocation(&kMemberWorldLoc);
                            float fDistanceSquared = kMemberWorldLoc.GetDistanceSquared(kSpeakerWorldLoc);
                            if (fMaxDistanceSquared > static_cast<double>(fDistanceSquared)) {
                                pDialogueTarget = pMember;
                                fMaxDistanceSquared = fDistanceSquared;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!pDialogueTarget) {
        pDialogueTarget = pSpeaker;
    }
    TESTopic* pTopic = TESTopic::GetTopic(aeDialogueType, aiDialogueTopic);
    if (!pTopic) {
        return false;
    }
    if (pSpeaker == pPlayer) {
        DialogueItem* pVoice = pTopic->CreateDialogueItem(apSpeaker, apTarget, pTopic, nullptr, true);
        if (pVoice) {
            if (pVoice->FirstResponse()) {
                DialogueResponse* pResponse = pVoice->GetCurrentResponse();
                if (pResponse) {
                    pPlayer->SetSpeaking(true);
                    BSSoundHandle kSoundHandlePointer = pSpeaker->pCurrentProcess->GetSoundHandle(
                        ACTOR_SOUND_HANDLE_VOICE
                    );
                    uint32_t uiLength = pResponse->cText.GetLength();
                    uint32_t uiEmotionValue = pResponse->uiEmotion;
                    DIALOGUE_EMOTION eEmotion = pResponse->eEmotion;
                    char* pVoiceFile = pResponse->GetVoice()->pString;
                    pSpeaker->SpeakSoundFunction(
                        pVoiceFile,
                        kSoundHandlePointer,
                        eEmotion,
                        uiEmotionValue,
                        uiLength,
                        nullptr,
                        nullptr,
                        nullptr,
                        false,
                        false,
                        false,
                        false,
                        true
                    );
                    kSoundHandlePointer.SetCompletionCallback(&PlayerCharacter::SpeakDoneCallback, nullptr);
                    pVoice->RunResult(TIRS_BEGIN);
                }
            }
            if (pVoice) {
                delete pVoice;
            }
        }
    }
    else {
        if (!bUseDialoguePackage) {
            pSpeaker->pCurrentProcess->ProcessGreet(pSpeaker, pTopic, false, false, false, false);
            return true;
        }
        if (pSpeaker->InitiateDialogue(pDialogueTarget, nullptr, nullptr, false, true, true, pTopic, true, false)) {
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
