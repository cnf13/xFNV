#include "Fallout Shared/ScreenCustomSplatter.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <cstdint>

// The globals this file defines, in their initialization order.
NiRTTI BSCustomSplatterExtraData::ms_RTTI("BSCustomSplatterExtraData", &NiExtraData::ms_RTTI);
SettingT<INISettingCollection> bCustomSplatterEnabled("bCustomSplatterEnabled:ScreenSplatter", true);
NiPointer<NiCamera> ScreenCustomSplatter::spCamera;
NiPointer<NiNode> ScreenCustomSplatter::spRoot;
ScreenCustomSplatter::spTextures_t ScreenCustomSplatter::spTextures;

char SetCustomSplatterFade(float afFadeStart, bool abNoFade) {
    fBloodSplatterFadeStart_0 = afFadeStart;
    ScreenCustomSplatter::bNoFade = abNoFade;
    return 1;
}

void CustomBloodSplatter() {
    if (ScreenCustomSplatter::bEnabled) {
        ScreenCustomSplatter::spRoot->RemoveAllChildren();
        ScreenCustomSplatter::uiCount = 0;
        fBloodSplatterFadeStart_0 = 0.0f;
        ScreenCustomSplatter::bNoFade = false;
        SkyTextureManager::LoadTextureInBackground(
            &ScreenCustomSplatter::spTextures[1],
            sBloodSplatterAlpha01OPTFilename.String()
        );
        SkyTextureManager::LoadTextureInBackground(
            ScreenCustomSplatter::spTextures,
            sBloodSplatterColor01OPTFilename.String()
        );
        if (BSShaderManager::bHDR) {
            SkyTextureManager::LoadTextureInBackground(
                &ScreenCustomSplatter::spTextures[2],
                sBloodSplatterFlare01Filename.String()
            );
        }
    }
}

