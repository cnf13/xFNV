#include "Fallout Shared/BGSPerk.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/BGSAbilityPerkEntry.h"
#include "Fallout Shared/BGSEntryPointFunction.h"
#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Fallout Shared/BGSEntryPointPerkEntry.h"
#include "Fallout Shared/BGSPerkEntry.h"
#include "Fallout Shared/BGSQuestPerkEntry.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// Reads DATA (8 bytes; the quest's form ID, which InitItem resolves, and the stage) up to the end of the entry
// (PRKF). A DATA chunk of the wrong size fails.
bool BGSQuestPerkEntry::Load(TESFile* apFile) {
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        if (eChunk == DATA_ID) {
            uint32_t uiSize = apFile->GetChunkSize();
            if (uiSize != 8) {
                return false;
            }
            apFile->GetChunkData(&Data, uiSize);
            // The file's record is 8 bytes, the quest's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(QuestPerkEntryData, pQuest)}});
            if (apFile->QEndian()) {
                Data.Endian();
            }
        }
        else if (eChunk == PRKF_ID) {
            return true;
        }
    }
    while (apFile->NextChunk());
    return true;
}

BGSPerk::BGSPerk() {
    SetFormType(86u);
    BGSPerk::InitializeData();
}

BGSPerk::~BGSPerk() {
    BGSPerk::ClearData();
}

void BGSPerk::InitializeData() {
    Data.bTrait = false;
    Data.cLevel = 0;
    Data.cNumRanks = 1;
    Data.bPlayable = true;
    Data.bHidden = false;
}

// Deletes each entry, then calls RemoveAll and ClearAllConditionItems.
void BGSPerk::ClearData() {
    for (BSSimpleList<BGSPerkEntry*>* pNode = &PerkEntries; pNode;) {
        BGSPerkEntry* pEntry;
        pEntry = pNode->GetItem();
        delete pEntry;
        pNode = pNode->GetNext();
    }
    PerkEntries.RemoveAll();
    PerkConditions.ClearAllConditionItems();
}

// Reads the perk's chunks. DATA comes in three sizes: the current five bytes, and the older three and four, whose
// missing bytes are filled with 0, 0, 1 and 1. PRKE holds a perk entry, which is read into three bytes, created from
// the first, initialised and loaded before being added.
bool BGSPerk::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != PERK_ID) {
        return false;
    }
    this->LoadForm(apFile);
    this->SetInitialized(false);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case DATA_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                if (uiChunkSize == 5) {
                    apFile->GetChunkData(&Data, 5u);
                }
                else if (uiChunkSize == 3) {
                    uint8_t aucData[3] = {0, 0, 1};
                    apFile->GetChunkData(aucData, 3u);
                    Data.bTrait = (aucData[0] != 0);
                    Data.cLevel = aucData[1];
                    Data.cNumRanks = aucData[2];
                }
                else if (uiChunkSize == 4) {
                    uint8_t aucData[4] = {0, 0, 1, 1};
                    apFile->GetChunkData(aucData, 4u);
                    Data.bTrait = (aucData[0] != 0);
                    Data.cLevel = aucData[1];
                    Data.cNumRanks = aucData[2];
                    Data.bPlayable = (aucData[3] != 0);
                }
                else {
                    return false;
                }
                break;
            }
            case DESC_ID: TESDescription::LoadDescriptionData(this, apFile); break;
            case CIDA_ID: PerkConditions.Load(apFile); break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case PRKE_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                if (uiChunkSize != 3) {
                    return false;
                }
                uint8_t aucData[3];
                apFile->GetChunkData(aucData, uiChunkSize);
                BGSPerkEntry* pEntry = BGSPerkEntry::CreatePerkEntryOfType(static_cast<PERK_ENTRY_TYPE>(aucData[0]));
                if (!pEntry) {
                    return false;
                }
                pEntry->InitializeFromSaveData(reinterpret_cast<PerkEntrySaveData*>(aucData));
                if (!pEntry->Load(apFile)) {
                    return false;
                }
                this->AddPerkEntry(pEntry);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSPerk::Save() {
    StartForm();
    TESFullName::Save();
    TESDescription::Save();
    TESTexture::Save(ICON_ID);
    PerkConditions.Save();
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(DATA_ID, &Data, 5u);
    if (TESForm::QEndianSwapOnSave()) {}
    for (BGSPerkEntry* pentry : PerkEntries) {
        if (pentry) {
            pentry->Save();
        }
    }
    CloseForm();
}

