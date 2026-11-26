#include "BSShader/WaterShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/WaterShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

NiRTTI WaterShaderProperty::ms_RTTI("WaterShaderProperty", &BSShaderProperty::ms_RTTI);

WaterShaderProperty::WaterShaderProperty()
  : bDisplacement(false), bLOD(false), bFullReflections(false), bDepth(false), uwTexOffsetX(0), uwTexOffsetY(0),
    fBlendRadius(1.0f), fBlendNormalsAmount(0.0f), bIsMoving(false), bInWater(false), bUnderWater(false),
    bUpdateConstants(true), bReflections(true), bRefractions(true), bObjectTexCoords(false), bSpecularLighting(false),
    iStencilMask(0), pShallowColor(0.0f, 0.0f, 0.0f, 0.0f), pDeepColor(0.0f, 0.0f, 0.0f, 0.0f),
    pReflectionColor(0.0f, 0.0f, 0.0f, 0.0f), fTile(0.0f, 0.0f, 0.0f, 0.0f), fNoiseScale(1.0f), fFogAmount(1.0f),
    fUVScale(1365.33f), pWaterPass(nullptr) {
    pVarAmounts.x = 0.0f;
    pVarAmounts.y = 0.0f;
    pVarAmounts.z = 0.0f;
    pVarAmounts.w = 0.0f;
    pBlendRadius.x = 0.0f;
    pBlendRadius.y = 0.0f;
    pBlendRadius.z = 0.0f;
    pBlendRadius.w = 0.0f;
    pDepthFalloff.x = 0.0f;
    pDepthFalloff.y = 0.0f;
    pDepthFalloff.z = 0.0f;
    pDepthFalloff.w = 0.0f;
    pDepthOffset.x = 0.0f;
    pDepthOffset.y = 0.0f;
    pDepthOffset.z = 0.0f;
    pDepthOffset.w = 0.0f;
    pFresnelRI.x = 0.0f;
    pFresnelRI.y = 0.0f;
    pFresnelRI.z = 0.0f;
    pFresnelRI.w = 0.0f;
    iShader = 17;
    iShaderPropertyType = 14;
}

// The light list and the textures release themselves.
WaterShaderProperty::~WaterShaderProperty() {}

// CopyMembers still runs when the allocation failed, with a null destination.
NiObject* WaterShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    WaterShaderProperty* pClone = new WaterShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void WaterShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderProperty::LoadBinary(kStream);
}

void WaterShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderProperty::LinkObject(kStream);
}

bool WaterShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderProperty::RegisterStreamables(kStream);
}

void WaterShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderProperty::SaveBinary(kStream);
}

void WaterShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(WaterShaderProperty::ms_RTTI.GetName()));
}