// Advances the custom splatters.
void ScreenCustomSplatter::Update() {
    if (ScreenCustomSplatter::bEnabled) {
        uint32_t uichildCount = ScreenCustomSplatter::spRoot->GetArrayCount();
        float felapsedtime = appTimer.fDelta;
        for (uint32_t ui = 0; ui < uichildCount; ++ui) {
            NiAVObject* pchild = ScreenCustomSplatter::spRoot->GetAt(ui);
            if (pchild) {
                BSCustomSplatterExtraData* ped = static_cast<BSCustomSplatterExtraData*>(
                    pchild->GetExtraData(BSCustomSplatterExtraData::Tag())
                );
                BSSplatterData* splatterData = &ped->SplatterData;
                splatterData->fAge = splatterData->fAge + felapsedtime;
                if (splatterData->fDuration <= static_cast<double>(splatterData->fAge)) {
                    ScreenCustomSplatter::uiCount -= splatterData->uiCount;
                    ScreenCustomSplatter::spRoot->DetachChildAt(ui);
                }
                else {
                    if (ScreenCustomSplatter::bNoFade ||
                        !(fBloodSplatterFadeStart_0 <= splatterData->fAge / splatterData->fDuration)) {
                        if (ScreenCustomSplatter::bNoFade &&
                            splatterData->fAge >= splatterData->fDuration - fBloodSplatterFadeStart_0) {
                            float fComputedFadePct = (splatterData->fDuration - fBloodSplatterFadeStart_0) /
                                splatterData->fDuration;
                            float fFadeStartAge = fComputedFadePct * splatterData->fDuration;
                            double
                                dFade = FLerp(0.0f, 1.0f, fFadeStartAge, splatterData->fDuration, splatterData->fAge);
                            splatterData->fAlpha = 1.0 - dFade;
                        }
                    }
                    else {
                        float fAge = splatterData->fAge;
                        float fDuration = splatterData->fDuration;
                        float fFadeStartAge = fBloodSplatterFadeStart.Float() * splatterData->fDuration;
                        double dFade = FLerp(0.0f, 1.0f, fFadeStartAge, fDuration, fAge);
                        splatterData->fAlpha = 1.0 - dFade;
                    }
                    NiUpdateData updateData(splatterData->fAge, true, false);
                    pchild->Update(updateData);
                }
            }
        }
        BSShaderBloodSplatter::SetFlareOffsetScale(fBloodSplatterFlareOffsetScale.Float());
        BSShaderBloodSplatter::SetFlareMult(fBloodSplatterFlareMult.Float());
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
    if (ScreenCustomSplatter::bEnabled && auiCount) {
        NiNode* pnode = ScreenCustomSplatter::CreateGeometry(auiCount, fSplatScale, fOpacityScale);
        ScreenCustomSplatter::uiCount += auiCount;
        splatterData.fAlpha = 1.0f;
        splatterData.fAge = 0.0;
        if (afDuration > 0.0) {
            splatterData.fDuration = afDuration;
        }
        else {
            splatterData.fDuration = fBloodSplatterDuration.Float();
        }
        splatterData.uiCount = auiCount;
        BSCustomSplatterExtraData* ped = new BSCustomSplatterExtraData(&splatterData);
        pnode->AddExtraData(ped);
        pnode->SetName("ScreenSplatterRoot");
        ScreenCustomSplatter::spRoot->AttachChild(pnode, 1);
        BSShaderBloodSplatterProperty::SetFadeSourceRecurse(pnode, &ped->SplatterData.fAlpha);
        NiUpdateData updateData(0.0f, false, false);
        ScreenCustomSplatter::spRoot->Update(updateData);
    }
}

void ScreenCustomSplatter::Init() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ScreenCustomSplatter.cpp",
        189
    );
    ScreenCustomSplatter::bEnabled = bCustomSplatterEnabled.Bool();
    // Tests the whole copied byte, not its low bit.
    if (static_cast<uint8_t>(ScreenCustomSplatter::bEnabled)) {
        NiRenderer* prenderer = NiRenderer::GetRenderer();
        double dScreenWidth = static_cast<double>(prenderer->GetScreenWidth());
        float faspectratio = dScreenWidth / static_cast<double>(prenderer->GetScreenHeight());
        ScreenCustomSplatter::spCamera = new NiCamera;
        ScreenCustomSplatter::spCamera->SetTranslate(NiPoint3(0.0f, 0.0f, 1.00001f));
        NiUpdateData UpdateData1(0.0f, false, false);
        ScreenCustomSplatter::spCamera->Update(UpdateData1);
        ScreenCustomSplatter::spCamera->LookAtWorldPoint(NiPoint3::ZERO, NiPoint3::UNIT_Y);
        NiFrustum frustum(false);
        frustum.m_bOrtho = false;
        frustum.m_fTop = 0.5f;
        frustum.m_fBottom = -0.5f;
        frustum.m_fLeft = faspectratio * -0.5;
        frustum.m_fRight = faspectratio * 0.5;
        frustum.m_fFar = 10000.0f;
        frustum.m_fNear = 1.0f;
        ScreenCustomSplatter::spCamera->SetViewFrustum(frustum);
        NiUpdateData UpdateData2(0.0f, false, false);
        ScreenCustomSplatter::spCamera->Update(UpdateData2);
        ScreenCustomSplatter::fWRadius = frustum.m_fRight;
        ScreenCustomSplatter::fHRadius = frustum.m_fTop;
        fBloodSplatterFadeStart_0 = fBloodSplatterFadeStart.Float();
        ScreenCustomSplatter::bNoFade = false;
        ScreenCustomSplatter::spRoot = new NiNode(0);
        ScreenCustomSplatter::uiCount = 0;
        SkyTextureManager::LoadTextureInBackground(
            &ScreenCustomSplatter::spTextures[1],
            sBloodSplatterAlpha01OPTFilename.String()
        );
        SkyTextureManager::LoadTextureInBackground(
            ScreenCustomSplatter::spTextures,
            sBloodSplatterColor01OPTFilename.String()
        );
        if (BSShaderManager::bHDR) {
            SkyTextureManager::LoadTextureInBackground(
                &ScreenCustomSplatter::spTextures[2],
                sBloodSplatterFlare01Filename.String()
            );
        }
    }
}

NiCamera* ScreenCustomSplatter::GetCamera() {
    return ScreenCustomSplatter::spCamera;
}

bool ScreenCustomSplatter::IsActiveSplatter() {
    if (!ScreenCustomSplatter::spRoot) {
        return false;
    }
    NiNode* pRoot = ScreenCustomSplatter::spRoot;
    return pRoot->GetChildCount() != 0;
}

NiNode* ScreenCustomSplatter::GetCurrentRoot() {
    return ScreenCustomSplatter::spRoot;
}

