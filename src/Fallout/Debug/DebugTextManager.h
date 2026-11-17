#pragma once

#include "Fallout/Debug/DebugTextPages/IDebugTextPage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class TESObjectREFR;

// The debug text pages: the pages being tracked, by id, and the table of registered page names, with the page and
// sub-page shown on screen. A singleton (ourMalloc(0x1120) in CreateSingleton 0x802A90).
class DebugTextManager {
public:
	// A registered page: its name and the function that creates it.
	struct DTPEntry {
		DTPEntry();

		char pName[64];
		// The PDB's creator of the page for a reference. The PC exe only clears it (no page is registered there).
		IDebugTextPage* (*pfCreateFunction)(TESObjectREFR*);
	};

	DebugTextManager();

	// Passes the registered page names to apfOutput, five to a line.
	void OutputDebugPageNames(void (*)(const char*, ...));
	// IDA's SetDebugTextString; the PDB's SetScreenDebugPage. Shows the page named apScreen at
	// sub-page aiSubPage; false when no page has that name (thiscall, ret 8, 0x8029C0).
	bool SetScreenDebugPage(const char* apScreen, int32_t aiSubPage);
	// The index of the registered page named apPage, -1 when none.
	int32_t FindPageIndex(const char* apPage);
	// The name of the page shown on screen, RegisteredPages[iScreenPage].pName.
	const char* GetScreenDebugPage();
	// Starts tracking the page named apScreen for apReference; the PC build only returns 0.
	int32_t AddTrackedDebugPage(const char* apScreen, TESObjectREFR* apReference);

	// IDA name; not in the PDB (the PDB's Initialize). Creates s_pInstance when there is none
	// (cdecl, ret 0, 0x802A90).
	static void CreateSingleton();
	// IDA name; not in the PDB (the PDB's Instance). s_pInstance, created first when missing
	// (cdecl, ret 0, 0x5BA8D0).
	static DebugTextManager* GetSingleton();

	int32_t iCurrentGlobalId;
	NiTMap<uint32_t, IDebugTextPage*> TrackedDebugPages;
	int32_t iNumPages;
	int32_t iScreenPage;
	int32_t iScreenSubPage;
	DTPEntry RegisteredPages[64];

	// The PDB's s_pInstance.
	static inline DebugTextManager* s_pInstance;
};
