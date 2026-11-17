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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
        ppSceneLights =
            static_cast<ShadowSceneLight**>(MemoryManager_Singleton.Allocate(sizeof(ShadowSceneLight*) * ucNumLights));
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
    usPassEnum = 0;
    if (ppSceneLights) {
        MemoryManager_Singleton.Deallocate(ppSceneLights);
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
            (&MemoryManager_Singleton)->Deallocate(ppSceneLights);
            ppSceneLights = nullptr;
        }
        ppSceneLights =
            static_cast<ShadowSceneLight**>((&MemoryManager_Singleton)->Allocate(sizeof(ShadowSceneLight*) * ucCount));
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
        (&MemoryManager_Singleton)->Deallocate(ppSceneLights);
        ppSceneLights = (ShadowSceneLight**)0;
    }
}

void BSShaderProperty::RenderPass::AllocateLightArray(uint8_t a2) {
    ppSceneLights = static_cast<ShadowSceneLight**>(MemoryManager_Singleton.Allocate(
        a2 <= SIZE_MAX / sizeof(ShadowSceneLight*) ? sizeof(ShadowSceneLight*) * a2 : SIZE_MAX
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
        void* pMemory = (&MemoryManager_Singleton)->Allocate(sizeof(RenderPass));
        RenderPass* pPass = nullptr;
        if (pMemory) {
            pPass = ::new (pMemory) RenderPass(
                apGeometry, ausPassEnum, abFirstPass, ausNumLights, apSceneLight, apSceneLight1, apSceneLight2,
                apSceneLight3
            );
        }
        pPassA.Add(pPass);
    }
    ++iSize;
}

void BSShaderProperty::RenderPassArray::Clear() {
    iSize = (uint32_t)0;
}

void BSShaderProperty::PreviousTransform::SetFreeze(bool abFreeze) {
    BSShaderProperty::PreviousTransform::bFrozen = (bool)((int8_t)abFreeze);
}

void BSShaderProperty::PreviousTransform::ToggleConsole() {
    BSShaderProperty::PreviousTransform::bConsole = !BSShaderProperty::PreviousTransform::bConsole;
}

void BSShaderProperty::PreviousTransform::SaveCamera(NiCamera* apCamera) {
    if (!BSShaderProperty::PreviousTransform::bFrozen && !BSShaderProperty::PreviousTransform::bConsole) {
        BSShaderProperty::PreviousTransform::matView = BSShaderManager::pRenderer.m_pObject->m_kD3DView;
        BSShaderProperty::PreviousTransform::matProj = BSShaderManager::pRenderer.m_pObject->m_kD3DProj;
        BSShaderProperty::PreviousTransform::bViewProjReady = 1;
        BSShaderProperty::PreviousTransform::bDirty = 0;
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
    void* v3 = NiMemObject::operator new(sizeof(BSShaderProperty));
    if (v3) {
        v4 = ::new (v3) BSShaderProperty;
    }
    else {
        v4 = nullptr;
    }
    NiShadeProperty::CopyMembers(v4, arCloneData);
    v4->ulFlags[0] = ulFlags[0];
    v4->ulFlags[1] = ulFlags[1];
    v4->fAlpha = fAlpha;
    fFadeAlpha = (double)this->fFadeAlpha;
    v4->iLastRenderPassState = (uint32_t)0;
    v4->fFadeAlpha = (float)fFadeAlpha;
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
    if (ulFlags[1] & 0x2000) {
        if (!(ulFlags[1] & 0x8000)) {
            iLastRenderPassState = 0;
        }
        ulFlags[1] |= 0x8000;
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
    for (uint32_t i = 0; i < 2; ++i) {
        NiStreamSaveBinary(kStream, ulFlags[i]);
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
    if (ulFlags[0] & 0x80) {
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
        if (pPass->usPassEnum < 0x2F7) {
            apStrings->Add(NiGetViewerString(
                pPass->bFirstPass ? " Fpass" : "  pass", BSShaderManager::GetPassName(pPass->usPassEnum)
            ));
        }
    }
}

void BSShaderProperty::PostLinkObject(NiStream& kStream) {
    NiShadeProperty::PostLinkObject(kStream);
    if (ulFlags[0] & 0x4000000) {
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
        // Does what NiObjectNET::SetControllers does.
        NiTimeController* pOldControllers = pDest->m_spControllers;
        if (pOldControllers != m_spControllers) {
            if (pOldControllers) {
                NiDecRefCount(pOldControllers);
            }
            pDest->m_spControllers = m_spControllers;
            if (m_spControllers) {
                NiIncRefCount(m_spControllers);
            }
        }
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
    else if (fAlpha < 1.0f) {
        if (afAlpha >= 1.0f) {
            iLastRenderPassState = 0;
        }
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
    void* v4;
    BSShaderProperty::RenderPassArray* v5;
    if (!pRenderPassList) {
        v3 = aiNumPasses;
        if (aiNumPasses == ((-1))) {
            v3 = 5;
        }
        v4 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v4) {
            v5 = ::new (v4) BSShaderProperty::RenderPassArray(v3);
        }
        else {
            v5 = nullptr;
        }
        pRenderPassList = v5;
    }
}

void BSShaderProperty::CheckCreateWaterFogRenderPassArray(int32_t aiNumPasses) {
    void* v3;
    BSShaderProperty::RenderPassArray* v4;
    if (!pWaterFogRenderPassList) {
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v3) {
            v4 = ::new (v3) BSShaderProperty::RenderPassArray(aiNumPasses);
        }
        else {
            v4 = nullptr;
        }
        pWaterFogRenderPassList = v4;
    }
}

void BSShaderProperty::CheckSilhouetteRenderPassArray(int32_t aiNumPasses) {
    void* v3;
    BSShaderProperty::RenderPassArray* v4;
    if (!pSilhouettePassList) {
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v3) {
            v4 = ::new (v3) BSShaderProperty::RenderPassArray(aiNumPasses);
        }
        else {
            v4 = nullptr;
        }
        pSilhouettePassList = v4;
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
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pDepthMapRenderPassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pDepthMapRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pLocalMapRenderPassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pLocalMapRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pSIBlockRenderPassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pSIBlockRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pConstAlphaRenderPassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pConstAlphaRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pWaterFogRenderPassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pWaterFogRenderPassList = nullptr;
    }
    if (RenderPassArray* pArray = pSilhouettePassList) {
        pArray->~RenderPassArray();
        (&MemoryManager_Singleton)->Deallocate(pArray);
        pSilhouettePassList = nullptr;
    }
}

void BSShaderProperty::SetFadeAlpha(float afFadeAlpha) {
    fFadeAlpha = afFadeAlpha;
}

// Tests the low byte of abIsMotionBlurred only.
void BSShaderProperty::SetMotionBlurred(int32_t abIsMotionBlurred) {
    if (static_cast<uint8_t>(abIsMotionBlurred)) {
        if (((int32_t)(ulFlags[0] & 16)) == 0) {
            iLastRenderPassState = (uint32_t)0;
        }
        ulFlags[0] |= 0x10u;
    }
    else {
        if (((int32_t)(ulFlags[0] & 16)) != 0) {
            iLastRenderPassState = (uint32_t)0;
        }
        ulFlags[0] &= (~0x10u);
    }
}

// Only the low byte of the update flag is compared with 1.
int32_t BSShaderProperty::AmbientDirAndPoint(
    NiGeometry* a2,
    ShadowSceneLight* a6,
    ShadowSceneLight* a7,
    uint16_t* a5,
    int32_t aiUpdateList,
    bool* arg14,
    char a8,
    char a9
) {
    const uint8_t arg10 = static_cast<uint8_t>(aiUpdateList);
    if (a8) {
        if (arg10 == 1) {
            pRenderPassList->Add(a2, 0x7Du, 1, 2u, a6, a7, nullptr, nullptr);
            *arg14 = (uint8_t)0;
            return 0;
        }
    }
    else {
        if (a9) {
            if (arg10 == 1) {
                pRenderPassList->Add(a2, 0x7Eu, 1, 2u, a6, a7, nullptr, nullptr);
                *arg14 = (uint8_t)0;
                return 0;
            }
        }
        else {
            if (arg10 == 1) {
                pRenderPassList->Add(a2, 0x7Cu, 1, 2u, a6, a7, nullptr, nullptr);
                *arg14 = (uint8_t)0;
                return 0;
            }
        }
    }
    ++(*a5);
    *arg14 = (uint8_t)0;
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

void BSShaderProperty::InitRenderPassArray_depthMap(uint32_t a1) {
    void* v3;
    BSShaderProperty::RenderPassArray* v4;
    if (!pDepthMapRenderPassList) {
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v3) {
            v4 = ::new (v3) BSShaderProperty::RenderPassArray(a1);
        }
        else {
            v4 = nullptr;
        }
        pDepthMapRenderPassList = v4;
    }
}

void BSShaderProperty::AddLocalMapPass(NiGeometry* apGeometry, bool abSkinned, bool abHidden) {
    BSShaderProperty::RenderPassArray* pLocalMapRenderPassArray = this->pLocalMapRenderPassList;
    if (!pLocalMapRenderPassArray->iSize) {
        if (abHidden) {
            pLocalMapRenderPassArray->Add(apGeometry, 494, 1, 0, 0, 0, 0, 0);
        }
        else {
            if (abSkinned) {
                pLocalMapRenderPassArray->Add(apGeometry, 493, 1, 0, 0, 0, 0, 0);
            }
            else {
                pLocalMapRenderPassArray->Add(apGeometry, 492, 1, 0, 0, 0, 0, 0);
            }
        }
    }
}

// Only the update flag's low byte is compared with 1.
int32_t BSShaderProperty::Ambient(
    NiTriShape* pShape,
    ShadowSceneLight* pLight,
    uint16_t* a4,
    int32_t aiUpdateList,
    uint8_t* arg10,
    uint8_t a7,
    uint8_t a8,
    uint8_t bGlowMap,
    uint8_t bTreeBranch
) {
    const uint8_t a5 = static_cast<uint8_t>(aiUpdateList);
    if (a7) {
        if (a8) {
            if (bGlowMap) {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Fu, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Eu, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
        }
        else {
            if (bGlowMap) {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Du, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Cu, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
        }
    }
    else {
        if (a8) {
            if (bGlowMap) {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Bu, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x5Au, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
        }
        else {
            if (bGlowMap) {
                if (a5 == 1) {
                    pRenderPassList->Add(pShape, 0x59u, 1, 1u, pLight, nullptr, nullptr, nullptr);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (bTreeBranch) {
                    if (a5 == 1) {
                        pRenderPassList->Add(pShape, 0x60u, 1, 1u, pLight, nullptr, nullptr, nullptr);
                        *arg10 = 0;
                        return 0;
                    }
                }
                else {
                    if (a5 == 1) {
                        pRenderPassList->Add(pShape, 0x58u, 1, 1u, pLight, nullptr, nullptr, nullptr);
                        *arg10 = 0;
                        return 0;
                    }
                }
            }
        }
    }
    ++(*a4);
    *arg10 = 0;
    return 0;
}

void BSShaderProperty::InitRenderPassArray_localMap(uint32_t a2) {
    void* v3;
    BSShaderProperty::RenderPassArray* v4;
    if (!pLocalMapRenderPassList) {
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v3) {
            v4 = ::new (v3) BSShaderProperty::RenderPassArray(a2);
        }
        else {
            v4 = nullptr;
        }
        pLocalMapRenderPassList = v4;
    }
}

void BSShaderProperty::InitRenderPassArray_constAlpha(uint32_t a2) {
    void* v3;
    BSShaderProperty::RenderPassArray* v4;
    if (!pConstAlphaRenderPassList) {
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(BSShaderProperty::RenderPassArray));
        if (v3) {
            v4 = ::new (v3) BSShaderProperty::RenderPassArray(a2);
        }
        else {
            v4 = nullptr;
        }
        pConstAlphaRenderPassList = v4;
    }
}

void BSShaderProperty::AddConstAlphaPass(NiGeometry* a1, bool a2, bool a3) {
    BSShaderProperty::RenderPassArray* pConstAlphaRenderPassArray = this->pConstAlphaRenderPassList;
    if (a2) {
        pConstAlphaRenderPassArray->Add(a1, 8u, 1, 0, 0, 0, 0, 0);
    }
    else {
        pConstAlphaRenderPassArray->Add(a1, a3 + 6, 1, 0, 0, 0, 0, 0);
    }
}

void BSShaderProperty::AddPass_2X_EnvMap(
    NiGeometry* a1,
    ShadowSceneLight* a2,
    ShadowSceneLight* a3,
    uint16_t* a4,
    int32_t a5,
    bool* a6,
    bool a7,
    bool a8,
    bool a9
) {
    uint16_t usnewPassEnum;
    if (a7) {
        if (static_cast<uint8_t>(a5) == 1) {
            ShadowSceneLight* v14;
            BSShaderProperty::RenderPassArray* pRenderPassArray = this->pRenderPassList;
            if (a3) {
                v14 = a3;
            }
            else {
                v14 = a2;
            }
            pRenderPassArray->Add(a1, 585, *a6, 1, v14, 0, 0, 0);
            return;
        }
    LABEL_16:;
        ++*a4;
        return;
    }
    if (static_cast<uint8_t>(a5) != 1) {
        goto LABEL_16;
    }
    if (a8) {
        usnewPassEnum = 587;
    }
    else {
        if (a9) {
            this->pRenderPassList->Add(a1, 586, *a6, 0, 0, 0, 0, 0);
            return;
        }
        usnewPassEnum = 584;
    }
    ShadowSceneLight* v11 = a3;
    BSShaderProperty::RenderPassArray* v12 = this->pRenderPassList;
    if (!a3) {
        v11 = a2;
    }
    v12->Add(a1, usnewPassEnum, *a6, 1, v11, 0, 0, 0);
}

void BSShaderProperty::SelfIlluminationBlock(NiGeometry* a2, uint16_t* a3, int32_t a4, bool bSkinned) {
    int16_t v5;
    v5 = static_cast<int16_t>(
        (static_cast<uint32_t>(v5) & 0xFFFF00FFu) | (static_cast<uint32_t>(static_cast<uint8_t>(0)) << 8)
    );
    if (static_cast<uint8_t>(a4)) {
        BSShaderProperty::RenderPassArray* pSIBlockRenderPassArray = this->pSIBlockRenderPassList;
        if (pSIBlockRenderPassArray) {
            v5 = static_cast<int16_t>(
                (static_cast<uint32_t>(v5) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(bSkinned)))
            );
            pSIBlockRenderPassArray->Add(a2, v5 + 20, 0, 0, 0, 0, 0, 0);
        }
    }
}

// Only the update flag's low byte is compared with 1.
int32_t BSShaderProperty::AddDepthPassOrCount(NiGeometry* a2, uint16_t* a3, int32_t a4, bool* bEnable, bool bSkinned) {
    if (bSkinned) {
        if (static_cast<uint8_t>(a4) == 1) {
            pRenderPassList->Add(a2, 593, *bEnable, 0, 0, 0, 0, 0);
        }
        else {
            ++*a3;
        }
        *bEnable = 0;
        return 0;
    }
    else {
        if (static_cast<uint8_t>(a4) == 1) {
            pRenderPassList->Add(a2, 592, *bEnable, 0, 0, 0, 0, 0);
        }
        else {
            ++*a3;
        }
        *bEnable = 0;
        return 0;
    }
}

int32_t BSShaderProperty::AD2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* a5,
    int32_t arg10,
    bool* bEnable,
    bool bSkinned,
    char bGlowMap,
    bool bFaceGen,
    bool a11,
    bool bShadows,
    bool bParallax,
    bool bTreeBranch
) {
    if (bSkinned) {
        if (bGlowMap) {
            if (!bFaceGen) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 230, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 221, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                goto LABEL_51;
            }
        }
        else {
            if (bFaceGen) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 231, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 222, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                goto LABEL_51;
            }
            if (!bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 229, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 220, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                goto LABEL_51;
            }
        }
    }
    else {
        if (!bGlowMap) {
            if (bFaceGen) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 228, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 219, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bParallax) {
                    if (bShadows) {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(pShape, 226, 1, 2, pLight, pLight1, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(pShape, 217, 1, 2, pLight, pLight1, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                }
                else {
                    if (bShadows) {
                        if (bTreeBranch) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 232, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 224, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                    else {
                        if (bTreeBranch) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 223, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 215, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
            }
        LABEL_51:;
            ++*a5;
            goto LABEL_52;
        }
        if (!bFaceGen) {
            if (bShadows) {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(pShape, 225, 1, 2, pLight, pLight1, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(pShape, 216, 1, 2, pLight, pLight1, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            goto LABEL_51;
        }
    }
LABEL_52:;
    *bEnable = 0;
    return 0;
}

int32_t BSShaderProperty::AD3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* arg10,
    int32_t arg14,
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
                goto LABEL_59;
            }
            if (bHasShadow) {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 248, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 239, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            goto LABEL_58;
        }
        if (bIsFaceGen) {
            if (bHasShadow) {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 249, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 240, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            goto LABEL_58;
        }
        if (!bHasParallax) {
            if (bHasShadow) {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 247, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 238, 1, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            goto LABEL_58;
        }
    }
    else {
        if (!bGlowMap) {
            if (bIsFaceGen) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 246, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 237, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHasParallax) {
                    if (bHasShadow) {
                        if (static_cast<uint8_t>(arg14) == 1) {
                            pRenderPassList->Add(pShape, 244, 1, 3, pLight, pLight1, pLight2, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(arg14) == 1) {
                            pRenderPassList->Add(pShape, 235, 1, 3, pLight, pLight1, pLight2, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                }
                else {
                    if (bHasShadow) {
                        if (bIsBranch) {
                            if (static_cast<uint8_t>(arg14) == 1) {
                                pRenderPassList->Add(pShape, 250, 1, 3, pLight, pLight1, pLight2, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg14) == 1) {
                                pRenderPassList->Add(pShape, 242, 1, 3, pLight, pLight1, pLight2, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                    else {
                        if (bIsBranch) {
                            if (static_cast<uint8_t>(arg14) == 1) {
                                pRenderPassList->Add(pShape, 241, 1, 3, pLight, pLight1, pLight2, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg14) == 1) {
                                pRenderPassList->Add(pShape, 233, 1, 3, pLight, pLight1, pLight2, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
            }
        LABEL_58:;
            ++*arg10;
            goto LABEL_59;
        }
        if (!bIsFaceGen) {
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 245, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 236, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 243, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 234, 1, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_58;
        }
    }
LABEL_59:;
    *bEnable = 0;
    return 0;
}

int32_t BSShaderProperty::ADT2(
    NiGeometry* a1,
    ShadowSceneLight* a2,
    ShadowSceneLight* a3,
    uint16_t* a4,
    int32_t arg10,
    bool* arg14,
    char a7,
    char a8,
    char a9,
    char a10,
    char a11,
    char a12,
    char a13,
    char a14
) {
    if (a7) {
        if (a8) {
            if (a9 || a12) {
                goto LABEL_71;
            }
            if (a11) {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(a1, 296, 1, 2, a2, a3, 0, 0);
                    *arg14 = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(a1, 285, 1, 2, a2, a3, 0, 0);
                    *arg14 = 0;
                    return 0;
                }
            }
            goto LABEL_70;
        }
        if (a9) {
            if (a11) {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(a1, 297, 1, 2, a2, a3, 0, 0);
                    *arg14 = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(a1, 286, 1, 2, a2, a3, 0, 0);
                    *arg14 = 0;
                    return 0;
                }
            }
            goto LABEL_70;
        }
        if (!a12) {
            if (a11) {
                if (a14) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 299, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 295, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
            }
            else {
                if (a14) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 288, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 284, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_70;
        }
    }
    else {
        if (!a8) {
            if (a9) {
                if (a11) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 291, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 280, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
            }
            else {
                if (a12) {
                    if (a11) {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(a1, 292, 1, 2, a2, a3, 0, 0);
                            *arg14 = 0;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(a1, 281, 1, 2, a2, a3, 0, 0);
                            *arg14 = 0;
                            return 0;
                        }
                    }
                }
                else {
                    if (a11) {
                        if (a13) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(a1, 298, 1, 2, a2, a3, 0, 0);
                                *arg14 = 0;
                                return 0;
                            }
                        }
                        else {
                            if (a14) {
                                if (static_cast<uint8_t>(arg10) == 1) {
                                    pRenderPassList->Add(a1, 294, 1, 2, a2, a3, 0, 0);
                                    *arg14 = 0;
                                    return 0;
                                }
                            }
                            else {
                                if (static_cast<uint8_t>(arg10) == 1) {
                                    pRenderPassList->Add(a1, 289, 1, 2, a2, a3, 0, 0);
                                    *arg14 = 0;
                                    return 0;
                                }
                            }
                        }
                    }
                    else {
                        if (a13) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(a1, 287, 1, 2, a2, a3, 0, 0);
                                *arg14 = 0;
                                return 0;
                            }
                        }
                        else {
                            if (a14) {
                                if (static_cast<uint8_t>(arg10) == 1) {
                                    pRenderPassList->Add(a1, 283, 1, 2, a2, a3, 0, 0);
                                    *arg14 = 0;
                                    return 0;
                                }
                            }
                            else {
                                if (static_cast<uint8_t>(arg10) == 1) {
                                    pRenderPassList->Add(a1, 278, 1, 2, a2, a3, 0, 0);
                                    *arg14 = 0;
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            goto LABEL_70;
        }
        if (!a9) {
            if (a12) {
                if (a11) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 293, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 282, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
            }
            else {
                if (a11) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 290, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(a1, 279, 1, 2, a2, a3, 0, 0);
                        *arg14 = 0;
                        return 0;
                    }
                }
            }
        LABEL_70:;
            ++*a4;
        }
    }
LABEL_71:;
    *arg14 = 0;
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
                goto LABEL_61;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(pShape, 167, 1, 1, pLight, 0, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(pShape, 156, 1, 1, pLight, 0, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen || bParallax) {
                goto LABEL_61;
            }
            if (bShadows) {
                if (bHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 168, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 166, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 157, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 155, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        }
        goto LABEL_60;
    }
    if (bGlowMap) {
        if (!bFaceGen) {
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 165, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 154, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 161, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 150, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        LABEL_60:;
            ++*apusPassCount;
        }
    }
    else {
        if (!bFaceGen) {
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 164, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(pShape, 153, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(abAddPass) == 1) {
                            pRenderPassList->Add(pShape, 170, 1, 1, pLight, 0, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(pShape, 162, 1, 1, pLight, 0, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(pShape, 160, 1, 1, pLight, 0, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(abAddPass) == 1) {
                            pRenderPassList->Add(pShape, 159, 1, 1, pLight, 0, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(pShape, 151, 1, 1, pLight, 0, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(abAddPass) == 1) {
                                pRenderPassList->Add(pShape, 149, 1, 1, pLight, 0, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
            }
            goto LABEL_60;
        }
    }
LABEL_61:;
    *bEnable = 0;
    return 0;
}

int32_t BSShaderProperty::ADTS2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* a5,
    int32_t arg10,
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
                goto LABEL_61;
            }
            if (bShadows) {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(pShape, 189, 1, 2, pLight, pLight1, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(pShape, 178, 1, 2, pLight, pLight1, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
        }
        else {
            if (bFaceGen || bParallax) {
                goto LABEL_61;
            }
            if (bShadows) {
                if (bHair) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 190, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 188, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 179, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 177, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        }
        goto LABEL_60;
    }
    if (bGlowMap) {
        if (!bFaceGen) {
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 187, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 176, 1, 1, pLight, 0, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 183, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 172, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        LABEL_60:;
            ++*a5;
        }
    }
    else {
        if (!bFaceGen) {
            if (bParallax) {
                if (bShadows) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 186, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 175, 1, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bShadows) {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(pShape, 192, 1, 2, pLight, pLight1, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 184, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 182, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bTreeBranch) {
                        if (static_cast<uint8_t>(arg10) == 1) {
                            pRenderPassList->Add(pShape, 181, 1, 2, pLight, pLight1, 0, 0);
                            *bEnable = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bHair) {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 173, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(arg10) == 1) {
                                pRenderPassList->Add(pShape, 171, 1, 2, pLight, pLight1, 0, 0);
                                *bEnable = 0;
                                return 0;
                            }
                        }
                    }
                }
            }
            goto LABEL_60;
        }
    }
LABEL_61:;
    *bEnable = 0;
    return 0;
}

int32_t BSShaderProperty::DiffusePoint2(
    NiGeometry* a2,
    ShadowSceneLight* apLight0,
    ShadowSceneLight* apLight1,
    uint16_t* apusPassCount,
    int32_t abAddPass,
    bool* abEnablePass,
    bool abIsSkinned,
    bool abIsFaceGen,
    int32_t empty,
    bool abHasParallax,
    bool a12
) {
    if (!abIsSkinned) {
        if (abIsFaceGen) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(a2, 307, *abEnablePass, 2, apLight0, apLight1, 0, 0);
                *abEnablePass = 0;
                return 0;
            }
        }
        else {
            if (abHasParallax) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(a2, 308, *abEnablePass, 2, apLight0, apLight1, 0, 0);
                    *abEnablePass = 0;
                    return 0;
                }
            }
            else {
                if (a12) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(a2, 311, *abEnablePass, 2, apLight0, apLight1, 0, 0);
                        *abEnablePass = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(a2, 306, *abEnablePass, 2, apLight0, apLight1, 0, 0);
                        *abEnablePass = 0;
                        return 0;
                    }
                }
            }
        }
    LABEL_19:;
        ++*apusPassCount;
        goto LABEL_20;
    }
    if (abIsFaceGen) {
        if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(a2, 310, *abEnablePass, 2, apLight0, apLight1, 0, 0);
            *abEnablePass = 0;
            return 0;
        }
        goto LABEL_19;
    }
    if (!abHasParallax) {
        if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(a2, 309, *abEnablePass, 2, apLight0, apLight1, 0, 0);
            *abEnablePass = 0;
            return 0;
        }
        goto LABEL_19;
    }
LABEL_20:;
    *abEnablePass = 0;
    return 0;
}

int32_t BSShaderProperty::DiffusePoint3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* arg10,
    int32_t arg14,
    bool* bEnable,
    bool bSkinned,
    bool bFaceGen,
    int32_t empty,
    bool bParallax,
    bool bTreeBranch
) {
    if (!bSkinned) {
        if (bFaceGen) {
            if (static_cast<uint8_t>(arg14) == 1) {
                pRenderPassList->Add(pShape, 319, *bEnable, 3, pLight, pLight1, pLight2, 0);
                *bEnable = 0;
                return 0;
            }
        }
        else {
            if (bParallax) {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 320, *bEnable, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (bTreeBranch) {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 323, *bEnable, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 318, *bEnable, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        }
    LABEL_19:;
        ++*arg10;
        goto LABEL_20;
    }
    if (bFaceGen) {
        if (static_cast<uint8_t>(arg14) == 1) {
            pRenderPassList->Add(pShape, 322, *bEnable, 3, pLight, pLight1, pLight2, 0);
            *bEnable = 0;
            return 0;
        }
        goto LABEL_19;
    }
    if (!bParallax) {
        if (static_cast<uint8_t>(arg14) == 1) {
            pRenderPassList->Add(pShape, 321, *bEnable, 3, pLight, pLight1, pLight2, 0);
            *bEnable = 0;
            return 0;
        }
        goto LABEL_19;
    }
LABEL_20:;
    *bEnable = 0;
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
                pRenderPassList->Add(pShape, 339, *bEnable, 0, 0, 0, 0, 0);
            }
            else {
                ++*apusPassCount;
            }
        }
    }
    else {
        static_cast<BSShaderPPLightingProperty*>(this)->AddTexturePass_1x(
            pShape, pLight, *apusPassCount, abAddPass, *bEnable, bSkinned, empty, bFaceGen, bVertexColors, bTreeBranch,
            bHair
        );
    }
}

int32_t BSShaderProperty::SpecularPoint2(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    uint16_t* a5,
    int32_t arg10,
    bool* bEnable,
    bool bSkinned,
    bool bParallax,
    bool bTreeBranch,
    bool bHair
) {
    if (!bSkinned) {
        if (bParallax) {
            if (static_cast<uint8_t>(arg10) == 1) {
                pRenderPassList->Add(pShape, 366, *bEnable, 2, pLight, pLight1, 0, 0);
                *bEnable = 0;
                return 0;
            }
        }
        else {
            if (bTreeBranch) {
                if (static_cast<uint8_t>(arg10) == 1) {
                    pRenderPassList->Add(pShape, 369, *bEnable, 2, pLight, pLight1, 0, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 365, *bEnable, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg10) == 1) {
                        pRenderPassList->Add(pShape, 364, *bEnable, 2, pLight, pLight1, 0, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        }
    LABEL_19:;
        ++*a5;
        goto LABEL_20;
    }
    if (!bParallax) {
        if (bHair) {
            if (static_cast<uint8_t>(arg10) == 1) {
                pRenderPassList->Add(pShape, 368, *bEnable, 2, pLight, pLight1, 0, 0);
                *bEnable = 0;
                return 0;
            }
        }
        else {
            if (static_cast<uint8_t>(arg10) == 1) {
                pRenderPassList->Add(pShape, 367, *bEnable, 2, pLight, pLight1, 0, 0);
                *bEnable = 0;
                return 0;
            }
        }
        goto LABEL_19;
    }
LABEL_20:;
    *bEnable = 0;
    return 0;
}

int32_t BSShaderProperty::SpecularPoint3(
    NiGeometry* pShape,
    ShadowSceneLight* pLight,
    ShadowSceneLight* pLight1,
    ShadowSceneLight* pLight2,
    uint16_t* arg10,
    int32_t arg14,
    bool* bEnable,
    bool bSkinned,
    bool bParallax,
    bool bTreeBranch,
    bool bHair
) {
    if (!bSkinned) {
        if (bParallax) {
            if (static_cast<uint8_t>(arg14) == 1) {
                pRenderPassList->Add(pShape, 372, *bEnable, 3, pLight, pLight1, pLight2, 0);
                *bEnable = 0;
                return 0;
            }
        }
        else {
            if (bTreeBranch) {
                if (static_cast<uint8_t>(arg14) == 1) {
                    pRenderPassList->Add(pShape, 375, *bEnable, 3, pLight, pLight1, pLight2, 0);
                    *bEnable = 0;
                    return 0;
                }
            }
            else {
                if (bHair) {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 371, *bEnable, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(arg14) == 1) {
                        pRenderPassList->Add(pShape, 370, *bEnable, 3, pLight, pLight1, pLight2, 0);
                        *bEnable = 0;
                        return 0;
                    }
                }
            }
        }
    LABEL_19:;
        ++*arg10;
        goto LABEL_20;
    }
    if (!bParallax) {
        if (bHair) {
            if (static_cast<uint8_t>(arg14) == 1) {
                pRenderPassList->Add(pShape, 374, *bEnable, 3, pLight, pLight1, pLight2, 0);
                *bEnable = 0;
                return 0;
            }
        }
        else {
            if (static_cast<uint8_t>(arg14) == 1) {
                pRenderPassList->Add(pShape, 373, *bEnable, 3, pLight, pLight1, pLight2, 0);
                *bEnable = 0;
                return 0;
            }
        }
        goto LABEL_19;
    }
LABEL_20:;
    *bEnable = 0;
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
            if (bIsFaceGen || bHasParallax) {
                goto LABEL_97;
            }
            if (bHasShadow) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 274, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 262, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            goto LABEL_96;
        }
        if (bIsFaceGen) {
            if (bHasShadow) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 275, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 263, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            goto LABEL_96;
        }
        if (!bHasParallax) {
            if (bHasShadow) {
                if (bIsHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 277, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 273, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bIsHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 265, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 261, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_96;
        }
        goto LABEL_97;
    }
    if (abGlowMap) {
        if (!bIsFaceGen) {
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 271, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 259, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 268, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 255, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_96;
        }
    LABEL_97:;
        *abEnable = 0;
        return 0;
    }
    if (bIsFaceGen) {
        if (bHasShadow) {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(apGeometry, 269, 1, 1, pLight, 0, 0, 0);
                *abEnable = 0;
                return 0;
            }
        }
        else {
            if (static_cast<uint8_t>(abAddPass) == 1) {
                pRenderPassList->Add(apGeometry, 256, 1, 1, pLight, 0, 0, 0);
                *abEnable = 0;
                return 0;
            }
        }
        goto LABEL_96;
    }
    if (!bHasParallax) {
        if (bHasModelSpaceNormals) {
            if (bHasShadow) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 267, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 254, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            goto LABEL_96;
        }
        if (bHasShadow) {
            if (bIsBranch) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 276, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
            }
            else {
                if (bIsHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 272, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 266, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_96;
        }
        if (bIsBranch) {
            if (!bIsTree) {
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    pRenderPassList->Add(apGeometry, 264, 1, 1, pLight, 0, 0, 0);
                    *abEnable = 0;
                    return 0;
                }
                goto LABEL_96;
            }
        }
        else {
            if (!bIsTree) {
                if (bIsHair) {
                    if (static_cast<uint8_t>(abAddPass) == 1) {
                        pRenderPassList->Add(apGeometry, 260, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                    goto LABEL_96;
                }
                if (static_cast<uint8_t>(abAddPass) == 1) {
                    NiMaterialProperty* pMaterial =
                        static_cast<NiMaterialProperty*>(apGeometry->m_PropertyState.m_aspProps[2].m_pObject);
                    BSShaderProperty* v20 =
                        static_cast<BSShaderProperty*>(apGeometry->m_PropertyState.m_aspProps[3].m_pObject);
                    if (!BSShaderManager::bUseOptPasses) {
                        goto LABEL_36;
                    }
                    if (pMaterial) {
                        // EXE-INLINED none: NiMaterialProperty::GetEmittance
                        const NiColor* pEmit = pMaterial->m_external_emit;
                        if (!pEmit) {
                            pEmit = &pMaterial->m_emit;
                        }
                        if (!(*pEmit == NiColor_BLACK)) {
                            goto LABEL_36;
                        }
                    }
                    if (!(v20->fAlpha < 1.0f) && v20->GetFlag(37u)) {
                        bool Flag = v20->GetFlag(34u);
                        pRenderPassList->Add(apGeometry, Flag + BSShaderManager::BSSM_ADT_Opt, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                    else {
                    LABEL_36:;
                        pRenderPassList->Add(apGeometry, 251, 1, 1, pLight, 0, 0, 0);
                        *abEnable = 0;
                        return 0;
                    }
                }
            LABEL_96:;
                ++*apusPassCount;
                goto LABEL_97;
            }
        }
        if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, 565, 1, 1, pLight, 0, 0, 0);
            *abEnable = 0;
            return 0;
        }
        goto LABEL_96;
    }
    if (bHasShadow) {
        if (static_cast<uint8_t>(abAddPass) == 1) {
            pRenderPassList->Add(apGeometry, 270, 1, 1, pLight, 0, 0, 0);
            *abEnable = 0;
            return 0;
        }
        goto LABEL_96;
    }
    if (static_cast<uint8_t>(abAddPass) != 1) {
        goto LABEL_96;
    }
    NiMaterialProperty* v23 = static_cast<NiMaterialProperty*>(apGeometry->m_PropertyState.m_aspProps[2].m_pObject);
    BSShaderProperty* v24 = static_cast<BSShaderProperty*>(apGeometry->m_PropertyState.m_aspProps[3].m_pObject);
    if (!BSShaderManager::bUseOptPasses) {
        goto LABEL_67;
    }
    if (v23) {
        // EXE-INLINED none: NiMaterialProperty::GetEmittance
        const NiColor* pEmit = v23->m_external_emit;
        if (!pEmit) {
            pEmit = &v23->m_emit;
        }
        if (!(*pEmit == NiColor_BLACK)) {
            goto LABEL_67;
        }
    }
    if (v24->GetFlag(37u) && !(v24->fFadeAlpha < 1.0f)) {
        pRenderPassList->Add(apGeometry, 258, 1, 1, pLight, 0, 0, 0);
        *abEnable = 0;
        return 0;
    }
    else {
    LABEL_67:;
        pRenderPassList->Add(apGeometry, 257, 1, 1, pLight, 0, 0, 0);
        *abEnable = 0;
        return 0;
    }
}

int32_t BSShaderProperty::Specular(
    NiGeometry* a2,
    ShadowSceneLight* pLight,
    uint16_t* a4,
    int32_t a5,
    bool* arg10,
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
    if (NiColor_BLACK.r == r && (NiColor_BLACK.g == p_m_kDiff->g && NiColor_BLACK.b == p_m_kDiff->b)) {
        return 0;
    }
    if (!bIsSkinned) {
        if (bPointLight) {
            if (bHasParallax) {
                if (static_cast<uint8_t>(a5) == 1) {
                    pRenderPassList->Add(a2, 360, *arg10, 1, pLight, 0, 0, 0);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (bIsBranch) {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 363, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
                else {
                    if (bIsHair) {
                        if (static_cast<uint8_t>(a5) == 1) {
                            pRenderPassList->Add(a2, 359, *arg10, 1, pLight, 0, 0, 0);
                            *arg10 = 0;
                            return 0;
                        }
                    }
                    else {
                        if (static_cast<uint8_t>(a5) == 1) {
                            pRenderPassList->Add(a2, 358, *arg10, 1, pLight, 0, 0, 0);
                            *arg10 = 0;
                            return 0;
                        }
                    }
                }
            }
        }
        else {
            if (bHasParallax) {
                if (bHasShadow) {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 354, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 348, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bHasShadow) {
                    if (bIsBranch) {
                        if (static_cast<uint8_t>(a5) == 1) {
                            pRenderPassList->Add(a2, 357, *arg10, 1, pLight, 0, 0, 0);
                            *arg10 = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bIsHair) {
                            if (static_cast<uint8_t>(a5) == 1) {
                                pRenderPassList->Add(a2, 353, *arg10, 1, pLight, 0, 0, 0);
                                *arg10 = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(a5) == 1) {
                                pRenderPassList->Add(a2, 352, *arg10, 1, pLight, 0, 0, 0);
                                *arg10 = 0;
                                return 0;
                            }
                        }
                    }
                }
                else {
                    if (bIsBranch) {
                        if (static_cast<uint8_t>(a5) == 1) {
                            pRenderPassList->Add(a2, 351, *arg10, 1, pLight, 0, 0, 0);
                            *arg10 = 0;
                            return 0;
                        }
                    }
                    else {
                        if (bIsHair) {
                            if (static_cast<uint8_t>(a5) == 1) {
                                pRenderPassList->Add(a2, 347, *arg10, 1, pLight, 0, 0, 0);
                                *arg10 = 0;
                                return 0;
                            }
                        }
                        else {
                            if (static_cast<uint8_t>(a5) == 1) {
                                pRenderPassList->Add(a2, 346, *arg10, 1, pLight, 0, 0, 0);
                                *arg10 = 0;
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    LABEL_59:;
        ++*a4;
        goto LABEL_60;
    }
    if (bPointLight) {
        if (!bHasParallax) {
            if (bIsHair) {
                if (static_cast<uint8_t>(a5) == 1) {
                    pRenderPassList->Add(a2, 362, *arg10, 1, pLight, 0, 0, 0);
                    *arg10 = 0;
                    return 0;
                }
            }
            else {
                if (static_cast<uint8_t>(a5) == 1) {
                    pRenderPassList->Add(a2, 361, *arg10, 1, pLight, 0, 0, 0);
                    *arg10 = 0;
                    return 0;
                }
            }
            goto LABEL_59;
        }
    }
    else {
        if (!bHasParallax) {
            if (bHasShadow) {
                if (bIsHair) {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 356, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 355, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
            }
            else {
                if (bIsHair) {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 350, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
                else {
                    if (static_cast<uint8_t>(a5) == 1) {
                        pRenderPassList->Add(a2, 349, *arg10, 1, pLight, 0, 0, 0);
                        *arg10 = 0;
                        return 0;
                    }
                }
            }
            goto LABEL_59;
        }
    }
LABEL_60:;
    *arg10 = 0;
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
                property->SetMotionBlurred(((int32_t)(property->ulFlags[0] & 16)) == 0);
            }
        }
        else {
            v2 = apObject->IsNode();
            v3 = v2;
            if (v2) {
                m_usSize = (uint32_t)v2->m_kChildren.m_usSize;
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
    BSShaderProperty* pObject = static_cast<BSShaderProperty*>(NiMemObject::operator new(sizeof(BSShaderProperty)));
    if (pObject) {
        return ::new (pObject) BSShaderProperty;
    }
    return nullptr;
}
