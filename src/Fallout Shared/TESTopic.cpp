#include "Fallout Shared/TESTopic.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConversationData.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TopicInfoArray.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/time.h"

// The built-in topics of each dialogue type, which pDefaultTopics indexes (the counts are
// uiNumHardcodedTopicsPerCategory); AddDefaultTopics creates their forms.
using RadioTopicArray = DEFAULT_TOPIC[1];
static RadioTopicArray RadioTopic{{nullptr, 0x126, "RadioHello"}};
using PlayerTopicArray = DEFAULT_TOPIC[15];
static PlayerTopicArray PlayerTopics{
    {nullptr, 0xC8, "GREETING"},
    {nullptr, 0xC9, "PLAYERSWINGMELEEWEAPON"},
    {nullptr, 0xCA, "PLAYERTHROWGRENADE"},
    {nullptr, 0xCB, "PLAYERFIREWEAPON"},
    {nullptr, 0xCC, "PLAYERLAYMINE"},
    {nullptr, 0xCD, "PLAYERZKEYOBJECT"},
    {nullptr, 0xCE, "PLAYERJUMP"},
    {nullptr, 0xCF, "PLAYERKNOCKOVEROBJECTS"},
    {nullptr, 0xD0, "PLAYERINIRONSITES"},
    {nullptr, 0xD1, "PLAYERDESTROYOBJECT"},
    {nullptr, 0xC7, "PLAYERSTANDONFURNITURE"},
    {nullptr, 0xFD, "SpeechChallengeFailure"},
    {nullptr, 0x118, "InfoRefusal"},
    {nullptr, 0xC5, "PLAYERLOCKEDOBJECT"},
    {nullptr, 0xC6, "PLAYERPICKPOCKET "}
};
using MiscTopicArray = DEFAULT_TOPIC[3];
static MiscTopicArray MiscTopics{
    {nullptr, 0x11A, "ObserveCombat"},
    {nullptr, 0x11B, "Corpse"},
    {nullptr, 0x11C, "TimeToGo"}
};
using ConversationTopicArray = DEFAULT_TOPIC[6];
static ConversationTopicArray ConversationTopics{{nullptr, 0xD2, "HELLO"},     {nullptr, 0xD3, "ANY"},
                                                 {nullptr, 0xD4, "GOODBYE"},   {nullptr, 0xD5, "IdleChatter"},
                                                 {nullptr, 0xD6, "SPELLHELP"}, {nullptr, 0xD7, "INFOGENERAL"}};
using CombatTopicArray = DEFAULT_TOPIC[21];
static CombatTopicArray CombatTopics{{nullptr, 0xDC, "Attack"},        {nullptr, 0xE6, "PowerAttack"},
                                     {nullptr, 0xDD, "Hit"},           {nullptr, 0xDE, "Flee"},
                                     {nullptr, 0xEC, "FireExplosive"}, {nullptr, 0xED, "AvoidThreat"},
                                     {nullptr, 0xEF, "Death"},         {nullptr, 0xF0, "CombatIdleChatter"},
                                     {nullptr, 0xF1, "GroupStrategy"}, {nullptr, 0xDF, "Steal"},
                                     {nullptr, 0xE1, "Yield"},         {nullptr, 0xE2, "AcceptYield"},
                                     {nullptr, 0xE3, "Pickpocket"},    {nullptr, 0xE4, "Assault"},
                                     {nullptr, 0xE5, "Murder"},        {nullptr, 0xE7, "AssaultNoCrime"},
                                     {nullptr, 0xE8, "MurderNoCrime"}, {nullptr, 0xE9, "PickpocketNoCrime"},
                                     {nullptr, 0xEA, "StealNoCrime"},  {nullptr, 0xEB, "TrespassNoCrime"},
                                     {nullptr, 0xF2, "GuardTrespass"}};
static int32_t data_119A334;
using DetectionTopicArray = DEFAULT_TOPIC[10];
static DetectionTopicArray DetectionTopics{{nullptr, 0x104, "AlertIdle"},     {nullptr, 0x107, "LostIdle"},
                                           {nullptr, 0xF4, "NormalToAlert"},  {nullptr, 0xF3, "AlertToCombat"},
                                           {nullptr, 0xEE, "NormalToCombat"}, {nullptr, 0xF5, "AlertToNormal"},
                                           {nullptr, 0xF6, "CombatToNormal"}, {nullptr, 0xF7, "CombatToLost"},
                                           {nullptr, 0xF8, "LostToNormal"},   {nullptr, 0xF9, "LostToCombat"}};
using ServiceTopicArray = DEFAULT_TOPIC[9];
static ServiceTopicArray ServiceTopics{{nullptr, 0x10E, "ServiceRefusal"}, {nullptr, 0x111, "Repair"},
                                       {nullptr, 0x112, "Travel"},         {nullptr, 0x113, "Training"},
                                       {nullptr, 0x116, "BarterExit"},     {nullptr, 0x11D, "RepairExit"},
                                       {nullptr, 0x120, "Recharge"},       {nullptr, 0x121, "RechargeExit"},
                                       {nullptr, 0x124, "TrainingExit"}};
TESTopic::DefaultTopicArray TESTopic::pDefaultTopics{PlayerTopics,    ConversationTopics, CombatTopics, nullptr,
                                                     DetectionTopics, ServiceTopics,      MiscTopics,   RadioTopic};

QUEST_INFO::QUEST_INFO() : pQuest(nullptr), pRemovedQuest(nullptr), bInitialized(false) {}

TopicInfoArray* QUEST_INFO::GetInfoArray() {
    return &infoArray;
}

TESTopic::TESTopic(uint8_t aeType) : m_fPriority(50.0f), m_iJournalIndex(0) {
    ourMemset(&m_Data, 0, sizeof(m_Data));
    m_Data.type = aeType;
    cFormType = 69;
}

TESTopic::~TESTopic() {
    for (BSSimpleList<QUEST_INFO*>* pList = &m_listQuestInfo; pList && pList->m_item; pList->RemoveHead()) {
        QUEST_INFO* pQuestInfo = pList->m_item;
        uint32_t maxSize = pQuestInfo->infoArray.m_uiSize;
        for (uint32_t i = 0; i < maxSize; ++i) {
            TESTopicInfo* info = TESTopic::GetInfoByIndex(pQuestInfo, i);
            if (info) {
                TESTopicInfo* pNoInfo = nullptr;
                pQuestInfo->infoArray.SetAt(i, pNoInfo);
                info->SetInfoIndex(0xFFFF);
                delete info;
            }
        }
        delete pQuestInfo;
    }
}

void TESTopic::InitializeData() {
    TESForm::InitializeData();
    SetDummyPrompt("");
}