void BGSPerk::InitItem() {
    if (GetInitialized()) {
        return;
    }
    PerkConditions.InitItem(this);
    for (BSSimpleList<BGSPerkEntry*>* pNode = &PerkEntries; pNode;) {
        BGSPerkEntry* pEntry;
        pEntry = pNode->GetItem();
        if (pEntry) {
            pEntry->InitItem(this);
        }
        pNode = pNode->GetNext();
    }
    SetInitialized(true);
}

void BGSPerk::Copy(TESForm* apCopy) {
    ClearData();
    BGSPerk* pPerk = dynamic_cast<BGSPerk*>(apCopy);
    if (pPerk) {
        ourMemcpy(&Data, &pPerk->Data, sizeof(Data));
        CopyAllComponents(apCopy);
        PerkConditions.Copy(&pPerk->PerkConditions, nullptr);
        for (BGSPerkEntry* pEntry : pPerk->PerkEntries) {
            if (pEntry) {
                BGSPerkEntry* pDuplicate = pEntry->Duplicate();
                PerkEntries.AddTail(pDuplicate);
            }
        }
    }
}

bool BGSPerk::Compare(TESForm* apCompare) {
    BGSPerk* pPerk = dynamic_cast<BGSPerk*>(apCompare);
    if (!pPerk) {
        return true;
    }
    if (Data.bPlayable != pPerk->Data.bPlayable ||
        Data.bTrait != pPerk->Data.bTrait ||
        Data.cLevel != pPerk->Data.cLevel ||
        Data.cNumRanks != pPerk->Data.cNumRanks ||
        Data.bHidden != pPerk->Data.bHidden) {
        return true;
    }
    if (CompareAllComponents(pPerk)) {
        return true;
    }
    if (PerkConditions.Compare(&pPerk->PerkConditions)) {
        return true;
    }
    int32_t iCount = PerkEntries.ItemsInList();
    if (iCount != pPerk->PerkEntries.ItemsInList()) {
        return true;
    }
    BSSimpleList<BGSPerkEntry*>* pList1 = &PerkEntries;
    BSSimpleList<BGSPerkEntry*>* pList2 = &pPerk->PerkEntries;
    while (pList1) {
        BGSPerkEntry* pPerkEntry1 = pList1->m_item;
        BGSPerkEntry* pPerkEntry2 = pList2->m_item;
        if ((pPerkEntry1 && !pPerkEntry2) || (!pPerkEntry1 && pPerkEntry2)) {
            return true;
        }
        if (pPerkEntry1 && pPerkEntry2 && pPerkEntry1->Compare(pPerkEntry2)) {
            return true;
        }
        pList1 = pList1->m_pkNext;
        pList2 = pList2->m_pkNext;
    }
    return false;
}

bool BGSPerk::IsPerkAvailable(TESObjectREFR* apActionRef) {
    bool bDispositionFailure;
    return PerkConditions.IsTrueFull(apActionRef, nullptr, &bDispositionFailure, false);
}

uint8_t BGSPerk::GetNumRanks() {
    return Data.cNumRanks;
}

char BGSPerk::GetLevel() {
    return Data.cLevel;
}

bool BGSPerk::GetIsTrait() {
    return Data.bTrait;
}

bool BGSPerk::IsPerkHidden() {
    return Data.bHidden;
}

// Compares the entry's rank plus one, zero-extended, with acRank.
BGSPerkEntry* BGSPerk::GetPerkEntryByRank(uint8_t acRank) {
    BGSPerkEntry* pResult = nullptr;
    for (BSSimpleList<BGSPerkEntry*>* pNode = &PerkEntries; !pResult && pNode;) {
        BGSPerkEntry* pEntry;
        pEntry = pNode->GetItem();
        if (!pEntry) {
            break;
        }
        pEntry = pNode->GetItem();
        if (pEntry->GetRank() + 1 == acRank) {
            pResult = pNode->GetItem();
        }
        pNode = pNode->GetNext();
    }
    return pResult;
}

bool BGSPerk::AddPerkEntry(BGSPerkEntry* apPerkEntry) {
    if (!apPerkEntry || PerkEntries.IsInList(apPerkEntry)) {
        return false;
    }
    PerkEntries.Insert(apPerkEntry, &BGSPerk::PerkEntryRankSortFunction);
    return true;
}

