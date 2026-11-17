#include "BSShader/WaterShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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

// CopyMembers still runs when the allocation failed, with a null destination, as the game does.
NiObject* WaterShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    WaterShaderProperty* pClone =
        static_cast<WaterShaderProperty*>(NiMemObject::operator new(sizeof(WaterShaderProperty)));
    if (pClone) {
        pClone = ::new (pClone) WaterShaderProperty;
    }
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
    TLSData* v8 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v8->eMemContext;
    v8->eMemContext = MEM_CONTEXT_WATER;
    if (!WaterShaderProperty::bEnabled) {
        v8->eMemContext = eMemContext;
        return nullptr;
    }
    bool bHasVertexColors = apGeom->m_spModelData.m_pObject->m_pkColor != nullptr;
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
            ambPass = 701;
        }
        else {
            if (bHasVertexColors) {
                if (bDisplacement) {
                    int16_t v14;
                    v14 = static_cast<int16_t>(
                        (static_cast<uint32_t>(v14) & 0xFFFF00FFu) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(0)) << 8)
                    );
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            v14 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v14) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v14 + 675;
                            }
                            else {
                                ambPass = 4 * v14 + 677;
                            }
                        }
                        else {
                            v14 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v14) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v14 + 676;
                            }
                            else {
                                ambPass = 4 * v14 + 678;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            v14 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v14) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v14 + 667;
                            }
                            else {
                                ambPass = 4 * v14 + 669;
                            }
                        }
                        else {
                            v14 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v14) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v14 + 668;
                            }
                            else {
                                ambPass = 4 * v14 + 670;
                            }
                        }
                    }
                    goto LABEL_76;
                }
                if (!bLOD) {
                    if (WaterShader::bInterior) {
                        int16_t v15;
                        v15 = static_cast<int16_t>(
                            (static_cast<uint32_t>(v15) & 0xFFFF00FFu) |
                            (static_cast<uint32_t>(static_cast<uint8_t>(0)) << 8)
                        );
                        if (bReflections) {
                            v15 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v15) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 8 * v15 + 706;
                            }
                            else {
                                ambPass = 8 * v15 + 708;
                            }
                        }
                        else {
                            v15 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v15) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 8 * v15 + 709;
                            }
                            else {
                                ambPass = 8 * v15 + 707;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
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
                                ambPass = bDepth ? 705 : 711;
                            }
                            else {
                                ambPass = bDepth ? 703 : 713;
                            }
                        }
                    }
                    goto LABEL_76;
                }
            }
            else {
                if (bDisplacement) {
                    int16_t v18;
                    v18 = static_cast<int16_t>(
                        (static_cast<uint32_t>(v18) & 0xFFFF00FFu) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(0)) << 8)
                    );
                    if (WaterShader::bInterior) {
                        if (bReflections) {
                            v18 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v18) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v18 + 658;
                            }
                            else {
                                ambPass = 4 * v18 + 660;
                            }
                        }
                        else {
                            v18 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v18) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v18 + 661;
                            }
                            else {
                                ambPass = 4 * v18 + 659;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
                            v18 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v18) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v18 + 650;
                            }
                            else {
                                ambPass = 4 * v18 + 652;
                            }
                        }
                        else {
                            v18 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v18) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 4 * v18 + 653;
                            }
                            else {
                                ambPass = 4 * v18 + 651;
                            }
                        }
                    }
                    goto LABEL_76;
                }
                if (!bLOD) {
                    if (WaterShader::bInterior) {
                        int16_t v19;
                        v19 = static_cast<int16_t>(
                            (static_cast<uint32_t>(v19) & 0xFFFF00FFu) |
                            (static_cast<uint32_t>(static_cast<uint8_t>(0)) << 8)
                        );
                        if (bReflections) {
                            v19 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v19) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 8 * v19 + 688;
                            }
                            else {
                                ambPass = 8 * v19 + 690;
                            }
                        }
                        else {
                            v19 = static_cast<int16_t>(
                                (static_cast<uint32_t>(v19) & 0xFFFFFF00u) |
                                (static_cast<uint32_t>(static_cast<uint8_t>((!this->bDepth))))
                            );
                            if (bRefractions) {
                                ambPass = 8 * v19 + 691;
                            }
                            else {
                                ambPass = 8 * v19 + 689;
                            }
                        }
                    }
                    else {
                        if (bReflections) {
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
                                ambPass = v20 ? 687 : 693;
                            }
                            else {
                                ambPass = v20 ? 685 : 695;
                            }
                        }
                    }
                    goto LABEL_76;
                }
            }
            ambPass = !bReflections + BSShaderManager::BSSM_WATER_LOD;
        }
    LABEL_76:;
        v12->Add(apGeom, ambPass, 1, 0, nullptr, nullptr, nullptr, nullptr);
        if (!bDisplacement && !bLOD) {
            BSShaderProperty::RenderPass* pwaterPass = GetWaterPass(apGeom);
            this->pRenderPassList->Add(
                pwaterPass->pGeometry, pwaterPass->usPassEnum, 1, 0, nullptr, nullptr, nullptr, nullptr
            );
        }
        if ((bSpecularLighting) && (LightList.m_uiCount)) {
            BSShaderProperty::RenderPassArray* v23 = this->pRenderPassList;
            if (bDisplacement) {
                if (bHasVertexColors) {
                    v23->Add(apGeom, 683, 1, 0, nullptr, nullptr, nullptr, nullptr);
                }
                else {
                    v23->Add(apGeom, 666, 1, 0, nullptr, nullptr, nullptr, nullptr);
                }
            }
            else {
                if (bHasVertexColors) {
                    v23->Add(apGeom, 718, 1, 0, nullptr, nullptr, nullptr, nullptr);
                }
                else {
                    v23->Add(apGeom, 700, 1, 0, nullptr, nullptr, nullptr, nullptr);
                }
            }
        }
    }
    BSShaderProperty::RenderPassArray* result = this->pRenderPassList;
    v8->eMemContext = eMemContext;
    return result;
}

