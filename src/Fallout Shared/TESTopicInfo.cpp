#include "Fallout Shared/TESTopicInfo.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ResponseListWrapper.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConversationData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESResponse.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TopicInfoArray.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "xfnv/Compiler.h"

extern TESTopicInfo* pCurrentResultScriptInfo;

// The info whose responses kResponseList holds, and the info whose result script is running.
static TESTopicInfo* pCurrentResponsesInfo;
TESTopicInfo* pCurrentResultScriptInfo;

// The two shared result scripts (begin and end) the last asked info's scripts are loaded into.
using ResultScriptArray = Script[TIRS_COUNT];
static ResultScriptArray pResultScriptA;

// GetResultScript's function-local static: the variable an SLSD chunk added last, which SCVR names.
static ScriptVariable* pNewVariable;

// The game settings this file defines, in their initialization order.
SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryEasy("iSpeechChallengeDifficultyVeryEasy", 0);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyEasy("iSpeechChallengeDifficultyEasy", 25);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyAverage("iSpeechChallengeDifficultyAverage", 50);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyHard("iSpeechChallengeDifficultyHard", 75);
SettingT<GameSettingCollection> iSpeechChallengeDifficultyVeryHard("iSpeechChallengeDifficultyVeryHard", 100);
ResponseListWrapper kResponseList;

TESTopicInfo::TESTopicInfo(DIALOGUE_TYPE aeType) {
    TESTopicInfo::InitializeData();
    m_Data.type = static_cast<uint8_t>(aeType);
    if (aeType == 1) {
        m_pConversationData = new TESConversationData();
        SetRandom(true);
    }
}

TESTopicInfo::~TESTopicInfo() {
    TESTopicInfo::ClearData();
}

// Stores the fields in this order; the two calls to the shared setter clear pPerkSkillStat.
void TESTopicInfo::InitializeData() {
    ourMemset(&m_Data, 0, sizeof(m_Data));
    iFileOffset = 0;
    bSaidOnce = false;
    iInfoIndex = 0xFFFF;
    SetFormType(INFO_ID);
    m_pConversationData = nullptr;
    pOwnerQuest = nullptr;
    SetPrompt("");
    SetSpeaker(nullptr);
    SetActorValue(nullptr);
    SetPerk(nullptr);
    SetDifficultyLevel(SCL_VERYEASY);
}

void TESTopicInfo::ClearData() {
    if (!GetTemporary()) {
        if (GetInfoIndex() != 0xFFFF) {
            TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(this);
            if (pOwnerTopic) {
                RemoveInfoFromTopic(pOwnerTopic);
            }
        }
        GetInfoConditions()->ClearAllConditionItems();
        ClearResponseList();
        ClearAddTopicList();
        // The result-script temporary: raw storage, so no C++ destructor runs on it.
        alignas(4) uint8_t aucScript[sizeof(Script)];
        Script* pScript = ::new (aucScript) Script;
        pResultScriptA[TIRS_BEGIN].CopyResultScript(pScript);
        pResultScriptA[TIRS_END].CopyResultScript(pScript);
        pScript->~Script();
    }
    pCurrentResponsesInfo = nullptr;
    pCurrentResultScriptInfo = nullptr;
    if (m_pConversationData) {
        delete m_pConversationData;
    }
}

// The DNAM value that GetChunkData fills starts at 0 for a zero-size chunk (malformed data). Form IDs read from the
// file are stored in the pointer fields and lists until InitItem resolves them.
bool TESTopicInfo::Load(TESFile* apFile) {
    uint8_t formid = apFile->GetTESForm();
    if (formid != 70) {
        return false;
    }
    if (!apFile->GetOptimizedFile()) {
        iFileOffset = apFile->GetOffset();
    }
    uint32_t iPrevInfoID = -1;
    uint32_t questID = 0;
    LoadForm(apFile);
    do {
        CHUNK_ID chunkid = apFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        if (chunkid > DNAM_ID) {
            if (chunkid > RNAM_ID) {
                if (chunkid == TCLT_ID) {
                    if (!GetConversationData()) {
                        LoadConversation(nullptr);
                    }
                    uint32_t topicID = 0;
                    apFile->GetChunkData<uint32_t>(topicID);
                    TESTopic* pLinkTo = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID));
                    GetConversationData()->m_listLinkTo.AddTail(pLinkTo);
                }
                else if (chunkid == TCFU_ID) {
                    if (!GetConversationData()) {
                        LoadConversation(nullptr);
                    }
                    uint32_t topicInfoID = 0;
                    apFile->GetChunkData<uint32_t>(topicInfoID);
                    TESTopicInfo* pFollowUp = reinterpret_cast<TESTopicInfo*>(static_cast<uintptr_t>(topicInfoID));
                    GetConversationData()->GetFollowUpInfoList()->AddTail(pFollowUp);
                }
            }
            else {
                switch (chunkid) {
                    case RNAM_ID: {
                        char szBuf[256];
                        apFile->GetChunkData(szBuf, 256u);
                        SetPrompt(szBuf);
                        break;
                    }
                    case KNAM_ID: {
                        uint32_t skillID = 0;
                        apFile->GetChunkData<uint32_t>(skillID);
                        pPerkSkillStat = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(skillID));
                        break;
                    }
                    case PNAM_ID: {
                        apFile->GetChunkData<uint32_t>(iPrevInfoID);
                        if (iPrevInfoID) {
                            TESForm::AddCompileIndex(iPrevInfoID, apFile);
                        }
                        break;
                    }
                }
            }
        }
        else if (chunkid == DNAM_ID) {
            uint32_t uiDifficulty = 0;
            apFile->GetChunkData<uint32_t>(uiDifficulty);
            if (uiDifficulty < static_cast<uint32_t>(SCL_COUNT)) {
                SetDifficultyLevel(static_cast<SPEECH_CHALLENGE_LEVEL>(uiDifficulty));
            }
            if (eDifficulty == SCL_NONE) {
                eDifficulty = SCL_VERYEASY;
            }
        }
        else if (chunkid > TCLF_ID) {
            if (chunkid == QSTI_ID) {
                apFile->GetChunkData<uint32_t>(questID);
                TESForm::AddCompileIndex(questID, apFile);
            }
            else if (chunkid == ANAM_ID) {
                uint32_t iid = 0;
                apFile->GetChunkData<uint32_t>(iid);
                pSpeaker = reinterpret_cast<TESActorBase*>(static_cast<uintptr_t>(iid));
            }
        }
        else {
            switch (chunkid) {
                case TCLF_ID: {
                    if (!GetConversationData()) {
                        LoadConversation(nullptr);
                    }
                    uint32_t topicID = 0;
                    apFile->GetChunkData<uint32_t>(topicID);
                    TESTopic* pLinkFrom = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID));
                    GetConversationData()->m_listLinkFrom.AddTail(pLinkFrom);
                    break;
                }
                case CIDA_ID: {
                    objConditions.Load(apFile);
                    break;
                }
                case DATA_ID: {
                    apFile->GetChunkData(&m_Data, sizeof(TOPIC_INFO_DATA));
                    if (m_Data.type >= 8) {
                        SetDelete(true);
                    }
                    if (m_Data.type == 1) {
                        LoadConversation(nullptr);
                    }
                    break;
                }
                case NAME_ID: {
                    uint32_t topicID = 0;
                    apFile->GetChunkData<uint32_t>(topicID);
                    TESTopic* pAddTopic = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(topicID));
                    m_listAddTopics.AddTail(pAddTopic);
                    break;
                }
            }
        }
    }
    while (apFile->NextChunk());
    if (m_Data.type == 3) {
        m_Data.type = 0;
    }
    if (!GetDelete() &&
        !TESTopic::pLastTopic
             ->InsertInfo(reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(questID)), this, iPrevInfoID)) {
        uint32_t uiQuestID = questID;
        int32_t iTopicID = TESTopic::pLastTopic->iFormID;
        const char* pcTopicName = TESTopic::pLastTopic->GetFormEditorID();
        Warning(
            "MASTERFILE: Unable to insert topic info (%08X) into topic '%s' (%08X), quest (%08X).",
            iFormID,
            pcTopicName,
            iTopicID,
            uiQuestID
        );
    }
    return true;
}