void BGSPerk::AddPerkEntryPoints(Actor* apActor, uint8_t acRank, bool bAlt) {
    for (BSSimpleList<BGSPerkEntry*>* pList = &PerkEntries; pList; pList = pList->m_pkNext) {
        BGSPerkEntry* pPerkEntry = pList->m_item;
        if (pPerkEntry && pPerkEntry->GetRank() + 1 == acRank && pPerkEntry->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
            pPerkEntry->ApplyPerkEntry(apActor, bAlt);
        }
    }
}

void BGSPerk::RemovePerk(Actor* apActor, bool bAlt) {
    for (BGSPerkEntry* pPerkEntry : PerkEntries) {
        if (pPerkEntry) {
            if (bAlt) {
                // The count is tested signed.
                int32_t iTeammateCount = pPlayer->GetTeammateCount();
                if (iTeammateCount > 0) {
                    for (BSSimpleList<Actor*>* pList = &pPlayer->pListOfTeammates; pList; pList = pList->m_pkNext) {
                        Actor* pActor = pList->m_item;
                        if (pPerkEntry->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
                            pPerkEntry->RemovePerkEntry(pPlayer, bAlt);
                        }
                        else {
                            pPerkEntry->RemovePerkEntry(pActor, bAlt);
                        }
                    }
                }
            }
            else {
                pPerkEntry->RemovePerkEntry(apActor, false);
            }
        }
    }
}

// The perk is playable when its DATA says so and every GetIsSex condition holds for the player.
bool BGSPerk::GetIsPlayable() {
    bool bResult = Data.bPlayable;
    if (bResult) {
        for (BSSimpleList<void*>* pList = &PerkConditions; bResult && pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESConditionItem* pItem = static_cast<TESConditionItem*>(pList->m_item);
            if (pItem && pItem->GetConditionFunction() == FUNCTION_GET_IS_SEX - START_OF_FUNCTION_SECTION)
            // A condition item stores its function's index from the function section's start.
            {
                bool bIgnore;
                bResult = pItem->IsTrue(pPlayer, nullptr, bIgnore);
            }
        }
    }
    return bResult;
}

void BGSPerk::ApplyPerk(Actor* apActor, uint8_t acOldRank, uint8_t acNewRank, bool bAlt) {
    for (BGSPerkEntry* pPerkEntry : PerkEntries) {
        if (pPerkEntry) {
            if (bAlt) {
                // The count is tested signed.
                int32_t iTeammateCount = pPlayer->GetTeammateCount();
                if (iTeammateCount > 0) {
                    for (BSSimpleList<Actor*>* pList = &pPlayer->pListOfTeammates; pList; pList = pList->m_pkNext) {
                        Actor* pActor = pList->m_item;
                        if (pPerkEntry->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
                            pPerkEntry->ApplyPerkEntry(pPlayer, bAlt);
                        }
                        else {
                            pPerkEntry->ApplyPerkEntry(pActor, bAlt);
                        }
                    }
                }
            }
            else if (pPerkEntry->cRank + 1 == acOldRank) {
                pPerkEntry->RemovePerkEntry(apActor, false);
            }
            else if (pPerkEntry->cRank + 1 == acNewRank) {
                pPerkEntry->ApplyPerkEntry(apActor, false);
            }
        }
    }
}

bool BGSPerk::IsPerkAttainable(TESObjectREFR* apActionRef) {
    bool bResult = true;
    TESConditionItem* pCurrCondition = PerkConditions.GetConditionItem(0);
    int32_t iConditionIndex = 1;
    while (pCurrCondition) {
        if (pCurrCondition->Data.kFunctionData.usFunction == 70) {
            double dReturnVal = 0.0;
            Script::GetIsSexConditionFunction(
                apActionRef,
                pCurrCondition->Data.kFunctionData.uParam[0].pPointer,
                nullptr,
                dReturnVal
            );
            bResult = 1.0 == dReturnVal;
        }
        pCurrCondition = PerkConditions.GetConditionItem(iConditionIndex++);
    }
    return bResult;
}

