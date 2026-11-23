#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "SDK/MSVC/ehvec.h"

#include <cstdint>
#include <xmmintrin.h>

// The element count is taken as an int: a count with the top bit set constructs nothing.
template <>
NiD3DTextureStageGroup* NiTNewInterface<NiD3DTextureStageGroup>::Allocate(uint32_t uiNumElements) {
    void* pMemory = MemoryManager::Instance().Allocate(sizeof(NiD3DTextureStageGroup) * size_t{uiNumElements});
    if (!pMemory) {
        return nullptr;
    }
    NiD3DTextureStageGroup* pArray = static_cast<NiD3DTextureStageGroup*>(pMemory);
    NiD3DTextureStageGroup* pElement = pArray;
    for (int32_t iCount = static_cast<int32_t>(uiNumElements); --iCount >= 0; ++pElement) {
        ::new (static_cast<void*>(pElement)) NiD3DTextureStageGroup();
    }
    return pArray;
}

template <>
void NiTNewInterface<NiD3DTextureStageGroup>::Deallocate(NiD3DTextureStageGroup* pArray) {
    if (pArray) {
        MemoryManager::Instance().Deallocate(pArray);
    }
}

// Not renderer-owned, no counts or iterators, every value and flag cleared.
NiD3DTextureStageGroup::NiD3DTextureStageGroup()
  : m_bRendererOwned(false), m_uiNumSavedStages(0), m_uiSavedStageArrayIter(0), m_auiSavedStageArray{},
    m_abSavedStageValid{}, m_uiNumStages(0), m_uiStageArrayIter(0), m_auiStageArray{}, m_abStageValid{},
    m_uiNumSavedSamplers(0), m_uiSavedSamplerArrayIter(0), m_auiSavedSamplerArray{}, m_abSavedSamplerValid{},
    m_uiNumSamplers(0), m_uiSamplerArrayIter(0), m_auiSamplerArray{}, m_abSamplerValid{}, m_uiNumUseMapValue(0),
    m_abSamplerUseMapValue{} {}

uint32_t NiD3DTextureStageGroup::SetTextureStage(uint32_t uiStage) const {
    uint32_t v2 = 0;
    if (uiStage < NiD3DPass::ms_uiMaxTextureBlendStages) {
        v2 = SetAllStageStates(uiStage);
    }
    uint32_t result = SetAllStageSamplers(uiStage);
    if (v2) {
        return v2;
    }
    return result;
}

// Sets each stage state the group has, else its saved value when it has one.
uint32_t NiD3DTextureStageGroup::SetAllStageStates(uint32_t uiStage) const {
    for (uint32_t i = 0; i < 8; ++i) {
        if (m_abStageValid[i]) {
            ms_pkD3DRenderState->SetTextureStageState(uiStage, textureStageTypes[i], m_auiStageArray[i], false);
        }
        else if (m_abSavedStageValid[i]) {
            ms_pkD3DRenderState->SetTextureStageState(uiStage, textureStageTypes[i], m_auiSavedStageArray[i], false);
        }
    }
    return 0;
}

// Sets each sampler state the group has, else its saved value when it has one.
uint32_t NiD3DTextureStageGroup::SetAllStageSamplers(uint32_t uiStage) const {
    for (uint32_t i = 0; i < 5; ++i) {
        if (m_abSamplerValid[i]) {
            ms_pkD3DRenderState->SetSamplerState(uiStage, ms_aeArrayToSamplerState[i], m_auiSamplerArray[i], false);
        }
        else if (m_abSavedSamplerValid[i]) {
            ms_pkD3DRenderState
                ->SetSamplerState(uiStage, ms_aeArrayToSamplerState[i], m_auiSavedSamplerArray[i], false);
        }
    }
    return 0;
}