// Within a GRUP: a top group saves as any form; a topic children group saves before this info when its topic
// does; the other group types never.
bool TESTopicInfo::SavesBefore(FORM* apGroupFORM) {
    bool bretval = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        switch (apGroupFORM->iFormID) {
            case 0: bretval = TESForm::SavesBefore(apGroupFORM); break;
            case 7: {
                TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
                TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
                if (pTopic) {
                    bretval = SavesBefore(pTopic);
                }
                break;
            }
            default: break;
        }
    }
    return bretval;
}

bool TESTopicInfo::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    int32_t iFormType = apForm->GetFormType();
    if (iFormType == 69) {
        TESTopic* pothertopic = dynamic_cast<TESTopic*>(apForm);
        if (pothertopic) {
            TESTopic* pourtopic = TESTopic::GetOwnerTopic(this);
            if (pourtopic && pourtopic != pothertopic) {
                return pourtopic->SavesBefore(pothertopic);
            }
        }
    }
    else if (iFormType == 70) {
        TESTopicInfo* pinfo = dynamic_cast<TESTopicInfo*>(apForm);
        if (pinfo) {
            TESTopic* pourtopic = TESTopic::GetOwnerTopic(this);
            TESTopic* pothertopic = TESTopic::GetOwnerTopic(pinfo);
            if (pothertopic && pourtopic && pothertopic != pourtopic) {
                return pourtopic->SavesBefore(pothertopic);
            }
        }
    }
    else {
        return TESForm::SavesBefore(apForm);
    }
    return bretval;
}

// The same for the new save system, reading the change flags from the load buffer.
void TESTopicInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_TOPIC_SAIDONCE)) {
        bSaidOnce = true;
    }
}

// Loads the form's changes from an old-format save; a changed said-once flag means the info was said.
void TESTopicInfo::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    if (aiFlags & CHANGE_TOPIC_SAIDONCE) {
        bSaidOnce = true;
    }
}

// Reverts the form; an info whose said-once flag had changed is unsaid again.
void TESTopicInfo::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_TOPIC_SAIDONCE)) {
        bSaidOnce = false;
    }
}

void TESTopicInfo::Revert(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_TOPIC_SAIDONCE) != 0) {
        bSaidOnce = false;
    }
}

