#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"

#include <cstdint>
#include <xmmintrin.h>

// new NiD3DRenderStateGroup[auiCount]. The overflow guards turn a size that overflows into 0xFFFFFFFF, so the
// allocation fails. The element count is stored in a cookie before the first group.
template <>
NiD3DRenderStateGroup* NiTNewInterface<NiD3DRenderStateGroup>::Allocate(uint32_t auiCount) {
    size_t uiArraySize =
        auiCount <= SIZE_MAX / sizeof(NiD3DRenderStateGroup) ? sizeof(NiD3DRenderStateGroup) * auiCount : SIZE_MAX;
    uint32_t uiSize = uiArraySize + sizeof(size_t) < uiArraySize ? 0xFFFFFFFFu : uiArraySize + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(NiMemObject::operator new[](uiSize));
    if (!puiCookie) {
        return nullptr;
    }
    *puiCookie = auiCount;
    // The memory after the count cookie is the NiD3DRenderStateGroup array (the vector new's own layout).
    NiD3DRenderStateGroup* pkArray = reinterpret_cast<NiD3DRenderStateGroup*>(puiCookie + 1);
    // The constructor loop takes the element count as an int.
    for (int32_t i = 0; i < static_cast<int32_t>(auiCount); ++i) {
        ::new (static_cast<void*>(pkArray + i)) NiD3DRenderStateGroup;
    }
    return pkArray;
}

// delete[] of a NiD3DRenderStateGroup array: the destructor on each group, last first (the count from the cookie
// before the array, tested signed), then the block from the cookie.
template <>
void NiTNewInterface<NiD3DRenderStateGroup>::Deallocate(NiD3DRenderStateGroup* apArray) {
    if (apArray) {
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(apArray)[-1]);
        NiD3DRenderStateGroup* pkGroup = apArray + iCount;
        for (int32_t i = iCount - 1; i >= 0; --i) {
            (--pkGroup)->~NiD3DRenderStateGroup();
        }
        NiMemObject::operator delete[](reinterpret_cast<size_t*>(apArray) - 1, sizeof(NiD3DRenderStateGroup));
    }
}

// new NiD3DRSEntry[auiCount]: the entry is a NiMemObject with no destructor, so NiMemObject's operator new[] with no
// count cookie; a size past the address width saturates so that the allocation fails.
template <>
NiD3DRenderStateGroup::NiD3DRSEntry* NiTNewInterface<NiD3DRenderStateGroup::NiD3DRSEntry>::Allocate(uint32_t auiCount) {
    size_t stSize = auiCount <= SIZE_MAX / sizeof(NiD3DRSEntry) ? sizeof(NiD3DRSEntry) * auiCount : SIZE_MAX;
    NiD3DRSEntry* pkArray = static_cast<NiD3DRSEntry*>(NiMemObject::operator new[](stSize));
    if (pkArray) {
        // The element count is taken as an int.
        ::new (static_cast<void*>(pkArray)) NiD3DRSEntry[static_cast<int32_t>(auiCount)];
    }
    return pkArray;
}

// delete[] of an entry array: no destructor to run, the block straight to NiMemObject's operator delete[].
template <>
void NiTNewInterface<NiD3DRenderStateGroup::NiD3DRSEntry>::Deallocate(NiD3DRenderStateGroup::NiD3DRSEntry* apArray) {
    NiMemObject::operator delete[](apArray, sizeof(NiD3DRSEntry));
}

NiD3DRenderStateGroup::NiD3DRSEntry::NiD3DRSEntry()
  : m_uiRenderState(0), m_uiValue(0), m_pkNext(nullptr), m_pkPrev(nullptr) {}

NiD3DRenderStateGroup::NiD3DRenderStateGroup()
  : m_bRendererOwned(false), m_uiStateCount(0), m_pkRenderStates(nullptr), m_uiSavedStateCount(0),
    m_pkSavedRenderStates(nullptr) {}

