#pragma once

#include "Fallout/Debug/DebugTextPages/IDebugTextPage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "SDK/MSVC/ehvec.h"

class TESObjectREFR;

// The debug text pages: the pages being tracked, by id, and the table of registered page names, with the page and
// sub-page shown on screen. A singleton.
class DebugTextManager {
public:
    // A registered page: its name and the function that creates it.
    struct DTPEntry {
        DTPEntry();

        char pName[64];
        // Creator of the page for a reference. On PC it is only cleared and never set.
        IDebugTextPage* (*pfCreateFunction)(TESObjectREFR*);
    };

    DebugTextManager();

    // Passes the registered page names to apfOutput, five to a line.
    void OutputDebugPageNames(void (*)(const char*, ...));
    // Shows the page named apScreen at sub-page aiSubPage; false when no page has that name.
    bool SetScreenDebugPage(const char* apScreen, int32_t aiSubPage);
    // The index of the registered page named apPage, -1 when none.
    int32_t FindPageIndex(const char* apPage);
    // The name of the page shown on screen, RegisteredPages[iScreenPage].pName.
    const char* GetScreenDebugPage();
    // Starts tracking the page named apScreen for apReference; the PC build only returns 0.
    int32_t AddTrackedDebugPage(const char* apScreen, TESObjectREFR* apReference);

    // Creates s_pInstance when there is none.
    static void CreateSingleton();
    // s_pInstance, created first when missing.
    static DebugTextManager* GetSingleton();

    int32_t iCurrentGlobalId;
    NiTMap<uint32_t, IDebugTextPage*> TrackedDebugPages;
    int32_t iNumPages;
    int32_t iScreenPage;
    int32_t iScreenSubPage;
    DTPEntry RegisteredPages[64];

    static inline DebugTextManager* s_pInstance;
};