// Resolves the info's form IDs once: the add-topics list (dropping, with a warning, the IDs that are not topics), the
// speaker, the conditions, the conversation data, the shared response list and the perk or actor value. The
// reinterpret_casts move form IDs out of pointer slots, which hold untyped IDs until here.
void TESTopicInfo::InitItem() {
    if (GetInitialized()) {
        return;
    }
    TESFile* pFile = GetFile(-1);
    BSSimpleList<TESTopic*>* pPrev = nullptr;
    BSSimpleList<TESTopic*>* pNode = GetAddTopicList();
    while (pNode && pNode->GetItem()) {
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNode->GetItem()));
        TESForm::AddCompileIndex(uiID, pFile);
        TESTopic* pTopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(uiID));
        if (pTopic) {
            pNode->SetItem(pTopic);
            pPrev = pNode;
            pNode = pNode->GetNext();
        }
        else {
            if (pNode->GetNext() || !pPrev) {
                pNode->RemoveHead();
            }
            else {
                TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
                pPrev->Remove(pRemoveID);
                pNode = pPrev->GetNext();
            }
            TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(this);
            TESQuest* pOwnerQuest = nullptr;
            if (pOwnerTopic) {
                pOwnerQuest = pOwnerTopic->GetOwnerQuest(this);
            }
            char acInfoText[0x104];
            alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
            BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
            GetInfoDisplayText(*pString, true);
            if (pOwnerTopic && pOwnerQuest) {
                uint32_t uiTopicID = pOwnerTopic->GetFormID();
                const char* pTopicName = pOwnerTopic->GetFormEditorID();
                uint32_t uiQuestID = pOwnerQuest->GetFormID();
                const char* pQuestName = pOwnerQuest->GetEditorName();
                const char* pText = pString->String();
                std::sprintf(
                    acInfoText,
                    "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                    GetFormID(),
                    pText,
                    pQuestName,
                    uiQuestID,
                    pTopicName,
                    uiTopicID
                );
            }
            else {
                const char* pText = pString->String();
                std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", GetFormID(), pText);
            }
            Warning("MASTERFILE: Unable to find topic (%08X) for %s", uiID, acInfoText);
            pString->~BSStringT<char>();
        }
    }

    // pSpeaker holds the speaker's form ID until here.
    uint32_t uiSpeakerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetSpeaker()));
    if (uiSpeakerID) {
        TESForm::AddCompileIndex(uiSpeakerID, GetFile(-1));
        SetSpeaker(dynamic_cast<TESActorBase*>(TESForm::GetFormByNumericID(uiSpeakerID)));
    }
    if (GetInfoConditions()) {
        GetInfoConditions()->InitItem(this);
    }
    if (GetConversationData()) {
        GetConversationData()->InitItem(this);
    }
    for (TESResponse* pResponse = kResponseList.QResponseList(); pResponse; pResponse = pResponse->QNext()) {
        pResponse->InitItem(this);
    }
    if (pPerkSkillStat) {
        uint32_t uiStatID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPerkSkillStat));
        TESForm::AddCompileIndex(uiStatID, GetFile(-1));
        TESForm* pStat = TESForm::GetFormByNumericID(uiStatID);
        TESForm* pPerk = dynamic_cast<BGSPerk*>(pStat);
        if (pPerk) {
            pPerkSkillStat = pPerk;
        }
        else {
            pPerkSkillStat = dynamic_cast<ActorValueInfo*>(pStat);
        }
    }
    SetInitialized(true);
}

void TESTopicInfo::GetFormDetailedString(BSStringT<char>& aString) {
    char cQuestText[264];
    char cTopicText[264];
    char cResponseText[268];
    cTopicText[0] = 0;
    TESTopic* ptopic = TESTopic::GetOwnerTopic(this);
    if (ptopic) {
        int32_t iTopicID = ptopic->iFormID;
        std::sprintf(cTopicText, ", Topic '%s' (%08X)", ptopic->GetFormEditorID(), iTopicID);
    }
    cQuestText[0] = 0;
    if (ptopic && GetInitialized()) {
        TESQuest* pquest = ptopic->GetOwnerQuest(this);
        if (pquest) {
            int32_t iQuestID = pquest->iFormID;
            std::sprintf(cQuestText, "Quest '%s' (%08X)", pquest->GetEditorName(), iQuestID);
        }
    }

    cResponseText[0] = 0;
    TESResponse* presponse = GetResponseList(nullptr)->pHead;
    if (presponse) {
        std::sprintf(cResponseText, ", Text: \"%s\"", presponse->GetResponseText());
    }
    int32_t iInfoID = iFormID;
    aString.SPrintF(
        "%s Form '%s' (%08X): %s%s%s",
        GetFormTypeString(),
        TESForm::GetFormEditorID(),
        iInfoID,
        cQuestText,
        cTopicText,
        cResponseText
    );
}

// Altering a loaded, non-temporary info also marks its topic altered before the form itself.
void TESTopicInfo::SetAltered(bool abVal) {
    if (!GetTemporary() && abVal && GetInitialized()) {
        TESTopic* pTopic = TESTopic::GetOwnerTopic(this);
        if (pTopic) {
            pTopic->SetAltered(true);
        }
    }
    TESForm::SetAltered(abVal);
}

void TESTopicInfo::Copy(TESForm* apCopy) {
    TESTopicInfo* pInfoToCopy = dynamic_cast<TESTopicInfo*>(apCopy);
    if (pInfoToCopy) {
        CopyAllComponents(apCopy);
        m_Data = *pInfoToCopy->GetData();
        ClearAddTopicList();
        BSSimpleList<TESTopic*>* pTopicList = &pInfoToCopy->m_listAddTopics;
        while (pTopicList && pTopicList->m_item) {
            TESTopic* pTopic = pTopicList->m_item;
            pTopicList = pTopicList->m_pkNext;
            m_listAddTopics.AddTail(pTopic);
        }
        ClearResponseList();
        for (TESResponse* presponse = pInfoToCopy->GetResponseList(nullptr)->pHead; presponse;
             presponse = presponse->pNext) {
            TESResponse* pNewResponse = new TESResponse;
            pNewResponse->Copy(presponse);
            GetResponseList(nullptr)->AddTail(pNewResponse);
        }
        SetPrompt(pInfoToCopy->cPrompt.pString);
        if (pInfoToCopy->pPerkSkillStat) {
            int32_t iFormType = pInfoToCopy->pPerkSkillStat->GetFormType();
            if (iFormType == 86 || iFormType == 89) {
                pPerkSkillStat = pInfoToCopy->pPerkSkillStat;
            }
            else {
                pPerkSkillStat = pInfoToCopy->pPerkSkillStat;
            }
        }
        else {
            pPerkSkillStat = nullptr;
        }
        eDifficulty = pInfoToCopy->eDifficulty;
        pSpeaker = pInfoToCopy->GetSpeaker();
        Script* pBeginScript = pInfoToCopy->GetResultScript(TIRS_BEGIN);
        GetResultScript(TIRS_BEGIN)->CopyResultScript(pBeginScript);
        Script* pEndScript = pInfoToCopy->GetResultScript(TIRS_END);
        GetResultScript(TIRS_END)->CopyResultScript(pEndScript);
        SetInfoIndex(pInfoToCopy->GetInfoIndex());
        SetConditions(&pInfoToCopy->objConditions);
        if (pInfoToCopy->GetConversationData()) {
            LoadConversation(pInfoToCopy->GetConversationData());
        }
        pOwnerQuest = pInfoToCopy->pOwnerQuest;
        SetInitialized(pInfoToCopy->GetInitialized());
    }
}

