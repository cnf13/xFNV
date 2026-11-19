#include "BSShader/HairShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI HairShaderProperty::ms_RTTI("HairShaderProperty", &BSShaderPPLightingProperty::ms_RTTI);

HairShaderProperty::HairShaderProperty()
  : spPrimaryLight(nullptr), spPreviousPrimaryLight(nullptr), pHairTintColor(0.0f, 0.0f, 0.0f, 0.0f),
    spHeightMap(nullptr) {
    for (uint32_t i = 0; i < 3; ++i) {
        pHairLightColor[i] = NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
        pHairLightPos[i] = NiPoint3_ZERO;
        fHairLightRadius[i] = 0.0f;
    }
    iShaderPropertyType = 9;
}

// Clears the hair lights; the NiPointer members release the height map and both lights themselves.
HairShaderProperty::~HairShaderProperty() {
    for (uint32_t i = 0; i < 3; ++i) {
        pHairLightPos[i] = NiPoint3_ZERO;
        fHairLightRadius[i] = 0.0f;
    }
}

// The clone is built while the thread's heap context is MEM_CONTEXT_HAIRSHADER, covering the allocation and
// CopyMembers.
NiObject* HairShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext memContext(MEM_CONTEXT_HAIRSHADER);
    HairShaderProperty* pClone = new HairShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void HairShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::LoadBinary(kStream);
}

void HairShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderPPLightingProperty::LinkObject(kStream);
}

bool HairShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderPPLightingProperty::RegisterStreamables(kStream);
}

void HairShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::SaveBinary(kStream);
}

void HairShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderPPLightingProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(HairShaderProperty::ms_RTTI.GetName()));
    apStrings->Add(NiGetViewerString("height map", static_cast<const void*>(spHeightMap.m_pObject)));
    apStrings->Add(NiGetViewerString("primary light is point", (ulFlags[1] & 0x80) != 0));
    apStrings->Add(NiGetViewerString("second light", (ulFlags[1] & 0x100) != 0));
    apStrings->Add(NiGetViewerString("third light", (ulFlags[1] & 0x200) != 0));
}

bool HairShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
    bool bResult = BSShaderPPLightingProperty::SetupGeometry(apGeometry);
    NiProperty* pProperty = apGeometry->GetProperty(NiProperty::VERTEX_COLOR);
    if (pProperty) {
        apGeometry->DetachProperty(pProperty);
    }
    return bResult;
}