bool TESTopic::Load(TESFile* pFile) {
    uint8_t cFormType = static_cast<uint8_t>(pFile->GetTESForm());
    QUEST_INFO* pQuestInfo = nullptr;
    if (cFormType != DIAL_ID) {
        return false;
    }
    LoadForm(pFile);

    for (BSSimpleList<QUEST_INFO*>* pIter = GetQuestInfoList(); pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        QUEST_INFO* pInfo = pIter->GetItem();
        if (pInfo) {
            pInfo->infoLinkArray.Clear(true);
        }
    }

    do {
        uint32_t uiChunk = pFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(pFile->GetChunkSize(), 128));
                pFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(pFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, pFile); break;
            case QSTI_ID: {
                uint32_t uiQuestID = 0;
                pFile->GetChunkData(uiQuestID);
                TESForm::AddCompileIndex(uiQuestID, GetFile(-1));
                // the quest's form ID stands in for the quest until the forms are resolved
                QUEST_INFO* pInfo = InsertQuest(reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(uiQuestID)), false);
                if (pInfo) {
                    pQuestInfo = pInfo;
                    uint32_t uiMaxSize = pInfo->GetInfoArray()->GetAllocatedSize();
                    pInfo->GetInfoArray()->SetSize(uiMaxSize + 100);
                    pInfo->GetInfoArray()->SetGrowBy(10);
                }
                break;
            }
            case INFC_ID: {
                uint32_t uiInfoID = 0;
                pFile->GetChunkData(uiInfoID);
                TESForm::AddCompileIndex(uiInfoID, GetFile(-1));
                if (pQuestInfo) {
                    INFO_LINK_ELEMENT kLink;
                    kLink.pInfo = nullptr;
                    kLink.nID = uiInfoID;
                    kLink.nDesiredIndex = -1;
                    pQuestInfo->infoLinkArray.Add(kLink);
                }
                break;
            }
            case INFX_ID: {
                int32_t iDesiredIndex = -1;
                pFile->GetChunkData(iDesiredIndex);
                if (pQuestInfo) {
                    int32_t iLast = pQuestInfo->infoLinkArray.GetSize() - 1;
                    if (iLast >= 0) {
                        pQuestInfo->infoLinkArray[iLast].nDesiredIndex = iDesiredIndex;
                        // Reads the size again and leaves the result unused, as the game does.
                        pQuestInfo->infoLinkArray.GetSize();
                        // a link added twice keeps its first slot: that slot takes the index and the new one goes
                        for (int32_t i = 0; i < iLast; ++i) {
                            INFO_LINK_ELEMENT& rLink = pQuestInfo->infoLinkArray[i];
                            INFO_LINK_ELEMENT& rLast = pQuestInfo->infoLinkArray[iLast];
                            if (rLink.nID == rLast.nID) {
                                pQuestInfo->infoLinkArray[i].nDesiredIndex = iDesiredIndex;
                                pQuestInfo->infoLinkArray.Remove(iLast, true);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case PNAM_ID: pFile->GetChunkData(m_fPriority); break;
            case TDUM_ID: {
                char cPrompt[256];
                pFile->GetChunkData(cPrompt, 0x100);
                SetDummyPrompt(cPrompt);
                break;
            }
            case DATA_ID: pFile->GetChunkData(&m_Data, 2); break;
            case XIDX_ID: pFile->GetChunkData(m_iJournalIndex); break;
        }
    }
    while (pFile->NextChunk());

    if (GetFormID() <= 0x7FF) {
        for (uint32_t i = 0; i < 8; ++i) {
            for (uint32_t j = 0; j < uiNumHardcodedTopicsPerCategory[i]; ++j) {
                if (pDefaultTopics[i][j].pTopic == this) {
                    m_Data.type = static_cast<uint8_t>(i);
                    SetFormEditorID(pDefaultTopics[i][j].pEdID);
                }
            }
        }
    }
    SetInitialized(false);
    pLastTopic = this;
    return true;
}

void TESTopic::Save() {
    TESForm::StartForm();
    TESForm::AddChunk(XIDX_ID, (uint32_t)m_iJournalIndex);
    TESTopic::pLastTopic = this;
    TESForm::CloseForm();
}

bool TESTopic::Save(TESFile* apFile) {
    Save();
    apFile->AddTESForm(this);
    const BSSimpleList<QUEST_INFO*>* pList = &m_listQuestInfo;
    while (pList && pList->m_item) {
        QUEST_INFO* pQuestInfo = pList->m_item;
        pList = pList->m_pkNext;
        TopicInfoArray* pArray = (&pQuestInfo->infoArray);
        uint32_t maxSize = pArray->m_uiSize;
        for (uint32_t i = 0; i < maxSize; ++i) {
            TESTopicInfo* info = pArray->GetAt(i);
            if (info) {
                bool Master = apFile->GetMaster();
                pDataHandler->SaveForm(info, Master);
            }
        }
    }
    return true;
}

[[clang::disable_tail_calls]] bool TESTopic::SaveEdit(TESFile* apFile) {
    return Save(apFile);
}

bool TESTopic::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    int32_t FormType = apForm->GetFormType();
    if (FormType == 69) {
        TESTopic* pothertopic = dynamic_cast<TESTopic*>(apForm);
        if (pothertopic) {
            uint32_t CastingType = iFormID;
            return CastingType < pothertopic->iFormID;
        }
    }
    else {
        if (FormType == 70) {
            TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apForm);
            if (pinfo) {
                TESTopic* ptopic = TESTopic::GetOwnerTopic(pinfo);
                if (ptopic) {
                    return ptopic == this || SavesBefore(ptopic);
                }
            }
        }
        else {
            return TESForm::SavesBefore(apForm);
        }
    }
    return bretval;
}

bool TESTopic::SavesBefore(FORM* apGroupFORM) {
    bool bretval = false;
    if (apGroupFORM) {
        if (apGroupFORM->form == formEnumString[2].uiFormString) {
            uint32_t form = apGroupFORM->form;
            switch (apGroupFORM->iFormID) {
                case 0u: {
                    bretval = TESForm::SavesBefore(apGroupFORM);
                    break;
                }
                case 1u:
                case 2u:
                case 3u:
                case 4u:
                case 5u:
                case 6u: {
                    bretval = false;
                    break;
                }
                case 7u: {
                    TESForm* FormByNumericID = TESForm::GetFormByNumericID(apGroupFORM->flags);
                    TESTopic* ptopic = dynamic_cast<TESTopic*>(FormByNumericID);
                    if (ptopic) {
                        bretval = SavesBefore(ptopic);
                    }
                    break;
                }
                default: {
                    return bretval;
                }
            }
        }
    }
    return bretval;
}