bool TESTopicInfo::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    bool bBelongs = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(this);
        if (pOwnerTopic) {
            if (abAllowParentGroups) {
                bBelongs = pOwnerTopic->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
            }
            if (!bBelongs && apGroupFORM->iFormID == 7) {
                return pOwnerTopic->FormIDMatchesWithoutIndex(apGroupFORM->flags);
            }
        }
    }

    return bBelongs;
}

void TESTopicInfo::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if (apOutGroupFORM) {
        apOutGroupFORM->form = 0;
        if (apParentGroup &&
            !apParentGroup->GroupData.iFormID &&
            apParentGroup->GroupData.flags == formEnumString[69].uiFormString) {
            TESTopic* pparenttopic = TESTopic::GetOwnerTopic(this);
            if (pparenttopic) {
                apOutGroupFORM->form = formEnumString[2].uiFormString;
                apOutGroupFORM->iFormID = 7;
                apOutGroupFORM->flags = pparenttopic->iFormID;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
        }
    }
}

void TESTopicInfo::ResetSaidOnceFlags() {
    bSaidOnce = false;
    RemoveChange(CHANGE_TOPIC_SAIDONCE);
}

void TESTopicInfo::SetSaidOnce() {
    if (!IsInfoRefusal()) {
        bSaidOnce = true;
        AddChange(CHANGE_TOPIC_SAIDONCE);
    }
}

void TESTopicInfo::SetTempDataOffset(uint32_t aiOffset) {
    iFileOffset = aiOffset;
}

TOPIC_INFO_DATA* TESTopicInfo::GetData() {
    return &m_Data;
}

// The result scripts are read on demand from the info's file at iFileOffset; NEXT moves from the begin script to the
// end script. Both shared scripts are first reset from an empty stack script. An empty SCRO/SCRV chunk stores 0 into
// the new reference.
Script* TESTopicInfo::GetResultScript(TOPIC_INFO_RESULT_SCRIPT aeScript) {
    int32_t iScript = aeScript;
    if (this == pCurrentResultScriptInfo) {
        return &pResultScriptA[iScript];
    }
    pCurrentResultScriptInfo = this;
    alignas(Script) uint8_t aucTempScript[sizeof(Script)];
    Script* pTempScript = static_cast<Script*>(static_cast<void*>(aucTempScript));
    ::new (pTempScript) Script;
    pResultScriptA[TIRS_BEGIN].CopyResultScript(pTempScript);
    pResultScriptA[TIRS_BEGIN].SetTemporary();
    pResultScriptA[TIRS_BEGIN].GetHeader()->bIsQuestScript = false;
    pResultScriptA[TIRS_END].CopyResultScript(pTempScript);
    pResultScriptA[TIRS_END].SetTemporary();
    pResultScriptA[TIRS_END].GetHeader()->bIsQuestScript = false;
    if (!GetEmpty()) {
        Script* pResult = &pResultScriptA[iScript];
        pTempScript->~Script();
        return pResult;
    }
    pResultScriptA[TIRS_BEGIN].SetInitialized(false);
    pResultScriptA[TIRS_END].SetInitialized(false);
    TESFile* pFile = GetFile(-1);
    if (!pFile || !iFileOffset) {
        Script* pResult = &pResultScriptA[iScript];
        pTempScript->~Script();
        return pResult;
    }
    pFile = pFile->GetThreadSafeFile();
    if (!pFile->SetOffset(iFileOffset)) {
        Script* pResult = &pResultScriptA[iScript];
        pTempScript->~Script();
        return pResult;
    }
    uint8_t formid = static_cast<uint8_t>(pFile->GetTESForm());
    if (formid != INFO_ID) {
        Script* pResult = &pResultScriptA[iScript];
        pTempScript->~Script();
        return pResult;
    }
    int32_t iloadscript = 0;
    do {
        uint32_t chunkid = pFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        switch (chunkid) {
            case NEXT_ID: ++iloadscript; break;
            case SLSD_ID:
            case SCVR_ID:
                if (pFile->GetTESChunk() == SLSD_ID) {
                    pNewVariable = new ScriptVariable;
                    pNewVariable->Load(pFile);
                    pResultScriptA[iloadscript].GetVariableList()->AddTail(pNewVariable);
                }
                else if (pNewVariable) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(pFile->GetChunkSize()));
                    pFile->GetChunkData(pBuffer, 0x200);
                    pNewVariable->cName.Set(pBuffer, 0);
                }
                break;
            case SCHR_ID:
                pFile->GetChunkData(pResultScriptA[iloadscript].GetHeader(), 0);
                if (pFile->QEndian()) {
                    pResultScriptA[iloadscript].GetHeader()->Endian();
                }
                break;
            case SCDA_ID: {
                uint32_t n = pFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* tmpdata = static_cast<char*>(XFNV_ALLOCA16(n));
                ourMemset(tmpdata, 0, n);
                pFile->GetChunkData(tmpdata, 0);
                pResultScriptA[iloadscript].SetCompileData(n, tmpdata);
                break;
            }
            case SCRO_ID: {
                Script::RefVariable* pNewItem = new Script::RefVariable;
                uint32_t tempID = 0;
                pFile->GetChunkData(tempID);
                // The form ID stands in for the form until InitItem resolves it.
                pNewItem->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(tempID));
                pResultScriptA[iloadscript].GetReferencedObjectList()->AddTail(pNewItem);
                break;
            }
            case SCRV_ID: {
                Script::RefVariable* pNewItem = new Script::RefVariable;
                uint32_t tempID = 0;
                pFile->GetChunkData(tempID);
                pNewItem->varIdx = tempID;
                pResultScriptA[iloadscript].GetReferencedObjectList()->AddTail(pNewItem);
                break;
            }
            default: break;
        }
    }
    while (pFile->NextChunk());
    pResultScriptA[TIRS_BEGIN].InitItem(this);
    pResultScriptA[TIRS_END].InitItem(this);
    Script* pResult = &pResultScriptA[iScript];
    pTempScript->~Script();
    return pResult;
}