BSShaderProperty::RenderPassArray* WaterShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    AutoMemContext kMemContext(MEM_CONTEXT_WATER);
    if (!WaterShaderProperty::bEnabled) {
        return nullptr;
    }
    bool bHasVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
    if (WaterShader::bUnderWater != bUnderWater) {
        BSShaderProperty::RenderPassArray* pRenderPassArray = this->pRenderPassList;
        if (pRenderPassArray) {
            pRenderPassArray->iSize = 0;
        }
        bUnderWater = WaterShader::bUnderWater;
    }
    CheckCreateRenderPassArray(-1);
    BSShaderProperty::RenderPassArray* v12 = this->pRenderPassList;
    if (!v12->iSize) {
        uint16_t ambPass;
        if (bUnderWater) {
            ambPass = BSShaderManager::BSSM_WATER_UNDERWATER;
        }
        else {
            if (bHasVertexColors) {
                if (bDisplacement) {
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_Vc
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_Vc;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS_Vc
                                    : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS_Vc;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_Vc
                                    : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_Vc;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc
                                    : BSShaderManager::
                                          BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING_Vc
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_Vc;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING_NO_REFRACTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS_Vc;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_Vc;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS_Vc
                                    : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc;
                            }
                        }
                    }
                }
                else if (!bLOD) {
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_Vc
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_Vc;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFRACTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS_Vc;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_Vc;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS_Vc
                                    : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            // BSSM_WATER_Vc and BSSM_WATER_NO_REFRACTIONS_Vc, or their NODEPTH passes eight further on.
                            int32_t v17 = !this->bDepth;
                            if (bRefractions) {
                                ambPass = 8 * v17 + 702;
                            }
                            else {
                                ambPass = 8 * v17 + 704;
                            }
                        }
                        else {
                            int32_t bDepth = this->bDepth;
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_NO_REFLECTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_Vc;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS_Vc
                                                 : BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS_Vc;
                            }
                        }
                    }
                }
                else {
                    ambPass = !bReflections + BSShaderManager::BSSM_WATER_LOD;
                }
            }
            else {
                if (bDisplacement) {
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_WADING
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFRACTIONS
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFRACTIONS;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_WADING_NO_REFLECTIONS_NO_REFRACTIONS
                                    : BSShaderManager::
                                          BSSM_WATER_INTERIOR_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING_NO_REFRACTIONS
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFRACTIONS;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS
                                                 : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_WADING_NO_REFLECTIONS_NO_REFRACTIONS
                                    : BSShaderManager::BSSM_WATER_WADING_NO_DEPTH_NO_REFLECTIONS_NO_REFRACTIONS;
                            }
                        }
                    }
                }
                else if (!bLOD) {
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH;
                            }
                            else {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFRACTIONS
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFRACTIONS;
                            }
                        }
                        else {
                            if (bRefractions) {
                                ambPass = bDepth ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS
                                                 : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS;
                            }
                            else {
                                ambPass = bDepth
                                    ? BSShaderManager::BSSM_WATER_INTERIOR_NO_REFLECTIONS_NO_REFRACTIONS
                                    : BSShaderManager::BSSM_WATER_INTERIOR_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            // BSSM_WATER and BSSM_WATER_NO_REFRACTIONS, or their NODEPTH passes eight further on.
                            int32_t v21 = !this->bDepth;
                            if (bRefractions) {
                                ambPass = 8 * v21 + 684;
                            }
                            else {
                                ambPass = 8 * v21 + 686;
                            }
                        }
                        else {
                            int32_t v20 = this->bDepth;
                            if (bRefractions) {
                                ambPass = v20 ? BSShaderManager::BSSM_WATER_NO_REFLECTIONS
                                              : BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS;
                            }
                            else {
                                ambPass = v20 ? BSShaderManager::BSSM_WATER_NO_REFLECTIONS_NO_REFRACTIONS
                                              : BSShaderManager::BSSM_WATER_NODEPTH_NO_REFLECTIONS_NO_REFRACTIONS;
                            }
                        }
                    }
                }
                else {
                    ambPass = !bReflections + BSShaderManager::BSSM_WATER_LOD;
                }
            }
        }
        v12->Add(apGeom, ambPass, 1, 0, nullptr, nullptr, nullptr, nullptr);
        if (!bDisplacement && !bLOD) {
            BSShaderProperty::RenderPass* pwaterPass = GetWaterPass(apGeom);
            this->pRenderPassList
                ->Add(pwaterPass->pGeometry, pwaterPass->usPassEnum, 1, 0, nullptr, nullptr, nullptr, nullptr);
        }
        if (bSpecularLighting && LightList.m_uiCount) {
            BSShaderProperty::RenderPassArray* v23 = this->pRenderPassList;
            if (bDisplacement) {
                if (bHasVertexColors) {
                    v23->Add(
                        apGeom,
                        BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc,
                        1,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
                else {
                    v23->Add(
                        apGeom,
                        BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING,
                        1,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
            }
            else {
                if (bHasVertexColors) {
                    v23->Add(
                        apGeom,
                        BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING_Vc,
                        1,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
                else {
                    v23->Add(
                        apGeom,
                        BSShaderManager::BSSM_WATER_SPECULAR_LIGHTING,
                        1,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
            }
        }
    }
    BSShaderProperty::RenderPassArray* result = this->pRenderPassList;
    return result;
}

void WaterShaderProperty::GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) {
    // Tests the height map against NiSourceTexture, then NiRenderedTexture, and calls back on every path; the second
    // test's result is unused.
    if (spNoiseHeightMap.m_pObject) {
        if (!NiObject::IsKindOf(&NiSourceTexture::ms_RTTI, spNoiseHeightMap.m_pObject)) {
            NiObject::IsKindOf(&NiRenderedTexture::ms_RTTI, spNoiseHeightMap.m_pObject);
        }
        apTextureUseCB(spNoiseHeightMap.m_pObject, 0, apParam);
    }
    if (spNoiseNormalMap.m_pObject && spNoiseNormalMap->GetTexture(0)) {
        apTextureUseCB(spNoiseNormalMap->GetTexture(0), 0, apParam);
    }
    if (spReflectionMap.m_pObject && spReflectionMap->GetTexture(0)) {
        apTextureUseCB(spReflectionMap->GetTexture(0), 0, apParam);
    }
    if (spRefractionMap.m_pObject && spRefractionMap->GetTexture(0)) {
        apTextureUseCB(spRefractionMap->GetTexture(0), 0, apParam);
    }
    if (spDepthMap.m_pObject && spDepthMap->GetTexture(0)) {
        apTextureUseCB(spDepthMap->GetTexture(0), 0, apParam);
    }
}

void WaterShaderProperty::CopyMembers(WaterShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderProperty::CopyMembers(pDest, arCloneData);
    pDest->spDisplacementNormalMap = spDisplacementNormalMap;
    pDest->bLOD = bLOD;
    pDest->bFullReflections = bFullReflections;
    pDest->bDepth = bDepth;
    pDest->uwTexOffsetX = uwTexOffsetX;
    pDest->uwTexOffsetY = uwTexOffsetY;
    pDest->fBlendRadius = fBlendRadius;
    pDest->fBlendNormalsAmount = fBlendNormalsAmount;
    pDest->bInWater = bInWater;
    pDest->bIsMoving = bIsMoving;
    pDest->bUpdateConstants = bUpdateConstants;
    pDest->bReflections = bReflections;
    pDest->bRefractions = bRefractions;
    pDest->pShallowColor = pShallowColor;
    pDest->pDeepColor = pDeepColor;
    pDest->pReflectionColor = pReflectionColor;
    pDest->pVarAmounts = pVarAmounts;
    pDest->pBlendRadius = pBlendRadius;
    pDest->pDepthFalloff = pDepthFalloff;
    pDest->pDepthOffset = pDepthOffset;
    pDest->pFresnelRI = pFresnelRI;
    pDest->fTile = fTile;
    pDest->fFresnelAmount = fFresnelAmount;
    pDest->fUVScale = fUVScale;
    pDest->fNoiseScale = fNoiseScale;
    pDest->fFogAmount = fFogAmount;
    pDest->fFogFar = fFogFar;
    pDest->fFogRange = fFogRange;
    pDest->bObjectTexCoords = bObjectTexCoords;
    pDest->spReflectionMap = spReflectionMap;
    pDest->spRefractionMap = spRefractionMap;
    pDest->spNoiseNormalMap = spNoiseNormalMap;
    pDest->spDepthMap = spDepthMap;
    pDest->iStencilMask = iStencilMask;
    pDest->bSpecularLighting = bSpecularLighting;
    pDest->LightList.RemoveAll();
    NiTListItem<NiLight*>* pkPos = LightList.m_pkHead;
    while (pkPos) {
        NiLight* pLight = pkPos->m_element;
        pkPos = pkPos->m_pkNext;
        if (pLight) {
            // The list's AddTail with the pool allocator's Allocate: a cleared node from the engine's pool, linked in
            // at the tail.
            NiTAbstractPoolAllocator<uint32_t>::Lock();
            NiTAbstractPoolAllocator<uint32_t>::AllocNode* pkFree = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
            if (!NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem) {
                NiTAbstractPoolAllocator<uint32_t>::CreateFreeMem();
                pkFree = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
            }
            NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkFree->m_pkNext;
            pkFree->m_element = 0;
            pkFree->m_pkNext = nullptr;
            pkFree->m_pkData = nullptr;
            NiTAbstractPoolAllocator<uint32_t>::Unlock();
            // Gamebryo's NewItem casts the allocator's void* node to the list's item.
            NiTListItem<NiLight*>* pkNode = static_cast<NiTListItem<NiLight*>*>(static_cast<void*>(pkFree));
            pkNode->m_element = pLight;
            pkNode->m_pkNext = nullptr;
            pkNode->m_pkPrev = pDest->LightList.m_pkTail;
            if (pDest->LightList.m_pkTail) {
                pDest->LightList.m_pkTail->m_pkNext = pkNode;
            }
            else {
                pDest->LightList.m_pkHead = pkNode;
            }
            ++pDest->LightList.m_uiCount;
            pDest->LightList.m_pkTail = pkNode;
        }
    }
}

BSShaderProperty::RenderPass* WaterShaderProperty::GetWaterPass(NiGeometry* apGeometry) {
    if (!pWaterPass) {
        // Geometry with vertex colors gets pass 0x289, other geometry 0x288.
        uint16_t usPass = apGeometry->m_spModelData->m_pkColor ? BSShaderManager::BSSM_WATER_STENCIL_Vc
                                                               : BSShaderManager::BSSM_WATER_STENCIL;
        pWaterPass = new BSShaderProperty::RenderPass(apGeometry, usPass, true, 0, nullptr);
    }
    return pWaterPass;
}

void WaterShaderProperty::Copy(WaterShaderProperty* pDest) {
    NiCloningProcess arCloneData(257u);
    CopyMembers(pDest, arCloneData);
}

NiObject* WaterShaderProperty::CreateObject() {
    return new WaterShaderProperty;
}
