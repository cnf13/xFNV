#include "Fallout/Misc/SaveLoad/BGSSaveLoadStructs.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormIDMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadInitialData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Fallout/Misc/SaveLoad/DebugSaveDumpData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/io.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"
#include "platform/System.h"

#include <filesystem>
#include <system_error>

SavedFormData::SavedFormData() : iChangeFlags() {}

uint32_t SavedFormData::GetTotalSize() {
    return iDebugOverhead + (iOverhead + iSize);
}

void SavedFormData::GetFormName(char* apName, uint32_t aiNameSize) {
    if (pForm) {
        const char* FullName;
        TESObjectREFR* pref;
        if (pForm->IsReference()) {
            pref = static_cast<TESObjectREFR*>(pForm);
        }
        else {
            pref = nullptr;
        }
        if (pref) {
            FullName = pref->GetFullName();
        }
        else {
            FullName = TESFullName::GetFullName(pForm);
        }
        BSstrcpy(apName, static_cast<int32_t>(aiNameSize), FullName);
        if (!BSstrlen(apName)) {
            const char* v3 = pForm->GetFormEditorID();
            BSstrcpy(apName, static_cast<int32_t>(aiNameSize), v3);
        }
        if (!BSstrlen(apName) && pForm->GetFormType() == INFO_ID) {
            // A topic info: the editor name of its owner quest.
            TESTopicInfo* pinfo = static_cast<TESTopicInfo*>(pForm);
            BSstrcpy(apName, static_cast<int32_t>(aiNameSize), pinfo->pOwnerQuest->GetEditorName());
        }
    }
    else {
        BSstrcpy(apName, static_cast<int32_t>(aiNameSize), "UNLOADED");
    }
}

uint32_t MiscSaveStat::GetTotalSize() {
    return (uint32_t)((int32_t)(iDebugOverhead + iSize));
}

int32_t MiscSaveStatCompare(MiscSaveStat* apCompare1, MiscSaveStat* apCompare2) {
    uint32_t totalSize = apCompare1->GetTotalSize();
    uint32_t v5 = apCompare2->GetTotalSize();
    if (totalSize <= v5) {
        return (int32_t)((uint8_t)(totalSize < v5));
    }
    else {
        return (-1);
    }
}

int32_t SavedFormDataCompare(SavedFormData* a1, SavedFormData* a2) {
    uint32_t totalSize = a1->GetTotalSize();
    uint32_t v5 = a2->GetTotalSize();
    if (totalSize <= v5) {
        return (int32_t)((uint8_t)(totalSize < v5));
    }
    else {
        return (-1);
    }
}

BGSSaveLoadStatsMap::BGSSaveLoadStatsMap()
  : NiTMap<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*>(37u), MiscSaveStats(), pFile(nullptr) {}

// The map's lists and the stats list own their items, freed here; the lists' nodes go with the members.
BGSSaveLoadStatsMap::~BGSSaveLoadStatsMap() {
    delete pFile;
    NiTMapItem<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*>* pPos = GetFirstPos();
    while (pPos) {
        ENUM_FORM_ID eformType = NO_FORM;
        BSSimpleList<SavedFormData*>* plist = nullptr;
        GetNext(pPos, eformType, plist);
        while (plist) {
            ourFree(plist->m_item);
            plist = plist->m_pkNext;
        }
    }
    for (BSSimpleList<MiscSaveStat*>* pIter = &MiscSaveStats; pIter; pIter = pIter->m_pkNext) {
        ourFree(pIter->m_item);
    }
}