void TESTopicInfo::SetInfoIndex(uint16_t iIdx) {
    iInfoIndex = iIdx;
}

void TESTopicInfo::SetRandom(bool abVal) {
    if (abVal) {
        m_Data.flags |= TOPIC_INFO_DATA::FLAG_RANDOM;
    }
    else {
        m_Data.flags &= ~TOPIC_INFO_DATA::FLAG_RANDOM;
    }
}

void TESTopicInfo::ClearResponseList() {
    kResponseList.ClearResponseList();
}

// Removes the info from apTopic's info list.
void TESTopicInfo::RemoveInfoFromTopic(TESTopic* apTopic) {
    if (apTopic && GetInfoIndex() != 0xFFFF) {
        QUEST_INFO* pQuestInfo = apTopic->GetQuestInfoElement(this);
        if (pQuestInfo) {
            TopicInfoArray* pInfoArray = &pQuestInfo->infoArray;
            if (pInfoArray->GetAt(GetInfoIndex()) == this) {
                pInfoArray->RemoveAt(GetInfoIndex());
            }
            else {
                int32_t isize = pInfoArray->m_uiSize;
                for (int32_t i = 0; i < isize; ++i) {
                    if (pInfoArray->GetAt(i) == this) {
                        pInfoArray->RemoveAt(i);
                        break;
                    }
                }
            }
            pInfoArray->Compact();
            TESTopic::SetTopicIndices(pInfoArray);
        }
    }
}

void TESTopicInfo::SetSpeaker(TESActorBase* apActor) {
    pSpeaker = apActor;
}

void TESTopicInfo::SetPrompt(const char* apPrompt) {
    cPrompt.Set(apPrompt, 0);
}

void TESTopicInfo::SetDifficultyLevel(SPEECH_CHALLENGE_LEVEL aeDifficulty) {
    eDifficulty = aeDifficulty;
}

void TESTopicInfo::SetActorValue(ActorValueInfo* apAVI) {
    pPerkSkillStat = apAVI;
}

void TESTopicInfo::SetPerk(BGSPerk* apPerk) {
    pPerkSkillStat = apPerk;
}

ResponseListWrapper* TESTopicInfo::CloneResponseList(ResponseListWrapper* apList) {
    return GetResponseList(apList);
}

bool TESTopicInfo::IsRunImmediately() {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_RUN_IMMEDIATELY) != 0;
}

bool TESTopicInfo::IsSayOnce() const {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_SAY_ONCE) != 0;
}

bool TESTopicInfo::IsGoodbye() const {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_GOODBYE) != 0;
}

bool TESTopicInfo::IsRandomEnd() const {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_RANDOM_END) != 0;
}

bool TESTopicInfo::IsSayOnceADay() {
    return (m_Data.flagsExt & TOPIC_INFO_DATA::EXT_FLAG_SAY_ONCE_A_DAY) != 0;
}

bool TESTopicInfo::IsInfoRefusal() const {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_INFO_REFUSAL) != 0;
}

void TESTopicInfo::RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript, TESObjectREFR* apRef) {
    if (apRef) {
        Script* pScript = GetResultScript(aeScript);
        if (Script::GetProcessScripts() && pScript && pScript->GetHeader()->dataSize) {
            ScriptLocals* pLocals = apRef->TESObjectREFR::GetExtra()->GetScriptLocals();
            pScript->Run(apRef, pLocals, nullptr, true);
        }

        SetSaidOnce();
        if (IsSayOnceADay() && apRef->IsMobileObject()) {
            static_cast<MobileObject*>(apRef)->AddSayOnceTopicInfo(this);
        }
    }
}

void TESTopicInfo::AddTopicList() {
    BSSimpleList<TESTopic*>* pTopicList = GetAddTopicList();
    pPlayer->AddTopics(pTopicList);
}

bool TESTopicInfo::IsSpeechChallenge() const {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_SPEECH_CHALLENGE) != 0;
}

bool TESTopicInfo::IsAlwaysDarkened() {
    return (m_Data.flagsExt & TOPIC_INFO_DATA::EXT_FLAG_ALWAYS_DARKENED) != 0;
}

bool TESTopicInfo::IsRunForRumors() {
    return (m_Data.flags & TOPIC_INFO_DATA::FLAG_RUN_FOR_RUMORS) != 0;
}

DUMMY_NODE_STATE TESTopicInfo::GetDummyNodeState() {
    if ((m_Data.flagsExt & TOPIC_INFO_DATA::EXT_FLAG_DUMMY_ONLY) != 0) {
        return DUM_DUMMY_ONLY;
    }

    return static_cast<DUMMY_NODE_STATE>(((m_Data.flagsExt & TOPIC_INFO_DATA::EXT_FLAG_NON_DUMMY_ONLY) != 0) + 2);
}

uint32_t TESTopicInfo::GetSpeechChallengeDifficultyMult() {
    uint32_t uiMult;
    switch (eDifficulty) {
        case SCL_VERYEASY: {
            uiMult = iSpeechChallengeDifficultyVeryEasy.UInt();
            break;
        }
        case SCL_EASY: {
            uiMult = iSpeechChallengeDifficultyEasy.UInt();
            break;
        }
        case SCL_AVERAGE: {
            uiMult = iSpeechChallengeDifficultyAverage.UInt();
            break;
        }
        case SCL_HARD: {
            uiMult = iSpeechChallengeDifficultyHard.UInt();
            break;
        }
        case SCL_VERYHARD: {
            uiMult = iSpeechChallengeDifficultyVeryHard.UInt();
            break;
        }
        default: {
            uiMult = static_cast<uint32_t>(-1);
            break;
        }
    }
    return uiMult;
}

void TESTopicInfo::SetConditions(TESCondition* apCondition) {
    if (apCondition) {
        objConditions.Copy(apCondition, this);
    }
    else {
        TESCondition kTemp;
        objConditions.Copy(&kTemp, this);
    }
}

