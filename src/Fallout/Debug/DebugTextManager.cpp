#include "Fallout/Debug/DebugTextManager.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/Debug/DebugTextPages/IDebugTextPage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

// pName is deliberately left unset.
DebugTextManager::DTPEntry::DTPEntry() : pfCreateFunction(nullptr) {}

DebugTextManager::DebugTextManager()
  : iCurrentGlobalId(1), TrackedDebugPages(37u), iNumPages(0), iScreenPage(0), iScreenSubPage(0) {}

void DebugTextManager::OutputDebugPageNames(void (*apfOutput)(const char*, ...)) {
    char cLine[516];
    std::memset(cLine, 0, 512);
    for (int32_t i = 0; i < iNumPages; i += 5) {
        cLine[0] = 0;
        for (int32_t x = 0; x < 5 && i + x < iNumPages; ++x) {
            BSstrcat(cLine, 512, RegisteredPages[x + i].pName);
            if (i + x + 1 < iNumPages) {
                BSstrcat(cLine, 512, ", ");
            }
        }
        apfOutput(cLine);
    }
}

bool DebugTextManager::SetScreenDebugPage(const char* apScreen, int32_t aiSubPage) {
    int32_t iPageIndex = FindPageIndex(apScreen);
    if (iPageIndex != -1) {
        iScreenPage = iPageIndex;
        iScreenSubPage = aiSubPage;
    }
    return iPageIndex != -1;
}

int32_t DebugTextManager::FindPageIndex(const char* apPage) {
    int32_t iIndex = -1;
    for (int32_t i = 0; i < iNumPages && iIndex == -1; ++i) {
        if (!BSstrnicmp(apPage, RegisteredPages[i].pName, 64u)) {
            iIndex = i;
        }
    }
    return iIndex;
}

// Only returns 0: no page is tracked.
int32_t DebugTextManager::AddTrackedDebugPage(
    [[maybe_unused]] const char* apScreen,
    [[maybe_unused]] TESObjectREFR* apReference
) {
    return 0;
}

void DebugTextManager::CreateSingleton() {
    if (!DebugTextManager::s_pInstance) {
        DebugTextManager* pManager;
        pManager = new DebugTextManager;
        DebugTextManager::s_pInstance = pManager;
    }
}

DebugTextManager* DebugTextManager::GetSingleton() {
    if (!DebugTextManager::s_pInstance) {
        DebugTextManager::CreateSingleton();
    }
    return DebugTextManager::s_pInstance;
}