// Writes the statistics: every saved form by type, the per-type and overall size totals, the misc stats and the save
// game's history.
void BGSSaveLoadStatsMap::OutputStats() {
    char acText[1000];
    BGSSaveFormBuffer saveFormBuffer;
    DebugSaveDumpData totalSizeTracker;
    totalSizeTracker.~DebugSaveDumpData();
    ::new (&totalSizeTracker) DebugSaveDumpData;
    BSsprintf(
        acText, 1000, "  %-8s %-5s %-8s %3s %-40s  %s\r\n\r\n", "Form ID", "Size", "Flags", "Ver", "Name", "Changes"
    );
    WriteFile(acText);
    NiTMapItem<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*>* pPos = GetFirstPos();
    while (pPos) {
        DebugSaveDumpData formTypeSizeTracker;
        formTypeSizeTracker.~DebugSaveDumpData();
        ::new (&formTypeSizeTracker) DebugSaveDumpData;
        ENUM_FORM_ID eFormType = NO_FORM;
        BSSimpleList<SavedFormData*>* pList = nullptr;
        GetNext(pPos, eFormType, pList);
        BSsprintf(acText, 1000, "%ss:\r\n\r\n", formEnumString[eFormType].pFormString);
        WriteFile(acText);
        while (pList) {
            SavedFormData* pFormData = pList->m_item;
            if (pFormData) {
                // A `= ""` initialiser: the empty string's byte, then memset of the rest.
                char acChanges[1000];
                acChanges[0] = ""[0];
                std::memset(&acChanges[1], 0, 999);
                char acName[1000];
                char acEntry[1000];
                char acColumn[1000];
                uint32_t uiBaseSize = 0;
                if (pFormData->pForm) {
                    BGSChangeFlags kNoFlags;
                    kNoFlags.~BGSChangeFlags();
                    ::new (&kNoFlags) BGSChangeFlags(0);
                    SaveForm(pFormData->pForm, kNoFlags, &saveFormBuffer);
                    uiBaseSize = saveFormBuffer.iBufferPosition;
                    if (uiBaseSize) {
                        BSstrcpy(acName, 1000, "Base");
                        if (pFormData->pForm->IsReference()) {
                            MobileObject* pMob = static_cast<MobileObject*>(pFormData->pForm);
                            if (pMob->IsMobileObject()) {
                                PROCESS_TYPE eLevel = PROCESS_TYPE_NONE;
                                if (pMob->pCurrentProcess) {
                                    eLevel = pMob->pCurrentProcess->eLevel;
                                }
                                switch (eLevel) {
                                    case PROCESS_TYPE_NONE: BSstrcpy(acName, 1000, "None"); break;
                                    case PROCESS_TYPE_HIGH: BSstrcpy(acName, 1000, "High"); break;
                                    case PROCESS_TYPE_MIDDLE_HIGH: BSstrcpy(acName, 1000, "Mid High"); break;
                                    case PROCESS_TYPE_MIDDLE_LOW: BSstrcpy(acName, 1000, "Mid Low"); break;
                                    case PROCESS_TYPE_LOW: BSstrcpy(acName, 1000, "Low"); break;
                                    default: break;
                                }
                            }
                        }
                        BSsprintf(acEntry, 1000, "%s(%i)", acName, uiBaseSize);
                        BSsprintf(acColumn, 1000, "%-25s", acEntry);
                        BSstrcat(acChanges, 1000, acColumn);
                    }
                }
                for (uint32_t i = 0; i < 32; ++i) {
                    int32_t iFlag = 1 << i;
                    if (pFormData->iChangeFlags.CheckFlags(iFlag)) {
                        BSstrcpy(acName, 1000, BGSChangeFlags::GetFlagName(iFlag, eFormType, true));
                        if (pFormData->pForm) {
                            BGSChangeFlags kFlag;
                            kFlag.~BGSChangeFlags();
                            ::new (&kFlag) BGSChangeFlags(iFlag);
                            SaveForm(pFormData->pForm, kFlag, &saveFormBuffer);
                            BSsprintf(acEntry, 1000, "%s(%i)", acName, saveFormBuffer.iBufferPosition - uiBaseSize);
                        }
                        else {
                            BSsprintf(acEntry, 1000, "%s", acName);
                        }
                        BSsprintf(acColumn, 1000, "%-25s", acEntry);
                        BSstrcat(acChanges, 1000, acColumn);
                    }
                }
                BSstrcpy(acName, 1000, "");
                pFormData->GetFormName(acName, 1000);
                uint32_t uiVersion = pFormData->cVersion;
                int32_t iFlags = pFormData->iChangeFlags.iFlags;
                uint32_t uiTotalSize = pFormData->GetTotalSize();
                BSsprintf(
                    acText, 1000, "  %08X %5i %08X %3i %-40s- %s\r\n", pFormData->iFormID, uiTotalSize, iFlags,
                    uiVersion, acName, acChanges
                );
                WriteFile(acText);
            }
            // The sizes are read outside the null test above, as the game does.
            formTypeSizeTracker.Add(pFormData->iSize, pFormData->iOverhead, pFormData->iDebugOverhead);
            totalSizeTracker.Add(pFormData->iSize, pFormData->iOverhead, pFormData->iDebugOverhead);
            pList = pList->m_pkNext;
        }
        double dAverage = formTypeSizeTracker.CalcAverageSize();
        uint32_t uiSize = formTypeSizeTracker.CalcSize();
        BSsprintf(
            acText, 1000,
            "\r\nTotal %s Count: %i  Total %s Size: %i - Data: %i Overhead: %i Debug: %i Min %s Size: %i  Max %s Size: "
            "%i  Average %s Size: %.2f\r\n\r\n",
            formEnumString[eFormType].pFormString, formTypeSizeTracker.total, formEnumString[eFormType].pFormString,
            uiSize, formTypeSizeTracker.data, formTypeSizeTracker.overhead, formTypeSizeTracker.debug,
            formEnumString[eFormType].pFormString, formTypeSizeTracker.minSize, formEnumString[eFormType].pFormString,
            formTypeSizeTracker.maxSize, formEnumString[eFormType].pFormString, dAverage
        );
        WriteFile(acText);
    }
    double dTotalAverage = totalSizeTracker.CalcAverageSize();
    uint32_t uiTotalSize = totalSizeTracker.CalcSize();
    BSsprintf(
        acText, 1000,
        "\r\nTotal Form Count: %i  Total Form Size: %i - Data: %i Overhead: %i Debug: %i  Min Form Size: %i  Max Form "
        "Size: %i  Average Form Size: %.2f\r\n\r\n",
        totalSizeTracker.total, uiTotalSize, totalSizeTracker.data, totalSizeTracker.overhead, totalSizeTracker.debug,
        totalSizeTracker.minSize, totalSizeTracker.maxSize, dTotalAverage
    );
    WriteFile(acText);
    DebugSaveDumpData miscSizeTracker;
    miscSizeTracker.~DebugSaveDumpData();
    ::new (&miscSizeTracker) DebugSaveDumpData;
    WriteFile("\r\nMisc Stats:\r\n\r\n");
    for (BSSimpleList<MiscSaveStat*>* pIter = &MiscSaveStats; pIter; pIter = pIter->m_pkNext) {
        MiscSaveStat* pMiscStat = pIter->m_item;
        if (pMiscStat) {
            BSsprintf(acText, 1000, "  %8i      %s\r\n", pMiscStat->GetTotalSize(), pMiscStat->pName);
            WriteFile(acText);
            miscSizeTracker.Add(pMiscStat->iSize, 0, pMiscStat->iDebugOverhead);
        }
    }
    double dMiscAverage = miscSizeTracker.CalcAverageSize();
    uint32_t uiMiscSize = miscSizeTracker.CalcSize();
    BSsprintf(
        acText, 1000,
        "\r\nTotal Misc Count: %i  Total Misc Size: %i - Data: %i Overhead: %i Debug: %i  Min Misc Size: %i  Max Misc "
        "Size: %i  Average Misc Size: %.2f\r\n\r\n",
        miscSizeTracker.total, uiMiscSize, miscSizeTracker.data, miscSizeTracker.overhead, miscSizeTracker.debug,
        miscSizeTracker.minSize, miscSizeTracker.maxSize, dMiscAverage
    );
    WriteFile(acText);
    BGSSaveLoadHistory* pHistory = pSaveLoadGame->pHistory;
    uint32_t uiCount = pHistory->GetCount();
    int32_t iIndentLevel = 0;
    if (uiCount) {
        WriteFile("\r\n\r\nSave Game History:\r\n\r\n");
        for (uint32_t i = 0; i < uiCount; ++i) {
            const char* pNote = pHistory->GetNote(i);
            int32_t iIndentChange = 0;
            if (!BSstrnicmp(pNote, "***", 3)) {
                iIndentChange = 1;
            }
            else if (!BSstrnicmp(pNote, "---", 3)) {
                iIndentChange = -1;
                --iIndentLevel;
            }
            for (int32_t j = 0; j < iIndentLevel; ++j) {
                WriteFile("  ");
            }
            WriteFile(iIndentChange ? pNote + 3 : pNote);
            WriteFile("\r\n");
            if (iIndentChange == 1) {
                ++iIndentLevel;
            }
        }
    }
}

