#include "Fallout Shared/ScreenCustomSplatter.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderBloodSplatter.h"
#include "BSShader/BSShaderBloodSplatterProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BSCustomSplatterExtraData.h"
#include "Fallout Shared/BSSplatterData.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cstdint>

// The globals this file defines, in their initialization order.
NiRTTI BSCustomSplatterExtraData::ms_RTTI("BSCustomSplatterExtraData", &NiExtraData::ms_RTTI);
SettingT<INISettingCollection> bCustomSplatterEnabled("bCustomSplatterEnabled:ScreenSplatter", true);
NiPointer<NiCamera> ScreenCustomSplatter::spCamera;
NiPointer<NiNode> ScreenCustomSplatter::spRoot;
ScreenCustomSplatter::spTextures_t ScreenCustomSplatter::spTextures;

float RandomFloatNeg1To1() {
    BSRandom* singleton = BSRandom::GetSingleton();
    return singleton->FloatNeg1To1();
}

bool RandomBool(float afPercentTrue) {
    BSRandom* singleton = BSRandom::GetSingleton();
    return singleton->Bool(afPercentTrue);
}

char SetCustomSplatterFade(float a1, bool a2) {
    fBloodSplatterFadeStart_0 = a1;
    ScreenCustomSplatter::bNoFade = a2;
    return 1;
}

void CustomBloodSplatter() {
    if (ScreenCustomSplatter::bEnabled) {
        ScreenCustomSplatter::spRoot.m_pObject->RemoveAllChildren();
        ScreenCustomSplatter::uiCount = 0;
        fBloodSplatterFadeStart_0 = 0.0f;
        ScreenCustomSplatter::bNoFade = false;
        const char* v0 = sBloodSplatterAlpha01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[1], v0);
        const char* v1 = sBloodSplatterColor01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(ScreenCustomSplatter::spTextures, v1);
        if (BSShaderManager::bHDR) {
            const char* v2 = sBloodSplatterFlare01Filename.String();
            SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[2], v2);
        }
    }
}

// Advances the custom splatters.
void ScreenCustomSplatter::Update() {
    if (ScreenCustomSplatter::bEnabled) {
        uint32_t uichildCount = ScreenCustomSplatter::spRoot.m_pObject->GetArrayCount();
        float felapsedtime = appTimer.fDelta;
        for (uint32_t ui = 0; ui < uichildCount; ++ui) {
            NiAVObject* pchild = ScreenCustomSplatter::spRoot.m_pObject->GetAt(ui);
            if (pchild) {
                const NiFixedString* v0 = &BSCustomSplatterExtraData::Tag();
                BSCustomSplatterExtraData* ped = static_cast<BSCustomSplatterExtraData*>(pchild->GetExtraData(*v0));
                BSSplatterData* splatterData = &ped->SplatterData;
                splatterData->fAge = splatterData->fAge + felapsedtime;
                if (!(splatterData->fDuration <= static_cast<double>(splatterData->fAge))) {
                    if (ScreenCustomSplatter::bNoFade ||
                        !(fBloodSplatterFadeStart_0 <= splatterData->fAge / splatterData->fDuration)) {
                        if (ScreenCustomSplatter::bNoFade &&
                            splatterData->fAge >= splatterData->fDuration - fBloodSplatterFadeStart_0) {
                            float fComputedFadePct =
                                (splatterData->fDuration - fBloodSplatterFadeStart_0) / splatterData->fDuration;
                            float v8 = fComputedFadePct * splatterData->fDuration;
                            double v3 = FLerp(0.0f, 1.0f, v8, splatterData->fDuration, splatterData->fAge);
                            splatterData->fAlpha = 1.0 - v3;
                        }
                    }
                    else {
                        float value = splatterData->fAge;
                        float y2 = splatterData->fDuration;
                        float* v1 = &fBloodSplatterFadeStart.Float();
                        float v9 = *v1 * splatterData->fDuration;
                        double v2 = FLerp(0.0f, 1.0f, v9, y2, value);
                        splatterData->fAlpha = 1.0 - v2;
                    }
                    NiUpdateData UpdateData(splatterData->fAge, true, false);
                    pchild->Update(UpdateData);
                }
                else {
                    ScreenCustomSplatter::uiCount -= splatterData->uiCount;
                    NiNode* v10 = ScreenCustomSplatter::spRoot.m_pObject;
                    v10->DetachChildAt(ui);
                }
            }
        }
        float* v4 = &fBloodSplatterFlareOffsetScale.Float();
        BSShaderBloodSplatter::SetFlareOffsetScale(*v4);
        float* v5 = &fBloodSplatterFlareMult.Float();
        BSShaderBloodSplatter::SetFlareMult(*v5);
        float fglobalAlpha = BSShaderBloodSplatter::GetGlobalAlpha();
        if (fglobalAlpha < 1.0) {
            fglobalAlpha = felapsedtime / 0.4 + fglobalAlpha;
            fglobalAlpha = BSMIN(1.0f, fglobalAlpha);
            BSShaderBloodSplatter::SetGlobalAlpha(fglobalAlpha);
        }
    }
}

