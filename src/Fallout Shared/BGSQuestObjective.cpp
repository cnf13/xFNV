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
    return (bool)((uint8_t)(((int32_t)eState) > 1));
}

// The display text's characters.
const char* BGSQuestObjective::GetDisplayText() {
    if ((&cDisplayText)->pString) {
        return (&cDisplayText)->pString;
    }

    return "";
}

bool BGSQuestObjective::GetDisplayed() {
    return (bool)(((int32_t)eState) & 1);
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
    (&cDisplayText)->Set(apDisplayText, (uint32_t)0);
}

// Sets the objective's state: a started objective enables its quest, shows the update text and, the first time, the
// quest's name; a completed one shows the completion text. The player's objective list follows every change.
void BGSQuestObjective::SetState(QUEST_OBJECTIVE_STATE aeNewState) {
    TESQuest* pOwnerQuest = GetOwner();
    pOwnerQuest->AddChange(0x20000000u);
    if (aeNewState == 1 || eState == 0) {
        if (aeNewState == 1) {
            if (!pOwnerQuest->GetCompleted()) {
                pOwnerQuest->SetEnabled(true);
                if (!pPlayer->GetActiveQuest()) {
                    pPlayer->SetActiveQuest(pOwnerQuest);
                }
            }
            HUDMainMenu::ShowQuestObjective(GetDisplayText(), false, false);
            if (!(uint8_t)pOwnerQuest->GetHasShownQuestUpdate()) {
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

bool BGSQuestObjective::GetStatus(int32_t a2) {
    return ((a2 & eState) != 0);
}

void BGSQuestObjective::InitializeData() {
    iIndex = (uint32_t)0;
    cDisplayText = "";
    bInitialized = false;
    eState = QOS_DORMANT;
    pOwnerQuest = nullptr;
}

void BGSQuestObjective::UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill) {
    if (eState == QOS_DISPLAYED) {
        BSSimpleList<TESQuestTarget*>* pMasterList = &ListTargets;
        void* pQuestTarget = nullptr;
        while (pMasterList && pMasterList->m_item) {
            TESQuestTarget* pTarget = pMasterList->m_item;
            pMasterList = pMasterList->m_pkNext;
            TESObjectREFR* Reference = pTarget->GetReference(false);
            if ((&pTarget->objConditions)->IsTrue(Reference, nullptr)) {
                bool IgnoreLocks = pTarget->GetIgnoreLocks();
                TeleportPath* v4 = pTarget->QTargetPath();
                TESObjectREFR* v3 = pTarget->GetReference(true);
                pPlayer->BuildPathToTarget(v3, v4, IgnoreLocks);
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
    uint32_t v2 = GetDisplayTextLength() + 1;
    const char* DisplayText = GetDisplayText();
    TESForm::AddChunkArray(NNAM_ID, DisplayText, v2);
    for (BSSimpleList<TESQuestTarget*>* pTargetList = &ListTargets; pTargetList && !pTargetList->IsEmpty();
         pTargetList = pTargetList->m_pkNext) {}
}

TeleportPath* TESQuestTarget::QTargetPath() {
    return &m_TargetPath;
}

void PlayerCharacter::AddQuestObjective(BGSQuestObjective* apObj) {
    BSSimpleList<BGSQuestObjective*>* plist = GetQuestObjectiveList();
    TESQuest* pquest = apObj->pOwnerQuest;
    bool v6 = false;
    char v7 = 0;
    while (plist && plist->m_item) {
        BGSQuestObjective* m_item = plist->m_item;
        if (apObj == m_item) {
            v7 = 1;
            break;
        }
        if (!v6) {
            v6 = m_item->pOwnerQuest == pquest;
        }
        plist = plist->m_pkNext;
    }
    if (apObj->pOwnerQuest->GetObjectiveList() && !apObj->pOwnerQuest->GetObjectiveList()->IsEmpty() && !v7) {
        GetQuestObjectiveList()->AddHead(apObj);
    }
    if (apObj->pOwnerQuest == pActiveQuest) {
        apObj->UpdateCurrentTargetList(&listQuestTargets);
    }
}

void HUDMainMenu::ShowQuestObjective(const char* apText, bool abCompleted, bool abFailed) {
    if ((HUDMainMenu::pme) && (apText)) {
        HUDMainMenu::HUDShowQuestObjectivsCritSect.Enter(nullptr);
        uiMessageDelayTime = appTimer.iLastTime;
        HUDMainMenu::QueuedQuestText* a2 =
            static_cast<HUDMainMenu::QueuedQuestText*>(ourMalloc(sizeof(HUDMainMenu::QueuedQuestText)));
        BSstrcpy(a2->msg, 260, apText);
        a2->byte104 = abCompleted;
        a2->byte105 = abFailed;
        HUDMainMenu::pme->xQuestObjectives.AddHead(a2);
        HUDMainMenu::HUDShowQuestObjectivsCritSect.Leave();
    }
}
