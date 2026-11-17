#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
        BSClonePools::ClonePool* ppool = nullptr;
        for (BSSimpleList<BSClonePools::ClonePool*>* plist = &ClonePools; plist && (!ppool && !plist->IsEmpty());
             plist = plist->m_pkNext) {
            BSClonePools::ClonePool* pitem = plist->m_item;
            if ((pitem) && (pitem->pObject == apLoadedObject)) {
                ppool = pitem;
            }
        }
        if (ppool) {
            for (uint32_t i = 0; i < aiCloneCount && !ppool->kAvailableClones.IsEmpty(); ++i) {
                ppool->kAvailableClones.RemoveHead();
            }
            if (ppool->kAvailableClones.IsEmpty()) {
                ClonePools.Remove(ppool);
                if (ppool) {
                    delete ppool;
                }
            }
        }
    }
}

void BSClonePools::MakeClones(NiAVObject* apLoadedObject, uint32_t aiCloneCount) {
    if (apLoadedObject) {
        AutoMemContext tempMemContext(
            MEM_CONTEXT_TES_REFERENCES, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BSClonePool.cpp",
            56
        );
        BSClonePools::ClonePool* ppool = nullptr;
        for (BSSimpleList<BSClonePools::ClonePool*>* plist = &ClonePools; plist && (!ppool && !plist->IsEmpty());
             plist = plist->m_pkNext) {
            BSClonePools::ClonePool* pitem = plist->m_item;
            if ((pitem) && (pitem->pObject == apLoadedObject)) {
                ppool = pitem;
            }
        }
        if (!ppool) {
            BSClonePools::ClonePool* ppool_2;
            BSClonePools::ClonePool* v6 =
                static_cast<BSClonePools::ClonePool*>(ourMalloc(sizeof(BSClonePools::ClonePool)));
            if (v6) {
                ppool_2 = ::new (v6) BSClonePools::ClonePool;
            }
            else {
                ppool_2 = nullptr;
            }
            ppool = ppool_2;
            ppool_2->pObject = (NiObject*)apLoadedObject;
            ClonePools.AddHead(ppool);
        }
        for (uint32_t i = 0; i < aiCloneCount; ++i) {
            BSCloningProcess process(1.0f);
            NiAVObject* pclone = static_cast<NiAVObject*>(apLoadedObject->Clone(process));
            NiPointer<NiRefObject> spClone(static_cast<NiRefObject*>(pclone));
            ppool->kAvailableClones.AddHead(spClone);
        }
    }
}

NiAVObject* BSClonePools::RequestClone(NiAVObject* apLoadedObject) {
    BSClonePools::ClonePool* ppool = nullptr;
    for (BSSimpleList<BSClonePools::ClonePool*>* plist = &ClonePools; plist && (!ppool && !plist->IsEmpty());
         plist = plist->m_pkNext) {
        BSClonePools::ClonePool* pitem = plist->m_item;
        if ((pitem) && (pitem->pObject == apLoadedObject)) {
            ppool = pitem;
        }
    }
    NiAVObject* presult = nullptr;
    if (ppool) {
        for (BSSimpleList<NiPointer<NiRefObject>>* pclones = &ppool->kAvailableClones;
             !presult && (pclones && !pclones->IsEmpty()); pclones = pclones->m_pkNext) {
            // The pool holds the clones MakeClones made of an NiAVObject.
            NiAVObject* pclone = static_cast<NiAVObject*>(pclones->m_item.m_pObject);
            if ((pclone) && (pclone->m_uiRefCount == 1)) {
                presult = pclone;
            }
        }
    }
    return presult;
}

BSClonePools* BSClonePools::GetInstance(bool a1) {
    BSClonePools* v2;
    void* v3;
    if ((!BSClonePools::pInstance) && a1) {
        v3 = ourMalloc(sizeof(BSClonePools));
        if (v3) {
            v2 = ::new (v3) BSClonePools;
        }
        else {
            v2 = nullptr;
        }
        BSClonePools::pInstance = v2;
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
    if ((!NiAtomicDecrement(iRefCount)) && (this)) {
        delete this;
    }
}

void BSCloneReserver::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

void BSCloneReserver::Init() {
    BSClonePools* singleton;
    NiAVObject* pModel;
    int32_t uiCloneCount;
    if ((this->pModel) && (iCloneCount)) {
        uiCloneCount = (int32_t)iCloneCount;
        pModel = this->pModel;
        singleton = BSClonePools::GetInstance(true);
        singleton->MakeClones(pModel, uiCloneCount);
    }
}

void BSCloneReserver::operator delete(void* apMemory) {
    ourFree(apMemory);
}