void BGSPerk::BuildDescriptionString(char* apOutText, uint32_t aiOutSize) {
    char cText[132];
    char cNoLevel = 0;
    TESConditionItem* pCurrConditon = PerkConditions.GetConditionItem(0);
    if (Data.cLevel <= 1u) {
        BSsprintf(apOutText, static_cast<int32_t>(aiOutSize), "%s: ", sRequirementsText.String());
        cNoLevel = 1;
    }
    else {
        int32_t iLevel = Data.cLevel;
        const char* pcLevelAbbrev = sLevelAbbrev.String();
        BSsprintf(
            apOutText,
            static_cast<int32_t>(aiOutSize),
            "%s: %s %d",
            sRequirementsText.String(),
            pcLevelAbbrev,
            iLevel
        );
    }
    char cHasRequirement = 0;
    int32_t iConditionIndex = 0;
    while (pCurrConditon) {
        // A condition item stores its function's index from the function section's start.
        if (pCurrConditon->GetConditionFunction() == FUNCTION_GET_PERMANENT_ACTOR_VALUE - START_OF_FUNCTION_SECTION) {
            ActorValue::Index eIndex = static_cast<ActorValue::Index>(
                pCurrConditon->Data.kFunctionData.uParam[0].uiNumber
            );
            const char* cSymbol = " ";
            int32_t iValueMod = 0;
            switch (pCurrConditon->GetComparisonCondition()) {
                case CC_GREATER_THAN: {
                    iValueMod = -1;
                    break;
                }
                case CC_LESS_THAN: {
                    cSymbol = " < ";
                    break;
                }
                case CC_LESS_THAN_EQUAL: {
                    cSymbol = " < ";
                    iValueMod = 1;
                    break;
                }
            }
            if (eIndex != ActorValue::KARMA &&
                ActorValue::GetActorValueAbbreviation(eIndex) &&
                iValueMod + static_cast<int32_t>(pCurrConditon->GetComparisonValue())) {
                cHasRequirement = 1;
                if (!iConditionIndex && !cNoLevel) {
                    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
                }
                if (pCurrConditon->GetOr() && PerkConditions.GetConditionItem(iConditionIndex + 1)) {
                    const char* pcOr = sOr.String();
                    int32_t iValue = iValueMod + static_cast<int32_t>(pCurrConditon->GetComparisonValue());
                    BSsprintf(
                        cText,
                        128,
                        " %s%s%d %s ",
                        ActorValue::GetActorValueAbbreviation(eIndex),
                        cSymbol,
                        iValue,
                        pcOr
                    );
                }
                else if (PerkConditions.GetConditionItem(iConditionIndex + 1)) {
                    int32_t iValue = iValueMod + static_cast<int32_t>(pCurrConditon->GetComparisonValue());
                    BSsprintf(cText, 128, "%s%s%d, ", ActorValue::GetActorValueAbbreviation(eIndex), cSymbol, iValue);
                }
                else {
                    int32_t iValue = iValueMod + static_cast<int32_t>(pCurrConditon->GetComparisonValue());
                    BSsprintf(cText, 128, "%s%s%d", ActorValue::GetActorValueAbbreviation(eIndex), cSymbol, iValue);
                }
                BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), cText);
            }
        }
        else if (pCurrConditon->GetConditionFunction() == FUNCTION_HAS_PERK - START_OF_FUNCTION_SECTION) {
            cHasRequirement = 1;
            if (!iConditionIndex && !cNoLevel) {
                BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
            }
            // HasPerk's parameters: the perk and its rank (the rank is unsigned).
            BGSPerk* pPerk = static_cast<BGSPerk*>(pCurrConditon->Data.kFunctionData.uParam[0].pPointer);
            uint32_t uiRank = pCurrConditon->Data.kFunctionData.uParam[1].uiNumber;
            if (pPerk && uiRank) {
                BSsprintf(cText, 128, "%s %d", static_cast<TESFullName*>(pPerk)->GetFullName(), uiRank);
            }
            else if (pPerk) {
                BSsprintf(cText, 128, "%s", static_cast<TESFullName*>(pPerk)->GetFullName());
            }
            BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), cText);
        }
        else {
            cHasRequirement = 1;
            if (!iConditionIndex && !cNoLevel) {
                BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
            }
        }
        iConditionIndex = iConditionIndex + 1;
        pCurrConditon = PerkConditions.GetConditionItem(iConditionIndex);
        iConditionIndex = iConditionIndex + 1;
    }
    if (!cHasRequirement) {
        BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), "--");
    }
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), "\n");
    BSsprintf(cText, 128, "%s: %d\n\n", sRanksText.String(), Data.cNumRanks);
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), cText);
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), GetDescription(nullptr, DESC_ID));
}

