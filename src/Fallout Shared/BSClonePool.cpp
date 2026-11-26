#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BSClonePools.h"
#include "Fallout Shared/BSCloneReserver.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSClonePools::ClonePool::ClonePool() : pObject(nullptr) {
    kAvailableClones.RemoveAll();
}

// The destructor drops the model, and the list's own destructor releases the clones, which is the work Free does.
BSClonePools::ClonePool::~ClonePool() {
    pObject = nullptr;
}

// Each pass hands a null pointer to the head item (no change), releases it, then drops the head node; the list is
// destroyed at the end.
void BSClonePools::ClonePool::Free() {
    pObject = nullptr;
    while (!kAvailableClones.IsEmpty()) {
        {
            NiPointer<NiRefObject> spClone(nullptr);
            kAvailableClones.m_item.SetIfNotNull(&spClone);
        }
        kAvailableClones.RemoveHead();
    }
}

void BSClonePools::ClonePool::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSClonePools::BSClonePools() {
    ClonePools.RemoveAll();
}

void BSClonePools::KillClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount) {
    if (apLoadedObject) {
        BSClonePools::ClonePool* pPool = nullptr;
        for (BSSimpleList<BSClonePools::ClonePool*>* pList = &ClonePools; pList && !pPool && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            BSClonePools::ClonePool* pItem = pList->m_item;
            if (pItem && pItem->pObject == apLoadedObject) {
                pPool = pItem;
            }
        }
        if (pPool) {
            for (uint32_t i = 0; i < aiCloneCount && !pPool->kAvailableClones.IsEmpty(); ++i) {
                pPool->kAvailableClones.RemoveHead();
            }
            if (pPool->kAvailableClones.IsEmpty()) {
                ClonePools.Remove(pPool);
                if (pPool) {
                    delete pPool;
                }
            }
        }
    }
}

void BSClonePools::MakeClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount) {
    if (apLoadedObject) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_TES_REFERENCES,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BSClonePool.cpp",
            56
        );
        BSClonePools::ClonePool* pPool = nullptr;
        for (BSSimpleList<BSClonePools::ClonePool*>* pList = &ClonePools; pList && !pPool && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            BSClonePools::ClonePool* pItem = pList->m_item;
            if (pItem && pItem->pObject == apLoadedObject) {
                pPool = pItem;
            }
        }
        if (!pPool) {
            pPool = new BSClonePools::ClonePool;
            pPool->pObject = apLoadedObject;
            ClonePools.AddHead(pPool);
        }
        for (uint32_t i = 0; i < aiCloneCount; ++i) {
            BSCloningProcess process(1.0f);
            NiAVObject* pClone = static_cast<NiAVObject*>(apLoadedObject->Clone(process));
            NiPointer<NiRefObject> spClone(static_cast<NiRefObject*>(pClone));
            pPool->kAvailableClones.AddHead(spClone);
        }
    }
}

NiAVObject* BSClonePools::RequestClone(NiAVObject* apLoadedObject) {
    BSClonePools::ClonePool* pPool = nullptr;
    for (BSSimpleList<BSClonePools::ClonePool*>* pList = &ClonePools; pList && !pPool && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        BSClonePools::ClonePool* pItem = pList->m_item;
        if (pItem && pItem->pObject == apLoadedObject) {
            pPool = pItem;
        }
    }
    NiAVObject* pResult = nullptr;
    if (pPool) {
        for (BSSimpleList<NiPointer<NiRefObject>>* pClones = &pPool->kAvailableClones;
             !pResult && pClones && !pClones->IsEmpty();
             pClones = pClones->m_pkNext) {
            // The pool holds the clones MakeClones made of an NiAVObject.
            NiAVObject* pClone = static_cast<NiAVObject*>(pClones->m_item.m_pObject);
            if (pClone && pClone->m_uiRefCount == 1) {
                pResult = pClone;
            }
        }
    }
    return pResult;
}

BSClonePools* BSClonePools::GetInstance(bool abCreate) {
    if (!BSClonePools::pInstance && abCreate) {
        BSClonePools* pClonePools = new BSClonePools;
        BSClonePools::pInstance = pClonePools;
    }
    return BSClonePools::pInstance;
}

BSCloneReserver::BSCloneReserver(NiAVObject* apModel, uint32_t aiCloneCount)
  : pModel(apModel), iCloneCount(aiCloneCount), iRefCount(0) {
    Init();
}

BSCloneReserver::~BSCloneReserver() {
    if (pModel && iCloneCount) {
        BSClonePools::GetInstance(true)->KillClones(pModel, iCloneCount);
    }
}

void BSCloneReserver::DecRefCount() {
    if (!NiAtomicDecrement(iRefCount) && this) {
        delete this;
    }
}

void BSCloneReserver::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

void BSCloneReserver::Init() {
    if (pModel && iCloneCount) {
        int32_t iCount = static_cast<int32_t>(iCloneCount);
        NiAVObject* pLoadedObject = pModel;
        BSClonePools* pClonePools = BSClonePools::GetInstance(true);
        pClonePools->MakeClones(pLoadedObject, iCount);
    }
}

void BSCloneReserver::operator delete(void* apMemory) {
    ourFree(apMemory);
}
