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
    uint8_t v2 = 1;
    if (iLastCheckedMemLevel > iLowMemoryLevelIdleCritical) {
        return (bool)((uint8_t)((iLastCheckedMemLevel - iLowMemoryLevelIdleCritical) <= 5242880));
    }
    return (bool)v2;
}

void MemoryLevelManager::BeginMemoryFree(
    MEM_FREE_DATA* aFreeData,
    bool abLockRenderer,
    bool abRunHeapClean,
    bool abLockFaceGen
) {
    NiDX9Renderer* renderer;
    pIOManager->Pause();
    if (abLockFaceGen) {
        aFreeData->bLockedFaceGen = (bool)((uint8_t)BSFaceGenManager::TryLockFaceGenAccess());
    }
    else {
        aFreeData->bLockedFaceGen = (bool)0;
    }
    if (abLockRenderer) {
        renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        renderer->LockRenderer();
    }
    aFreeData->bLockedRenderer = (bool)((uint8_t)abLockRenderer);
    aFreeData->bRunHeapClean = (bool)((uint8_t)abRunHeapClean);
    aFreeData->iTexturePurgeCount = BSTexturePalette::GetMaxTexturesToPurge();
    BSTexturePalette::SetMaxTexturesToPurge(2147483647);
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
        NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        Renderer_->UnlockRenderer();
    }
    pIOManager->Resume();
}

MemoryLevelManager* MemoryLevelManager::QInstance() {
    return &Instance;
}

void MemoryLevelManager::RunNonDestructiveFree() {
    MemoryLevelManager::MEM_FREE_DATA freedata;
    MemoryLevelManager::BeginMemoryFree(&freedata, 1, 1, 1);
    MemoryLevelManager::FreeReleasedObjects(freedata.bLockedFaceGen);
    if (pPlayer->GetAllowEGMCacheClear()) {
        if (BSFaceGenManager::GetModelCache()) {
            BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
            if (ModelCache->bHasFaceGenData) {
                if (!Menu::GetMenuByClass(1036)) {
                    BSFaceGenModelMap* v1 = BSFaceGenManager::GetModelCache();
                    v1->UnloadAllEGMAndEGTData(0);
                }
            }
        }
    }
    MemoryLevelManager::EndMemoryFree(freedata);
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