void NiD3DTextureStageGroup::SetSamplerState(uint32_t uiState, uint32_t uiValue, bool bSave, bool bUseMapValue) {
    uint16_t v5 = NiD3DRenderState::ms_auiSamplesStatesRevMappings[uiState];
    if (v5 < 5u) {
        if (bSave) {
            if (m_abSavedSamplerValid[v5]) {
                m_auiSavedSamplerArray[v5] = uiValue;
                if (bUseMapValue && !m_abSamplerUseMapValue[v5]) {
                    ++m_uiNumUseMapValue;
                }
                else if (!bUseMapValue && m_abSamplerUseMapValue[v5]) {
                    --m_uiNumUseMapValue;
                }
                m_abSamplerUseMapValue[v5] = bUseMapValue;
            }
            else {
                if (m_abSamplerValid[v5]) {
                    m_abSamplerValid[v5] = false;
                    --m_uiNumSamplers;
                    if (m_abSamplerUseMapValue[v5]) {
                        --m_uiNumUseMapValue;
                    }
                    m_abSamplerUseMapValue[v5] = false;
                }
                m_abSavedSamplerValid[v5] = true;
                m_auiSavedSamplerArray[v5] = uiValue;
                ++m_uiNumSavedSamplers;
                if (bUseMapValue) {
                    ++m_uiNumUseMapValue;
                }
                m_abSamplerUseMapValue[v5] = bUseMapValue;
            }
        }
        else if (m_abSamplerValid[v5]) {
            m_auiSamplerArray[v5] = uiValue;
            if (bUseMapValue && !m_abSamplerUseMapValue[v5]) {
                ++m_uiNumUseMapValue;
            }
            else if (!bUseMapValue && m_abSamplerUseMapValue[v5]) {
                --m_uiNumUseMapValue;
            }
            m_abSamplerUseMapValue[v5] = bUseMapValue;
        }
        else {
            if (m_abSavedSamplerValid[v5]) {
                m_abSavedSamplerValid[v5] = false;
                --m_uiNumSavedSamplers;
                if (m_abSamplerUseMapValue[v5]) {
                    --m_uiNumUseMapValue;
                }
                m_abSamplerUseMapValue[v5] = false;
            }
            m_abSamplerValid[v5] = true;
            m_auiSamplerArray[v5] = uiValue;
            ++m_uiNumSamplers;
            if (bUseMapValue) {
                ++m_uiNumUseMapValue;
            }
            m_abSamplerUseMapValue[v5] = bUseMapValue;
        }
    }
}

// The live value wins over the saved one.
bool NiD3DTextureStageGroup::GetStageState(uint32_t uiState, uint32_t& uiValue, bool& bSave) {
    uint16_t usIndex = NiD3DRenderState::ms_auiTextureStagesStatesRevMappings[uiState];
    if (usIndex >= 8) {
        return false;
    }
    if (m_abStageValid[usIndex]) {
        uiValue = m_auiStageArray[usIndex];
        bSave = false;
        return true;
    }
    if (!m_abSavedStageValid[usIndex]) {
        return false;
    }
    uiValue = m_auiSavedStageArray[usIndex];
    bSave = true;
    return true;
}

void NiD3DTextureStageGroup::SetStageState(uint32_t uiState, uint32_t uiValue, bool bSave) {
    uint16_t v4 = NiD3DRenderState::ms_auiTextureStagesStatesRevMappings[uiState];
    if (v4 < 8u) {
        if (bSave) {
            if (m_abSavedStageValid[v4]) {
                m_auiSavedStageArray[v4] = uiValue;
            }
            else {
                if (m_abStageValid[v4]) {
                    m_abStageValid[v4] = false;
                    m_abSavedStageValid[v4] = true;
                    m_auiSavedStageArray[v4] = uiValue;
                    --m_uiNumStages;
                    ++m_uiNumSavedStages;
                }
                m_abSavedStageValid[v4] = true;
                m_auiSavedStageArray[v4] = uiValue;
                ++m_uiNumSavedStages;
            }
        }
        else {
            if (m_abStageValid[v4]) {
                m_auiStageArray[v4] = uiValue;
            }
            else {
                if (m_abSavedStageValid[v4]) {
                    m_abSavedStageValid[v4] = false;
                    m_abStageValid[v4] = true;
                    m_auiStageArray[v4] = uiValue;
                    ++m_uiNumStages;
                    --m_uiNumSavedStages;
                }
                m_abStageValid[v4] = true;
                m_auiStageArray[v4] = uiValue;
                ++m_uiNumStages;
            }
        }
    }
}

void NiD3DTextureStageGroup::RemoveAllStageStates() {
    bool* m_abStageValid = this->m_abStageValid;
    int32_t v2 = 8;
    do {
        *(m_abStageValid - 48) = false;
        *m_abStageValid++ = false;
        --v2;
    }
    while (v2);
    m_uiNumStages = 0;
    m_uiNumSavedStages = 0;
}

