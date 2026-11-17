#include "Fallout/AI/FollowerBarks.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
// earlier tests passed, as the game does. CreateReference(ACHR_ID) makes an actor reference, passed as the speaker and
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
            TESObjectREFR* pcurrentScriptTarget =
                m_pSpeaker->GetCurrentAIProcess()->GetCurrentHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_SCRIPT);
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
            BSSoundHandle kVoice =
                m_pSpeaker->VoiceSoundFunctionEx(pTopic, m_pSpeaker, bplayerhead, balwaysplay, 0, false);
            kVoice.SetCompletionCallback(
                &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
            );
            return;
        }
        if (pPlayer->IsSleepingorResting() || m_pSpeaker->GetCurrentAIProcess()->GetProcessLevel() ||
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
        BSSoundHandle kVoice =
            pRef->VoiceSoundFunction(pTopic, pRefActor, bplayerhead, balwaysplay, false, false, false);
        kVoice.SetCompletionCallback(
            &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
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
            &TESObjectREFR::SayToCallBack, reinterpret_cast<void*>(static_cast<uintptr_t>(m_pSpeaker->GetFormID()))
        );
    }
}

bool FollowerBarks::FollowerBarkEntry::GetShouldRepeat() {
    bool result;
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
            result = false;
            break;
        }
        case FollowerBarks::BARK_MINOR_HEALTH_WARNING: {
            float v4 = m_pSpeaker->GetActorFloatValue(ActorValue::HEALTH);
            float v5 = m_pSpeaker->GetBaseActorFloatValue(ActorValue::HEALTH);
            float v6 = 1.0f;
            if (v5 != 0.0) {
                v6 = v4 / v5;
            }
            result = v6 > 0.25 && v6 <= 0.5;
            break;
        }
        case FollowerBarks::BARK_MAJOR_HEALTH_WARNING: {
            float v7 = m_pSpeaker->GetActorFloatValue(ActorValue::HEALTH);
            float v8 = m_pSpeaker->GetBaseActorFloatValue(ActorValue::HEALTH);
            float v9 = 1.0f;
            if (v8 != 0.0) {
                v9 = v7 / v8;
            }
            result = v9 <= 0.25;
            break;
        }
        case FollowerBarks::BARK_CRIPPLED_REMINDER: {
            for (ActorValue::Index eav = ActorValue::PERCEPTION_CONDITION; eav < ActorValue::BRAIN_CONDITION;
                 eav = static_cast<ActorValue::Index>(eav + 1)) {
                if (m_pSpeaker->GetActorFloatValue(eav) == 0.0) {
                    return true;
                }
            }
            result = false;
            break;
        }
        default: {
            result = false;
            break;
        }
    }
    return result;
}

void FollowerBarks::TriggerFollowerBark(Actor* apSpeaker, FollowerBarks::FollowerBarkType aeBarkType) {
    uint32_t nCurrentTime = BSGetTickCount();
    bool bPlayBark = true;
    bool bAddBark = true;
    BSSimpleList<FollowerBarks::FollowerBarkEntry*>* pHistory = &FollowerBarks::m_BarkHistory;
    while (pHistory) {
        FollowerBarks::FollowerBarkEntry* pEntry = pHistory->m_item;
        pHistory = pHistory->m_pkNext;
        if (pEntry) {
            if (apSpeaker == pEntry->m_pSpeaker) {
                int32_t BarkPriority = FollowerBarks::GetBarkPriority(pEntry->m_eBarkType);
                if (BarkPriority >= FollowerBarks::GetBarkPriority(aeBarkType) &&
                    pEntry->m_tNoOtherBarksExpiration > nCurrentTime) {
                    bPlayBark = false;
                }
                if (pEntry->m_eBarkType == aeBarkType && pEntry->m_tNoRepeatExpiration > nCurrentTime) {
                    bPlayBark = false;
                    bAddBark = false;
                }
            }
        }
    }
    if (bAddBark) {
        FollowerBarks::FollowerBarkEntry* pNewEntry;
        pHistory = &FollowerBarks::m_BarkHistory;
        while (pHistory) {
            FollowerBarks::FollowerBarkEntry* pEntry_2 = pHistory->m_item;
            bool bDeleteEntry = false;
            if (pEntry_2) {
                if (apSpeaker == pEntry_2->m_pSpeaker) {
                    bDeleteEntry = pEntry_2->m_eBarkType == aeBarkType;
                }
            }
            if (bDeleteEntry) {
                pHistory->RemoveHead();
            }
            else {
                pHistory = pHistory->m_pkNext;
            }
        }
        FollowerBarks::FollowerBarkEntry* v4 =
            static_cast<FollowerBarks::FollowerBarkEntry*>(ourMalloc(sizeof(FollowerBarks::FollowerBarkEntry)));
        if (v4) {
            std::memset(v4, 0, sizeof(FollowerBarks::FollowerBarkEntry));
            pNewEntry = v4;
        }
        else {
            pNewEntry = nullptr;
        }
        FollowerBarks::FollowerBarkEntry* pNewEntry_2 = pNewEntry;
        FollowerBarks::m_BarkHistory.AddHead(pNewEntry_2);
        pNewEntry_2->BuildBarkEntry(apSpeaker, aeBarkType);
        if (bPlayBark) {
            pNewEntry_2->Trigger();
        }
    }
}

