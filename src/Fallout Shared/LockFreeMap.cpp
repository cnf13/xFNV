#include "Fallout Shared/LockFreeMap.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSSystem/BSArchive.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/QueuedReplacementKFList.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/LipSyncBackgroundManager.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

// The per-thread table (an empty entry per thread) and the thread-local storage index.
ThreadSpecificInterfaceManager::ThreadSpecificInterfaceManager(uint32_t auiMaxThreads)
  : uiMaxThreads(auiMaxThreads), uiTLSIndex(0), pInterfaces(nullptr), uiThreadCount(0) {
    pInterfaces = static_cast<ThreadSpecificInterface*>(ourMalloc(sizeof(ThreadSpecificInterface) * uiMaxThreads));
    for (uint32_t i = 0; i < uiMaxThreads; ++i) {
        pInterfaces[i].iThreadID = 0;
        pInterfaces[i].pInterface = nullptr;
    }
    uiTLSIndex = BSTlsAlloc();
}

// Frees the table and the thread-local storage index. The threads' interfaces are not deleted here: their class depends
// on the map or queue that handed them out, which this class does not know.
ThreadSpecificInterfaceManager::~ThreadSpecificInterfaceManager() {
    ourFree(pInterfaces);
    BSTlsFree(uiTLSIndex);
}

InterfacedClass::BaseInterface* ThreadSpecificInterfaceManager::GetInterface(InterfacedClass* apInterfacedClass) {
    InterfacedClass::BaseInterface* pInterface = static_cast<InterfacedClass::BaseInterface*>(GetTlsValue(uiTLSIndex));
    if (pInterface) {
        return pInterface;
    }
    return AddInterface(apInterfacedClass);
}

void ThreadSpecificInterfaceManager::Shutdown() {
    for (uint32_t i = 0; i < uiMaxThreads; ++i) {
        delete static_cast<LockFreeQueue<NiPointer<IOTask>>::LockFreeQueueInterface*>(pInterfaces[i].pInterface);
    }
    MemoryManager::Instance().Deallocate(pInterfaces);
    TlsFree(uiTLSIndex);
}

void* GetTlsValue(uint32_t dwTlsIndex) {
    return TlsGetValue(dwTlsIndex);
}