int32_t BGSPerk::PerkEntryPrioritySortFunction(BGSPerkEntry* apPerkEntry1, BGSPerkEntry* apPerkEntry2) {
    if (apPerkEntry1->GetPriority() > apPerkEntry2->GetPriority()) {
        return -1;
    }
    if (apPerkEntry1->GetPriority() < apPerkEntry2->GetPriority()) {
        return 1;
    }
    if (apPerkEntry1->GetRank() < apPerkEntry2->GetRank()) {
        return -1;
    }
    return apPerkEntry1->GetRank() > apPerkEntry2->GetRank();
}

int32_t BGSPerk::PerkEntryRankSortFunction(BGSPerkEntry* apPerkEntry1, BGSPerkEntry* apPerkEntry2) {
    if (apPerkEntry1->cRank < apPerkEntry2->cRank) {
        return -1;
    }
    if (apPerkEntry1->cRank > apPerkEntry2->cRank) {
        return 1;
    }
    if (apPerkEntry1->cPriority > apPerkEntry2->cPriority) {
        return -1;
    }
    if (apPerkEntry1->cPriority < apPerkEntry2->cPriority) {
        return 1;
    }
    if (apPerkEntry1->GetType() < apPerkEntry2->GetType()) {
        return -1;
    }
    return apPerkEntry1->GetType() > apPerkEntry2->GetType();
}

int32_t BGSPerk::CompareForSorting(ListBoxItem<BGSPerk*>* apItem1, ListBoxItem<BGSPerk*>* apItem2) {
    double dAlpha = apItem1->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha);
    if (apItem2->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha) > dAlpha) {
        return 1;
    }
    dAlpha = apItem1->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha);
    if (apItem2->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha) < dAlpha) {
        return -1;
    }
    int32_t iLevel = apItem1->data->GetLevel();
    if (iLevel < apItem2->data->GetLevel()) {
        return -1;
    }
    iLevel = apItem1->data->GetLevel();
    if (iLevel > apItem2->data->GetLevel()) {
        return 1;
    }
    const char* pcName2 = apItem2->pTile->GetString(4036);
    const char* pcName1 = apItem1->pTile->GetString(4036);
    return BSstrcmp(pcName1, pcName2);
}

BGSAbilityPerkEntry::BGSAbilityPerkEntry() : pAbility(nullptr) {}

BGSAbilityPerkEntry::~BGSAbilityPerkEntry() {}

PERK_ENTRY_TYPE BGSAbilityPerkEntry::GetType() {
    return PERK_ENTRY_TYPE_ABILITY;
}

void BGSAbilityPerkEntry::InitItem(TESForm* apOwner) {
    if (pAbility) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAbility));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(itempint);
        pAbility = dynamic_cast<SpellItem*>(pForm);
        if (!pAbility) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* pcEditorID = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find ability (%08X) on owner object \"%s\".", itempint, pcEditorID);
            }
            else {
                int32_t iFormID = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find ability (%08X) on owner object (%08X).", itempint, iFormID);
            }
        }
    }
}

BGSPerkEntry* BGSAbilityPerkEntry::Duplicate() {
    BGSAbilityPerkEntry* pDuplicate = new BGSAbilityPerkEntry;
    pDuplicate->pAbility = pAbility;
    pDuplicate->Copy(this);
    return pDuplicate;
}

bool BGSAbilityPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSAbilityPerkEntry* pCompare = dynamic_cast<BGSAbilityPerkEntry*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    return pAbility != pCompare->pAbility;
}

// Writes the ability's form ID as the DATA chunk between the entry's header and closing chunks.
void BGSAbilityPerkEntry::Save() {
    StartPerkEntry(PERK_ENTRY_TYPE_ABILITY);
    if (pAbility) {
        TESForm::AddChunk(DATA_ID, pAbility->GetFormID());
    }
    FinishPerkEntry();
}