// Resolves each quest entry's form IDs (quest, removed quest and info links) to forms, drops links to infos that
// are already in the info array or did not load, then packs the info array and gives each info its quest and index.
void TESTopic::InitItem() {
    if (GetInitialized()) {
        return;
    }
    for (BSSimpleList<QUEST_INFO*>* pIter = GetQuestInfoList(); pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        QUEST_INFO* pInfo = pIter->GetItem();
        if (!pInfo) {
            continue;
        }
        if (!pInfo->bInitialized) {
            int32_t iLinkCount = pInfo->infoLinkArray.GetSize();
            for (int32_t i = 0; i < iLinkCount; ++i) {
                uint32_t uiInfoID = pInfo->infoLinkArray[i].nID;
                TESTopicInfo* pTopicInfo = dynamic_cast<TESTopicInfo*>(TESForm::GetFormByNumericID(uiInfoID));
                pInfo->infoLinkArray[i].pInfo = pTopicInfo;
                if (pTopicInfo) {
                    bool bFound = false;
                    for (uint32_t j = 0; j < pInfo->GetInfoArray()->GetSize(); ++j) {
                        if (pInfo->GetInfoArray()->GetAt(j) == pTopicInfo) {
                            bFound = true;
                            break;
                        }
                    }
                    if (bFound) {
                        pInfo->infoLinkArray.Remove(i, true);
                        --iLinkCount;
                        --i;
                    }
                }
                else {
                    // Looks the quest up and leaves the result unused, as the game does.
                    uint32_t uiQuestID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pInfo->pQuest));
                    FNVD_DYNAMIC_CAST_VOID(TESQuest)(
                        TESForm::GetFormByNumericID(uiQuestID), RTTI_TESForm, RTTI_TESQuest
                    );
                    pInfo->infoLinkArray.Remove(i, true);
                    --iLinkCount;
                    --i;
                }
            }
            // until here pQuest and pRemovedQuest hold form IDs
            if (pInfo->pQuest) {
                uint32_t uiQuestID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pInfo->pQuest));
                pInfo->pQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(uiQuestID));
            }
            else if (pInfo->pRemovedQuest) {
                uint32_t uiQuestID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pInfo->pRemovedQuest));
                TESForm::AddCompileIndex(uiQuestID, GetFile(-1));
                pInfo->pRemovedQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(uiQuestID));
            }
            else {
                int32_t iInfoCount = 0;
                if (pInfo->GetInfoArray()) {
                    iInfoCount = pInfo->GetInfoArray()->GetSize();
                }
                int32_t iLiveCount = 0;
                if (iInfoCount) {
                    for (int32_t j = 0; j < iInfoCount; ++j) {
                        TESTopicInfo* pTopicInfo = pInfo->GetInfoArray()->GetAt(j);
                        if (!pTopicInfo->GetDelete()) {
                            ++iLiveCount;
                        }
                    }
                }
                if (iLiveCount > 0) {
                    Warning(
                        "MASTERFILE: No Quest Reference on Topic \"%s\" (%d non-deleted infos attached to this "
                        "quest).",
                        GetFormEditorID(), iLiveCount
                    );
                }
            }
            pInfo->bInitialized = true;
        }
        NiTLargePrimitiveArray<TESTopicInfo*>* pArray = pInfo->GetInfoArray();
        if (pArray) {
            pArray->Compact();
            pArray->SetSize(pArray->GetSize());
            uint32_t uiCount = pArray->GetSize();
            for (uint32_t k = 0; k < uiCount; ++k) {
                TESTopicInfo* pTopicInfo = pArray->GetAt(k);
                if (pTopicInfo) {
                    pTopicInfo->SetOwnerQuest(pInfo->pQuest);
                    static_cast<TESForm*>(pTopicInfo)->InitItem();
                    pTopicInfo->SetInfoIndex(static_cast<uint16_t>(k));
                }
            }
        }
    }
    if (IsRandom()) {
        pPlayer->AddTopic(this, false, false);
    }
    SetInitialized(true);
}

bool TESTopic::IsFormTypeChild(uint8_t aeType) {
    return TESTopic::IsFormTypeDialogueChild(aeType);
}

const char* TESTopic::GetFormEditorID() {
    if (strEditorID.String()) {
        return strEditorID.String();
    }
    return "";
}

// The editor ID is copied to a stack buffer first, so apEditorID may point into the string itself.
bool TESTopic::SetFormEditorID(const char* apEditorID) {
    uint32_t uiSize = BSstrlen(apEditorID) + 1;
    // A 16-byte aligned stack buffer.
    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
    BSstrcpy(pBuffer, uiSize, apEditorID);
    if (!strEditorID.StrCmp(pBuffer, true)) {
        return false;
    }
    strEditorID.Set(pBuffer, 0);
    return true;
}

TESTopicInfo* TESTopic::GetMatchingInfo(bool& abDispositionFailure, TESObjectREFR* apSpeaker, TESObjectREFR* apTarget) {
    return GetMatchingInfo(abDispositionFailure, apSpeaker, apTarget, false, nullptr, nullptr);
}

TESTopicInfo* TESTopic::GetMatchingInfo(TESObjectREFR* apSpeaker, TESObjectREFR* apTarget) {
    bool bDispositionFailure = false;
    TESTopicInfo* matchingInfo_0 = GetMatchingInfo(bDispositionFailure, apSpeaker, apTarget);
    if (bDispositionFailure) {
        return nullptr;
    }
    else {
        return matchingInfo_0;
    }
}

TESTopicInfo* TESTopic::GetMatchingInfo(
    bool& abDispositionFailure,
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    bool abIsConversation,
    TESTopic* apPreviousTopic,
    BSSimpleList<DialogueItem*>* apConversationList
) {
    return TESTopic::GetMatchingInfo(
        this, abDispositionFailure, apSpeaker, apTarget, abIsConversation, apPreviousTopic, apConversationList
    );
}

TESQuest* TESTopic::GetOwnerQuest(TESTopicInfo* apInfo) {
    if (!apInfo) {
        return nullptr;
    }
    if (IsRemappedRumors()) {
        TESTopic* Topic = TESTopic::GetTopic(DT_CONVERSATION, 5);
        return Topic->GetOwnerQuest(apInfo);
    }
    else {
        const BSSimpleList<QUEST_INFO*>* pList = &m_listQuestInfo;
        while (pList && pList->m_item) {
            QUEST_INFO* pQuestInfo = pList->m_item;
            pList = pList->m_pkNext;
            uint32_t maxSize = (&pQuestInfo->infoArray)->m_uiSize;
            if (maxSize) {
                for (uint32_t i = 0; i < maxSize; ++i) {
                    if (TESTopic::GetInfoByIndex(pQuestInfo, i) == apInfo) {
                        return pQuestInfo->pQuest;
                    }
                }
            }
        }
        return nullptr;
    }
}

bool TESTopic::HasQuest(TESQuest* apQuest, bool bRequireInfos) {
    QUEST_INFO* v6;
    if (!apQuest) {
        return false;
    }
    BSSimpleList<QUEST_INFO*>* pIter = GetQuestInfoList();
    do {
        if (!pIter || !pIter->m_item) {
            return false;
        }
        v6 = pIter->m_item;
        pIter = pIter->m_pkNext;
    }
    while (v6->pQuest != apQuest);
    NiTLargePrimitiveArray<TESTopicInfo*>* ActorPackageThatIsRunning = v6->GetInfoArray();
    return ActorPackageThatIsRunning->GetSize() || !bRequireInfos;
}

// Bubble sort by quest priority, highest first; a missing quest counts as priority -1.
bool TESTopic::SortQuests() {
    InitItem();
    int32_t iCount = GetQuestInfoList()->ItemsInList();
    for (int32_t i = 0; i < iCount - 1; ++i) {
        for (BSSimpleList<QUEST_INFO*>* pIter = GetQuestInfoList(); pIter && pIter->GetItem();
             pIter = pIter->GetNext()) {
            if (!pIter->GetNext()) {
                continue;
            }
            QUEST_INFO* pFirst = pIter->GetItem();
            QUEST_INFO* pSecond = pIter->GetNext()->GetItem();
            if (!pFirst || !pSecond) {
                continue;
            }
            int32_t iFirstPriority = pFirst->pQuest ? pFirst->pQuest->GetPriority() : -1;
            int32_t iSecondPriority = pSecond->pQuest ? pSecond->pQuest->GetPriority() : -1;
            if (iSecondPriority > iFirstPriority) {
                pIter->SetItem(pSecond);
                pIter->GetNext()->SetItem(pFirst);
            }
        }
    }
    return true;
}

