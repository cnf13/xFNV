#include "Fallout Shared/TESFaction.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Reads the faction's chunks. RNAM starts a new rank; the MNAM, FNAM and INAM chunks after it fill that rank's titles
// and insignia. When a chunk is empty (malformed data), the EDID, MNAM, FNAM and INAM buffers and the old-format flags
// byte are initialised to "" and 0 (allocated even for an empty chunk).
bool TESFaction::Load(TESFile* apFile) {
    RANK_DATA* pRankData = nullptr;
    LoadForm(apFile);
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // Not done by the game: the buffer is cleared first because an empty chunk leaves it unwritten (see
                // above).
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case XNAM_ID: {
                GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
                apFile->GetChunkData(pReaction, 0xC);
                // The file's record is 0xC bytes, the form's 4-byte ID first.
                TESFile::WidenFormIDRecords(
                    pReaction,
                    1,
                    0xC,
                    sizeof(GROUP_REACTION),
                    {{0, offsetof(GROUP_REACTION, pForm)}}
                );
                if (apFile->QEndian()) {
                    pReaction->Endian();
                }
                static_cast<TESReactionForm*>(this)->QReactionList()->AddTail(pReaction);
                break;
            }
            case DATA_ID:
                if (apFile->GetFormVersion() >= 9) {
                    LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
                    if (apFile->QEndian()) {
                        data.Endian();
                    }
                }
                else {
                    // Older files store the flags in one signed byte, which is sign-extended. An empty chunk (malformed
                    // data) would leave the value unwritten, so it is initialised to 0.
                    char cFlags = 0;
                    LoadData(apFile, &cFlags, sizeof(cFlags));
                    data.iFlags = cFlags;
                }
                break;
            case RNAM_ID: {
                pRankData = new RANK_DATA;
                QRankDataList()->AddTail(pRankData);
                break;
            }
            case MNAM_ID:
                if (pRankData) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                    // Not done by the game: the buffer is cleared first because an empty chunk leaves it unwritten (see
                    // above).
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0);
                    pRankData->cMaleRankTitle.Set(pBuffer, 0);
                }
                break;
            case FNAM_ID:
                if (pRankData) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                    // Not done by the game: the buffer is cleared first because an empty chunk leaves it unwritten (see
                    // above).
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0);
                    pRankData->cFemaleRankTitle.Set(pBuffer, 0);
                }
                break;
            case INAM_ID:
                if (pRankData) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                    // Not done by the game: the buffer is cleared first because an empty chunk leaves it unwritten (see
                    // above).
                    pBuffer[0] = '\0';
                    apFile->GetChunkData(pBuffer, 0);
                    pRankData->TextureInsignia.TESTexture::SetTextureName(pBuffer);
                }
                break;
            case WMI1_ID: {
                uint32_t uiReputation = 0;
                apFile->GetChunkData(uiReputation);
                // Stores the chunk's value in the pointer field unchanged.
                if (uiReputation) {
                    pReputation = reinterpret_cast<TESReputation*>(static_cast<uintptr_t>(uiReputation));
                }
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

bool Cmd_GetPCFactionAttack(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (apParam1 && static_cast<TESFaction*>(apParam1)->GetPCHasAttackedMember()) {
        afReturnValue = 1.0;
    }

    if (Script::bSystemOutput) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("PC did not attack a faction member.");
        }
        else {
            Interface::PrintLine("PC attacked a faction member.");
        }
    }
    return true;
}

bool Cmd_GetPCEnemyofFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (apParam1 && static_cast<TESFaction*>(apParam1)->GetPCAnEnemy()) {
        afReturnValue = 1.0;
    }

    if (Script::bSystemOutput) {
        if (0.0 == afReturnValue) {
            Interface::PrintLine("PC is not an enemy of this faction.");
        }
        else {
            Interface::PrintLine("PC Is an Enemy of the faction.");
        }
    }
    return true;
}

RANK_DATA::RANK_DATA() : cMaleRankTitle(), cFemaleRankTitle(), TextureInsignia() {}