// Reads the ability's form ID (DATA) into pAbility, which InitItem resolves, up to the end of the entry (PRKF). A
// DATA chunk of the wrong size fails.
bool BGSAbilityPerkEntry::Load(TESFile* apFile) {
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        if (eChunk == DATA_ID) {
            if (apFile->GetChunkSize() != 4) {
                return false;
            }
            uint32_t iFormID = 0;
            apFile->GetChunkData(&iFormID, 0);
            if (apFile->QEndian()) {
                ENDIAN(iFormID, false);
            }
            pAbility = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(iFormID));
        }
        else if (eChunk == PRKF_ID) {
            return true;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSAbilityPerkEntry::ApplyPerkEntry(Actor* apActor, bool bAlt) {
    if (pAbility) {
        apActor->AddSpell(pAbility);
        apActor->UpdateMagic(false, false);
    }
}

void BGSAbilityPerkEntry::RemovePerkEntry(Actor* apActor, bool bAlt) {
    if (pAbility) {
        apActor->RemoveSpell(pAbility);
        apActor->UpdateMagic(false, false);
    }
}

BGSEntryPointPerkEntry::BGSEntryPointPerkEntry() : BGSPerkEntry(), pFunctionData(nullptr), pConditionsArray(nullptr) {
    Data.cNumConditions = 0;
    Data.cFunction = 0;
}

bool BGSEntryPointPerkEntry::CheckConditionFilters(uint32_t aiNumFilterForms, TESForm** apFilterForms) {
    bool bIgnore;
    if (aiNumFilterForms == Data.cNumConditions) {
        for (uint32_t i = 0; i < aiNumFilterForms; ++i) {
            TESForm* pForm = apFilterForms[i];
            TESObjectREFR* pRef = nullptr;
            TESObjectREFR tempRef;
            tempRef.SetTemporary();
            if (pForm->IsBoundObject()) {
                tempRef.SetObjectReference(static_cast<TESBoundObject*>(pForm));
                pRef = &tempRef;
            }
            else if (pForm->IsReference()) {
                pRef = static_cast<TESObjectREFR*>(pForm);
            }
            if (pRef) {
                if (!pConditionsArray[i].IsTrueFull(pRef, nullptr, &bIgnore, false)) {
                    return false;
                }
            }
            else {
                // The filter is reported through pRef, which is null on this path (not pForm).
                Warning(
                    "DEFAULT: Bad Filter form %08X %s sent to BGSEntryPointPerkEntry::CheckConditionFilters.",
                    pRef->iFormID,
                    pRef->GetFormEditorID()
                );
            }
        }
        return true;
    }

    Warning(
        "FORMS: BGSEntryPointPerkEntry::CheckConditionFilters() was called with %i filter forms, but there are "
        "%i conditions",
        aiNumFilterForms,
        Data.cNumConditions
    );
    return false;
}

uint8_t BGSEntryPointPerkEntry::GetFunction() {
    return Data.cFunction;
}

BGSEntryPointPerkEntry::~BGSEntryPointPerkEntry() {
    BGSEntryPointPerkEntry::ClearData();
}

PERK_ENTRY_TYPE BGSEntryPointPerkEntry::GetType() {
    return PERK_ENTRY_TYPE_ENTRY_POINT;
}

// The conditions array is deleted as an array, the function data is deleted, then the data bytes and the two pointers
// are cleared.
void BGSEntryPointPerkEntry::ClearData() {
    delete[] pConditionsArray;
    if (pFunctionData) {
        delete pFunctionData;
    }
    Data.cEntryPoint = 0;
    Data.cFunction = 0;
    Data.cNumConditions = 0;
    pConditionsArray = nullptr;
    pFunctionData = nullptr;
}

// Each filter condition, then the function data through its virtual InitItem (slot 0x18).
void BGSEntryPointPerkEntry::InitItem(TESForm* apOwner) {
    for (uint32_t i = 0; i < Data.cNumConditions; i++) {
        pConditionsArray[i].InitItem(apOwner);
    }
    if (pFunctionData) {
        pFunctionData->InitItem(apOwner);
    }
}

BGSPerkEntry* BGSEntryPointPerkEntry::Duplicate() {
    BGSEntryPointPerkEntry* pDuplicate = new BGSEntryPointPerkEntry;
    pDuplicate->Data.cEntryPoint = Data.cEntryPoint;
    pDuplicate->Data.cFunction = Data.cFunction;
    pDuplicate->Data.cNumConditions = Data.cNumConditions;
    if (pFunctionData) {
        pDuplicate->pFunctionData = pFunctionData->Duplicate();
    }
    if (Data.cNumConditions) {
        pDuplicate->pConditionsArray = new TESCondition[Data.cNumConditions];
        for (uint32_t i = 0; i < Data.cNumConditions; ++i) {
            pDuplicate->pConditionsArray[i].Copy(&pConditionsArray[i], nullptr);
        }
    }
    pDuplicate->Copy(this);
    return pDuplicate;
}

bool BGSEntryPointPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSEntryPointPerkEntry* pEntryPointPerkEntry = dynamic_cast<BGSEntryPointPerkEntry*>(apCompare);
    if (!pEntryPointPerkEntry) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    if (Data.cEntryPoint != pEntryPointPerkEntry->Data.cEntryPoint) {
        return true;
    }
    if (Data.cFunction != pEntryPointPerkEntry->Data.cFunction) {
        return true;
    }
    if (Data.cNumConditions != pEntryPointPerkEntry->Data.cNumConditions) {
        return true;
    }
    if ((pFunctionData && !pEntryPointPerkEntry->pFunctionData) ||
        (!pFunctionData && pEntryPointPerkEntry->pFunctionData)) {
        return true;
    }
    if (pFunctionData &&
        pEntryPointPerkEntry->pFunctionData &&
        pFunctionData->Compare(pEntryPointPerkEntry->pFunctionData)) {
        return true;
    }
    for (uint32_t i = 0; i < Data.cNumConditions; ++i) {
        if (pConditionsArray[i].Compare(&pEntryPointPerkEntry->pConditionsArray[i])) {
            return true;
        }
    }
    return false;
}