BSSimpleList<QUEST_INFO*>* TESTopic::GetQuestInfoList() {
    return &m_listQuestInfo;
}

TopicInfoArray* TESTopic::GetInfoArray(TESQuest* apQuest) {
    BSSimpleList<QUEST_INFO*>* pList = &m_listQuestInfo;
    while (pList && pList->m_item) {
        QUEST_INFO* pQuestInfo = pList->m_item;
        pList = pList->m_pkNext;
        if (pQuestInfo->pQuest == apQuest && (apQuest || !pQuestInfo->pRemovedQuest)) {
            return (&pQuestInfo->infoArray);
        }
    }
    return nullptr;
}

QUEST_INFO* TESTopic::GetQuestInfoElement(TESTopicInfo* apInfo) {
    NiTLargePrimitiveArray<TESTopicInfo*>* v2;
    uint32_t a2;
    QUEST_INFO* a1;
    uint32_t v7;
    BSSimpleList<QUEST_INFO*>* ecx0 = TESTopic::GetQuestInfoList();
    while (ecx0 && ecx0->m_item) {
        a1 = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        v2 = a1->QUEST_INFO::GetInfoArray();
        v7 = v2->m_uiSize;
        if (v7) {
            for (a2 = 0; a2 < v7; ++a2) {
                if (TESTopic::GetInfoByIndex(a1, a2) == apInfo) {
                    return a1;
                }
            }
        }
    }
    return nullptr;
}

void TESTopic::SetDummyPrompt(const char* apPrompt) {
    cDummyPrompt.Set(apPrompt, 0);
}

// While the quests are unresolved (bInitialized clear) the entries and apQuest hold form IDs; afterwards the quests'
// form IDs are still compared with apQuest's raw value, as the game does, hence the pointer-to-integer casts. abReSort
// is unused.
QUEST_INFO* TESTopic::InsertQuest(TESQuest* apQuest, [[maybe_unused]] bool abReSort) {
    if (!apQuest) {
        return nullptr;
    }

    BSSimpleList<QUEST_INFO*>* pIter = GetQuestInfoList();
    QUEST_INFO* pInfo = nullptr;
    while (pIter && pIter->GetItem() && !pInfo) {
        pInfo = pIter->GetItem();
        pIter = pIter->GetNext();
        if (pInfo->bInitialized) {
            uint32_t uiQuestValue = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apQuest));
            if ((!pInfo->pQuest || pInfo->pQuest->GetFormID() != uiQuestValue) &&
                (!pInfo->pRemovedQuest || pInfo->pRemovedQuest->GetFormID() != uiQuestValue)) {
                pInfo = nullptr;
            }
        }
        else if (pInfo && pInfo->pQuest != apQuest && pInfo->pRemovedQuest != apQuest) {
            pInfo = nullptr;
        }
    }

    if (!pInfo) {
        void* pMemory = ourMalloc(sizeof(QUEST_INFO));
        pInfo = pMemory ? ::new (pMemory) QUEST_INFO : nullptr;
        pInfo->pQuest = apQuest;
        GetQuestInfoList()->AddTail(pInfo);
    }
    return pInfo;
}

float TESTopic::GetPriority() {
    return m_fPriority;
}

void TESTopic::SetPriority(float fP) {
    m_fPriority = fP;
}

bool TESTopic::IsRandom() {
    return (m_Data.cFlags & 2) != 0;
}

DialogueItem* TESTopic::CreateDialogueItem(
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    TESTopic* apPreviousTopic,
    BSSimpleList<DialogueItem*>* apConversationList,
    bool abRunImmediate
) {
    DialogueItem* v12;
    bool abDispositionFailure;
    TESTopicInfo* apTopicInfo =
        GetMatchingInfo(abDispositionFailure, apSpeaker, apTarget, true, apPreviousTopic, apConversationList);
    if (!apTopicInfo) {
        return nullptr;
    }
    DialogueItem* apMemory = static_cast<DialogueItem*>(ourMalloc(sizeof(DialogueItem)));
    if (apMemory) {
        TESQuest* CombatStyle = apTopicInfo->pOwnerQuest;
        v12 = ::new (apMemory) DialogueItem(CombatStyle, this, apTopicInfo, apSpeaker);
    }
    else {
        v12 = nullptr;
    }
    if (abRunImmediate) {
        if (v12) {
            if (v12->GetInfo()) {
                TESTopicInfo* CastingType = v12->GetInfo();
                if (CastingType->IsRunImmediately()) {
                    TESTopicInfo* v8 = v12->GetInfo();
                    v8->RunResult(TIRS_BEGIN, apSpeaker);
                    TESTopicInfo* v9 = v12->GetInfo();
                    v9->AddTopicList();
                }
            }
        }
    }
    return v12;
}

int32_t TESTopic::HasLowIntDialog() {
    return (int32_t)((uint8_t)((&cDummyPrompt)->GetLength() != 0));
}

DIALOGUE_TYPE TESTopic::GetDialogueType() const {
    return static_cast<DIALOGUE_TYPE>((int8_t)m_Data.type);
}

TESTopicInfo* TESTopic::GetInfoByID(uint32_t aiInfoID, bool abReverse) {
    NiTLargePrimitiveArray<TESTopicInfo*>* pInfoArray;
    uint32_t j;
    TESTopicInfo* pTopicInfo;
    int32_t i;
    QUEST_INFO* pItem;
    uint32_t uiInfoCount;
    BSSimpleList<QUEST_INFO*>* pIter = TESTopic::GetQuestInfoList();
    while (pIter && pIter->m_item) {
        pItem = pIter->m_item;
        pIter = pIter->m_pkNext;
        pInfoArray = pItem->QUEST_INFO::GetInfoArray();
        uiInfoCount = pInfoArray->m_uiSize;
        if (uiInfoCount) {
            if (abReverse) {
                for (i = (int32_t)(uiInfoCount - 1); i >= 0; --i) {
                    pTopicInfo = TESTopic::GetInfoByIndex(pItem, i);
                    if (pTopicInfo && (pTopicInfo->TESForm::GetFormID() == aiInfoID)) {
                        return pTopicInfo;
                    }
                }
            }
            else {
                for (j = 0; j < uiInfoCount; ++j) {
                    pTopicInfo = TESTopic::GetInfoByIndex(pItem, j);
                    if (pTopicInfo && (pTopicInfo->TESForm::GetFormID() == aiInfoID)) {
                        return pTopicInfo;
                    }
                }
            }
        }
    }
    return nullptr;
}

bool TESTopic::IsPersuasion() {
    return (bool)((uint8_t)((TESForm::GetFormID() == 253) || (TESForm::GetFormID() == 280)));
}

bool TESTopic::IsRemappedRumors() const {
    return (m_Data.cFlags & 1) != 0;
}

