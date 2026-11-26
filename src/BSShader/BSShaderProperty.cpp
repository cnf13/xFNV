#include "BSShader/BSShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstdarg>

NiRTTI BSShaderProperty::ms_RTTI("BSShaderProperty", &NiShadeProperty::ms_RTTI);

// One name per shader flag, in flag bit order.
static const char* const kShaderFlagNames[BSShaderProperty::BSSP_FLAG_COUNT] = {
    "specular",
    "skinned",
    "lowdetail",
    "vertex alpha",
    "motion blur",
    "single pass",
    "EMPTY",
    "environment mapping",
    "alpha texture",
    "z prepass",
    "facegen",
    "parallax",
    "model-space normals",
    "non-projective shadows",
    "landscape",
    "refraction",
    "fire refraction",
    "eye environment mapping",
    "hair",
    "dynamic alpha",
    "localmap hide secret",
    "window environment mapping",
    "tree billboard",
    "shadow frustum",
    "multiple textures",
    "remappable textures",
    "decal",
    "dynamic decal",
    "parallax occlusion",
    "external emittance",
    "shadowmap",
    "zbuffer test",
    "zbuffer write",
    "lod landscape",
    "lod building",
    "no fade",
    "refraction tint",
    "vertex colors",
    "1st person",
    "1st light is point light",
    "2nd light",
    "3rd light",
    "vertex lighting",
    "uniform scale",
    "fit slope",
    "billboard",
    "no lod land blend",
    "envmap light fade",
    "wireframe",
    "vats selection",
    "show in local map",
    "premult alpha",
    "skip normal maps",
    "alpha decal",
    "no transparency multisampling",
    "stinger prop",
};

// The flag's byte is stored as it is; the aucNumLights scene lights follow, copied into a new ppSceneLights.
BSShaderProperty::RenderPass::RenderPass(
    NiGeometry* apGeometry,
    int32_t aePassEnum,
    int32_t abFirstPass,
    int32_t aucNumLights,
    ...
)
  : pGeometry(apGeometry), usPassEnum(static_cast<uint16_t>(aePassEnum)), eAccumulationHint(BSSP_HINT_NORMAL),
    bFirstPass(static_cast<uint8_t>(abFirstPass)), bLastPass(false), ucNumLights(static_cast<uint8_t>(aucNumLights)),
    ucMaxNumLights(0), ucExtraParam(0), ppSceneLights(nullptr) {
    if (ucNumLights) {
        ppSceneLights = static_cast<ShadowSceneLight**>(
            MemoryManager::Instance().Allocate(sizeof(ShadowSceneLight*) * ucNumLights)
        );
        ucMaxNumLights = ucNumLights;
        va_list vList;
        va_start(vList, aucNumLights);
        for (int32_t i = 0; i < ucNumLights; ++i) {
            ppSceneLights[i] = va_arg(vList, ShadowSceneLight*);
        }
        va_end(vList);
    }
}

BSShaderProperty::RenderPass::~RenderPass() {
    usPassEnum = BSShaderManager::BSSM_EMPTY;
    if (ppSceneLights) {
        MemoryManager::Instance().Deallocate(ppSceneLights);
        ppSceneLights = nullptr;
    }
    ucExtraParam = 0;
}

// The count is the argument's low byte, the allocation holds count pointers, and the lights are apSceneLight and the
// arguments after it.
void BSShaderProperty::RenderPass::SetLights(int32_t ausNumLights, ShadowSceneLight* apSceneLight, ...) {
    const uint8_t ucCount = static_cast<uint8_t>(ausNumLights);
    if (ucCount > ucMaxNumLights) {
        if (ppSceneLights) {
            MemoryManager::Instance().Deallocate(ppSceneLights);
            ppSceneLights = nullptr;
        }
        ppSceneLights = static_cast<ShadowSceneLight**>(
            MemoryManager::Instance().Allocate(sizeof(ShadowSceneLight*) * ucCount)
        );
        ucMaxNumLights = ucCount;
    }
    ucNumLights = ucCount;
    va_list kLights;
    va_start(kLights, apSceneLight);
    for (int32_t i = 0; i < static_cast<int32_t>(ucCount); ++i) {
        ppSceneLights[i] = i == 0 ? apSceneLight : va_arg(kLights, ShadowSceneLight*);
    }
    va_end(kLights);
}

void BSShaderProperty::RenderPass::ClearSceneLights() {
    if (ppSceneLights) {
        MemoryManager::Instance().Deallocate(ppSceneLights);
        ppSceneLights = nullptr;
    }
}

void BSShaderProperty::RenderPass::AllocateLightArray(uint8_t aucNumLights) {
    ppSceneLights = static_cast<ShadowSceneLight**>(MemoryManager::Instance().Allocate(
        aucNumLights <= SIZE_MAX / sizeof(ShadowSceneLight*) ? sizeof(ShadowSceneLight*) * aucNumLights : SIZE_MAX
    ));
}

BSShaderProperty::RenderPassArray::RenderPassArray(uint32_t auiNumPasses) : pPassA(auiNumPasses, 3u), iSize(0) {
    pPassA.SetSize(auiNumPasses);
}

// Every pass in the array's slots, in use or kept for reuse, is destroyed; pPassA frees its buffer itself.
BSShaderProperty::RenderPassArray::~RenderPassArray() {
    for (uint32_t i = 0; i < pPassA.m_usESize; ++i) {
        delete pPassA.m_pBase[i];
    }
    iSize = 0xFFDEAD;
}

// A free slot is reused (its lights reset, its fields rewritten); otherwise a new pass is appended. The pass enum,
// first-pass flag and light count go on to SetLights and the constructor as the caller's whole values; a reused pass
// stores only their low word and byte, the flag's byte as it is.
void BSShaderProperty::RenderPassArray::Add(
    NiGeometry* apGeometry,
    int32_t ausPassEnum,
    int32_t abFirstPass,
    int32_t ausNumLights,
    ShadowSceneLight* apSceneLight,
    ShadowSceneLight* apSceneLight1,
    ShadowSceneLight* apSceneLight2,
    ShadowSceneLight* apSceneLight3
) {
    if (iSize < pPassA.m_usESize) {
        RenderPass* pPass = pPassA.m_pBase[iSize];
        pPass->SetLights(ausNumLights, apSceneLight, apSceneLight1, apSceneLight2, apSceneLight3);
        pPass->usPassEnum = static_cast<uint16_t>(ausPassEnum);
        pPass->bFirstPass = static_cast<uint8_t>(abFirstPass);
        pPass->pGeometry = apGeometry;
        pPass->bLastPass = false;
        pPass->eAccumulationHint = BSSP_HINT_NORMAL;
        pPass->ucExtraParam = 0;
    }
    else {
        RenderPass* pPass = new RenderPass(
            apGeometry,
            ausPassEnum,
            abFirstPass,
            ausNumLights,
            apSceneLight,
            apSceneLight1,
            apSceneLight2,
            apSceneLight3
        );
        pPassA.Add(pPass);
    }
    ++iSize;
}