void BGSEntryPointPerkEntry::Save() {
    StartPerkEntry(PERK_ENTRY_TYPE_ENTRY_POINT);
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(DATA_ID, &Data, 3u);
    if (TESForm::QEndianSwapOnSave()) {}
    for (uint8_t i = 0; i < static_cast<int32_t>(Data.cNumConditions); ++i) {
        if (pConditionsArray[i].GetConditionItem(0)) {
            TESForm::AddChunk(PRKC_ID, i);
            pConditionsArray[i].Save();
        }
    }
    if (pFunctionData) {
        pFunctionData->Save();
    }
    FinishPerkEntry();
}

bool BGSEntryPointPerkEntry::Load(TESFile* apFile) {
    uint8_t cFunctionDataType;
    uint8_t cCurrentConditionsIndex;
    cCurrentConditionsIndex = 0;
    do {
        CHUNK_ID eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case PRKF_ID: {
                return true;
            }
            case SCRV_ID:
            case SCTX_ID:
            case SCRO_ID:
            case SCHR_ID:
            case EPFD_ID:
            case EPF2_ID:
            case SCDA_ID: {
                if (!pFunctionData) {
                    Warning(
                        "MASTERFILE: Function data chunk is loading without function data existing on perk "
                        "entry"
                    );
                    return false;
                }
                pFunctionData->Load(apFile);
                break;
            }
            case EPFT_ID: {
                cFunctionDataType = 0;
                if (apFile->GetChunkSize() != 1) {
                    return false;
                }
                apFile->GetChunkData(&cFunctionDataType, 0);
                pFunctionData = CreateEntryPointFunctionObject(cFunctionDataType);
                if (!pFunctionData) {
                    return false;
                }
                break;
            }
            case PRKC_ID: {
                apFile->GetChunkData(&cCurrentConditionsIndex, 1u);
                break;
            }
            case DATA_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                if (uiSize != 3) {
                    return false;
                }
                apFile->GetChunkData(&Data, 3u);
                pConditionsArray = new TESCondition[Data.cNumConditions];
                break;
            }
            case CIDA_ID: {
                if (!pConditionsArray) {
                    return false;
                }
                pConditionsArray[cCurrentConditionsIndex].Load(apFile);
                break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSEntryPointPerkEntry::ApplyPerkEntry(Actor* apActor, bool bAlt) {
    apActor->AddPerkEntry(this, bAlt);
}

// The same for the actor's RemovePerkEntry (slot 0x12A).
void BGSEntryPointPerkEntry::RemovePerkEntry(Actor* apActor, bool bAlt) {
    apActor->RemovePerkEntry(this, bAlt);
}

BGSPerkEntry::BGSPerkEntry() : cRank(0), cPriority(0) {}

BGSPerkEntry::~BGSPerkEntry() {}

// The ability, entry point and quest entries inherit this slot.
void BGSPerkEntry::Copy(BGSPerkEntry* apCopy) {
    cRank = apCopy->cRank;
    cPriority = apCopy->cPriority;
}

bool BGSPerkEntry::Compare(BGSPerkEntry* apCompare) {
    return cRank != apCompare->cRank || cPriority != apCompare->cPriority;
}

void BGSPerkEntry::StartPerkEntry(PERK_ENTRY_TYPE aePerkEntryType) {
    uint8_t cHeader[4];
    cHeader[2] = cPriority;
    cHeader[1] = cRank;
    cHeader[0] = static_cast<uint8_t>(aePerkEntryType);
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(PRKE_ID, cHeader, 3u);
    if (TESForm::QEndianSwapOnSave()) {}
}

void BGSPerkEntry::FinishPerkEntry() {
    TESForm::AddChunk(PRKF_ID);
}

uint8_t BGSPerkEntry::GetRank() {
    return cRank;
}

uint8_t BGSPerkEntry::GetPriority() {
    return cPriority;
}

void BGSPerkEntry::InitializeFromSaveData(PerkEntrySaveData* apSaveData) {
    cRank = apSaveData->cRank;
    cPriority = apSaveData->cPriority;
}

BGSPerkEntry* BGSPerkEntry::CreatePerkEntryOfType(PERK_ENTRY_TYPE aePerkEntryType) {
    if (aePerkEntryType) {
        if (aePerkEntryType == PERK_ENTRY_TYPE_ABILITY) {
            return new BGSAbilityPerkEntry;
        }

        if (aePerkEntryType == PERK_ENTRY_TYPE_ENTRY_POINT) {
            return new BGSEntryPointPerkEntry;
        }

        return nullptr;
    }

    return new BGSQuestPerkEntry;
}

void QuestPerkEntryData::Endian() {
    ENDIAN(pQuest, false);
}

BGSQuestPerkEntry::BGSQuestPerkEntry() {
    Data.pQuest = nullptr;
    Data.cQuestStage = 0;
}

BGSQuestPerkEntry::~BGSQuestPerkEntry() {}

PERK_ENTRY_TYPE BGSQuestPerkEntry::GetType() {
    return PERK_ENTRY_TYPE_QUEST;
}

void BGSQuestPerkEntry::InitItem(TESForm* apOwner) {
    if (Data.pQuest) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pQuest));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(itempint);
        Data.pQuest = dynamic_cast<TESQuest*>(pForm);
        if (!Data.pQuest) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* pcEditorID = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find quest (%08X) on owner object \"%s\".", itempint, pcEditorID);
            }
            else {
                int32_t iFormID = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find quest (%08X) on owner object (%08X).", itempint, iFormID);
            }
        }
    }
}