// Both entry lists go back to the entry pool: an entry not yet on the free set is added to it.
NiD3DRenderStateGroup::~NiD3DRenderStateGroup() {
    for (NiD3DRSEntry* pkEntry : {m_pkRenderStates, m_pkSavedRenderStates}) {
        while (pkEntry) {
            NiD3DRSEntry* pkNext = pkEntry->GetNext();
            RSEntryPool* pkPool = NiD3DRenderStateGroup::ms_pkRenderStateEntryPool;
            if (pkPool->m_kFreeObjects.Find(pkEntry) == -1) {
                pkPool->m_kFreeObjects.Add(pkEntry);
            }
            pkEntry = pkNext;
        }
    }
    m_pkRenderStates = nullptr;
    m_pkSavedRenderStates = nullptr;
}

void NiD3DRenderStateGroup::SetRenderState(uint32_t uiRenderState, uint32_t uiValue, bool bSave) {
    bool bInSave;
    bInSave = false;
    NiD3DRenderStateGroup::NiD3DRSEntry* pkRSEntry = FindRenderStateEntry(uiRenderState, bInSave);
    if (pkRSEntry) {
        bool v6 = bInSave;
        pkRSEntry->m_uiValue = uiValue;
        if (v6 != bSave) {
            if (v6) {
                RemoveRSEntryFromSavedList(pkRSEntry);
                NiD3DRenderStateGroup::NiD3DRSEntry* m_pkRenderStates = this->m_pkRenderStates;
                if (m_pkRenderStates) {
                    m_pkRenderStates->m_pkPrev = pkRSEntry;
                    pkRSEntry->m_pkNext = this->m_pkRenderStates;
                }
                pkRSEntry->m_pkPrev = nullptr;
                ++m_uiStateCount;
                this->m_pkRenderStates = pkRSEntry;
            }
            else {
                RemoveRSEntryFromNoSaveList(pkRSEntry);
                NiD3DRenderStateGroup::NiD3DRSEntry* m_pkSavedRenderStates = this->m_pkSavedRenderStates;
                if (m_pkSavedRenderStates) {
                    m_pkSavedRenderStates->m_pkPrev = pkRSEntry;
                    pkRSEntry->m_pkNext = this->m_pkSavedRenderStates;
                }
                pkRSEntry->m_pkPrev = nullptr;
                ++m_uiSavedStateCount;
                this->m_pkSavedRenderStates = pkRSEntry;
            }
        }
    }
    else {
        uint32_t* p_m_uiUsed = &NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->m_kFreeObjects.m_uiUsed;
        NiTPool<NiD3DRenderStateGroup::NiD3DRSEntry, NiTNewInterface<NiD3DRenderStateGroup::NiD3DRSEntry>>* v10 =
            NiD3DRenderStateGroup::ms_pkRenderStateEntryPool;
        if (!NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->m_kFreeObjects.m_uiUsed) {
            uint32_t* p_m_uiCurrentSize = &NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->m_uiCurrentSize;
            NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->CreateNewObjects(
                NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->m_uiCurrentSize
            );
            *p_m_uiCurrentSize *= 2;
        }
        NiD3DRenderStateGroup::NiD3DRSEntry** m_pBase = v10->m_kFreeObjects.m_pBase;
        NiD3DRenderStateGroup::NiD3DRSEntry* v13 = *v10->m_kFreeObjects.m_pBase;
        *m_pBase = m_pBase[--*p_m_uiUsed];
        v13->m_pkNext = nullptr;
        v13->m_pkPrev = nullptr;
        v13->m_uiRenderState = uiRenderState;
        v13->m_uiValue = uiValue;
        if (bSave) {
            NiD3DRenderStateGroup::NiD3DRSEntry* v14 = this->m_pkSavedRenderStates;
            if (v14) {
                v14->m_pkPrev = v13;
                v13->m_pkNext = this->m_pkSavedRenderStates;
            }
            v13->m_pkPrev = nullptr;
            ++m_uiSavedStateCount;
            this->m_pkSavedRenderStates = v13;
        }
        else {
            NiD3DRenderStateGroup::NiD3DRSEntry* v15 = this->m_pkRenderStates;
            if (v15) {
                v15->m_pkPrev = v13;
                v13->m_pkNext = this->m_pkRenderStates;
            }
            v13->m_pkPrev = nullptr;
            ++m_uiStateCount;
            this->m_pkRenderStates = v13;
        }
    }
}

