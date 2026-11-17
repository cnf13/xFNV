#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

class BSSystemFile;
class LoadFormHeader;
struct SaveFormHeader;

// Debug bookkeeping of a save: per form type the headers of the changed forms, and a list of extra sized entries.
class SaveDataDebugInfo {
public:
	// One extra entry of the report, a size and its description.
	struct ExtraStat {
		int32_t iSize;
		char* pDescription;
	};

	// The count, total, smallest and largest size of a set of entries.
	struct Stats {
		// Constructs empty stats, with the smallest size set to 0xFFFF.
		Stats();

		int32_t iNum;
		int32_t iTotalSize;
		uint16_t iMinSize;
		uint16_t iMaxSize;
	};

	SaveDataDebugInfo();
	~SaveDataDebugInfo();

	// Writes the report to apFileName plus ".txt".
	void PrintStats(const char* apFileName);

	// Writes the string apBuffer to aFile; true when the write succeeds. Does not use this.
	bool WriteFile(BSSystemFile* aFile, const char* apBuffer);
	// Adds an extra entry of aiSize with a copy of apDescription to the report.
	void AddExtraStat(uint32_t aiSize, const char* apDescription);
	// Adds the header with the data size aiSize.
	void AddStats(SaveFormHeader* apHeader, uint16_t aiSize);
	// Adds a copy of the header to its form type's list, sorted by LoadFormHeaderCompare.
	void AddStats(LoadFormHeader* apHeader);

	static void operator delete(void* apMemory);
	// The headers' sort order for the per-type lists, the larger data size first.
	static int32_t LoadFormHeaderCompare(LoadFormHeader* apCompare1, LoadFormHeader* apCompare2);

	// The headers of the changed forms by form type, built with 0x25 buckets.
	NiTPointerMap<uint8_t, BSSimpleList<LoadFormHeader*>*>* headerMap;
	BSSimpleList<ExtraStat*>* unkList;
};