bool TESTopic::InsertInfo(TESQuest* apQuest, TESTopicInfo* apInfo, uint32_t auiPrevInfoID) {
    if (!apQuest || !apInfo) {
        return false;
    }
    QUEST_INFO* pQuestInfo = InsertQuest(apQuest, false);
    if (auiPrevInfoID) {
        if (auiPrevInfoID != -1 && apInfo->GetInfoIndex() != 0xFFFF) {
            TESTopicInfo* pNoInfo = nullptr;
            pQuestInfo->infoArray.SetAt(apInfo->GetInfoIndex(), pNoInfo);
        }
        if (auiPrevInfoID == -1) {
            if (apInfo->GetInfoIndex() == 0xFFFF) {
                int32_t lastspot;
                for (lastspot = pQuestInfo->infoArray.m_uiSize - 1;
                     lastspot > 0 && !TESTopic::GetInfoByIndex(pQuestInfo, static_cast<uint32_t>(lastspot));
                     --lastspot) {
                    ;
                }
                apInfo->SetInfoIndex(++lastspot);
                pQuestInfo->infoArray.SetAtGrow(lastspot, apInfo);
            }
        }
        else {
            TESTopicInfo* pPrevInfo = GetInfoByID(auiPrevInfoID, true);
            if (pPrevInfo && auiPrevInfoID) {
                int32_t iinsertindex = -1;
                if (pPrevInfo->GetInfoIndex() == 0xFFFF) {
                    for (uint32_t iindex = 0; iindex < pQuestInfo->infoArray.m_uiSize; ++iindex) {
                        if (pQuestInfo->infoArray.GetAt(iindex) == pPrevInfo) {
                            iinsertindex = iindex + 1;
                            break;
                        }
                    }
                }
                else {
                    iinsertindex = pPrevInfo->GetInfoIndex() + 1;
                }
                pQuestInfo->infoArray.InsertInfoAt(iinsertindex, apInfo);
            }
            else {
                int32_t v10 = TESTopic::pLastTopic->iFormID;
                const char* v9 = TESTopic::pLastTopic->GetFormEditorID();
                int32_t v8 = apInfo->iFormID;
                Warning(
                    "QUESTS: Could not find previous info (%08X) for TopicInfo (%08X) in Topic \"%s\" (%08X).",
                    auiPrevInfoID, v8, v9, v10
                );
                pQuestInfo->infoArray.InsertInfoAt(0, apInfo);
            }
        }
    }
    else {
        pQuestInfo->infoArray.InsertInfoAt(0, apInfo);
    }
    pQuestInfo->infoArray.Compact();
    TESTopic::SetTopicIndices(&pQuestInfo->infoArray);
    return true;
}

void TESTopic::ClearSaidOnceInfosForQuest(TESQuest* apQuest) {
    TopicInfoArray* pInfoArray = GetInfoArray(apQuest);
    if (pInfoArray) {
        uint32_t uiSize = pInfoArray->m_uiSize;
        for (uint32_t uiIndex = 0; uiIndex < uiSize; ++uiIndex) {
            TESTopicInfo* pInfo = pInfoArray->GetAt(uiIndex);
            if (pInfo && pInfo->IsSayOnce() && pInfo->bSaidOnce) {
                pInfo->ResetSaidOnceFlags();
            }
        }
    }
}

TESTopic* TESTopic::GetTopic(DIALOGUE_TYPE type, int32_t index) {
    if (index < 0) {
        return nullptr;
    }
    if (index < TESTopic::uiNumHardcodedTopicsPerCategory[type]) {
        return TESTopic::pDefaultTopics[type][index].pTopic;
    }
    return nullptr;
}

bool TESTopic::SortQuestsOnAllTopics(TESQuest* apQuest) {
    bool bResult = true;
    for (auto* pIter = pDataHandler->GetTopicList(); pIter && pIter->GetItem() && bResult; pIter = pIter->GetNext()) {
        if (!apQuest || static_cast<TESTopic*>(pIter->GetItem())->HasQuest(apQuest, false)) {
            bResult = static_cast<TESTopic*>(pIter->GetItem())->SortQuests();
        }
    }
    return bResult;
}

TESTopic* TESTopic::GetOwnerTopic(TESTopicInfo* pInfo) {
    BSSimpleList<TESTopic*>* ecx0 = pDataHandler->GetTopicList();
    while (ecx0 && ecx0->m_item) {
        TESTopic* th_s = ecx0->m_item;
        ecx0 = ecx0->m_pkNext;
        uint32_t CastingType = pInfo->GetFormID();
        if (th_s->GetInfoByID(CastingType, false) == pInfo) {
            return th_s;
        }
    }
    return nullptr;
}

void TESTopic::SortTopics(bool abSortByPriority, BSSimpleList<TESTopic*>* apTopicList) {
    BSSimpleList<TESTopic*>* pBaseList = apTopicList;
    if (!apTopicList) {
        pBaseList = pDataHandler->GetTopicList();
    }
    BSSimpleList<TESTopic*> sortlist;
    BSSimpleList<TESTopic*>* plist = pBaseList;
    BSSimpleList<TESTopic*>* plastlist = nullptr;
    while (plist && plist->m_item) {
        bool badvanced = false;
        if (plist->m_pkNext) {
            TESTopic* ptopic1 = plist->m_item;
            TESTopic* apTopic2 = plist->m_pkNext->m_item;
            if (TESTopic::CompareTopics(ptopic1, apTopic2, abSortByPriority)) {
                if (plastlist) {
                    plastlist->Remove(ptopic1);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
                sortlist.AddHead(ptopic1);
                badvanced = true;
            }
        }
        if (!badvanced) {
            plastlist = plist;
            plist = plist->m_pkNext;
        }
    }
    while (!sortlist.IsEmpty()) {
        TESTopic* ptopic = (&sortlist)->m_item;
        sortlist.RemoveHead();
        TESTopic::AddTopicToSortedList(ptopic, pBaseList, abSortByPriority);
    }
}

void TESTopic::AddDefaultTopics() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_DIALOGUE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTopic.cpp", 0x1573
    );
    for (DIALOGUE_TYPE topic = DT_TOPIC; topic < DIALOGUE_TYPE_COUNT; topic = static_cast<DIALOGUE_TYPE>(topic + 1)) {
        for (int32_t n = 0; n < TESTopic::uiNumHardcodedTopicsPerCategory[topic]; ++n) {
            if (!TESTopic::pDefaultTopics[topic][n].pTopic) {
                TESTopic* v0;
                TESTopic* v1 = static_cast<TESTopic*>(ourMalloc(sizeof(TESTopic)));
                if (v1) {
                    v0 = ::new (v1) TESTopic(topic);
                }
                else {
                    v0 = nullptr;
                }
                TESTopic::pDefaultTopics[topic][n].pTopic = v0;
                TESTopic::pDefaultTopics[topic][n].pTopic->SetFormID(TESTopic::pDefaultTopics[topic][n].iID, true);
                TESTopic::pDefaultTopics[topic][n].pTopic->SetFormEditorID(TESTopic::pDefaultTopics[topic][n].pEdID);
                TESTopic::pDefaultTopics[topic][n].pTopic->SetFullName(TESTopic::pDefaultTopics[topic][n].pEdID);
                pDataHandler->AddTopic(TESTopic::pDefaultTopics[topic][n].pTopic);
                TESForm::pAllFormsByEditorID->SetAt(
                    TESTopic::pDefaultTopics[topic][n].pEdID, TESTopic::pDefaultTopics[topic][n].pTopic
                );
            }
        }
    }
}

