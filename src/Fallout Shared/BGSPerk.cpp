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
#if FNVD_64BIT
            // The file's record is 8 bytes, the quest's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(QuestPerkEntryData, pQuest)}});
#endif
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
    Data.cNumRanks = (uint8_t)1;
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
    BGSPerk* pperk = dynamic_cast<BGSPerk*>(apCopy);
    if (pperk) {
        ourMemcpy(&Data, &pperk->Data, sizeof(Data));
        CopyAllComponents(apCopy);
        PerkConditions.Copy(&pperk->PerkConditions, nullptr);
        for (BGSPerkEntry* pentry : pperk->PerkEntries) {
            if (pentry) {
                BGSPerkEntry* pduplicate = pentry->Duplicate();
                PerkEntries.AddTail(pduplicate);
            }
        }
    }
}

bool BGSPerk::Compare(TESForm* apCompare) {
    BGSPerk* pperk = dynamic_cast<BGSPerk*>(apCompare);
    if (!pperk) {
        return true;
    }
    if (Data.bPlayable != pperk->Data.bPlayable ||
        (Data.bTrait != pperk->Data.bTrait ||
         (Data.cLevel != pperk->Data.cLevel ||
          (Data.cNumRanks != pperk->Data.cNumRanks || Data.bHidden != pperk->Data.bHidden)))) {
        return true;
    }
    if (CompareAllComponents(pperk)) {
        return true;
    }
    if (PerkConditions.Compare(&pperk->PerkConditions)) {
        return true;
    }
    int32_t v3 = PerkEntries.ItemsInList();
    if (v3 != pperk->PerkEntries.ItemsInList()) {
        return true;
    }
    BSSimpleList<BGSPerkEntry*>* plist1 = &PerkEntries;
    BSSimpleList<BGSPerkEntry*>* plist2 = &pperk->PerkEntries;
    while (plist1) {
        BGSPerkEntry* pperkEntry1 = plist1->m_item;
        BGSPerkEntry* pperkEntry2 = plist2->m_item;
        if ((pperkEntry1 && !pperkEntry2) || (!pperkEntry1 && pperkEntry2)) {
            return true;
        }
        if (pperkEntry1 && (pperkEntry2 && pperkEntry1->Compare(pperkEntry2))) {
            return true;
        }
        plist1 = plist1->m_pkNext;
        plist2 = plist2->m_pkNext;
    }
    return false;
}

bool BGSPerk::IsPerkAvailable(TESObjectREFR* apActionRef) {
    uint8_t arg8;
    return (bool)((uint8_t)(&PerkConditions)->IsTrueFull(apActionRef, nullptr, (bool*)(&arg8), false));
}

uint8_t BGSPerk::GetNumRanks() {
    return Data.cNumRanks;
}

char BGSPerk::GetLevel() {
    return Data.cLevel;
}

bool BGSPerk::GetIsTrait() {
    return (bool)((uint8_t)Data.bTrait);
}

bool BGSPerk::IsPerkHidden() {
    return (bool)((uint8_t)Data.bHidden);
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
    BGSPerkEntry* v4;
    BSSimpleList<BGSPerkEntry*>* ecx0;
    for (ecx0 = &PerkEntries; ecx0; ecx0 = ecx0->m_pkNext) {
        v4 = ecx0->m_item;
        if ((v4) && ((((uint32_t)(v4->GetRank() + 1)) == ((uint32_t)acRank)) && (((uint32_t)v4->GetType()) == 2))) {
            v4->ApplyPerkEntry(apActor, bAlt);
        }
    }
}

void BGSPerk::RemovePerk(Actor* apActor, bool bAlt) {
    for (BGSPerkEntry* pperkEntry_2 : PerkEntries) {
        if (pperkEntry_2) {
            if (bAlt) {
                // The count is tested signed.
                int32_t iTeammateCount = pPlayer->GetTeammateCount();
                if (iTeammateCount > 0) {
                    for (BSSimpleList<Actor*>* pList = &pPlayer->pListOfTeammates; pList; pList = pList->m_pkNext) {
                        Actor* pActor = pList->m_item;
                        if (pperkEntry_2->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
                            pperkEntry_2->RemovePerkEntry(pPlayer, bAlt);
                        }
                        else {
                            pperkEntry_2->RemovePerkEntry(pActor, bAlt);
                        }
                    }
                }
            }
            else {
                pperkEntry_2->RemovePerkEntry(apActor, false);
            }
        }
    }
}