void TESTopicInfo::GetInfoDisplayText(BSStringT<char>& aString, bool abFirstOnly) {
    aString.Set(static_cast<const char*>(nullptr), 0u);
    for (TESResponse* plist = GetResponseList(nullptr)->pHead; plist; plist = plist->pNext) {
        aString += plist->GetResponseText();
        if (plist->pNext) {
            aString += " | ";
        }
        if (abFirstOnly) {
            break;
        }
    }
}

BSSimpleList<TESTopic*>* TESTopicInfo::GetAddTopicList() {
    return &m_listAddTopics;
}

bool TESTopicInfo::HasPrompt() const {
    return cPrompt.GetLength() != 0;
}

void TESTopicInfo::ClearAddTopicList() {
    for (BSSimpleList<TESTopic*>* pTopicList = &m_listAddTopics; pTopicList && !pTopicList->IsEmpty();
         pTopicList->RemoveHead()) {
        ;
    }
}

// The responses are read on demand from the info's file at iFileOffset. Without apList they go to the shared
// kResponseList, which is kept while the same info asks again; with apList the shared list is only copied.
ResponseListWrapper* TESTopicInfo::GetResponseList(ResponseListWrapper* apList) {
    if (this == pCurrentResponsesInfo) {
        if (apList) {
            apList->CopyResponseList(kResponseList.QResponseList());
            return apList;
        }
        return &kResponseList;
    }
    if (!apList) {
        pCurrentResponsesInfo = this;
        ClearResponseList();
    }
    TESFile* pFile = GetFile(-1);
    if (!pFile || !iFileOffset) {
        return apList ? apList : &kResponseList;
    }
    pFile = pFile->GetThreadSafeFile();
    if (!pFile->SetOffset(iFileOffset)) {
        return apList ? apList : &kResponseList;
    }
    uint8_t cFormType = static_cast<uint8_t>(pFile->GetTESForm());
    if (cFormType != INFO_ID) {
        return apList ? apList : &kResponseList;
    }
    TESResponse* pResponse = nullptr;
    do {
        uint32_t uiChunk = pFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case TRDT_ID: {
                pResponse = new TESResponse;
                pResponse->Load(pFile);
                pResponse->InitItem(this);
                if (apList) {
                    apList->AddTail(pResponse);
                }
                else {
                    kResponseList.AddTail(pResponse);
                }
                break;
            }
            case NAM1_ID:
                if (pResponse) {
                    uint32_t uiSize = pFile->GetChunkSize();
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                    pFile->GetChunkData(pBuffer, uiSize);
                    pResponse->SetResponseText(pBuffer);
                }
                break;
            case SNAM_ID:
                if (pResponse) {
                    uint32_t uiIdleID = 0;
                    pFile->GetChunkData(uiIdleID);
                    TESForm::AddCompileIndex(uiIdleID, pFile);
                    TESIdleForm* pIdle = nullptr;
                    TESForm* pForm = TESForm::GetFormByNumericID(uiIdleID);
                    if (pForm && pForm->GetFormType() == IDLE_ID) {
                        pIdle = static_cast<TESIdleForm*>(pForm);
                    }
                    if (uiIdleID && !pIdle) {
                        Warning("DEFAULT: Unable to find idle (%08X) for info (%08X).", uiIdleID, GetFormID());
                    }
                    pResponse->SetSpeakerIdle(pIdle);
                }
                break;
            case LNAM_ID:
                if (pResponse) {
                    uint32_t uiIdleID = 0;
                    pFile->GetChunkData(uiIdleID);
                    TESForm::AddCompileIndex(uiIdleID, pFile);
                    TESIdleForm* pIdle = nullptr;
                    TESForm* pForm = TESForm::GetFormByNumericID(uiIdleID);
                    if (pForm && pForm->GetFormType() == IDLE_ID) {
                        pIdle = static_cast<TESIdleForm*>(pForm);
                    }
                    if (uiIdleID && !pIdle) {
                        Warning("DEFAULT: Unable to find idle (%08X) for info (%08X).", uiIdleID, GetFormID());
                    }
                    pResponse->SetListenerIdle(pIdle);
                }
                break;
            default: break;
        }
    }
    while (pFile->NextChunk());
    return apList ? apList : &kResponseList;
}

void TESTopicInfo::LoadConversation(TESConversationData* apSource) {
    if (apSource) {
        if (!m_pConversationData) {
            m_pConversationData = new TESConversationData;
        }
        m_pConversationData->Copy(this, apSource);
    }
    else {
        if (m_pConversationData) {
            delete m_pConversationData;
            m_pConversationData = nullptr;
        }
        m_pConversationData = new TESConversationData;
    }
}

bool TESTopicInfo::CheckCondition(
    bool& abDispositionFailure,
    TESQuest* apQuest,
    TESObjectREFR* apActionRef,
    TESObjectREFR* apTargetRef
) {
    int32_t iIntelligence;
    if (GetDelete()) {
        return false;
    }
    if (IsSayOnce() && bSaidOnce) {
        return false;
    }
    DUMMY_NODE_STATE eDummyFlag = GetDummyNodeState();
    Actor* pTargetActor = nullptr;
    if (apTargetRef && apTargetRef->IsActor()) {
        pTargetActor = static_cast<Actor*>(apTargetRef);
    }

    if (pTargetActor &&
        (iIntelligence = pTargetActor->GetActorValue(ActorValue::INTELLIGENCE),
         iIntelligence <= iDialogueDummySpeakThisIntOrBelow.Int())) {
        if (eDummyFlag == DUM_NON_DUMMY_ONLY) {
            return false;
        }
    }
    else if (eDummyFlag == DUM_DUMMY_ONLY) {
        return false;
    }
    bool bretval = false;
    bool bquestconditions = true;
    if (apQuest) {
        bquestconditions = apQuest->objConditions.IsTrueFull(apActionRef, apTargetRef, &abDispositionFailure, false);
    }
    if (bquestconditions) {
        return objConditions.IsTrueFull(apActionRef, apTargetRef, &abDispositionFailure, false);
    }
    return bretval;
}