// SetRenderState's unused third argument is 1 for the saved list only.
uint32_t NiD3DRenderStateGroup::SetRenderStates() {
    for (NiD3DRSEntry* pkEntry = m_pkRenderStates; pkEntry; pkEntry = pkEntry->m_pkNext) {
        ms_pkD3DRenderState->SetRenderState(pkEntry->m_uiRenderState, pkEntry->m_uiValue, 0, false);
    }
    for (NiD3DRSEntry* pkEntry = m_pkSavedRenderStates; pkEntry; pkEntry = pkEntry->m_pkNext) {
        ms_pkD3DRenderState->SetRenderState(pkEntry->m_uiRenderState, pkEntry->m_uiValue, 1, false);
    }
    return 0;
}

uint32_t NiD3DRenderStateGroup::RestoreRenderStates() {
    for (NiD3DRSEntry* pkEntry = m_pkSavedRenderStates; pkEntry; pkEntry = pkEntry->m_pkNext) {
        NiD3DRenderState* pkRenderState = ms_pkD3DRenderState;
        uint32_t uiState = pkEntry->m_uiRenderState;
        pkRenderState->SetRenderState(uiState, pkRenderState->m_akRenderStateSettings[uiState].m_uiPrevValue, 0, false);
    }
    return 0;
}

void NiD3DRenderStateGroup::RemoveRSEntryFromNoSaveList(NiD3DRenderStateGroup::NiD3DRSEntry* pkEntry) {
    NiD3DRenderStateGroup::NiD3DRSEntry* m_pkNext;
    NiD3DRenderStateGroup::NiD3DRSEntry* m_pkPrev;
    m_pkNext = pkEntry->m_pkNext;
    m_pkPrev = pkEntry->m_pkPrev;
    if (m_pkNext) {
        m_pkNext->m_pkPrev = m_pkPrev;
    }
    if (m_pkPrev) {
        m_pkPrev->m_pkNext = m_pkNext;
    }
    if (pkEntry == m_pkRenderStates) {
        m_pkRenderStates = m_pkNext;
    }
    --m_uiStateCount;
    pkEntry->m_pkNext = nullptr;
    pkEntry->m_pkPrev = nullptr;
}

void NiD3DRenderStateGroup::RemoveRSEntryFromSavedList(NiD3DRenderStateGroup::NiD3DRSEntry* pkEntry) {
    NiD3DRenderStateGroup::NiD3DRSEntry* m_pkNext;
    NiD3DRenderStateGroup::NiD3DRSEntry* m_pkPrev;
    m_pkNext = pkEntry->m_pkNext;
    m_pkPrev = pkEntry->m_pkPrev;
    if (m_pkNext) {
        m_pkNext->m_pkPrev = m_pkPrev;
    }
    if (m_pkPrev) {
        m_pkPrev->m_pkNext = m_pkNext;
    }
    if (pkEntry == m_pkSavedRenderStates) {
        m_pkSavedRenderStates = m_pkNext;
    }
    --m_uiSavedStateCount;
    pkEntry->m_pkNext = nullptr;
    pkEntry->m_pkPrev = nullptr;
}

NiD3DRenderStateGroup::NiD3DRSEntry* NiD3DRenderStateGroup::FindRenderStateEntry(uint32_t uiState, bool& bInSaveList) {
    NiD3DRenderStateGroup::NiD3DRSEntry* result = m_pkRenderStates;
    if (result) {
        while (result->GetRenderState() != uiState) {
            result = result->GetNext();
            if (!result) {
                goto LABEL_4;
            }
        }
        bInSaveList = false;
    }
    else {
    LABEL_4:;
        result = m_pkSavedRenderStates;
        if (result) {
            while (result->GetRenderState() != uiState) {
                result = result->GetNext();
                if (!result) {
                    return nullptr;
                }
            }
            bInSaveList = true;
        }
        else {
            return nullptr;
        }
    }
    return result;
}

