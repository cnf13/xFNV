#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

// Watches the free memory and frees caches when it runs low. Only the entry points the reconstructed code calls are
// declared.
class MemoryLevelManager {
public:
	// The renderer and release state BeginMemoryFree saves and EndMemoryFree restores.
	struct MEM_FREE_DATA {
		bool bOldInstantResourceRelease;
		bool bOldInstantVertexDeclRelease;
		bool bOldInstantTextureRelease;
		bool bLockedRenderer;
		bool bRunHeapClean;
		bool bLockedFaceGen;
		int32_t iTexturePurgeCount;
	};

	MemoryLevelManager();
	~MemoryLevelManager();

	// Runs the memory passes the main thread was signalled for.
	void CheckMemoryLevel(bool abInIdle);
	bool WasLastCheckedMemLevelCloseToLow();

	// Saves the release state into aFreeData and locks what the flags ask for (cdecl,
	// 0x878160). The exe takes a third flag the PDB does not have (IDA name; not in the PDB).
	static void BeginMemoryFree(MEM_FREE_DATA* aFreeData, bool abLockRenderer, bool abRunHeapClean, bool abLockFaceGen);
	// Restores the state BeginMemoryFree saved.
	static void EndMemoryFree(MemoryLevelManager::MEM_FREE_DATA& aFreeData);
	// The global manager (cdecl, 0x446EF0, the copy the PDB places in ModelLoader.cpp).
	static MemoryLevelManager* QInstance();
	// Frees what can be freed without unloading anything in use.
	static void RunNonDestructiveFree();
	// Frees released objects, models and textures; with abLockedFaceGen also the face-gen caches.
	static void FreeReleasedObjects(uint8_t abLockedFaceGen);

	uint32_t iLowMemoryLevelLoading;
	uint32_t iLowMemoryLevelIdle;
	uint32_t iLowMemoryLevelIdleCritical;
	uint32_t iLowMemoryBufferSize;
	uint32_t iLastCheckedMemLevel;
	BSCriticalSection cCheckMemoryLevelLock;

	// The manager.
	static MemoryLevelManager Instance;
};

// The game settings MemoryLevelManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bShowCheckMemoryOutput;
