#include "Fallout Shared/TESChallenge.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSEntryPointFunctionDataActivateChoice.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> sChallengeComplete("sChallengeComplete", "Challenge Completed");
SettingT<GameSettingCollection> sChallengeProgress("sChallengeProgress", "Challenge Progress");
SettingT<INISettingCollection> bShowChallengeUpdates("bShowChallengeUpdates:GamePlay", true);
TESChallengeManager::ChallengeListTable TESChallengeManager::pInactiveChallengeList;
TESChallengeManager::ChallengeListTable TESChallengeManager::pChallengeList;

void CHALLENGE_DATA::Endian() {
    ENDIAN(nChallengeType, false);
    ENDIAN(iThreshold, false);
    ENDIAN(nFlags, false);
    ENDIAN(iInterval, false);
    ENDIAN(nSpecialDataOne, false);
    ENDIAN(nSpecialDataTwo, false);
    ENDIAN(nSpecialDataThree, false);
}

TESChallenge::TESChallenge() {
    TESChallenge::InitializeData();
    SetFormType(CHAL_ID);
}

TESChallenge::~TESChallenge() {}

void TESChallenge::InitializeData() {
    ourMemset(&data, 0, sizeof(CHALLENGE_DATA));
    ourMemset(&progress, 0, sizeof(CHALLENGE_PROGRESS));
    pFormOne = nullptr;
    pFormTwo = nullptr;
    SetShowZero(true);
}

bool TESChallenge::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case DATA_ID:
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case OBND_ID: LoadObjectBound(apFile); break;
            case SCRI_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiFormID)));
                pScriptableForm->InitItem(this);
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(this, apFile); break;
            case SNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                SetFormOne(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            case XNAM_ID: {
                uint32_t uiFormID = 0;
                apFile->GetChunkData(uiFormID);
                SetFormTwo(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID)));
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESChallenge::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESTexture*>(this)->Save(ICON_ID);
    static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<TESDescription*>(this)->Save();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    if (pFormOne) {
        TESForm::AddChunk(SNAM_ID, pFormOne->TESForm::GetFormID());
    }
    if (pFormTwo) {
        TESForm::AddChunk(XNAM_ID, pFormTwo->TESForm::GetFormID());
    }
    TESForm::CloseForm();
}

uint16_t TESChallenge::GetSaveSize(uint32_t aiFlags) {
    return TESForm::GetSaveSize(aiFlags);
}

void TESChallenge::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
}

// The form's data, then the progress and its flags.
void TESChallenge::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&progress.iProgress), sizeof(progress.iProgress), 0);
    apSaveGameBuffer
        ->SaveDataEndian(reinterpret_cast<char*>(&progress.nProgressFlags), sizeof(progress.nProgressFlags), 0);
}

void TESChallenge::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESChallenge::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&progress.iProgress), sizeof(progress.iProgress));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&progress.nProgressFlags), sizeof(progress.nProgressFlags));
}

void TESChallenge::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormOne));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, GetFile(-1));
            pFormOne = TESForm::GetFormByNumericID(itempInt);
            if (!pFormOne) {
                int32_t iChallengeID = iFormID;
                Warning(
                    "MASTERFILE: Could not find Form (%08X) on challenge '%s' (%08X).",
                    itempInt,
                    GetFormEditorID(),
                    iChallengeID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pFormTwo));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, GetFile(-1));
            pFormTwo = TESForm::GetFormByNumericID(itempInt);
            if (!pFormTwo) {
                int32_t iChallengeID = iFormID;
                Warning(
                    "MASTERFILE: Could not find Form (%08X) on challenge '%s' (%08X).",
                    itempInt,
                    GetFormEditorID(),
                    iChallengeID
                );
            }
        }
        SetInitialized(true);
    }
}

