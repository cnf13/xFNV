#include "BSCore/IMemoryHeap.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/MemoryPool.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/BSNiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

void MemoryManager::SpecifyPools() {
    // AddPool takes the pool description as char*; the names are constant text, hence the const_cast.
    AddPool(8, 6291456, const_cast<char*>("Pool 8"));
    AddPool(12, 5242880, const_cast<char*>("Pool 12"));
    AddPool(16, 4194304, const_cast<char*>("Pool 16"));
    AddPool(20, 4194304, const_cast<char*>("Pool 20"));
    AddPool(24, 8388608, const_cast<char*>("Pool 24"));
    AddPool(28, 4194304, const_cast<char*>("Pool 28"));
    AddPool(32, 8388608, const_cast<char*>("Pool 32"));
    AddPool(36, 3145728, const_cast<char*>("Pool 36"));
    AddPool(40, 2097152, const_cast<char*>("Pool 40"));
    AddPool(44, 4194304, const_cast<char*>("Pool 44"));
    AddPool(48, 2097152, const_cast<char*>(aPool48));
    AddPool(52, 3145728, const_cast<char*>("Pool 52"));
    AddPool(56, 1048576, const_cast<char*>(aPool56));
    AddPool(64, 2097152, const_cast<char*>("Pool 64"));
    AddPool(72, 13631488, const_cast<char*>(aPool72));
    AddPool(76, 1048576, const_cast<char*>("Pool 76"));
    AddPool(80, 3145728, const_cast<char*>("Pool 80"));
    AddPool(92, 2097152, const_cast<char*>("Pool 92"));
    AddPool(96, 15728640, const_cast<char*>("Pool 96"));
    AddPool(100, 1048576, const_cast<char*>("Pool 100"));
    AddPool(104, 4194304, const_cast<char*>("Pool 104"));
    AddPool(120, 4194304, const_cast<char*>("Pool 120"));
    AddPool(128, 1048576, const_cast<char*>("Pool 128"));
    AddPool(160, 2097152, const_cast<char*>("Pool 160"));
    AddPool(192, 2097152, const_cast<char*>("Pool 192"));
    AddPool(200, 3145728, const_cast<char*>("Pool 200"));
    AddPool(208, 3145728, const_cast<char*>("Pool 208"));
    AddPool(240, 3145728, const_cast<char*>("Pool 240"));
    AddPool(256, 1048576, const_cast<char*>("Pool 256"));
    AddPool(272, 2097152, const_cast<char*>("Pool 272"));
    AddPool(288, 3145728, const_cast<char*>("Pool 288"));
    AddPool(400, 1048576, const_cast<char*>("Pool 400"));
    AddPool(448, 1048576, const_cast<char*>("Pool 448"));
    AddPool(480, 4194304, const_cast<char*>("Pool 480"));
    AddPool(508, 4194304, const_cast<char*>("Pool 508"));
    AddPool(576, 2097152, const_cast<char*>("Pool 576"));
    AddPool(596, 1048576, const_cast<char*>("Pool 596"));
}

uint32_t MemoryManager::ProcessMemoryProblem(IMemoryHeap* apHeap, int32_t aiMemoryPass, bool* apbAllowSystemAllocs) {
    *apbAllowSystemAllocs = true;
    uint32_t uiCurrentThreadID = BSGetCurrentThreadID();
    uint32_t uiMainThreadID = appMain->uiThreadID;
    switch (aiMemoryPass) {
        case 0: {
            uiReallocOnCrossPageBoundaryPaddingST = 0;
            if (uiCurrentThreadID == uiMainThreadID) {
                TES::pInstance->CleanUpUnusedTextures(false);
            }
            return aiMemoryPass + 1;
        }
        case 1: {
            ScrapHeapManager::GetSingleton()->FreeAllBuffers();
            return aiMemoryPass + 1;
        }
        case 2: {
            if (BSFaceGenManager::GetModelCache()) {
                BSFaceGenModelMap* ModelCache = BSFaceGenManager::GetModelCache();
                if (ModelCache->bHasFaceGenData && BSFaceGenManager::TryLockFaceGenAccess()) {
                    if (BSFaceGenModelMap::GetCSEnterCounter() == 1) {
                        BSFaceGenModelMap* pModelCache = BSFaceGenManager::GetModelCache();
                        pModelCache->UnloadAllEGMAndEGTData(false);
                    }
                    BSFaceGenManager::UnlockFaceGenAccess();
                }
            }
            return aiMemoryPass + 1;
        }
        case 5: {
            if (uiCurrentThreadID != uiMainThreadID) {
                return aiMemoryPass + 1;
            }
            GarbageCollector::CanRunGarbageCollector(false);
            if (TES::pInstance->ClearOldestBufferedCell()) {
                --aiMemoryPass;
            }
            else {
                TES::pInstance->ClearAllCells(true, false);
            }
            TES::pInstance->CleanUpUnusedTextures(false);
            GarbageCollector::CanRunGarbageCollector(true);
            GarbageCollector::ClearAll(true);
            [[fallthrough]];
        }
        case 4: {
            if (ReferenceProcessLists.kUpdateLock.TryLock()) {
                GarbageCollector::ClearAll(true);
                ReferenceProcessLists.kUpdateLock.Unlock();
            }
            [[fallthrough]];
        }
        case 3: {
            NiObjectGarbage::ClearAll(true);
            break;
        }
        case 6: {
            if (uiCurrentThreadID == uiMainThreadID) {
                MemoryPool::FreeUnusedPagesForAllPools();
            }
            return aiMemoryPass + 1;
        }
        case 7: {
            if (!*apbAllowSystemAllocs && uiCurrentThreadID == uiMainThreadID) {
                *apbAllowSystemAllocs = true;
                break;
            }
            [[fallthrough]];
        }
        case 8: {
            if (uiCurrentThreadID != uiMainThreadID) {
                uiEmergencyCleanupPassCount = 6;
                if (uiReallocOnCrossPageBoundaryPaddingST >= 0x3A98u) {
                    if (!*apbAllowSystemAllocs) {
                        *apbAllowSystemAllocs = true;
                    }
                }
                else {
                    uint32_t TaskThreadID = pIOManager->GetTaskThreadID(0);
                    if (TaskThreadID == BSGetCurrentThreadID()) {
                        pIOManager->PauseThread(0);
                        pIOManager->WaitForThread(0);
                    }
                    uint32_t uiTaskThread1ID = pIOManager->GetTaskThreadID(1);
                    if (uiTaskThread1ID == BSGetCurrentThreadID()) {
                        pIOManager->PauseThread(1);
                        pIOManager->WaitForThread(1);
                    }
                    BSSleep(1u);
                    ++uiReallocOnCrossPageBoundaryPaddingST;
                    --aiMemoryPass;
                }
            }
            break;
        }
        default: {
            return aiMemoryPass + 1;
        }
    }
    return aiMemoryPass + 1;
}

void MemoryManager::RegisterMemoryManager() {
    NiStaticDataManager::ms_pkInitOptions = new NiInitOptions(new BSNiAllocator);
    NiMemManager::_SDMInit();
    BSSystemUtility::QInstance().Initialize(nullptr, nullptr, 0);
}
