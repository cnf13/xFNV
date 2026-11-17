#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiStaticDataManager.h"

void NiMemManager::_SDMInit() {
    NiMemManager::ms_pkMemManager =
        static_cast<NiMemManager*>((&MemoryManager_Singleton)->Allocate(sizeof(NiMemManager)));
    NiMemManager::ms_pkMemManager->m_pkAllocator = NiStaticDataManager::ms_pkInitOptions->GetAllocator();
    NiMemManager::ms_pkMemManager->m_pkAllocator->Initialize();
}
