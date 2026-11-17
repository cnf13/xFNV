#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

class BSFile;

// A save file being read or written: its name, the BSFile and the simulated size and position of the stream.
class BGSSaveLoadFile {
public:
	BGSSaveLoadFile(const char* apName, bool abWrite, uint32_t aiBufferSize);
	~BGSSaveLoadFile();

	uint32_t Read(void* apBuffer, uint32_t aiBytes);
	// Writes aiBytes from apBuffer: only advances the simulated size and position while the
	// save manager simulates, else writes to pFile; the bytes written, aiBytes while simulating.
	uint32_t Write(const void* apBuffer, uint32_t aiBytes);
	// Moves the file position aiPosition bytes from the current one.
	void SeekCurrent(int32_t aiPosition);
	// Moves the file position to aiPosition.
	void SeekSet(int32_t aiPosition);
	// pSaveFileName, through the exe's shared identity function (thiscall, ret 0,
	// 0x6815C0).
	char* GetSaveFileName();
	uint32_t GetFilePosition();
	// Copies apFileName into pSaveFileName.
	void SetSaveFileName(const char* apFileName);
	bool Open();

	static int32_t SaveGlobalDataRange(BGSSaveLoadFile* apFile, uint32_t aiStart, uint32_t aiEnd);
	// Inferred type: a1: BGSSaveLoadFile* from 0x847956 (parameter 0 of BGSSaveLoadGame::SaveGame)
	static int32_t SaveGlobalDataRange0_C(BGSSaveLoadFile* a1);
	// Saves the global data of types 0x3E8 to 0x3E9.
	// Inferred type: apFile: BGSSaveLoadFile* from 0x847bf0 (parameter 0 of BGSSaveLoadGame::SaveGame)
	static int32_t SaveGlobalDataRange3E8_3E9(BGSSaveLoadFile* apFile);
	static BGSSaveLoadFile* CreateFileA(const char* apFileName, bool abWrite, uint32_t aiBufferSize);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	char pSaveFileName[260];
	BSFile* pFile;
	uint32_t iSimulatedFileSize;
	uint32_t iSimulatedFilePosition;
};