// The perk is playable when its DATA says so and every GetIsSex condition holds for the player.
bool BGSPerk::GetIsPlayable() {
    bool bresult = Data.bPlayable;
    if (bresult) {
        for (BSSimpleList<void*>* plist = &PerkConditions; bresult && (plist && !plist->IsEmpty());
             plist = plist->m_pkNext) {
            TESConditionItem* pitem = static_cast<TESConditionItem*>(plist->m_item);
            if ((pitem) && (pitem->GetConditionFunction() == FUNCTION_GET_IS_SEX - START_OF_FUNCTION_SECTION))
            // A condition item stores its function's index from the function section's start.
            {
                bool bignore;
                bresult = pitem->IsTrue(pPlayer, nullptr, bignore);
            }
        }
    }
    return bresult;
}

void BGSPerk::ApplyPerk(Actor* apActor, uint8_t acOldRank, uint8_t acNewRank, bool bAlt) {
    for (BGSPerkEntry* pperkEntry_2 : PerkEntries) {
        if (pperkEntry_2) {
            if (bAlt) {
                // The count is tested signed.
                int32_t iTeammateCount = pPlayer->GetTeammateCount();
                if (iTeammateCount > 0) {
                    for (BSSimpleList<Actor*>* pList = &pPlayer->pListOfTeammates; pList; pList = pList->m_pkNext) {
                        Actor* pActor = pList->m_item;
                        if (pperkEntry_2->GetType() == PERK_ENTRY_TYPE_ENTRY_POINT) {
                            pperkEntry_2->ApplyPerkEntry(pPlayer, bAlt);
                        }
                        else {
                            pperkEntry_2->ApplyPerkEntry(pActor, bAlt);
                        }
                    }
                }
            }
            else {
                if (pperkEntry_2->cRank + 1 == acOldRank) {
                    pperkEntry_2->RemovePerkEntry(apActor, false);
                }
                else {
                    if (pperkEntry_2->cRank + 1 == acNewRank) {
                        pperkEntry_2->ApplyPerkEntry(apActor, false);
                    }
                }
            }
        }
    }
}

bool BGSPerk::IsPerkAttainable(TESObjectREFR* apActionRef) {
    bool bresult = true;
    TESConditionItem* pcurrCondition = PerkConditions.GetConditionItem(0);
    int32_t iconditionIndex = 1;
    while (pcurrCondition) {
        if (pcurrCondition->Data.kFunctionData.usFunction == 70) {
            double freturnVal = 0.0;
            void* v3 = pcurrCondition->Data.kFunctionData.uParam[0].pPointer;
            Script::GetIsSexConditionFunction(apActionRef, v3, nullptr, freturnVal);
            bresult = 1.0 == freturnVal;
        }
        pcurrCondition = PerkConditions.GetConditionItem(iconditionIndex++);
    }
    return bresult;
}