void TESChallenge::Copy(TESForm* apCopy) {
    TESChallenge* pChallenge = dynamic_cast<TESChallenge*>(apCopy);
    if (pChallenge) {
        ourMemcpy(
            &data.nChallengeType,
            &pChallenge->data,
            sizeof(CHALLENGE_DATA) - offsetof(CHALLENGE_DATA, nChallengeType)
        );
        SetFormOne(static_cast<TESWaterForm*>(pChallenge->GetFormOne()));
        SetFormTwo(static_cast<TESWaterForm*>(pChallenge->GetFormTwo()));
        TESForm::CopyAllComponents(apCopy);
    }
}

// True when the forms differ: data, the two forms, then the components.
bool TESChallenge::Compare(TESForm* apCompare) {
    TESChallenge* pChallenge = dynamic_cast<TESChallenge*>(apCompare);
    if (!pChallenge) {
        return true;
    }
    if (std::memcmp(&data, pChallenge->GetData(), sizeof(CHALLENGE_DATA))) {
        return true;
    }
    if (GetFormTwo() != pChallenge->GetFormTwo()) {
        return true;
    }
    if (GetFormOne() != pChallenge->GetFormOne()) {
        return true;
    }
    return CompareAllComponents(pChallenge);
}

CHALLENGE_TYPE TESChallenge::GetChallengeType() {
    return static_cast<CHALLENGE_TYPE>(data.nChallengeType);
}

void TESChallenge::MarkForSave() {
    AddChange(CHANGE_BASE_OBJECT_VALUE);
}

void TESChallenge::SetShowZero(bool abVal) {
    if (abVal) {
        data.nFlags |= CHALLENGE_DATA::CHALLENGE_SHOW_ZERO;
    }
    else {
        data.nFlags &= ~CHALLENGE_DATA::CHALLENGE_SHOW_ZERO;
    }
}

TESForm* TESChallenge::GetFormTwo() {
    return pFormTwo;
}

TESForm* TESChallenge::GetFormOne() {
    return pFormOne;
}

CHALLENGE_DATA* TESChallenge::GetData() {
    return &data;
}

int32_t TESChallenge::GetUnlocked() {
    return progress.nProgressFlags & CHALLENGE_PROGRESS::PROGRESS_UNLOCKED;
}

uint32_t TESChallenge::GetStartsLocked() {
    return data.nFlags & CHALLENGE_DATA::CHALLENGE_STARTS_LOCKED;
}

int32_t TESChallenge::GetCompleted() {
    return progress.nProgressFlags & CHALLENGE_PROGRESS::PROGRESS_COMPLETED;
}

void TESChallenge::ClearProgress() {
    progress.iProgress = 0;
}

int32_t TESChallenge::GetRecurring() {
    return data.nFlags & CHALLENGE_DATA::CHALLENGE_RECURRING;
}

int32_t TESChallenge::GetSpecialOne() {
    return data.nSpecialDataOne;
}

int32_t TESChallenge::GetSpecialThree() {
    return data.nSpecialDataThree;
}

void TESChallenge::SetCompleted(bool abVal) {
    if (abVal) {
        progress.nProgressFlags |= CHALLENGE_PROGRESS::PROGRESS_COMPLETED;
    }
    else {
        progress.nProgressFlags &= ~CHALLENGE_PROGRESS::PROGRESS_COMPLETED;
    }
    MarkForSave();
}

void TESChallenge::SetCompletedRecurring(bool abVal) {
    if (abVal) {
        progress.nProgressFlags |= CHALLENGE_PROGRESS::PROGRESS_COMPLETED_RECURRING;
    }
    else {
        progress.nProgressFlags &= ~CHALLENGE_PROGRESS::PROGRESS_COMPLETED_RECURRING;
    }
    MarkForSave();
}

int32_t TESChallenge::GetRemovedRecurring() {
    return progress.nProgressFlags & CHALLENGE_PROGRESS::PROGRESS_REMOVED_RECURRING;
}