void TESTopicInfo::AddTopicInfoToSortedList(TESTopicInfo* apTopicInfo, BSSimpleList<TESTopicInfo*>* apTopicInfoList) {
    if (apTopicInfo && apTopicInfoList) {
        if (apTopicInfoList->IsEmpty()) {
            apTopicInfoList->AddHead(apTopicInfo);
        }
        else {
            for (BSSimpleList<TESTopicInfo*>* plist = apTopicInfoList; plist; plist = plist->m_pkNext) {
                if (plist->IsEmpty()) {
                    break;
                }
                TESTopicInfo* pInfo = plist->m_item;
                if (pInfo == apTopicInfo) {
                    break;
                }
                if (CompareTopicInfos(pInfo, apTopicInfo)) {
                    plist->AddHead(apTopicInfo);
                    return;
                }
                if (!plist->m_pkNext) {
                    plist->AddTail(apTopicInfo);
                    return;
                }
            }
        }
    }
}

bool TESTopicInfo::CompareTopicInfos(TESTopicInfo* apTopicInfo1, TESTopicInfo* apTopicInfo2) {
    return apTopicInfo1 < apTopicInfo2;
}

ResponseListWrapper::ResponseListWrapper() : pHead(nullptr) {}

ResponseListWrapper::~ResponseListWrapper() {
    ClearResponseList();
}

void ResponseListWrapper::AddTail(TESResponse* apRes) {
    if (!pHead) {
        pHead = apRes;
    }
    else {
        pHead->AddTail(apRes);
    }
}

int32_t ResponseListWrapper::GetResponseID() {
    if (pHead) {
        return pHead->GetListCount();
    }

    return 0;
}

void ResponseListWrapper::ClearResponseList() {
    TESResponse* pNext;
    for (TESResponse* pResponse = pHead; pResponse; pResponse = pNext) {
        pNext = pResponse->pNext;
        if (pResponse) {
            delete pResponse;
        }
    }
    pHead = nullptr;
}

void ResponseListWrapper::CopyResponseList(TESResponse* apHead) {
    ClearResponseList();
    TESResponse* pprev = nullptr;
    while (apHead) {
        TESResponse* pnewResponse = new TESResponse;
        pnewResponse->Copy(apHead);
        if (pprev) {
            pprev->pNext = pnewResponse;
        }
        else {
            AddTail(pnewResponse);
        }
        pprev = pnewResponse;
        apHead = apHead->pNext;
    }
}

TESResponse* ResponseListWrapper::QResponseList() {
    return pHead;
}

TESConversationData::TESConversationData() {}

TESConversationData::~TESConversationData() {
    ClearLists();
}