TESTopic* FollowerBarks::GetBarkTopicByType(FollowerBarks::FollowerBarkType aeBarkType) {
    TESTopic* result;
    switch (aeBarkType) {
        case FollowerBarks::BARK_KNOCKED_OUT: {
            TESForm* FormByEditorID = TESForm::GetFormByEditorID("KOed");
            result = dynamic_cast<TESTopic*>(FormByEditorID);
            break;
        }
        case FollowerBarks::BARK_KNOCKED_BACK: {
            TESForm* v4 = TESForm::GetFormByEditorID("KnockedBack");
            result = dynamic_cast<TESTopic*>(v4);
            break;
        }
        case FollowerBarks::BARK_CRIPPLED:
        case FollowerBarks::BARK_CRIPPLED_REMINDER: {
            TESForm* v9 = TESForm::GetFormByEditorID("Crippled");
            result = dynamic_cast<TESTopic*>(v9);
            break;
        }
        case FollowerBarks::BARK_DIED: {
            result = nullptr;
            break;
        }
        case FollowerBarks::BARK_MINOR_HEALTH_WARNING: {
            TESForm* v8 = TESForm::GetFormByEditorID("HealthHalf");
            result = dynamic_cast<TESTopic*>(v8);
            break;
        }
        case FollowerBarks::BARK_MAJOR_HEALTH_WARNING: {
            TESForm* v7 = TESForm::GetFormByEditorID("HealthQuarter");
            result = dynamic_cast<TESTopic*>(v7);
            break;
        }
        case FollowerBarks::BARK_OUT_OF_AMMO: {
            TESForm* v5 = TESForm::GetFormByEditorID("NoAmmo");
            result = dynamic_cast<TESTopic*>(v5);
            break;
        }
        case FollowerBarks::BARK_WEAPON_BROKE: {
            TESForm* v6 = TESForm::GetFormByEditorID("WeaponBroke");
            result = dynamic_cast<TESTopic*>(v6);
            break;
        }
        case FollowerBarks::BARK_WEAPON_INEFFECTIVE: {
            TESForm* v10 = TESForm::GetFormByEditorID("WeaponIneffective");
            result = dynamic_cast<TESTopic*>(v10);
            break;
        }
        case FollowerBarks::BARK_USED_DOCTORS_BAG: {
            TESForm* v11 = TESForm::GetFormByEditorID("UsedDoctorBag");
            result = dynamic_cast<TESTopic*>(v11);
            break;
        }
        case FollowerBarks::BARK_CAME_TO: {
            TESForm* v2 = TESForm::GetFormByEditorID("Wake");
            result = dynamic_cast<TESTopic*>(v2);
            break;
        }
        case FollowerBarks::BARK_STEALTHING: {
            TESForm* v12 = TESForm::GetFormByEditorID("FollowersStealthing");
            result = dynamic_cast<TESTopic*>(v12);
            break;
        }
        case FollowerBarks::BARK_REGENERATING: {
            TESForm* v13 = TESForm::GetFormByEditorID("Regenerating");
            result = dynamic_cast<TESTopic*>(v13);
            break;
        }
        default: {
            result = nullptr;
            break;
        }
    }
    return result;
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
        if (pEntry) {
            if (pEntry->m_pSpeaker) {
                if (pEntry->m_tNoRepeatExpiration <= nCurrentTime) {
                    if (pEntry->GetShouldRepeat() && pEntry->m_pSpeaker->IsPlayerTeammate()) {
                        pEntry->BuildBarkEntry(pEntry->m_pSpeaker, pEntry->m_eBarkType);
                        pEntry->Trigger();
                    }
                    else {
                        if (pEntry->m_tNoOtherBarksExpiration > nCurrentTime) {
                            if (!pEntry->m_pSpeaker->IsPlayerTeammate()) {
                                bDeleteEntry = true;
                            }
                        }
                        else {
                            bDeleteEntry = true;
                        }
                    }
                }
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