void TESChallenge::IncrementProgress(int32_t iValue) {
    progress.iProgress += iValue;
    MarkForSave();
}

void TESChallenge::SetUnlocked(bool abVal) {
    if (abVal) {
        progress.nProgressFlags |= CHALLENGE_PROGRESS::PROGRESS_UNLOCKED;
    }
    else {
        progress.nProgressFlags &= ~CHALLENGE_PROGRESS::PROGRESS_UNLOCKED;
    }
    MarkForSave();
}

int32_t TESChallenge::GetSpecialTwo() {
    return data.nSpecialDataTwo;
}

bool TESChallenge::GetShowZero() {
    return (data.nFlags & CHALLENGE_DATA::CHALLENGE_SHOW_ZERO) != 0;
}

int32_t TESChallenge::GetCompletedRecurring() {
    return progress.nProgressFlags & CHALLENGE_PROGRESS::PROGRESS_COMPLETED_RECURRING;
}

void TESChallenge::RemoveRecurringFromChallenge(char acRemove) {
    if (acRemove) {
        progress.nProgressFlags |= CHALLENGE_PROGRESS::PROGRESS_REMOVED_RECURRING;
    }
    else {
        progress.nProgressFlags &= ~CHALLENGE_PROGRESS::PROGRESS_REMOVED_RECURRING;
    }
    MarkForSave();
}

