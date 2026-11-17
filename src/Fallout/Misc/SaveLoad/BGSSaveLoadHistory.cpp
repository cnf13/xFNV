#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
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
        char Buffer[1024];
        int32_t isize = BSvsprintf(Buffer, apNote, aVarList);
        if (isize > 0) {
            char* pstring = static_cast<char*>(ourMalloc(static_cast<uint32_t>(isize + 1)));
            BSstrcpy(pstring, isize + 1, Buffer);
            Notes.Add(pstring);
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
    return Notes.operator[](aiIndex);
}

void BGSSaveLoadHistory::Save(BGSSaveLoadFile* apFile) {
    uint32_t icount;
    BGSSaveGameBuffer saveBuffer;
    BGSSaveGameBuffer* apSaveGameBuffer = &saveBuffer;
    icount = Notes.iSize;
    apSaveGameBuffer->SaveDataEndian(&icount, 4u, 0);
    for (uint32_t i = 0; i < icount; ++i) {
        char* v2 = Notes.operator[](i);
        apSaveGameBuffer->SaveString(v2, 0);
    }
    saveBuffer.Save(apFile);
}

void BGSSaveLoadHistory::Load(BGSSaveLoadFile* apFile) {
    uint32_t icount;
    BGSLoadGameBuffer loadBuffer;
    loadBuffer.Load(apFile);
    BGSLoadGameBuffer* apLoadGameBuffer = &loadBuffer;
    icount = 0;
    loadBuffer.LoadData(&icount, 4u);
    for (uint32_t i = 0; i < icount; ++i) {
        char* pstring = apLoadGameBuffer->LoadString(0);
        Notes.Add(pstring);
    }
}

void BGSSaveLoadHistory::Rewind(uint32_t aiCount) {
    uint32_t Count = GetCount();
    if (aiCount < Count) {
        for (uint32_t auiIndex = aiCount; auiIndex < Count; ++auiIndex) {
            ourFree(Notes.operator[](auiIndex));
        }
        Notes.RemoveFast(aiCount, Count - aiCount);
    }
}

void BGSSaveLoadHistory::Clear() {
    uint32_t i_2 = 0;
    for (uint32_t i = Notes.iSize; i_2 < i; i = Notes.iSize) {
        ourFree(Notes.operator[](i_2++));
    }
    Notes.SetSize(0, 0);
}