void BGSPerk::BuildDescriptionString(char* apOutText, uint32_t aiOutSize) {
    char Src[132];
    char v22 = 0;
    TESConditionItem* pcurrConditon = PerkConditions.GetConditionItem(0);
    if (Data.cLevel <= 1u) {
        BSsprintf(apOutText, static_cast<int32_t>(aiOutSize), "%s: ", sRequirementsText.String());
        v22 = 1;
    }
    else {
        int32_t cLevel = Data.cLevel;
        const char* v3 = sLevelAbbrev.String();
        BSsprintf(apOutText, static_cast<int32_t>(aiOutSize), "%s: %s %d", sRequirementsText.String(), v3, cLevel);
    }
    char v21 = 0;
    int32_t iConditionIndex = 0;
    while (pcurrConditon) {
        // A condition item stores its function's index from the function section's start.
        if (pcurrConditon->GetConditionFunction() == FUNCTION_GET_PERMANENT_ACTOR_VALUE - START_OF_FUNCTION_SECTION) {
            ActorValue::Index eindex =
                static_cast<ActorValue::Index>(pcurrConditon->Data.kFunctionData.uParam[0].uiNumber);
            const char* cSymbol = " ";
            int32_t iValueMod = 0;
            ENUM_COMPARISON_CONDITION ComparisonCondition = pcurrConditon->GetComparisonCondition();
            switch (ComparisonCondition) {
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
            if ((eindex != ActorValue::KARMA) && (ActorValue::GetActorValueAbbreviation(eindex)) &&
                (iValueMod + static_cast<int32_t>(pcurrConditon->GetComparisonValue()))) {
                v21 = 1;
                if (!iConditionIndex && !v22) {
                    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
                }
                if (pcurrConditon->GetOr() && PerkConditions.GetConditionItem(iConditionIndex + 1)) {
                    const char* v6 = sOr.String();
                    int32_t v4 = iValueMod + static_cast<int32_t>(pcurrConditon->GetComparisonValue());
                    BSsprintf(Src, 128, " %s%s%d %s ", ActorValue::GetActorValueAbbreviation(eindex), cSymbol, v4, v6);
                }
                else {
                    if (PerkConditions.GetConditionItem(iConditionIndex + 1)) {
                        int32_t v7 = iValueMod + static_cast<int32_t>(pcurrConditon->GetComparisonValue());
                        BSsprintf(Src, 128, "%s%s%d, ", ActorValue::GetActorValueAbbreviation(eindex), cSymbol, v7);
                    }
                    else {
                        int32_t v8 = iValueMod + static_cast<int32_t>(pcurrConditon->GetComparisonValue());
                        BSsprintf(Src, 128, "%s%s%d", ActorValue::GetActorValueAbbreviation(eindex), cSymbol, v8);
                    }
                }
                BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), Src);
            }
        }
        else {
            if (pcurrConditon->GetConditionFunction() == FUNCTION_HAS_PERK - START_OF_FUNCTION_SECTION) {
                v21 = 1;
                if (!iConditionIndex && !v22) {
                    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
                }
                // HasPerk's parameters: the perk and its rank (the rank is unsigned).
                BGSPerk* pPerk = static_cast<BGSPerk*>(pcurrConditon->Data.kFunctionData.uParam[0].pPointer);
                uint32_t uiRank = pcurrConditon->Data.kFunctionData.uParam[1].uiNumber;
                if (pPerk && uiRank) {
                    BSsprintf(Src, 128, "%s %d", static_cast<TESFullName*>(pPerk)->GetFullName(), uiRank);
                }
                else {
                    if (pPerk) {
                        BSsprintf(Src, 128, "%s", static_cast<TESFullName*>(pPerk)->GetFullName());
                    }
                }
                BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), Src);
            }
            else {
                v21 = 1;
                if (!iConditionIndex && !v22) {
                    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), ", ");
                }
            }
        }
        int32_t iConditionIndex_2 = iConditionIndex + 1;
        pcurrConditon = PerkConditions.GetConditionItem(iConditionIndex_2);
        iConditionIndex = iConditionIndex_2 + 1;
    }
    if (!v21) {
        BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), "--");
    }
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), "\n");
    BSsprintf(Src, 128, "%s: %d\n\n", sRanksText.String(), Data.cNumRanks);
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), Src);
    const char* v9 = GetDescription(nullptr, DESC_ID);
    BSstrcat(apOutText, static_cast<int32_t>(aiOutSize), v9);
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
    double Float = apItem1->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha);
    if (apItem2->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha) > Float) {
        return 1;
    }
    double v7 = apItem1->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha);
    if (apItem2->pTile->GetFloat(g_levelUpMenu_Trait_TextAlpha) < v7) {
        return -1;
    }
    int32_t Level = apItem1->data->GetLevel();
    if (Level < apItem2->data->GetLevel()) {
        return -1;
    }
    int32_t v3 = apItem1->data->GetLevel();
    if (v3 > apItem2->data->GetLevel()) {
        return 1;
    }
    const char* String = apItem2->pTile->GetString(4036);
    const char* v4 = apItem1->pTile->GetString(4036);
    return BSstrcmp(v4, String);
}