void ScreenCustomSplatter::Activate(
    uint32_t auiCount,
    float afDuration,
    ScreenCustomSplatterType aeType,
    float fSplatScale,
    float fOpacityScale
) {
    BSSplatterData splatterData;
    if ((ScreenCustomSplatter::bEnabled) && (auiCount)) {
        BSCustomSplatterExtraData* ped;
        NiNode* pnode = ScreenCustomSplatter::CreateGeometry(auiCount, fSplatScale, fOpacityScale);
        ScreenCustomSplatter::uiCount += auiCount;
        splatterData.fAlpha = 1.0f;
        splatterData.fAge = 0.0;
        if (!(afDuration > 0.0)) {
            splatterData.fDuration = fBloodSplatterDuration.Float();
        }
        else {
            splatterData.fDuration = afDuration;
        }
        splatterData.uiCount = auiCount;
        void* apMemory = NiMemObject::operator new(sizeof(BSCustomSplatterExtraData));
        if (apMemory) {
            ped = ::new (apMemory) BSCustomSplatterExtraData(&splatterData);
        }
        else {
            ped = nullptr;
        }
        pnode->AddExtraData(ped);
        pnode->SetName("ScreenSplatterRoot");
        NiNode* pRootNode = ScreenCustomSplatter::spRoot.m_pObject;
        pRootNode->AttachChild(pnode, 1);
        float* BroadPhaseHandle = &ped->SplatterData.fAlpha;
        BSShaderBloodSplatterProperty::SetFadeSourceRecurse(pnode, BroadPhaseHandle);
        NiUpdateData UpdateData(0.0f, false, false);
        ScreenCustomSplatter::spRoot.m_pObject->Update(UpdateData);
    }
}

void ScreenCustomSplatter::Init() {
    NiPoint3 dst;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ScreenCustomSplatter.cpp", 189
    );
    ScreenCustomSplatter::bEnabled = bCustomSplatterEnabled.Bool();
    // Tests the whole copied byte, not its low bit.
    if (static_cast<uint8_t>(ScreenCustomSplatter::bEnabled)) {
        NiNode* v4;
        NiCamera* NiCamera_;
        NiRenderer* prenderer = NiRenderer::GetRenderer();
        double ScreenWidth = static_cast<double>(prenderer->GetScreenWidth());
        float faspectratio = ScreenWidth / static_cast<double>(prenderer->GetScreenHeight());
        NiCamera* v9 = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
        if (v9) {
            NiCamera_ = ::new (v9) NiCamera;
        }
        else {
            NiCamera_ = nullptr;
        }
        NiRefObject* v10 = NiCamera_;
        ScreenCustomSplatter::spCamera.operator=(NiCamera_);
        NiPoint3* v0 = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.00001f);
        ScreenCustomSplatter::spCamera.m_pObject->SetTranslate(*v0);
        NiUpdateData UpdateData1(0.0f, false, false);
        ScreenCustomSplatter::spCamera.m_pObject->Update(UpdateData1);
        ScreenCustomSplatter::spCamera.m_pObject->LookAtWorldPoint(NiPoint3_ZERO, NiPoint3_UNIT_Y);
        NiFrustum frustum(false);
        frustum.m_bOrtho = false;
        frustum.m_fTop = 0.5f;
        frustum.m_fBottom = -0.5f;
        frustum.m_fLeft = faspectratio * -0.5;
        frustum.m_fRight = faspectratio * 0.5;
        frustum.m_fFar = 10000.0f;
        frustum.m_fNear = 1.0f;
        ScreenCustomSplatter::spCamera.m_pObject->SetViewFrustum(frustum);
        NiUpdateData UpdateData2(0.0f, false, false);
        ScreenCustomSplatter::spCamera.m_pObject->Update(UpdateData2);
        ScreenCustomSplatter::fWRadius = frustum.m_fRight;
        ScreenCustomSplatter::fHRadius = frustum.m_fTop;
        fBloodSplatterFadeStart_0 = fBloodSplatterFadeStart.Float();
        ScreenCustomSplatter::bNoFade = false;
        NiNode* v7 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        if (v7) {
            v4 = ::new (v7) NiNode(0);
        }
        else {
            v4 = nullptr;
        }
        ScreenCustomSplatter::spRoot.operator=(v4);
        ScreenCustomSplatter::uiCount = 0;
        const char* v1 = sBloodSplatterAlpha01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[1], v1);
        const char* v2 = sBloodSplatterColor01OPTFilename.String();
        SkyTextureManager::LoadTextureInBackground(ScreenCustomSplatter::spTextures, v2);
        if (BSShaderManager::bHDR) {
            const char* v3 = sBloodSplatterFlare01Filename.String();
            SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[2], v3);
        }
    }
}