NiNode* ScreenCustomSplatter::CreateGeometry(uint32_t auiCount, float fSplatScale, float fOpacityScale) {
    NiNode* pnode = new NiNode(0);
    NiPoint3* pverticesA = new NiPoint3[4 * auiCount];
    NiPoint2* ptexcoordsA = new NiPoint2[4 * auiCount];
    uint16_t* pindicesA = static_cast<uint16_t*>(
        ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
    );
    NiPoint3* pflareVerticesA = nullptr;
    NiPoint2* pflareTexcoordsA = nullptr;
    uint16_t* pflareIndicesA = nullptr;
    if (BSShaderManager::bHDR) {
        pflareVerticesA = new NiPoint3[4 * auiCount];
        pflareTexcoordsA = new NiPoint2[4 * auiCount];
        pflareIndicesA = static_cast<uint16_t*>(
            ourMalloc(6 * auiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (6 * auiCount) : SIZE_MAX)
        );
    }
    for (uint32_t i = 0; i < auiCount; ++i) {
        NiMatrix3 rotationMatrix;
        float fx = RandomFloatNeg1To1() * ScreenCustomSplatter::fWRadius;
        float fy = RandomFloatNeg1To1() * ScreenCustomSplatter::fHRadius;
        NiPoint3 translate(fx, fy, 0.0f);
        float fMaxSize = fBloodSplatterMaxSize.Float();
        float fscale = RandomFloat(fBloodSplatterMinSize.Float(), fMaxSize);
        fscale = fscale * fSplatScale;
        float fMaxOpacity = fBloodSplatterMaxOpacity.Float();
        float falpha1 = RandomFloat(fBloodSplatterMinOpacity.Float(), fMaxOpacity);
        float fMaxOpacity2 = fBloodSplatterMaxOpacity2.Float();
        float falpha2 = RandomFloat(fBloodSplatterMinOpacity2.Float(), fMaxOpacity2);
        falpha1 = falpha1 * fOpacityScale;
        falpha2 = falpha2 * fOpacityScale;
        rotationMatrix.MakeIdentity();
        if (RandomBool(fBloodSplatterOpacityChance.Float())) {
            falpha1 = falpha2;
        }
        float fuoffset = RandomFloatNeg1To1() > 0.0 ? 0.0 : 0.5;
        float fvoffset = RandomFloatNeg1To1() > 0.0 ? 0.0 : 0.5;
        NiPoint2 texTranslate(fuoffset, fvoffset);
        float fflareSize = fBloodSplatterFlareSize.Float();
        uint32_t uibase = 4 * i;
        pverticesA[uibase] = rotationMatrix * NiPoint3(-fscale, fscale, falpha1) + translate;
        pverticesA[uibase + 1] = rotationMatrix * NiPoint3(fscale, fscale, falpha1) + translate;
        float fNegScale = -fscale;
        pverticesA[uibase + 2] = rotationMatrix * NiPoint3(fNegScale, fNegScale, falpha1) + translate;
        pverticesA[uibase + 3] = rotationMatrix * NiPoint3(fscale, -fscale, falpha1) + translate;
        if (BSShaderManager::bHDR) {
            pflareVerticesA[uibase] = rotationMatrix * NiPoint3(-fflareSize, fflareSize, falpha1) + translate;
            pflareVerticesA[uibase + 1] = rotationMatrix * NiPoint3(fflareSize, fflareSize, falpha1) + translate;
            float fNegFlareSize = -fflareSize;
            pflareVerticesA[uibase + 2] = rotationMatrix * NiPoint3(fNegFlareSize, fNegFlareSize, falpha1) + translate;
            pflareVerticesA[uibase + 3] = rotationMatrix * NiPoint3(fflareSize, -fflareSize, falpha1) + translate;
        }
        NiPoint2 texCoord = NiPoint2(0.5f, 0.5f) + texTranslate;
        ptexcoordsA[uibase].x = texCoord.x;
        ptexcoordsA[uibase].y = texCoord.y;
        texCoord = NiPoint2(0.0f, 0.5f) + texTranslate;
        ptexcoordsA[uibase + 1].x = texCoord.x;
        ptexcoordsA[uibase + 1].y = texCoord.y;
        texCoord = NiPoint2(0.5f, 0.0f) + texTranslate;
        ptexcoordsA[uibase + 2].x = texCoord.x;
        ptexcoordsA[uibase + 2].y = texCoord.y;
        texCoord = NiPoint2(0.0f, 0.0f) + texTranslate;
        ptexcoordsA[uibase + 3].x = texCoord.x;
        ptexcoordsA[uibase + 3].y = texCoord.y;
        if (BSShaderManager::bHDR) {
            texCoord = NiPoint2(1.0f, 1.0f);
            pflareTexcoordsA[uibase].x = texCoord.x;
            pflareTexcoordsA[uibase].y = texCoord.y;
            texCoord = NiPoint2(0.0f, 1.0f);
            pflareTexcoordsA[uibase + 1].x = texCoord.x;
            pflareTexcoordsA[uibase + 1].y = texCoord.y;
            texCoord = NiPoint2(1.0f, 0.0f);
            pflareTexcoordsA[uibase + 2].x = texCoord.x;
            pflareTexcoordsA[uibase + 2].y = texCoord.y;
            texCoord = NiPoint2(0.0f, 0.0f);
            pflareTexcoordsA[uibase + 3].x = texCoord.x;
            pflareTexcoordsA[uibase + 3].y = texCoord.y;
        }
        uint32_t uiindexBase = 6 * i;
        pindicesA[6 * i] = uibase;
        pindicesA[uiindexBase + 1] = uibase + 2;
        pindicesA[uiindexBase + 2] = uibase + 1;
        pindicesA[uiindexBase + 3] = uibase + 1;
        pindicesA[uiindexBase + 4] = uibase + 2;
        pindicesA[uiindexBase + 5] = uibase + 3;
        if (BSShaderManager::bHDR) {
            pflareIndicesA[uiindexBase] = uibase;
            pflareIndicesA[uiindexBase + 1] = uibase + 2;
            pflareIndicesA[uiindexBase + 2] = uibase + 1;
            pflareIndicesA[uiindexBase + 3] = uibase + 1;
            pflareIndicesA[uiindexBase + 4] = uibase + 2;
            pflareIndicesA[uiindexBase + 5] = uibase + 3;
        }
    }
    NiTriShape* pgeom = new NiTriShape(
        4 * auiCount,
        pverticesA,
        nullptr,
        nullptr,
        ptexcoordsA,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2 * auiCount,
        pindicesA
    );
    BSShaderBloodSplatterProperty* pshaderprop = new BSShaderBloodSplatterProperty;
    pshaderprop->SetTexture(0, ScreenCustomSplatter::spTextures[1].m_pObject);
    pshaderprop->SetTexture(1u, ScreenCustomSplatter::spTextures->m_pObject);
    pgeom->AttachProperty(pshaderprop);
    BSShaderManager::PrepareObject(pgeom, 0, 0);
    pnode->AttachChild(pgeom, 1);
    if (BSShaderManager::bHDR) {
        pgeom = new NiTriShape(
            4 * auiCount,
            pflareVerticesA,
            nullptr,
            nullptr,
            pflareTexcoordsA,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            2 * auiCount,
            pflareIndicesA
        );
        pshaderprop = new BSShaderBloodSplatterProperty;
        pshaderprop->SetTexture(0, ScreenCustomSplatter::spTextures[2].m_pObject);
        pgeom->AttachProperty(pshaderprop);
        BSShaderManager::PrepareObject(pgeom, 0, 0);
        pnode->AttachChild(pgeom, 1);
    }
    return pnode;
}

