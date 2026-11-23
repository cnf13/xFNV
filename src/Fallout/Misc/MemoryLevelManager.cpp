#include "Fallout/Misc/MemoryLevelManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryPool.h"
#include "BSMain/IOManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMenu/Menu.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bShowCheckMemoryOutput("bShowCheckMemoryOutput:General", false);
MemoryLevelManager MemoryLevelManager::Instance;

MemoryLevelManager::MemoryLevelManager()
  : iLowMemoryLevelLoading(20971520), iLowMemoryLevelIdle(15728640), iLowMemoryLevelIdleCritical(10485760),
    iLowMemoryBufferSize(2097152), iLastCheckedMemLevel(0xFFFFFFFFu) {}

MemoryLevelManager::~MemoryLevelManager() {}

// Runs every memory pass the main thread was signalled for, once nobody else holds the lock.
void MemoryLevelManager::CheckMemoryLevel(bool abInIdle) {
    if (cCheckMemoryLevelLock.TryLock()) {
        if (MemoryManager::Instance().GetMainThreadMemoryProblemPassSignal()) {
            bool bAllowSystemAllocs = true;
            for (uint32_t i = 0; i <= MemoryManager::Instance().GetMainThreadMemoryProblemPassSignal(); ++i) {
                MemoryManager::Instance().ProcessMemoryProblem(nullptr, i, &bAllowSystemAllocs);
            }
            MemoryManager::Instance().ClearMainThreadMemoryProblemPassSignal();
        }
        cCheckMemoryLevelLock.Unlock();
    }
}

bool MemoryLevelManager::WasLastCheckedMemLevelCloseToLow() {
    if (iLastCheckedMemLevel > iLowMemoryLevelIdleCritical) {
        return iLastCheckedMemLevel - iLowMemoryLevelIdleCritical <= 5242880;
    }
    return true;
}

void MemoryLevelManager::BeginMemoryFree(
    MEM_FREE_DATA* aFreeData,
    bool abLockRenderer,
    bool abRunHeapClean,
    bool abLockFaceGen
) {
    pIOManager->Pause();
    if (abLockFaceGen) {
        aFreeData->bLockedFaceGen = BSFaceGenManager::TryLockFaceGenAccess();
    }
    else {
        aFreeData->bLockedFaceGen = false;
    }
    if (abLockRenderer) {
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->LockRenderer();
    }
    aFreeData->bLockedRenderer = abLockRenderer;
    aFreeData->bRunHeapClean = abRunHeapClean;
    aFreeData->iTexturePurgeCount = BSTexturePalette::GetMaxTexturesToPurge();
    BSTexturePalette::SetMaxTexturesToPurge(std::numeric_limits<int32_t>::max());
    Interface::ForceTextureRelease();
}

void MemoryLevelManager::EndMemoryFree(MemoryLevelManager::MEM_FREE_DATA& aFreeData) {
    NiGlobalStringTable::FreeUnusedStrings();
    MemoryPool::FreeUnusedPagesForAllPools();
    if (aFreeData.bLockedFaceGen) {
        BSFaceGenManager::UnlockFaceGenAccess();
    }
    BSTexturePalette::SetMaxTexturesToPurge(aFreeData.iTexturePurgeCount);
    if (aFreeData.bLockedRenderer) {
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->UnlockRenderer();
    }
    pIOManager->Resume();
}

MemoryLevelManager* MemoryLevelManager::QInstance() {
    return &Instance;
}

void MemoryLevelManager::RunNonDestructiveFree() {
    MemoryLevelManager::MEM_FREE_DATA kFreeData;
    MemoryLevelManager::BeginMemoryFree(&kFreeData, true, true, true);
    MemoryLevelManager::FreeReleasedObjects(kFreeData.bLockedFaceGen);
    if (pPlayer->GetAllowEGMCacheClear() && BSFaceGenManager::GetModelCache()) {
        BSFaceGenModelMap* pModelCache = BSFaceGenManager::GetModelCache();
        if (pModelCache->bHasFaceGenData && !Menu::GetMenuByClass(1036)) {
            pModelCache = BSFaceGenManager::GetModelCache();
            pModelCache->UnloadAllEGMAndEGTData(false);
        }
    }

    MemoryLevelManager::EndMemoryFree(kFreeData);
}

// abLockedFaceGen says whether the face-gen caches may be cleared. It is a uint8_t rather than bool because the whole
// byte is tested; bool would assume 0/1.
void MemoryLevelManager::FreeReleasedObjects(uint8_t abLockedFaceGen) {
    GarbageCollector::ClearAll(true);
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->ProcessQueuedLights();
    NiObjectGarbage::ClearAll(false);
    if (abLockedFaceGen) {
        BSFaceGenManager::ClearModelCache();
        BSFaceGenManager::ClearEGTCache();
    }
    pModelLoader->ClearUnusedModels(true);
    TES::pInstance->CleanUpUnusedTextures(false);
}