BGSPerkEntry* BGSQuestPerkEntry::Duplicate() {
    BGSQuestPerkEntry* pDuplicate = new BGSQuestPerkEntry;
    // Copies the two fields, not the struct's padding.
    pDuplicate->Data.pQuest = Data.pQuest;
    pDuplicate->Data.cQuestStage = Data.cQuestStage;
    pDuplicate->Copy(this);
    return pDuplicate;
}

bool BGSQuestPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSQuestPerkEntry* pCompare = dynamic_cast<BGSQuestPerkEntry*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    if (Data.pQuest == pCompare->Data.pQuest) {
        return Data.cQuestStage != pCompare->Data.cQuestStage;
    }
    return true;
}

// Writes the PRKE header, then DATA with the quest swapped for its form ID, then PRKF.
void BGSQuestPerkEntry::Save() {
    StartPerkEntry(PERK_ENTRY_TYPE_QUEST);
    TESQuest* pQuest = Data.pQuest;
    if (Data.pQuest) {
        Data.pQuest = reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(Data.pQuest->GetFormID()));
    }
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    // The file's record is Load's 8 bytes, the quest's 4-byte form ID first; the native layout is spread back after
    // the write.
    TESFile::NarrowFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(QuestPerkEntryData, pQuest)}});
    TESForm::__AddChunkData(DATA_ID, &Data, 8);
    TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(QuestPerkEntryData, pQuest)}});
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    Data.pQuest = pQuest;
    FinishPerkEntry();
}

void BGSQuestPerkEntry::ApplyPerkEntry(Actor* apActor, bool bAlt) {
    if (Data.pQuest) {
        Data.pQuest->SetStageDone(Data.cQuestStage);
    }
}
