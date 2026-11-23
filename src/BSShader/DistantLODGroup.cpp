#include "BSShader/DistantLODGroup.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/TallGrassTriShape.h"
#include "BSShader/TallGrassTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"

// The instance and cell key arrays start zeroed and empty; the group's geometry is a new instancing tri-strips or
// tri-shape on the cached data, with the cached alpha (blended from shader level 2 on) and stencil properties and a
// new distant LOD shader property, and an additional geometry data block holding the instance indices when the
// model data has none.
DistantLODGroup::DistantLODGroup(
    DistantLODShaderProperty::CachedGeometry* apCachedGeometry,
    uint32_t aiCellChunk,
    bool abLowDetail
)
  : spDistantLODGeometry(nullptr), spCachedGeometry(apCachedGeometry), spDistantLODProperty(nullptr),
    iMaxInstanceCount(static_cast<uint16_t>(apCachedGeometry->iMaxInstanceCount)), iInstanceCount(0),
    pInstanceData(
        static_cast<DistantLODInstanceData*>(
            MemoryManager::Instance().Allocate(iMaxInstanceCount * sizeof(DistantLODInstanceData))
        )
    ),
    piCellKeys(static_cast<uint32_t*>(MemoryManager::Instance().Allocate(iMaxInstanceCount * sizeof(uint32_t)))),
    iCellChunk(aiCellChunk) {
    for (int32_t i = 0; i < iMaxInstanceCount; ++i) {
        pInstanceData[i].fXPosNorm = 0.0f;
        pInstanceData[i].fYPosNorm = 0.0f;
        pInstanceData[i].fZPosNorm = 0.0f;
        pInstanceData[i].fScaleColor = 0.0f;
        piCellKeys[i] = 0;
    }
    UpdateGroupBound();
    NiTriBasedGeomData* pGeomData = apCachedGeometry->spTriGeomData.m_pObject;
    if (apCachedGeometry->bTristrip) {
        spDistantLODGeometry = new TallGrassTriStrips(static_cast<NiTriStripsData*>(pGeomData));
    }
    else {
        spDistantLODGeometry = new TallGrassTriShape(static_cast<NiTriShapeData*>(pGeomData));
    }
    NiAlphaProperty* pAlpha = BSShaderManager::eShaderLevel >= 2 ? apCachedGeometry->spAlphaBlend.m_pObject
                                                                 : apCachedGeometry->spAlphaNoBlend.m_pObject;
    if (pAlpha) {
        spDistantLODGeometry->AttachProperty(pAlpha);
    }
    NiStencilProperty* pStencil = apCachedGeometry->spStencil.m_pObject;
    if (pStencil) {
        spDistantLODGeometry->AttachProperty(pStencil);
    }
    DistantLODShaderProperty* pShaderProperty = new DistantLODShaderProperty;
    pShaderProperty->spBaseTexture = apCachedGeometry->spBaseTexture;
    pShaderProperty->pDistantLODGroup = this;
    // BSShaderProperty::SetFlag of the low detail flag (word 0, bit 2) with abLowDetail.
    if (abLowDetail) {
        if ((pShaderProperty->ulFlags[0] & BSS1_LowDetail) == 0) {
            pShaderProperty->iLastRenderPassState = 0;
        }
        pShaderProperty->ulFlags[0] |= BSS1_LowDetail;
    }
    else {
        if ((pShaderProperty->ulFlags[0] & BSS1_LowDetail) != 0) {
            pShaderProperty->iLastRenderPassState = 0;
        }
        pShaderProperty->ulFlags[0] &= ~BSS1_LowDetail;
    }
    // BSShaderProperty::SetFlag of flag 45 (word 1, bit 13) with bBillboard.
    if (apCachedGeometry->bBillboard) {
        if ((pShaderProperty->ulFlags[1] & BSS2_Billboard_and_Envmap_Light_Fade) == 0) {
            pShaderProperty->iLastRenderPassState = 0;
        }
        pShaderProperty->ulFlags[1] |= BSS2_Billboard_and_Envmap_Light_Fade;
    }
    else {
        if ((pShaderProperty->ulFlags[1] & BSS2_Billboard_and_Envmap_Light_Fade) != 0) {
            pShaderProperty->iLastRenderPassState = 0;
        }
        pShaderProperty->ulFlags[1] &= ~BSS2_Billboard_and_Envmap_Light_Fade;
    }
    spDistantLODGeometry->AttachProperty(pShaderProperty);
    spDistantLODProperty = pShaderProperty;
    BSShaderManager::PrepareObject(spDistantLODGeometry.m_pObject, 0, 0);
    NiGeometryData* pModelData = spDistantLODGeometry->m_spModelData.m_pObject;
    if (pModelData && pModelData->m_spAdditionalGeomData.m_pObject) {
        return;
    }
    int32_t iCount = apCachedGeometry->iVertices * apCachedGeometry->iMaxInstanceCount;
    NiAdditionalGeometryData* pAGD = new NiAdditionalGeometryData(static_cast<uint16_t>(iCount), 1);
    pAGD->SetDataBlockCount(1);
    // The instance index array is handed over as the block's raw bytes.
    pAGD->SetDataBlock(0, reinterpret_cast<uint8_t*>(apCachedGeometry->pInstanceIndex), iCount * 4, false);
    pAGD->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1, static_cast<uint16_t>(iCount), 4, 4);
    spDistantLODGeometry->m_spModelData->SetAdditionalGeometryData(pAGD);
}