char ScreenCustomSplatter::SetCustomSplatterTexture(char* apFilename, uint8_t aucIndex) {
    if (aucIndex > 2u) {
        return 0;
    }
    SkyTextureManager::LoadTextureInBackground(&ScreenCustomSplatter::spTextures[aucIndex], apFilename);
    return 1;
}

void ScreenCustomSplatter::Shutdown() {
    if (ScreenCustomSplatter::bEnabled) {
        ScreenCustomSplatter::spRoot->RemoveAllChildren();
        ScreenCustomSplatter::spRoot = static_cast<NiNode*>(nullptr);
        ScreenCustomSplatter::spCamera = static_cast<NiCamera*>(nullptr);
    }
    ScreenCustomSplatter::bEnabled = false;
    for (NiPointer<NiTexture>& spTexture : ScreenCustomSplatter::spTextures) {
        spTexture = static_cast<NiTexture*>(nullptr);
    }
}

// The tag string goes to the base constructor by address, and the splatter data is copied whole.
BSCustomSplatterExtraData::BSCustomSplatterExtraData(BSSplatterData* aSplatterData)
  : NiExtraData(&Tag()), SplatterData(*aSplatterData) {}

BSCustomSplatterExtraData::~BSCustomSplatterExtraData() {}

void BSCustomSplatterExtraData::InitTag() {
    BSCustomSplatterExtraData::pTag = new NiFixedString("CSD");
}

// The "CSD" tag string.
const NiFixedString& BSCustomSplatterExtraData::Tag() {
    return *BSCustomSplatterExtraData::pTag;
}