void WaterShaderProperty::GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) {
    // Tests the height map against NiSourceTexture, then NiRenderedTexture, and calls back on every path; the second
    // test's result is unused, as the game does.
    if (spNoiseHeightMap.m_pObject) {
        if (!NiObject::IsKindOf(&NiSourceTexture::ms_RTTI, spNoiseHeightMap.m_pObject)) {
            NiObject::IsKindOf(&NiRenderedTexture::ms_RTTI, spNoiseHeightMap.m_pObject);
        }
        apTextureUseCB(spNoiseHeightMap.m_pObject, 0, apParam);
    }
    if (spNoiseNormalMap.m_pObject && spNoiseNormalMap.m_pObject->GetTexture(0)) {
        apTextureUseCB(spNoiseNormalMap.m_pObject->GetTexture(0), 0, apParam);
    }
    if (spReflectionMap.m_pObject && spReflectionMap.m_pObject->GetTexture(0)) {
        apTextureUseCB(spReflectionMap.m_pObject->GetTexture(0), 0, apParam);
    }
    if (spRefractionMap.m_pObject && spRefractionMap.m_pObject->GetTexture(0)) {
        apTextureUseCB(spRefractionMap.m_pObject->GetTexture(0), 0, apParam);
    }
    if (spDepthMap.m_pObject && spDepthMap.m_pObject->GetTexture(0)) {
        apTextureUseCB(spDepthMap.m_pObject->GetTexture(0), 0, apParam);
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
        uint16_t usPass = apGeometry->m_spModelData.m_pObject->m_pkColor ? 0x289 : 0x288;
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSShaderProperty::RenderPass));
        pWaterPass =
            pMemory ? ::new (pMemory) BSShaderProperty::RenderPass(apGeometry, usPass, true, 0, nullptr) : nullptr;
    }
    return pWaterPass;
}

void WaterShaderProperty::Copy(WaterShaderProperty* pDest) {
    FNVD_INLINED_LOCAL(NiCloningProcess, arCloneData, std::nullopt, std::nullopt);
    ::new (&arCloneData) NiCloningProcess(257u);
    CopyMembers(pDest, arCloneData);
    if (arCloneData.m_pkCloneMap) {
        delete arCloneData.m_pkCloneMap;
    }
    if (arCloneData.m_pkProcessMap) {
        delete arCloneData.m_pkProcessMap;
    }
}

NiObject* WaterShaderProperty::CreateObject() {
    WaterShaderProperty* pObject =
        static_cast<WaterShaderProperty*>(NiMemObject::operator new(sizeof(WaterShaderProperty)));
    if (pObject) {
        return ::new (pObject) WaterShaderProperty;
    }
    return nullptr;
}