void BSShaderProperty::RenderPassArray::Clear() {
    iSize = 0;
}

void BSShaderProperty::PreviousTransform::SetFreeze(bool abFreeze) {
    BSShaderProperty::PreviousTransform::bFrozen = abFreeze;
}

void BSShaderProperty::PreviousTransform::ToggleConsole() {
    BSShaderProperty::PreviousTransform::bConsole = !BSShaderProperty::PreviousTransform::bConsole;
}

void BSShaderProperty::PreviousTransform::SaveCamera(NiCamera* apCamera) {
    if (!BSShaderProperty::PreviousTransform::bFrozen && !BSShaderProperty::PreviousTransform::bConsole) {
        BSShaderProperty::PreviousTransform::matView = BSShaderManager::pRenderer->m_kD3DView;
        BSShaderProperty::PreviousTransform::matProj = BSShaderManager::pRenderer->m_kD3DProj;
        BSShaderProperty::PreviousTransform::bViewProjReady = true;
        BSShaderProperty::PreviousTransform::bDirty = false;
        float v1 = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
        float v2 = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
        BSShaderProperty::PreviousTransform::vecPrevCameraDir.x = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
        BSShaderProperty::PreviousTransform::vecPrevCameraDir.y = v1;
        BSShaderProperty::PreviousTransform::vecPrevCameraDir.z = v2;
    }
}

// The shade property's flags (1) and type (-1) come from NiShadeProperty's constructor.
BSShaderProperty::BSShaderProperty()
  : ulFlags{0, 0}, fAlpha(1.0f), fFadeAlpha(1.0f), fEnvmapScale(1.0f), fCameraDistance(0.0f), iLastRenderPassState(0),
    pRenderPassList(nullptr), pDepthMapRenderPassList(nullptr), pConstAlphaRenderPassList(nullptr),
    pLocalMapRenderPassList(nullptr), pSIBlockRenderPassList(nullptr), pWaterFogRenderPassList(nullptr), iShader(0),
    fDepthBias(0.0f) {}

BSShaderProperty::~BSShaderProperty() {
    ulFlags[0] = 0;
    ulFlags[1] = 0;
    FreeRenderPasses();
}

NiObject* BSShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSShaderProperty* v4;
    double fFadeAlpha;
    v4 = new BSShaderProperty;
    NiShadeProperty::CopyMembers(v4, arCloneData);
    v4->ulFlags[0] = ulFlags[0];
    v4->ulFlags[1] = ulFlags[1];
    v4->fAlpha = fAlpha;
    fFadeAlpha = this->fFadeAlpha;
    v4->iLastRenderPassState = 0;
    v4->fFadeAlpha = static_cast<float>(fFadeAlpha);
    v4->iShader = iShader;
    v4->fDepthBias = fDepthBias;
    return v4;
}

void BSShaderProperty::LoadBinary(NiStream& kStream) {
    NiShadeProperty::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, iShader);
    for (uint32_t i = 0; i < 2; ++i) {
        NiStreamLoadBinary(kStream, ulFlags[i]);
    }

    // Files that carry flag 0x2D (bit 13 of the second word) get its companion flag 0x2F (bit 15) set.
    if (ulFlags[1] & BSS2_Billboard_and_Envmap_Light_Fade) {
        if (!(ulFlags[1] & BSS2_Envmap_Light_Fade)) {
            iLastRenderPassState = 0;
        }
        ulFlags[1] |= BSS2_Envmap_Light_Fade;
    }
    NiStreamLoadBinary(kStream, fEnvmapScale);
}

void BSShaderProperty::LinkObject(NiStream& kStream) {
    NiShadeProperty::LinkObject(kStream);
}

bool BSShaderProperty::RegisterStreamables(NiStream& kStream) {
    return NiShadeProperty::RegisterStreamables(kStream);
}

void BSShaderProperty::SaveBinary(NiStream& kStream) {
    NiShadeProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, iShader);
    for (const uint32_t& rulFlags : ulFlags) {
        NiStreamSaveBinary(kStream, rulFlags);
    }
    NiStreamSaveBinary(kStream, fEnvmapScale);
}

void BSShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiShadeProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));

    for (uint32_t i = 0; i < BSSP_FLAG_COUNT; ++i) {
        if ((ulFlags[i >> 5] & (1u << (i & 31))) && kShaderFlagNames[i]) {
            apStrings->Add(NiGetViewerString(kShaderFlagNames[i], true));
        }
    }

    if (fAlpha < 1.0f) {
        apStrings->Add(NiGetViewerString("fAlpha", fAlpha));
    }
    if (fFadeAlpha < 1.0f) {
        apStrings->Add(NiGetViewerString("fFadeAlpha", fFadeAlpha));
    }
    if (fDepthBias > 0.0f) {
        apStrings->Add(NiGetViewerString("fDepthBias", fDepthBias));
    }
    if (ulFlags[0] & BSS1_Environment_Mapping) {
        apStrings->Add(NiGetViewerString("Envmap Scale", fEnvmapScale));
    }

    apStrings->Add(NiGetViewerString("Shader Index", static_cast<int32_t>(iShader)));
    apStrings->Add(NiGetViewerString("iLastRenderPassState", static_cast<int32_t>(iLastRenderPassState)));

    if (!pRenderPassList) {
        apStrings->Add(NiGetViewerString("pRenderPassList not found"));
        return;
    }

    const uint32_t uiCount = pRenderPassList->iSize;
    apStrings->Add(NiGetViewerString("number of passes", uiCount));

    // The list pointer and its slot array are read again on every iteration.
    for (uint32_t i = 0; i < uiCount; ++i) {
        RenderPass* pPass = pRenderPassList->pPassA.m_pBase[i];
        if (pPass->usPassEnum < BSShaderManager::BSSM_FINAL_PASS) {
            apStrings->Add(NiGetViewerString(
                pPass->bFirstPass ? " Fpass" : "  pass",
                BSShaderManager::GetPassName(pPass->usPassEnum)
            ));
        }
    }
}

void BSShaderProperty::PostLinkObject(NiStream& kStream) {
    NiShadeProperty::PostLinkObject(kStream);
    if (ulFlags[0] & BSS1_Decal_Single_Pass) {
        fDepthBias = 1.0f;
    }
}

// Calls CopyToMembers through the vtable.
void BSShaderProperty::CopyTo(BSShaderProperty* pDest) {
    CopyToMembers(pDest);
}

void BSShaderProperty::CopyToMembers(BSShaderProperty* pDest) {
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    pDest->fAlpha = fAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;

    if (m_spControllers) {
        pDest->m_spControllers = m_spControllers;
        pDest->m_spControllers->SetTarget(pDest);
    }
}

void BSShaderProperty::SetDepthBias(float afDepthBias) {
    fDepthBias = afDepthBias;
}

void BSShaderProperty::SetShader(int32_t aiShader) {
    iShader = aiShader;
}