RANK_DATA::~RANK_DATA() {}

void RANK_DATA::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void FACTION_DATA::Endian() {
    ENDIAN(iFlags, false);
}

TESFaction::TESFaction() : TESForm(), TESFullName(), TESReactionForm(), m_rankDataList() {
    TESFaction::InitializeData();
    SetFormType(FACT_ID);
}

TESFaction::~TESFaction() {
    TESFaction::ClearData();
}

void TESFaction::InitializeData() {
    iMajorCrime = 0;
    iMinorCrime = 0;
    pReputation = nullptr;
    ourMemset(&data.iFlags, 0, sizeof(data.iFlags));
}

void TESFaction::ClearData() {
    ClearRankDataList();
}

void TESFaction::Save() {
    StartForm();
    TESFullName::Save();
    TESReactionForm::Save();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    int32_t iRank = 0;
    for (BSSimpleList<RANK_DATA*>* pRankList = &m_rankDataList; pRankList && !pRankList->IsEmpty();
         pRankList = pRankList->m_pkNext) {
        RANK_DATA* pData = pRankList->m_item;
        TESForm::AddChunk(RNAM_ID, static_cast<uint32_t>(iRank));
        if (pData->cMaleRankTitle.GetLength()) {
            uint32_t uiLength = pData->cMaleRankTitle.GetLength();
            TESForm::AddChunkArray(MNAM_ID, pData->cMaleRankTitle.pString, uiLength + 1);
        }
        if (pData->cFemaleRankTitle.GetLength()) {
            uint32_t uiLength = pData->cFemaleRankTitle.GetLength();
            TESForm::AddChunkArray(FNAM_ID, pData->cFemaleRankTitle.pString, uiLength + 1);
        }
        if (pData->TextureInsignia.GetTextureNameLength()) {
            TESForm::AddChunkArray(
                INAM_ID,
                pData->TextureInsignia.GetTextureName(),
                pData->TextureInsignia.GetTextureNameLength() + 1
            );
        }
        ++iRank;
    }
    if (pReputation) {
        TESForm::AddChunk(WMI1_ID, pReputation->iFormID);
    }
    CloseForm();
}

uint16_t TESFaction::GetSaveSize(uint32_t aiFlags) {
    uint16_t usFormSize = TESForm::GetSaveSize(aiFlags);
    uint16_t usSaveSize = static_cast<uint16_t>(static_cast<TESReactionForm*>(this)->GetSaveSize(aiFlags) + usFormSize);
    if ((aiFlags & CHANGE_FACTION_FLAGS) != 0) {
        usSaveSize += 4;
    }
    return usSaveSize;
}

// Saves the form's changes into the save buffer: the base form's and the reactions', then the faction flags and the
// two crime counts when they changed.
void TESFaction::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    TESReactionForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_FLAGS)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&data), sizeof(data), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMajorCrime), sizeof(iMajorCrime), 0);
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMinorCrime), sizeof(iMinorCrime), 0);
    }
}

void TESFaction::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    SaveGameTES(aiFlags);
    if ((aiFlags & CHANGE_FACTION_FLAGS) != 0) {
        SaveGameDataOLD(&data, sizeof(data));
    }
}

// Loads the same changes back from the load buffer.
void TESFaction::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    TESReactionForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_FLAGS)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&data), sizeof(data));
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iMajorCrime), sizeof(iMajorCrime));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iMinorCrime), sizeof(iMinorCrime));
    }
}

void TESFaction::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    LoadGameTES(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_FACTION_FLAGS) != 0) {
        LoadGameDataOLD(&data, sizeof(data));
    }
}

void TESFaction::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESReactionForm::InitLoadGame(apLoadGameBuffer);
}

void TESFaction::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    TESForm::InitLoadGame(aiFlags, aiOldFlags);
    TESReactionForm::InitLoadGame(aiFlags, aiOldFlags);
}

void TESFaction::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
        iMajorCrime = 0;
        iMinorCrime = 0;
    }
}