void TESTopic::AddTopicToSortedList(TESTopic* apTopic, BSSimpleList<TESTopic*>* apTopicList, bool abUsePriority) {
    if (!apTopic || !apTopicList) {
        return;
    }

    if (apTopicList->IsEmpty()) {
        apTopicList->AddHead(apTopic);
        return;
    }

    for (BSSimpleList<TESTopic*>* pIter = apTopicList; pIter && !pIter->IsEmpty();) {
        TESTopic* pTopic = pIter->GetItem();
        if (pTopic == apTopic) {
            return;
        }
        if (CompareTopics(pTopic, apTopic, abUsePriority)) {
            pIter->AddHead(apTopic);
            return;
        }
        if (!pIter->GetNext()) {
            pIter->AddTail(apTopic);
            return;
        }
        pIter = pIter->GetNext();
    }
}

// The priorities' difference is rounded to a float, then compared with the double 0.0001.
bool TESTopic::CompareTopics(TESTopic* apTopic1, TESTopic* apTopic2, bool abUsePriority) {
    if (!apTopic1 || !apTopic2) {
        return false;
    }
    const char* pName1 = apTopic1->GetFormEditorID();
    const char* pName2 = apTopic2->GetFormEditorID();
    if (abUsePriority) {
        float fDiff = apTopic2->GetPriority() - apTopic1->GetPriority();
        if (fDiff > 0.0001) {
            return true;
        }
        if (!FloatEQ(fDiff, 0.0f, 0.001f)) {
            return false;
        }
    }
    if (pName1[0] < pName2[0]) {
        return true;
    }
    const char* pRight = apTopic2->GetFormEditorID();
    const char* pLeft = apTopic1->GetFormEditorID();
    return BSstricmp(pLeft, pRight) > 0;
}

TESTopicInfo* TESTopic::GetInfoByIndex(QUEST_INFO* apQuestInfo, uint32_t aiInfoIndex) {
    if (!apQuestInfo) {
        return nullptr;
    }
    if (aiInfoIndex < apQuestInfo->GetInfoArray()->GetSize()) {
        return apQuestInfo->GetInfoArray()->GetAt(aiInfoIndex);
    }
    return nullptr;
}

void TESTopic::SetShowConversationStats(bool abShow) {
    bIsConversationStats = (bool)((int8_t)abShow);
}

bool TESTopic::GetShowConversationStats() {
    return (bool)((int8_t)bIsConversationStats);
}

bool TESTopic::IsFormTypeDialogueChild(uint8_t aeType) {
    return aeType == INFO_ID;
}

void TESTopic::ClearAllSaidOnceInfosForQuest(TESQuest* apQuest) {
    if (apQuest) {
        for (BSSimpleList<TESTopic*>* plist = pDataHandler->GetTopicList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESTopic** v1 = &plist->m_item;
            (*v1)->ClearSaidOnceInfosForQuest(apQuest);
        }
    }
}

void TESTopic::SetTopicIndices(NiTLargePrimitiveArray<TESTopicInfo*>* apInfoArray) {
    if (apInfoArray) {
        uint32_t uiSize = apInfoArray->GetSize();
        for (uint32_t uiIndex = 0; uiIndex < uiSize; ++uiIndex) {
            TESTopicInfo* pInfo = apInfoArray->GetAt(uiIndex);
            if (pInfo) {
                pInfo->SetInfoIndex(uiIndex);
            }
        }
    }
}

void TESTopic::RemoveDefaultTopics() {
    for (int32_t i = 0; i < 8; ++i) {
        for (int32_t j = 0; j < TESTopic::uiNumHardcodedTopicsPerCategory[i]; ++j) {
            TESTopic::pDefaultTopics[i][j].pTopic = nullptr;
        }
    }
}

