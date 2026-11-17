#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiD3DRenderState;
class NiDX9Renderer;

// A set of render states a pass applies and restores, kept as two linked lists (applied only, and saved/restored).
// Groups and entries come from pools.
class NiD3DRenderStateGroup : public NiMemObject {
public:
    // The group pool, made by InitializePools and used as an NiTPool by GetFreeRenderStateGroup and ShutdownPools.
    using RSGroupPool = NiTPool<NiD3DRenderStateGroup, NiTNewInterface<NiD3DRenderStateGroup>>;

    class NiD3DRSEntry : public NiMemObject {
    public:
        NiD3DRSEntry();

        uint32_t GetRenderState() const {
            return m_uiRenderState;
        }
        uint32_t GetValue() const {
            return m_uiValue;
        }
        NiD3DRSEntry* GetNext() const {
            return m_pkNext;
        }
        NiD3DRSEntry* GetPrev() const {
            return m_pkPrev;
        }

        uint32_t m_uiRenderState;
        uint32_t m_uiValue;
        NiD3DRSEntry* m_pkNext;
        NiD3DRSEntry* m_pkPrev;
    };

    // The entry pool, made by InitializePools.
    using RSEntryPool = NiTPool<NiD3DRSEntry, NiTNewInterface<NiD3DRSEntry>>;

    NiD3DRenderStateGroup();
    ~NiD3DRenderStateGroup();

    void SetRenderState(uint32_t uiRenderState, uint32_t uiValue, bool bSave);
    // Applies every state: the unsaved list first, then the saved list. Always 0.
    uint32_t SetRenderStates();
    // Puts back the previous value of every saved state. Always 0.
    uint32_t RestoreRenderStates();
    void RemoveRSEntryFromNoSaveList(NiD3DRenderStateGroup::NiD3DRSEntry* pkEntry);
    void RemoveRSEntryFromSavedList(NiD3DRenderStateGroup::NiD3DRSEntry* pkEntry);

protected:
    NiD3DRSEntry* FindRenderStateEntry(uint32_t uiState, bool& bInSaveList);

public:
    void RemoveRenderState(uint32_t uiRenderState);

    // Takes a group from the pool and marks it renderer-owned.
    static NiD3DRenderStateGroup* GetFreeRenderStateGroup();
    // Returns a renderer-owned group's entries and the group to their pools; deletes any other group.
    static void ReleaseRenderStateGroup(NiD3DRenderStateGroup* pkGroup);
    static void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);
    static void InitializePools();
    static void ShutdownPools();

    bool m_bRendererOwned;
    uint32_t m_uiStateCount;
    NiD3DRSEntry* m_pkRenderStates;
    uint32_t m_uiSavedStateCount;
    NiD3DRSEntry* m_pkSavedRenderStates;

    static inline RSGroupPool* ms_pkRenderStateGroupPool;
    // The renderer's render state the groups apply through.
    static inline NiD3DRenderState* ms_pkD3DRenderState;
    // The renderer SetD3DRenderer sets.
    static inline NiDX9Renderer* ms_pkD3DRenderer;
    static inline RSEntryPool* ms_pkRenderStateEntryPool;
};

using NiD3DRSEntry = NiD3DRenderStateGroup::NiD3DRSEntry;

// The entry pool's new[] and delete[]: NiMemObject's operator new[] and delete[], as for the group pool (whose
// specializations are in NiTCollection.h). Defined in NiD3DRenderStateGroup.cpp.
template <>
NiD3DRSEntry* NiTNewInterface<NiD3DRSEntry>::Allocate(uint32_t uiNumElements);
template <>
void NiTNewInterface<NiD3DRSEntry>::Deallocate(NiD3DRSEntry* pArray);