void BGSSaveLoadStatsMap::SaveForm(TESForm* apForm, BGSChangeFlags aiChangeFlags, BGSSaveFormBuffer* apSaveFormBuffer) {
    apSaveFormBuffer->ClearPosition();
    uint8_t MinorVersion = pSaveLoadManager->GetMinorVersion();
    ENUM_FORM_ID FormType = apForm->GetFormType();
    uint32_t CastingType = apForm->GetFormID();
    apSaveFormBuffer->SetHeader(CastingType, aiChangeFlags, FormType, MinorVersion);
    apSaveFormBuffer->SetForm(apForm);
    BGSSaveLoadInitialData::SaveInitialData(apSaveFormBuffer, apForm, aiChangeFlags);
    apForm->SaveGame(apSaveFormBuffer);
}

void BGSSaveLoadStatsMap::WriteFile(const char* a1) {
    if (pFile) {
        uint32_t v2 = BSstrlen(a1);
        pFile->WriteHere(a1, v2);
    }
}

void BGSSaveLoadStatsMap::CloseFile() {
    if (pFile) {
        delete pFile;
    }
    pFile = nullptr;
}

void BGSSaveLoadStatsMap::BuildStatsMap() {
    BGSSaveFormBuffer apSaveFormBuffer;
    BGSSaveLoadChangesMap* pchangesMap = pSaveLoadGame->pChangesMap;
    NiTMapItem<uint32_t, BGSFormChanges*>* kIter = pchangesMap->GetFirstPos();
    while (kIter) {
        uint32_t iformID = 0;
        BGSFormChanges* pchanges = nullptr;
        pchangesMap->GetNext(kIter, iformID, pchanges);
        if (pchanges->UnloadedFormBuffer.pBuffer) {
            BGSChangeFlags v5;
            uint8_t Version = pchanges->UnloadedFormBuffer.GetVersion();
            uint32_t v9 = pchanges->UnloadedFormBuffer.GetOverhead();
            uint32_t Size = pchanges->UnloadedFormBuffer.GetSize();
            v5.iFlags = pchanges->iChangeFlags.iFlags;
            ENUM_FORM_ID FormType = pchanges->UnloadedFormBuffer.GetFormType();
            AddForm(iformID, nullptr, FormType, v5, Size, v9, 0, Version);
        }
        else {
            TESForm* pform = TESForm::GetFormByNumericID(iformID);
            if (pform) {
                BGSChangeFlags v6;
                SaveForm(pform, pchanges->iChangeFlags, &apSaveFormBuffer);
                uint8_t v15 = apSaveFormBuffer.Header.cVersion;
                uint32_t v11 = apSaveFormBuffer.iDebugOverhead;
                uint32_t Overhead = apSaveFormBuffer.GetOverhead();
                uint32_t DataSize = apSaveFormBuffer.GetDataSize();
                v6.iFlags = pchanges->iChangeFlags.iFlags;
                ENUM_FORM_ID v2 = pform->GetFormType();
                AddForm(iformID, pform, v2, v6, DataSize, Overhead, v11, v15);
            }
        }
    }
    for (uint32_t i = 0; i < 12; ++i) {
        apSaveFormBuffer.ClearPosition();
        if (BGSSaveLoadGlobalData::SaveGlobalData(i, &apSaveFormBuffer)) {
            uint32_t v16 = apSaveFormBuffer.iDebugOverhead;
            uint32_t v12 = apSaveFormBuffer.GetDataSize();
            const char* v3 = BGSSaveLoadGlobalData::GetGlobalDataName(i);
            AddMiscSaveStat(v3, v12, v16);
        }
    }
    for (uint32_t i_2 = 1000; i_2 < 1001; ++i_2) {
        apSaveFormBuffer.ClearPosition();
        if (BGSSaveLoadGlobalData::SaveGlobalData(i_2, &apSaveFormBuffer)) {
            uint32_t v17 = apSaveFormBuffer.iDebugOverhead;
            uint32_t v13 = apSaveFormBuffer.GetDataSize();
            const char* v4 = BGSSaveLoadGlobalData::GetGlobalDataName(i_2);
            AddMiscSaveStat(v4, v13, v17);
        }
    }
}