void NiD3DRenderStateGroup::RemoveRenderState(uint32_t uiRenderState) {
    bool bInSave;
    NiD3DRenderStateGroup::NiD3DRSEntry* v7;
    bInSave = false;
    NiD3DRenderStateGroup::NiD3DRSEntry* pkRSEntry = FindRenderStateEntry(uiRenderState, bInSave);
    if (pkRSEntry) {
        NiD3DRenderStateGroup::NiD3DRSEntry* m_pkNext = pkRSEntry->GetNext();
        NiD3DRenderStateGroup::NiD3DRSEntry* m_pkPrev = pkRSEntry->GetPrev();
        if (bInSave) {
            if (m_pkNext) {
                m_pkNext->m_pkPrev = m_pkPrev;
            }
            if (m_pkPrev) {
                m_pkPrev->m_pkNext = m_pkNext;
            }
            if (pkRSEntry == m_pkSavedRenderStates) {
                m_pkSavedRenderStates = m_pkNext;
            }
            --m_uiSavedStateCount;
        }
        else {
            if (m_pkNext) {
                m_pkNext->m_pkPrev = m_pkPrev;
            }
            if (m_pkPrev) {
                m_pkPrev->m_pkNext = m_pkNext;
            }
            if (pkRSEntry == m_pkRenderStates) {
                m_pkRenderStates = m_pkNext;
            }
            --m_uiStateCount;
        }
        pkRSEntry->m_pkPrev = nullptr;
        pkRSEntry->m_pkNext = nullptr;
        v7 = pkRSEntry;
        NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->ReleaseObject(v7);
    }
}

// Takes a group from the pool as NiTPool::GetFreeObject does: an empty free set makes m_uiCurrentSize new groups and
// doubles that size; the first free group is taken and the last one moved into its slot.
NiD3DRenderStateGroup* NiD3DRenderStateGroup::GetFreeRenderStateGroup() {
    RSGroupPool* pkPool = NiD3DRenderStateGroup::ms_pkRenderStateGroupPool;
    if (!pkPool->m_kFreeObjects.m_uiUsed) {
        pkPool->CreateNewObjects(pkPool->m_uiCurrentSize);
        pkPool->m_uiCurrentSize *= 2;
    }
    NiD3DRenderStateGroup** ppkBase = pkPool->m_kFreeObjects.m_pBase;
    NiD3DRenderStateGroup* pkGroup = *ppkBase;
    *ppkBase = ppkBase[--pkPool->m_kFreeObjects.m_uiUsed];
    if (!pkGroup->m_bRendererOwned) {
        pkGroup->m_bRendererOwned = true;
    }
    return pkGroup;
}

// A renderer-owned group's entries go back to the entry pool and the emptied group to the group pool; any other group
// is destroyed and deleted.
void NiD3DRenderStateGroup::ReleaseRenderStateGroup(NiD3DRenderStateGroup* pkGroup) {
    if (pkGroup) {
        if (pkGroup->m_bRendererOwned) {
            NiD3DRSEntry* pkEntry = pkGroup->m_pkRenderStates;
            while (pkEntry) {
                NiD3DRSEntry* pkNext = pkEntry->GetNext();
                NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->ReleaseObject(pkEntry);
                pkEntry = pkNext;
            }
            pkGroup->m_pkRenderStates = nullptr;
            pkGroup->m_uiStateCount = 0;
            pkEntry = pkGroup->m_pkSavedRenderStates;
            while (pkEntry) {
                NiD3DRSEntry* pkNext = pkEntry->GetNext();
                NiD3DRenderStateGroup::ms_pkRenderStateEntryPool->ReleaseObject(pkEntry);
                pkEntry = pkNext;
            }
            pkGroup->m_pkSavedRenderStates = nullptr;
            pkGroup->m_uiSavedStateCount = 0;
            NiD3DRenderStateGroup::ms_pkRenderStateGroupPool->ReleaseObject(pkGroup);
        }
        else {
            delete pkGroup;
        }
    }
}

