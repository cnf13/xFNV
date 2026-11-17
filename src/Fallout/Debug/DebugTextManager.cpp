#include "Fallout/Debug/DebugTextManager.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/Debug/DebugTextPages/IDebugTextPage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

// pName is left unset, as the exe leaves it.
DebugTextManager::DTPEntry::DTPEntry() : pfCreateFunction(nullptr) {}

DebugTextManager::DebugTextManager()
	: iCurrentGlobalId(1), TrackedDebugPages(37u), iNumPages(0), iScreenPage(0), iScreenSubPage(0) {}

void DebugTextManager::OutputDebugPageNames(void (*apfOutput)(const char*, ...)) {
	char Dst[516];
	std::memset(Dst, 0, 512);
	for (int32_t i = 0; i < iNumPages; i += 5) {
		Dst[0] = 0;
		for (int32_t x = 0; x < 5 && i + x < iNumPages; ++x) {
			BSstrcat(Dst, 512, RegisteredPages[x + i].pName);
			if (i + x + 1 < iNumPages) {
				BSstrcat(Dst, 512, ", ");
			}
		}
		apfOutput(Dst);
	}
}

bool DebugTextManager::SetScreenDebugPage(const char* apScreen, int32_t aiSubPage) {
	int32_t PageIndex = FindPageIndex(apScreen);
	if (PageIndex != -1) {
		iScreenPage = PageIndex;
		iScreenSubPage = aiSubPage;
	}
	return PageIndex != -1;
}

int32_t DebugTextManager::FindPageIndex(const char* apPage) {
	int32_t v5 = -1;
	for (int32_t i = 0; i < iNumPages && v5 == -1; ++i) {
		if (!BSstrnicmp(apPage, RegisteredPages[i].pName, 64u)) {
			v5 = i;
		}
	}
	return v5;
}

// The PC build's body only returns 0: no page is tracked.
int32_t DebugTextManager::AddTrackedDebugPage([[maybe_unused]] const char* apScreen,
	[[maybe_unused]] TESObjectREFR* apReference) {
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
void DebugTextManager::CreateSingleton() {
	if (!DebugTextManager::s_pInstance) {
		DebugTextManager* v0;
		DebugTextManager* apMemory = static_cast<DebugTextManager*>(ourMalloc(sizeof(DebugTextManager)));
		if (apMemory) {
			v0 = ::new (apMemory) DebugTextManager;
		} else {
			v0 = 0;
		}
		DebugTextManager::s_pInstance = v0;
	}
}

DebugTextManager* DebugTextManager::GetSingleton() {
	if (!DebugTextManager::s_pInstance) {
		DebugTextManager::CreateSingleton();
	}
	return DebugTextManager::s_pInstance;
}