void BGSSaveLoadStatsMap::AddForm(
    uint32_t aiFormID,
    TESForm* apForm,
    ENUM_FORM_ID aeFormType,
    BGSChangeFlags aiChangeFlags,
    uint32_t aiSize,
    uint32_t aiOverhead,
    uint32_t aiDebugOverhead,
    uint8_t acVersion
) {
    SavedFormData* pformData;
    SavedFormData* apMemory = static_cast<SavedFormData*>(ourMalloc(sizeof(SavedFormData)));
    if (apMemory) {
        pformData = ::new (apMemory) SavedFormData;
    }
    else {
        pformData = nullptr;
    }
    pformData->iFormID = aiFormID;
    pformData->pForm = apForm;
    pformData->iChangeFlags = aiChangeFlags;
    pformData->iSize = aiSize;
    pformData->iOverhead = aiOverhead;
    pformData->iDebugOverhead = aiDebugOverhead;
    pformData->cVersion = acVersion;
    BSSimpleList<SavedFormData*>* plist = nullptr;
    if (!GetAt(aeFormType, plist)) {
        BSSimpleList<SavedFormData*>* plist_2;
        BSSimpleList<SavedFormData*>* v12 =
            static_cast<BSSimpleList<SavedFormData*>*>(ourMalloc(sizeof(BSSimpleList<SavedFormData*>)));
        if (v12) {
            plist_2 = ::new (v12) BSSimpleList<SavedFormData*>;
        }
        else {
            plist_2 = nullptr;
        }
        plist = plist_2;
        SetAt(aeFormType, plist_2);
    }
    plist->Insert(pformData, &SavedFormDataCompare);
}