// The smart pointers' own destructors run after the body, on pointers it has cleared.
DistantLODGroup::~DistantLODGroup() {
    spDistantLODProperty = nullptr;
    spCachedGeometry = nullptr;
    if (spDistantLODGeometry->m_pkParent) {
        spDistantLODGeometry->m_pkParent->DetachChild(spDistantLODGeometry.m_pObject);
    }
    spDistantLODGeometry = nullptr;
    BD6150();
    if (pInstanceData) {
        MemoryManager::Instance().Deallocate(pInstanceData);
        pInstanceData = nullptr;
    }
    MemoryManager::Instance().Deallocate(piCellKeys);
    piCellKeys = nullptr;
}

void DistantLODGroup::UpdateGroupBound() {
    float fbig = 5e+09f;
    uint16_t iInstanceCount = this->iInstanceCount;
    if (!iInstanceCount) {
        fbig = 0.0f;
    }
    float v34 = fbig;
    float v36 = fbig;
    float v37 = fbig;
    float v18 = -fbig;
    float v28 = v18;
    float v30 = v18;
    float v32 = v18;
    if (iInstanceCount) {
        DistantLODGroup::DistantLODInstanceData* pInstanceData = this->pInstanceData;
        int32_t v4 = iInstanceCount;
        do {
            if (pInstanceData->fZPosNorm > -2.1474836e+09f) {
                float fXPosNorm = pInstanceData->fXPosNorm;
                if (!(fXPosNorm > static_cast<double>(v34))) {
                    v34 = fXPosNorm;
                }
                float v20 = pInstanceData->fXPosNorm;
                if (!(v20 < static_cast<double>(v28))) {
                    v28 = v20;
                }
                float fYPosNorm = pInstanceData->fYPosNorm;
                if (!(fYPosNorm > static_cast<double>(v36))) {
                    v36 = fYPosNorm;
                }
                float v22 = pInstanceData->fYPosNorm;
                if (!(v22 < static_cast<double>(v30))) {
                    v30 = v22;
                }
                float fZPosNorm = pInstanceData->fZPosNorm;
                if (!(fZPosNorm > static_cast<double>(v37))) {
                    v37 = fZPosNorm;
                }
                float v24 = pInstanceData->fZPosNorm;
                if (!(v24 < static_cast<double>(v32))) {
                    v32 = v24;
                }
            }
            ++pInstanceData;
            --v4;
        }
        while (v4);
    }
    DistantLODShaderProperty::CachedGeometry* m_pObject = spCachedGeometry.m_pObject;
    if (m_pObject) {
        NiGeometry* v6 = spDistantLODGeometry.m_pObject;
        if (spDistantLODGeometry.m_pObject) {
            char v11;
            double v7 = v28;
            float v39 = v28 - v34;
            double v8 = v30;
            float v41 = v30 - v36;
            double v9 = v32;
            float v43 = v32 - v37;
            float v29 = v39 * 0.5;
            float v31 = v41 * 0.5;
            float v33 = 0.5 * v43;
            NiPoint3 kCenter;
            kCenter.x = v34 + v29;
            kCenter.y = v36 + v31;
            kCenter.z = v37 + v33;
            float v40 = v7 - kCenter.x;
            float v42 = v8 - kCenter.y;
            float v44 = v9 - kCenter.z;
            float v25 = v42 * v42 + v40 * v40 + v44 * v44;
            float v26 = std::sqrt(static_cast<double>(v25));
            float v45 = v26 + m_pObject->fBoundRadius;
            v11 = 0;
            for (const NiRTTI* v10 = v6->GetRTTI(); v10; v10 = v10->GetBaseRTTI()) {
                if (v10 == &TallGrassTriShape::ms_RTTI) {
                    v11 = 1;
                    break;
                }
            }
            TallGrassTriShape* ptrishape = v11 != 0 ? static_cast<TallGrassTriShape*>(v6) : nullptr;
            if (ptrishape) {
                NiBound v15;
                v15.m_kCenter = kCenter;
                v15.m_fRadius = v45;
                ptrishape->SetLocalBound(v15);
                ptrishape->SetInstanceCount(this->iInstanceCount);
            }
            else {
                if (spDistantLODGeometry.m_pObject) {
                    TallGrassTriStrips* ptristrips = static_cast<TallGrassTriStrips*>(
                        spDistantLODGeometry->DynamicCastMember(&TallGrassTriStrips::ms_RTTI)
                    );
                    if (ptristrips) {
                        NiBound v16;
                        v16.m_kCenter = kCenter;
                        v16.m_fRadius = v45;
                        ptristrips->SetLocalBound(v16);
                        ptristrips->SetInstanceCount(this->iInstanceCount);
                    }
                }
            }
        }
    }
}

