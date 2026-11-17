#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Reconstruction.hpp"

class BSSystemFile;
class LoadFormHeader;
struct SaveFormHeader;

// The PDB's SaveStats (Fallout Shared\TESSaveLoadGame.cpp; TESSaveLoadGame::m_pSaveLoadStats); src keeps IDA's class
// and field names. Debug bookkeeping of a save: per form type the headers of the changed forms, and a list of extra
// sized entries.
class SaveDataDebugInfo {
public:
	// The PDB's SaveStats::ExtraStat: one extra entry of the report, a size and its description (the destructor
	// frees both).
	struct ExtraStat {
		int32_t iSize;
		char* pDescription;
	};

	// The PDB's SaveStats::Stats: the count, total, smallest and largest size of a set of entries.
	struct Stats {
		// The PDB's Stats(): empty stats, the smallest size 0xFFFF (thiscall, ret 0, 0x8562C0).
		Stats();

		int32_t iNum;
		int32_t iTotalSize;
		uint16_t iMinSize;
		uint16_t iMaxSize;
	};

	SaveDataDebugInfo();
	~SaveDataDebugInfo();

	// The PDB's SaveStats::PrintStats(apFileName): writes the report to apFileName plus ".txt" (thiscall, ret 4,
	// 0x855BA0).
	void PrintStats(const char* apFileName);

	// The PDB's SaveStats::WriteFile(aFile, apBuffer): writes the string apBuffer to aFile; true when the write
	// succeeds (thiscall, ret 8, this unused, 0x856300).
	bool WriteFile(BSSystemFile* aFile, const char* apBuffer);
	// The PDB's SaveStats::AddExtraStat(unsigned int aiSize, char* apDescription): adds an extra entry of aiSize with a
	// copy of apDescription to the report (thiscall, ret 8, 0x8558A0).
	void AddExtraStat(uint32_t aiSize, const char* apDescription);
	// The PDB's SaveStats::AddStats(SaveFormHeader*, unsigned short): adds the header with the data size aiSize
	// (thiscall, ret 8, 0x855970).
	void AddStats(SaveFormHeader* apHeader, uint16_t aiSize);
	// The PDB's SaveStats::AddStats(LoadFormHeader*): adds a copy of the header to its form type's list, sorted by
	// LoadFormHeaderCompare (thiscall, ret 4, 0x855A20).
	void AddStats(LoadFormHeader* apHeader);

	static void operator delete(void* apMemory);
	// The PDB's SaveStats::LoadFormHeaderCompare: the headers' sort order for the per-type lists, the larger data
	// size first (cdecl, 0x855B60).
	static int32_t LoadFormHeaderCompare(LoadFormHeader* apCompare1, LoadFormHeader* apCompare2);

	// IDA name; the PDB's pStatsMap. The headers of the changed forms by form type (the exe's
	// NiTPointerMap<unsigned char,BSSimpleList<LoadFormHeader *> *>, built with 0x25 buckets).
	NiTPointerMap<uint8_t, BSSimpleList<LoadFormHeader*>*>* headerMap;
	// IDA name; the PDB's pExtraStats. The extra entries.
	BSSimpleList<ExtraStat*>* unkList;
};