// The three lists hold form IDs until here; each is resolved against apOwner's file, and an ID that is not a form of
// the list's type is dropped with a warning naming the owning info. The reinterpret_casts move a form ID in and out of
// an item slot, which holds an untyped value until here.
void TESConversationData::InitItem(TESForm* apOwner) {
    BSSimpleList<TESTopic*>* pPrevTopic = nullptr;
    BSSimpleList<TESTopic*>* pTopicNode = GetLinkToList();
    TESFile* pFile = apOwner ? apOwner->GetFile(-1) : nullptr;
    while (pTopicNode && pTopicNode->GetItem()) {
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTopicNode->GetItem()));
        TESForm::AddCompileIndex(uiID, pFile);
        TESTopic* pTopic = nullptr;
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        if (pForm && pForm->GetFormType() == DIAL_ID) {
            pTopic = static_cast<TESTopic*>(pForm);
        }
        if (pTopic) {
            pTopicNode->SetItem(pTopic);
            pPrevTopic = pTopicNode;
            pTopicNode = pTopicNode->GetNext();
        }
        else {
            if (pTopicNode->GetNext() || !pPrevTopic) {
                pTopicNode->RemoveHead();
            }
            else {
                TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
                pPrevTopic->Remove(pRemoveID);
                pTopicNode = pPrevTopic->GetNext();
            }
            TESTopicInfo* pInfo = nullptr;
            if (apOwner && apOwner->GetFormType() == INFO_ID) {
                pInfo = static_cast<TESTopicInfo*>(apOwner);
            }
            TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
            TESQuest* pOwnerQuest = nullptr;
            if (pOwnerTopic) {
                pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
            }
            char acInfoText[0x400];
            alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
            BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
            pInfo->GetInfoDisplayText(*pString, true);
            if (pOwnerTopic && pOwnerQuest) {
                uint32_t uiTopicID = pOwnerTopic->GetFormID();
                const char* pTopicName = pOwnerTopic->GetFormEditorID();
                uint32_t uiQuestID = pOwnerQuest->GetFormID();
                const char* pQuestName = pOwnerQuest->GetEditorName();
                const char* pText = pString->String();
                std::sprintf(
                    acInfoText,
                    "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                    pInfo->GetFormID(),
                    pText,
                    pQuestName,
                    uiQuestID,
                    pTopicName,
                    uiTopicID
                );
            }
            else {
                const char* pText = pString->String();
                std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
            }
            Warning("MASTERFILE: Unable to find topic (%08X) for conversation data 'Link To' on %s", uiID, acInfoText);
            pString->~BSStringT<char>();
        }
    }

    pPrevTopic = nullptr;
    pTopicNode = GetLinkFromList();
    while (pTopicNode && pTopicNode->GetItem()) {
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTopicNode->GetItem()));
        TESForm::AddCompileIndex(uiID, pFile);
        TESTopic* pTopic = nullptr;
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        if (pForm && pForm->GetFormType() == DIAL_ID) {
            pTopic = static_cast<TESTopic*>(pForm);
        }
        if (pTopic) {
            pTopicNode->SetItem(pTopic);
            pPrevTopic = pTopicNode;
            pTopicNode = pTopicNode->GetNext();
        }
        else {
            if (pTopicNode->GetNext() || !pPrevTopic) {
                pTopicNode->RemoveHead();
            }
            else {
                TESTopic* pRemoveID = reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiID));
                pPrevTopic->Remove(pRemoveID);
                pTopicNode = pPrevTopic->GetNext();
            }
            TESTopicInfo* pInfo = nullptr;
            if (apOwner && apOwner->GetFormType() == INFO_ID) {
                pInfo = static_cast<TESTopicInfo*>(apOwner);
            }
            TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
            TESQuest* pOwnerQuest = nullptr;
            if (pOwnerTopic) {
                pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
            }
            char acInfoText[0x400];
            alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
            BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
            pInfo->GetInfoDisplayText(*pString, true);
            if (pOwnerTopic && pOwnerQuest) {
                uint32_t uiTopicID = pOwnerTopic->GetFormID();
                const char* pTopicName = pOwnerTopic->GetFormEditorID();
                uint32_t uiQuestID = pOwnerQuest->GetFormID();
                const char* pQuestName = pOwnerQuest->GetEditorName();
                const char* pText = pString->String();
                std::sprintf(
                    acInfoText,
                    "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                    pInfo->GetFormID(),
                    pText,
                    pQuestName,
                    uiQuestID,
                    pTopicName,
                    uiTopicID
                );
            }
            else {
                const char* pText = pString->String();
                std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
            }
            Warning(
                "MASTERFILE: Unable to find topic (%08X) for conversation data 'Link From' on %s",
                uiID,
                acInfoText
            );
            pString->~BSStringT<char>();
        }
    }

    BSSimpleList<TESTopicInfo*>* pInfoNode = GetFollowUpInfoList();
    BSSimpleList<TESTopicInfo*>* pPrevInfo = nullptr;
    while (pInfoNode && pInfoNode->GetItem()) {
        uint32_t uiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pInfoNode->GetItem()));
        TESForm::AddCompileIndex(uiID, pFile);
        TESTopicInfo* pFollowUp = nullptr;
        TESForm* pForm = TESForm::GetFormByNumericID(uiID);
        if (pForm && pForm->GetFormType() == INFO_ID) {
            pFollowUp = static_cast<TESTopicInfo*>(pForm);
        }
        if (pFollowUp) {
            pInfoNode->SetItem(pFollowUp);
            pPrevInfo = pInfoNode;
            pInfoNode = pInfoNode->GetNext();
        }
        else {
            if (pInfoNode->GetNext() || !pPrevInfo) {
                pInfoNode->RemoveHead();
            }
            else {
                TESTopicInfo* pRemoveID = reinterpret_cast<TESTopicInfo*>(static_cast<uintptr_t>(uiID));
                pPrevInfo->Remove(pRemoveID);
                pInfoNode = pPrevInfo->GetNext();
            }
            TESTopicInfo* pInfo = nullptr;
            if (apOwner && apOwner->GetFormType() == INFO_ID) {
                pInfo = static_cast<TESTopicInfo*>(apOwner);
            }
            TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
            TESQuest* pOwnerQuest = nullptr;
            if (pOwnerTopic) {
                pOwnerQuest = pOwnerTopic->GetOwnerQuest(pInfo);
            }
            char acInfoText[0x400];
            alignas(4) uint8_t aucString[sizeof(BSStringT<char>)];
            BSStringT<char>* pString = ::new (aucString) BSStringT<char>;
            pInfo->GetInfoDisplayText(*pString, true);
            if (pOwnerTopic && pOwnerQuest) {
                uint32_t uiTopicID = pOwnerTopic->GetFormID();
                const char* pTopicName = pOwnerTopic->GetFormEditorID();
                uint32_t uiQuestID = pOwnerQuest->GetFormID();
                const char* pQuestName = pOwnerQuest->GetEditorName();
                const char* pText = pString->String();
                std::sprintf(
                    acInfoText,
                    "TopicInfo %08X \"%s\" in Quest %s (%08x) Topic %s (%08x)",
                    pInfo->GetFormID(),
                    pText,
                    pQuestName,
                    uiQuestID,
                    pTopicName,
                    uiTopicID
                );
            }
            else {
                const char* pText = pString->String();
                std::sprintf(acInfoText, "TopicInfo %08X \"%s\"", pInfo->GetFormID(), pText);
            }
            Warning(
                "MASTERFILE: Unable to find topic (%08X) for conversation data 'Follow Up' on %s",
                uiID,
                acInfoText
            );
            pString->~BSStringT<char>();
        }
    }
}

BSSimpleList<TESTopicInfo*>* TESConversationData::GetFollowUpInfoList() {
    return &m_listFollowUpInfos;
}

void TESConversationData::ClearLists() {
    m_listLinkTo.RemoveAll();
    m_listLinkFrom.RemoveAll();
    m_listFollowUpInfos.RemoveAll();
}

void TESConversationData::Copy(TESTopicInfo* apOwner, TESConversationData* apDataToCopy) {
    if (apDataToCopy) {
        ClearLists();
        BSSimpleList<TESTopic*>* pTopicList = &apDataToCopy->m_listLinkTo;
        TESTopic* pTopic = nullptr;
        while (pTopicList && pTopicList->m_item) {
            pTopic = pTopicList->m_item;
            pTopicList = pTopicList->m_pkNext;
            m_listLinkTo.AddTail(pTopic);
        }
        pTopicList = &apDataToCopy->m_listLinkFrom;
        pTopic = nullptr;
        while (pTopicList && pTopicList->m_item) {
            pTopic = pTopicList->m_item;
            pTopicList = pTopicList->m_pkNext;
            m_listLinkFrom.AddTail(pTopic);
        }
        BSSimpleList<TESTopicInfo*>* pTopicInfoList = apDataToCopy->GetFollowUpInfoList();
        while (pTopicInfoList) {
            TESTopicInfo* pTopicInfo = pTopicInfoList->m_item;
            if (!pTopicInfo) {
                break;
            }
            pTopicInfoList = pTopicInfoList->m_pkNext;
            GetFollowUpInfoList()->AddTail(pTopicInfo);
        }
    }
}

void TESResponse::SetResponseText(const char* apResponseText) {
    cResponseText.Set(apResponseText, 0);
}
