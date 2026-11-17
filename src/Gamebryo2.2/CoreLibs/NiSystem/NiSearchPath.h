#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

// Iterates the directories a file is looked up in: the file's own path first, then the reference path.
class NiSearchPath : public NiMemObject {
public:
	// Callback the engine uses to create search paths.
	using SEARCHPATHCREATEFUNC = NiSearchPath* (*)();
	// The default search path (the 260 bytes up to spSearchPathCreateCallback).
	using DefaultPath = char[260];

	NiSearchPath();
	virtual ~NiSearchPath();

	virtual void Reset();
	// copies the next directory to look the file up in to pcPath; false when there is none left
	virtual bool GetNextSearchPath(char* pcPath, uint32_t uiStringLen);

	// Copies and standardizes the file path; an empty or null path clears it.
	void SetFilePath(const char* pcFilePath);

	// Frees through MemoryManager::Deallocate, as the game's deleting destructor does.
	static void operator delete(void* apMemory);
	static void SetSearchPathCreateCallback(SEARCHPATHCREATEFUNC pFunc);

	uint32_t m_uiNextPath;
	char m_acFilePath[260];
	char m_acReferencePath[260];

	static inline SEARCHPATHCREATEFUNC spSearchPathCreateCallback{};
	static inline DefaultPath ms_acDefPath{};
};