void BGSSaveLoadStatsMap::AddMiscSaveStat(const char* apName, uint32_t aiSize, uint32_t aiDebugOverhead) {
    MiscSaveStat* pmiscStat = static_cast<MiscSaveStat*>(ourMalloc(sizeof(MiscSaveStat)));
    BSstrcpy(pmiscStat->pName, 260, apName);
    pmiscStat->iSize = aiSize;
    pmiscStat->iDebugOverhead = aiDebugOverhead;
    MiscSaveStats.Insert(pmiscStat, &MiscSaveStatCompare);
}

void BGSSaveLoadStatsMap::OutputSaveLoadStats(const char* apSaveFileName) {
    BSFile* v1;
    char FileName[260];
    BGSSaveLoadStatsMap statsMap;
    BSstrcpy(FileName, 260, apSaveFileName);
    BSstrcat(FileName, 260, ".txt");
    if (rt::_access(FileName, 0) != -1) {
        std::error_code kError;
        std::filesystem::remove(platform::NativePath(FileName), kError);
    }
    BSFile* apMemory = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
    if (apMemory) {
        v1 = ::new (apMemory) BSFile(FileName, NiFile::WRITE_ONLY, 0x4000u, false);
    }
    else {
        v1 = nullptr;
    }
    statsMap.pFile = v1;
    statsMap.BuildStatsMap();
    statsMap.OutputStats();
    statsMap.CloseFile();
    platform::Open(FileName);
}