void NiD3DTextureStageGroup::RemoveAllSamplerStates() {
    bool* m_abSamplerValid = this->m_abSamplerValid;
    int32_t v2 = 5;
    do {
        *(m_abSamplerValid - 36) = false;
        *m_abSamplerValid = false;
        m_abSamplerValid[12] = false;
        ++m_abSamplerValid;
        --v2;
    }
    while (v2);
    m_uiNumSamplers = 0;
    m_uiNumSavedSamplers = 0;
    m_uiNumUseMapValue = 0;
}

NiD3DTextureStageGroup* NiD3DTextureStageGroup::GetFreeTextureStageGroup() {
    uint32_t* p_m_uiUsed = &NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->m_kFreeObjects.m_uiUsed;
    NiTObjectPool<NiD3DTextureStageGroup>* v1 = NiD3DTextureStageGroup::ms_pkTextureStageGroupPool;
    if (!NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->m_kFreeObjects.m_uiUsed) {
        uint32_t* p_m_uiCurrentSize = &NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->m_uiCurrentSize;
        NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->CreateNewObjects(
            NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->m_uiCurrentSize
        );
        *p_m_uiCurrentSize *= 2;
    }
    NiD3DTextureStageGroup** m_pBase = v1->m_kFreeObjects.m_pBase;
    NiD3DTextureStageGroup* result = *v1->m_kFreeObjects.m_pBase;
    *m_pBase = m_pBase[--*p_m_uiUsed];
    if (!result->m_bRendererOwned) {
        result->m_bRendererOwned = true;
    }
    return result;
}

// An empty pool of eight from the engine heap; null when the allocation fails.
void NiD3DTextureStageGroup::InitializePools() {
    NiTObjectPool<NiD3DTextureStageGroup>* pkPool = static_cast<NiTObjectPool<NiD3DTextureStageGroup>*>(
        MemoryManager::Instance().Allocate(sizeof(NiTObjectPool<NiD3DTextureStageGroup>))
    );
    if (pkPool) {
        pkPool->m_kFreeObjects.m_pBase = nullptr;
        pkPool->m_kFreeObjects.m_uiAlloced = 0;
        pkPool->m_kFreeObjects.m_uiUsed = 0;
        pkPool->m_pkContainers = nullptr;
        pkPool->m_uiCurrentSize = 8;
        pkPool->m_uiInitialSize = 8;
    }
    ms_pkTextureStageGroupPool = pkPool;
}

// A renderer-owned group has every value, flag and count cleared and goes back to the pool's free list; any other group
// is left alone.
void NiD3DTextureStageGroup::ReleaseTextureStageGroup(NiD3DTextureStageGroup* pkGroup) {
    if (!pkGroup->m_bRendererOwned) {
        return;
    }
    for (uint32_t i = 0; i < 8; ++i) {
        pkGroup->m_auiSavedStageArray[i] = 0;
        pkGroup->m_abSavedStageValid[i] = false;
        pkGroup->m_auiStageArray[i] = 0;
        pkGroup->m_abStageValid[i] = false;
    }
    for (uint32_t i = 0; i < 5; ++i) {
        pkGroup->m_auiSavedSamplerArray[i] = 0;
        pkGroup->m_abSavedSamplerValid[i] = false;
        pkGroup->m_auiSamplerArray[i] = 0;
        pkGroup->m_abSamplerValid[i] = false;
        pkGroup->m_abSamplerUseMapValue[i] = false;
    }
    pkGroup->m_uiNumStages = 0;
    pkGroup->m_uiNumSavedStages = 0;
    pkGroup->m_uiNumSamplers = 0;
    pkGroup->m_uiNumSavedSamplers = 0;
    pkGroup->m_uiNumUseMapValue = 0;
    ms_pkTextureStageGroupPool->ReleaseObject(pkGroup);
}

void NiD3DTextureStageGroup::ShutdownPools() {
    NiTObjectPool<NiD3DTextureStageGroup>* v0 = NiD3DTextureStageGroup::ms_pkTextureStageGroupPool;
    if (NiD3DTextureStageGroup::ms_pkTextureStageGroupPool) {
        NiD3DTextureStageGroup::ms_pkTextureStageGroupPool->~NiTObjectPool<NiD3DTextureStageGroup>();
        MemoryManager::Instance().Deallocate(v0);
    }
}