void BSShaderProperty::SetFlag(uint32_t aiFlag, int32_t abSetFlag) {
    const uint32_t uiWord = aiFlag >> 5;
    const uint32_t uiMask = 1u << (aiFlag % 32);
    const bool bWasSet = (ulFlags[uiWord] & uiMask) != 0;

    // Any actual change invalidates the cached render pass state. Only the flag's low byte is tested.
    if (static_cast<uint8_t>(abSetFlag)) {
        if (!bWasSet) {
            iLastRenderPassState = 0;
        }
        ulFlags[uiWord] |= uiMask;
    }
    else {
        if (bWasSet) {
            iLastRenderPassState = 0;
        }
        ulFlags[uiWord] &= ~uiMask;
    }
}

void BSShaderProperty::SetAlpha(float afAlpha) {
    // Crossing the opaque threshold (alpha 1.0) changes which render passes apply.
    if (fAlpha >= 1.0f) {
        if (afAlpha < 1.0f) {
            iLastRenderPassState = 0;
        }
    }
    else if ((fAlpha < 1.0f) && (afAlpha >= 1.0f)) {
        iLastRenderPassState = 0;
    }

    fAlpha = afAlpha;
}

bool BSShaderProperty::GetFlag(uint32_t aiFlag) const {
    return (ulFlags[aiFlag >> 5] & (1u << (aiFlag & 31))) != 0;
}

void BSShaderProperty::AddDepthPass(
    NiGeometry* apGeometry,
    bool abSkinned,
    bool abAlphaTest,
    BSShaderAccumulator* apAccumulator
) {
    uint32_t uiPass;
    if (abSkinned) {
        uiPass = abAlphaTest ? BSShaderManager::BSSM_DEPTHMAP_SAt : BSShaderManager::BSSM_DEPTHMAP_S;
    }
    else {
        uiPass = abAlphaTest ? BSShaderManager::BSSM_DEPTHMAP_At : BSShaderManager::BSSM_DEPTHMAP;
    }

    pDepthMapRenderPassList->Add(apGeometry, uiPass, true, 1, apAccumulator->spShadowLight, nullptr, nullptr, nullptr);
}

void BSShaderProperty::CheckCreateRenderPassArray(int32_t aiNumPasses) {
    uint32_t v3;
    if (!pRenderPassList) {
        v3 = aiNumPasses;
        if (aiNumPasses == (-1)) {
            v3 = 5;
        }
        pRenderPassList = new BSShaderProperty::RenderPassArray(v3);
    }
}

void BSShaderProperty::CheckCreateWaterFogRenderPassArray(int32_t aiNumPasses) {
    if (!pWaterFogRenderPassList) {
        pWaterFogRenderPassList = new BSShaderProperty::RenderPassArray(aiNumPasses);
    }
}

void BSShaderProperty::CheckSilhouetteRenderPassArray(int32_t aiNumPasses) {
    if (!pSilhouettePassList) {
        pSilhouettePassList = new BSShaderProperty::RenderPassArray(aiNumPasses);
    }
}

void BSShaderProperty::ClearRenderPasses() {
    if (pRenderPassList) {
        pRenderPassList->Clear();
    }
}

// Frees the lists in this order: main, depth map, local map, self-illumination, constant alpha, water fog, silhouette.
void BSShaderProperty::FreeRenderPasses() {
    iLastRenderPassState = 0;
    if (RenderPassArray* pArray = pRenderPassList) {
        delete pArray;
        pRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pDepthMapRenderPassList) {
        delete pArray;
        pDepthMapRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pLocalMapRenderPassList) {
        delete pArray;
        pLocalMapRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pSIBlockRenderPassList) {
        delete pArray;
        pSIBlockRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pConstAlphaRenderPassList) {
        delete pArray;
        pConstAlphaRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pWaterFogRenderPassList) {
        delete pArray;
        pWaterFogRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pSilhouettePassList) {
        delete pArray;
        pSilhouettePassList = nullptr;
    }
}

void BSShaderProperty::SetFadeAlpha(float afFadeAlpha) {
    fFadeAlpha = afFadeAlpha;
}

// Tests the low byte of abIsMotionBlurred only.
void BSShaderProperty::SetMotionBlurred(int32_t abIsMotionBlurred) {
    if (static_cast<uint8_t>(abIsMotionBlurred)) {
        if ((ulFlags[0] & BSS1_Motion_Blur) == 0) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] |= BSS1_Motion_Blur;
    }
    else {
        if ((ulFlags[0] & BSS1_Motion_Blur) != 0) {
            iLastRenderPassState = 0;
        }
        ulFlags[0] &= ~BSS1_Motion_Blur;
    }
}

// Only the low byte of the update flag is compared with 1.
int32_t BSShaderProperty::AmbientDirAndPoint(
    NiGeometry* apGeometry,
    ShadowSceneLight* apDirLight,
    ShadowSceneLight* apPointLight,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* apbFirstPass,
    char abSkinned,
    char abTreeBranch
) {
    const uint8_t ucUpdateList = static_cast<uint8_t>(aiUpdateList);
    if (abSkinned) {
        if (ucUpdateList == 1) {
            pRenderPassList->Add(
                apGeometry,
                BSShaderManager::BSSM_AMBDIFFDIRANDPT_S,
                1,
                2u,
                apDirLight,
                apPointLight,
                nullptr,
                nullptr
            );
            *apbFirstPass = false;
            return 0;
        }
    }
    else {
        if (abTreeBranch) {
            if (ucUpdateList == 1) {
                pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_AMBDIFFDIRANDPT_Sb,
                    1,
                    2u,
                    apDirLight,
                    apPointLight,
                    nullptr,
                    nullptr
                );
                *apbFirstPass = false;
                return 0;
            }
        }
        else {
            if (ucUpdateList == 1) {
                pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_AMBDIFFDIRANDPT,
                    1,
                    2u,
                    apDirLight,
                    apPointLight,
                    nullptr,
                    nullptr
                );
                *apbFirstPass = false;
                return 0;
            }
        }
    }
    ++(*apusPassCount);
    *apbFirstPass = false;
    return 0;
}

// NiShadeProperty's members, then the shader flags, alphas, shader index and depth bias; the render pass state of
// pDest is reset.
void BSShaderProperty::CopyMembers(BSShaderProperty* pDest, NiCloningProcess& arCloneData) {
    NiShadeProperty::CopyMembers(pDest, arCloneData);
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    pDest->fAlpha = fAlpha;
    double fFadeAlpha = this->fFadeAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;
}

void BSShaderProperty::InitRenderPassArray_depthMap(uint32_t auiNumPasses) {
    if (!pDepthMapRenderPassList) {
        pDepthMapRenderPassList = new BSShaderProperty::RenderPassArray(auiNumPasses);
    }
}

