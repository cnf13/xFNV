#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSoundFile.h"

// A music type form (MUSC).
class BGSMusicType : public TESForm, public TESSoundFile {
public:
	// One music file's path, as BuildFileList allocates one per file found (ourMalloc 0x104).
	struct MusicFileEntry {
		MusicFileEntry(char* apFilename);

		char cFilename[260];
	};

	BGSMusicType();
	~BGSMusicType() override;

	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	bool GetLooping();
	float GetAttenuation();
	void BuildFileList();
	char* GetRandomEntry();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	float fAttenuation;
	// The music file names (ourMalloc'd strings ClearData frees).
	BSSimpleList<char*> pMusicFileList;
	// The index GetRandomEntry picked last.
	uint32_t iLastSelection;
};
