#include "Fallout Shared/BGSQuestObjective.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "xfnv/Compiler.h"

BGSQuestObjective::BGSQuestObjective() {
    InitializeData();
}

BGSQuestObjective::~BGSQuestObjective() {
    ClearData();
}

bool BGSQuestObjective::GetCompleted() {
    return eState > QOS_DISPLAYED;
}

// The display text's characters.
const char* BGSQuestObjective::GetDisplayText() {
    if (cDisplayText.pString) {
        return cDisplayText.pString;
    }

    return "";
}

bool BGSQuestObjective::GetDisplayed() {
    return (eState & QOS_DISPLAYED) != 0;
}

TESQuest* BGSQuestObjective::GetOwner() {
    return pOwnerQuest;
}

uint32_t BGSQuestObjective::GetIndex() {
    return iIndex;
}

void BGSQuestObjective::ClearTargetList() {
    for (BSSimpleList<TESQuestTarget*>* pIter = GetTargetList(); pIter && !pIter->IsEmpty(); pIter->RemoveHead()) {
        TESQuestTarget* pTarget = pIter->GetItem();
        if (pTarget) {
            delete pTarget;
        }
    }
}

void BGSQuestObjective::ClearData() {
    ClearTargetList();
}

// Loads the objective's index and display text from apFile's chunks.
void BGSQuestObjective::Load(TESFile* apFile) {
    do {
        uint32_t uiTESChunk = apFile->GetTESChunk();
        if (uiTESChunk == NO_CHUNK) {
            break;
        }
        if (uiTESChunk == QOBJ_ID) {
            apFile->GetChunkData<uint32_t>(iIndex);
        }
        else if (uiTESChunk == NNAM_ID) {
            if (apFile->GetChunkSize()) {
                char* pcText = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pcText, 0);
                SetDisplayText(pcText);
            }
            else {
                SetDisplayText(nullptr);
            }
            return;
        }
    }
    while (apFile->NextChunk());
}

void BGSQuestObjective::InitItem(TESQuest* apUser) {
    if (bInitialized) {
        return;
    }
    pOwnerQuest = apUser;
    for (BSSimpleList<TESQuestTarget*>* pIter = GetTargetList(); pIter && !pIter->IsEmpty(); pIter = pIter->GetNext()) {
        pIter->GetItem()->InitItem(apUser);
    }
    bInitialized = true;
}

void BGSQuestObjective::SetDisplayText(const char* apDisplayText) {
    cDisplayText.Set(apDisplayText, 0);
}

// Sets the objective's state: a started objective enables its quest, shows the update text and, the first time, the
// quest's name; a completed one shows the completion text. The player's objective list follows every change.
void BGSQuestObjective::SetState(QUEST_OBJECTIVE_STATE aeNewState) {
    TESQuest* pOwnerQuest = GetOwner();
    pOwnerQuest->AddChange(CHANGE_QUEST_OBJECTIVES);
    if (aeNewState == 1 || eState == 0) {
        if (aeNewState == 1) {
            if (!pOwnerQuest->GetCompleted()) {
                pOwnerQuest->SetEnabled(true);
                if (!pPlayer->GetActiveQuest()) {
                    pPlayer->SetActiveQuest(pOwnerQuest);
                }
            }
            HUDMainMenu::ShowQuestObjective(GetDisplayText(), false, false);
            if (!static_cast<uint8_t>(pOwnerQuest->GetHasShownQuestUpdate())) {
                HUDMainMenu::ShowQuestName(pOwnerQuest);
                pOwnerQuest->SetDisplayedQuestStarted(true);
            }
        }
        pPlayer->AddQuestObjective(this);
    }
    if (aeNewState == 3 && eState == 1 && !pOwnerQuest->GetCompleted()) {
        HUDMainMenu::ShowQuestObjective(GetDisplayText(), true, false);
    }
    eState = aeNewState;
}

bool BGSQuestObjective::GetStatus(int32_t aiStatus) {
    return (aiStatus & eState) != 0;
}

void BGSQuestObjective::InitializeData() {
    iIndex = 0;
    cDisplayText = "";
    bInitialized = false;
    eState = QOS_DORMANT;
    pOwnerQuest = nullptr;
}

void BGSQuestObjective::UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill) {
    if (eState == QOS_DISPLAYED) {
        BSSimpleList<TESQuestTarget*>* pMasterList = &ListTargets;
        while (pMasterList && pMasterList->m_item) {
            TESQuestTarget* pTarget = pMasterList->m_item;
            pMasterList = pMasterList->m_pkNext;
            TESObjectREFR* pReference = pTarget->GetReference(false);
            if (pTarget->objConditions.IsTrue(pReference, nullptr)) {
                bool bIgnoreLocks = pTarget->GetIgnoreLocks();
                TeleportPath* pTargetPath = pTarget->QTargetPath();
                TESObjectREFR* pTargetRef = pTarget->GetReference(true);
                pPlayer->BuildPathToTarget(pTargetRef, pTargetPath, bIgnoreLocks);
                apListToFill->AddTail(pTarget);
            }
        }
    }
}

uint32_t BGSQuestObjective::GetDisplayTextLength() {
    return cDisplayText.GetLength();
}

void BGSQuestObjective::Save() {
    TESForm::AddChunk(QOBJ_ID, iIndex);
    uint32_t uiLength = GetDisplayTextLength() + 1;
    const char* pDisplayText = GetDisplayText();
    TESForm::AddChunkArray(NNAM_ID, pDisplayText, uiLength);
    for (BSSimpleList<TESQuestTarget*>* pTargetList = &ListTargets; pTargetList && !pTargetList->IsEmpty();
         pTargetList = pTargetList->m_pkNext) {}
}

TeleportPath* TESQuestTarget::QTargetPath() {
    return &m_TargetPath;
}

void PlayerCharacter::AddQuestObjective(BGSQuestObjective* apObj) {
    BSSimpleList<BGSQuestObjective*>* pList = GetQuestObjectiveList();
    TESQuest* pQuest = apObj->pOwnerQuest;
    bool bQuestFound = false;
    char cFound = 0;
    while (pList && pList->m_item) {
        BGSQuestObjective* pObjective = pList->m_item;
        if (apObj == pObjective) {
            cFound = 1;
            break;
        }
        if (!bQuestFound) {
            bQuestFound = pObjective->pOwnerQuest == pQuest;
        }
        pList = pList->m_pkNext;
    }
    if (apObj->pOwnerQuest->GetObjectiveList() && !apObj->pOwnerQuest->GetObjectiveList()->IsEmpty() && !cFound) {
        GetQuestObjectiveList()->AddHead(apObj);
    }
    if (apObj->pOwnerQuest == pActiveQuest) {
        apObj->UpdateCurrentTargetList(&listQuestTargets);
    }
}

void HUDMainMenu::ShowQuestObjective(const char* apText, bool abCompleted, bool abReminder) {
    if (HUDMainMenu::pme && apText) {
        HUDMainMenu::HUDShowQuestObjectivsCritSect.Enter(nullptr);
        uiMessageDelayTime = appTimer.iLastTime;
        HUDMainMenu::QueuedQuestText* pQueuedText = static_cast<HUDMainMenu::QueuedQuestText*>(
            ourMalloc(sizeof(HUDMainMenu::QueuedQuestText))
        );
        BSstrcpy(pQueuedText->msg, 260, apText);
        pQueuedText->bCompleted = abCompleted;
        pQueuedText->bReminder = abReminder;
        HUDMainMenu::pme->xQuestObjectives.AddHead(pQueuedText);
        HUDMainMenu::HUDShowQuestObjectivsCritSect.Leave();
    }
}