TESTopicInfo* TESTopic::GetMatchingInfo(
    const TESTopic* apTopic,
    bool& abDispositionFailure,
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    bool abIsConversation,
    TESTopic* apPreviousTopic,
    BSSimpleList<DialogueItem*>* apConversationList
) {
    const TESTopic* pchecktopic = apTopic;
    if (apTopic) {
        if (pchecktopic->IsRemappedRumors()) {
            pchecktopic = TESTopic::GetTopic(DT_CONVERSATION, 5);
        }
    }
    abDispositionFailure = false;
    if (!pchecktopic || pchecktopic->GetDelete()) {
        return nullptr;
    }
    TESTopicInfo* pUsedInfo = nullptr;
    BSSimpleList<TESTopicInfo*> matchinginfolist;
    uint32_t iinfocount = 0;
    bool bFinished = false;
    const BSSimpleList<QUEST_INFO*>* pList = &pchecktopic->m_listQuestInfo;
    TESTopicInfo* pInfoRefusal = nullptr;
    while (pList && (pList->m_item && !bFinished)) {
        QUEST_INFO* pQuestInfo = pList->m_item;
        TESQuest* pquest = pQuestInfo->pQuest;
        pList = pList->m_pkNext;
        if (pquest && (pquest->GetEnabled() && (&pquest->objConditions)->IsTrue(apSpeaker, apTarget))) {
            TopicInfoArray BuiltArray;
            TopicInfoArray* pinfoarray = &pQuestInfo->infoArray;
            uint32_t maxinfoSize = pinfoarray->m_uiSize;
            for (uint32_t infoiter = 0; infoiter < maxinfoSize; ++infoiter) {
                BuiltArray.Add(pinfoarray->GetAt(infoiter));
            }
            uint32_t maxLinkSize = pQuestInfo->infoLinkArray.iSize;
            for (uint32_t infolinkiter = 0; infolinkiter < maxLinkSize; ++infolinkiter) {
                if ((&pQuestInfo->infoLinkArray.operator[](infolinkiter))->pInfo) {
                    uint32_t CastingType;
                    int32_t insertIdx = (&pQuestInfo->infoLinkArray.operator[](infolinkiter))->nDesiredIndex;
                    if (insertIdx < 0 || (CastingType = BuiltArray.m_uiSize, insertIdx > CastingType)) {
                        insertIdx = BuiltArray.m_uiSize;
                    }
                    TESTopicInfo* pInfo = (&pQuestInfo->infoLinkArray.operator[](infolinkiter))->pInfo;
                    BuiltArray.InsertInfoAt(insertIdx, pInfo);
                }
            }
            uint32_t iter = 0;
            TESTopicInfo* info = nullptr;
            uint32_t v9 = BuiltArray.m_uiSize;
            if (v9 > iter) {
            LABEL_26:;
                info = BuiltArray.GetAt(iter);
            }
            while (info) {
                bool bDispositionInfoFailure = false;
                bool bNotSaidOnce = true;
                if (info->IsSayOnceADay() && apSpeaker->IsMobileObject()) {
                    bNotSaidOnce = !apSpeaker->HasSaidTopicInfo(info);
                }
                if (info &&
                    (bNotSaidOnce && info->CheckCondition(bDispositionInfoFailure, nullptr, apSpeaker, apTarget))) {
                    abDispositionFailure = false;
                    bool bOK = true;
                    if (abIsConversation || (apPreviousTopic && apPreviousTopic->GetDialogueType() == DT_TOPIC)) {
                        TESConversationData* pconvdata = info->GetConversationData();
                        if (pconvdata) {
                            if (apPreviousTopic) {
                                BSSimpleList<TESTopic*>* pLinkFromList;
                                bOK = false;
                                for (pLinkFromList = &pconvdata->m_listLinkFrom;
                                     pLinkFromList && !pLinkFromList->IsEmpty();
                                     pLinkFromList = pLinkFromList->m_pkNext) {
                                    if (pLinkFromList->m_item == apPreviousTopic ||
                                        pLinkFromList->m_item->iFormID == 211) {
                                        bOK = true;
                                        break;
                                    }
                                }
                                if (!abIsConversation) {
                                    if (pLinkFromList) {
                                        if (pLinkFromList->IsEmpty()) {
                                            bOK = true;
                                        }
                                    }
                                }
                            }
                            else {
                                if (!pconvdata->m_listLinkFrom.IsEmpty() && apTarget != pPlayer) {
                                    bOK = false;
                                }
                            }
                        }
                        if (bOK) {
                            if (info->IsRandomEnd()) {
                                bFinished = true;
                            }
                            for (BSSimpleList<DialogueItem*>* pList_2 = apConversationList;
                                 pList_2 && !pList_2->IsEmpty(); pList_2 = pList_2->m_pkNext) {
                                DialogueItem* pItem = pList_2->m_item;
                                TESTopicInfo* v10 = pItem->pInfo;
                                if (v10 == info) {
                                    pUsedInfo = info;
                                    bOK = false;
                                    break;
                                }
                                if (!pQuestInfo->pQuest->GetAllowRepeats()) {
                                    TESQuest* Time = pItem->pQuest;
                                    if (Time == pQuestInfo->pQuest && pItem->pTopic == apTopic) {
                                        pUsedInfo = info;
                                        bOK = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (bOK) {
                        if (info->IsRandomEnd()) {
                            bFinished = true;
                        }
                        if (matchinginfolist.IsEmpty() && !info->IsRandom()) {
                            TESTopicInfo* v16 = info;
                            return v16;
                        }
                        if (!info->IsRandom()) {
                            bFinished = true;
                            break;
                        }
                        matchinginfolist.AddHead(info);
                        ++iinfocount;
                    }
                }
                if (!abIsConversation && bDispositionInfoFailure) {
                    pInfoRefusal = info;
                    abDispositionFailure = true;
                }
                if (bFinished) {
                    break;
                }
                ++iter;
                uint32_t v12 = BuiltArray.m_uiSize;
                if (iter < v12) {
                    goto LABEL_26;
                }
                info = nullptr;
            }
        }
    }
    if (matchinginfolist.IsEmpty()) {
        if (abIsConversation) {
            return nullptr;
        }
        else {
            return pInfoRefusal;
        }
    }
    else {
        int32_t v14;
        if (iinfocount) {
            v14 = Random() % iinfocount + 1;
        }
        else {
            v14 = 0;
        }
        int32_t count = 0;
        BSSimpleList<TESTopicInfo*>* pRandomList = &matchinginfolist;
        TESTopicInfo* v17 = nullptr;
        while (count < v14 && pRandomList) {
            TESTopicInfo* m_item = pRandomList->m_item;
            if (m_item) {
                v17 = m_item;
                ++count;
            }
            pRandomList = pRandomList->m_pkNext;
        }
        matchinginfolist.RemoveAll();
        return v17;
    }
}

TESTopicInfo* TESTopic::GetMatchingFollowUpInfo(
    TESTopicInfo* apTopicInfo,
    bool& abDispositionFailure,
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    bool abIsConversation,
    BSSimpleList<DialogueItem*>* apConversationList
) {
    abDispositionFailure = false;
    if (!apTopicInfo || apTopicInfo->GetDelete()) {
        return nullptr;
    }
    TESTopicInfo* pUsedInfo = nullptr;
    BSSimpleList<TESTopicInfo*> matchinginfolist;
    uint32_t iinfocount = 0;
    bool bFinished = false;
    const BSSimpleList<TESTopicInfo*>* pList = nullptr;
    if (apTopicInfo->GetConversationData()) {
        pList = apTopicInfo->GetConversationData()->GetFollowUpInfoList();
    }
    TESTopicInfo* pInfoRefusal = nullptr;
    while (pList && (pList->m_item && !bFinished)) {
        TESTopicInfo* pinfo = pList->m_item;
        TESQuest* pquest = pinfo->pOwnerQuest;
        pList = pList->m_pkNext;
        if (pquest) {
            if (pquest->GetEnabled() && (&pquest->objConditions)->IsTrue(apSpeaker, apTarget)) {
                bool bDispositionInfoFailure = false;
                bool bNotSaidOnce = true;
                if (pinfo) {
                    if (pinfo->IsSayOnceADay() && apSpeaker->IsMobileObject()) {
                        bNotSaidOnce = !apSpeaker->HasSaidTopicInfo(pinfo);
                    }
                }
                if (pinfo &&
                    (bNotSaidOnce && pinfo->CheckCondition(bDispositionInfoFailure, nullptr, apSpeaker, apTarget))) {
                    abDispositionFailure = false;
                    bool bOK = true;
                    if (pinfo->IsRandomEnd()) {
                        bFinished = true;
                    }
                    for (BSSimpleList<DialogueItem*>* pList_2 = apConversationList; pList_2 && !pList_2->IsEmpty();
                         pList_2 = pList_2->m_pkNext) {
                        TESTopicInfo* Info = pList_2->m_item->pInfo;
                        if (Info == pinfo) {
                            pUsedInfo = pinfo;
                            bOK = false;
                            break;
                        }
                    }
                    if (bOK) {
                        if (pinfo->IsRandomEnd()) {
                            bFinished = true;
                        }
                        if (matchinginfolist.IsEmpty() && !pinfo->IsRandom()) {
                            TESTopicInfo* v10 = pinfo;
                            return v10;
                        }
                        if (!pinfo->IsRandom()) {
                            bFinished = true;
                            break;
                        }
                        matchinginfolist.AddHead(pinfo);
                        ++iinfocount;
                    }
                    if (!abIsConversation && bDispositionInfoFailure) {
                        pInfoRefusal = pinfo;
                        abDispositionFailure = true;
                    }
                    if (bFinished) {
                        break;
                    }
                }
            }
        }
    }
    if (matchinginfolist.IsEmpty()) {
        if (abIsConversation) {
            return nullptr;
        }
        else {
            return pInfoRefusal;
        }
    }
    else {
        int32_t dieRoll;
        if (iinfocount) {
            dieRoll = Random() % iinfocount + 1;
        }
        else {
            dieRoll = 0;
        }
        int32_t count = 0;
        BSSimpleList<TESTopicInfo*>* pRandomList = &matchinginfolist;
        TESTopicInfo* v11 = nullptr;
        while (count < dieRoll && pRandomList) {
            TESTopicInfo* v13 = pRandomList->m_item;
            if (v13) {
                v11 = v13;
                ++count;
            }
            pRandomList = pRandomList->m_pkNext;
        }
        matchinginfolist.RemoveAll();
        return v11;
    }
}

uint32_t TESTopic::CreateConversation(
    TESObjectREFR* apSpeaker,
    TESObjectREFR* apTarget,
    BSSimpleList<DialogueItem*>* apConversationList,
    TESTopic* apStartingTopic
) {
    if (!apConversationList) {
        return 0;
    }
    while (apConversationList && !apConversationList->IsEmpty()) {
        DialogueItem* pItem = apConversationList->m_item;
        if (pItem) {
            delete pItem;
        }
        apConversationList->RemoveHead();
    }
    if (!apSpeaker || !apTarget) {
        return 0;
    }
    TESObjectREFR* pCurrentSpeaker = apSpeaker;
    TESObjectREFR* pCurrentTarget = apTarget;
    TESTopic* pLastTopic = nullptr;
    TESForm* pspeakerForm = apSpeaker->GetObjectReference();
    if (pspeakerForm) {
        if (pspeakerForm->GetFormType() == CREA_ID) {
            // A creature without a voice type has no conversation.
            TESActorBaseData* pActorData = static_cast<TESCreature*>(pspeakerForm);
            if (!pActorData->GetVoiceType()) {
                return 0;
            }
        }
    }
    TESTopic* pNextTopic = apStartingTopic;
    if (!apStartingTopic) {
        pNextTopic = pDataHandler->GetGlobalTopic(kTopicHELLO);
    }
    rt::srand(static_cast<uint32_t>(std::time(nullptr)));
    while (pNextTopic) {
        int32_t apDest[500];
        // The count is compared unsigned.
        if (static_cast<uint32_t>(apConversationList->ItemsInList()) >= 100) {
            break;
        }
        DialogueItem* pConvItem =
            pNextTopic->CreateDialogueItem(pCurrentSpeaker, pCurrentTarget, pLastTopic, apConversationList, false);
        if (!pConvItem) {
            if (!apStartingTopic || apStartingTopic->GetDialogueType() == DT_CONVERSATION) {
                // The goodbye topic replaces pNextTopic: it becomes pLastTopic below.
                pNextTopic = pDataHandler->GetGlobalTopic(kTopicGOODBYE);
                pConvItem = pNextTopic->CreateDialogueItem(
                    pCurrentSpeaker, pCurrentTarget, pLastTopic, apConversationList, false
                );
            }
            if (!pConvItem) {
                break;
            }
        }
        if (pConvItem->pInfo) {
            if (pConvItem->pInfo->IsRunImmediately()) {
                pConvItem->pInfo->RunResult(TIRS_BEGIN, apSpeaker);
                pConvItem->pInfo->RunResult(TIRS_END, apSpeaker);
                pConvItem->pInfo->AddTopicList();
            }
        }
        apConversationList->AddTail(pConvItem);
        pLastTopic = pNextTopic;
        pNextTopic = nullptr;
        DialogueItem* pValidDiagItem = nullptr;
        bool bAllTried = false;
        ourMemset(apDest, 0, sizeof(apDest));
        int32_t iNextTopic = -1;
        while (!pValidDiagItem && !bAllTried) {
            if (pConvItem->pInfo && pConvItem->pInfo->GetConversationData()) {
                BSSimpleList<TESTopic*>* pList = &pConvItem->pInfo->GetConversationData()->m_listLinkTo;
                int32_t iTotal = pList->ItemsInList();
                if (iTotal) {
                    if (iNextTopic == -1) {
                        iNextTopic = rt::rand() % iTotal + 1;
                    }
                    else {
                        while (apDest[iNextTopic - 1]) {
                            iNextTopic = rt::rand() % iTotal + 1;
                        }
                    }
                }
                if (iNextTopic > 0) {
                    apDest[iNextTopic - 1] = 1;
                }
                for (int32_t iCount = 0; iCount < iNextTopic && (pList && !pList->IsEmpty()); ++iCount) {
                    pNextTopic = pList->m_item;
                    pList = pList->m_pkNext;
                }
                DIALOGUE_SPEAKER eNextSpeaker = pConvItem->pInfo->GetNextSpeaker();
                if (eNextSpeaker == DS_TARGET || (eNextSpeaker == DS_EITHER && !(rt::rand() % 2))) {
                    TESObjectREFR* pOldSpeaker = pCurrentSpeaker;
                    pCurrentSpeaker = pCurrentTarget;
                    pCurrentTarget = pOldSpeaker;
                }
                bAllTried = true;
                for (uint32_t i = 0; i < static_cast<uint32_t>(iTotal) && bAllTried; ++i) {
                    if (!apDest[i]) {
                        bAllTried = false;
                    }
                }
                pValidDiagItem = pNextTopic->CreateDialogueItem(
                    pCurrentSpeaker, pCurrentTarget, pLastTopic, apConversationList, false
                );
            }
            else {
                bAllTried = true;
            }
        }
    }
    if (bIsConversationStats) {
        Interface::PrintLine("------NEW CONVERSATION CREATED---------------------");
        BSSimpleList<DialogueItem*>* pList_2 = apConversationList;
        int32_t iCount_2 = 1;
        while (pList_2 && pList_2->m_item) {
            DialogueItem* pItem = pList_2->m_item;
            pList_2 = pList_2->m_pkNext;
            if (pItem) {
                if (pItem->FirstResponse()) {
                    // Reads the text before it calls the speaker's GetFormEditorID.
                    TESObjectREFR* pSpeaker = pItem->pSpeaker;
                    const char* pText = pItem->GetCurrentResponse()->GetText()->String();
                    Interface::PrintLine("Item %d: %s says '%s'", iCount_2, pSpeaker->GetFormEditorID(), pText);
                }
                else {
                    TESObjectREFR* pSpeaker = pItem->pSpeaker;
                    Interface::PrintLine("Item %d: %s has no response!", iCount_2, pSpeaker->GetFormEditorID());
                }
            }
            ++iCount_2;
        }
    }
    return apConversationList->ItemsInList();
}

BSSimpleList<TESTopic*>* TESConversationData::GetLinkFromList() {
    return &m_listLinkFrom;
}

BSSimpleList<TESTopic*>* TESConversationData::GetLinkToList() {
    return &m_listLinkTo;
}

DIALOGUE_SPEAKER TESTopicInfo::GetNextSpeaker() {
    return static_cast<DIALOGUE_SPEAKER>((int8_t)m_Data.nextSpeaker);
}

void TESTopicInfo::SetOwnerQuest(TESQuest* apQuest) {
    pOwnerQuest = apQuest;
}

uint16_t TESTopicInfo::GetInfoIndex() {
    return iInfoIndex;
}

bool TESTopicInfo::IsRandom() {
    return (m_Data.flags & 2) != 0;
}

TESConversationData* TESTopicInfo::GetConversationData() {
    return m_pConversationData;
}

TopicInfoArray::TopicInfoArray() : NiTLargePrimitiveArray<TESTopicInfo*>(0, 1u) {}

TopicInfoArray::~TopicInfoArray() {}

void TopicInfoArray::InsertInfoAt(uint32_t aiIndex, TESTopicInfo* apInfo) {
    Compact();
    if (aiIndex <= m_uiSize) {
        uint32_t newsize = m_uiSize + 1;
        if (m_uiMaxSize < newsize) {
            SetSize(m_uiGrowBy + newsize);
        }
        std::memmove(&m_pBase[aiIndex + 1], &m_pBase[aiIndex], sizeof(TESTopicInfo*) * (m_uiSize - aiIndex));
        m_uiESize = ++m_uiSize;
        m_pBase[aiIndex] = apInfo;
    }
    else {
        SetAtGrow(aiIndex, apInfo);
    }
}

void TopicInfoArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}