void TESFaction::InitItem() {
    if (!GetInitialized()) {
        TESReactionForm::InitItem(this);
        SetInitialized(true);
        if (pReputation) {
            uint32_t itempint = 0;
            itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pReputation));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            pReputation = pDataHandler->GetReputation(itempint);
        }
    }
}

// Copies apCopy's components, flags, reputation and ranks.
void TESFaction::Copy(TESForm* apCopy) {
    TESFaction* pFaction = dynamic_cast<TESFaction*>(apCopy);
    if (!pFaction) {
        return;
    }
    CopyAllComponents(apCopy);
    ourMemcpy(&data, &pFaction->data, sizeof(FACTION_DATA));
    pReputation = pFaction->pReputation;
    ClearRankDataList();
    BSSimpleList<RANK_DATA*>* pRankList = QRankDataList();
    for (BSSimpleList<RANK_DATA*>* pNode = pFaction->QRankDataList(); pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        RANK_DATA* pRank = new RANK_DATA;
        pRank->cMaleRankTitle.Set(pNode->GetItem()->cMaleRankTitle.pString, 0);
        pRank->cFemaleRankTitle.Set(pNode->GetItem()->cFemaleRankTitle.pString, 0);
        // Copies the insignia through the virtual CopyComponent via a BaseFormComponent pointer; a call on the member
        // itself would be devirtualised.
        BaseFormComponent* pInsignia = &pRank->TextureInsignia;
        pInsignia->CopyComponent(&pNode->GetItem()->TextureInsignia);
        pRankList->AddTail(pRank);
        if (pRankList->GetNext()) {
            pRankList = pRankList->GetNext();
        }
    }
}

bool TESFaction::Compare(TESForm* apCompare) {
    TESFaction* pComp = dynamic_cast<TESFaction*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (std::memcmp(&data, &pComp->data, sizeof(data))) {
        return true;
    }
    if (pReputation != pComp->pReputation) {
        return true;
    }
    BSSimpleList<RANK_DATA*>* pRankList = &m_rankDataList;
    BSSimpleList<RANK_DATA*>* pCompRankList = &pComp->m_rankDataList;
    while (pRankList && !pRankList->IsEmpty()) {
        if (!pCompRankList || !pCompRankList->m_item) {
            return true;
        }
        RANK_DATA* pData = pRankList->m_item;
        RANK_DATA* pCompData = pCompRankList->m_item;
        if (pData->cMaleRankTitle.GetLength() || pCompData->cMaleRankTitle.GetLength()) {
            uint32_t uiLength = pData->cMaleRankTitle.GetLength();
            if (uiLength != pCompData->cMaleRankTitle.GetLength()) {
                return true;
            }
            if (pData->cMaleRankTitle.StrCmp(static_cast<const char*>(pCompData->cMaleRankTitle.pString), false)) {
                return true;
            }
        }
        if (pData->cFemaleRankTitle.GetLength() || pCompData->cFemaleRankTitle.GetLength()) {
            uint32_t uiLength = pData->cFemaleRankTitle.GetLength();
            if (uiLength != pCompData->cFemaleRankTitle.GetLength()) {
                return true;
            }
            if (pData->cFemaleRankTitle.StrCmp(static_cast<const char*>(pCompData->cFemaleRankTitle.pString), false)) {
                return true;
            }
        }
        if (pData->TextureInsignia.CompareComponent(&pCompData->TextureInsignia)) {
            return true;
        }
        pRankList = pRankList->m_pkNext;
        pCompRankList = pCompRankList->m_pkNext;
    }
    return pCompRankList && !pCompRankList->IsEmpty();
}

bool TESFaction::GetIsEvil() {
    return (data.iFlags & FACTION_DATA::FLAG_EVIL) != 0;
}