BGSAbilityPerkEntry::BGSAbilityPerkEntry() : pAbility(nullptr) {}

BGSAbilityPerkEntry::~BGSAbilityPerkEntry() {}

PERK_ENTRY_TYPE BGSAbilityPerkEntry::GetType() {
    return PERK_ENTRY_TYPE_ABILITY;
}

void BGSAbilityPerkEntry::InitItem(TESForm* apOwner) {
    if (pAbility) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pAbility));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
        pAbility = dynamic_cast<SpellItem*>(FormByNumericID);
        if (!pAbility) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* v4 = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find ability (%08X) on owner object \"%s\".", itempint, v4);
            }
            else {
                int32_t CastingType = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find ability (%08X) on owner object (%08X).", itempint, CastingType);
            }
        }
    }
}

BGSPerkEntry* BGSAbilityPerkEntry::Duplicate() {
    BGSAbilityPerkEntry* v2;
    void* apMemory = ourMalloc(sizeof(BGSAbilityPerkEntry));
    if (apMemory) {
        v2 = ::new (apMemory) BGSAbilityPerkEntry;
    }
    else {
        v2 = nullptr;
    }
    v2->pAbility = pAbility;
    v2->Copy(this);
    return v2;
}

bool BGSAbilityPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSAbilityPerkEntry* v4 = dynamic_cast<BGSAbilityPerkEntry*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pAbility != v4->pAbility));
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
    bool bignore;
    if (aiNumFilterForms == Data.cNumConditions) {
        for (uint32_t i = 0; i < aiNumFilterForms; ++i) {
            TESForm* pform = apFilterForms[i];
            TESObjectREFR* pref = nullptr;
            TESObjectREFR tempRef;
            tempRef.SetTemporary();
            if (pform->IsBoundObject()) {
                tempRef.SetObjectReference(static_cast<TESBoundObject*>(pform));
                pref = &tempRef;
            }
            else {
                if (pform->IsReference()) {
                    pref = static_cast<TESObjectREFR*>(pform);
                }
            }
            if (pref) {
                if (!pConditionsArray[i].IsTrueFull(pref, nullptr, &bignore, false)) {
                    return false;
                }
            }
            else {
                // The filter is reported through pref, which is null on this path (not pform), as the game does.
                Warning(
                    "DEFAULT: Bad Filter form %08X %s sent to BGSEntryPointPerkEntry::CheckConditionFilters.",
                    pref->iFormID, pref->GetFormEditorID()
                );
            }
        }
        return true;
    }

    Warning(
        "FORMS: BGSEntryPointPerkEntry::CheckConditionFilters() was called with %i filter forms, but there are "
        "%i conditions",
        aiNumFilterForms, Data.cNumConditions
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
    if (pConditionsArray) {
        pConditionsArray->DeleteArray(3);
    }
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
    BGSEntryPointPerkEntry* pduplicate;
    BGSEntryPointPerkEntry* apMemory = static_cast<BGSEntryPointPerkEntry*>(ourMalloc(sizeof(BGSEntryPointPerkEntry)));
    if (apMemory) {
        pduplicate = ::new (apMemory) BGSEntryPointPerkEntry;
    }
    else {
        pduplicate = nullptr;
    }
    pduplicate->Data.cEntryPoint = Data.cEntryPoint;
    pduplicate->Data.cFunction = Data.cFunction;
    pduplicate->Data.cNumConditions = Data.cNumConditions;
    if (pFunctionData) {
        pduplicate->pFunctionData = pFunctionData->Duplicate();
    }
    if (Data.cNumConditions) {
        TESCondition* v3;
        int32_t cNumConditions = Data.cNumConditions;
        int32_t v1 =
            Data.cNumConditions <= SIZE_MAX / sizeof(TESCondition) ? sizeof(TESCondition) * cNumConditions : SIZE_MAX;
        size_t* v6 = static_cast<size_t*>(ourMalloc(
            ((static_cast<uint32_t>(v1) + static_cast<uint32_t>(sizeof(size_t))) < static_cast<uint32_t>(v1))
                ? -1
                : v1 + sizeof(size_t)
        ));
        if (v6) {
            *v6 = cNumConditions;
            rt::eh_vector_constructor_iterator(
                v6 + 1, sizeof(TESCondition), cNumConditions, &fnvd::constructElement<TESCondition>,
                &fnvd::destructElement<TESCondition>
            );
            // new[]'s array cookie: the count is in the 4 bytes before the elements.
            v3 = reinterpret_cast<TESCondition*>(v6 + 1);
        }
        else {
            v3 = nullptr;
        }
        pduplicate->pConditionsArray = v3;
        for (uint32_t i = 0; i < Data.cNumConditions; ++i) {
            pduplicate->pConditionsArray[i].Copy(&pConditionsArray[i], nullptr);
        }
    }
    pduplicate->Copy(this);
    return pduplicate;
}