void NiD3DRenderStateGroup::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    NiD3DRenderStateGroup::ms_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        NiD3DRenderStateGroup::ms_pkD3DRenderState = pkD3DRenderer->m_pkRenderState;
    }
    else {
        NiD3DRenderStateGroup::ms_pkD3DRenderState = nullptr;
    }
}

void NiD3DRenderStateGroup::InitializePools() {
    NiD3DRenderStateGroup::RSGroupPool* v0 = static_cast<NiD3DRenderStateGroup::RSGroupPool*>(
        MemoryManager_Singleton.Allocate(sizeof(NiD3DRenderStateGroup::RSGroupPool))
    );
    if (v0) {
        v0->m_kFreeObjects.m_pBase = nullptr;
        v0->m_kFreeObjects.m_uiAlloced = 0;
        v0->m_kFreeObjects.m_uiUsed = 0;
        v0->m_pkContainers = nullptr;
        v0->m_uiCurrentSize = 8;
        v0->m_uiInitialSize = 8;
        NiD3DRenderStateGroup::ms_pkRenderStateGroupPool = v0;
    }
    else {
        NiD3DRenderStateGroup::ms_pkRenderStateGroupPool = nullptr;
    }
    NiD3DRenderStateGroup::RSEntryPool* v1 = static_cast<NiD3DRenderStateGroup::RSEntryPool*>(
        MemoryManager_Singleton.Allocate(sizeof(NiD3DRenderStateGroup::RSEntryPool))
    );
    if (v1) {
        v1->m_kFreeObjects.m_pBase = nullptr;
        v1->m_kFreeObjects.m_uiAlloced = 0;
        v1->m_kFreeObjects.m_uiUsed = 0;
        v1->m_pkContainers = nullptr;
        v1->m_uiCurrentSize = 16;
        v1->m_uiInitialSize = 16;
        NiD3DRenderStateGroup::ms_pkRenderStateEntryPool = v1;
    }
    else {
        NiD3DRenderStateGroup::ms_pkRenderStateEntryPool = nullptr;
    }
}

// Deletes both pools: for each, the free set is emptied, the containers are deleted, and the free set's buffer is
// freed.
void NiD3DRenderStateGroup::ShutdownPools() {
    RSGroupPool* pkGroupPool = NiD3DRenderStateGroup::ms_pkRenderStateGroupPool;
    if (pkGroupPool) {
        NiTPoolContainer<NiD3DRenderStateGroup, NiTNewInterface<NiD3DRenderStateGroup>>* pkContainers =
            pkGroupPool->m_pkContainers;
        pkGroupPool->m_kFreeObjects.m_uiUsed = 0;
        if (pkContainers) {
            pkContainers->~NiTPoolContainer<NiD3DRenderStateGroup, NiTNewInterface<NiD3DRenderStateGroup>>();
            MemoryManager_Singleton.Deallocate(pkContainers);
        }
        _NiFree(pkGroupPool->m_kFreeObjects.m_pBase);
        MemoryManager_Singleton.Deallocate(pkGroupPool);
    }
    RSEntryPool* pkEntryPool = NiD3DRenderStateGroup::ms_pkRenderStateEntryPool;
    if (pkEntryPool) {
        NiTPoolContainer<NiD3DRenderStateGroup::NiD3DRSEntry, NiTNewInterface<NiD3DRenderStateGroup::NiD3DRSEntry>>*
            pkContainers = pkEntryPool->m_pkContainers;
        pkEntryPool->m_kFreeObjects.m_uiUsed = 0;
        if (pkContainers) {
            NiMemObject::operator delete[](pkContainers->m_pkObjectArray, sizeof(NiD3DRenderStateGroup::NiD3DRSEntry));
            NiTPoolContainer<NiD3DRenderStateGroup::NiD3DRSEntry, NiTNewInterface<NiD3DRenderStateGroup::NiD3DRSEntry>>*
                pkNext = pkContainers->m_pkNext;
            delete pkNext;
            MemoryManager_Singleton.Deallocate(pkContainers);
        }
        _NiFree(pkEntryPool->m_kFreeObjects.m_pBase);
        MemoryManager_Singleton.Deallocate(pkEntryPool);
    }
}