bool TESChallengeManager::UpdateChallenge(
    CHALLENGE_TYPE cType,
    int32_t iCount,
    TESForm* pFormOne,
    TESForm* pFormTwo,
    uint16_t nSpecialDataOne,
    uint16_t nSpecialDataTwo,
    uint16_t nSpecialDataThree
) {
    // The type is tested signed, against the enum's constant, not the class's unsigned CHALLENGE_TYPE_COUNT.
    if (cType >= CHALLENGE_TYPE_COUNT || cType < 0) {
        return false;
    }
    BSSimpleList<TESChallenge*>* pChallenges = &TESChallengeManager::pInactiveChallengeList[cType];
    bool bCompletedChallenges = false;
    TESChallenge* pItem = nullptr;
    for (; pChallenges; pChallenges = pChallenges->m_pkNext) {
        pItem = pChallenges->m_item;
        bool bFound = true;
        if (!pItem) {
            continue;
        }
        if (!pItem->GetCompleted() || pItem->GetRecurring()) {
            if (!pItem->GetStartsLocked() || pItem->GetUnlocked()) {
                if (pItem->GetFormOne()) {
                    if (pFormOne) {
                        switch (cType) {
                            case CHALLENGE_TYPE_KILL_LIST:
                            case CHALLENGE_TYPE_USE_ITEM_LIST:
                            case CHALLENGE_TYPE_AQUIRE_LIST: {
                                BGSListForm* pformlist = static_cast<BGSListForm*>(pItem->GetFormOne());
                                if (pformlist && !pformlist->ListOfForms.IsInList(pFormOne)) {
                                    bFound = false;
                                }

                                break;
                            }
                            case CHALLENGE_TYPE_KILL_SPECIFIC:
                            case CHALLENGE_TYPE_HIT:
                            case CHALLENGE_TYPE_USE_ITEM:
                            case CHALLENGE_TYPE_AQUIRE:
                            case CHALLENGE_TYPE_SKILL:
                            case CHALLENGE_TYPE_DAMAGE:
                            case CHALLENGE_TYPE_CRAFT: {
                                BGSListForm* pformlist = static_cast<BGSListForm*>(pItem->GetFormOne());
                                if (pFormOne != pformlist) {
                                    bFound = false;
                                }
                                break;
                            }
                            case CHALLENGE_TYPE_DISCOVER: {
                                BGSListForm* pformlist = static_cast<BGSListForm*>(pItem->GetFormOne());
                                if (pFormOne != pformlist) {
                                    bFound = false;
                                }
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                    else {
                        bFound = false;
                    }
                }
                if (!bFound) {
                    continue;
                }
                if (pItem->GetFormTwo()) {
                    if (pFormTwo) {
                        if (cType <= CHALLENGE_TYPE_DISCOVER || cType == CHALLENGE_TYPE_DAMAGE) {
                            BGSListForm* pformlist = static_cast<BGSListForm*>(pItem->GetFormTwo());
                            if (pformlist && !pformlist->ListOfForms.IsInList(pFormTwo)) {
                                bFound = false;
                            }
                        }
                    }
                    else {
                        bFound = false;
                    }
                }
                if (!bFound) {
                    continue;
                }
                if (pItem->GetSpecialOne()) {
                    if (!nSpecialDataOne) {
                        bFound = false;
                    }
                    int32_t iSpecialOne = nSpecialDataOne;
                    if (iSpecialOne != pItem->GetSpecialOne()) {
                        bFound = false;
                    }
                }
                else if (pItem->GetChallengeType() == CHALLENGE_TYPE_MISC_STAT) {
                    int32_t iSpecialOne = nSpecialDataOne;
                    if (iSpecialOne != pItem->GetSpecialOne()) {
                        bFound = false;
                    }
                }
                if (pItem->GetSpecialTwo()) {
                    if (!nSpecialDataTwo) {
                        bFound = false;
                    }
                    int32_t iSpecialTwo = nSpecialDataTwo;
                    if (iSpecialTwo != pItem->GetSpecialTwo()) {
                        bFound = false;
                    }
                }
                else if (pItem->GetChallengeType() == CHALLENGE_TYPE_HIT && nSpecialDataTwo && nSpecialDataTwo != 1) {
                    bFound = false;
                }
                if (pItem->GetSpecialThree()) {
                    if (!nSpecialDataThree) {
                        bFound = false;
                    }
                    int32_t iSpecialThree = nSpecialDataThree;
                    if (iSpecialThree != pItem->GetSpecialThree()) {
                        bFound = false;
                    }
                }
                if (!bFound) {
                    continue;
                }
                pItem->IncrementProgress(iCount);
                int32_t iProgress = pItem->progress.iProgress;
                if (iProgress < pItem->data.iThreshold) {
                    uint32_t uiInterval;
                    uint32_t uiProgress = pItem->progress.iProgress;
                    if (pItem->data.iInterval) {
                        uiInterval = pItem->data.iInterval;
                    }
                    else {
                        uiInterval = 100;
                    }
                    uint32_t uiStep = uiInterval;
                    for (int32_t i = uiProgress; i >= 0; i -= uiStep) {
                        if (!i &&
                            pItem->GetFullName() &&
                            pItem->GetDescription(nullptr, DESC_ID) &&
                            bShowChallengeUpdates.Bool()) {
                            char cDest[512];
                            const char* pDescription = pItem->GetDescription(nullptr, DESC_ID);
                            int32_t iThreshold = pItem->data.iThreshold;
                            uint32_t uiCurrent = pItem->progress.iProgress;
                            BSsprintf(
                                cDest,
                                512,
                                "%s   %d\\%d\n%s",
                                pItem->GetFullName(),
                                uiCurrent,
                                iThreshold,
                                pDescription
                            );
                            Interface::ShowMessage(
                                cDest,
                                0,
                                TESTexture::GetTextureName(pItem, nullptr),
                                nullptr,
                                2.0f,
                                false
                            );
                        }
                    }
                }
                else {
                    if (pItem->pFormScript) {
                        pItem->pFormScript->RunScriptEffectStart(pPlayer, nullptr);
                    }
                    if (pItem->GetChallengeType() != CHALLENGE_TYPE_MISC_STAT || pItem->GetSpecialOne() != 27) {
                        MiscStatManager::Increment(MiscStatManager::MS_CHALLENGES_COMPLETED);
                    }
                    if (pItem->GetFullName() &&
                        pItem->GetDescription(nullptr, DESC_ID) &&
                        bShowChallengeUpdates.Bool()) {
                        char msg[512];
                        const char* pDescription = pItem->GetDescription(nullptr, DESC_ID);
                        int32_t iThreshold = pItem->data.iThreshold;
                        BSsprintf(
                            msg,
                            512,
                            "%s   %d\\%d\n%s",
                            pItem->GetFullName(),
                            iThreshold,
                            iThreshold,
                            pDescription
                        );
                        Interface::ShowMessage(
                            msg,
                            0,
                            TESTexture::GetTextureName(pItem, nullptr),
                            nullptr,
                            2.0f,
                            false
                        );
                        Interface::PlayMenuSound(21);
                    }

                    if (!pItem->GetRecurring() || pItem->GetRemovedRecurring()) {
                        pItem->SetCompleted(true);
                        bCompletedChallenges = true;
                    }
                    else {
                        uint32_t uiProgress = pItem->progress.iProgress;
                        int32_t iSpillOver = uiProgress - pItem->data.iThreshold;
                        pItem->ClearProgress();
                        pItem->SetCompletedRecurring(true);
                        pItem->IncrementProgress(iSpillOver);
                    }
                }
            }
        }
    }
    if (bCompletedChallenges) {
        TESChallengeManager::LoadChallenges();
    }
    return true;
}

// The index is compared signed, against the global enum's count, not the class's unsigned copy.
void TESChallengeManager::UnloadChallenges() {
    for (int32_t i = 0; i < CHALLENGE_TYPE_COUNT; ++i) {
        pInactiveChallengeList[i].RemoveAll();
        pChallengeList[i].RemoveAll();
    }
}

void TESChallengeManager::LoadChallenges() {
    TESChallengeManager::UnloadChallenges();
    BSSimpleList<TESChallenge*>* pChallenges = pDataHandler->GetChallengeList();
    while (pChallenges) {
        TESChallenge* pItem = pChallenges->m_item;
        if (!pItem || pItem->GetCompleted() || (pItem->GetStartsLocked() && !pItem->GetUnlocked())) {
            if (pItem) {
                CHALLENGE_TYPE eType = pItem->GetChallengeType();
                TESChallengeManager::pChallengeList[eType].AddHead(pItem);
            }
        }
        else {
            CHALLENGE_TYPE eType = pItem->GetChallengeType();
            TESChallengeManager::pInactiveChallengeList[eType].AddHead(pItem);
        }
        pChallenges = pChallenges->m_pkNext;
    }
}

bool TESChallengeManager::UnlockChallenge(TESChallenge* pChallenge) {
    if (pChallenge) {
        CHALLENGE_TYPE cType = pChallenge->GetChallengeType();
        pChallenge->SetUnlocked(true);
        if (!TESChallengeManager::pInactiveChallengeList[cType].IsInList(pChallenge)) {
            TESChallengeManager::pInactiveChallengeList[cType].AddHead(pChallenge);
        }
        if (TESChallengeManager::pChallengeList[cType].IsInList(pChallenge)) {
            TESChallengeManager::pChallengeList[cType].Remove(pChallenge);
        }
    }
    return true;
}

bool TESChallengeManager::IncrementScriptedChallenge(TESChallenge* apChallenge, int32_t aiCount) {
    bool bLoadChallenges = false;
    TESChallenge* pChallenge = nullptr;
    for (BSSimpleList<TESChallenge*>* pNode = &pInactiveChallengeList[CHALLENGE_TYPE_SCRIPTED]; pNode;
         pNode = pNode->m_pkNext) {
        pChallenge = pNode->m_item;
        if (!pChallenge) {
            continue;
        }
        if (pChallenge->GetCompleted() && !pChallenge->GetRecurring()) {
            continue;
        }
        if (pChallenge->GetStartsLocked() && !pChallenge->GetUnlocked()) {
            continue;
        }
        if (apChallenge->iFormID != pChallenge->iFormID) {
            continue;
        }
        pChallenge->IncrementProgress(aiCount);
        if (static_cast<int32_t>(pChallenge->progress.iProgress) < pChallenge->data.iThreshold) {
            // Not done yet: a progress message when the progress is a multiple of the interval (100 when it is 0),
            // found by counting down from the progress.
            int32_t iInterval = pChallenge->data.iInterval ? pChallenge->data.iInterval : 100;
            for (int32_t i = static_cast<int32_t>(pChallenge->progress.iProgress); i >= 0; i -= iInterval) {
                if (i != 0 ||
                    !static_cast<TESFullName*>(pChallenge)->GetFullName() ||
                    !static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID) ||
                    !bShowChallengeUpdates.Bool()) {
                    continue;
                }
                char acMessage[512];
                const char* pDescription = static_cast<TESDescription*>(pChallenge)
                                               ->GetDescription(nullptr, CHUNK_ID::DESC_ID);
                int32_t iThreshold = pChallenge->data.iThreshold;
                uint32_t iProgress = pChallenge->progress.iProgress;
                BSsprintf(
                    acMessage,
                    sizeof(acMessage),
                    "%s   %d\\%d\n%s",
                    static_cast<TESFullName*>(pChallenge)->GetFullName(),
                    iProgress,
                    iThreshold,
                    pDescription
                );
                Interface::ShowMessage(
                    acMessage,
                    0,
                    TESTexture::GetTextureName(pChallenge, nullptr),
                    nullptr,
                    2.0f,
                    false
                );
            }
            continue;
        }
        if (pChallenge->pFormScript) {
            pChallenge->pFormScript->RunScriptEffectStart(pPlayer, nullptr);
        }
        // A misc-stat challenge on the completed-challenges stat does not count itself.
        if (pChallenge->GetChallengeType() != CHALLENGE_TYPE_MISC_STAT ||
            pChallenge->GetSpecialOne() != MiscStatManager::MS_CHALLENGES_COMPLETED) {
            MiscStatManager::Increment(MiscStatManager::MS_CHALLENGES_COMPLETED);
        }
        if (static_cast<TESFullName*>(pChallenge)->GetFullName() &&
            static_cast<TESDescription*>(pChallenge)->GetDescription(nullptr, CHUNK_ID::DESC_ID) &&
            bShowChallengeUpdates.Bool()) {
            char acMessage[512];
            const char* pDescription = static_cast<TESDescription*>(pChallenge)
                                           ->GetDescription(nullptr, CHUNK_ID::DESC_ID);
            // Passes the threshold for both numbers.
            int32_t iThreshold = pChallenge->data.iThreshold;
            BSsprintf(
                acMessage,
                sizeof(acMessage),
                "%s   %d\\%d\n%s",
                static_cast<TESFullName*>(pChallenge)->GetFullName(),
                iThreshold,
                iThreshold,
                pDescription
            );
            Interface::ShowMessage(acMessage, 0, TESTexture::GetTextureName(pChallenge, nullptr), nullptr, 2.0f, false);
            Interface::PlayMenuSound(21);
        }
        if (!pChallenge->GetRecurring() || pChallenge->GetRemovedRecurring()) {
            pChallenge->SetCompleted(true);
            bLoadChallenges = true;
        }
        else {
            // A recurring challenge starts again with what went past the threshold.
            int32_t iOverflow = static_cast<int32_t>(pChallenge->progress.iProgress) - pChallenge->data.iThreshold;
            pChallenge->ClearProgress();
            pChallenge->SetCompletedRecurring(true);
            pChallenge->IncrementProgress(iOverflow);
        }
    }
    if (bLoadChallenges) {
        TESChallengeManager::LoadChallenges();
    }
    return true;
}