void DistantLODGroup::SetDistantLODParam(DistantLODShaderProperty::DistantLODParam* apDistantLODParam) {
    DistantLODShaderProperty* m_pObject;
    if (apDistantLODParam) {
        kDistantLODParam = *apDistantLODParam;
    }
    m_pObject = spDistantLODProperty.m_pObject;
    if ((m_pObject->ulFlags[1] & BSS2_Vertex_Lighting) != 0) {
        m_pObject->iLastRenderPassState = 0;
    }
    m_pObject->ulFlags[1] &= ~BSS2_Vertex_Lighting;
    DistantLODShaderProperty* v3 = spDistantLODProperty.m_pObject;
    if ((v3->ulFlags[1] & BSS2_Uniform_Scale) != 0) {
        v3->iLastRenderPassState = 0;
    }
    v3->ulFlags[1] &= ~BSS2_Uniform_Scale;
    DistantLODShaderProperty* v4 = spDistantLODProperty.m_pObject;
    if ((v4->ulFlags[1] & BSS2_Fit_Slope) != 0) {
        v4->iLastRenderPassState = 0;
    }
    v4->ulFlags[1] &= ~BSS2_Fit_Slope;
}

bool DistantLODGroup::Add(DistantLODGroup::DistantLODInstanceData* apInstanceData, uint32_t aiCellKey) {
    uint16_t iInstanceCount;
    iInstanceCount = this->iInstanceCount;
    if ((iInstanceCount == iMaxInstanceCount) || (!apInstanceData)) {
        return false;
    }
    pInstanceData[iInstanceCount] = *apInstanceData;
    piCellKeys[this->iInstanceCount++] = aiCellKey;
    return true;
}

void DistantLODGroup::BD6150() {
    int32_t v1 = 0;
    int32_t v2 = 0;
    if (iInstanceCount) {
        do {
            pInstanceData[v1].fZPosNorm = 0.0f;
            pInstanceData[v1].fScaleColor = 0.0f;
            piCellKeys[v2++] = 0;
            ++v1;
        }
        while (v2 < iInstanceCount);
    }
    iInstanceCount = 0;
    UpdateGroupBound();
}
