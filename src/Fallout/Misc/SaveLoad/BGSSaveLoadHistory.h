#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"

#include <cstdarg>

class BGSSaveLoadFile;

// The notes a save game's history records (the text lines shown with a save).
class BGSSaveLoadHistory {
public:
	BGSSaveLoadHistory();
	~BGSSaveLoadHistory();

	// Adds a formatted note.
	void AddNoteVarArgs(const char* apNote, va_list aVarList);

	// Adds a formatted note through the varargs AddNote.
	template <class... A> void AddNote(const char* apNote, A... aArguments) {
		// The C-variadic overload by pointer: a plain call would pick this template again.
		void (BGSSaveLoadHistory::*pVariadic)(const char*, ...) = &BGSSaveLoadHistory::AddNote;
		(this->*pVariadic)(apNote, aArguments...);
	}

	void AddNote(const char* apNote, ...);
	// Appends every note of apHistory to this history.
	void AddHistory(BGSSaveLoadHistory* apHistory);
	// The number of notes.
	uint32_t GetCount();
	// The note at auiIndex.
	const char* GetNote(uint32_t aiIndex);
	// Writes the notes to apFile.
	void Save(BGSSaveLoadFile* apFile);
	// Reads the notes from apFile.
	void Load(BGSSaveLoadFile* apFile);
	void Rewind(uint32_t aiCount);
	void Clear();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSimpleArray<char*, 1024> Notes;
};

extern SettingT<INISettingCollection> bUseSaveGameHistory;