NiCamera* ScreenCustomSplatter::GetCamera() {
    return (&ScreenCustomSplatter::spCamera)->operator->();
}

bool ScreenCustomSplatter::IsActiveSplatter() {
    if (!(&ScreenCustomSplatter::spRoot)->operator->()) {
        return false;
    }
    NiNode* v1 = (&ScreenCustomSplatter::spRoot)->operator->();
    return (v1->GetChildCount() != 0);
}

NiNode* ScreenCustomSplatter::GetCurrentRoot() {
    return (&ScreenCustomSplatter::spRoot)->operator->();
}

NiNode* ScreenCustomSplatter::CreateGeometry(uint32_t auiCount, float fSplatScale, float fOpacityScale) {
    BSShaderBloodSplatterProperty* pshaderprop;
    NiTriShape* pgeom;
    NiPoint2* ptexcoordsA;
    NiPoint3* pverticesA;
    NiNode* pnode;
    NiPoint2 v103;
    NiPoint2 v104;
    NiPoint2 v105;
    NiPoint2 v106;
    NiPoint2 v107;
    NiPoint2 v108;
    NiPoint2 v109;
    NiPoint2 v110;
    NiPoint2 v111;
    NiPoint2 v112;
    NiPoint2 v113;
    NiPoint2 v114;
    NiPoint3 v117;
    NiPoint3 v120;
    NiPoint3 v123;
    NiPoint3 v126;
    NiPoint3 v129;
    NiPoint3 v132;
    NiPoint3 v135;
    NiPoint3 v138;
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        pnode = ::new (apMemory) NiNode(0);
    }
    else {
        pnode = nullptr;
    }
    int32_t numObjs = 4 * auiCount;
    void* ptr = ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (4 * auiCount) : SIZE_MAX);
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), numObjs, &emptyPointConstructor);
        pverticesA = static_cast<NiPoint3*>(ptr);
    }
    else {
        pverticesA = nullptr;
    }
    int32_t count = 4 * auiCount;
    void* v147 = ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * (4 * auiCount) : SIZE_MAX);
    if (v147) {
        rt::eh_vector_constructor_iterator(v147, sizeof(NiPoint2), count, &emptyPointConstructor);
        ptexcoordsA = static_cast<NiPoint2*>(v147);
    }
    else {
        ptexcoordsA = nullptr;
    }
    uint16_t* v146 = static_cast<uint16_t*>(
        ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
    );
    NiPoint3* pflareVerticesA_2 = nullptr;
    NiPoint2* pflareTexcoordsA_2 = nullptr;
    uint16_t* pflareIndicesA = nullptr;
    if (BSShaderManager::bHDR) {
        NiPoint2* pflareTexcoordsA;
        NiPoint3* pflareVerticesA;
        int32_t aiCount = 4 * auiCount;
        void* v143 =
            ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (4 * auiCount) : SIZE_MAX);
        if (v143) {
            rt::eh_vector_constructor_iterator(v143, sizeof(NiPoint3), aiCount, &emptyPointConstructor);
            pflareVerticesA = static_cast<NiPoint3*>(v143);
        }
        else {
            pflareVerticesA = nullptr;
        }
        pflareVerticesA_2 = pflareVerticesA;
        int32_t v142 = 4 * auiCount;
        void* v140 =
            ourMalloc(4 * auiCount <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * (4 * auiCount) : SIZE_MAX);
        if (v140) {
            rt::eh_vector_constructor_iterator(v140, sizeof(NiPoint2), v142, &emptyPointConstructor);
            pflareTexcoordsA = static_cast<NiPoint2*>(v140);
        }
        else {
            pflareTexcoordsA = nullptr;
        }
        pflareTexcoordsA_2 = pflareTexcoordsA;
        uint16_t* v139 = static_cast<uint16_t*>(
            ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
        );
        pflareIndicesA = v139;
    }
    for (uint32_t i = 0; i < auiCount; ++i) {
        double fvoffset;
        double fuoffset;
        NiMatrix3 rotationMatrix;
        float fx = RandomFloatNeg1To1() * ScreenCustomSplatter::fWRadius;
        float fy = RandomFloatNeg1To1() * ScreenCustomSplatter::fHRadius;
        NiPoint3 translate(fx, fy, 0.0f);
        float fBloodSplatterMaxSize = ::fBloodSplatterMaxSize.Float();
        float* v3 = &fBloodSplatterMinSize.Float();
        float fscale = RandomFloat(*v3, fBloodSplatterMaxSize);
        fscale = fscale * fSplatScale;
        float fBloodSplatterMaxSizea = fBloodSplatterMaxOpacity.Float();
        float* v4 = &fBloodSplatterMinOpacity.Float();
        float falpha1 = RandomFloat(*v4, fBloodSplatterMaxSizea);
        float fBloodSplatterMaxSizeb = fBloodSplatterMaxOpacity2.Float();
        float* v5 = &fBloodSplatterMinOpacity2.Float();
        float falpha2 = RandomFloat(*v5, fBloodSplatterMaxSizeb);
        falpha1 = falpha1 * fOpacityScale;
        falpha2 = falpha2 * fOpacityScale;
        rotationMatrix.MakeIdentity();
        float* v6 = &fBloodSplatterOpacityChance.Float();
        if (RandomBool(*v6)) {
            falpha1 = falpha2;
        }
        if (!(RandomFloatNeg1To1() > 0.0)) {
            fuoffset = 0.5;
        }
        else {
            fuoffset = 0.0;
        }
        float fuoffset_2 = fuoffset;
        if (!(RandomFloatNeg1To1() > 0.0)) {
            fvoffset = 0.5;
        }
        else {
            fvoffset = 0.0;
        }
        float fvoffset_2 = fvoffset;
        NiPoint2 texTranslate(fuoffset_2, fvoffset_2);
        float fflareSize = fBloodSplatterFlareSize.Float();
        uint32_t uibase = 4 * i;
        NiPoint3* v7 = ::new (&v138) NiPoint3(-fscale, fscale, falpha1);
        pverticesA[uibase] = rotationMatrix * *v7 + translate;
        NiPoint3* v10 = ::new (&v135) NiPoint3(fscale, fscale, falpha1);
        pverticesA[uibase + 1] = rotationMatrix * *v10 + translate;
        float fY = -fscale;
        NiPoint3* v13 = ::new (&v132) NiPoint3(fY, fY, falpha1);
        pverticesA[uibase + 2] = rotationMatrix * *v13 + translate;
        NiPoint3* v16 = ::new (&v129) NiPoint3(fscale, -fscale, falpha1);
        pverticesA[uibase + 3] = rotationMatrix * *v16 + translate;
        if (BSShaderManager::bHDR) {
            NiPoint3* v19 = ::new (&v126) NiPoint3(-fflareSize, fflareSize, falpha1);
            pflareVerticesA_2[uibase] = rotationMatrix * *v19 + translate;
            NiPoint3* v22 = ::new (&v123) NiPoint3(fflareSize, fflareSize, falpha1);
            pflareVerticesA_2[uibase + 1] = rotationMatrix * *v22 + translate;
            float fYb = -fflareSize;
            NiPoint3* v25 = ::new (&v120) NiPoint3(fYb, fYb, falpha1);
            pflareVerticesA_2[uibase + 2] = rotationMatrix * *v25 + translate;
            NiPoint3* v28 = ::new (&v117) NiPoint3(fflareSize, -fflareSize, falpha1);
            pflareVerticesA_2[uibase + 3] = rotationMatrix * *v28 + translate;
        }
        NiPoint2* v31 = ::new (&v114) NiPoint2(0.5f, 0.5f);
        NiPoint2* v32 = ::new (&v113) NiPoint2(*v31 + texTranslate);
        float x = v32->x;
        float y = v32->y;
        uint32_t v35 = uibase;
        ptexcoordsA[uibase].x = x;
        ptexcoordsA[v35].y = y;
        NiPoint2* v37 = ::new (&v112) NiPoint2(0.0f, 0.5f);
        NiPoint2* v38 = ::new (&v111) NiPoint2(*v37 + texTranslate);
        float v39 = v38->x;
        float v40 = v38->y;
        uint32_t v41 = uibase;
        ptexcoordsA[uibase + 1].x = v39;
        ptexcoordsA[v41 + 1].y = v40;
        NiPoint2* v43 = ::new (&v110) NiPoint2(0.5f, 0.0f);
        NiPoint2* v44 = ::new (&v109) NiPoint2(*v43 + texTranslate);
        float v45 = v44->x;
        float v46 = v44->y;
        uint32_t v47 = uibase;
        ptexcoordsA[uibase + 2].x = v45;
        ptexcoordsA[v47 + 2].y = v46;
        NiPoint2* v49 = ::new (&v108) NiPoint2(0.0f, 0.0f);
        NiPoint2* v50 = ::new (&v107) NiPoint2(*v49 + texTranslate);
        float v51 = v50->x;
        float v52 = v50->y;
        uint32_t v53 = uibase;
        ptexcoordsA[uibase + 3].x = v51;
        ptexcoordsA[v53 + 3].y = v52;
        if (BSShaderManager::bHDR) {
            NiPoint2* v55 = ::new (&v106) NiPoint2(1.0f, 1.0f);
            float v56 = v55->x;
            float v57 = v55->y;
            uint32_t v58 = uibase;
            pflareTexcoordsA_2[uibase].x = v56;
            pflareTexcoordsA_2[v58].y = v57;
            NiPoint2* v60 = ::new (&v105) NiPoint2(0.0f, 1.0f);
            float v61 = v60->x;
            float v62 = v60->y;
            uint32_t v63 = uibase;
            pflareTexcoordsA_2[uibase + 1].x = v61;
            pflareTexcoordsA_2[v63 + 1].y = v62;
            NiPoint2* v65 = ::new (&v104) NiPoint2(1.0f, 0.0f);
            float v66 = v65->x;
            float v67 = v65->y;
            uint32_t v68 = uibase;
            pflareTexcoordsA_2[uibase + 2].x = v66;
            pflareTexcoordsA_2[v68 + 2].y = v67;
            NiPoint2* v70 = ::new (&v103) NiPoint2(0.0f, 0.0f);
            float v71 = v70->x;
            float v72 = v70->y;
            uint32_t v73 = uibase;
            pflareTexcoordsA_2[uibase + 3].x = v71;
            pflareTexcoordsA_2[v73 + 3].y = v72;
        }
        uint32_t uiindexBase = 6 * i;
        v146[6 * i] = uibase;
        v146[uiindexBase + 1] = uibase + 2;
        v146[uiindexBase + 2] = uibase + 1;
        v146[uiindexBase + 3] = uibase + 1;
        v146[uiindexBase + 4] = uibase + 2;
        v146[uiindexBase + 5] = uibase + 3;
        if (BSShaderManager::bHDR) {
            pflareIndicesA[uiindexBase] = uibase;
            pflareIndicesA[uiindexBase + 1] = uibase + 2;
            pflareIndicesA[uiindexBase + 2] = uibase + 1;
            pflareIndicesA[uiindexBase + 3] = uibase + 1;
            pflareIndicesA[uiindexBase + 4] = uibase + 2;
            pflareIndicesA[uiindexBase + 5] = uibase + 3;
        }
    }
    NiTriShape* v102 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v102) {
        pgeom = ::new (v102) NiTriShape(
            4 * auiCount, pverticesA, nullptr, nullptr, ptexcoordsA, 1, NiGeometryData::NBT_METHOD_NONE, 2 * auiCount,
            v146
        );
    }
    else {
        pgeom = nullptr;
    }
    NiTriShape* pgeom_2 = pgeom;
    BSShaderBloodSplatterProperty* v101 =
        static_cast<BSShaderBloodSplatterProperty*>(NiMemObject::operator new(sizeof(BSShaderBloodSplatterProperty)));
    if (v101) {
        pshaderprop = ::new (v101) BSShaderBloodSplatterProperty;
    }
    else {
        pshaderprop = nullptr;
    }
    BSShaderBloodSplatterProperty* pshaderprop_2 = pshaderprop;
    NiTexture* fBloodSplatterMaxSizec = ScreenCustomSplatter::spTextures[1].m_pObject;
    pshaderprop_2->SetTexture(0, fBloodSplatterMaxSizec);
    NiTexture* fBloodSplatterMaxSized = ScreenCustomSplatter::spTextures->m_pObject;
    pshaderprop_2->SetTexture(1u, fBloodSplatterMaxSized);
    pgeom_2->AttachProperty(pshaderprop_2);
    BSShaderManager::PrepareObject(pgeom_2, 0, 0);
    pnode->AttachChild(pgeom_2, 1);
    if (BSShaderManager::bHDR) {
        BSShaderBloodSplatterProperty* pshaderprop_3;
        NiTriShape* pgeom_3;
        NiTriShape* v100 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
        if (v100) {
            pgeom_3 = ::new (v100) NiTriShape(
                4 * auiCount, pflareVerticesA_2, nullptr, nullptr, pflareTexcoordsA_2, 1,
                NiGeometryData::NBT_METHOD_NONE, 2 * auiCount, pflareIndicesA
            );
        }
        else {
            pgeom_3 = nullptr;
        }
        pgeom_2 = pgeom_3;
        BSShaderBloodSplatterProperty* v99 = static_cast<BSShaderBloodSplatterProperty*>(
            NiMemObject::operator new(sizeof(BSShaderBloodSplatterProperty))
        );
        if (v99) {
            pshaderprop_3 = ::new (v99) BSShaderBloodSplatterProperty;
        }
        else {
            pshaderprop_3 = nullptr;
        }
        pshaderprop_2 = pshaderprop_3;
        NiTexture* fBloodSplatterMaxSizee = ScreenCustomSplatter::spTextures[2].m_pObject;
        pshaderprop_2->SetTexture(0, fBloodSplatterMaxSizee);
        pgeom_2->AttachProperty(pshaderprop_2);
        BSShaderManager::PrepareObject(pgeom_2, 0, 0);
        pnode->AttachChild(pgeom_2, 1);
    }
    return pnode;
}