bool BGSEntryPointPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSEntryPointPerkEntry* pentryPointPerkEntry = dynamic_cast<BGSEntryPointPerkEntry*>(apCompare);
    if (!pentryPointPerkEntry) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    if (Data.cEntryPoint != pentryPointPerkEntry->Data.cEntryPoint) {
        return true;
    }
    if (Data.cFunction != pentryPointPerkEntry->Data.cFunction) {
        return true;
    }
    if (Data.cNumConditions != pentryPointPerkEntry->Data.cNumConditions) {
        return true;
    }
    if ((pFunctionData && !pentryPointPerkEntry->pFunctionData) ||
        (!pFunctionData && pentryPointPerkEntry->pFunctionData)) {
        return true;
    }
    if (pFunctionData &&
        (pentryPointPerkEntry->pFunctionData && pFunctionData->Compare(pentryPointPerkEntry->pFunctionData))) {
        return true;
    }
    for (uint32_t i = 0; i < Data.cNumConditions; ++i) {
        if (pConditionsArray[i].Compare(&pentryPointPerkEntry->pConditionsArray[i])) {
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
    uint8_t cfunctionDataType;
    uint8_t ccurrentConditionsIndex;
    ccurrentConditionsIndex = 0;
    do {
        CHUNK_ID chunkid = apFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        if (chunkid > PRKF_ID) {
            if (chunkid > EPFT_ID) {
                if (chunkid == SCRV_ID || chunkid == SCTX_ID) {
                LABEL_37:;
                    if (!pFunctionData) {
                        Warning(
                            "MASTERFILE: Function data chunk is loading without function data existing on perk "
                            "entry"
                        );
                        return false;
                    }
                    pFunctionData->Load(apFile);
                }
            }
            else {
                if (chunkid == EPFT_ID) {
                    cfunctionDataType = 0;
                    if (apFile->GetChunkSize() != 1) {
                        return false;
                    }
                    apFile->GetChunkData(&cfunctionDataType, 0);
                    pFunctionData = (BGSEntryPointFunctionData*)CreateEntryPointFunctionObject(cfunctionDataType);
                    if (!pFunctionData) {
                        return false;
                    }
                }
                else {
                    if (chunkid == SCRO_ID || chunkid == SCHR_ID) {
                        goto LABEL_37;
                    }
                }
            }
        }
        else {
            if (chunkid == PRKF_ID) {
                return true;
            }
            if (chunkid > DATA_ID) {
                if (chunkid == PRKC_ID) {
                    apFile->GetChunkData(&ccurrentConditionsIndex, 1u);
                }
                else {
                    if (chunkid == EPFD_ID) {
                        goto LABEL_37;
                    }
                }
            }
            else {
                switch (chunkid) {
                    case DATA_ID: {
                        TESCondition* v4;
                        uint32_t isize = apFile->GetChunkSize();
                        if (isize != 3) {
                            return false;
                        }
                        apFile->GetChunkData(&Data, 3u);
                        int32_t cNumConditions = Data.cNumConditions;
                        int32_t v3 = Data.cNumConditions <= SIZE_MAX / sizeof(TESCondition)
                            ? sizeof(TESCondition) * cNumConditions
                            : SIZE_MAX;
                        size_t* v6 = static_cast<size_t*>(ourMalloc(
                            ((static_cast<uint32_t>(v3) + static_cast<uint32_t>(sizeof(size_t))) <
                             static_cast<uint32_t>(v3))
                                ? -1
                                : v3 + sizeof(size_t)
                        ));
                        if (v6) {
                            *v6 = cNumConditions;
                            rt::eh_vector_constructor_iterator(
                                v6 + 1, sizeof(TESCondition), cNumConditions, &fnvd::constructElement<TESCondition>,
                                &fnvd::destructElement<TESCondition>
                            );
                            // new[]'s array cookie: the count is in the 4 bytes before the elements.
                            v4 = reinterpret_cast<TESCondition*>(v6 + 1);
                        }
                        else {
                            v4 = nullptr;
                        }
                        pConditionsArray = v4;
                        break;
                    }
                    case EPF2_ID:
                    case SCDA_ID: {
                        goto LABEL_37;
                    }
                    case CIDA_ID: {
                        if (!pConditionsArray) {
                            return false;
                        }
                        pConditionsArray[ccurrentConditionsIndex].Load(apFile);
                        break;
                    }
                }
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
    return (bool)((uint8_t)((cRank != apCompare->cRank) || (cPriority != apCompare->cPriority)));
}

void BGSPerkEntry::StartPerkEntry(PERK_ENTRY_TYPE aePerkEntryType) {
    uint8_t th_s[4];
    th_s[2] = cPriority;
    th_s[1] = cRank;
    th_s[0] = static_cast<uint8_t>(aePerkEntryType);
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(PRKE_ID, th_s, 3u);
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
            BGSAbilityPerkEntry* v6 = static_cast<BGSAbilityPerkEntry*>(ourMalloc(sizeof(BGSAbilityPerkEntry)));
            if (v6) {
                return ::new (v6) BGSAbilityPerkEntry;
            }

            return nullptr;
        }

        if (aePerkEntryType == PERK_ENTRY_TYPE_ENTRY_POINT) {
            BGSEntryPointPerkEntry* v5 =
                static_cast<BGSEntryPointPerkEntry*>(ourMalloc(sizeof(BGSEntryPointPerkEntry)));
            if (v5) {
                return ::new (v5) BGSEntryPointPerkEntry;
            }

            return nullptr;
        }

        return nullptr;
    }

    BGSQuestPerkEntry* apMemory = static_cast<BGSQuestPerkEntry*>(ourMalloc(sizeof(BGSQuestPerkEntry)));
    if (apMemory) {
        return ::new (apMemory) BGSQuestPerkEntry;
    }

    return nullptr;
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
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(itempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
        Data.pQuest = dynamic_cast<TESQuest*>(FormByNumericID);
        if (!Data.pQuest) {
            if (apOwner->GetFormEditorIDLength()) {
                const char* v4 = apOwner->GetFormEditorID();
                Warning("MASTERFILE: Unable to find quest (%08X) on owner object \"%s\".", itempint, v4);
            }
            else {
                int32_t CastingType = apOwner->iFormID;
                Warning("MASTERFILE: Unable to find quest (%08X) on owner object (%08X).", itempint, CastingType);
            }
        }
    }
}

BGSPerkEntry* BGSQuestPerkEntry::Duplicate() {
    BGSQuestPerkEntry* v2;
    BGSQuestPerkEntry* apMemory = static_cast<BGSQuestPerkEntry*>(ourMalloc(sizeof(BGSQuestPerkEntry)));
    if (apMemory) {
        v2 = ::new (apMemory) BGSQuestPerkEntry;
    }
    else {
        v2 = nullptr;
    }
    // Copies the two fields, not the struct's padding.
    v2->Data.pQuest = Data.pQuest;
    v2->Data.cQuestStage = Data.cQuestStage;
    v2->Copy(this);
    return v2;
}

bool BGSQuestPerkEntry::Compare(BGSPerkEntry* apCompare) {
    BGSQuestPerkEntry* v4 = dynamic_cast<BGSQuestPerkEntry*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BGSPerkEntry::Compare(apCompare)) {
        return true;
    }
    if (Data.pQuest == v4->Data.pQuest) {
        return (bool)((uint8_t)(Data.cQuestStage != v4->Data.cQuestStage));
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
    TESForm::__AddChunkData(DATA_ID, &Data, 8);
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
