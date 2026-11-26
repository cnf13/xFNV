#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"

#include <cstdarg>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseSaveGameHistory("bUseSaveGameHistory:SaveGame", false);

BGSSaveLoadHistory::BGSSaveLoadHistory() : Notes() {
    Notes.SetReservedSize(0x40u);
}

BGSSaveLoadHistory::~BGSSaveLoadHistory() {
    Clear();
}

void BGSSaveLoadHistory::AddNoteVarArgs(const char* apNote, va_list aVarList) {
    if (bUseSaveGameHistory.Bool()) {
        char cBuffer[1024];
        int32_t iSize = BSvsprintf(cBuffer, apNote, aVarList);
        if (iSize > 0) {
            char* pString = static_cast<char*>(ourMalloc(static_cast<uint32_t>(iSize + 1)));
            BSstrcpy(pString, iSize + 1, cBuffer);
            Notes.Add(pString);
        }
    }
}

void BGSSaveLoadHistory::AddNote(const char* apNote, ...) {
    va_list varArgs;
    va_start(varArgs, apNote);
    AddNoteVarArgs(apNote, varArgs);
    va_end(varArgs);
}

// Each note of apHistory goes in as the format of a note of its own, with no arguments.
void BGSSaveLoadHistory::AddHistory(BGSSaveLoadHistory* apHistory) {
    uint32_t uiCount = apHistory->GetCount();
    for (uint32_t i = 0; i < uiCount; ++i) {
        AddNote<>(apHistory->GetNote(i));
    }
}

uint32_t BGSSaveLoadHistory::GetCount() {
    return Notes.iSize;
}

const char* BGSSaveLoadHistory::GetNote(uint32_t aiIndex) {
    return Notes[aiIndex];
}

void BGSSaveLoadHistory::Save(BGSSaveLoadFile* apFile) {
    BGSSaveGameBuffer saveBuffer;
    uint32_t uiCount = Notes.iSize;
    saveBuffer.SaveDataEndian(&uiCount, sizeof(uiCount), 0);
    for (uint32_t i = 0; i < uiCount; ++i) {
        saveBuffer.SaveString(Notes[i], 0);
    }
    saveBuffer.Save(apFile);
}

void BGSSaveLoadHistory::Load(BGSSaveLoadFile* apFile) {
    BGSLoadGameBuffer loadBuffer;
    loadBuffer.Load(apFile);
    uint32_t uiCount = 0;
    loadBuffer.LoadData(&uiCount, sizeof(uiCount));
    for (uint32_t i = 0; i < uiCount; ++i) {
        char* pString = loadBuffer.LoadString(nullptr);
        Notes.Add(pString);
    }
}

void BGSSaveLoadHistory::Rewind(uint32_t aiCount) {
    uint32_t uiCount = GetCount();
    if (aiCount < uiCount) {
        for (uint32_t uiIndex = aiCount; uiIndex < uiCount; ++uiIndex) {
            ourFree(Notes[uiIndex]);
        }
        Notes.RemoveFast(aiCount, uiCount - aiCount);
    }
}

void BGSSaveLoadHistory::Clear() {
    for (uint32_t i = 0; i < Notes.iSize; ++i) {
        ourFree(Notes[i]);
    }
    Notes.SetSize(0, false);
}