// Appends a new, empty rank data to the faction's rank list and returns its index: the count of the entries the walk
// passes before it.
int32_t TESFaction::AddRankData() {
    RANK_DATA* pRankData = new RANK_DATA;

    int32_t iRank = 0;
    BSSimpleList<RANK_DATA*>* pNode = QRankDataList();
    BSSimpleList<RANK_DATA*>* pLast = nullptr;
    while (pNode && !pNode->IsEmpty()) {
        ++iRank;
        pLast = pNode;
        pNode = pNode->GetNext();
    }

    if (pLast) {
        pLast->AddTail(pRankData);
    }
    else {
        QRankDataList()->AddHead(pRankData);
    }

    return iRank;
}

bool TESFaction::GetPCAnEnemy() {
    return (data.iFlags & FACTION_DATA::FLAG_PC_ENEMY) != 0;
}

bool TESFaction::GetAllowSell() {
    return (data.iFlags & FACTION_DATA::FLAG_ALLOW_SELL) != 0;
}

int32_t TESFaction::GetFactionReaction(TESFaction* pFaction) {
    return TESReactionForm::GetReaction(pFaction);
}

void TESFaction::ModFactionReaction(TESFaction* pFaction, int32_t reaction) {
    TESReactionForm::ModReaction(pFaction, reaction);
    AddChange(CHANGE_FACTION_REACTIONS);
}

void TESFaction::SetFactionReaction(TESFaction* pFaction, int32_t reaction) {
    TESReactionForm::SetFightReaction(pFaction, reaction);
    AddChange(CHANGE_FACTION_REACTIONS);
}

bool TESFaction::GetPCExpelled() {
    return (data.iFlags & FACTION_DATA::FLAG_PC_EXPELLED) != 0;
}

bool TESFaction::GetPCHasMurdereredMember() {
    return (data.iFlags & FACTION_DATA::FLAG_PC_MURDERED) != 0;
}

bool TESFaction::GetPCHasAttackedMember() {
    return (data.iFlags & FACTION_DATA::FLAG_PC_ATTACKED) != 0;
}

bool TESFaction::IsTrackCrime() {
    return (data.iFlags & FACTION_DATA::FLAG_TRACK_CRIME) != 0;
}

void TESFaction::SetPCMurder(bool abVal) {
    SetFlag(FACTION_DATA::FLAG_PC_MURDERED, abVal);
}

void TESFaction::SetPCAttack(bool abVal) {
    SetFlag(FACTION_DATA::FLAG_PC_ATTACKED, abVal);
}

bool TESFaction::GetSpecialCombat() {
    return (data.iFlags & FACTION_DATA::FLAG_SPECIAL_COMBAT) != 0;
}

void TESFaction::SetPCExpelled(bool abVal) {
    SetFlag(FACTION_DATA::FLAG_PC_EXPELLED, abVal);
}

void TESFaction::AddMinorCrime(int32_t aiNum, uint8_t abModRep) {
    iMinorCrime += aiNum;
    if (pReputation && abModRep) {
        pReputation->RemoveReputationMinorOrMajorCrime(0);
    }

    AddChange(CHANGE_FACTION_CRIME_COUNTS);
}

void TESFaction::ClearRankDataList() {
    while (!m_rankDataList.IsEmpty()) {
        RANK_DATA* pRankData = m_rankDataList.m_item;
        if (pRankData) {
            delete pRankData;
        }
        m_rankDataList.RemoveHead();
    }
}

void TESFaction::SetFlag(uint32_t aiFlag, bool abVal) {
    if (abVal) {
        data.iFlags |= aiFlag;
    }
    else {
        data.iFlags &= ~aiFlag;
    }
    AddChange(CHANGE_FACTION_FLAGS);
}

void TESFaction::HandleMajorCrimeReputation(int32_t aiNum, char abModRep) {
    iMajorCrime += aiNum;
    if (pReputation && abModRep) {
        pReputation->RemoveReputationMinorOrMajorCrime(1);
    }

    AddChange(CHANGE_FACTION_CRIME_COUNTS);
}

void TESFaction::SetFactionReaction_0(TESFaction* pFaction, int32_t reaction) {
    SetReaction(pFaction, reaction);
    AddChange(CHANGE_FACTION_REACTIONS);
}