char ScreenCustomSplatter::SetCustomSplatterTexture(char* a1, uint8_t arg4) {
    if (arg4 > 2u) {
        return 0;
    }
    SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[arg4], a1);
    return 1;
}

void ScreenCustomSplatter::Shutdown() {
    if (ScreenCustomSplatter::bEnabled) {
        ScreenCustomSplatter::spRoot.m_pObject->RemoveAllChildren();
        ScreenCustomSplatter::spRoot.operator=(static_cast<NiNode*>(nullptr));
        ScreenCustomSplatter::spCamera.operator=(static_cast<NiCamera*>(nullptr));
    }
    ScreenCustomSplatter::bEnabled = false;
    for (int32_t i = 0; i < 3; ++i) {
        ScreenCustomSplatter::spTextures[i].operator=(static_cast<NiTexture*>(nullptr));
    }
}

void BSShaderBloodSplatterProperty::SetTexture(uint32_t auiIndex, NiTexture* apTex) {
    spTexture[auiIndex] = apTex;
}

// The tag string goes to the base constructor by address, and the splatter data is copied whole.
BSCustomSplatterExtraData::BSCustomSplatterExtraData(BSSplatterData* aSplatterData)
  : NiExtraData(&Tag()), SplatterData(*aSplatterData) {}

BSCustomSplatterExtraData::~BSCustomSplatterExtraData() {}

void BSCustomSplatterExtraData::InitTag() {
    NiFixedString* v0;
    NiFixedString* v1 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
    if (v1) {
        v0 = ::new (static_cast<void*>(v1)) NiFixedString("CSD");
    }
    else {
        v0 = nullptr;
    }
    BSCustomSplatterExtraData::pTag = v0;
}

// The "CSD" tag string.
const NiFixedString& BSCustomSplatterExtraData::Tag() {
    return *BSCustomSplatterExtraData::pTag;
}
