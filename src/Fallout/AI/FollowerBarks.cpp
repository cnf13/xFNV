#include "Fallout/AI/FollowerBarks.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "SDK/MSVC/rtti.h"

BSSimpleList<FollowerBarks::FollowerBarkEntry*> FollowerBarks::m_BarkHistory;

// No other bark may follow for 1800 ms.
void FollowerBarks::FollowerBarkEntry::BuildBarkEntry(Actor* apBarker, FollowerBarkType aeBarkType) {
    m_eBarkType = aeBarkType;
    m_pSpeaker = apBarker;
    m_tNoRepeatExpiration = BSGetTickCount() + GetBarkNoRepeatDuration(aeBarkType);
    m_tNoOtherBarksExpiration = BSGetTickCount() + 1800;
}

// 0x40000 is the SayToDone script action flag. The player is cast to TESBoundObject, which a reference never is, so the
// temporary-actor branch runs only if that cast succeeds; the topic and m_pSpeaker are tested again although the
// earlier tests passed. CreateReference(ACHR_ID) makes an actor reference, passed as the speaker and
// the voice's target.
void FollowerBarks::FollowerBarkEntry::Trigger() {
    TESTopic* pTopic = GetBarkTopicByType(m_eBarkType);
    if (!pTopic) {
        return;
    }
    TESTopicInfo* pInfo = pTopic->GetMatchingInfo(m_pSpeaker, pPlayer);
    if (!pInfo || !m_pSpeaker || !m_pSpeaker->GetCurrentAIProcess()) {
        return;
    }
    bool bplayerhead = true;
    bool balwaysplay = true;
    bool iPCHead = true;
    TESBoundObject* pNpc = dynamic_cast<TESBoundObject*>(pPlayer);
    bool bforcesub = false;
    if (!pTopic) {
        return;
    }
    if (m_pSpeaker) {
        if (!m_pSpeaker->GetCurrentAIProcess() || m_pSpeaker->IsDead(true)) {
            if (m_pSpeaker->GetCurrentAIProcess()) {
                Script::SetActionFlag(pTopic, m_pSpeaker->GetExtra(), SCRIPT_ACTION_ONSAYTODONE);
                m_pSpeaker->GetCurrentAIProcess()->SetDoingSayTo(false);
            }
            return;
        }
        m_pSpeaker->fSayToFailVoiceTimer = -1.0f;
        m_pSpeaker->GetCurrentAIProcess()->SetWaitingforLipFile(false);
        m_pSpeaker->GetCurrentAIProcess()->ClearDialogTarget();
        if (m_pSpeaker->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
            TESObjectREFR* pcurrentScriptTarget = m_pSpeaker->GetCurrentAIProcess()->GetCurrentHeadTrackTarget(
                BaseProcess::HEAD_TRACK_TYPE_SCRIPT
            );
            m_pSpeaker->GetCurrentAIProcess()->ClearScriptHeadTrackTarget(false);
            m_pSpeaker->GetCurrentAIProcess()->SetActionHeadTrackTarget(pcurrentScriptTarget);
            m_pSpeaker->GetCurrentAIProcess()->ClearGreetingInfoData();
        }
        else if (iPCHead) {
            m_pSpeaker->SetSayToTopic(pTopic);
            DialogueItem* pItem = pTopic->CreateDialogueItem(m_pSpeaker, nullptr, nullptr, nullptr, false);
            if (pItem) {
                m_pSpeaker->SetSayToTopicInfo(pItem->GetInfo());
            }
            BSSoundHandle kVoice = m_pSpeaker
                                       ->VoiceSoundFunctionEx(pTopic, m_pSpeaker, bplayerhead, balwaysplay, 0, false);
            kVoice.SetCompletionCallback(
                &TESObjectREFR::SayToCallBack,
                reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
            );
            return;
        }
        if (pPlayer->IsSleepingorResting() ||
            m_pSpeaker->GetCurrentAIProcess()->GetProcessLevel() ||
            (m_pSpeaker->GetEssential() && m_pSpeaker->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
            DialogueItem* pVoice = pTopic->CreateDialogueItem(m_pSpeaker, nullptr, nullptr, nullptr, true);
            if (pVoice) {
                pVoice->FirstResponse();
                pVoice->GetCurrentResponse();
                pVoice->RunResult(TIRS_BEGIN);
                pVoice->RunResult(TIRS_END);
                Script::SetActionFlag(pTopic, m_pSpeaker->GetExtra(), SCRIPT_ACTION_ONSAYTODONE);
                delete pVoice;
            }
        }
        else {
            m_pSpeaker->GetCurrentAIProcess()->ProcessGreet(m_pSpeaker, pTopic, bforcesub, false, true, true);
        }
    }
    else if (m_pSpeaker && pNpc) {
        TESObjectREFR* pRef = TESObjectREFR::CreateReference(ACHR_ID, false);
        pRef->SetTemporary();
        pRef->SetObjectReference(pNpc);
        pRef->SetLocationOnReference(m_pSpeaker->GetLocationOnReference());
        m_pSpeaker->SetSayToTopic(pTopic);
        Actor* pRefActor = static_cast<Actor*>(pRef);
        DialogueItem* pItem = pTopic->CreateDialogueItem(pRefActor, nullptr, nullptr, nullptr, false);
        if (pItem) {
            m_pSpeaker->SetSayToTopicInfo(pItem->GetInfo());
        }
        BSSoundHandle
            kVoice = pRef->VoiceSoundFunction(pTopic, pRefActor, bplayerhead, balwaysplay, false, false, false);
        kVoice.SetCompletionCallback(
            &TESObjectREFR::SayToCallBack,
            reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
        );
        delete pRef;
    }
    else if (m_pSpeaker->Get3D() || (m_pSpeaker->IsMobileObject() && m_pSpeaker->GetTalkingActivator()->Get3D())) {
        m_pSpeaker->SetSayToTopic(pTopic);
        DialogueItem* pItem = pTopic->CreateDialogueItem(m_pSpeaker, nullptr, nullptr, nullptr, false);
        if (pItem) {
            m_pSpeaker->SetSayToTopicInfo(pItem->GetInfo());
        }
        BSSoundHandle kVoice = m_pSpeaker->VoiceSoundFunctionEx(pTopic, nullptr, bplayerhead, balwaysplay, 0, false);
        kVoice.SetCompletionCallback(
            &TESObjectREFR::SayToCallBack,
            reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
        );
    }
}

bool FollowerBarks::FollowerBarkEntry::GetShouldRepeat() {
    bool bRepeat;
    if (!m_pSpeaker) {
        return false;
    }
    switch (m_eBarkType) {
        case FollowerBarks::BARK_KNOCKED_OUT:
        case FollowerBarks::BARK_KNOCKED_BACK:
        case FollowerBarks::BARK_CRIPPLED:
        case FollowerBarks::BARK_DIED:
        case FollowerBarks::BARK_OUT_OF_AMMO:
        case FollowerBarks::BARK_WEAPON_BROKE:
        case FollowerBarks::BARK_WEAPON_INEFFECTIVE:
        case FollowerBarks::BARK_USED_DOCTORS_BAG:
        case FollowerBarks::BARK_CAME_TO:
        case FollowerBarks::BARK_STEALTHING:
        case FollowerBarks::BARK_REGENERATING: {
            bRepeat = false;
            break;
        }
        case FollowerBarks::BARK_MINOR_HEALTH_WARNING: {
            float fHealth = m_pSpeaker->GetActorFloatValue(ActorValue::HEALTH);
            float fBaseHealth = m_pSpeaker->GetBaseActorFloatValue(ActorValue::HEALTH);
            float fRatio = 1.0f;
            if (fBaseHealth != 0.0) {
                fRatio = fHealth / fBaseHealth;
            }
            bRepeat = fRatio > 0.25 && fRatio <= 0.5;
            break;
        }
        case FollowerBarks::BARK_MAJOR_HEALTH_WARNING: {
            float fHealth = m_pSpeaker->GetActorFloatValue(ActorValue::HEALTH);
            float fBaseHealth = m_pSpeaker->GetBaseActorFloatValue(ActorValue::HEALTH);
            float fRatio = 1.0f;
            if (fBaseHealth != 0.0) {
                fRatio = fHealth / fBaseHealth;
            }
            bRepeat = fRatio <= 0.25;
            break;
        }
        case FollowerBarks::BARK_CRIPPLED_REMINDER: {
            for (ActorValue::Index eav = ActorValue::PERCEPTION_CONDITION; eav < ActorValue::BRAIN_CONDITION;
                 eav = static_cast<ActorValue::Index>(eav + 1)) {
                if (m_pSpeaker->GetActorFloatValue(eav) == 0.0) {
                    return true;
                }
            }
            bRepeat = false;
            break;
        }
        default: {
            bRepeat = false;
            break;
        }
    }
    return bRepeat;
}

void FollowerBarks::TriggerFollowerBark(Actor* apSpeaker, FollowerBarks::FollowerBarkType aeBarkType) {
    uint32_t nCurrentTime = BSGetTickCount();
    bool bPlayBark = true;
    bool bAddBark = true;
    BSSimpleList<FollowerBarks::FollowerBarkEntry*>* pHistory = &FollowerBarks::m_BarkHistory;
    while (pHistory) {
        FollowerBarks::FollowerBarkEntry* pEntry = pHistory->m_item;
        pHistory = pHistory->m_pkNext;
        if (pEntry && apSpeaker == pEntry->m_pSpeaker) {
            int32_t iPriority = FollowerBarks::GetBarkPriority(pEntry->m_eBarkType);
            if (iPriority >= FollowerBarks::GetBarkPriority(aeBarkType) &&
                pEntry->m_tNoOtherBarksExpiration > nCurrentTime) {
                bPlayBark = false;
            }
            if (pEntry->m_eBarkType == aeBarkType && pEntry->m_tNoRepeatExpiration > nCurrentTime) {
                bPlayBark = false;
                bAddBark = false;
            }
        }
    }
    if (bAddBark) {
        pHistory = &FollowerBarks::m_BarkHistory;
        while (pHistory) {
            FollowerBarks::FollowerBarkEntry* pEntry = pHistory->m_item;
            bool bDeleteEntry = false;
            if (pEntry && apSpeaker == pEntry->m_pSpeaker) {
                bDeleteEntry = pEntry->m_eBarkType == aeBarkType;
            }

            if (bDeleteEntry) {
                pHistory->RemoveHead();
            }
            else {
                pHistory = pHistory->m_pkNext;
            }
        }
        FollowerBarks::FollowerBarkEntry* pNewEntry = static_cast<FollowerBarks::FollowerBarkEntry*>(
            ourMalloc(sizeof(FollowerBarks::FollowerBarkEntry))
        );
        if (pNewEntry) {
            std::memset(pNewEntry, 0, sizeof(FollowerBarks::FollowerBarkEntry));
        }
        FollowerBarks::m_BarkHistory.AddHead(pNewEntry);
        pNewEntry->BuildBarkEntry(apSpeaker, aeBarkType);
        if (bPlayBark) {
            pNewEntry->Trigger();
        }
    }
}

TESTopic* FollowerBarks::GetBarkTopicByType(FollowerBarks::FollowerBarkType aeBarkType) {
    TESTopic* pTopic;
    switch (aeBarkType) {
        case FollowerBarks::BARK_KNOCKED_OUT: {
            TESForm* pForm = TESForm::GetFormByEditorID("KOed");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_KNOCKED_BACK: {
            TESForm* pForm = TESForm::GetFormByEditorID("KnockedBack");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_CRIPPLED:
        case FollowerBarks::BARK_CRIPPLED_REMINDER: {
            TESForm* pForm = TESForm::GetFormByEditorID("Crippled");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_DIED: {
            pTopic = nullptr;
            break;
        }
        case FollowerBarks::BARK_MINOR_HEALTH_WARNING: {
            TESForm* pForm = TESForm::GetFormByEditorID("HealthHalf");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_MAJOR_HEALTH_WARNING: {
            TESForm* pForm = TESForm::GetFormByEditorID("HealthQuarter");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_OUT_OF_AMMO: {
            TESForm* pForm = TESForm::GetFormByEditorID("NoAmmo");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_WEAPON_BROKE: {
            TESForm* pForm = TESForm::GetFormByEditorID("WeaponBroke");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_WEAPON_INEFFECTIVE: {
            TESForm* pForm = TESForm::GetFormByEditorID("WeaponIneffective");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_USED_DOCTORS_BAG: {
            TESForm* pForm = TESForm::GetFormByEditorID("UsedDoctorBag");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_CAME_TO: {
            TESForm* pForm = TESForm::GetFormByEditorID("Wake");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_STEALTHING: {
            TESForm* pForm = TESForm::GetFormByEditorID("FollowersStealthing");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        case FollowerBarks::BARK_REGENERATING: {
            TESForm* pForm = TESForm::GetFormByEditorID("Regenerating");
            pTopic = dynamic_cast<TESTopic*>(pForm);
            break;
        }
        default: {
            pTopic = nullptr;
            break;
        }
    }
    return pTopic;
}

int32_t FollowerBarks::GetBarkPriority(FollowerBarkType aeBarkType) {
    int32_t iPriority;
    switch (aeBarkType) {
        case BARK_KNOCKED_OUT: iPriority = 80; break;
        case BARK_KNOCKED_BACK: iPriority = 80; break;
        case BARK_CRIPPLED: iPriority = 40; break;
        case BARK_DIED: iPriority = 100; break;
        case BARK_MINOR_HEALTH_WARNING: iPriority = 50; break;
        case BARK_MAJOR_HEALTH_WARNING: iPriority = 60; break;
        case BARK_CRIPPLED_REMINDER: iPriority = 10; break;
        case BARK_OUT_OF_AMMO: iPriority = 70; break;
        case BARK_WEAPON_BROKE: iPriority = 70; break;
        case BARK_WEAPON_INEFFECTIVE: iPriority = 30; break;
        case BARK_USED_DOCTORS_BAG: iPriority = 20; break;
        case BARK_CAME_TO: iPriority = 90; break;
        case BARK_STEALTHING: iPriority = 15; break;
        case BARK_REGENERATING: iPriority = 5; break;
        default: iPriority = 0; break;
    }
    return iPriority;
}

int32_t FollowerBarks::GetBarkNoRepeatDuration(FollowerBarkType aeBarkType) {
    uint32_t uiDuration;
    switch (aeBarkType) {
        case BARK_KNOCKED_OUT: uiDuration = 1500; break;
        case BARK_KNOCKED_BACK: uiDuration = 1500; break;
        case BARK_CRIPPLED: uiDuration = 500; break;
        case BARK_DIED: uiDuration = 500; break;
        case BARK_MINOR_HEALTH_WARNING: uiDuration = 180000; break;
        case BARK_MAJOR_HEALTH_WARNING: uiDuration = 120000; break;
        case BARK_CRIPPLED_REMINDER: uiDuration = 180000; break;
        case BARK_OUT_OF_AMMO: uiDuration = 1000; break;
        case BARK_WEAPON_BROKE: uiDuration = 1000; break;
        case BARK_WEAPON_INEFFECTIVE: uiDuration = 60000; break;
        case BARK_USED_DOCTORS_BAG: uiDuration = 1000; break;
        case BARK_CAME_TO: uiDuration = 500; break;
        case BARK_STEALTHING: uiDuration = 20000; break;
        case BARK_REGENERATING: uiDuration = 3000; break;
        default: uiDuration = 500; break;
    }
    return uiDuration;
}

void FollowerBarks::ProcessFollowerBarks() {
    uint32_t nCurrentTime = BSGetTickCount();
    BSSimpleList<FollowerBarks::FollowerBarkEntry*>* pHistory = &FollowerBarks::m_BarkHistory;
    while (pHistory) {
        FollowerBarks::FollowerBarkEntry* pEntry = pHistory->m_item;
        bool bDeleteEntry = false;
        if (pEntry && pEntry->m_pSpeaker && pEntry->m_tNoRepeatExpiration <= nCurrentTime) {
            if (pEntry->GetShouldRepeat() && pEntry->m_pSpeaker->IsPlayerTeammate()) {
                pEntry->BuildBarkEntry(pEntry->m_pSpeaker, pEntry->m_eBarkType);
                pEntry->Trigger();
            }
            else if (pEntry->m_tNoOtherBarksExpiration > nCurrentTime) {
                if (!pEntry->m_pSpeaker->IsPlayerTeammate()) {
                    bDeleteEntry = true;
                }
            }
            else {
                bDeleteEntry = true;
            }
        }

        if (bDeleteEntry) {
            ourFree(pEntry);
            pHistory->RemoveHead();
        }
        else {
            pHistory = pHistory->m_pkNext;
        }
    }
}