BSShaderProperty::RenderPassArray* HairShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    RenderPassArray* pResult;
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eOldMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAIRSHADER;
    uint32_t uiFlags = ulFlags[0];
    if ((uiFlags & BSS1_Refraction) != 0 || (uiFlags & BSS1_Fire_Refraction) != 0) {
        pResult = BSShaderPPLightingProperty::GetRenderPasses(
            apGeom, aiEnabledPasses, asCount, auiRenderMode, apAccumulator, abUpdateList
        );
    }
    else {
        uint32_t uiRenderPassState = aiEnabledPasses | auiRenderMode << 8;
        if (auiRenderMode == 7) {
            pResult = pDepthMapRenderPassList;
        }
        else if (auiRenderMode == 8) {
            pResult = pConstAlphaRenderPassList;
        }
        else if (auiRenderMode == 9) {
            AddLocalMapPass(apGeom, apGeom->m_spSkinInstance.m_pObject != nullptr, false);
            pResult = pLocalMapRenderPassList;
        }
        else {
            if (iLastRenderPassState != uiRenderPassState || !static_cast<uint8_t>(abUpdateList)) {
                if (static_cast<uint8_t>(abUpdateList) == 1) {
                    CheckCreateRenderPassArray(-1);
                    pRenderPassList->iSize = 0;
                    iLastRenderPassState = uiRenderPassState;
                }
                else {
                    asCount = 0;
                }
                ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
                if (!GetNumberofActiveNonShadowLights()) {
                    if (static_cast<uint8_t>(abUpdateList) == 1) {
                        pRenderPassList->Add(apGeom, 0xC1, 1, 1, pSunLight, nullptr, nullptr, nullptr);
                        if ((ulFlags[1] & BSS2_1st_Light_is_Point_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_1st_Light_is_Point_Light;
                        if ((ulFlags[1] & BSS2_2nd_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_2nd_Light;
                        if ((ulFlags[1] & BSS2_3rd_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_3rd_Light;
                    }
                    else {
                        ++asCount;
                    }
                }
                else {
                    void* pIterator = nullptr;
                    ShadowSceneLight* pPointLight = GetFirstActiveNonShadowLight(pIterator);
                    NiColor kDirColor = pSunLight->spLight.m_pObject->m_kDiff;
                    float fDirLuminance = kDirColor.Luminance();
                    const NiBound* pWorldBound = apGeom->m_kWorldBound;
                    if (!pWorldBound) {
                        pWorldBound = &NiAVObject::NullBoundS;
                    }
                    NiBound kBound = *pWorldBound;
                    NiLight* pLight = pPointLight->spLight.m_pObject;
                    NiPoint3 kToLight;
                    kToLight.x = pLight->m_kWorld.m_Translate.x - kBound.m_kCenter.x;
                    kToLight.y = pLight->m_kWorld.m_Translate.y - kBound.m_kCenter.y;
                    kToLight.z = pLight->m_kWorld.m_Translate.z - kBound.m_kCenter.z;
                    float fEdgeDist = kToLight.Length() - kBound.m_fRadius;
                    float fDist = BSMAX(0.0f, fEdgeDist);
                    NiColor kPointColor = pLight->m_kDiff;
                    float fPointLuminance = kPointColor.Luminance() * (fDist * (1.0f / fDist));
                    ShadowSceneLight* pLight0;
                    ShadowSceneLight* pLight1;
                    ShadowSceneLight* pLight2;
                    if (fPointLuminance < fDirLuminance) {
                        // The sun first, then the point lights.
                        pLight2 = nullptr;
                        pLight0 = pSunLight;
                        if ((ulFlags[1] & BSS2_1st_Light_is_Point_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_1st_Light_is_Point_Light;
                        if ((ulFlags[1] & BSS2_2nd_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_2nd_Light;
                        if ((ulFlags[1] & BSS2_3rd_Light) != 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] &= ~BSS2_3rd_Light;
                        pLight1 = pPointLight;
                        if ((ulFlags[1] & BSS2_2nd_Light) == 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] |= BSS2_2nd_Light;
                        ShadowSceneLight* pNextLight = GetNextActiveNonShadowLight(pIterator);
                        if (pNextLight) {
                            pLight2 = pNextLight;
                            if ((ulFlags[1] & BSS2_3rd_Light) == 0) {
                                iLastRenderPassState = 0;
                            }
                            ulFlags[1] |= BSS2_3rd_Light;
                        }
                    }
                    else {
                        // The brightest point light first.
                        pLight0 = pPointLight;
                        pLight1 = pSunLight;
                        if ((ulFlags[1] & BSS2_1st_Light_is_Point_Light) == 0) {
                            iLastRenderPassState = 0;
                        }
                        ulFlags[1] |= BSS2_1st_Light_is_Point_Light;
                        if (0.0f < fDirLuminance) {
                            ShadowSceneLight* pNextLight = GetNextActiveNonShadowLight(pIterator);
                            if (pNextLight) {
                                pLight2 = pNextLight;
                                if ((ulFlags[1] & BSS2_2nd_Light) == 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] |= BSS2_2nd_Light;
                                if ((ulFlags[1] & BSS2_3rd_Light) == 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] |= BSS2_3rd_Light;
                            }
                            else {
                                pLight2 = nullptr;
                                if ((ulFlags[1] & BSS2_2nd_Light) == 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] |= BSS2_2nd_Light;
                                if ((ulFlags[1] & BSS2_3rd_Light) != 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] &= ~BSS2_3rd_Light;
                            }
                        }
                        else {
                            // No sun: the next two point lights follow the first.
                            ShadowSceneLight* pNextLight = GetNextActiveNonShadowLight(pIterator);
                            if (pNextLight) {
                                pLight1 = pNextLight;
                                pLight2 = GetNextActiveNonShadowLight(pIterator);
                                if ((ulFlags[1] & BSS2_2nd_Light) == 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] |= BSS2_2nd_Light;
                                SetFlag(HSP_FLAG_3RDLIGHT, pLight2 != nullptr);
                            }
                            else {
                                pLight1 = nullptr;
                                pLight2 = nullptr;
                                if ((ulFlags[1] & BSS2_2nd_Light) != 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] &= ~BSS2_2nd_Light;
                                if ((ulFlags[1] & BSS2_3rd_Light) != 0) {
                                    iLastRenderPassState = 0;
                                }
                                ulFlags[1] &= ~BSS2_3rd_Light;
                            }
                        }
                    }
                    if (static_cast<uint8_t>(abUpdateList) == 1) {
                        pRenderPassList->Add(apGeom, 0xC2, 1, 3, pLight0, pLight1, pLight2, nullptr);
                    }
                    else {
                        ++asCount;
                    }
                }
                if (BSShaderManager::GetShadows() && static_cast<uint8_t>(abUpdateList)) {
                    NiAlphaProperty* pAlphaProperty =
                        static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiAlphaProperty::GetType()));
                    if (!pAlphaProperty) {
                        pAlphaProperty = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
                    }
                    bool bAlphaTest =
                        pAlphaProperty && (pAlphaProperty->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK) != 0;
                    AddDepthPass(apGeom, apGeom->m_spSkinInstance.m_pObject != nullptr, bAlphaTest, apAccumulator);
                }
            }
            if (spTexEffectData.m_pObject) {
                bool bFirst = true;
                AddTextureEffectPass_1x(apGeom, asCount, abUpdateList, bFirst, false);
            }
            pResult = pRenderPassList;
        }
    }
    pTLSData->eMemContext = eOldMemContext;
    return pResult;
}

// Copies this property's members into pDest.
void HairShaderProperty::CopyMembers(HairShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderPPLightingProperty::CopyMembers(pDest, arCloneData);
    pDest->spHeightMap = spHeightMap;
    pDest->HairShaderProperty::pHairTintColor = this->HairShaderProperty::pHairTintColor;
}

// The stream's factory: a property from a 0x180-byte engine block, or null when the block could not be had.
NiObject* HairShaderProperty::CreateObject() {
    return new HairShaderProperty;
}