void BSShaderProperty::AddLocalMapPass(NiGeometry* apGeometry, bool abSkinned, bool abHidden) {
    BSShaderProperty::RenderPassArray* pLocalMapRenderPassArray = this->pLocalMapRenderPassList;
    if (!pLocalMapRenderPassArray->iSize) {
        if (abHidden) {
            pLocalMapRenderPassArray
                ->Add(apGeometry, BSShaderManager::BSSM_LOCALMAP_CLEAR, 1, 0, nullptr, nullptr, nullptr, nullptr);
        }
        else {
            if (abSkinned) {
                pLocalMapRenderPassArray
                    ->Add(apGeometry, BSShaderManager::BSSM_LOCALMAP_S, 1, 0, nullptr, nullptr, nullptr, nullptr);
            }
            else {
                pLocalMapRenderPassArray
                    ->Add(apGeometry, BSShaderManager::BSSM_LOCALMAP, 1, 0, nullptr, nullptr, nullptr, nullptr);
            }
        }
    }
}

// Only the update flag's low byte is compared with 1.
int32_t BSShaderProperty::Ambient(
    NiTriShape* pShape,
    ShadowSceneLight* pLight,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    uint8_t* apbFirstPass,
    uint8_t abSkinned,
    uint8_t abAlphaTest,
    uint8_t bGlowMap,
    uint8_t bTreeBranch
) {
    const uint8_t ucUpdateList = static_cast<uint8_t>(aiUpdateList);
    if (abSkinned) {
        if (abAlphaTest) {
            if (bGlowMap) {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_SGAt, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
            else {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_SAt, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
        }
        else {
            if (bGlowMap) {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_SG, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
            else {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_S, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
        }
    }
    else {
        if (abAlphaTest) {
            if (bGlowMap) {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_GAt, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
            else {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_At, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
        }
        else {
            if (bGlowMap) {
                if (ucUpdateList == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AMBIENT_G, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *apbFirstPass = 0;
                    return 0;
                }
            }
            else {
                if (bTreeBranch) {
                    if (ucUpdateList == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AMBIENT_Sb, 1, 1u, pLight, nullptr, nullptr, nullptr);
                        *apbFirstPass = 0;
                        return 0;
                    }
                }
                else {
                    if (ucUpdateList == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AMBIENT, 1, 1u, pLight, nullptr, nullptr, nullptr);
                        *apbFirstPass = 0;
                        return 0;
                    }
                }
            }
        }
    }
    ++(*apusPassCount);
    *apbFirstPass = 0;
    return 0;
}

void BSShaderProperty::InitRenderPassArray_localMap(uint32_t auiNumPasses) {
    if (!pLocalMapRenderPassList) {
        pLocalMapRenderPassList = new BSShaderProperty::RenderPassArray(auiNumPasses);
    }
}

void BSShaderProperty::InitRenderPassArray_constAlpha(uint32_t auiNumPasses) {
    if (!pConstAlphaRenderPassList) {
        pConstAlphaRenderPassList = new BSShaderProperty::RenderPassArray(auiNumPasses);
    }
}

void BSShaderProperty::AddConstAlphaPass(NiGeometry* apGeometry, bool abSkinned, bool abAlphaTest) {
    BSShaderProperty::RenderPassArray* pConstAlphaRenderPassArray = this->pConstAlphaRenderPassList;
    if (abSkinned) {
        pConstAlphaRenderPassArray
            ->Add(apGeometry, BSShaderManager::BSSM_CONSTALPHA_S, 1, 0, nullptr, nullptr, nullptr, nullptr);
    }
    else {
        pConstAlphaRenderPassArray->Add(apGeometry, abAlphaTest + 6, 1, 0, nullptr, nullptr, nullptr, nullptr);
    }
}

void BSShaderProperty::AddPass_2X_EnvMap(
    NiGeometry* apGeometry,
    ShadowSceneLight* apSunLight,
    ShadowSceneLight* apLight,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* apbFirstPass,
    bool abSkinned,
    bool abEyeEnvMap,
    bool abWindowEnvMap
) {
    uint16_t usnewPassEnum;
    if (abSkinned) {
        if (static_cast<uint8_t>(aiUpdateList) == 1) {
            ShadowSceneLight* v14;
            BSShaderProperty::RenderPassArray* pRenderPassArray = this->pRenderPassList;
            if (apLight) {
                v14 = apLight;
            }
            else {
                v14 = apSunLight;
            }
            pRenderPassArray
                ->Add(apGeometry, BSShaderManager::BSSM_2x_ENVMAP_S, *apbFirstPass, 1, v14, nullptr, nullptr, nullptr);
            return;
        }
    }
    else if (static_cast<uint8_t>(aiUpdateList) == 1) {
        if (abEyeEnvMap) {
            usnewPassEnum = BSShaderManager::BSSM_2x_ENVMAP_EYE;
        }
        else {
            if (abWindowEnvMap) {
                this->pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_2x_ENVMAP_W,
                    *apbFirstPass,
                    0,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr
                );
                return;
            }
            usnewPassEnum = BSShaderManager::BSSM_2x_ENVMAP;
        }
        ShadowSceneLight* v11 = apLight;
        BSShaderProperty::RenderPassArray* v12 = this->pRenderPassList;
        if (!apLight) {
            v11 = apSunLight;
        }
        v12->Add(apGeometry, usnewPassEnum, *apbFirstPass, 1, v11, nullptr, nullptr, nullptr);
        return;
    }
    ++*apusPassCount;
}

void BSShaderProperty::SelfIlluminationBlock(
    NiGeometry* apGeometry,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool bSkinned
) {
    if (static_cast<uint8_t>(aiUpdateList)) {
        BSShaderProperty::RenderPassArray* pSIBlockRenderPassArray = this->pSIBlockRenderPassList;
        if (pSIBlockRenderPassArray) {
            pSIBlockRenderPassArray->Add(
                apGeometry,
                bSkinned ? BSShaderManager::BSSM_SELFILLUMALPHABLOCK_S : BSShaderManager::BSSM_SELFILLUMALPHABLOCK,
                0,
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }
    }
}

// Only the update flag's low byte is compared with 1.
int32_t BSShaderProperty::AddDepthPassOrCount(
    NiGeometry* apGeometry,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned
) {
    if (bSkinned) {
        if (static_cast<uint8_t>(aiUpdateList) == 1) {
            pRenderPassList->Add(
                apGeometry,
                BSShaderManager::BSSM_2x_TEXEFFECT_S,
                *bEnable,
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }
        else {
            ++*apusPassCount;
        }
        *bEnable = false;
        return 0;
    }

    if (static_cast<uint8_t>(aiUpdateList) == 1) {
        pRenderPassList
            ->Add(apGeometry, BSShaderManager::BSSM_2x_TEXEFFECT, *bEnable, 0, nullptr, nullptr, nullptr, nullptr);
    }
    else {
        ++*apusPassCount;
    }
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::AD2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned,
    char bGlowMap,
    bool bFaceGen,
    bool abUnused,
    bool bShadows,
    bool bParallax,
    bool bTreeBranch
) {
    if (bSkinned) {
        if (bGlowMap) {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AD2_SGShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(pShape, BSShaderManager::BSSM_AD2_SG, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen) {
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_SFgShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_SFg, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bParallax) {
                    *bEnable = false;
                    return 0;
                }
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_SShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_S, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bGlowMap) {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AD2_GShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(pShape, BSShaderManager::BSSM_AD2_G, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen) {
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_FgShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD2_Fg, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bParallax) {
                    if (bShadows) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_AD2_PxShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_AD2_Px, 1, 2, pLight, pLight1, nullptr, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                }
                else {
                    if (bShadows) {
                        if (bTreeBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD2_SbShp,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD2_Shp,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                    else {
                        if (bTreeBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD2_Sb,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList
                                    ->Add(pShape, BSShaderManager::BSSM_AD2, 1, 2, pLight, pLight1, nullptr, nullptr);
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::AD3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bIsSkinned,
    bool bGlowMap,
    bool bIsFaceGen,
    int32_t unused,
    bool bHasShadow,
    bool bHasParallax,
    bool bIsBranch
) {
    if (bIsSkinned) {
        if (bGlowMap) {
            if (bIsFaceGen || bHasParallax) {
                *bEnable = false;
                return 0;
            }
            if (bHasShadow) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_AD3_SGShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(pShape, BSShaderManager::BSSM_AD3_SG, 1, 3, pLight, pLight1, pLight2, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
        }
        else {
            if (bIsFaceGen) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_SFgShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_SFg, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHasParallax) {
                    *bEnable = false;
                    return 0;
                }
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_SShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_S, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bGlowMap) {
            if (bIsFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_GPxShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_GPx, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_GShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_G, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
        else {
            if (bIsFaceGen) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_FgShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_AD3_Fg, 1, 3, pLight, pLight1, pLight2, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHasParallax) {
                    if (bHasShadow) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_AD3_PxShp, 1, 3, pLight, pLight1, pLight2, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_AD3_Px, 1, 3, pLight, pLight1, pLight2, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                }
                else {
                    if (bHasShadow) {
                        if (bIsBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD3_SbShp,
                                    1,
                                    3,
                                    pLight,
                                    pLight1,
                                    pLight2,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD3_Shp,
                                    1,
                                    3,
                                    pLight,
                                    pLight1,
                                    pLight2,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                    else {
                        if (bIsBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_AD3_Sb,
                                    1,
                                    3,
                                    pLight,
                                    pLight1,
                                    pLight2,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList
                                    ->Add(pShape, BSShaderManager::BSSM_AD3, 1, 3, pLight, pLight1, pLight2, nullptr);
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::ADT2(
    NiGeometry* apGeometry,
    ShadowSceneLight* apSunLight,
    ShadowSceneLight* apLight,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* apbFirstPass,
    char abSkinned,
    char abGlowMap,
    char abFaceGen,
    char abUnused,
    char abProjShadow,
    char abParallax,
    char abTreeBranch,
    char abHair
) {
    if (abSkinned) {
        if (abGlowMap) {
            if (abFaceGen || abParallax) {
                *apbFirstPass = false;
                return 0;
            }
            if (abProjShadow) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        apGeometry,
                        BSShaderManager::BSSM_ADT2_SGShp,
                        1,
                        2,
                        apSunLight,
                        apLight,
                        nullptr,
                        nullptr
                    );
                    *apbFirstPass = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT2_SG, 1, 2, apSunLight, apLight, nullptr, nullptr);
                    *apbFirstPass = false;
                    return 0;
                }
            }
        }
        else {
            if (abFaceGen) {
                if (abProjShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_SFgShp,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_SFg,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
            else {
                if (abParallax) {
                    *apbFirstPass = false;
                    return 0;
                }
                if (abProjShadow) {
                    if (abHair) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_SHShp,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_SShp,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                }
                else {
                    if (abHair) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_SH,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_S,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                }
            }
        }
    }
    else {
        if (abGlowMap) {
            if (abFaceGen) {
                *apbFirstPass = false;
                return 0;
            }
            if (abParallax) {
                if (abProjShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_GPxShp,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_GPx,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
            else {
                if (abProjShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_GShp,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_G,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
        }
        else {
            if (abFaceGen) {
                if (abProjShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_FgShp,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT2_Fg,
                            1,
                            2,
                            apSunLight,
                            apLight,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
            else {
                if (abParallax) {
                    if (abProjShadow) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_PxShp,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_ADT2_Px,
                                1,
                                2,
                                apSunLight,
                                apLight,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                }
                else {
                    if (abProjShadow) {
                        if (abTreeBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_ADT2_SbShp,
                                    1,
                                    2,
                                    apSunLight,
                                    apLight,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                        else {
                            if (abHair) {
                                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                    pRenderPassList->Add(
                                        apGeometry,
                                        BSShaderManager::BSSM_ADT2_HShp,
                                        1,
                                        2,
                                        apSunLight,
                                        apLight,
                                        nullptr,
                                        nullptr
                                    );
                                    *apbFirstPass = false;
                                    return 0;
                                }
                            }
                            else {
                                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                    pRenderPassList->Add(
                                        apGeometry,
                                        BSShaderManager::BSSM_ADT2_Shp,
                                        1,
                                        2,
                                        apSunLight,
                                        apLight,
                                        nullptr,
                                        nullptr
                                    );
                                    *apbFirstPass = false;
                                    return 0;
                                }
                            }
                        }
                    }
                    else {
                        if (abTreeBranch) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_ADT2_Sb,
                                    1,
                                    2,
                                    apSunLight,
                                    apLight,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                        else {
                            if (abHair) {
                                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                    pRenderPassList->Add(
                                        apGeometry,
                                        BSShaderManager::BSSM_ADT2_H,
                                        1,
                                        2,
                                        apSunLight,
                                        apLight,
                                        nullptr,
                                        nullptr
                                    );
                                    *apbFirstPass = false;
                                    return 0;
                                }
                            }
                            else {
                                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                    pRenderPassList->Add(
                                        apGeometry,
                                        BSShaderManager::BSSM_ADT2,
                                        1,
                                        2,
                                        apSunLight,
                                        apLight,
                                        nullptr,
                                        nullptr
                                    );
                                    *apbFirstPass = false;
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *apbFirstPass = false;
    return 0;
}

// Only the update flag's low byte is compared with 1.
int32_t BSShaderProperty::ADTS(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    uint16_t* apusPassCount,
    int32_t abAddPass,
    bool* bEnable,
    bool bSkinned,
    bool bGlowMap,
    bool bFaceGen,
    int32_t empty,
    bool bShadows,
    bool bParallax,
    char bTreeBranch,
    char bHair
) {
    if (bSkinned) {
        if (bGlowMap) {
            if (bFaceGen || bParallax) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_ADTS_SGShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_ADTS_SG, 1, 1, pLight, nullptr, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen || bParallax) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (bHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_SHShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_SShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_SH, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_S, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bGlowMap) {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_GPxShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_GPx, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_GShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_G, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
        else {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_PxShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS_Px, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(abAddPass) == 1) {
                            pRenderPassList->Add(
                                pShape,
                                BSShaderManager::BSSM_ADTS_SbShp,
                                1,
                                1,
                                pLight,
                                nullptr,
                                nullptr,
                                nullptr
                            );
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS_HShp,
                                    1,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS_Shp,
                                    1,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(abAddPass) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_ADTS_Sb, 1, 1, pLight, nullptr, nullptr, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS_H,
                                    1,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList
                                    ->Add(pShape, BSShaderManager::BSSM_ADTS, 1, 1, pLight, nullptr, nullptr, nullptr);
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::ADTS2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned,
    bool bGlowMap,
    bool bFaceGen,
    int32_t empty,
    bool bShadows,
    bool bParallax,
    bool bTreeBranch,
    bool bHair
) {
    if (bSkinned) {
        if (bGlowMap) {
            if (bFaceGen || bParallax) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_ADTS2_SGShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList
                        ->Add(pShape, BSShaderManager::BSSM_ADTS2_SG, 1, 2, pLight, pLight1, nullptr, nullptr);
                    *bEnable = false;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen || bParallax) {
                *bEnable = false;
                return 0;
            }
            if (bShadows) {
                if (bHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_SHShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_SShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_SH, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_S, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bGlowMap) {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_GPxShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_GPx, 1, 1, pLight, nullptr, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_GShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_G, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
        else {
            if (bFaceGen) {
                *bEnable = false;
                return 0;
            }
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_PxShp, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList
                            ->Add(pShape, BSShaderManager::BSSM_ADTS2_Px, 1, 2, pLight, pLight1, nullptr, nullptr);
                        *bEnable = false;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                pShape,
                                BSShaderManager::BSSM_ADTS2_SbShp,
                                1,
                                2,
                                pLight,
                                pLight1,
                                nullptr,
                                nullptr
                            );
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS2_HShp,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS2_Shp,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList
                                ->Add(pShape, BSShaderManager::BSSM_ADTS2_Sb, 1, 2, pLight, pLight1, nullptr, nullptr);
                            *bEnable = false;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    pShape,
                                    BSShaderManager::BSSM_ADTS2_H,
                                    1,
                                    2,
                                    pLight,
                                    pLight1,
                                    nullptr,
                                    nullptr
                                );
                                *bEnable = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList
                                    ->Add(pShape, BSShaderManager::BSSM_ADTS2, 1, 2, pLight, pLight1, nullptr, nullptr);
                                *bEnable = false;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::DiffusePoint2(
    NiGeometry* apGeometry,
    ShadowSceneLight* apLight0,
    ShadowSceneLight* apLight1,
    uint16_t* apusPassCount,
    int32_t abAddPass,
    bool* abEnablePass,
    bool abIsSkinned,
    bool abIsFaceGen,
    int32_t empty,
    bool abHasParallax,
    bool abTreeBranch
) {
    if (!abIsSkinned) {
        if (abIsFaceGen) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_DIFFUSEPT2_Fg,
                    *abEnablePass,
                    2,
                    apLight0,
                    apLight1,
                    nullptr,
                    nullptr
                );
                *abEnablePass = false;
                return 0;
            }
        }
        else {
            if (abHasParallax) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(
                        apGeometry,
                        BSShaderManager::BSSM_DIFFUSEPT2_Px,
                        *abEnablePass,
                        2,
                        apLight0,
                        apLight1,
                        nullptr,
                        nullptr
                    );
                    *abEnablePass = false;
                    return 0;
                }
            }
            else {
                if (abTreeBranch) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_DIFFUSEPT2_Sb,
                            *abEnablePass,
                            2,
                            apLight0,
                            apLight1,
                            nullptr,
                            nullptr
                        );
                        *abEnablePass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_DIFFUSEPT2,
                            *abEnablePass,
                            2,
                            apLight0,
                            apLight1,
                            nullptr,
                            nullptr
                        );
                        *abEnablePass = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (abIsFaceGen) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_DIFFUSEPT2_SFg,
                    *abEnablePass,
                    2,
                    apLight0,
                    apLight1,
                    nullptr,
                    nullptr
                );
                *abEnablePass = false;
                return 0;
            }
        }
        else {
            if (abHasParallax) {
                *abEnablePass = false;
                return 0;
            }
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(
                    apGeometry,
                    BSShaderManager::BSSM_DIFFUSEPT2_S,
                    *abEnablePass,
                    2,
                    apLight0,
                    apLight1,
                    nullptr,
                    nullptr
                );
                *abEnablePass = false;
                return 0;
            }
        }
    }
    ++*apusPassCount;
    *abEnablePass = false;
    return 0;
}

int32_t BSShaderProperty::DiffusePoint3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned,
    bool bFaceGen,
    int32_t empty,
    bool bParallax,
    bool bTreeBranch
) {
    if (!bSkinned) {
        if (bFaceGen) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList
                    ->Add(pShape, BSShaderManager::BSSM_DIFFUSEPT3_Fg, *bEnable, 3, pLight, pLight1, pLight2, nullptr);
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (bParallax) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        pShape,
                        BSShaderManager::BSSM_DIFFUSEPT3_Px,
                        *bEnable,
                        3,
                        pLight,
                        pLight1,
                        pLight2,
                        nullptr
                    );
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (bTreeBranch) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_DIFFUSEPT3_Sb,
                            *bEnable,
                            3,
                            pLight,
                            pLight1,
                            pLight2,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_DIFFUSEPT3,
                            *bEnable,
                            3,
                            pLight,
                            pLight1,
                            pLight2,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bFaceGen) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList
                    ->Add(pShape, BSShaderManager::BSSM_DIFFUSEPT3_SFg, *bEnable, 3, pLight, pLight1, pLight2, nullptr);
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (bParallax) {
                *bEnable = false;
                return 0;
            }
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList
                    ->Add(pShape, BSShaderManager::BSSM_DIFFUSEPT3_S, *bEnable, 3, pLight, pLight1, pLight2, nullptr);
                *bEnable = false;
                return 0;
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

void BSShaderProperty::Textures(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    uint16_t* apusPassCount,
    int32_t abAddPass,
    bool* bEnable,
    bool bSkinned,
    int32_t empty,
    bool bFaceGen,
    bool bVertexColors,
    bool bTreeBranch,
    int32_t empty1,
    bool bParallax,
    bool bHair
) {
    if (bParallax) {
        if (!bSkinned) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(pShape, BSShaderManager::BSSM_TEXTURE_Px, *bEnable, 0, nullptr, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
    }
    else {
        static_cast<BSShaderPPLightingProperty*>(this)->AddTexturePass_1x(
            pShape,
            pLight,
            *apusPassCount,
            abAddPass,
            *bEnable,
            bSkinned,
            empty,
            bFaceGen,
            bVertexColors,
            bTreeBranch,
            bHair
        );
    }
}

int32_t BSShaderProperty::SpecularPoint2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned,
    bool bParallax,
    bool bTreeBranch,
    bool bHair
) {
    if (!bSkinned) {
        if (bParallax) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT2_Px,
                    *bEnable,
                    2,
                    pLight,
                    pLight1,
                    nullptr,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (bTreeBranch) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        pShape,
                        BSShaderManager::BSSM_2x_SPECULARPT2_Sb,
                        *bEnable,
                        2,
                        pLight,
                        pLight1,
                        nullptr,
                        nullptr
                    );
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_2x_SPECULARPT2_H,
                            *bEnable,
                            2,
                            pLight,
                            pLight1,
                            nullptr,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_2x_SPECULARPT2,
                            *bEnable,
                            2,
                            pLight,
                            pLight1,
                            nullptr,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bParallax) {
            *bEnable = false;
            return 0;
        }
        if (bHair) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT2_SH,
                    *bEnable,
                    2,
                    pLight,
                    pLight1,
                    nullptr,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT2_S,
                    *bEnable,
                    2,
                    pLight,
                    pLight1,
                    nullptr,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::SpecularPoint3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* bEnable,
    bool bSkinned,
    bool bParallax,
    bool bTreeBranch,
    bool bHair
) {
    if (!bSkinned) {
        if (bParallax) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT3_Px,
                    *bEnable,
                    3,
                    pLight,
                    pLight1,
                    pLight2,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (bTreeBranch) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        pShape,
                        BSShaderManager::BSSM_2x_SPECULARPT3_Sb,
                        *bEnable,
                        3,
                        pLight,
                        pLight1,
                        pLight2,
                        nullptr
                    );
                    *bEnable = false;
                    return 0;
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_2x_SPECULARPT3_H,
                            *bEnable,
                            3,
                            pLight,
                            pLight1,
                            pLight2,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            pShape,
                            BSShaderManager::BSSM_2x_SPECULARPT3,
                            *bEnable,
                            3,
                            pLight,
                            pLight1,
                            pLight2,
                            nullptr
                        );
                        *bEnable = false;
                        return 0;
                    }
                }
            }
        }
    }
    else {
        if (bParallax) {
            *bEnable = false;
            return 0;
        }
        if (bHair) {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT3_SH,
                    *bEnable,
                    3,
                    pLight,
                    pLight1,
                    pLight2,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
        else {
            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                pRenderPassList->Add(
                    pShape,
                    BSShaderManager::BSSM_2x_SPECULARPT3_S,
                    *bEnable,
                    3,
                    pLight,
                    pLight1,
                    pLight2,
                    nullptr
                );
                *bEnable = false;
                return 0;
            }
        }
    }
    ++*apusPassCount;
    *bEnable = false;
    return 0;
}

int32_t BSShaderProperty::ADT(
    NiGeometry* apGeometry,
    ShadowSceneLight* pLight,
    uint16_t* apusPassCount,
    int32_t abAddPass,
    bool* abEnable,
    bool abIsSkinned,
    bool abGlowMap,
    bool bIsFaceGen,
    bool unused,
    bool bHasShadow,
    bool bHasParallax,
    bool bHasModelSpaceNormals,
    bool bIsBranch,
    bool bIsTree,
    bool bIsHair,
    int32_t x0
) {
    if (abIsSkinned) {
        if (abGlowMap) {
            if (!bIsFaceGen && !bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(apGeometry, BSShaderManager::BSSM_ADT_SGShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                    }
                    else {
                        ++*apusPassCount;
                    }
                }
                else if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_SG, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
        }
        else if (bIsFaceGen) {
            if (bHasShadow) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_SFgShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
            else if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_SFg, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (!bHasParallax) {
            if (bHasShadow) {
                if (bIsHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList
                            ->Add(apGeometry, BSShaderManager::BSSM_ADT_SHShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                    }
                    else {
                        ++*apusPassCount;
                    }
                }
                else if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_SShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
            else if (bIsHair) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_SH, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
            else if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_S, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
    }
    else if (abGlowMap) {
        if (!bIsFaceGen) {
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_ADT_GPxShp,
                            1,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                    }
                    else {
                        ++*apusPassCount;
                    }
                }
                else if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_GPx, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
            else if (bHasShadow) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList
                        ->Add(apGeometry, BSShaderManager::BSSM_ADT_GShp, 1, 1, pLight, nullptr, nullptr, nullptr);
                }
                else {
                    ++*apusPassCount;
                }
            }
            else if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_G, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
    }
    else if (bIsFaceGen) {
        if (bHasShadow) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_FgShp, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_Fg, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (bHasParallax) {
        if (bHasShadow) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_PxShp, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (static_cast<uint8_t>(abAddPass) == 1) {
            NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(
                apGeometry->m_PropertyState.m_aspProps[2].m_pObject
            );
            BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(
                apGeometry->m_PropertyState.m_aspProps[3].m_pObject
            );
            if (BSShaderManager::bUseOptPasses &&
                (!pMaterial || pMaterial->GetEmittance() == NiColor::BLACK) &&
                pShaderProp->GetFlag(BSSP_FLAG_VERTEXCOLORS) &&
                !(pShaderProp->fFadeAlpha < 1.0f)) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_PxOpt, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_Px, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (bHasModelSpaceNormals) {
        if (bHasShadow) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_MnShp, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_Mn, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (bHasShadow) {
        if (bIsBranch) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_SbShp, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (bIsHair) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_HShp, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_Shp, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (bIsBranch || bIsTree) {
        if (bIsTree) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList
                    ->Add(apGeometry, BSShaderManager::BSSM_ADT_Sbb, 1, 1, pLight, nullptr, nullptr, nullptr);
            }
            else {
                ++*apusPassCount;
            }
        }
        else if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_Sb, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (bIsHair) {
        if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT_H, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
        else {
            ++*apusPassCount;
        }
    }
    else if (static_cast<uint8_t>(abAddPass) == 1) {
        NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(
            apGeometry->m_PropertyState.m_aspProps[2].m_pObject
        );
        BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(
            apGeometry->m_PropertyState.m_aspProps[3].m_pObject
        );
        if (BSShaderManager::bUseOptPasses &&
            (!pMaterial || pMaterial->GetEmittance() == NiColor::BLACK) &&
            !(pShaderProp->fAlpha < 1.0f) &&
            pShaderProp->GetFlag(BSSP_FLAG_VERTEXCOLORS)) {
            bool bLODBuilding = pShaderProp->GetFlag(BSSP_FLAG_LODBUILDING);
            pRenderPassList->Add(
                apGeometry,
                bLODBuilding + BSShaderManager::BSSM_ADT_Opt,
                1,
                1,
                pLight,
                nullptr,
                nullptr,
                nullptr
            );
        }
        else {
            pRenderPassList->Add(apGeometry, BSShaderManager::BSSM_ADT, 1, 1, pLight, nullptr, nullptr, nullptr);
        }
    }
    else {
        ++*apusPassCount;
    }
    *abEnable = false;
    return 0;
}

int32_t BSShaderProperty::Specular(
    NiGeometry* apGeometry,
    ShadowSceneLight* pLight,
    uint16_t* apusPassCount,
    int32_t aiUpdateList,
    bool* apbFirstPass,
    bool bIsSkinned,
    int32_t unused,
    bool bHasShadow,
    bool bHasParallax,
    bool bIsBranch,
    bool bIsHair
) {
    bool bPointLight = pLight->bPointLight;
    NiLight* m_pObject = pLight->spLight.m_pObject;
    double r = m_pObject->m_kDiff.r;
    NiColor* p_m_kDiff = &m_pObject->m_kDiff;
    if (NiColor::BLACK.r == r && (NiColor::BLACK.g == p_m_kDiff->g && NiColor::BLACK.b == p_m_kDiff->b)) {
        return 0;
    }
    if (!bIsSkinned) {
        if (bPointLight) {
            if (bHasParallax) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        apGeometry,
                        BSShaderManager::BSSM_2x_SPECULARPT_Px,
                        *apbFirstPass,
                        1,
                        pLight,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                    *apbFirstPass = false;
                    return 0;
                }
            }
            else {
                if (bIsBranch) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARPT_Sb,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (bIsHair) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_2x_SPECULARPT_H,
                                *apbFirstPass,
                                1,
                                pLight,
                                nullptr,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_2x_SPECULARPT,
                                *apbFirstPass,
                                1,
                                pLight,
                                nullptr,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                }
            }
        }
        else {
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_PxShp,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_Px,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
            else {
                if (bHasShadow) {
                    if (bIsBranch) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_2x_SPECULARDIR_SbShp,
                                *apbFirstPass,
                                1,
                                pLight,
                                nullptr,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (bIsHair) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_2x_SPECULARDIR_HShp,
                                    *apbFirstPass,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_2x_SPECULARDIR_Shp,
                                    *apbFirstPass,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bIsBranch) {
                        if (static_cast<uint8_t>(aiUpdateList) == 1) {
                            pRenderPassList->Add(
                                apGeometry,
                                BSShaderManager::BSSM_2x_SPECULARDIR_Sb,
                                *apbFirstPass,
                                1,
                                pLight,
                                nullptr,
                                nullptr,
                                nullptr
                            );
                            *apbFirstPass = false;
                            return 0;
                        }
                    }
                    else {
                        if (bIsHair) {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_2x_SPECULARDIR_H,
                                    *apbFirstPass,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(aiUpdateList) == 1) {
                                pRenderPassList->Add(
                                    apGeometry,
                                    BSShaderManager::BSSM_2x_SPECULARDIR,
                                    *apbFirstPass,
                                    1,
                                    pLight,
                                    nullptr,
                                    nullptr,
                                    nullptr
                                );
                                *apbFirstPass = false;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        if (bPointLight) {
            if (bHasParallax) {
                *apbFirstPass = false;
                return 0;
            }
            if (bIsHair) {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        apGeometry,
                        BSShaderManager::BSSM_2x_SPECULARPT_SH,
                        *apbFirstPass,
                        1,
                        pLight,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                    *apbFirstPass = false;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(aiUpdateList) == 1) {
                    pRenderPassList->Add(
                        apGeometry,
                        BSShaderManager::BSSM_2x_SPECULARPT_S,
                        *apbFirstPass,
                        1,
                        pLight,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                    *apbFirstPass = false;
                    return 0;
                }
            }
        }
        else {
            if (bHasParallax) {
                *apbFirstPass = false;
                return 0;
            }
            if (bHasShadow) {
                if (bIsHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_SHShp,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_SShp,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
            else {
                if (bIsHair) {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_SH,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(aiUpdateList) == 1) {
                        pRenderPassList->Add(
                            apGeometry,
                            BSShaderManager::BSSM_2x_SPECULARDIR_S,
                            *apbFirstPass,
                            1,
                            pLight,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        *apbFirstPass = false;
                        return 0;
                    }
                }
            }
        }
    }
    ++*apusPassCount;
    *apbFirstPass = false;
    return 0;
}

void BSShaderProperty::ToggleMotionBlurRecurse(NiAVObject* apObject) {
    BSShaderProperty* property;
    NiNode* v2;
    NiNode* v3;
    uint32_t m_usSize;
    uint32_t i;
    if (apObject) {
        if (apObject->IsTriBasedGeom()) {
            property = static_cast<BSShaderProperty*>(apObject->GetProperty(3));
            if (property) {
                property->SetMotionBlurred((property->ulFlags[0] & BSS1_Motion_Blur) == 0);
            }
        }
        else {
            v2 = apObject->IsNode();
            v3 = v2;
            if (v2) {
                m_usSize = v2->m_kChildren.m_usSize;
                for (i = 0; i < m_usSize; ++i) {
                    if (v3->m_kChildren.m_pBase[i].m_pObject) {
                        BSShaderProperty::ToggleMotionBlurRecurse(v3->m_kChildren.m_pBase[i].m_pObject);
                    }
                }
            }
        }
    }
}

// A tri-based geometry's shade property gets the flag; a node's children are visited in order. The property pointer is
// loaded once, so the property is called only when GetProperty returned one.
void BSShaderProperty::SetMotionBlurRecurse(NiAVObject* apObject, int32_t abMotionBlur) {
    if (!apObject) {
        return;
    }
    if (apObject->IsTriBasedGeom()) {
        NiProperty* pProperty = apObject->GetProperty(NiProperty::SHADE);
        if (!pProperty) {
            return;
        }
        static_cast<BSShaderProperty*>(pProperty)->SetMotionBlurred(abMotionBlur);
        return;
    }
    NiNode* pNode = apObject->IsNode();
    if (!pNode) {
        return;
    }
    // Reads the child count once and re-reads the slot array on every iteration.
    const uint16_t usChildren = pNode->m_kChildren.m_usSize;
    for (uint32_t i = 0; i < usChildren; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            BSShaderProperty::SetMotionBlurRecurse(pChild, abMotionBlur);
        }
    }
}

void BSShaderProperty::SetFlagRecurse(NiAVObject* apObject, EShaderPropertyFlag aeFlag, int32_t abEnable) {
    if (!apObject) {
        return;
    }
    if (apObject->IsGeometry()) {
        NiProperty* pProperty = apObject->GetProperty(NiProperty::SHADE);
        if (!pProperty) {
            return;
        }
        static_cast<BSShaderProperty*>(pProperty)->SetFlag(aeFlag, abEnable);
        return;
    }
    NiNode* pNode = apObject->IsNode();
    if (!pNode) {
        return;
    }
    const uint16_t usChildren = pNode->m_kChildren.m_usSize;
    for (uint32_t i = 0; i < usChildren; ++i) {
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            BSShaderProperty::SetFlagRecurse(pChild, aeFlag, abEnable);
        }
    }
}

NiObject* BSShaderProperty::CreateObject() {
    return new BSShaderProperty;
}
