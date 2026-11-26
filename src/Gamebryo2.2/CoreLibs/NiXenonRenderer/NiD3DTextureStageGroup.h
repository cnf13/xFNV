#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"

#include <stdint.h>

class NiD3DRenderState;

// Texture stage and sampler states a texture stage applies and restores, as fixed arrays with valid flags.
class NiD3DTextureStageGroup : public NiMemObject {
public:
    // D3DTEXTURESTAGESTATETYPE of each stage state array index.
    using TextureStageTypeArray = uint16_t[8];
    // D3DSAMPLERSTATETYPE of each sampler state array index.
    using SamplerStateArray = uint32_t[5];

    NiD3DTextureStageGroup();

    uint32_t SetTextureStage(uint32_t uiStage) const;
    uint32_t SetAllStageStates(uint32_t uiStage) const;
    uint32_t SetAllStageSamplers(uint32_t uiStage) const;
    void SetSamplerState(uint32_t uiState, uint32_t uiValue, bool bSave, bool bUseMapValue);
    // The current value of uiState in uiValue, and in bSave whether it is a saved one; false when it is unset.
    bool GetStageState(uint32_t uiState, uint32_t& uiValue, bool& bSave);
    void SetStageState(uint32_t uiState, uint32_t uiValue, bool bSave);
    void RemoveAllStageStates();
    void RemoveAllSamplerStates();

    static NiD3DTextureStageGroup* GetFreeTextureStageGroup();
    static void InitializePools();
    static void ReleaseTextureStageGroup(NiD3DTextureStageGroup* pkGroup);
    static void ShutdownPools();

    bool m_bRendererOwned;
    uint32_t m_uiNumSavedStages;
    uint32_t m_uiSavedStageArrayIter;
    uint32_t m_auiSavedStageArray[8];
    bool m_abSavedStageValid[8];
    uint32_t m_uiNumStages;
    uint32_t m_uiStageArrayIter;
    uint32_t m_auiStageArray[8];
    bool m_abStageValid[8];
    uint32_t m_uiNumSavedSamplers;
    uint32_t m_uiSavedSamplerArrayIter;
    uint32_t m_auiSavedSamplerArray[5];
    bool m_abSavedSamplerValid[5];
    uint32_t m_uiNumSamplers;
    uint32_t m_uiSamplerArrayIter;
    uint32_t m_auiSamplerArray[5];
    bool m_abSamplerValid[5];
    uint32_t m_uiNumUseMapValue;
    bool m_abSamplerUseMapValue[5];

    // The render state the groups apply their states through.
    static inline NiD3DRenderState* ms_pkD3DRenderState;
    static inline NiTObjectPool<NiD3DTextureStageGroup>* ms_pkTextureStageGroupPool;
    static inline TextureStageTypeArray textureStageTypes{};
    static inline SamplerStateArray ms_aeArrayToSamplerState{};
};