uint32_t BGSSaveGameBuffer::GetDataSize() {
    return iBufferPosition - iDebugOverhead;
}

bool BGSNumericIDIndex::IsZero() {
    return (bool)((uint8_t)((!cData1) && ((!cData2) && (!cData3))));
}

uint32_t BGSNumericIDIndex::GetNumericID() {
    uint32_t iindex = cData3 + ((cData2 << 8) + (cData1 << 16));
    if ((iindex & 0x800000) != 0) {
        return (iindex & 0x7FFFFF) | 0xFF000000u;
    }
    else {
        return pSaveLoadGame->pFormIDMap->GetFormID(iindex);
    }
}

void BGSNumericIDIndex::SetNumericID(uint32_t aiNumericID) {
    uint32_t iindex = pSaveLoadGame->pFormIDMap->AddFormID(aiNumericID);
    if (static_cast<uint8_t>(iindex >> 24) == 255) {
        iindex |= 0x800000u;
    }
    cData1 = static_cast<uint8_t>(iindex >> 16);
    cData2 = static_cast<uint8_t>(iindex >> 8);
    cData3 = iindex;
}

ENUM_FORM_ID BGSSaveLoadFormInfo::GetFormType() {
    return static_cast<ENUM_FORM_ID>(cSaveGameFormTypes[cData & 63]);
}

void BGSSaveLoadFormInfo::SetFormType(ENUM_FORM_ID aeFormType) {
    uint8_t csaveGameFormType = cSaveGameFormTypeArray[aeFormType];
    if (csaveGameFormType >= iSaveGameFormTypeCount) {
        SaveGameWarning(
            "Unknown form type %i %s in BGSSaveLoadFormInfo::SetFormType()", aeFormType,
            TESForm::GetFormTypeString(aeFormType)
        );
    }
    cData = csaveGameFormType | (cData & 192);
}

void BGSSaveLoadFormInfo::SetSizeType(SIZE_TYPE aeSizeType) {
    cData = (uint8_t)((((int8_t)aeSizeType) << 6) | (cData & 63));
}

// Clears the table to 0xFF, then gives each saved form type its index; the count is reloaded each pass.
void BGSSaveLoadFormInfo::InitSaveGameFormTypes() {
    ourMemset(cSaveGameFormTypeArray, 0xFF, 0x79);
    iSaveGameFormTypeCount = 55;
    for (uint32_t i = 0; i < iSaveGameFormTypeCount; ++i) {
        cSaveGameFormTypeArray[cSaveGameFormTypes[i]] = static_cast<uint8_t>(i);
    }
}

const char* BGSSaveLoadGlobalData::GetGlobalDataName(uint32_t aiType) {
    if (aiType < GLOBAL_DATA_FINAL_START) {
        return pGlobalDataInitialNames[aiType];
    }
    else {
        return pGlobalDataFinalNames[aiType - GLOBAL_DATA_FINAL_START];
    }
}

DebugSaveDumpData::DebugSaveDumpData() : total(0), data(0), overhead(0), debug(0), minSize(0xFFFFFFFFu), maxSize(0) {}

void DebugSaveDumpData::Add(uint32_t aiData, uint32_t aiOverhead, uint32_t aiDebug) {
    ++total;
    data += aiData;
    overhead += aiOverhead;
    debug += aiDebug;
    uint32_t uiSize = aiDebug + (aiOverhead + aiData);
    if (uiSize < minSize) {
        minSize = uiSize;
    }
    if (uiSize > maxSize) {
        maxSize = uiSize;
    }
}

uint32_t DebugSaveDumpData::CalcSize() {
    return debug + (overhead + data);
}

double DebugSaveDumpData::CalcAverageSize() {
    return static_cast<float>(static_cast<double>(CalcSize()) / static_cast<double>(total));
}
