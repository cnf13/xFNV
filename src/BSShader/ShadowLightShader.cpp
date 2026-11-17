#include "BSShader/ShadowLightShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/HairShader.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/ParallaxShader.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkinShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cstdint>

ShadowLightShaderPassArray ShadowLightShader::spPass;
NiRTTI ShadowLightShader::ms_RTTI("ShadowLightShader", &NiD3DDefaultShader::ms_RTTI);

double fMin_(double a1, double a2) {
    if (a1 < a2) {
        return a1;
    }
    return a2;
}

ShadowLightShader::ShadowLightShader(
    NiDX9ShaderDeclaration* apShaderDecl,
    NiDX9ShaderDeclaration* apSkinnedShaderDecl,
    NiDX9ShaderDeclaration* apModelSpaceNormShaderDecl,
    NiDX9ShaderDeclaration* apSplatTexturingShaderDecl
)
  : spShaderDecl(apShaderDecl), spSkinnedShaderDecl(apSkinnedShaderDecl),
    spModelSpaceNormShaderDecl(apModelSpaceNormShaderDecl), spSplatTexturingShaderDecl(apSplatTexturingShaderDecl),
    spTexEffectPConstantMap(nullptr), spTexEffectVConstantMap(nullptr), spDefaultPixelConstantMap(nullptr),
    spDefaultVertexConstantMap(nullptr) {
    iShaderType = 1;
    m_spShaderDecl = apShaderDecl;
    kFarNear.x = 1.0 / fRenderDepthMaxDepth.uValue.f;
    kFarNear.z = 0.0f;
    kFarNear.y = 0.0f;
    kFarNear.w = 0.0f;
    ePass = BSShaderManager::BSSM_EMPTY;
}

ShadowLightShader::~ShadowLightShader() {
    for (NiD3DVertexShader*& rpShader : spVertexShaders) {
        if (rpShader) {
            rpShader->DecRefCount();
            rpShader = nullptr;
        }
    }
    for (NiD3DPixelShader*& rpShader : spPixelShaders) {
        if (rpShader) {
            rpShader->DecRefCount();
            rpShader = nullptr;
        }
    }
    for (NiPointer<NiD3DPass>& rspPass : spPass) {
        rspPass = nullptr;
    }
    for (NiShaderConstantMapEntry*& rpEntry : spVertexConstantMapEntries) {
        if (rpEntry) {
            rpEntry->DecRefCount();
            rpEntry = nullptr;
        }
    }
    for (NiShaderConstantMapEntry*& rpEntry : spPixelConstantMapEntries) {
        if (rpEntry) {
            rpEntry->DecRefCount();
            rpEntry = nullptr;
        }
    }
    m_spShaderDecl = nullptr;
}

const NiRTTI* ShadowLightShader::GetRTTI() const {
    return &ShadowLightShader::ms_RTTI;
}

// Releases every shader, pass and constant map entry the class holds and clears the per-pass constant flags.
bool ShadowLightShader::Initialize() {
    BSShader::Initialize();
    for (uint32_t i = 0; i < 156; ++i) {
        NiD3DVertexShader* pShader = ShadowLightShader::spVertexShaders[i];
        if (pShader) {
            pShader->DecRefCount();
            ShadowLightShader::spVertexShaders[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < 208; ++i) {
        NiD3DPixelShader* pShader = ShadowLightShader::spPixelShaders[i];
        if (pShader) {
            pShader->DecRefCount();
            ShadowLightShader::spPixelShaders[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < 759; ++i) {
        ShadowLightShader::spPass[i] = nullptr;
    }
    for (uint32_t i = 0; i < 22; ++i) {
        NiShaderConstantMapEntry* pEntry = ShadowLightShader::spVertexConstantMapEntries[i];
        if (pEntry) {
            pEntry->DecRefCount();
            ShadowLightShader::spVertexConstantMapEntries[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < 22; ++i) {
        NiShaderConstantMapEntry* pEntry = ShadowLightShader::spPixelConstantMapEntries[i];
        if (pEntry) {
            pEntry->DecRefCount();
            ShadowLightShader::spPixelConstantMapEntries[i] = nullptr;
        }
    }
    std::memset(ShadowLightShader::ulVertexConstantFlags, 0, sizeof(ShadowLightShader::ulVertexConstantFlags));
    std::memset(ShadowLightShader::ulPixelConstantFlags, 0, sizeof(ShadowLightShader::ulPixelConstantFlags));
    std::memset(
        ShadowLightShader::uiVertexNonVolatileConstantFlags, 0,
        sizeof(ShadowLightShader::uiVertexNonVolatileConstantFlags)
    );
    std::memset(
        ShadowLightShader::uiPixelNonVolatileConstantFlags, 0,
        sizeof(ShadowLightShader::uiPixelNonVolatileConstantFlags)
    );
    return true;
}

uint32_t ShadowLightShader::SetupBoneMatrices(
    NiGeometry* a1,
    const NiSkinInstance* a2,
    const NiSkinPartition::Partition* a3,
    NiGeometryData::RendererData* a4,
    const NiPropertyState* a5,
    const NiDynamicEffectState* a6,
    const NiTransform& a7,
    const NiBound& a8
) {
    if ((a3 &&
         (!m_uiCurrentPass &&
          (ShadowLightShader::spVertexConstantMapEntries[10] &&
           ShadowLightShader::spVertexConstantMapEntries[10]->m_bEnabled))) ||
        m_spVertexConstantMap.m_pObject == spTexEffectVConstantMap.m_pObject) {
        BSShaderManager::pRenderer.m_pObject->CalculateBoneMatrices(a2, a7, false, 3u, true);
        NiD3DShaderConstantMap* pTexEffectMap = spTexEffectVConstantMap.m_pObject;
        if (m_spVertexConstantMap.m_pObject == pTexEffectMap) {
            NiD3DVertexShader* pVertexShader = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
            pVertexShader->SetShaderConstantArray(
                pTexEffectMap->GetEntry("BoneMatrix3"), a2->m_pvBoneMatrices, a3->m_usBones, 3u, a3->m_pusBones
            );
            return 0;
        }
        NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->SetShaderConstantArray(
            ShadowLightShader::spVertexConstantMapEntries[10], a2->m_pvBoneMatrices, a3->m_usBones, 3u, a3->m_pusBones
        );
    }
    return 0;
}

void ShadowLightShader::SetupGeometryTextures(const NiPropertyState* apState) {
    NiSourceTexture* v6;
    NiD3DTextureStage* v7;
    BSShaderPPLightingProperty* pprop = static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    bool v5 = (pprop->ulFlags[1] & BSS2_Skip_Normal_Maps) == 0;
    switch (BSShaderManager::pCurrentRenderPass->usPassEnum) {
        case 2:
        case 6:
        case 7:
        case 8:
        case 17:
        case 19:
        case 88:
        case 90:
        case 92:
        case 94:
        case 98:
        case 99:
        case 330:
        case 333:
        case 334:
        case 335:
        case 595:
        case 597:
        case 734:
        case 735:
        case 746:
        case 747: {
            goto LABEL_11;
        }
        case 4:
        case 5: {
            NiSourceTexture* v15;
            BSShaderPPLightingProperty::TextureEffectData* m_pObject = pprop->spTexEffectData.m_pObject;
            NiD3DTextureStage* v14 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
            if (!m_pObject ||
                (v15 = static_cast<NiSourceTexture*>(m_pObject->spBlockOutTexture.m_pObject)) == nullptr) {
                v15 = BSShaderManager::spDefaultTexEffectMap;
            }
            BSShaderPPLightingProperty* v16 =
                static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
            v14->m_spTexture = v15;
            SetClampMode(v14, v16);
            goto LABEL_49;
        }
        case 89:
        case 91:
        case 93:
        case 95:
        case 497:
        case 500: {
            SetDiffuseMap(pprop, 0, 0);
            v6 = static_cast<NiSourceTexture*>(pprop->spTextures[2]->m_pObject);
            v7 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
            if (v6) {
                goto LABEL_20;
            }
            goto LABEL_3;
        }
        case 100:
        case 101:
        case 105:
        case 106:
        case 111:
        case 112:
        case 116:
        case 117:
        case 127:
        case 128:
        case 131:
        case 132:
        case 137:
        case 138:
        case 141:
        case 142:
        case 195:
        case 198:
        case 201:
        case 205:
        case 215:
        case 220:
        case 224:
        case 229:
        case 233:
        case 238:
        case 242:
        case 247:
        case 251:
        case 261:
        case 266:
        case 273:
        case 278:
        case 284:
        case 289:
        case 295:
        case 565: {
            goto LABEL_10;
        }
        case 102:
        case 103:
        case 107:
        case 108:
        case 113:
        case 114:
        case 118:
        case 119:
        case 129:
        case 130:
        case 134:
        case 135:
        case 139:
        case 140:
        case 144:
        case 145: {
            NiSourceTexture* v8;
            v8 = static_cast<NiSourceTexture*>(pprop->spTextures[2]->m_pObject);
            if (!v8) {
                v8 = BSShaderManager::spDefaultHeightMap;
            }
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[2].m_pObject->m_spTexture = v8;
        LABEL_10:;
            SetNormalMap(pprop, 1u, 0);
        LABEL_11:;
            SetDiffuseMap(pprop, 0, 0);
            ApplyTextureStages();
            return;
        }
        case 104:
        case 109:
        case 115:
        case 120:
        case 133:
        case 143: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            SetDiffuseMap(pprop, 2u, 1u);
            SetNormalMap(pprop, 3u, 1u);
            ApplyTextureStages();
            return;
        }
        case 124:
        case 125:
        case 300:
        case 301:
        case 302:
        case 303:
        case 306:
        case 309:
        case 318:
        case 321:
        case 340:
        case 341:
        case 342:
        case 343:
        case 346:
        case 347:
        case 349:
        case 350:
        case 352:
        case 353:
        case 355:
        case 356:
        case 358:
        case 359:
        case 361:
        case 362:
        case 364:
        case 365:
        case 367:
        case 368:
        case 370:
        case 371:
        case 373:
        case 374:
        case 482:
        case 483:
        case 484:
        case 587: {
            goto LABEL_17;
        }
        case 147:
        case 148:
        case 495:
        case 496:
        case 498:
        case 499:
        case 501:
        case 502: {
            int32_t itexture = BSShaderManager::pCurrentRenderPass->ucExtraParam;
            SetDiffuseMap(pprop, 0, itexture);
            SetNormalMap(pprop, 1u, itexture);
            ApplyTextureStages();
            return;
        }
        case 149:
        case 155:
        case 160:
        case 166:
        case 171:
        case 177:
        case 182:
        case 188:
        case 208:
        case 212:
        case 267: {
            goto LABEL_15;
        }
        case 150:
        case 156:
        case 161:
        case 167:
        case 172:
        case 178:
        case 183:
        case 189:
        case 210:
        case 213: {
            NiSourceTexture* v9;
            v9 = static_cast<NiSourceTexture*>(pprop->spTextures[2]->m_pObject);
            if (!v9) {
                v9 = BSShaderManager::spDefaultHeightMap;
            }
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[4].m_pObject->m_spTexture = v9;
        LABEL_15:;
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            ApplyTextureStages();
            return;
        }
        case 151:
        case 157:
        case 162:
        case 168:
        case 173:
        case 179:
        case 184:
        case 190:
        case 260:
        case 265:
        case 272:
        case 277:
        case 283:
        case 288:
        case 294:
        case 299: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            SetDiffuseMap(pprop, 4u, 1u);
            ApplyTextureStages();
            return;
        }
        case 196:
        case 199:
        case 203:
        case 206:
        case 255:
        case 262:
        case 268:
        case 274:
        case 279:
        case 285:
        case 290:
        case 296: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            v6 = static_cast<NiSourceTexture*>(pprop->spTextures[2]->m_pObject);
            v7 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[4].m_pObject;
            if (v6) {
                goto LABEL_20;
            }
        LABEL_3:;
            v7->m_spTexture = BSShaderManager::spDefaultHeightMap;
            ApplyTextureStages();
            return;
        }
        case 216:
        case 221:
        case 225:
        case 230:
        case 234:
        case 239:
        case 243:
        case 248: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            v6 = static_cast<NiSourceTexture*>(pprop->spTextures[2]->m_pObject);
            v7 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[3].m_pObject;
            if (!v6) {
                goto LABEL_5;
            }
            goto LABEL_20;
        }
        case 254: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            SetDiffuseMap(pprop, 4u, 1u);
            SetNormalMap(pprop, 6u, 1u);
            SetLODNoiseMap(7);
            ApplyTextureStages();
            return;
        }
        case 331:
        case 336: {
            SetDiffuseMap(pprop, 0, 0);
            SetDiffuseMap(pprop, 1u, 1u);
            SetNormalMap(pprop, 2u, 1u);
            ApplyTextureStages();
            return;
        }
        case 332:
        case 338: {
            SetDiffuseMap(pprop, 0, 0);
            SetDiffuseMap(pprop, 1u, 1u);
            ApplyTextureStages();
            return;
        }
        case 503:
        case 504:
        case 505:
        case 506:
        case 507:
        case 508:
        case 509:
        case 510: {
            goto LABEL_43;
        }
        case 511:
        case 512:
        case 513:
        case 514:
        case 515:
        case 516:
        case 517:
        case 518: {
            goto LABEL_41;
        }
        case 519:
        case 520:
        case 521:
        case 522:
        case 523:
        case 524:
        case 525:
        case 526: {
            goto LABEL_39;
        }
        case 527:
        case 528:
        case 529:
        case 530:
        case 531:
        case 532:
        case 533:
        case 534: {
            goto LABEL_37;
        }
        case 535:
        case 536:
        case 537:
        case 538:
        case 539:
        case 540:
        case 541:
        case 542: {
            goto LABEL_35;
        }
        case 543:
        case 544:
        case 545:
        case 546:
        case 547:
        case 548:
        case 549:
        case 550: {
            goto LABEL_33;
        }
        case 551:
        case 552:
        case 553:
        case 554:
        case 555:
        case 556:
        case 557:
        case 558: {
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                pprop->spTextures[0][6].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[13].m_pObject->m_spTexture = pprop->spTextures[1][6].m_pObject;
            }
        LABEL_33:;
            v3->m_kStages.m_pBase[5].m_pObject->m_spTexture = pprop->spTextures[0][5].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[12].m_pObject->m_spTexture = pprop->spTextures[1][5].m_pObject;
            }
        LABEL_35:;
            v3->m_kStages.m_pBase[4].m_pObject->m_spTexture = pprop->spTextures[0][4].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[11].m_pObject->m_spTexture = pprop->spTextures[1][4].m_pObject;
            }
        LABEL_37:;
            v3->m_kStages.m_pBase[3].m_pObject->m_spTexture = pprop->spTextures[0][3].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[10].m_pObject->m_spTexture = pprop->spTextures[1][3].m_pObject;
            }
        LABEL_39:;
            v3->m_kStages.m_pBase[2].m_pObject->m_spTexture = pprop->spTextures[0][2].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[9].m_pObject->m_spTexture = pprop->spTextures[1][2].m_pObject;
            }
        LABEL_41:;
            v3->m_kStages.m_pBase[1].m_pObject->m_spTexture = pprop->spTextures[0][1].m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[8].m_pObject->m_spTexture = pprop->spTextures[1][1].m_pObject;
            }
        LABEL_43:;
            v3->m_kStages.m_pBase->m_pObject->m_spTexture = pprop->spTextures[0]->m_pObject;
            if (v5) {
                v3->m_kStages.m_pBase[7].m_pObject->m_spTexture = pprop->spTextures[1]->m_pObject;
                ApplyTextureStages();
            }
            else {
            LABEL_49:;
                ApplyTextureStages();
            }
            return;
        }
        case 560: {
            int32_t itexture_2 = BSShaderManager::pCurrentRenderPass->ucExtraParam;
            SetDiffuseMap(pprop, 0, itexture_2);
            SetNormalMap(pprop, 1u, itexture_2);
            SetLODNoiseMap(2);
            ApplyTextureStages();
            return;
        }
        case 561:
        case 562:
        case 563: {
            SetDiffuseMap(pprop, 0, 0);
            SetShadowMap(6u);
            ApplyTextureStages();
            return;
        }
        case 580:
        case 581:
        case 582:
        case 583: {
            SetEnvMask(pprop, 2, 0);
            SetCubeMap(pprop, 3, 0);
            goto LABEL_17;
        }
        case 584:
        case 585:
        case 586: {
            SetEnvMask(pprop, 3, 0);
            SetCubeMap(pprop, 1, 0);
        LABEL_17:;
            SetNormalMap(pprop, 0, 0);
            ApplyTextureStages();
            break;
        }
        case 726:
        case 727: {
            NiPointer<NiTexture>* v10;
            v10 = pprop->spTextures[2];
            v7 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
            if (v10->m_pObject) {
                v7->m_spTexture = v10->m_pObject;
                ApplyTextureStages();
            }
            else {
            LABEL_5:;
                v6 = BSShaderManager::spDefaultHeightMap;
            LABEL_20:;
                v7->m_spTexture = v6;
                ApplyTextureStages();
            }
            break;
        }
        default: {
            return;
        }
    }
}

void ShadowLightShader::SetupGeometryConstants(const NiPropertyState* apState) {
    _D3DMATRIX d3dInvWorld;
    NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSShaderProperty::RenderPass* apRenderPass = BSShaderManager::pCurrentRenderPass;
    BSNiBound* m_kWorldBound = pgeometry->m_kWorldBound;
    BSShaderManager::eBSSMPassEnum v4 = BSShaderManager::eCurrentPass;
    uint32_t epass = BSShaderManager::eCurrentPass;
    BSNiBound* v24 = m_kWorldBound;
    if (!m_kWorldBound) {
        v24 = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    NiMaterialProperty* pmaterialProperty = static_cast<NiMaterialProperty*>(apState->m_aspProps[2].m_pObject);
    const NiSkinInstance* apSkin = pgeometry->m_spSkinInstance.m_pObject;
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
    BSShaderPPLightingProperty* pprop = static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
    NiAlphaProperty* palphaProp = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
    const NiSkinInstance* apSkin_2 = apSkin;
    if ((BSShaderManager::eCurrentPass == BSShaderManager::BSSM_CONSTALPHA ||
         BSShaderManager::eCurrentPass == BSShaderManager::BSSM_CONSTALPHA_S) &&
        (pprop->ulFlags[0] & BSS1_Hair) != 0 && (palphaProp->m_uFlags & 1) != 0) {
        BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
        BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
    }
    BSUtilities::GetInverseWorldMatrix(pgeometry->m_kWorld, apSkin, d3dInvWorld);
    if (!apSkin) {
        SetupGeometryOpt_WorldViewProj(pgeometry, pgeometry->m_kWorld);
    }
    if (v4 == BSShaderManager::BSSM_2x_TEXEFFECT || v4 == BSShaderManager::BSSM_2x_TEXEFFECT_S) {
        UpdatePipelineTexEffect(pgeometry, pprop, apRenderPass);
    }
    else {
        if (v4 == BSShaderManager::BSSM_ZONLY_TEXEFFECT || v4 == BSShaderManager::BSSM_ZONLY_TEXEFFECT_S) {
            UpdatePipelineZOnlyTexEffect(pgeometry, pprop, apRenderPass);
        }
        else {
            SetupGeometryConstants_Lights(pprop, apRenderPass, d3dInvWorld, pgeometry->m_kWorld, v4, apSkin);
            SetupGeometryConstants_AmbientEmittance(pprop, apRenderPass, pgeometry, pmaterialProperty);
            SetupGeometryConstants_ObjToCube(apRenderPass, epass, apSkin_2, pgeometry->m_kWorld);
            SetupGeometryConstants_ShadowProj(apState, apSkin_2, pgeometry->m_kWorld);
            SetupGeometryConstants_EyePos(epass, pgeometry->m_kWorld, d3dInvWorld);
            SetupGeometryConstants_SkinToCube(apSkin_2);
            SetupGeometryConstants_RefractionTime(pprop);
            SetupGeometryConstants_WorldBoundCenter(pgeometry);
            SetupGeometryConstants_HighDetailRange(pgeometry);
            SetupGeometryConstants_LODLandParams(epass, pprop, pgeometry);
            SetupGeometryConstants_LandTextureMask(epass, apRenderPass);
            SetupGeometryConstants_Toggles(epass, pgeometry, pprop, pmaterialProperty, apRenderPass, palphaProp);
            SetupGeometryConstants_RefractionPower(pprop);
            SetupGeometryConstants_WaterFog();
            SetupGeometryConstants_SimpleShadowClipPlanes(epass);
            SetupGeometryConstants_LODTexParams(pprop);
            apSkin = apSkin_2;
        }
    }
    NiD3DShaderConstantMap* m_pObject = m_spPixelConstantMap.m_pObject;
    if (m_pObject) {
        NiD3DPixelShader* v16 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v16) {
            m_pObject->SetShaderConstants(
                v16, pgeometry, apSkin, nullptr, pbufferData, apState, nullptr, pgeometry->m_kWorld, *v24, 0, true
            );
        }
    }
    NiD3DShaderConstantMap* v17 = m_spVertexConstantMap.m_pObject;
    if (v17) {
        NiD3DVertexShader* v18 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v18) {
            v17->SetShaderConstants(
                v18, pgeometry, apSkin, nullptr, pbufferData, apState, nullptr, pgeometry->m_kWorld, *v24, 0, true
            );
        }
    }
    NiStencilProperty* v19 = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((v19) && ((v19->m_uFlags & 1) != 0)) {
        m_pkD3DRenderState->ApplyStencil(static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject));
    }
}

void ShadowLightShader::PostGeometry(const NiPropertyState* apState) {
    BSShaderManager::eBSSMPassEnum v2 = BSShaderManager::eCurrentPass;
    if (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_2x_TEXEFFECT ||
        BSShaderManager::eCurrentPass == BSShaderManager::BSSM_2x_TEXEFFECT_S) {
        BSRenderState::SetZFunc(NiZBufferProperty::TEST_LESSEQUAL, BSRS_NOLOCK);
    }
    NiStencilProperty* m_pObject = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((m_pObject) && ((m_pObject->m_uFlags & 1) != 0)) {
        BSRenderState::SetStencilEnable(false, BSRS_NOLOCK);
    }

    // The shade property slot holds the BSShaderProperty; its first flag word is tested.
    if ((v2 == BSShaderManager::BSSM_CONSTALPHA || v2 == BSShaderManager::BSSM_CONSTALPHA_S) &&
        (static_cast<BSShaderProperty*>(apState->m_aspProps[3].m_pObject)->ulFlags[0] & BSS1_Hair) != 0) {
        --BSRenderState::iLock[1];
        BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
        --BSRenderState::iLock[16];
        BSRenderState::SetColorWriteEnable(7u, BSRS_NOLOCK);
    }
}

// Walks 156 vertex shaders (only with a shader declaration) and 208 pixel shaders, then the base.
void ShadowLightShader::RecreateRendererData() {
    if (m_spShaderDecl.m_pObject) {
        for (uint32_t i = 0; i < 156; ++i) {
            NiD3DVertexShader* pkVertexShader = ShadowLightShader::spVertexShaders[i];
            if (pkVertexShader) {
                pkVertexShader->RecreateRendererData();
            }
        }
    }
    for (uint32_t j = 0; j < 208; ++j) {
        NiD3DPixelShader* pkPixelShader = ShadowLightShader::spPixelShaders[j];
        if (pkPixelShader) {
            pkPixelShader->RecreateRendererData();
        }
    }
    NiD3DShader::RecreateRendererData();
}

void ShadowLightShader::InitPass(uint32_t auiCurrentPass) {
    NiD3DShader::m_spCurrentPass = ShadowLightShader::spPass[auiCurrentPass].m_pObject;
    if (auiCurrentPass <= 262) {
        if (auiCurrentPass < 261) {
            switch (auiCurrentPass) {
                case 3u:
                case 5u:
                case 8u:
                case 18u:
                case 19u:
                case 21u:
                case 92u:
                case 93u:
                case 94u:
                case 95u:
                case 111u:
                case 112u:
                case 113u:
                case 114u:
                case 115u:
                case 116u:
                case 117u:
                case 118u:
                case 119u:
                case 120u:
                case 121u:
                case 125u:
                case 137u:
                case 138u:
                case 139u:
                case 140u:
                case 141u:
                case 142u:
                case 143u:
                case 144u:
                case 145u:
                case 146u:
                case 155u:
                case 156u:
                case 157u:
                case 166u:
                case 167u:
                case 168u:
                case 177u:
                case 178u:
                case 179u:
                case 188u:
                case 189u:
                case 190u:
                case 198u:
                case 199u:
                case 205u:
                case 206u:
                case 212u:
                case 213u:
                case 220u:
                case 221u:
                case 229u:
                case 230u:
                case 238u:
                case 239u:
                case 247u:
                case 248u: {
                    break;
                }
                case 97u:
                case 254u: {
                    goto LABEL_10;
                }
                case 147u:
                case 148u: {
                    goto LABEL_18;
                }
                default: {
                    goto LABEL_19;
                }
            }
        }
    LABEL_12:;
        NiDX9ShaderDeclaration* m_pObject = spSkinnedShaderDecl.m_pObject;
        m_spShaderDecl = m_pObject;
        goto LABEL_20;
    }
    if (auiCurrentPass > 562) {
        switch (auiCurrentPass) {
            case 563u: {
                goto LABEL_18;
            }
            case 582u:
            case 583u:
            case 585u:
            case 591u:
            case 593u:
            case 596u:
            case 597u:
            case 727u:
            case 729u:
            case 735u:
            case 741u:
            case 747u: {
                goto LABEL_12;
            }
            default: {
                goto LABEL_19;
            }
        }
    }
    if (auiCurrentPass == 562) {
        goto LABEL_12;
    }
    if (auiCurrentPass <= 336) {
        if (auiCurrentPass < 335) {
            switch (auiCurrentPass) {
                case 265u:
                case 273u:
                case 274u:
                case 277u:
                case 284u:
                case 285u:
                case 288u:
                case 295u:
                case 296u:
                case 299u:
                case 302u:
                case 303u:
                case 309u:
                case 310u:
                case 315u:
                case 316u:
                case 321u:
                case 322u:
                case 327u:
                case 328u:
                case 333u: {
                    goto LABEL_12;
                }
                case 267u: {
                    goto LABEL_10;
                }
                default: {
                    goto LABEL_19;
                }
            }
        }
        goto LABEL_12;
    }
    switch (auiCurrentPass) {
        case 338u:
        case 342u:
        case 343u:
        case 349u:
        case 350u:
        case 355u:
        case 356u:
        case 361u:
        case 362u:
        case 367u:
        case 368u:
        case 373u:
        case 374u:
        case 483u:
        case 486u:
        case 488u:
        case 491u:
        case 493u: {
            goto LABEL_12;
        }
        case 489u: {
        LABEL_10:;
            m_spShaderDecl.SetNew(spModelSpaceNormShaderDecl.m_pObject);
            goto LABEL_20;
        }
        case 495u:
        case 496u:
        case 497u:
        case 498u:
        case 499u:
        case 500u:
        case 501u:
        case 502u:
        case 503u:
        case 504u:
        case 505u:
        case 506u:
        case 507u:
        case 508u:
        case 509u:
        case 510u:
        case 511u:
        case 512u:
        case 513u:
        case 514u:
        case 515u:
        case 516u:
        case 517u:
        case 518u:
        case 519u:
        case 520u:
        case 521u:
        case 522u:
        case 523u:
        case 524u:
        case 525u:
        case 526u:
        case 527u:
        case 528u:
        case 529u:
        case 530u:
        case 531u:
        case 532u:
        case 533u:
        case 534u:
        case 535u:
        case 536u:
        case 537u:
        case 538u:
        case 539u:
        case 540u:
        case 541u:
        case 542u:
        case 543u:
        case 544u:
        case 545u:
        case 546u:
        case 547u:
        case 548u:
        case 549u:
        case 550u:
        case 551u:
        case 552u:
        case 553u:
        case 554u:
        case 555u:
        case 556u:
        case 557u:
        case 558u:
        case 559u:
        case 560u: {
        LABEL_18:;
            m_spShaderDecl.SetNew(spSplatTexturingShaderDecl.m_pObject);
            goto LABEL_20;
        }
        default: {
        LABEL_19:;
            m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
            break;
        }
    }
LABEL_20:;
    if (auiCurrentPass == 592 || auiCurrentPass == 593) {
        m_spPixelConstantMap = spTexEffectPConstantMap;
        m_spVertexConstantMap = spTexEffectVConstantMap;
    }
    else {
        m_spPixelConstantMap.operator=(spDefaultPixelConstantMap);
        m_spVertexConstantMap.operator=(spDefaultVertexConstantMap);
    }
}

// The projected shadow map and its mask go to the two stages after the pass's other textures. The pass numbers are
// BSShaderManager::RenderPassType values: 148 BSSM_LANDAD_Shp, 499 BSSM_LANDAD_AShp, 160-299 the
// BSSM_ADTS*/BSSM_ADTS2*/BSSM_ADT*/BSSM_ADT2* shadow passes, 224-248 the BSSM_AD2*/BSSM_AD3* shadow passes, 352-356
// BSSM_2x_SPECULARDIR*_Shp, 504-556 BSSM_LAND1O_Shp to BSSM_LAND7O4_Shp.
void ShadowLightShader::SetupPassTextures(uint32_t auiCurrentPass) {
    switch (auiCurrentPass) {
        case 148:
        case 499:
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[2].m_pObject->m_spTexture =
                BSShaderManager::pProjectedShadowTexture.m_pObject;
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[3].m_pObject->m_spTexture =
                static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
            break;
        case 160:
        case 161:
        case 162:
        case 166:
        case 167:
        case 168:
        case 182:
        case 183:
        case 184:
        case 188:
        case 189:
        case 190:
        case 266:
        case 267:
        case 268:
        case 272:
        case 273:
        case 274:
        case 277:
        case 289:
        case 290:
        case 294:
        case 295:
        case 296:
        case 299:
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                BSShaderManager::pProjectedShadowTexture.m_pObject;
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[7].m_pObject->m_spTexture =
                static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
            break;
        case 224:
        case 225:
        case 229:
        case 230:
        case 242:
        case 243:
        case 247:
        case 248:
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[5].m_pObject->m_spTexture =
                BSShaderManager::pProjectedShadowTexture.m_pObject;
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
            break;
        case 352:
        case 353:
        case 355:
        case 356:
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[4].m_pObject->m_spTexture =
                BSShaderManager::pProjectedShadowTexture.m_pObject;
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[5].m_pObject->m_spTexture =
                static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
            break;
        case 504:
        case 508:
        case 512:
        case 516:
        case 520:
        case 524:
        case 528:
        case 532:
        case 536:
        case 540:
        case 544:
        case 548:
        case 552:
        case 556:
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[14].m_pObject->m_spTexture =
                BSShaderManager::pProjectedShadowTexture.m_pObject;
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[15].m_pObject->m_spTexture =
                static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
            break;
        default: break;
    }
}

void ShadowLightShader::SetupPassConstants(uint32_t auiCurrentPass) {
    if (auiCurrentPass != 592 && auiCurrentPass != 593) {
        // Entry i's flag is bit i + 1. The 22 entries never wrap the bit, so a shift is enough.
        uint32_t uiBit = 1;
        for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spVertexConstantMapEntries) {
            uiBit <<= 1;
            if (pEntry) {
                pEntry->m_bEnabled = (uiBit & ShadowLightShader::ulVertexConstantFlags[auiCurrentPass]) != 0;
            }
        }
        uiBit = 1;
        for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spPixelConstantMapEntries) {
            uiBit <<= 1;
            if (pEntry) {
                pEntry->m_bEnabled = (uiBit & ShadowLightShader::ulPixelConstantFlags[auiCurrentPass]) != 0;
            }
        }
        m_spVertexConstantMap.m_pObject->SortByEnabled();
        m_spPixelConstantMap.m_pObject->SortByEnabled();
        if (ShadowLightShader::spVertexConstantMapEntries[8u]->m_bEnabled) {
            BSFogProperty* pbsfog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
            if (pbsfog) {
                float g;
                float b;
                float a;
                float fStartDistance = pbsfog->fStartDistance;
                float fEndDistance = pbsfog->fEndDistance;
                float fPower = pbsfog->fPower;
                if (fEndDistance == 0.0f && 0.0f == fStartDistance) {
                    BSShaderLightingProperty::pShaderLightConstants[28].x = 500000.0f;
                    BSShaderLightingProperty::pShaderLightConstants[28].y = 0.0f;
                    BSShaderLightingProperty::pShaderLightConstants[28].z = 0.0f;
                    g = NiColorA_BLACK.g;
                    BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
                    b = NiColorA_BLACK.b;
                    BSShaderLightingProperty::pShaderLightConstants[29].x = NiColorA_BLACK.r;
                    a = NiColorA_BLACK.a;
                }
                else {
                    float r = pbsfog->m_kColor.r;
                    float v21 = fEndDistance - fStartDistance;
                    float v19 = pbsfog->m_kColor.g;
                    float v20 = pbsfog->m_kColor.b;
                    BSShaderLightingProperty::pShaderLightConstants[28].x = pbsfog->fEndDistance;
                    BSShaderLightingProperty::pShaderLightConstants[28].y = v21;
                    BSShaderLightingProperty::pShaderLightConstants[28].z = fPower;
                    g = v19;
                    BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
                    b = v20;
                    BSShaderLightingProperty::pShaderLightConstants[29].x = r;
                    a = 0.0f;
                }
                BSShaderLightingProperty::pShaderLightConstants[29].w = a;
                BSShaderLightingProperty::pShaderLightConstants[29].z = b;
                BSShaderLightingProperty::pShaderLightConstants[29].y = g;
            }
        }
        if (BSShaderManager::pCurrentRenderPass->pGeometry->m_spSkinInstance.m_pObject) {
            alignas(16) D3DXMATRIX kViewProjTranspose;
            D3DXMatrixTranspose_Thunk(&kViewProjTranspose, &m_pkD3DRenderer->m_kViewProj);
            NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
                1, &kViewProjTranspose._11, 4
            );
        }
        if (ShadowLightShader::spVertexConstantMapEntries[8]->m_bEnabled) {
            ShadowLightShader::pLightData[1] = NiColorA_BLACK;
            ShadowLightShader::pLightData[2] = NiColorA_BLACK;
            ShadowLightShader::pLightData[3] = NiColorA_BLACK;
        }
    }
}

NiD3DShaderDeclaration* ShadowLightShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    if (apGeometry->m_spSkinInstance.m_pObject) {
        return spSkinnedShaderDecl.m_pObject;
    }
    if (apProperty && ((apProperty->ulFlags[0] & 4096) != 0)) {
        return spModelSpaceNormShaderDecl.m_pObject;
    }
    if ((apProperty->ulFlags[0] & 16384) != 0) {
        return spSplatTexturingShaderDecl.m_pObject;
    }
    return spShaderDecl.m_pObject;
}

// Builds the vertex and pixel constant maps when missing, keeping a reference on each entry the passes set in
// spVertexConstantMapEntries / spPixelConstantMapEntries, and makes them the default maps; then builds the texture
// effect maps when missing. An empty string is passed as the variable names.
void ShadowLightShader::CreateConstantMaps() {
    if (!m_spVertexConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewProjTranspose", 0x20000009, 1, "", 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ShadowProj", 0x10000009, 0, 0x12, 4, "", 0x40, 4, &ShadowProjMatrix, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "SkinWorldTranspose", 0x10000009, 0, 9, 4, "", 0x40, 4, &kSkinWorldTranspose, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldTranspose", 0x10000009, 0, 8, 4, "", 0x40, 4, &kObjectToWorldTranspose, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ShadowProjData", 0x10000007, 0, 0x16, 1, "", 0x10, 4, &kShadowProjData, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ShadowProjTransform", 0x10000007, 0, 0x17, 1, "", 0x10, 4, &kShadowProjTransform, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "HighDetailRange", 0x10000007, 0, 0xC, 1, "", 0x10, 4, &BSShaderLightingProperty::pShaderLightConstants[34],
            false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "LightData", 0x10000009, 0, 0x19, 0xA, "", 0xA0, 4, pLightData, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "FogParam | ShadowVolumeFatness", 0x10000007, 0, 0xE, 1, "", 0x10, 4,
            &BSShaderLightingProperty::pShaderLightConstants[28], false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "FogColor | ShadowVolumeExtrudeDistance", 0x10000007, 0, 0xF, 1, "", 0x10, 4,
            &BSShaderLightingProperty::pShaderLightConstants[29], false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "EyePosition", 0x10000007, 0, 0x10, 1, "", 0x10, 4, &pEyePosition, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "BoundCenter", 0x10000007, 0, 0x18, 1, "", 0x10, 4, &kWorldBoundCenter, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "LODLandParams", 0x10000007, 0, 0x13, 1, "", 0x10, 4, &BSShaderLightingProperty::kLODLandParams, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry("DepthData", 0x10000007, 0, 0x11, 1, "", 0x10, 4, &kFarNear, false);
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevWorldViewPT", 0x10000009, 0, 0x23, 4, "", 0x40, 4, &kPrevModelViewProj, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevWorldViewT", 0x10000009, 0, 0x27, 4, "", 0x40, 4, &kPrevModelView, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "BoneMatrix3", 0x20000009, 0x120000, 0x2C, 0x36, nullptr, 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewTranspose", 0x20000009, 4, "", 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewTranspose", 0x20000009, 5, "", 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "Time", 0x10000004, 0, 0xD, 1, "", 4, 4, &fConstantfRefractionTime, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevBoneMatrix3", 0x10000009, 0, 0x62, 0x36, "", 0x360, 4, kPrevBones, false
        );
        NiShaderConstantMapEntry* pEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldViewProjTranspose");
        // EXE-INLINED 0x66B0D0
        NiShaderConstantMapEntry* pOldEntry = spVertexConstantMapEntries[0];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[0] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldTranspose");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[1];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[1] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("SkinWorldTranspose");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[12];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[12] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("SkinWorldViewProjTranspose");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[2];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[2] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("ShadowProj");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[3];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[3] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("ShadowProjData");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[4];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[4] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("ShadowProjTransform");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[5];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[5] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("HighDetailRange");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[16];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[16] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("LightData");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[6];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[6] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("FogParam | ShadowVolumeFatness");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[7];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[7] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("FogColor | ShadowVolumeExtrudeDistance");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[8];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[8] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("EyePosition");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[9];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[9] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("BoneMatrix3");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[10];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[10] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldViewTranspose");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[13];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[13] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("SkinWorldViewTranspose");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[14];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[14] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("Time");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[15];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[15] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("BoundCenter");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[17];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[17] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("LODLandParams");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[21];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[21] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("DepthData");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[18];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[18] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("PrevWorldViewPT");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[19];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[19] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("PrevWorldViewT");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[20];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[20] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("PrevBoneMatrix3");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spVertexConstantMapEntries[11];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spVertexConstantMapEntries[11] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        spDefaultVertexConstantMap = m_spVertexConstantMap;
    }
    if (!m_spPixelConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry("eye dir", 0x10000007, 0, 0, 1, "", 0x10, 4, &pEyeVector, false);
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Ambient Color", 0x10000007, 0, 1, 1, "", 0x10, 4, BSShaderLightingProperty::pShaderLightConstants, false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "LightColors", 0x10000009, 0, 3, 0xA, "", 0xA0, 4, &BSShaderLightingProperty::pShaderLightConstants[1],
            false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Light Direction", 0x10000007, 0, 0x12, 1, "", 0x10, 4, pLightData, false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Light Position0", 0x10000007, 0, 0x13, 1, "", 0x10, 4, &pLightData[1], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Light Position1", 0x10000007, 0, 0x14, 1, "", 0x10, 4, &pLightData[2], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Light Position2", 0x10000007, 0, 0x15, 1, "", 0x10, 4, &pLightData[3], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Light Position3", 0x10000007, 0, 0x16, 1, "", 0x10, 4, &pLightData[4], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Emittance Color", 0x10000007, 0, 2, 1, "", 0x10, 4, &BSShaderLightingProperty::pShaderLightConstants[27],
            false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "EyePosition", 0x10000007, 0, 0x11, 1, "", 0x10, 4, &pEyePosition, false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "Toggles", 0x10000007, 0, 0x1B, 1, "", 0x10, 4, &BSShaderLightingProperty::pShaderLightConstants[33], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "HairTint", 0x10000007, 0, 0x1C, 1, "", 0x10, 4, &BSShaderLightingProperty::kHairTint, false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "STBBColorConstants", 0x10000007, 0, 0x1E, 1, "", 0x10, 4,
            &BSShaderLightingProperty::pShaderLightConstants[35], false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "LODTEXPARAMS", 0x10000007, 0, 0x1F, 1, "", 0x10, 4, &BSShaderLightingProperty::pShaderLightConstants[36],
            false
        );
        m_spPixelConstantMap.m_pObject->AddEntry(
            "RefractionParams", 0x10000004, 0, 0x1D, 1, "", 4, 4, &kRefractionParams, false
        );
        m_spPixelConstantMap.m_pObject->AddEntry("FogParams", 0x10000004, 0, 0xE, 1, "", 4, 4, &kFogParams, false);
        m_spPixelConstantMap.m_pObject->AddEntry("FogColor", 0x10000004, 0, 0xF, 1, "", 4, 4, &pFogColor, false);
        m_spPixelConstantMap.m_pObject->AddEntry("FogPlane", 0x10000004, 0, 0x10, 1, "", 4, 4, &kFogPlane, false);
        NiShaderConstantMapEntry* pEntry = m_spPixelConstantMap.m_pObject->GetEntry("HairTint");
        // EXE-INLINED 0x66B0D0
        NiShaderConstantMapEntry* pOldEntry = spPixelConstantMapEntries[15];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[15] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("eye dir");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[0];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[0] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Ambient Color");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[1];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[1] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("LightColors");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[2];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[2] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Light Direction");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[6];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[6] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Light Position0");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[7];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[7] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Light Position1");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[8];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[8] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Light Position2");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[9];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[9] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Light Position3");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[10];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[10] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Emittance Color");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[3];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[3] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("EyePosition");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[5];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[5] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("Toggles");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[4];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[4] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("RefractionParams");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[16];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[16] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("FogParams");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[17];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[17] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("FogColor");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[18];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[18] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("FogPlane");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[19];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[19] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("LODTEXPARAMS");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[20];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[20] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("STBBColorConstants");
        // EXE-INLINED 0x66B0D0
        pOldEntry = spPixelConstantMapEntries[21];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            spPixelConstantMapEntries[21] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        spDefaultPixelConstantMap = m_spPixelConstantMap;
    }
    if (!spTexEffectVConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        spTexEffectVConstantMap = pMap;
        spTexEffectVConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewProjTranspose", 0x20000009, 4, "", 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "BoneMatrix3", 0x20000009, 0x120000, 0xE, 0x36, nullptr, 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "EyePosition", 0x10000007, 0, 8, 1, "", 0x10, 4, &pEyePosition, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry("U Offset", 0x10000004, 0, 9, 1, "", 4, 4, &fTextureUOffset, false);
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "V Offset", 0x10000004, 0, 0xA, 1, "", 4, 4, &fTextureVOffset, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "Z scaler", 0x10000004, 0, 0xB, 1, "", 4, 4, &fInvBoundDiameter, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "FogParam", 0x10000007, 0, 0xC, 1, "", 0x10, 4, &kTexEffectFogParam, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "FogColor", 0x10000007, 0, 0xD, 1, "", 0x10, 4, &kTexEffectFogColor, false
        );
    }
    if (!spTexEffectPConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        spTexEffectPConstantMap = pMap;
        spTexEffectPConstantMap.m_pObject->AddEntry("Fill Color", 0x10000007, 0, 0, 1, "", 0x10, 4, &kFillColor, false);
        spTexEffectPConstantMap.m_pObject->AddEntry("Rim Color", 0x10000007, 0, 1, 1, "", 0x10, 4, &kRimColor, false);
        spTexEffectPConstantMap.m_pObject->AddEntry("fVars", 0x10000007, 0, 2, 1, "", 0x10, 4, &fVars, false);
    }
}

void ShadowLightShader::ReloadShaders() {
    LoadShaders();
    PresetZOnlyPasses_1x();
    PresetAmbientPasses_1x();
    PresetAmbDiffTexPasses_1x();
    PresetAmbDiffDirAndPtPasses_1x();
    PresetDiffuseLightPasses_1x();
    PresetTexturePasses_1x();
    PresetSpecularPasses_1x();
    PresetEnvMapPasses_1x();
    PresetFogPasses_1x();
    PresetTextureEffectPasses_1x();
    if (BSShaderManager::eShaderLevel < 2) {
        PresetLandscape_1x();
    }
    else {
        PresetADPasses_2x();
        PresetADTPasses_2x();
        PresetADTSPasses_2x();
        PresetDiffusePtPasses_2x();
        PresetSpecularPasses_2x();
        PresetEnvMapPasses_2x();
        PresetLandscape_2x();
        PresetRenderNormalsPasses_2x();
        ShadowLightShader::PresetDepthPasses_2x();
        ShadowLightShader::PresetVelocityPasses_2x();
        ShadowLightShader::PresetLocalMapPasses_2x();
        PresetTextureEffectPasses_2x();
        PresetWaterFogPasses_2x();
    }
}

void ShadowLightShader::ClearPassStages() {
    for (uint32_t i = 0; i < 759; ++i) {
        ClearPass(spPass[i].m_pObject);
    }
}

void ShadowLightShader::LoadShaders() {
    LoadVertexShaders();
    LoadPixelShaders();
}

void ShadowLightShader::LoadVertexShaders() {
    BSShaderMacro kMacroArray_SM2[103];
    BSShaderMacro kMacroArray_SM1S[22];
    BSShaderMacro kMacroArray_SM1[31];
    char path[260];
    char compiledShaderName[260];
    kMacroArray_SM1[0].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[0].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[0].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[1].strPath = nullptr;
    kMacroArray_SM1[1].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[1].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[2].strPath = nullptr;
    kMacroArray_SM1[2].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[2].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[3].strPath = "lighting\\1x\\v\\diffusePt.v.hlsl";
    kMacroArray_SM1[3].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[3].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[4].strPath = "lighting\\1x\\v\\diffuseDir.v.hlsl";
    kMacroArray_SM1[4].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[4].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[5].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[5].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[5].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[6].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[6].kMacros[0].Name = "VC";
    kMacroArray_SM1[6].kMacros[0].Definition = "";
    kMacroArray_SM1[6].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[6].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[7].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[7].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[7].kMacros[0].Definition = "";
    kMacroArray_SM1[7].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[7].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[8].strPath = "lighting\\1x\\v\\specularPt.v.hlsl";
    kMacroArray_SM1[8].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[8].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[9].strPath = "lighting\\1x\\v\\specularDir.v.hlsl";
    kMacroArray_SM1[9].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[9].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[10].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[10].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[10].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[11].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[11].kMacros[0].Name = "FOG";
    kMacroArray_SM1[11].kMacros[0].Definition = "";
    kMacroArray_SM1[11].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[11].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[12].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[12].kMacros[0].Name = "VC";
    kMacroArray_SM1[12].kMacros[0].Definition = "";
    kMacroArray_SM1[12].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[12].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[13].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[13].kMacros[0].Name = "FOG";
    kMacroArray_SM1[13].kMacros[0].Definition = "";
    kMacroArray_SM1[13].kMacros[1].Name = "VC";
    kMacroArray_SM1[13].kMacros[1].Definition = "";
    kMacroArray_SM1[13].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[13].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[14].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[14].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[14].kMacros[0].Definition = "";
    kMacroArray_SM1[14].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[14].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[15].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1[15].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[15].kMacros[0].Definition = "";
    kMacroArray_SM1[15].kMacros[1].Name = "FOG";
    kMacroArray_SM1[15].kMacros[1].Definition = "";
    kMacroArray_SM1[15].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[15].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[16].strPath = "lighting\\1x\\v\\ambDiffuseDirAndPt.v.hlsl";
    kMacroArray_SM1[16].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[16].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[17].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[17].kMacros[0].Name = "FOG";
    kMacroArray_SM1[17].kMacros[0].Definition = "";
    kMacroArray_SM1[17].kMacros[1].Name = "NO_TEX";
    kMacroArray_SM1[17].kMacros[1].Definition = "";
    kMacroArray_SM1[17].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[17].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[18].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[18].kMacros[0].Name = "FOG";
    kMacroArray_SM1[18].kMacros[0].Definition = "";
    kMacroArray_SM1[18].kMacros[1].Name = "TEX";
    kMacroArray_SM1[18].kMacros[1].Definition = "";
    kMacroArray_SM1[18].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[18].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[19].strPath = "lighting\\1x\\v\\envmap.v.hlsl";
    kMacroArray_SM1[19].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[19].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[20].strPath = "lighting\\1x\\v\\vertDir.v.hlsl";
    kMacroArray_SM1[20].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[20].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[21].strPath = "lighting\\1x\\v\\vertDir.v.hlsl";
    kMacroArray_SM1[21].kMacros[0].Name = "VC";
    kMacroArray_SM1[21].kMacros[0].Definition = "";
    kMacroArray_SM1[21].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[21].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[22].strPath = "lighting\\1x\\v\\vertDirAndPt.v.hlsl";
    kMacroArray_SM1[22].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[22].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[23].strPath = "lighting\\1x\\v\\vertDirAndPt.v.hlsl";
    kMacroArray_SM1[23].kMacros[0].Name = "VC";
    kMacroArray_SM1[23].kMacros[0].Definition = "";
    kMacroArray_SM1[23].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[23].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[24].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1[24].kMacros[0].Name = "SI";
    kMacroArray_SM1[24].kMacros[0].Definition = "";
    kMacroArray_SM1[24].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[24].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[25].strPath = "lighting\\1x\\v\\land.v.hlsl";
    kMacroArray_SM1[25].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[25].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[26].strPath = "lighting\\1x\\v\\land.v.hlsl";
    kMacroArray_SM1[26].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[26].kMacros[0].Definition = "";
    kMacroArray_SM1[26].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[26].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[27].strPath = "lighting\\1x\\v\\land.v.hlsl";
    kMacroArray_SM1[27].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[27].kMacros[0].Definition = "";
    kMacroArray_SM1[27].kMacros[1].Name = "SI";
    kMacroArray_SM1[27].kMacros[1].Definition = "";
    kMacroArray_SM1[27].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[27].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[28].strPath = "lighting\\1x\\v\\land.v.hlsl";
    kMacroArray_SM1[28].kMacros[0].Name = "PTLIGHT";
    kMacroArray_SM1[28].kMacros[0].Definition = "";
    kMacroArray_SM1[28].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[28].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[29].strPath = "lighting\\1x\\v\\land.v.hlsl";
    kMacroArray_SM1[29].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[29].kMacros[0].Definition = "";
    kMacroArray_SM1[29].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM1[29].kMacros[1].Definition = "";
    kMacroArray_SM1[29].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[29].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[30].strPath = nullptr;
    kMacroArray_SM1[30].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[30].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1S[0].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[0].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[0].kMacros[0].Definition = "";
    kMacroArray_SM1S[0].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[0].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[1].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[1].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[1].kMacros[0].Definition = "";
    kMacroArray_SM1S[1].kMacros[1].Name = "FOG";
    kMacroArray_SM1S[1].kMacros[1].Definition = "";
    kMacroArray_SM1S[1].kMacros[2].Name = "NO_TEX";
    kMacroArray_SM1S[1].kMacros[2].Definition = "";
    kMacroArray_SM1S[1].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[1].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1S[2].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[2].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[2].kMacros[0].Definition = "";
    kMacroArray_SM1S[2].kMacros[1].Name = "FOG";
    kMacroArray_SM1S[2].kMacros[1].Definition = "";
    kMacroArray_SM1S[2].kMacros[2].Name = "TEX";
    kMacroArray_SM1S[2].kMacros[2].Definition = "";
    kMacroArray_SM1S[2].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[2].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1S[3].strPath = "lighting\\1x\\v\\diffusePt.v.hlsl";
    kMacroArray_SM1S[3].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[3].kMacros[0].Definition = "";
    kMacroArray_SM1S[3].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[3].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[4].strPath = "lighting\\1x\\v\\diffuseDir.v.hlsl";
    kMacroArray_SM1S[4].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[4].kMacros[0].Definition = "";
    kMacroArray_SM1S[4].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[4].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[5].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[5].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[5].kMacros[0].Definition = "";
    kMacroArray_SM1S[5].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[5].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[6].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[6].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[6].kMacros[0].Definition = "";
    kMacroArray_SM1S[6].kMacros[1].Name = "VC";
    kMacroArray_SM1S[6].kMacros[1].Definition = "";
    kMacroArray_SM1S[6].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[6].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[7].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[7].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[7].kMacros[0].Definition = "";
    kMacroArray_SM1S[7].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray_SM1S[7].kMacros[1].Definition = "";
    kMacroArray_SM1S[7].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[7].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[8].strPath = "lighting\\1x\\v\\specularPt.v.hlsl";
    kMacroArray_SM1S[8].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[8].kMacros[0].Definition = "";
    kMacroArray_SM1S[8].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[8].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[9].strPath = "lighting\\1x\\v\\specularDir.v.hlsl";
    kMacroArray_SM1S[9].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[9].kMacros[0].Definition = "";
    kMacroArray_SM1S[9].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[9].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[10].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[10].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[10].kMacros[0].Definition = "";
    kMacroArray_SM1S[10].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[10].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[11].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[11].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[11].kMacros[0].Definition = "";
    kMacroArray_SM1S[11].kMacros[1].Name = "VC";
    kMacroArray_SM1S[11].kMacros[1].Definition = "";
    kMacroArray_SM1S[11].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[11].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[12].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[12].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[12].kMacros[0].Definition = "";
    kMacroArray_SM1S[12].kMacros[1].Name = "FOG";
    kMacroArray_SM1S[12].kMacros[1].Definition = "";
    kMacroArray_SM1S[12].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[12].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[13].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[13].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[13].kMacros[0].Definition = "";
    kMacroArray_SM1S[13].kMacros[1].Name = "FOG";
    kMacroArray_SM1S[13].kMacros[1].Definition = "";
    kMacroArray_SM1S[13].kMacros[2].Name = "VC";
    kMacroArray_SM1S[13].kMacros[2].Definition = "";
    kMacroArray_SM1S[13].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[13].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1S[14].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[14].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[14].kMacros[0].Definition = "";
    kMacroArray_SM1S[14].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray_SM1S[14].kMacros[1].Definition = "";
    kMacroArray_SM1S[14].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[14].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[15].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
    kMacroArray_SM1S[15].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[15].kMacros[0].Definition = "";
    kMacroArray_SM1S[15].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray_SM1S[15].kMacros[1].Definition = "";
    kMacroArray_SM1S[15].kMacros[2].Name = "FOG";
    kMacroArray_SM1S[15].kMacros[2].Definition = "";
    kMacroArray_SM1S[15].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[15].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1S[16].strPath = "lighting\\1x\\v\\ambDiffuseDirAndPt.v.hlsl";
    kMacroArray_SM1S[16].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[16].kMacros[0].Definition = "";
    kMacroArray_SM1S[16].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[16].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[17].strPath = "lighting\\1x\\v\\envmap.v.hlsl";
    kMacroArray_SM1S[17].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[17].kMacros[0].Definition = "";
    kMacroArray_SM1S[17].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[17].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1S[18].strPath = nullptr;
    kMacroArray_SM1S[18].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[18].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1S[19].strPath = nullptr;
    kMacroArray_SM1S[19].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[19].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1S[20].strPath = "lighting\\1x\\v\\base.v.hlsl";
    kMacroArray_SM1S[20].kMacros[0].Name = "SKIN";
    kMacroArray_SM1S[20].kMacros[0].Definition = "";
    kMacroArray_SM1S[20].kMacros[1].Name = "SI";
    kMacroArray_SM1S[20].kMacros[1].Definition = "";
    kMacroArray_SM1S[20].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[20].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1S[21].strPath = nullptr;
    kMacroArray_SM1S[21].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1S[21].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[0].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[0].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[0].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[1].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[1].kMacros[0].Name = "LOD";
    kMacroArray_SM2[1].kMacros[0].Definition = "";
    kMacroArray_SM2[1].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[1].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[2].strPath = "lighting\\2x\\v\\landlod.v.hlsl";
    kMacroArray_SM2[2].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[2].kMacros[0].Definition = "";
    kMacroArray_SM2[2].kMacros[1].Name = "LODLANDCLIP";
    kMacroArray_SM2[2].kMacros[1].Definition = "";
    kMacroArray_SM2[2].kMacros[2].Name = "GEOMORPH";
    kMacroArray_SM2[2].kMacros[2].Definition = "";
    kMacroArray_SM2[2].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[2].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[3].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[3].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[3].kMacros[0].Definition = "";
    kMacroArray_SM2[3].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[3].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[4].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[4].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[4].kMacros[0].Definition = "";
    kMacroArray_SM2[4].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[4].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[5].strPath = "lighting\\2x\\v\\landlod.v.hlsl";
    kMacroArray_SM2[5].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[5].kMacros[0].Definition = "";
    kMacroArray_SM2[5].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[5].kMacros[1].Definition = "";
    kMacroArray_SM2[5].kMacros[2].Name = "LODLANDCLIP";
    kMacroArray_SM2[5].kMacros[2].Definition = "";
    kMacroArray_SM2[5].kMacros[3].Name = "GEOMORPH";
    kMacroArray_SM2[5].kMacros[3].Definition = "";
    kMacroArray_SM2[5].kMacros[4].Name = nullptr;
    kMacroArray_SM2[5].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[5].kMacros[5].Name = nullptr;
    kMacroArray_SM2[5].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[5].kMacros[6].Name = nullptr;
    kMacroArray_SM2[5].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[5].kMacros[7].Name = nullptr;
    kMacroArray_SM2[5].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[5].kMacros[8].Name = nullptr;
    kMacroArray_SM2[5].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[6].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[6].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[6].kMacros[0].Definition = "";
    kMacroArray_SM2[6].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[6].kMacros[1].Definition = "";
    kMacroArray_SM2[6].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[6].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[7].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[7].kMacros[0].Name = "STBB";
    kMacroArray_SM2[7].kMacros[0].Definition = "";
    kMacroArray_SM2[7].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[7].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[8].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[8].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[8].kMacros[0].Definition = "2";
    kMacroArray_SM2[8].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[8].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[9].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[9].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[9].kMacros[0].Definition = "2";
    kMacroArray_SM2[9].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[9].kMacros[1].Definition = "";
    kMacroArray_SM2[9].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[9].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[10].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[10].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[10].kMacros[0].Definition = "2";
    kMacroArray_SM2[10].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[10].kMacros[1].Definition = "";
    kMacroArray_SM2[10].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[10].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[11].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[11].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[11].kMacros[0].Definition = "2";
    kMacroArray_SM2[11].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[11].kMacros[1].Definition = "";
    kMacroArray_SM2[11].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[11].kMacros[2].Definition = "";
    kMacroArray_SM2[11].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[11].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[12].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[12].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[12].kMacros[0].Definition = "";
    kMacroArray_SM2[12].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[12].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[13].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[13].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[13].kMacros[0].Definition = "";
    kMacroArray_SM2[13].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[13].kMacros[1].Definition = "";
    kMacroArray_SM2[13].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[13].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[14].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[14].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[14].kMacros[0].Definition = "";
    kMacroArray_SM2[14].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[14].kMacros[1].Definition = "";
    kMacroArray_SM2[14].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[14].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[15].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[15].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[15].kMacros[0].Definition = "";
    kMacroArray_SM2[15].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[15].kMacros[1].Definition = "";
    kMacroArray_SM2[15].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[15].kMacros[2].Definition = "";
    kMacroArray_SM2[15].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[15].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[16].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[16].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[16].kMacros[0].Definition = "";
    kMacroArray_SM2[16].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[16].kMacros[1].Definition = "2";
    kMacroArray_SM2[16].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[16].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[17].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[17].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[17].kMacros[0].Definition = "";
    kMacroArray_SM2[17].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[17].kMacros[1].Definition = "2";
    kMacroArray_SM2[17].kMacros[2].Name = "SKIN";
    kMacroArray_SM2[17].kMacros[2].Definition = "";
    kMacroArray_SM2[17].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[17].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[18].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[18].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[18].kMacros[0].Definition = "";
    kMacroArray_SM2[18].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[18].kMacros[1].Definition = "2";
    kMacroArray_SM2[18].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[18].kMacros[2].Definition = "";
    kMacroArray_SM2[18].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[18].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[19].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray_SM2[19].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[19].kMacros[0].Definition = "";
    kMacroArray_SM2[19].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[19].kMacros[1].Definition = "2";
    kMacroArray_SM2[19].kMacros[2].Name = "SKIN";
    kMacroArray_SM2[19].kMacros[2].Definition = "";
    kMacroArray_SM2[19].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[19].kMacros[3].Definition = "";
    kMacroArray_SM2[19].kMacros[4].Name = nullptr;
    kMacroArray_SM2[19].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[19].kMacros[5].Name = nullptr;
    kMacroArray_SM2[19].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[19].kMacros[6].Name = nullptr;
    kMacroArray_SM2[19].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[19].kMacros[7].Name = nullptr;
    kMacroArray_SM2[19].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[19].kMacros[8].Name = nullptr;
    kMacroArray_SM2[19].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[20].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[20].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[20].kMacros[0].Definition = "9";
    kMacroArray_SM2[20].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[20].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[21].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[21].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[21].kMacros[0].Definition = "9";
    kMacroArray_SM2[21].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[21].kMacros[1].Definition = "";
    kMacroArray_SM2[21].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[21].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[22].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[22].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[22].kMacros[0].Definition = "4";
    kMacroArray_SM2[22].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[22].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[23].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[23].kMacros[0].Name = "OPT";
    kMacroArray_SM2[23].kMacros[0].Definition = "";
    kMacroArray_SM2[23].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[23].kMacros[1].Definition = "4";
    kMacroArray_SM2[23].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[23].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[24].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[24].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[24].kMacros[0].Definition = "4";
    kMacroArray_SM2[24].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[24].kMacros[1].Definition = "";
    kMacroArray_SM2[24].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[24].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[25].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[25].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[25].kMacros[0].Definition = "";
    kMacroArray_SM2[25].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[25].kMacros[1].Definition = "4";
    kMacroArray_SM2[25].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[25].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[26].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[26].kMacros[0].Name = "OPT";
    kMacroArray_SM2[26].kMacros[0].Definition = "";
    kMacroArray_SM2[26].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[26].kMacros[1].Definition = "";
    kMacroArray_SM2[26].kMacros[2].Name = "LIGHTS";
    kMacroArray_SM2[26].kMacros[2].Definition = "4";
    kMacroArray_SM2[26].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[26].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[27].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray_SM2[27].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[27].kMacros[0].Definition = "";
    kMacroArray_SM2[27].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[27].kMacros[1].Definition = "4";
    kMacroArray_SM2[27].kMacros[2].Name = "SKIN";
    kMacroArray_SM2[27].kMacros[2].Definition = "";
    kMacroArray_SM2[27].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[27].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[28].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[28].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[28].kMacros[0].Definition = "2";
    kMacroArray_SM2[28].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[28].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[29].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[29].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[29].kMacros[0].Definition = "2";
    kMacroArray_SM2[29].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[29].kMacros[1].Definition = "";
    kMacroArray_SM2[29].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[29].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[30].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[30].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[30].kMacros[0].Definition = "2";
    kMacroArray_SM2[30].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[30].kMacros[1].Definition = "";
    kMacroArray_SM2[30].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[30].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[31].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[31].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[31].kMacros[0].Definition = "2";
    kMacroArray_SM2[31].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[31].kMacros[1].Definition = "";
    kMacroArray_SM2[31].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[31].kMacros[2].Definition = "";
    kMacroArray_SM2[31].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[31].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[32].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[32].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[32].kMacros[0].Definition = "3";
    kMacroArray_SM2[32].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[32].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[33].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[33].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[33].kMacros[0].Definition = "3";
    kMacroArray_SM2[33].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[33].kMacros[1].Definition = "";
    kMacroArray_SM2[33].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[33].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[34].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[34].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[34].kMacros[0].Definition = "3";
    kMacroArray_SM2[34].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[34].kMacros[1].Definition = "";
    kMacroArray_SM2[34].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[34].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[35].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray_SM2[35].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[35].kMacros[0].Definition = "3";
    kMacroArray_SM2[35].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[35].kMacros[1].Definition = "";
    kMacroArray_SM2[35].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[35].kMacros[2].Definition = "";
    kMacroArray_SM2[35].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[35].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[36].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray_SM2[36].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[36].kMacros[0].Definition = "2";
    kMacroArray_SM2[36].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[36].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[37].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray_SM2[37].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[37].kMacros[0].Definition = "2";
    kMacroArray_SM2[37].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[37].kMacros[1].Definition = "";
    kMacroArray_SM2[37].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[37].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[38].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray_SM2[38].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[38].kMacros[0].Definition = "3";
    kMacroArray_SM2[38].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[38].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[39].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray_SM2[39].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[39].kMacros[0].Definition = "3";
    kMacroArray_SM2[39].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[39].kMacros[1].Definition = "";
    kMacroArray_SM2[39].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[39].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[40].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[40].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[40].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[41].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[41].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[41].kMacros[0].Definition = "";
    kMacroArray_SM2[41].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[41].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[42].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[42].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[42].kMacros[0].Definition = "";
    kMacroArray_SM2[42].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[42].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[43].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[43].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[43].kMacros[0].Definition = "";
    kMacroArray_SM2[43].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[43].kMacros[1].Definition = "";
    kMacroArray_SM2[43].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[43].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[44].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[44].kMacros[0].Name = "POINT";
    kMacroArray_SM2[44].kMacros[0].Definition = "";
    kMacroArray_SM2[44].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[44].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[45].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[45].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[45].kMacros[0].Definition = "";
    kMacroArray_SM2[45].kMacros[1].Name = "POINT";
    kMacroArray_SM2[45].kMacros[1].Definition = "";
    kMacroArray_SM2[45].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[45].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[46].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[46].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[46].kMacros[0].Definition = "2";
    kMacroArray_SM2[46].kMacros[1].Name = "POINT";
    kMacroArray_SM2[46].kMacros[1].Definition = "";
    kMacroArray_SM2[46].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[46].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[47].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[47].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[47].kMacros[0].Definition = "2";
    kMacroArray_SM2[47].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[47].kMacros[1].Definition = "";
    kMacroArray_SM2[47].kMacros[2].Name = "POINT";
    kMacroArray_SM2[47].kMacros[2].Definition = "";
    kMacroArray_SM2[47].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[47].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[48].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[48].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[48].kMacros[0].Definition = "3";
    kMacroArray_SM2[48].kMacros[1].Name = "POINT";
    kMacroArray_SM2[48].kMacros[1].Definition = "";
    kMacroArray_SM2[48].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[48].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[49].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
    kMacroArray_SM2[49].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[49].kMacros[0].Definition = "3";
    kMacroArray_SM2[49].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[49].kMacros[1].Definition = "";
    kMacroArray_SM2[49].kMacros[2].Name = "POINT";
    kMacroArray_SM2[49].kMacros[2].Definition = "";
    kMacroArray_SM2[49].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[49].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[50].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    kMacroArray_SM2[50].kMacros[0].Name = "ENVMAP";
    kMacroArray_SM2[50].kMacros[0].Definition = "";
    kMacroArray_SM2[50].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[50].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[51].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    kMacroArray_SM2[51].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[51].kMacros[0].Definition = "";
    kMacroArray_SM2[51].kMacros[1].Name = "ENVMAP";
    kMacroArray_SM2[51].kMacros[1].Definition = "";
    kMacroArray_SM2[51].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[51].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[52].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    kMacroArray_SM2[52].kMacros[0].Name = "ENVMAP";
    kMacroArray_SM2[52].kMacros[0].Definition = "";
    kMacroArray_SM2[52].kMacros[1].Name = "EYE";
    kMacroArray_SM2[52].kMacros[1].Definition = nullptr;
    kMacroArray_SM2[52].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[52].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[53].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[53].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[53].kMacros[0].Definition = "";
    kMacroArray_SM2[53].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[53].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[54].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[54].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[54].kMacros[0].Definition = "";
    kMacroArray_SM2[54].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[54].kMacros[1].Definition = "";
    kMacroArray_SM2[54].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[54].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[55].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[55].kMacros[0].Name = "DIFF";
    kMacroArray_SM2[55].kMacros[0].Definition = "";
    kMacroArray_SM2[55].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM2[55].kMacros[1].Definition = "";
    kMacroArray_SM2[55].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[55].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[56].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[56].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[56].kMacros[0].Definition = "";
    kMacroArray_SM2[56].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[56].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[57].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[57].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[58].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[58].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[58].kMacros[0].Definition = "";
    kMacroArray_SM2[58].kMacros[1].Name = "DIFF";
    kMacroArray_SM2[58].kMacros[1].Definition = "";
    kMacroArray_SM2[58].kMacros[2].Name = "PTLIGHT";
    kMacroArray_SM2[58].kMacros[2].Definition = "";
    kMacroArray_SM2[58].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[58].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[59].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[59].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[59].kMacros[0].Definition = "";
    kMacroArray_SM2[59].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[59].kMacros[1].Definition = "";
    kMacroArray_SM2[59].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[59].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[60].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[60].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[61].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[61].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[61].kMacros[0].Definition = "";
    kMacroArray_SM2[61].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[61].kMacros[1].Definition = "";
    kMacroArray_SM2[61].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[61].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[62].strPath = "lighting\\2x\\v\\landlighting.v.hlsl";
    kMacroArray_SM2[62].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[62].kMacros[0].Definition = "";
    kMacroArray_SM2[62].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[62].kMacros[1].Definition = "";
    kMacroArray_SM2[62].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[62].kMacros[2].Definition = "";
    kMacroArray_SM2[62].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[62].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[63].strPath = "lighting\\2x\\v\\depthmap.v.hlsl";
    kMacroArray_SM2[63].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[63].kMacros[0].Definition = "";
    kMacroArray_SM2[63].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[63].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[64].strPath = "lighting\\2x\\v\\depthmap.v.hlsl";
    kMacroArray_SM2[64].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[64].kMacros[0].Definition = "";
    kMacroArray_SM2[64].kMacros[1].Name = "ALPHATEST";
    kMacroArray_SM2[64].kMacros[1].Definition = "";
    kMacroArray_SM2[64].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[64].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[65].strPath = "lighting\\2x\\v\\depthmap.v.hlsl";
    kMacroArray_SM2[65].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[65].kMacros[0].Definition = "";
    kMacroArray_SM2[65].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[65].kMacros[1].Definition = "";
    kMacroArray_SM2[65].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[65].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[66].strPath = "lighting\\2x\\v\\depthmap.v.hlsl";
    kMacroArray_SM2[66].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[66].kMacros[0].Definition = "";
    kMacroArray_SM2[66].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[66].kMacros[1].Definition = "";
    kMacroArray_SM2[66].kMacros[2].Name = "ALPHATEST";
    kMacroArray_SM2[66].kMacros[2].Definition = "";
    kMacroArray_SM2[66].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[66].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[67].strPath = "lighting\\2x\\v\\depthmap.v.hlsl";
    kMacroArray_SM2[67].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[67].kMacros[0].Definition = "";
    kMacroArray_SM2[67].kMacros[1].Name = "WATERDEPTH";
    kMacroArray_SM2[67].kMacros[1].Definition = "";
    kMacroArray_SM2[67].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[67].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[68].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    kMacroArray_SM2[68].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[68].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[69].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    kMacroArray_SM2[69].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[69].kMacros[0].Definition = "";
    kMacroArray_SM2[69].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[69].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[70].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    kMacroArray_SM2[70].kMacros[0].Name = "FIRE";
    kMacroArray_SM2[70].kMacros[0].Definition = "";
    kMacroArray_SM2[70].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[70].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[71].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    kMacroArray_SM2[71].kMacros[0].Name = "CLEAR";
    kMacroArray_SM2[71].kMacros[0].Definition = "";
    kMacroArray_SM2[71].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[71].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[72].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    kMacroArray_SM2[72].kMacros[0].Name = "CLEAR";
    kMacroArray_SM2[72].kMacros[0].Definition = "";
    kMacroArray_SM2[72].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[72].kMacros[1].Definition = "";
    kMacroArray_SM2[72].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[72].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[73].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    kMacroArray_SM2[73].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[73].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[74].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    kMacroArray_SM2[74].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[74].kMacros[0].Definition = "";
    kMacroArray_SM2[74].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[74].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[75].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    kMacroArray_SM2[75].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[75].kMacros[0].Definition = "";
    kMacroArray_SM2[75].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[75].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[76].strPath = "lighting\\2x\\v\\renderVelocity.v.hlsl";
    kMacroArray_SM2[76].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[76].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[77].strPath = "lighting\\2x\\v\\renderVelocity.v.hlsl";
    kMacroArray_SM2[77].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[77].kMacros[0].Definition = "";
    kMacroArray_SM2[77].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[77].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[78].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[78].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[78].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[79].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[79].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[79].kMacros[0].Definition = "";
    kMacroArray_SM2[79].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[79].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[80].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[80].kMacros[0].Name = "LANDLO";
    kMacroArray_SM2[80].kMacros[0].Definition = "";
    kMacroArray_SM2[80].kMacros[1].Name = "FOG";
    kMacroArray_SM2[80].kMacros[1].Definition = "";
    kMacroArray_SM2[80].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[80].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[81].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[81].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[81].kMacros[0].Definition = "";
    kMacroArray_SM2[81].kMacros[1].Name = "SI";
    kMacroArray_SM2[81].kMacros[1].Definition = "";
    kMacroArray_SM2[81].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[81].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[82].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[82].kMacros[0].Name = "PTLIGHT";
    kMacroArray_SM2[82].kMacros[0].Definition = "";
    kMacroArray_SM2[82].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[82].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[83].strPath = "lighting\\2x\\v\\land.v.hlsl";
    kMacroArray_SM2[83].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[83].kMacros[0].Definition = "";
    kMacroArray_SM2[83].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM2[83].kMacros[1].Definition = "";
    kMacroArray_SM2[83].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[83].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[84].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    kMacroArray_SM2[84].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[84].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[85].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    kMacroArray_SM2[85].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[85].kMacros[0].Definition = "";
    kMacroArray_SM2[85].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[85].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[86].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    kMacroArray_SM2[86].kMacros[0].Name = "CLEAR";
    kMacroArray_SM2[86].kMacros[0].Definition = "";
    kMacroArray_SM2[86].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[86].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[87].strPath = "lighting\\1x\\v\\texEffect.v.hlsl";
    kMacroArray_SM2[87].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[87].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[88].strPath = "lighting\\1x\\v\\texEffect.v.hlsl";
    kMacroArray_SM2[88].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[88].kMacros[0].Definition = "";
    kMacroArray_SM2[88].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[88].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[89].strPath = "lighting\\2x\\v\\SimpleShadow.v.hlsl";
    kMacroArray_SM2[89].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[89].kMacros[0].Definition = "";
    kMacroArray_SM2[89].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[89].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[90].strPath = "lighting\\2x\\v\\SimpleShadow.v.hlsl";
    kMacroArray_SM2[90].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[90].kMacros[0].Definition = "";
    kMacroArray_SM2[90].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[90].kMacros[1].Definition = "";
    kMacroArray_SM2[90].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[90].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[91].strPath = "lighting\\2x\\v\\SimpleShadow.v.hlsl";
    kMacroArray_SM2[91].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[91].kMacros[0].Definition = "";
    kMacroArray_SM2[91].kMacros[1].Name = "LAND";
    kMacroArray_SM2[91].kMacros[1].Definition = "";
    kMacroArray_SM2[91].kMacros[2].Name = "VC";
    kMacroArray_SM2[91].kMacros[2].Definition = "";
    kMacroArray_SM2[91].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[91].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[92].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[92].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[92].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[93].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[93].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[93].kMacros[0].Definition = "";
    kMacroArray_SM2[93].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[93].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[94].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[94].kMacros[0].Name = "TEXCOORD";
    kMacroArray_SM2[94].kMacros[0].Definition = "";
    kMacroArray_SM2[94].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[94].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[95].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[95].kMacros[0].Name = "SKIN";
    kMacroArray_SM2[95].kMacros[0].Definition = "";
    kMacroArray_SM2[95].kMacros[1].Name = "TEXCOORD";
    kMacroArray_SM2[95].kMacros[1].Definition = "";
    kMacroArray_SM2[95].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[95].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[96].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[96].kMacros[0].Name = "VC";
    kMacroArray_SM2[96].kMacros[0].Definition = "";
    kMacroArray_SM2[96].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[96].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[97].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[97].kMacros[0].Name = "VC";
    kMacroArray_SM2[97].kMacros[0].Definition = "";
    kMacroArray_SM2[97].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[97].kMacros[1].Definition = "";
    kMacroArray_SM2[97].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[97].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[98].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[98].kMacros[0].Name = "VC";
    kMacroArray_SM2[98].kMacros[0].Definition = "";
    kMacroArray_SM2[98].kMacros[1].Name = "TEXCOORD";
    kMacroArray_SM2[98].kMacros[1].Definition = "";
    kMacroArray_SM2[98].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[98].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[99].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    kMacroArray_SM2[99].kMacros[0].Name = "VC";
    kMacroArray_SM2[99].kMacros[0].Definition = "";
    kMacroArray_SM2[99].kMacros[1].Name = "SKIN";
    kMacroArray_SM2[99].kMacros[1].Definition = "";
    kMacroArray_SM2[99].kMacros[2].Name = "TEXCOORD";
    kMacroArray_SM2[99].kMacros[2].Definition = "";
    kMacroArray_SM2[99].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[99].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[100].strPath = "lighting\\2x\\v\\LandO.v.hlsl";
    kMacroArray_SM2[100].kMacros[0].Name = "FOG";
    kMacroArray_SM2[100].kMacros[0].Definition = "";
    kMacroArray_SM2[100].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[100].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[101].strPath = "lighting\\2x\\v\\LandO.v.hlsl";
    kMacroArray_SM2[101].kMacros[0].Name = "FOG";
    kMacroArray_SM2[101].kMacros[0].Definition = "";
    kMacroArray_SM2[101].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[101].kMacros[1].Definition = "";
    kMacroArray_SM2[101].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[101].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[102].strPath = "lighting\\2x\\v\\adts.v.hlsl";
    kMacroArray_SM2[102].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[102].kMacros[0].Definition = a0_0;
    kMacroArray_SM2[102].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[102].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    int32_t v1 = 0;
    BSShaderMacro* v2 = kMacroArray_SM1;
    do {
        if (v2->strPath) {
            BSShader::MakePath(v2->strPath, path);
            BSsprintf(compiledShaderName, 260, "SLS1%03i.vso", v1);
            NiD3DVertexShader* v3 = CompileVertexShaderHLSL(path, v2->kMacros, ShaderTarget_vs_2_0, compiledShaderName);
            NiD3DVertexShader* v4 = ShadowLightShader::spVertexShaders[v1];
            NiD3DVertexShader* v16 = v3;
            if (v4 != v3) {
                if (v4) {
                    NiDecRefCount(v4);
                    v3 = v16;
                }
                ShadowLightShader::spVertexShaders[v1] = v3;
                if (v3) {
                    NiIncRefCount(v3);
                }
            }
        }
        ++v1;
        ++v2;
    }
    while (v1 < 31);
    BSShaderMacro* v5 = kMacroArray_SM1S;
    int32_t v6 = 0;
    BSShaderMacro* v17 = kMacroArray_SM1S;
    do {
        if (v5->strPath) {
            BSShader::MakePath(v5->strPath, path);
            BSsprintf(compiledShaderName, 260, "SLS1S%03i.vso", v6);
            NiD3DVertexShader* v7 = CompileVertexShaderHLSL(path, v5->kMacros, ShaderTarget_vs_2_0, compiledShaderName);
            NiD3DVertexShader* v8 = ShadowLightShader::spVertexShaders[v6 + 31];
            NiD3DVertexShader* v9 = v7;
            if (v8 != v7) {
                if (v8) {
                    NiDecRefCount(v8);
                }
                ShadowLightShader::spVertexShaders[v6 + 31] = v9;
                if (v9) {
                    NiIncRefCount(v9);
                }
            }
        }
        ++v6;
        v5 = ++v17;
    }
    while (v6 < 22);
    if (BSShaderManager::eShaderLevel >= 2) {
        BSShaderMacro* v10 = kMacroArray_SM2;
        int32_t v11 = 0;
        BSShaderMacro* v18 = kMacroArray_SM2;
        do {
            if (v10->strPath) {
                BSShader::MakePath(v10->strPath, path);
                BSsprintf(compiledShaderName, 260, "SLS2%03i.vso", v11);
                NiD3DVertexShader* v13 =
                    CompileVertexShaderHLSL(path, v10->kMacros, ShaderTarget_vs_2_0, compiledShaderName);
                NiD3DVertexShader* v14 = ShadowLightShader::spVertexShaders[v11 + 53];
                NiD3DVertexShader* v15 = v13;
                if (v14 != v13) {
                    if (v14) {
                        NiDecRefCount(v14);
                    }
                    ShadowLightShader::spVertexShaders[v11 + 53] = v15;
                    if (v15) {
                        NiIncRefCount(v15);
                    }
                }
            }
            else {
                NiD3DVertexShader* v12 = ShadowLightShader::spVertexShaders[v11 + 53];
                if (v12) {
                    NiDecRefCount(v12);
                    ShadowLightShader::spVertexShaders[v11 + 53] = nullptr;
                }
            }
            ++v11;
            v10 = ++v18;
        }
        while (v11 < 103);
    }
}

void ShadowLightShader::LoadPixelShaders() {
    // The shadow mode's "%i" text: four bytes it is formatted into.
    char acShadowMode[4];
    BSShaderMacro kMacroArray_SM2[160];
    BSShaderMacro kMacroArray_SM1[48];
    char PSOFile[260];
    char HLSLFile[260];
    char fEmissiveMult[32];
    char fSpecularMult[32];
    BSShader* v13 = (BSShader*)this;
    _gcvt(ImageSpaceEffectBlur::fSpecularMult, 12, fSpecularMult);
    _gcvt(ImageSpaceEffectBlur::fEmmisiveMult, 12, fEmissiveMult);
    kMacroArray_SM1[0].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[0].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[0].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[1].strPath = "lighting\\1x\\p\\diffusePt.p.hlsl";
    kMacroArray_SM1[1].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[1].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[2].strPath = "lighting\\1x\\p\\diffuseDir.p.hlsl";
    kMacroArray_SM1[2].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[2].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[3].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[3].kMacros[0].Name = "TEX";
    kMacroArray_SM1[3].kMacros[0].Definition = "";
    kMacroArray_SM1[3].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[3].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[4].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[4].kMacros[0].Name = "TEX";
    kMacroArray_SM1[4].kMacros[0].Definition = "";
    kMacroArray_SM1[4].kMacros[1].Name = "VC";
    kMacroArray_SM1[4].kMacros[1].Definition = "";
    kMacroArray_SM1[4].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[4].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[5].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[5].kMacros[0].Name = "TEX";
    kMacroArray_SM1[5].kMacros[0].Definition = "";
    kMacroArray_SM1[5].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray_SM1[5].kMacros[1].Definition = "";
    kMacroArray_SM1[5].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[5].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[6].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[6].kMacros[0].Name = "TEX";
    kMacroArray_SM1[6].kMacros[0].Definition = "";
    kMacroArray_SM1[6].kMacros[1].Name = "HAIR";
    kMacroArray_SM1[6].kMacros[1].Definition = "";
    kMacroArray_SM1[6].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[6].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[7].strPath = "lighting\\1x\\p\\specularPt.p.hlsl";
    kMacroArray_SM1[7].kMacros[0].Name = "SPECMULT";
    kMacroArray_SM1[7].kMacros[0].Definition = fSpecularMult;
    kMacroArray_SM1[7].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[7].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[8].strPath = "lighting\\1x\\p\\specularDir.p.hlsl";
    kMacroArray_SM1[8].kMacros[0].Name = "SPECMULT";
    kMacroArray_SM1[8].kMacros[0].Definition = fSpecularMult;
    kMacroArray_SM1[8].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[8].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[9].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[9].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[9].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[10].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[10].kMacros[0].Name = "FOG";
    kMacroArray_SM1[10].kMacros[0].Definition = "";
    kMacroArray_SM1[10].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[10].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[11].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[11].kMacros[0].Name = "ALPHA";
    kMacroArray_SM1[11].kMacros[0].Definition = "";
    kMacroArray_SM1[11].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[11].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[12].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[12].kMacros[0].Name = "ALPHA";
    kMacroArray_SM1[12].kMacros[0].Definition = "";
    kMacroArray_SM1[12].kMacros[1].Name = "FOG";
    kMacroArray_SM1[12].kMacros[1].Definition = "";
    kMacroArray_SM1[12].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[12].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[13].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[13].kMacros[0].Name = "VC";
    kMacroArray_SM1[13].kMacros[0].Definition = "";
    kMacroArray_SM1[13].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[13].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[14].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[14].kMacros[0].Name = "VC";
    kMacroArray_SM1[14].kMacros[0].Definition = "";
    kMacroArray_SM1[14].kMacros[1].Name = "FOG";
    kMacroArray_SM1[14].kMacros[1].Definition = "";
    kMacroArray_SM1[14].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[14].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[15].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[15].kMacros[0].Name = "VC";
    kMacroArray_SM1[15].kMacros[0].Definition = "";
    kMacroArray_SM1[15].kMacros[1].Name = "ALPHA";
    kMacroArray_SM1[15].kMacros[1].Definition = "";
    kMacroArray_SM1[15].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[15].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[16].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[16].kMacros[0].Name = "VC";
    kMacroArray_SM1[16].kMacros[0].Definition = "";
    kMacroArray_SM1[16].kMacros[1].Name = "ALPHA";
    kMacroArray_SM1[16].kMacros[1].Definition = "";
    kMacroArray_SM1[16].kMacros[2].Name = "FOG";
    kMacroArray_SM1[16].kMacros[2].Definition = "";
    kMacroArray_SM1[16].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[16].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1[17].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[17].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[17].kMacros[0].Definition = "";
    kMacroArray_SM1[17].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[17].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[18].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[18].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[18].kMacros[0].Definition = "";
    kMacroArray_SM1[18].kMacros[1].Name = "FOG";
    kMacroArray_SM1[18].kMacros[1].Definition = "";
    kMacroArray_SM1[18].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[18].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[19].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[19].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[19].kMacros[0].Definition = "";
    kMacroArray_SM1[19].kMacros[1].Name = "ALPHA";
    kMacroArray_SM1[19].kMacros[1].Definition = "";
    kMacroArray_SM1[19].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[19].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[20].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[20].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray_SM1[20].kMacros[0].Definition = "";
    kMacroArray_SM1[20].kMacros[1].Name = "FOG";
    kMacroArray_SM1[20].kMacros[1].Definition = "";
    kMacroArray_SM1[20].kMacros[2].Name = "ALPHA";
    kMacroArray_SM1[20].kMacros[2].Definition = "";
    kMacroArray_SM1[20].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[20].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1[21].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[21].kMacros[0].Name = "SI";
    kMacroArray_SM1[21].kMacros[0].Definition = "";
    kMacroArray_SM1[21].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[21].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[22].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[22].kMacros[0].Name = "SI";
    kMacroArray_SM1[22].kMacros[0].Definition = "";
    kMacroArray_SM1[22].kMacros[1].Name = "FOG";
    kMacroArray_SM1[22].kMacros[1].Definition = "";
    kMacroArray_SM1[22].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[22].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[23].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[23].kMacros[0].Name = "SI";
    kMacroArray_SM1[23].kMacros[0].Definition = "";
    kMacroArray_SM1[23].kMacros[1].Name = "ALPHA";
    kMacroArray_SM1[23].kMacros[1].Definition = "";
    kMacroArray_SM1[23].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[23].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[24].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[24].kMacros[0].Name = "SI";
    kMacroArray_SM1[24].kMacros[0].Definition = "";
    kMacroArray_SM1[24].kMacros[1].Name = "ALPHA";
    kMacroArray_SM1[24].kMacros[1].Definition = "";
    kMacroArray_SM1[24].kMacros[2].Name = "FOG";
    kMacroArray_SM1[24].kMacros[2].Definition = "";
    kMacroArray_SM1[24].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[24].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1[25].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[25].kMacros[0].Name = "SI";
    kMacroArray_SM1[25].kMacros[0].Definition = "";
    kMacroArray_SM1[25].kMacros[1].Name = "VC";
    kMacroArray_SM1[25].kMacros[1].Definition = "";
    kMacroArray_SM1[25].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[25].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[26].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[26].kMacros[0].Name = "SI";
    kMacroArray_SM1[26].kMacros[0].Definition = "";
    kMacroArray_SM1[26].kMacros[1].Name = "VC";
    kMacroArray_SM1[26].kMacros[1].Definition = "";
    kMacroArray_SM1[26].kMacros[2].Name = "FOG";
    kMacroArray_SM1[26].kMacros[2].Definition = "";
    kMacroArray_SM1[26].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[26].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1[27].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[27].kMacros[0].Name = "SI";
    kMacroArray_SM1[27].kMacros[0].Definition = "";
    kMacroArray_SM1[27].kMacros[1].Name = "VC";
    kMacroArray_SM1[27].kMacros[1].Definition = "";
    kMacroArray_SM1[27].kMacros[2].Name = "ALPHA";
    kMacroArray_SM1[27].kMacros[2].Definition = "";
    kMacroArray_SM1[27].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[27].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM1[28].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
    kMacroArray_SM1[28].kMacros[0].Name = "SI";
    kMacroArray_SM1[28].kMacros[0].Definition = "";
    kMacroArray_SM1[28].kMacros[1].Name = "VC";
    kMacroArray_SM1[28].kMacros[1].Definition = "";
    kMacroArray_SM1[28].kMacros[2].Name = "ALPHA";
    kMacroArray_SM1[28].kMacros[2].Definition = "";
    kMacroArray_SM1[28].kMacros[3].Name = "FOG";
    kMacroArray_SM1[28].kMacros[3].Definition = "";
    kMacroArray_SM1[28].kMacros[4].Name = nullptr;
    kMacroArray_SM1[28].kMacros[4].Definition = nullptr;
    kMacroArray_SM1[28].kMacros[5].Name = nullptr;
    kMacroArray_SM1[28].kMacros[5].Definition = nullptr;
    kMacroArray_SM1[28].kMacros[6].Name = nullptr;
    kMacroArray_SM1[28].kMacros[6].Definition = nullptr;
    kMacroArray_SM1[28].kMacros[7].Name = nullptr;
    kMacroArray_SM1[28].kMacros[7].Definition = nullptr;
    kMacroArray_SM1[28].kMacros[8].Name = nullptr;
    kMacroArray_SM1[28].kMacros[8].Definition = nullptr;
    kMacroArray_SM1[29].strPath = "lighting\\1x\\p\\ambDiffDirAndPt.p.hlsl";
    kMacroArray_SM1[29].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[29].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[30].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[30].kMacros[0].Name = "FOG";
    kMacroArray_SM1[30].kMacros[0].Definition = "";
    kMacroArray_SM1[30].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[30].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[31].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[31].kMacros[0].Name = "FOG";
    kMacroArray_SM1[31].kMacros[0].Definition = "";
    kMacroArray_SM1[31].kMacros[1].Name = "TEX";
    kMacroArray_SM1[31].kMacros[1].Definition = "";
    kMacroArray_SM1[31].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[31].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[32].strPath = "lighting\\1x\\p\\envmap.p.hlsl";
    kMacroArray_SM1[32].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[32].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[33].strPath = "lighting\\1x\\p\\envmap.p.hlsl";
    kMacroArray_SM1[33].kMacros[0].Name = "VC";
    kMacroArray_SM1[33].kMacros[0].Definition = "";
    kMacroArray_SM1[33].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[33].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[34].strPath = "lighting\\1x\\p\\vertDir.p.hlsl";
    kMacroArray_SM1[34].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[34].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[35].strPath = "lighting\\1x\\p\\vertDirAndPt.p.hlsl";
    kMacroArray_SM1[35].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[35].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[36].strPath = "lighting\\1x\\p\\SI.p.hlsl";
    kMacroArray_SM1[36].kMacros[0].Name = "AMBIENT";
    kMacroArray_SM1[36].kMacros[0].Definition = "";
    kMacroArray_SM1[36].kMacros[1].Name = "EMMITMULT";
    kMacroArray_SM1[36].kMacros[1].Definition = fEmissiveMult;
    kMacroArray_SM1[36].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[36].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[37].strPath = "lighting\\1x\\p\\SI.p.hlsl";
    kMacroArray_SM1[37].kMacros[0].Name = "EMMITMULT";
    kMacroArray_SM1[37].kMacros[0].Definition = fEmissiveMult;
    kMacroArray_SM1[37].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[37].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[38].strPath = "lighting\\1x\\p\\SI.p.hlsl";
    kMacroArray_SM1[38].kMacros[0].Name = "SIBLOCK";
    kMacroArray_SM1[38].kMacros[0].Definition = nullptr;
    kMacroArray_SM1[38].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[38].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[39].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[39].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[39].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[40].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[40].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[40].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM1[41].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[41].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[41].kMacros[0].Definition = "";
    kMacroArray_SM1[41].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[41].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[42].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[42].kMacros[0].Name = "SI";
    kMacroArray_SM1[42].kMacros[0].Definition = "";
    kMacroArray_SM1[42].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[42].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[43].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[43].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[43].kMacros[0].Definition = "";
    kMacroArray_SM1[43].kMacros[1].Name = "SI";
    kMacroArray_SM1[43].kMacros[1].Definition = "";
    kMacroArray_SM1[43].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[43].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[44].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[44].kMacros[0].Name = "PTLIGHT";
    kMacroArray_SM1[44].kMacros[0].Definition = "";
    kMacroArray_SM1[44].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[44].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM1[45].strPath = "lighting\\1x\\p\\land.p.hlsl";
    kMacroArray_SM1[45].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM1[45].kMacros[0].Definition = "";
    kMacroArray_SM1[45].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM1[45].kMacros[1].Definition = "";
    kMacroArray_SM1[45].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[45].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM1[46].strPath = nullptr;
    std::memset(
        &kMacroArray_SM1[46].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM1[47].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM1[47].kMacros[0].Name = "CONSTALPHA";
    kMacroArray_SM1[47].kMacros[0].Definition = "";
    kMacroArray_SM1[47].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM1[47].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[0].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[0].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[0].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[1].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[1].kMacros[0].Name = "OPT";
    kMacroArray_SM2[1].kMacros[0].Definition = "";
    kMacroArray_SM2[1].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[1].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[2].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[2].kMacros[0].Name = "OPT";
    kMacroArray_SM2[2].kMacros[0].Definition = "";
    kMacroArray_SM2[2].kMacros[1].Name = "LOD";
    kMacroArray_SM2[2].kMacros[1].Definition = "";
    kMacroArray_SM2[2].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[2].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[3].strPath = "lighting\\2x\\p\\landlod.p.hlsl";
    kMacroArray_SM2[3].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[3].kMacros[0].Definition = "";
    kMacroArray_SM2[3].kMacros[1].Name = "LODLANDCLIP";
    kMacroArray_SM2[3].kMacros[1].Definition = "";
    kMacroArray_SM2[3].kMacros[2].Name = "LODLANDNOISE";
    kMacroArray_SM2[3].kMacros[2].Definition = "";
    kMacroArray_SM2[3].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[3].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[4].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[4].kMacros[0].Name = "SI";
    kMacroArray_SM2[4].kMacros[0].Definition = "";
    kMacroArray_SM2[4].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[4].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[5].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[5].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[5].kMacros[0].Definition = "";
    kMacroArray_SM2[5].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[5].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[6].strPath = "lighting\\2x\\p\\landlod.p.hlsl";
    kMacroArray_SM2[6].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[6].kMacros[0].Definition = "";
    kMacroArray_SM2[6].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[6].kMacros[1].Definition = "";
    kMacroArray_SM2[6].kMacros[2].Name = "LODLANDCLIP";
    kMacroArray_SM2[6].kMacros[2].Definition = "";
    kMacroArray_SM2[6].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[6].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[7].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[7].kMacros[0].Name = "SI";
    kMacroArray_SM2[7].kMacros[0].Definition = "";
    kMacroArray_SM2[7].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[7].kMacros[1].Definition = "";
    kMacroArray_SM2[7].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[7].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[8].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[8].kMacros[0].Name = "STBB";
    kMacroArray_SM2[8].kMacros[0].Definition = "";
    kMacroArray_SM2[8].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[8].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[9].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[9].kMacros[0].Name = "HAIR";
    kMacroArray_SM2[9].kMacros[0].Definition = "";
    kMacroArray_SM2[9].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[9].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[10].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[10].kMacros[0].Name = "HAIR";
    kMacroArray_SM2[10].kMacros[0].Definition = "";
    kMacroArray_SM2[10].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[10].kMacros[1].Definition = "";
    kMacroArray_SM2[10].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[10].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[11].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[11].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[11].kMacros[0].Definition = "2";
    kMacroArray_SM2[11].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[11].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[12].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[12].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[12].kMacros[0].Definition = "2";
    kMacroArray_SM2[12].kMacros[1].Name = "SI";
    kMacroArray_SM2[12].kMacros[1].Definition = "";
    kMacroArray_SM2[12].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[12].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[13].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[13].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[13].kMacros[0].Definition = "2";
    kMacroArray_SM2[13].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[13].kMacros[1].Definition = "";
    kMacroArray_SM2[13].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[13].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[14].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[14].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[14].kMacros[0].Definition = "2";
    kMacroArray_SM2[14].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[14].kMacros[1].Definition = "";
    kMacroArray_SM2[14].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[14].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[15].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[15].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[15].kMacros[0].Definition = "2";
    kMacroArray_SM2[15].kMacros[1].Name = "SI";
    kMacroArray_SM2[15].kMacros[1].Definition = "";
    kMacroArray_SM2[15].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[15].kMacros[2].Definition = "";
    kMacroArray_SM2[15].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[15].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[16].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[16].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[16].kMacros[0].Definition = "2";
    kMacroArray_SM2[16].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[16].kMacros[1].Definition = "";
    kMacroArray_SM2[16].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[16].kMacros[2].Definition = "";
    kMacroArray_SM2[16].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[16].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[17].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[17].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[17].kMacros[0].Definition = "";
    kMacroArray_SM2[17].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[17].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[18].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[18].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[18].kMacros[0].Definition = "";
    kMacroArray_SM2[18].kMacros[1].Name = "SI";
    kMacroArray_SM2[18].kMacros[1].Definition = "";
    kMacroArray_SM2[18].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[18].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[19].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[19].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[19].kMacros[0].Definition = "";
    kMacroArray_SM2[19].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[19].kMacros[1].Definition = "";
    kMacroArray_SM2[19].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[19].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[20].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[20].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[20].kMacros[0].Definition = "";
    kMacroArray_SM2[20].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[20].kMacros[1].Definition = "";
    kMacroArray_SM2[20].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[20].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[21].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[21].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[21].kMacros[0].Definition = "";
    kMacroArray_SM2[21].kMacros[1].Name = "SI";
    kMacroArray_SM2[21].kMacros[1].Definition = "";
    kMacroArray_SM2[21].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[21].kMacros[2].Definition = "";
    kMacroArray_SM2[21].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[21].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[22].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[22].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[22].kMacros[0].Definition = "";
    kMacroArray_SM2[22].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[22].kMacros[1].Definition = "";
    kMacroArray_SM2[22].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[22].kMacros[2].Definition = "";
    kMacroArray_SM2[22].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[22].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[23].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[23].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[23].kMacros[0].Definition = "";
    kMacroArray_SM2[23].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[23].kMacros[1].Definition = "2";
    kMacroArray_SM2[23].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[23].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[24].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[24].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[24].kMacros[0].Definition = "";
    kMacroArray_SM2[24].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[24].kMacros[1].Definition = "2";
    kMacroArray_SM2[24].kMacros[2].Name = "SI";
    kMacroArray_SM2[24].kMacros[2].Definition = "";
    kMacroArray_SM2[24].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[24].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[25].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[25].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[26].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[26].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[26].kMacros[0].Definition = "";
    kMacroArray_SM2[26].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[26].kMacros[1].Definition = "2";
    kMacroArray_SM2[26].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[26].kMacros[2].Definition = "";
    kMacroArray_SM2[26].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[26].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[27].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray_SM2[27].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[27].kMacros[0].Definition = "";
    kMacroArray_SM2[27].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[27].kMacros[1].Definition = "2";
    kMacroArray_SM2[27].kMacros[2].Name = "SI";
    kMacroArray_SM2[27].kMacros[2].Definition = "";
    kMacroArray_SM2[27].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[27].kMacros[3].Definition = "";
    kMacroArray_SM2[27].kMacros[4].Name = nullptr;
    kMacroArray_SM2[27].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[27].kMacros[5].Name = nullptr;
    kMacroArray_SM2[27].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[27].kMacros[6].Name = nullptr;
    kMacroArray_SM2[27].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[27].kMacros[7].Name = nullptr;
    kMacroArray_SM2[27].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[27].kMacros[8].Name = nullptr;
    kMacroArray_SM2[27].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[28].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[28].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[29].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[29].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[29].kMacros[0].Definition = "9";
    kMacroArray_SM2[29].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[29].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[30].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[30].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[30].kMacros[0].Definition = "9";
    kMacroArray_SM2[30].kMacros[1].Name = "SI";
    kMacroArray_SM2[30].kMacros[1].Definition = nullptr;
    kMacroArray_SM2[30].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[30].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[31].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[31].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[31].kMacros[0].Definition = "4";
    kMacroArray_SM2[31].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[31].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[32].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[32].kMacros[0].Name = "OPT";
    kMacroArray_SM2[32].kMacros[0].Definition = "";
    kMacroArray_SM2[32].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[32].kMacros[1].Definition = "4";
    kMacroArray_SM2[32].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[32].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[33].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[33].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[33].kMacros[0].Definition = "4";
    kMacroArray_SM2[33].kMacros[1].Name = "SI";
    kMacroArray_SM2[33].kMacros[1].Definition = nullptr;
    kMacroArray_SM2[33].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[33].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[34].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[34].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[34].kMacros[0].Definition = "";
    kMacroArray_SM2[34].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[34].kMacros[1].Definition = "4";
    kMacroArray_SM2[34].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[34].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[35].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[35].kMacros[0].Name = "OPT";
    kMacroArray_SM2[35].kMacros[0].Definition = "";
    kMacroArray_SM2[35].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[35].kMacros[1].Definition = "";
    kMacroArray_SM2[35].kMacros[2].Name = "LIGHTS";
    kMacroArray_SM2[35].kMacros[2].Definition = "4";
    kMacroArray_SM2[35].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[35].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[36].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray_SM2[36].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[36].kMacros[0].Definition = "";
    kMacroArray_SM2[36].kMacros[1].Name = "LIGHTS";
    kMacroArray_SM2[36].kMacros[1].Definition = "4";
    kMacroArray_SM2[36].kMacros[2].Name = "SI";
    kMacroArray_SM2[36].kMacros[2].Definition = nullptr;
    kMacroArray_SM2[36].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[36].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[37].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[37].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[37].kMacros[0].Definition = "2";
    kMacroArray_SM2[37].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[37].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[38].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[38].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[38].kMacros[0].Definition = "2";
    kMacroArray_SM2[38].kMacros[1].Name = "SI";
    kMacroArray_SM2[38].kMacros[1].Definition = "";
    kMacroArray_SM2[38].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[38].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[39].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[39].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[39].kMacros[0].Definition = "2";
    kMacroArray_SM2[39].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[39].kMacros[1].Definition = "";
    kMacroArray_SM2[39].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[39].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[40].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[40].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[40].kMacros[0].Definition = "2";
    kMacroArray_SM2[40].kMacros[1].Name = "SI";
    kMacroArray_SM2[40].kMacros[1].Definition = "";
    kMacroArray_SM2[40].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[40].kMacros[2].Definition = "";
    kMacroArray_SM2[40].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[40].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[41].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[41].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[41].kMacros[0].Definition = "3";
    kMacroArray_SM2[41].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[41].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[42].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[42].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[42].kMacros[0].Definition = "3";
    kMacroArray_SM2[42].kMacros[1].Name = "SI";
    kMacroArray_SM2[42].kMacros[1].Definition = "";
    kMacroArray_SM2[42].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[42].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[43].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[43].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[43].kMacros[0].Definition = "3";
    kMacroArray_SM2[43].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[43].kMacros[1].Definition = "";
    kMacroArray_SM2[43].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[43].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[44].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray_SM2[44].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[44].kMacros[0].Definition = "3";
    kMacroArray_SM2[44].kMacros[1].Name = "SI";
    kMacroArray_SM2[44].kMacros[1].Definition = "";
    kMacroArray_SM2[44].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[44].kMacros[2].Definition = "";
    kMacroArray_SM2[44].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[44].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[45].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
    kMacroArray_SM2[45].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[45].kMacros[0].Definition = "2";
    kMacroArray_SM2[45].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[45].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[46].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
    kMacroArray_SM2[46].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[46].kMacros[0].Definition = "3";
    kMacroArray_SM2[46].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[46].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[47].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[47].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[47].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[48].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[48].kMacros[0].Name = "HAIR";
    kMacroArray_SM2[48].kMacros[0].Definition = "";
    kMacroArray_SM2[48].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[48].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[49].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[49].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[49].kMacros[0].Definition = "";
    kMacroArray_SM2[49].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[49].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[50].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[50].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[50].kMacros[0].Definition = "";
    kMacroArray_SM2[50].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[50].kMacros[1].Definition = "";
    kMacroArray_SM2[50].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[50].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[51].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[51].kMacros[0].Name = "POINT";
    kMacroArray_SM2[51].kMacros[0].Definition = "";
    kMacroArray_SM2[51].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[51].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[52].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[52].kMacros[0].Name = "POINT";
    kMacroArray_SM2[52].kMacros[0].Definition = "";
    kMacroArray_SM2[52].kMacros[1].Name = "HAIR";
    kMacroArray_SM2[52].kMacros[1].Definition = "";
    kMacroArray_SM2[52].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[52].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[53].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[53].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[53].kMacros[0].Definition = "2";
    kMacroArray_SM2[53].kMacros[1].Name = "POINT";
    kMacroArray_SM2[53].kMacros[1].Definition = "";
    kMacroArray_SM2[53].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[53].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[54].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[54].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[54].kMacros[0].Definition = "2";
    kMacroArray_SM2[54].kMacros[1].Name = "POINT";
    kMacroArray_SM2[54].kMacros[1].Definition = "";
    kMacroArray_SM2[54].kMacros[2].Name = "HAIR";
    kMacroArray_SM2[54].kMacros[2].Definition = "";
    kMacroArray_SM2[54].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[54].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[55].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[55].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[55].kMacros[0].Definition = "3";
    kMacroArray_SM2[55].kMacros[1].Name = "POINT";
    kMacroArray_SM2[55].kMacros[1].Definition = "";
    kMacroArray_SM2[55].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[55].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[56].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
    kMacroArray_SM2[56].kMacros[0].Name = "NUM_PT_LIGHTS";
    kMacroArray_SM2[56].kMacros[0].Definition = "3";
    kMacroArray_SM2[56].kMacros[1].Name = "POINT";
    kMacroArray_SM2[56].kMacros[1].Definition = "";
    kMacroArray_SM2[56].kMacros[2].Name = "HAIR";
    kMacroArray_SM2[56].kMacros[2].Definition = "";
    kMacroArray_SM2[56].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[56].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[57].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray_SM2[57].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[57].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[58].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray_SM2[58].kMacros[0].Name = "WINDOW";
    kMacroArray_SM2[58].kMacros[0].Definition = "";
    kMacroArray_SM2[58].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[58].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[59].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray_SM2[59].kMacros[0].Name = "EYE";
    kMacroArray_SM2[59].kMacros[0].Definition = "";
    kMacroArray_SM2[59].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[59].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[60].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[60].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[60].kMacros[0].Definition = "";
    kMacroArray_SM2[60].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[60].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[61].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[61].kMacros[0].Name = "PROJ_SHADOW";
    kMacroArray_SM2[61].kMacros[0].Definition = "";
    kMacroArray_SM2[61].kMacros[1].Name = "LANDALPHA";
    kMacroArray_SM2[61].kMacros[1].Definition = "";
    kMacroArray_SM2[61].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[61].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[62].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[62].kMacros[0].Name = "DIFF";
    kMacroArray_SM2[62].kMacros[0].Definition = "";
    kMacroArray_SM2[62].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM2[62].kMacros[1].Definition = "";
    kMacroArray_SM2[62].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[62].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[63].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[63].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[63].kMacros[0].Definition = "";
    kMacroArray_SM2[63].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[63].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[64].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[64].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[65].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[65].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[65].kMacros[0].Definition = "";
    kMacroArray_SM2[65].kMacros[1].Name = "DIFF";
    kMacroArray_SM2[65].kMacros[1].Definition = "";
    kMacroArray_SM2[65].kMacros[2].Name = "PTLIGHT";
    kMacroArray_SM2[65].kMacros[2].Definition = "";
    kMacroArray_SM2[65].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[65].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[66].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[66].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[66].kMacros[0].Definition = "";
    kMacroArray_SM2[66].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[66].kMacros[1].Definition = "";
    kMacroArray_SM2[66].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[66].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[67].strPath = nullptr;
    std::memset(
        &kMacroArray_SM2[67].kMacros[0].Name, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Name)
    );
    kMacroArray_SM2[68].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[68].kMacros[0].Name = "SPECULAR";
    kMacroArray_SM2[68].kMacros[0].Definition = "";
    kMacroArray_SM2[68].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray_SM2[68].kMacros[1].Definition = "";
    kMacroArray_SM2[68].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[68].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[69].strPath = "lighting\\2x\\p\\landlighting.p.hlsl";
    kMacroArray_SM2[69].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[69].kMacros[0].Definition = "";
    kMacroArray_SM2[69].kMacros[1].Name = "SPECULAR";
    kMacroArray_SM2[69].kMacros[1].Definition = "";
    kMacroArray_SM2[69].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray_SM2[69].kMacros[2].Definition = "";
    kMacroArray_SM2[69].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[69].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[70].strPath = "lighting\\2x\\p\\depthmap.p.hlsl";
    kMacroArray_SM2[70].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[70].kMacros[0].Definition = "";
    kMacroArray_SM2[70].kMacros[1].Name = "SHADOWMODE";
    kMacroArray_SM2[70].kMacros[1].Definition = "3";
    kMacroArray_SM2[70].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[70].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[71].strPath = "lighting\\2x\\p\\depthmap.p.hlsl";
    kMacroArray_SM2[71].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[71].kMacros[0].Definition = "";
    kMacroArray_SM2[71].kMacros[1].Name = "SHADOWMODE";
    kMacroArray_SM2[71].kMacros[1].Definition = "3";
    kMacroArray_SM2[71].kMacros[2].Name = "ALPHATEST";
    kMacroArray_SM2[71].kMacros[2].Definition = "";
    kMacroArray_SM2[71].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[71].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[72].strPath = "lighting\\2x\\p\\depthmap.p.hlsl";
    kMacroArray_SM2[72].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[72].kMacros[0].Definition = "";
    kMacroArray_SM2[72].kMacros[1].Name = "SHADOWMODE";
    kMacroArray_SM2[72].kMacros[1].Definition = "3";
    kMacroArray_SM2[72].kMacros[2].Name = "SKIN";
    kMacroArray_SM2[72].kMacros[2].Definition = "";
    kMacroArray_SM2[72].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[72].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[73].strPath = "lighting\\2x\\p\\depthmap.p.hlsl";
    kMacroArray_SM2[73].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[73].kMacros[0].Definition = "";
    kMacroArray_SM2[73].kMacros[1].Name = "SHADOWMODE";
    kMacroArray_SM2[73].kMacros[1].Definition = "3";
    kMacroArray_SM2[73].kMacros[2].Name = "SKIN";
    kMacroArray_SM2[73].kMacros[2].Definition = "";
    kMacroArray_SM2[73].kMacros[3].Name = "ALPHATEST";
    kMacroArray_SM2[73].kMacros[3].Definition = "";
    kMacroArray_SM2[73].kMacros[4].Name = nullptr;
    kMacroArray_SM2[73].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[73].kMacros[5].Name = nullptr;
    kMacroArray_SM2[73].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[73].kMacros[6].Name = nullptr;
    kMacroArray_SM2[73].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[73].kMacros[7].Name = nullptr;
    kMacroArray_SM2[73].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[73].kMacros[8].Name = nullptr;
    kMacroArray_SM2[73].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[74].strPath = "lighting\\2x\\p\\depthmap.p.hlsl";
    kMacroArray_SM2[74].kMacros[0].Name = "DEPTHMAP";
    kMacroArray_SM2[74].kMacros[0].Definition = "";
    kMacroArray_SM2[74].kMacros[1].Name = "WATERDEPTH";
    kMacroArray_SM2[74].kMacros[1].Definition = "";
    kMacroArray_SM2[74].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[74].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[75].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray_SM2[75].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[75].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[76].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray_SM2[76].kMacros[0].Name = "FIRE";
    kMacroArray_SM2[76].kMacros[0].Definition = "";
    kMacroArray_SM2[76].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[76].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[77].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray_SM2[77].kMacros[0].Name = "CLEAR";
    kMacroArray_SM2[77].kMacros[0].Definition = "";
    kMacroArray_SM2[77].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[77].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[78].strPath = "lighting\\2x\\p\\renderDepth.p.hlsl";
    kMacroArray_SM2[78].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[78].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[79].strPath = "lighting\\2x\\p\\renderVelocity.p.hlsl";
    kMacroArray_SM2[79].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[79].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[80].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[80].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[80].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[81].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[81].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[81].kMacros[0].Definition = "";
    kMacroArray_SM2[81].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[81].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[82].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[82].kMacros[0].Name = "LANDLO";
    kMacroArray_SM2[82].kMacros[0].Definition = "";
    kMacroArray_SM2[82].kMacros[1].Name = "FOG";
    kMacroArray_SM2[82].kMacros[1].Definition = "";
    kMacroArray_SM2[82].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[82].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[83].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[83].kMacros[0].Name = "SI";
    kMacroArray_SM2[83].kMacros[0].Definition = "";
    kMacroArray_SM2[83].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[83].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[84].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[84].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[84].kMacros[0].Definition = "";
    kMacroArray_SM2[84].kMacros[1].Name = "SI";
    kMacroArray_SM2[84].kMacros[1].Definition = "";
    kMacroArray_SM2[84].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[84].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[85].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[85].kMacros[0].Name = "PTLIGHT";
    kMacroArray_SM2[85].kMacros[0].Definition = "";
    kMacroArray_SM2[85].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[85].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[86].strPath = "lighting\\2x\\p\\land.p.hlsl";
    kMacroArray_SM2[86].kMacros[0].Name = "LANDALPHA";
    kMacroArray_SM2[86].kMacros[0].Definition = "";
    kMacroArray_SM2[86].kMacros[1].Name = "PTLIGHT";
    kMacroArray_SM2[86].kMacros[1].Definition = "";
    kMacroArray_SM2[86].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[86].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[87].strPath = "lighting\\2x\\p\\localMap.p.hlsl";
    kMacroArray_SM2[87].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[87].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[88].strPath = "lighting\\2x\\p\\localMap.p.hlsl";
    kMacroArray_SM2[88].kMacros[0].Name = "CLEAR";
    kMacroArray_SM2[88].kMacros[0].Definition = "";
    kMacroArray_SM2[88].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[88].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[89].strPath = "lighting\\1x\\p\\texEffect.p.hlsl";
    kMacroArray_SM2[89].kMacros[0].Name = "HQ";
    kMacroArray_SM2[89].kMacros[0].Definition = "";
    kMacroArray_SM2[89].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[89].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[90].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray_SM2[90].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[90].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray_SM2[91].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray_SM2[91].kMacros[0].Name = "ALPHATESTING";
    kMacroArray_SM2[91].kMacros[0].Definition = "";
    kMacroArray_SM2[91].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[91].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[92].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[92].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[92].kMacros[0].Definition = "1";
    kMacroArray_SM2[92].kMacros[1].Name = "FOG";
    kMacroArray_SM2[92].kMacros[1].Definition = "";
    kMacroArray_SM2[92].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[92].kMacros[2].Definition = "";
    kMacroArray_SM2[92].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[92].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[93].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[93].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[93].kMacros[0].Definition = "1";
    kMacroArray_SM2[93].kMacros[1].Name = "FOG";
    kMacroArray_SM2[93].kMacros[1].Definition = "";
    kMacroArray_SM2[93].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[93].kMacros[2].Definition = "";
    kMacroArray_SM2[93].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[93].kMacros[3].Definition = "";
    kMacroArray_SM2[93].kMacros[4].Name = nullptr;
    kMacroArray_SM2[93].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[93].kMacros[5].Name = nullptr;
    kMacroArray_SM2[93].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[93].kMacros[6].Name = nullptr;
    kMacroArray_SM2[93].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[93].kMacros[7].Name = nullptr;
    kMacroArray_SM2[93].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[93].kMacros[8].Name = nullptr;
    kMacroArray_SM2[93].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[94].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[94].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[94].kMacros[0].Definition = "1";
    kMacroArray_SM2[94].kMacros[1].Name = "FOG";
    kMacroArray_SM2[94].kMacros[1].Definition = "";
    kMacroArray_SM2[94].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[94].kMacros[2].Definition = "";
    kMacroArray_SM2[94].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[94].kMacros[3].Definition = "";
    kMacroArray_SM2[94].kMacros[4].Name = nullptr;
    kMacroArray_SM2[94].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[94].kMacros[5].Name = nullptr;
    kMacroArray_SM2[94].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[94].kMacros[6].Name = nullptr;
    kMacroArray_SM2[94].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[94].kMacros[7].Name = nullptr;
    kMacroArray_SM2[94].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[94].kMacros[8].Name = nullptr;
    kMacroArray_SM2[94].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[95].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[95].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[95].kMacros[0].Definition = "1";
    kMacroArray_SM2[95].kMacros[1].Name = "FOG";
    kMacroArray_SM2[95].kMacros[1].Definition = "";
    kMacroArray_SM2[95].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[95].kMacros[2].Definition = "";
    kMacroArray_SM2[95].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[95].kMacros[3].Definition = "";
    kMacroArray_SM2[95].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[95].kMacros[4].Definition = "";
    kMacroArray_SM2[95].kMacros[5].Name = nullptr;
    kMacroArray_SM2[95].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[95].kMacros[6].Name = nullptr;
    kMacroArray_SM2[95].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[95].kMacros[7].Name = nullptr;
    kMacroArray_SM2[95].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[95].kMacros[8].Name = nullptr;
    kMacroArray_SM2[95].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[96].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[96].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[96].kMacros[0].Definition = "1";
    kMacroArray_SM2[96].kMacros[1].Name = "FOG";
    kMacroArray_SM2[96].kMacros[1].Definition = "";
    kMacroArray_SM2[96].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[96].kMacros[2].Definition = "";
    kMacroArray_SM2[96].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[96].kMacros[3].Definition = "";
    kMacroArray_SM2[96].kMacros[4].Name = nullptr;
    kMacroArray_SM2[96].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[96].kMacros[5].Name = nullptr;
    kMacroArray_SM2[96].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[96].kMacros[6].Name = nullptr;
    kMacroArray_SM2[96].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[96].kMacros[7].Name = nullptr;
    kMacroArray_SM2[96].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[96].kMacros[8].Name = nullptr;
    kMacroArray_SM2[96].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[97].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[97].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[97].kMacros[0].Definition = "1";
    kMacroArray_SM2[97].kMacros[1].Name = "FOG";
    kMacroArray_SM2[97].kMacros[1].Definition = "";
    kMacroArray_SM2[97].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[97].kMacros[2].Definition = "";
    kMacroArray_SM2[97].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[97].kMacros[3].Definition = "";
    kMacroArray_SM2[97].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[97].kMacros[4].Definition = "";
    kMacroArray_SM2[97].kMacros[5].Name = nullptr;
    kMacroArray_SM2[97].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[97].kMacros[6].Name = nullptr;
    kMacroArray_SM2[97].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[97].kMacros[7].Name = nullptr;
    kMacroArray_SM2[97].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[97].kMacros[8].Name = nullptr;
    kMacroArray_SM2[97].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[98].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[98].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[98].kMacros[0].Definition = "1";
    kMacroArray_SM2[98].kMacros[1].Name = "FOG";
    kMacroArray_SM2[98].kMacros[1].Definition = "";
    kMacroArray_SM2[98].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[98].kMacros[2].Definition = "";
    kMacroArray_SM2[98].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[98].kMacros[3].Definition = "";
    kMacroArray_SM2[98].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[98].kMacros[4].Definition = "";
    kMacroArray_SM2[98].kMacros[5].Name = nullptr;
    kMacroArray_SM2[98].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[98].kMacros[6].Name = nullptr;
    kMacroArray_SM2[98].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[98].kMacros[7].Name = nullptr;
    kMacroArray_SM2[98].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[98].kMacros[8].Name = nullptr;
    kMacroArray_SM2[98].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[99].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[99].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[100].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[99].kMacros[0].Definition = "1";
    kMacroArray_SM2[99].kMacros[1].Name = "FOG";
    kMacroArray_SM2[99].kMacros[1].Definition = "";
    kMacroArray_SM2[99].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[99].kMacros[2].Definition = "";
    kMacroArray_SM2[99].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[99].kMacros[3].Definition = "";
    kMacroArray_SM2[99].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[99].kMacros[4].Definition = "";
    kMacroArray_SM2[99].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[99].kMacros[5].Definition = "";
    kMacroArray_SM2[99].kMacros[6].Name = nullptr;
    kMacroArray_SM2[99].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[99].kMacros[7].Name = nullptr;
    kMacroArray_SM2[99].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[99].kMacros[8].Name = nullptr;
    kMacroArray_SM2[99].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[100].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[100].kMacros[0].Definition = "2";
    kMacroArray_SM2[100].kMacros[1].Name = "FOG";
    kMacroArray_SM2[100].kMacros[1].Definition = "";
    kMacroArray_SM2[100].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[100].kMacros[2].Definition = "";
    kMacroArray_SM2[100].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[100].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[101].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[101].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[101].kMacros[0].Definition = "2";
    kMacroArray_SM2[101].kMacros[1].Name = "FOG";
    kMacroArray_SM2[101].kMacros[1].Definition = "";
    kMacroArray_SM2[101].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[101].kMacros[2].Definition = "";
    kMacroArray_SM2[101].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[101].kMacros[3].Definition = "";
    kMacroArray_SM2[101].kMacros[4].Name = nullptr;
    kMacroArray_SM2[101].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[101].kMacros[5].Name = nullptr;
    kMacroArray_SM2[101].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[101].kMacros[6].Name = nullptr;
    kMacroArray_SM2[101].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[101].kMacros[7].Name = nullptr;
    kMacroArray_SM2[101].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[101].kMacros[8].Name = nullptr;
    kMacroArray_SM2[101].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[102].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[102].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[102].kMacros[0].Definition = "2";
    kMacroArray_SM2[102].kMacros[1].Name = "FOG";
    kMacroArray_SM2[102].kMacros[1].Definition = "";
    kMacroArray_SM2[102].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[102].kMacros[2].Definition = "";
    kMacroArray_SM2[102].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[102].kMacros[3].Definition = "";
    kMacroArray_SM2[102].kMacros[4].Name = nullptr;
    kMacroArray_SM2[102].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[102].kMacros[5].Name = nullptr;
    kMacroArray_SM2[102].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[102].kMacros[6].Name = nullptr;
    kMacroArray_SM2[102].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[102].kMacros[7].Name = nullptr;
    kMacroArray_SM2[102].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[102].kMacros[8].Name = nullptr;
    kMacroArray_SM2[102].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[103].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[103].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[103].kMacros[0].Definition = "2";
    kMacroArray_SM2[103].kMacros[1].Name = "FOG";
    kMacroArray_SM2[103].kMacros[1].Definition = "";
    kMacroArray_SM2[103].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[103].kMacros[2].Definition = "";
    kMacroArray_SM2[103].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[103].kMacros[3].Definition = "";
    kMacroArray_SM2[103].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[103].kMacros[4].Definition = "";
    kMacroArray_SM2[103].kMacros[5].Name = nullptr;
    kMacroArray_SM2[103].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[103].kMacros[6].Name = nullptr;
    kMacroArray_SM2[103].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[103].kMacros[7].Name = nullptr;
    kMacroArray_SM2[103].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[103].kMacros[8].Name = nullptr;
    kMacroArray_SM2[103].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[104].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[104].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[104].kMacros[0].Definition = "2";
    kMacroArray_SM2[104].kMacros[1].Name = "FOG";
    kMacroArray_SM2[104].kMacros[1].Definition = "";
    kMacroArray_SM2[104].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[104].kMacros[2].Definition = "";
    kMacroArray_SM2[104].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[104].kMacros[3].Definition = "";
    kMacroArray_SM2[104].kMacros[4].Name = nullptr;
    kMacroArray_SM2[104].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[104].kMacros[5].Name = nullptr;
    kMacroArray_SM2[104].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[104].kMacros[6].Name = nullptr;
    kMacroArray_SM2[104].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[104].kMacros[7].Name = nullptr;
    kMacroArray_SM2[104].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[104].kMacros[8].Name = nullptr;
    kMacroArray_SM2[104].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[105].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[105].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[105].kMacros[0].Definition = "2";
    kMacroArray_SM2[105].kMacros[1].Name = "FOG";
    kMacroArray_SM2[105].kMacros[1].Definition = "";
    kMacroArray_SM2[105].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[105].kMacros[2].Definition = "";
    kMacroArray_SM2[105].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[105].kMacros[3].Definition = "";
    kMacroArray_SM2[105].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[105].kMacros[4].Definition = "";
    kMacroArray_SM2[105].kMacros[5].Name = nullptr;
    kMacroArray_SM2[105].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[105].kMacros[6].Name = nullptr;
    kMacroArray_SM2[105].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[105].kMacros[7].Name = nullptr;
    kMacroArray_SM2[105].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[105].kMacros[8].Name = nullptr;
    kMacroArray_SM2[105].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[106].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[106].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[106].kMacros[0].Definition = "2";
    kMacroArray_SM2[106].kMacros[1].Name = "FOG";
    kMacroArray_SM2[106].kMacros[1].Definition = "";
    kMacroArray_SM2[106].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[106].kMacros[2].Definition = "";
    kMacroArray_SM2[106].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[106].kMacros[3].Definition = "";
    kMacroArray_SM2[106].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[106].kMacros[4].Definition = "";
    kMacroArray_SM2[106].kMacros[5].Name = nullptr;
    kMacroArray_SM2[106].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[106].kMacros[6].Name = nullptr;
    kMacroArray_SM2[106].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[106].kMacros[7].Name = nullptr;
    kMacroArray_SM2[106].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[106].kMacros[8].Name = nullptr;
    kMacroArray_SM2[106].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[107].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[107].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[107].kMacros[0].Definition = "2";
    kMacroArray_SM2[107].kMacros[1].Name = "FOG";
    kMacroArray_SM2[107].kMacros[1].Definition = "";
    kMacroArray_SM2[107].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[107].kMacros[2].Definition = "";
    kMacroArray_SM2[107].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[107].kMacros[3].Definition = "";
    kMacroArray_SM2[107].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[107].kMacros[4].Definition = "";
    kMacroArray_SM2[107].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[107].kMacros[5].Definition = "";
    kMacroArray_SM2[107].kMacros[6].Name = nullptr;
    kMacroArray_SM2[108].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[107].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[107].kMacros[7].Name = nullptr;
    kMacroArray_SM2[107].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[107].kMacros[8].Name = nullptr;
    kMacroArray_SM2[107].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[108].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[108].kMacros[0].Definition = "3";
    kMacroArray_SM2[108].kMacros[1].Name = "FOG";
    kMacroArray_SM2[108].kMacros[1].Definition = "";
    kMacroArray_SM2[108].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[108].kMacros[2].Definition = "";
    kMacroArray_SM2[108].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[108].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[109].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[109].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[109].kMacros[0].Definition = "3";
    kMacroArray_SM2[109].kMacros[1].Name = "FOG";
    kMacroArray_SM2[109].kMacros[1].Definition = "";
    kMacroArray_SM2[109].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[109].kMacros[2].Definition = "";
    kMacroArray_SM2[109].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[109].kMacros[3].Definition = "";
    kMacroArray_SM2[109].kMacros[4].Name = nullptr;
    kMacroArray_SM2[109].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[109].kMacros[5].Name = nullptr;
    kMacroArray_SM2[109].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[109].kMacros[6].Name = nullptr;
    kMacroArray_SM2[109].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[109].kMacros[7].Name = nullptr;
    kMacroArray_SM2[109].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[109].kMacros[8].Name = nullptr;
    kMacroArray_SM2[109].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[110].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[110].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[110].kMacros[0].Definition = "3";
    kMacroArray_SM2[110].kMacros[1].Name = "FOG";
    kMacroArray_SM2[110].kMacros[1].Definition = "";
    kMacroArray_SM2[110].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[110].kMacros[2].Definition = "";
    kMacroArray_SM2[110].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[110].kMacros[3].Definition = "";
    kMacroArray_SM2[110].kMacros[4].Name = nullptr;
    kMacroArray_SM2[110].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[110].kMacros[5].Name = nullptr;
    kMacroArray_SM2[110].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[110].kMacros[6].Name = nullptr;
    kMacroArray_SM2[110].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[110].kMacros[7].Name = nullptr;
    kMacroArray_SM2[110].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[110].kMacros[8].Name = nullptr;
    kMacroArray_SM2[110].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[111].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[111].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[111].kMacros[0].Definition = "3";
    kMacroArray_SM2[111].kMacros[1].Name = "FOG";
    kMacroArray_SM2[111].kMacros[1].Definition = "";
    kMacroArray_SM2[111].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[111].kMacros[2].Definition = "";
    kMacroArray_SM2[111].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[111].kMacros[3].Definition = "";
    kMacroArray_SM2[111].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[111].kMacros[4].Definition = "";
    kMacroArray_SM2[111].kMacros[5].Name = nullptr;
    kMacroArray_SM2[111].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[111].kMacros[6].Name = nullptr;
    kMacroArray_SM2[111].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[111].kMacros[7].Name = nullptr;
    kMacroArray_SM2[111].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[111].kMacros[8].Name = nullptr;
    kMacroArray_SM2[111].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[112].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[112].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[112].kMacros[0].Definition = "3";
    kMacroArray_SM2[112].kMacros[1].Name = "FOG";
    kMacroArray_SM2[112].kMacros[1].Definition = "";
    kMacroArray_SM2[112].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[112].kMacros[2].Definition = "";
    kMacroArray_SM2[112].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[112].kMacros[3].Definition = "";
    kMacroArray_SM2[112].kMacros[4].Name = nullptr;
    kMacroArray_SM2[112].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[112].kMacros[5].Name = nullptr;
    kMacroArray_SM2[112].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[112].kMacros[6].Name = nullptr;
    kMacroArray_SM2[112].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[112].kMacros[7].Name = nullptr;
    kMacroArray_SM2[112].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[112].kMacros[8].Name = nullptr;
    kMacroArray_SM2[112].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[113].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[113].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[113].kMacros[0].Definition = "3";
    kMacroArray_SM2[113].kMacros[1].Name = "FOG";
    kMacroArray_SM2[113].kMacros[1].Definition = "";
    kMacroArray_SM2[113].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[113].kMacros[2].Definition = "";
    kMacroArray_SM2[113].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[113].kMacros[3].Definition = "";
    kMacroArray_SM2[113].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[113].kMacros[4].Definition = "";
    kMacroArray_SM2[113].kMacros[5].Name = nullptr;
    kMacroArray_SM2[113].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[113].kMacros[6].Name = nullptr;
    kMacroArray_SM2[113].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[113].kMacros[7].Name = nullptr;
    kMacroArray_SM2[113].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[113].kMacros[8].Name = nullptr;
    kMacroArray_SM2[113].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[114].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[114].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[114].kMacros[0].Definition = "3";
    kMacroArray_SM2[114].kMacros[1].Name = "FOG";
    kMacroArray_SM2[114].kMacros[1].Definition = "";
    kMacroArray_SM2[114].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[114].kMacros[2].Definition = "";
    kMacroArray_SM2[114].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[114].kMacros[3].Definition = "";
    kMacroArray_SM2[114].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[114].kMacros[4].Definition = "";
    kMacroArray_SM2[114].kMacros[5].Name = nullptr;
    kMacroArray_SM2[114].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[114].kMacros[6].Name = nullptr;
    kMacroArray_SM2[114].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[114].kMacros[7].Name = nullptr;
    kMacroArray_SM2[114].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[114].kMacros[8].Name = nullptr;
    kMacroArray_SM2[114].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[115].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[115].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[115].kMacros[0].Definition = "3";
    kMacroArray_SM2[115].kMacros[1].Name = "FOG";
    kMacroArray_SM2[115].kMacros[1].Definition = "";
    kMacroArray_SM2[115].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[115].kMacros[2].Definition = "";
    kMacroArray_SM2[115].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[115].kMacros[3].Definition = "";
    kMacroArray_SM2[115].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[115].kMacros[4].Definition = "";
    kMacroArray_SM2[115].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[115].kMacros[5].Definition = "";
    kMacroArray_SM2[115].kMacros[6].Name = nullptr;
    kMacroArray_SM2[115].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[115].kMacros[7].Name = nullptr;
    kMacroArray_SM2[115].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[115].kMacros[8].Name = nullptr;
    kMacroArray_SM2[115].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[116].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[116].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[116].kMacros[0].Definition = "4";
    kMacroArray_SM2[116].kMacros[1].Name = "FOG";
    kMacroArray_SM2[116].kMacros[1].Definition = "";
    kMacroArray_SM2[116].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[116].kMacros[2].Definition = "";
    kMacroArray_SM2[116].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[116].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[117].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[117].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[117].kMacros[0].Definition = "4";
    kMacroArray_SM2[117].kMacros[1].Name = "FOG";
    kMacroArray_SM2[117].kMacros[1].Definition = "";
    kMacroArray_SM2[117].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[117].kMacros[2].Definition = "";
    kMacroArray_SM2[117].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[117].kMacros[3].Definition = "";
    kMacroArray_SM2[117].kMacros[4].Name = nullptr;
    kMacroArray_SM2[117].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[117].kMacros[5].Name = nullptr;
    kMacroArray_SM2[117].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[117].kMacros[6].Name = nullptr;
    kMacroArray_SM2[117].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[117].kMacros[7].Name = nullptr;
    kMacroArray_SM2[117].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[117].kMacros[8].Name = nullptr;
    kMacroArray_SM2[117].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[118].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[118].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[118].kMacros[0].Definition = "4";
    kMacroArray_SM2[118].kMacros[1].Name = "FOG";
    kMacroArray_SM2[118].kMacros[1].Definition = "";
    kMacroArray_SM2[118].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[118].kMacros[2].Definition = "";
    kMacroArray_SM2[118].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[118].kMacros[3].Definition = "";
    kMacroArray_SM2[118].kMacros[4].Name = nullptr;
    kMacroArray_SM2[118].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[118].kMacros[5].Name = nullptr;
    kMacroArray_SM2[118].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[118].kMacros[6].Name = nullptr;
    kMacroArray_SM2[118].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[118].kMacros[7].Name = nullptr;
    kMacroArray_SM2[118].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[118].kMacros[8].Name = nullptr;
    kMacroArray_SM2[118].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[119].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[119].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[119].kMacros[0].Definition = "4";
    kMacroArray_SM2[119].kMacros[1].Name = "FOG";
    kMacroArray_SM2[119].kMacros[1].Definition = "";
    kMacroArray_SM2[119].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[119].kMacros[2].Definition = "";
    kMacroArray_SM2[119].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[119].kMacros[3].Definition = "";
    kMacroArray_SM2[119].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[119].kMacros[4].Definition = "";
    kMacroArray_SM2[119].kMacros[5].Name = nullptr;
    kMacroArray_SM2[119].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[119].kMacros[6].Name = nullptr;
    kMacroArray_SM2[119].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[119].kMacros[7].Name = nullptr;
    kMacroArray_SM2[119].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[119].kMacros[8].Name = nullptr;
    kMacroArray_SM2[119].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[120].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[120].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[120].kMacros[0].Definition = "4";
    kMacroArray_SM2[120].kMacros[1].Name = "FOG";
    kMacroArray_SM2[120].kMacros[1].Definition = "";
    kMacroArray_SM2[120].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[120].kMacros[2].Definition = "";
    kMacroArray_SM2[120].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[120].kMacros[3].Definition = "";
    kMacroArray_SM2[120].kMacros[4].Name = nullptr;
    kMacroArray_SM2[120].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[120].kMacros[5].Name = nullptr;
    kMacroArray_SM2[120].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[120].kMacros[6].Name = nullptr;
    kMacroArray_SM2[120].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[120].kMacros[7].Name = nullptr;
    kMacroArray_SM2[121].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[122].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[123].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[124].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[120].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[120].kMacros[8].Name = nullptr;
    kMacroArray_SM2[120].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[121].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[121].kMacros[0].Definition = "4";
    kMacroArray_SM2[121].kMacros[1].Name = "FOG";
    kMacroArray_SM2[121].kMacros[1].Definition = "";
    kMacroArray_SM2[121].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[121].kMacros[2].Definition = "";
    kMacroArray_SM2[121].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[121].kMacros[3].Definition = "";
    kMacroArray_SM2[121].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[121].kMacros[4].Definition = "";
    kMacroArray_SM2[121].kMacros[5].Name = nullptr;
    kMacroArray_SM2[121].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[121].kMacros[6].Name = nullptr;
    kMacroArray_SM2[121].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[121].kMacros[7].Name = nullptr;
    kMacroArray_SM2[121].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[121].kMacros[8].Name = nullptr;
    kMacroArray_SM2[121].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[122].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[122].kMacros[0].Definition = "4";
    kMacroArray_SM2[122].kMacros[1].Name = "FOG";
    kMacroArray_SM2[122].kMacros[1].Definition = "";
    kMacroArray_SM2[122].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[122].kMacros[2].Definition = "";
    kMacroArray_SM2[122].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[122].kMacros[3].Definition = "";
    kMacroArray_SM2[122].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[122].kMacros[4].Definition = "";
    kMacroArray_SM2[122].kMacros[5].Name = nullptr;
    kMacroArray_SM2[122].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[122].kMacros[6].Name = nullptr;
    kMacroArray_SM2[122].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[122].kMacros[7].Name = nullptr;
    kMacroArray_SM2[122].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[122].kMacros[8].Name = nullptr;
    kMacroArray_SM2[122].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[123].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[123].kMacros[0].Definition = "4";
    kMacroArray_SM2[123].kMacros[1].Name = "FOG";
    kMacroArray_SM2[123].kMacros[1].Definition = "";
    kMacroArray_SM2[123].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[123].kMacros[2].Definition = "";
    kMacroArray_SM2[123].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[123].kMacros[3].Definition = "";
    kMacroArray_SM2[123].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[123].kMacros[4].Definition = "";
    kMacroArray_SM2[123].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[123].kMacros[5].Definition = "";
    kMacroArray_SM2[123].kMacros[6].Name = nullptr;
    kMacroArray_SM2[123].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[123].kMacros[7].Name = nullptr;
    kMacroArray_SM2[123].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[123].kMacros[8].Name = nullptr;
    kMacroArray_SM2[123].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[124].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[124].kMacros[0].Definition = "5";
    kMacroArray_SM2[124].kMacros[1].Name = "FOG";
    kMacroArray_SM2[124].kMacros[1].Definition = "";
    kMacroArray_SM2[124].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[124].kMacros[2].Definition = "";
    kMacroArray_SM2[124].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[124].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[125].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[125].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[125].kMacros[0].Definition = "5";
    kMacroArray_SM2[125].kMacros[1].Name = "FOG";
    kMacroArray_SM2[125].kMacros[1].Definition = "";
    kMacroArray_SM2[125].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[125].kMacros[2].Definition = "";
    kMacroArray_SM2[125].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[125].kMacros[3].Definition = "";
    kMacroArray_SM2[125].kMacros[4].Name = nullptr;
    kMacroArray_SM2[125].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[125].kMacros[5].Name = nullptr;
    kMacroArray_SM2[125].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[125].kMacros[6].Name = nullptr;
    kMacroArray_SM2[125].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[125].kMacros[7].Name = nullptr;
    kMacroArray_SM2[125].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[125].kMacros[8].Name = nullptr;
    kMacroArray_SM2[125].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[126].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[126].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[126].kMacros[0].Definition = "5";
    kMacroArray_SM2[126].kMacros[1].Name = "FOG";
    kMacroArray_SM2[126].kMacros[1].Definition = "";
    kMacroArray_SM2[126].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[126].kMacros[2].Definition = "";
    kMacroArray_SM2[126].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[126].kMacros[3].Definition = "";
    kMacroArray_SM2[126].kMacros[4].Name = nullptr;
    kMacroArray_SM2[126].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[126].kMacros[5].Name = nullptr;
    kMacroArray_SM2[126].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[126].kMacros[6].Name = nullptr;
    kMacroArray_SM2[126].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[126].kMacros[7].Name = nullptr;
    kMacroArray_SM2[126].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[126].kMacros[8].Name = nullptr;
    kMacroArray_SM2[126].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[127].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[127].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[127].kMacros[0].Definition = "5";
    kMacroArray_SM2[127].kMacros[1].Name = "FOG";
    kMacroArray_SM2[127].kMacros[1].Definition = "";
    kMacroArray_SM2[127].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[127].kMacros[2].Definition = "";
    kMacroArray_SM2[127].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[127].kMacros[3].Definition = "";
    kMacroArray_SM2[127].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[127].kMacros[4].Definition = "";
    kMacroArray_SM2[127].kMacros[5].Name = nullptr;
    kMacroArray_SM2[127].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[127].kMacros[6].Name = nullptr;
    kMacroArray_SM2[127].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[127].kMacros[7].Name = nullptr;
    kMacroArray_SM2[127].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[127].kMacros[8].Name = nullptr;
    kMacroArray_SM2[127].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[128].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[128].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[128].kMacros[0].Definition = "5";
    kMacroArray_SM2[128].kMacros[1].Name = "FOG";
    kMacroArray_SM2[128].kMacros[1].Definition = "";
    kMacroArray_SM2[128].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[128].kMacros[2].Definition = "";
    kMacroArray_SM2[128].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[128].kMacros[3].Definition = "";
    kMacroArray_SM2[128].kMacros[4].Name = nullptr;
    kMacroArray_SM2[128].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[128].kMacros[5].Name = nullptr;
    kMacroArray_SM2[128].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[128].kMacros[6].Name = nullptr;
    kMacroArray_SM2[128].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[128].kMacros[7].Name = nullptr;
    kMacroArray_SM2[128].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[128].kMacros[8].Name = nullptr;
    kMacroArray_SM2[128].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[129].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[129].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[129].kMacros[0].Definition = "5";
    kMacroArray_SM2[129].kMacros[1].Name = "FOG";
    kMacroArray_SM2[129].kMacros[1].Definition = "";
    kMacroArray_SM2[129].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[129].kMacros[2].Definition = "";
    kMacroArray_SM2[129].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[129].kMacros[3].Definition = "";
    kMacroArray_SM2[130].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[131].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[132].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[129].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[129].kMacros[4].Definition = "";
    kMacroArray_SM2[129].kMacros[5].Name = nullptr;
    kMacroArray_SM2[129].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[129].kMacros[6].Name = nullptr;
    kMacroArray_SM2[129].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[129].kMacros[7].Name = nullptr;
    kMacroArray_SM2[129].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[129].kMacros[8].Name = nullptr;
    kMacroArray_SM2[129].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[130].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[130].kMacros[0].Definition = "5";
    kMacroArray_SM2[130].kMacros[1].Name = "FOG";
    kMacroArray_SM2[130].kMacros[1].Definition = "";
    kMacroArray_SM2[130].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[130].kMacros[2].Definition = "";
    kMacroArray_SM2[130].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[130].kMacros[3].Definition = "";
    kMacroArray_SM2[130].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[130].kMacros[4].Definition = "";
    kMacroArray_SM2[130].kMacros[5].Name = nullptr;
    kMacroArray_SM2[130].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[130].kMacros[6].Name = nullptr;
    kMacroArray_SM2[130].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[130].kMacros[7].Name = nullptr;
    kMacroArray_SM2[130].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[130].kMacros[8].Name = nullptr;
    kMacroArray_SM2[130].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[131].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[131].kMacros[0].Definition = "5";
    kMacroArray_SM2[131].kMacros[1].Name = "FOG";
    kMacroArray_SM2[131].kMacros[1].Definition = "";
    kMacroArray_SM2[131].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[131].kMacros[2].Definition = "";
    kMacroArray_SM2[131].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[131].kMacros[3].Definition = "";
    kMacroArray_SM2[131].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[131].kMacros[4].Definition = "";
    kMacroArray_SM2[131].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[131].kMacros[5].Definition = "";
    kMacroArray_SM2[131].kMacros[6].Name = nullptr;
    kMacroArray_SM2[131].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[131].kMacros[7].Name = nullptr;
    kMacroArray_SM2[131].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[131].kMacros[8].Name = nullptr;
    kMacroArray_SM2[131].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[132].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[132].kMacros[0].Definition = "6";
    kMacroArray_SM2[132].kMacros[1].Name = "FOG";
    kMacroArray_SM2[132].kMacros[1].Definition = "";
    kMacroArray_SM2[132].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[132].kMacros[2].Definition = "";
    kMacroArray_SM2[132].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[132].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[133].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[133].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[133].kMacros[0].Definition = "6";
    kMacroArray_SM2[133].kMacros[1].Name = "FOG";
    kMacroArray_SM2[133].kMacros[1].Definition = "";
    kMacroArray_SM2[133].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[133].kMacros[2].Definition = "";
    kMacroArray_SM2[133].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[133].kMacros[3].Definition = "";
    kMacroArray_SM2[133].kMacros[4].Name = nullptr;
    kMacroArray_SM2[133].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[133].kMacros[5].Name = nullptr;
    kMacroArray_SM2[133].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[133].kMacros[6].Name = nullptr;
    kMacroArray_SM2[133].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[133].kMacros[7].Name = nullptr;
    kMacroArray_SM2[133].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[133].kMacros[8].Name = nullptr;
    kMacroArray_SM2[133].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[134].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[134].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[134].kMacros[0].Definition = "6";
    kMacroArray_SM2[134].kMacros[1].Name = "FOG";
    kMacroArray_SM2[134].kMacros[1].Definition = "";
    kMacroArray_SM2[134].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[134].kMacros[2].Definition = "";
    kMacroArray_SM2[134].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[134].kMacros[3].Definition = "";
    kMacroArray_SM2[134].kMacros[4].Name = nullptr;
    kMacroArray_SM2[134].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[134].kMacros[5].Name = nullptr;
    kMacroArray_SM2[134].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[134].kMacros[6].Name = nullptr;
    kMacroArray_SM2[134].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[134].kMacros[7].Name = nullptr;
    kMacroArray_SM2[134].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[134].kMacros[8].Name = nullptr;
    kMacroArray_SM2[134].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[135].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[135].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[135].kMacros[0].Definition = "6";
    kMacroArray_SM2[135].kMacros[1].Name = "FOG";
    kMacroArray_SM2[135].kMacros[1].Definition = "";
    kMacroArray_SM2[135].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[135].kMacros[2].Definition = "";
    kMacroArray_SM2[135].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[135].kMacros[3].Definition = "";
    kMacroArray_SM2[135].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[135].kMacros[4].Definition = "";
    kMacroArray_SM2[135].kMacros[5].Name = nullptr;
    kMacroArray_SM2[135].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[135].kMacros[6].Name = nullptr;
    kMacroArray_SM2[135].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[135].kMacros[7].Name = nullptr;
    kMacroArray_SM2[135].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[135].kMacros[8].Name = nullptr;
    kMacroArray_SM2[135].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[136].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[136].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[136].kMacros[0].Definition = "6";
    kMacroArray_SM2[136].kMacros[1].Name = "FOG";
    kMacroArray_SM2[136].kMacros[1].Definition = "";
    kMacroArray_SM2[136].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[136].kMacros[2].Definition = "";
    kMacroArray_SM2[136].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[136].kMacros[3].Definition = "";
    kMacroArray_SM2[136].kMacros[4].Name = nullptr;
    kMacroArray_SM2[136].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[136].kMacros[5].Name = nullptr;
    kMacroArray_SM2[136].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[136].kMacros[6].Name = nullptr;
    kMacroArray_SM2[136].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[136].kMacros[7].Name = nullptr;
    kMacroArray_SM2[136].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[136].kMacros[8].Name = nullptr;
    kMacroArray_SM2[136].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[137].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[137].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[137].kMacros[0].Definition = "6";
    kMacroArray_SM2[137].kMacros[1].Name = "FOG";
    kMacroArray_SM2[137].kMacros[1].Definition = "";
    kMacroArray_SM2[137].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[137].kMacros[2].Definition = "";
    kMacroArray_SM2[137].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[137].kMacros[3].Definition = "";
    kMacroArray_SM2[137].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[137].kMacros[4].Definition = "";
    kMacroArray_SM2[137].kMacros[5].Name = nullptr;
    kMacroArray_SM2[137].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[137].kMacros[6].Name = nullptr;
    kMacroArray_SM2[137].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[137].kMacros[7].Name = nullptr;
    kMacroArray_SM2[137].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[137].kMacros[8].Name = nullptr;
    kMacroArray_SM2[137].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[138].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[138].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[138].kMacros[0].Definition = "6";
    kMacroArray_SM2[139].kMacros[0].Definition = "6";
    kMacroArray_SM2[138].kMacros[1].Name = "FOG";
    kMacroArray_SM2[138].kMacros[1].Definition = "";
    kMacroArray_SM2[138].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[138].kMacros[2].Definition = "";
    kMacroArray_SM2[138].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[138].kMacros[3].Definition = "";
    kMacroArray_SM2[138].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[138].kMacros[4].Definition = "";
    kMacroArray_SM2[138].kMacros[5].Name = nullptr;
    kMacroArray_SM2[138].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[138].kMacros[6].Name = nullptr;
    kMacroArray_SM2[138].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[138].kMacros[7].Name = nullptr;
    kMacroArray_SM2[138].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[138].kMacros[8].Name = nullptr;
    kMacroArray_SM2[138].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[139].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[139].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[139].kMacros[1].Name = "FOG";
    kMacroArray_SM2[139].kMacros[1].Definition = "";
    kMacroArray_SM2[139].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[139].kMacros[2].Definition = "";
    kMacroArray_SM2[139].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[139].kMacros[3].Definition = "";
    kMacroArray_SM2[139].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[139].kMacros[4].Definition = "";
    kMacroArray_SM2[139].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[139].kMacros[5].Definition = "";
    kMacroArray_SM2[139].kMacros[6].Name = nullptr;
    kMacroArray_SM2[139].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[139].kMacros[7].Name = nullptr;
    kMacroArray_SM2[139].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[139].kMacros[8].Name = nullptr;
    kMacroArray_SM2[139].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[140].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[140].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[140].kMacros[0].Definition = "7";
    kMacroArray_SM2[140].kMacros[1].Name = "FOG";
    kMacroArray_SM2[140].kMacros[1].Definition = "";
    kMacroArray_SM2[140].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[140].kMacros[2].Definition = "";
    kMacroArray_SM2[140].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[140].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray_SM2[141].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[141].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[141].kMacros[0].Definition = "7";
    kMacroArray_SM2[141].kMacros[1].Name = "FOG";
    kMacroArray_SM2[141].kMacros[1].Definition = "";
    kMacroArray_SM2[141].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[141].kMacros[2].Definition = "";
    kMacroArray_SM2[141].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray_SM2[141].kMacros[3].Definition = "";
    kMacroArray_SM2[141].kMacros[4].Name = nullptr;
    kMacroArray_SM2[141].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[141].kMacros[5].Name = nullptr;
    kMacroArray_SM2[141].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[141].kMacros[6].Name = nullptr;
    kMacroArray_SM2[141].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[141].kMacros[7].Name = nullptr;
    kMacroArray_SM2[141].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[141].kMacros[8].Name = nullptr;
    kMacroArray_SM2[141].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[142].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[142].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[142].kMacros[0].Definition = "7";
    kMacroArray_SM2[142].kMacros[1].Name = "FOG";
    kMacroArray_SM2[142].kMacros[1].Definition = "";
    kMacroArray_SM2[142].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[142].kMacros[2].Definition = "";
    kMacroArray_SM2[142].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[142].kMacros[3].Definition = "";
    kMacroArray_SM2[142].kMacros[4].Name = nullptr;
    kMacroArray_SM2[142].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[142].kMacros[5].Name = nullptr;
    kMacroArray_SM2[142].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[142].kMacros[6].Name = nullptr;
    kMacroArray_SM2[142].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[142].kMacros[7].Name = nullptr;
    kMacroArray_SM2[142].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[142].kMacros[8].Name = nullptr;
    kMacroArray_SM2[142].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[143].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[143].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[143].kMacros[0].Definition = "7";
    kMacroArray_SM2[143].kMacros[1].Name = "FOG";
    kMacroArray_SM2[143].kMacros[1].Definition = "";
    kMacroArray_SM2[143].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[143].kMacros[2].Definition = "";
    kMacroArray_SM2[143].kMacros[3].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[143].kMacros[3].Definition = "";
    kMacroArray_SM2[143].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[143].kMacros[4].Definition = "";
    kMacroArray_SM2[143].kMacros[5].Name = nullptr;
    kMacroArray_SM2[143].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[143].kMacros[6].Name = nullptr;
    kMacroArray_SM2[143].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[143].kMacros[7].Name = nullptr;
    kMacroArray_SM2[143].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[143].kMacros[8].Name = nullptr;
    kMacroArray_SM2[143].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[144].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[144].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[144].kMacros[0].Definition = "7";
    kMacroArray_SM2[144].kMacros[1].Name = "FOG";
    kMacroArray_SM2[144].kMacros[1].Definition = "";
    kMacroArray_SM2[144].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[144].kMacros[2].Definition = "";
    kMacroArray_SM2[144].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[144].kMacros[3].Definition = "";
    kMacroArray_SM2[144].kMacros[4].Name = nullptr;
    kMacroArray_SM2[144].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[144].kMacros[5].Name = nullptr;
    kMacroArray_SM2[144].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[144].kMacros[6].Name = nullptr;
    kMacroArray_SM2[144].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[144].kMacros[7].Name = nullptr;
    kMacroArray_SM2[144].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[144].kMacros[8].Name = nullptr;
    kMacroArray_SM2[144].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[145].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[145].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[145].kMacros[0].Definition = "7";
    kMacroArray_SM2[145].kMacros[1].Name = "FOG";
    kMacroArray_SM2[145].kMacros[1].Definition = "";
    kMacroArray_SM2[145].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[145].kMacros[2].Definition = "";
    kMacroArray_SM2[145].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[145].kMacros[3].Definition = "";
    kMacroArray_SM2[145].kMacros[4].Name = "PROJ_SHADOW";
    kMacroArray_SM2[145].kMacros[4].Definition = "";
    kMacroArray_SM2[145].kMacros[5].Name = nullptr;
    kMacroArray_SM2[145].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[145].kMacros[6].Name = nullptr;
    kMacroArray_SM2[145].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[145].kMacros[7].Name = nullptr;
    kMacroArray_SM2[145].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[145].kMacros[8].Name = nullptr;
    kMacroArray_SM2[145].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[146].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[146].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[146].kMacros[0].Definition = "7";
    kMacroArray_SM2[146].kMacros[1].Name = "FOG";
    kMacroArray_SM2[146].kMacros[1].Definition = "";
    kMacroArray_SM2[146].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[146].kMacros[2].Definition = "";
    kMacroArray_SM2[146].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[146].kMacros[3].Definition = "";
    kMacroArray_SM2[146].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[146].kMacros[4].Definition = "";
    kMacroArray_SM2[146].kMacros[5].Name = nullptr;
    kMacroArray_SM2[146].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[146].kMacros[6].Name = nullptr;
    kMacroArray_SM2[146].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[146].kMacros[7].Name = nullptr;
    kMacroArray_SM2[147].kMacros[0].Definition = "7";
    kMacroArray_SM2[147].kMacros[2].Name = "HIGHQUAL";
    kMacroArray_SM2[146].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[146].kMacros[8].Name = nullptr;
    kMacroArray_SM2[146].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[147].strPath = "lighting\\2x\\p\\LandO.p.hlsl";
    kMacroArray_SM2[147].kMacros[0].Name = "TEXCOUNT";
    kMacroArray_SM2[147].kMacros[1].Name = "FOG";
    kMacroArray_SM2[147].kMacros[1].Definition = "";
    kMacroArray_SM2[147].kMacros[2].Definition = "";
    kMacroArray_SM2[147].kMacros[3].Name = "PTLIGHTS";
    kMacroArray_SM2[147].kMacros[3].Definition = "";
    kMacroArray_SM2[147].kMacros[4].Name = "IGNORE_NORMALS";
    kMacroArray_SM2[147].kMacros[4].Definition = "";
    kMacroArray_SM2[147].kMacros[5].Name = "PROJ_SHADOW";
    kMacroArray_SM2[147].kMacros[5].Definition = "";
    kMacroArray_SM2[147].kMacros[6].Name = nullptr;
    kMacroArray_SM2[147].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[147].kMacros[7].Name = nullptr;
    kMacroArray_SM2[147].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[147].kMacros[8].Name = nullptr;
    kMacroArray_SM2[147].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[148].strPath = "lighting\\2x\\p\\Lando.p.hlsl";
    kMacroArray_SM2[148].kMacros[0].Name = "FOG";
    kMacroArray_SM2[148].kMacros[0].Definition = "";
    kMacroArray_SM2[148].kMacros[1].Name = "SILHOUETTE";
    kMacroArray_SM2[148].kMacros[1].Definition = "";
    std::memset(&kMacroArray_SM2[148].kMacros[2], 0, sizeof(_D3DXMACRO) * 7);
    kMacroArray_SM2[149].strPath = "lighting\\2x\\p\\landlod.p.hlsl";
    kMacroArray_SM2[149].kMacros[0].Name = "MODELSPACENORM";
    kMacroArray_SM2[149].kMacros[0].Definition = "";
    kMacroArray_SM2[149].kMacros[1].Name = "LODLANDCLIP";
    kMacroArray_SM2[149].kMacros[1].Definition = "";
    kMacroArray_SM2[149].kMacros[2].Name = "LODLANDNOISE";
    kMacroArray_SM2[149].kMacros[2].Definition = "";
    kMacroArray_SM2[149].kMacros[3].Name = "SILHOUETTE";
    kMacroArray_SM2[149].kMacros[3].Definition = "";
    kMacroArray_SM2[149].kMacros[4].Name = nullptr;
    kMacroArray_SM2[149].kMacros[4].Definition = nullptr;
    kMacroArray_SM2[149].kMacros[5].Name = nullptr;
    kMacroArray_SM2[149].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[149].kMacros[6].Name = nullptr;
    kMacroArray_SM2[149].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[149].kMacros[7].Name = nullptr;
    kMacroArray_SM2[149].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[149].kMacros[8].Name = nullptr;
    kMacroArray_SM2[149].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[150].strPath = "lighting\\1x\\p\\base.p.hlsl";
    kMacroArray_SM2[150].kMacros[0].Name = "FOG";
    kMacroArray_SM2[150].kMacros[0].Definition = "";
    kMacroArray_SM2[150].kMacros[1].Name = "FOGANDAMBIENT";
    kMacroArray_SM2[150].kMacros[1].Definition = "";
    kMacroArray_SM2[150].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[150].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[151].strPath = "lighting\\2x\\p\\adts.p.hlsl";
    kMacroArray_SM2[151].kMacros[0].Name = "LIGHTS";
    kMacroArray_SM2[151].kMacros[0].Definition = a0_0;
    kMacroArray_SM2[151].kMacros[1].Name = "SILHOUETTE";
    kMacroArray_SM2[151].kMacros[1].Definition = "";
    kMacroArray_SM2[151].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[151].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray_SM2[152].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[152].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[152].kMacros[0].Definition = "";
    kMacroArray_SM2[152].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[152].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[152].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[152].kMacros[2].Definition = "1";
    kMacroArray_SM2[152].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[152].kMacros[3].Definition = "1";
    kMacroArray_SM2[152].kMacros[4].Name = "SHADOWMODE";
    kMacroArray_SM2[152].kMacros[4].Definition = "3";
    kMacroArray_SM2[152].kMacros[5].Name = nullptr;
    kMacroArray_SM2[152].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[152].kMacros[6].Name = nullptr;
    kMacroArray_SM2[152].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[152].kMacros[7].Name = nullptr;
    kMacroArray_SM2[152].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[152].kMacros[8].Name = nullptr;
    kMacroArray_SM2[152].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[153].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[153].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[153].kMacros[0].Definition = "";
    kMacroArray_SM2[153].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[153].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[153].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[153].kMacros[2].Definition = "1";
    kMacroArray_SM2[153].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[153].kMacros[3].Definition = "1";
    kMacroArray_SM2[153].kMacros[4].Name = "ALPHATEST";
    kMacroArray_SM2[153].kMacros[4].Definition = "";
    kMacroArray_SM2[153].kMacros[5].Name = "SHADOWMODE";
    kMacroArray_SM2[153].kMacros[5].Definition = "3";
    kMacroArray_SM2[153].kMacros[6].Name = nullptr;
    kMacroArray_SM2[153].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[153].kMacros[7].Name = nullptr;
    kMacroArray_SM2[153].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[153].kMacros[8].Name = nullptr;
    kMacroArray_SM2[153].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[154].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[154].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[154].kMacros[0].Definition = "";
    kMacroArray_SM2[154].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[154].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[154].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[154].kMacros[2].Definition = "4";
    kMacroArray_SM2[154].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[154].kMacros[3].Definition = "1";
    kMacroArray_SM2[154].kMacros[4].Name = "SHADOWMODE";
    kMacroArray_SM2[154].kMacros[4].Definition = "3";
    kMacroArray_SM2[154].kMacros[5].Name = nullptr;
    kMacroArray_SM2[154].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[154].kMacros[6].Name = nullptr;
    kMacroArray_SM2[154].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[154].kMacros[7].Name = nullptr;
    kMacroArray_SM2[154].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[154].kMacros[8].Name = nullptr;
    kMacroArray_SM2[154].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[155].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[155].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[155].kMacros[0].Definition = "";
    kMacroArray_SM2[155].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[155].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[155].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[155].kMacros[2].Definition = "4";
    kMacroArray_SM2[155].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[155].kMacros[3].Definition = "1";
    kMacroArray_SM2[155].kMacros[4].Name = "ALPHATEST";
    kMacroArray_SM2[155].kMacros[4].Definition = "";
    kMacroArray_SM2[155].kMacros[5].Name = "SHADOWMODE";
    kMacroArray_SM2[155].kMacros[5].Definition = "3";
    kMacroArray_SM2[155].kMacros[6].Name = nullptr;
    kMacroArray_SM2[155].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[155].kMacros[7].Name = nullptr;
    kMacroArray_SM2[155].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[155].kMacros[8].Name = nullptr;
    kMacroArray_SM2[155].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[156].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[156].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[156].kMacros[0].Definition = "";
    kMacroArray_SM2[156].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[156].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[156].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[156].kMacros[2].Definition = "4";
    kMacroArray_SM2[156].kMacros[4].Definition = "3";
    kMacroArray_SM2[157].kMacros[5].Definition = "3";
    kMacroArray_SM2[156].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[157].kMacros[3].Name = "PASSES";
    kMacroArray_SM2[156].kMacros[3].Definition = "2";
    kMacroArray_SM2[156].kMacros[4].Name = "SHADOWMODE";
    kMacroArray_SM2[156].kMacros[5].Name = nullptr;
    kMacroArray_SM2[156].kMacros[5].Definition = nullptr;
    kMacroArray_SM2[156].kMacros[6].Name = nullptr;
    kMacroArray_SM2[156].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[156].kMacros[7].Name = nullptr;
    kMacroArray_SM2[156].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[156].kMacros[8].Name = nullptr;
    kMacroArray_SM2[156].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[157].strPath = "lighting\\2x\\p\\SimpleShadow.p.hlsl";
    kMacroArray_SM2[157].kMacros[0].Name = "SHADOWMAP";
    kMacroArray_SM2[157].kMacros[0].Definition = "";
    kMacroArray_SM2[157].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray_SM2[157].kMacros[1].Definition = "-0.1";
    kMacroArray_SM2[157].kMacros[2].Name = "SAMPLE";
    kMacroArray_SM2[157].kMacros[2].Definition = "4";
    kMacroArray_SM2[157].kMacros[3].Definition = "2";
    kMacroArray_SM2[157].kMacros[4].Name = "ALPHATEST";
    kMacroArray_SM2[157].kMacros[4].Definition = "";
    kMacroArray_SM2[157].kMacros[5].Name = "SHADOWMODE";
    kMacroArray_SM2[157].kMacros[6].Name = nullptr;
    kMacroArray_SM2[157].kMacros[6].Definition = nullptr;
    kMacroArray_SM2[157].kMacros[7].Name = nullptr;
    kMacroArray_SM2[157].kMacros[7].Definition = nullptr;
    kMacroArray_SM2[157].kMacros[8].Name = nullptr;
    kMacroArray_SM2[157].kMacros[8].Definition = nullptr;
    kMacroArray_SM2[158].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray_SM2[158].kMacros[0].Name = "SRT_EYE";
    kMacroArray_SM2[158].kMacros[0].Definition = "";
    kMacroArray_SM2[158].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[158].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray_SM2[159].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray_SM2[159].kMacros[0].Name = "SRT_HEIGHT";
    kMacroArray_SM2[159].kMacros[0].Definition = "";
    kMacroArray_SM2[159].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray_SM2[159].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    int32_t v1 = 0;
    BSShaderMacro* v2 = kMacroArray_SM1;
    do {
        if (v2->strPath) {
            BSShader::MakePath(v2->strPath, HLSLFile);
            BSsprintf(PSOFile, 260, "SLS1%03i.pso", v1);
            NiD3DPixelShader* v3 = v13->CompilePixelShaderHLSL(HLSLFile, v2->kMacros, "ps_2_0", PSOFile);
            NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[v1];
            if (v4 != v3) {
                if (v4) {
                    NiDecRefCount(v4);
                }
                ShadowLightShader::spPixelShaders[v1] = v3;
                if (v3) {
                    NiIncRefCount(v3);
                }
            }
        }
        ++v1;
        ++v2;
    }
    while (v1 < 48);
    int32_t v5 = BSShaderManager::eShaderLevel;
    int32_t v15 = BSShaderManager::eShaderLevel;
    if (BSShaderManager::eShaderLevel >= 2) {
        char v18[8];
        // EXE-INLINED none: strcpy
        __builtin_strcpy(v18, "-0.6");
        if (BSShaderManager::e2xShaderLevel == 6 || BSShaderManager::e2xShaderLevel == 4) {
            kMacroArray_SM2[152].kMacros[1].Definition = v18;
            kMacroArray_SM2[153].kMacros[1].Definition = v18;
            kMacroArray_SM2[154].kMacros[1].Definition = v18;
            kMacroArray_SM2[155].kMacros[1].Definition = v18;
            kMacroArray_SM2[156].kMacros[1].Definition = v18;
            kMacroArray_SM2[157].kMacros[1].Definition = v18;
        }
        std::sprintf(acShadowMode, "%i", BSShaderManager::eShadowMode);
        BSShaderMacro* v6 = kMacroArray_SM2;
        kMacroArray_SM2[152].kMacros[4].Definition = acShadowMode;
        kMacroArray_SM2[153].kMacros[5].Definition = acShadowMode;
        kMacroArray_SM2[70].kMacros[1].Definition = acShadowMode;
        kMacroArray_SM2[71].kMacros[1].Definition = acShadowMode;
        kMacroArray_SM2[72].kMacros[1].Definition = acShadowMode;
        kMacroArray_SM2[73].kMacros[1].Definition = acShadowMode;
        int32_t v7 = 0;
        BSShaderMacro* v14 = kMacroArray_SM2;
        while (true) {
            if (v6->strPath) {
                if (v7 <= 153 || v5 >= 5) {
                    BSShader::MakePath(v6->strPath, HLSLFile);
                    BSsprintf(PSOFile, 260, "SLS2%03i.pso", v7);
                    if (v7 == 152 || (v7 >= 70 && v7 <= 73)) {
                        BSsprintf(PSOFile, 260, "SLS2%03is%01i.pso", v7, BSShaderManager::eShadowMode);
                    }
                    NiD3DPixelShader* v9 = v13->CompilePixelShaderHLSL(HLSLFile, v6->kMacros, "ps_2_0", PSOFile);
                    NiD3DPixelShader* v10 = ShadowLightShader::spPixelShaders[v7 + 48];
                    NiD3DPixelShader* v11 = v9;
                    if (v10 != v9) {
                        if (v10) {
                            NiDecRefCount(v10);
                        }
                        ShadowLightShader::spPixelShaders[v7 + 48] = v11;
                        if (v11) {
                            NiIncRefCount(v11);
                        }
                    }
                }
            }
            else {
                NiD3DPixelShader* v8 = ShadowLightShader::spPixelShaders[v7 + 48];
                if (v8) {
                    NiDecRefCount(v8);
                    ShadowLightShader::spPixelShaders[v7 + 48] = nullptr;
                }
            }
            ++v7;
            v6 = ++v14;
            if (v7 >= 160) {
                break;
            }
            v5 = v15;
        }
    }
}

bool ShadowLightShader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* v1 = ShadowLightShader::spPass;
    int32_t a6 = 759;
    do {
        NiD3DPass::CreateNewPass(*v1);
        NiD3DVertexShader* m_pObject = v1->m_pObject->m_spVertexShader.m_pObject;
        NiPointer<NiD3DVertexShader>* p_m_spVertexShader = &v1->m_pObject->m_spVertexShader;
        NiD3DVertexShader* v4 = ShadowLightShader::spVertexShaders[0];
        if (m_pObject != ShadowLightShader::spVertexShaders[0]) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            p_m_spVertexShader->m_pObject = v4;
            if (v4) {
                NiIncRefCount(v4);
            }
        }
        ++v1;
        --a6;
    }
    while (a6);
    PresetZOnlyPasses_1x();
    PresetConstAlphaPasses_1x();
    PresetAmbientPasses_1x();
    PresetAmbDiffTexPasses_1x();
    PresetAmbDiffDirAndPtPasses_1x();
    PresetDiffuseLightPasses_1x();
    PresetTexturePasses_1x();
    PresetSpecularPasses_1x();
    PresetEnvMapPasses_1x();
    PresetFogPasses_1x();
    PresetTextureEffectPasses_1x();
    if (BSShaderManager::eShaderLevel < 2) {
        PresetLandscape_1x();
    }
    else {
        PresetADPasses_2x();
        PresetADTPasses_2x();
        PresetADTSPasses_2x();
        PresetDiffusePtPasses_2x();
        PresetSpecularPasses_2x();
        PresetEnvMapPasses_2x();
        PresetLandscape_2x();
        PresetRenderNormalsPasses_2x();
        ShadowLightShader::PresetDepthPasses_2x();
        ShadowLightShader::PresetVelocityPasses_2x();
        ShadowLightShader::PresetLocalMapPasses_2x();
        PresetTextureEffectPasses_2x();
        PresetWaterFogPasses_2x();
    }
    return true;
}

NiSourceTexture* ShadowLightShader::GetCubeMap(BSShaderPPLightingProperty* apShaderProp, uint32_t aiTextureNumber) {
    ShadowSceneNode* pActiveShadowSceneNode = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode;
    NiSourceTexture* result =
        static_cast<NiSourceTexture*>(apShaderProp->GetTexture(BSShaderProperty::TEXTYPE_ENV, aiTextureNumber));
    if (!result) {
        return BSShaderManager::spArmorReflectionCubeMap;
    }
    return result;
}

void ShadowLightShader::SetupPassOpt_AmbientLight() {
    NiLight* spLight;
    float g;
    float b;
    spLight = (*BSShaderManager::pCurrentRenderPass->ppSceneLights)->spLight.m_pObject;
    g = spLight->m_kAmb.g;
    b = spLight->m_kAmb.b;
    BSShaderLightingProperty::pShaderLightConstants[0].x = spLight->m_kAmb.r;
    BSShaderLightingProperty::pShaderLightConstants[0].y = g;
    BSShaderLightingProperty::pShaderLightConstants[0].z = b;
    BSShaderLightingProperty::pShaderLightConstants[0].w = (float)1.0;
}

// The scene graph's fog as vertex shader constants c14-c15 (pShaderLightConstants[28..29]): the end distance, the fog
// range and the power, then the fog colour with a zero alpha. With no fog distances, a far end of 500000 and
// NiColorA::BLACK.
void ShadowLightShader::SetupPassOpt_FogConstants() {
    BSFogProperty* pFogProperty = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
    if (pFogProperty) {
        float fStartDistance = pFogProperty->fStartDistance;
        float fEndDistance = pFogProperty->fEndDistance;
        float fPower = pFogProperty->fPower;
        float g;
        float b;
        float a;
        if (fEndDistance == 0.0f && 0.0f == fStartDistance) {
            BSShaderLightingProperty::pShaderLightConstants[28].x = 500000.0f;
            BSShaderLightingProperty::pShaderLightConstants[28].y = 0.0f;
            BSShaderLightingProperty::pShaderLightConstants[28].z = 0.0f;
            g = NiColorA_BLACK.g;
            BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
            b = NiColorA_BLACK.b;
            BSShaderLightingProperty::pShaderLightConstants[29].x = NiColorA_BLACK.r;
            a = NiColorA_BLACK.a;
        }
        else {
            float r = pFogProperty->m_kColor.r;
            float fNear = fEndDistance - fStartDistance;
            float fG = pFogProperty->m_kColor.g;
            float fB = pFogProperty->m_kColor.b;
            BSShaderLightingProperty::pShaderLightConstants[28].x = fEndDistance;
            BSShaderLightingProperty::pShaderLightConstants[28].y = fNear;
            BSShaderLightingProperty::pShaderLightConstants[28].z = fPower;
            g = fG;
            BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
            b = fB;
            BSShaderLightingProperty::pShaderLightConstants[29].x = r;
            a = 0.0f;
        }
        BSShaderLightingProperty::pShaderLightConstants[29].w = a;
        BSShaderLightingProperty::pShaderLightConstants[29].z = b;
        BSShaderLightingProperty::pShaderLightConstants[29].y = g;
    }
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
        14, &BSShaderLightingProperty::pShaderLightConstants[28].x, 2
    );
}

// Enables each pixel constant map entry whose bit is set in the current pass's flags (entry i is bit i + 1), then
// sorts the map's enabled entries first.
void ShadowLightShader::SetupPassOpt_PreparePixelShaderConstants() {
    uint32_t uiBit = 2;
    for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spPixelConstantMapEntries) {
        if (pEntry) {
            pEntry->m_bEnabled = (uiBit & ShadowLightShader::ulPixelConstantFlags[BSShaderManager::eCurrentPass]) != 0;
        }
        uiBit <<= 1;
    }
    m_spPixelConstantMap.m_pObject->SortByEnabled();
}

void ShadowLightShader::SetupPassOpt_LightColor() {
    ShadowSceneLight** ppSceneLights = BSShaderManager::pCurrentRenderPass->ppSceneLights;
    ShadowSceneLight* v2 = *ppSceneLights;
    NiLight* m_pObject = (*ppSceneLights)->spLight.m_pObject;
    double v4 = m_pObject->m_fDimmer;
    if (!BSShaderManager::bHDR && v4 > 1.0) {
        v4 = 1.0f;
    }
    float v9 = m_pObject->m_kDiff.r * v4;
    float v10 = m_pObject->m_kDiff.g * v4;
    float v11 = v4 * m_pObject->m_kDiff.b;
    double fLODDimmer = v2->fLODDimmer;
    float v6 = v9 * fLODDimmer;
    float v7 = fLODDimmer * v10;
    float v8 = fLODDimmer * v11;
    if (BSShaderManager::bHDR && !BSShaderManager::bInterior) {
        v6 = BSShaderManager::fSunlightDimmer * v6;
        v7 = BSShaderManager::fSunlightDimmer * v7;
        v8 = BSShaderManager::fSunlightDimmer * v8;
    }
    float fShadowLODDimmer = v2->fShadowLODDimmer;
    BSShaderLightingProperty::pShaderLightConstants[1].x = v6;
    BSShaderLightingProperty::pShaderLightConstants[1].y = v7;
    BSShaderLightingProperty::pShaderLightConstants[1].z = v8;
    BSShaderLightingProperty::pShaderLightConstants[1].w = fShadowLODDimmer;
}

void ShadowLightShader::SetupPass_ADT4_Opt() {
    SetupPassOpt_FogConstants();
    SetupPassOpt_PreparePixelShaderConstants();
    SetupPassOpt_AmbientLight();
    SetupPassOpt_LightColor();
}

void ShadowLightShader::SetClampMode(NiD3DTextureStage* textureStage, BSShaderPPLightingProperty* shaderProp) {
    int32_t v3;
    if ((textureStage) && (shaderProp)) {
        v3 = (int32_t)shaderProp->GetClampNI();
        textureStage->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v3));
    }
}

void ShadowLightShader::SetDiffuseMap(BSShaderPPLightingProperty* pprop, uint32_t auiStage, uint32_t auiIndex) {
    NiD3DTextureStage* m_pObject;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiStage].m_pObject;
    m_pObject->m_spTexture = pprop->spTextures[0][auiIndex].m_pObject;
    int32_t v5 = (int32_t)pprop->GetClampNI();
    m_pObject->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v5));
}

void ShadowLightShader::SetNormalMap(BSShaderPPLightingProperty* pprop, uint32_t auiStage, uint32_t auiIndex) {
    NiD3DTextureStage* m_pObject;
    int32_t v5;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiStage].m_pObject;
    m_pObject->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(pprop, auiIndex);
    if (pprop) {
        v5 = (int32_t)pprop->GetClampNI();
        m_pObject->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v5));
    }
}

void ShadowLightShader::SetShadowMap(uint32_t auiStage) {
    NiD3DTextureStage* m_pObject;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiStage].m_pObject;
    m_pObject->m_spTexture =
        (*BSShaderManager::pCurrentRenderPass->ppSceneLights)->spDynamicShadowMap.m_pObject->GetTexture(0);
}

void ShadowLightShader::SetProjectedShadowMap(uint32_t auiStage) {
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[(int32_t)auiStage].m_pObject->m_spTexture =
        BSShaderManager::pProjectedShadowTexture.m_pObject;
}

void ShadowLightShader::SetGlowMap(
    BSShaderPPLightingProperty* pprop,
    uint32_t auiStage,
    uint32_t auiIndex,
    bool abUseDefaultOnNULL
) {
    NiD3DTextureStage* m_pObject;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiStage].m_pObject;
    NiTexture* v6 = pprop->spTextures[2][auiIndex].m_pObject;
    if (v6) {
        m_pObject->m_spTexture = v6;
    }
    else {
        m_pObject->m_spTexture = abUseDefaultOnNULL ? BSShaderManager::spDefaultHeightMap.m_pObject : nullptr;
    }
}

void ShadowLightShader::SetCubeMap(
    BSShaderPPLightingProperty* pShaderProp,
    int32_t uiTexturePass,
    int32_t uiTextureNumber
) {
    NiD3DTextureStage* m_pObject;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[uiTexturePass].m_pObject;
    m_pObject->m_spTexture = GetCubeMap(pShaderProp, uiTextureNumber);
    m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
}

void ShadowLightShader::SetProjectedShadowMaskMap(uint32_t auiStage) {
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiStage].m_pObject->m_spTexture =
        static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
}

// The texture effect pass: the normal map and the effect texture (the default one when the effect has none) on
// stages 0 and 1, the effect's blend, blend op and z test, its constants, then the pass's stages.
void ShadowLightShader::UpdatePipelineTexEffect(
    NiGeometry* apGeometry,
    BSShaderPPLightingProperty* apBSSProp,
    BSShaderProperty::RenderPass* apPass
) {
    if (!apPass) {
        return;
    }
    NiD3DPass* pPass = ShadowLightShader::spPass[apPass->usPassEnum].m_pObject;
    BSShaderPPLightingProperty::TextureEffectData* pTexEffectData = apBSSProp->spTexEffectData;
    if (!pTexEffectData) {
        return;
    }
    NiD3DTextureStage* pStage = pPass->m_kStages.m_pBase[0].m_pObject;
    pStage->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(apBSSProp, 0);
    pStage->SetClampMode(apBSSProp->GetClampNI());
    pStage = pPass->m_kStages.m_pBase[1].m_pObject;
    if (pTexEffectData->spTexture.m_pObject) {
        pStage->m_spTexture = pTexEffectData->spTexture.m_pObject;
    }
    else {
        pStage->m_spTexture = BSShaderManager::spDefaultTexEffectMap;
    }
    pStage->SetClampMode(apBSSProp->GetClampNI());
    NiD3DShader::m_spCurrentPass->GetD3DRenderState()->SetDestAndSrcBlends(
        pTexEffectData->eSrcBlendMode, pTexEffectData->eDestBlendMode, 0, 0
    );
    NiD3DShader::m_spCurrentPass->GetD3DRenderState()->SetRenderState(
        D3DRS_BLENDOP, pTexEffectData->eBlendOperation, 0, false
    );
    NiD3DShader::m_spCurrentPass->GetD3DRenderState()->SetRenderState(
        D3DRS_ZFUNC, pTexEffectData->eZTestFunction, 0, false
    );
    UpdateTexEffectConstants(apGeometry, apBSSProp);
    ApplyTextureStages();
    NiD3DShader::m_spCurrentPass = pPass;
}

void ShadowLightShader::UpdateTexEffectConstants(NiGeometry* apGeometry, BSShaderPPLightingProperty* apProp) {
    if (apProp) {
        BSShaderPPLightingProperty::TextureEffectData* pData = apProp->spTexEffectData.m_pObject;
        if (pData) {
            ShadowLightShader::kFillColor.x = pData->CurrentFillColor.r;
            ShadowLightShader::kFillColor.y = pData->CurrentFillColor.g;
            ShadowLightShader::kFillColor.z = pData->CurrentFillColor.b;
            ShadowLightShader::kFillColor.w = pData->CurrentFillColor.a;
            ShadowLightShader::kRimColor.x = pData->CurrentRimColor.r;
            ShadowLightShader::kRimColor.y = pData->CurrentRimColor.g;
            ShadowLightShader::kRimColor.z = pData->CurrentRimColor.b;
            ShadowLightShader::kRimColor.w = pData->CurrentRimColor.a;
            ShadowLightShader::fTextureUOffset = pData->fTextureUOffset;
            ShadowLightShader::fTextureVOffset = pData->fTextureVOffset;
            ShadowLightShader::fVars.x = pData->fEdgeExponent;
            ShadowLightShader::fVars.y = 1.0f;
        }
        ShadowLightShader::kTexEffectFogColor.x = 0.0f;
        ShadowLightShader::kTexEffectFogColor.y = 0.0f;
        ShadowLightShader::kTexEffectFogColor.z = 0.0f;
        ShadowLightShader::kTexEffectFogColor.w = 0.0f;
        ShadowLightShader::kTexEffectFogParam.x = 0.0f;
        ShadowLightShader::kTexEffectFogParam.y = 0.0f;
        ShadowLightShader::kTexEffectFogParam.z = 0.0f;
        ShadowLightShader::kTexEffectFogParam.w = 0.0f;
        if (apGeometry) {
            BSFogProperty* pFog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
            if (pFog) {
                float fStartDistance = pFog->fStartDistance;
                float fEndDistance = pFog->fEndDistance;
                float fPower = pFog->fPower;
                if (fEndDistance != 0.0f || 0.0f != fStartDistance) {
                    NiColor kColor = pFog->m_kColor;
                    ShadowLightShader::kTexEffectFogParam.x = fEndDistance;
                    ShadowLightShader::kTexEffectFogParam.y = fEndDistance - fStartDistance;
                    ShadowLightShader::kTexEffectFogParam.z = fPower;
                    ShadowLightShader::kTexEffectFogParam.w = 0.0f;
                    ShadowLightShader::kTexEffectFogColor.x = kColor.r;
                    ShadowLightShader::kTexEffectFogColor.y = kColor.g;
                    ShadowLightShader::kTexEffectFogColor.z = kColor.b;
                    ShadowLightShader::kTexEffectFogColor.w = 0.0f;
                }
            }
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_RefractionPower(BSShaderPPLightingProperty* pprop) {
    if (ShadowLightShader::spPixelConstantMapEntries[0x10u]->m_bEnabled) {
        ShadowLightShader::kRefractionParams.x = pprop->fRefractionPower;
        if (((int32_t)(pprop->ulFlags[1] & 16)) != 0) {
            ShadowLightShader::kRefractionParams.y = 0.8999999761581421f;
        }
        else {
            ShadowLightShader::kRefractionParams.y = (float)1.0;
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_WorldBoundCenter(NiGeometry* pgeometry) {
    NiBound* m_kWorldBound;
    float v3;
    float v4;
    float v5;
    if (ShadowLightShader::spVertexConstantMapEntries[17]->m_bEnabled) {
        m_kWorldBound = pgeometry->m_kWorldBound;
        if (!m_kWorldBound) {
            m_kWorldBound = &NiAVObject::NullBoundS;
        }
        v3 = m_kWorldBound->m_kCenter.x - NiRenderer::PosAdjust.x;
        v4 = m_kWorldBound->m_kCenter.y - NiRenderer::PosAdjust.y;
        v5 = m_kWorldBound->m_kCenter.z - NiRenderer::PosAdjust.z;
        ShadowLightShader::kWorldBoundCenter.x = v3;
        ShadowLightShader::kWorldBoundCenter.y = v4;
        ShadowLightShader::kWorldBoundCenter.z = v5;
    }
}

void ShadowLightShader::SetEnvMask(
    BSShaderPPLightingProperty* pShaderProp,
    int32_t uiTexturePass,
    int32_t uiTextureNumber
) {
    NiD3DTextureStage* m_pObject;
    m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[uiTexturePass].m_pObject;
    if (pShaderProp->GetTexture(BSShaderProperty::TEXTYPE_ENV_MASK, (uint32_t)uiTextureNumber)) {
        m_pObject->m_spTexture = pShaderProp->GetTexture(BSShaderProperty::TEXTYPE_ENV_MASK, (uint32_t)uiTextureNumber);
    }
    else {
        m_pObject->m_spTexture = BSShaderManager::spDefaultErrorMap;
    }
}

void ShadowLightShader::UpdatePipelineZOnlyTexEffect(
    NiGeometry* apGeometry,
    BSShaderPPLightingProperty* apBSSProp,
    BSShaderProperty::RenderPass* apPass
) {
    BSShaderPPLightingProperty::TextureEffectData* m_pObject;
    NiD3DPass* v6;
    NiSourceTexture* v7;
    NiD3DTextureStage* v8;
    int32_t v9;
    if (apPass) {
        m_pObject = apBSSProp->spTexEffectData.m_pObject;
        v6 = ShadowLightShader::spPass[apPass->usPassEnum].m_pObject;
        if (m_pObject) {
            v7 = static_cast<NiSourceTexture*>(m_pObject->spBlockOutTexture.m_pObject);
            v8 = v6->m_kStages.m_pBase->m_pObject;
            if (!v7) {
                v7 = BSShaderManager::spDefaultTexEffectMap;
            }
            v8->m_spTexture = v7;
            v9 = (int32_t)apBSSProp->GetClampNI();
            v8->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v9));
            BSRenderState::SetAlphaTestEnable(true, BSRS_NOLOCK);
            BSRenderState::SetAlphaTestFuncRef(NiAlphaProperty::TEST_GREATER, m_pObject->iAlphaTest, BSRS_NOLOCK);
            ApplyTextureStages();
            NiD3DShader::m_spCurrentPass = v6;
        }
    }
}

void ShadowLightShader::DisableNonVolatileConstantsForPass(uint32_t auiPassEnum) {
    // Entry i's flag is bit i + 1. The 22 entries never wrap the bit, so a shift is enough.
    if (ShadowLightShader::uiVertexNonVolatileConstantFlags[auiPassEnum]) {
        uint32_t uiBit = 1;
        for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spVertexConstantMapEntries) {
            uiBit <<= 1;
            if (pEntry && pEntry->m_bEnabled) {
                pEntry->m_bEnabled = (uiBit & ShadowLightShader::uiVertexNonVolatileConstantFlags[auiPassEnum]) == 0;
            }
        }
        m_spVertexConstantMap.m_pObject->SortByEnabled();
    }
    if (ShadowLightShader::uiPixelNonVolatileConstantFlags[auiPassEnum]) {
        uint32_t uiBit = 1;
        for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spPixelConstantMapEntries) {
            uiBit <<= 1;
            if (pEntry && pEntry->m_bEnabled) {
                pEntry->m_bEnabled = (uiBit & ShadowLightShader::uiPixelNonVolatileConstantFlags[auiPassEnum]) == 0;
            }
        }
        m_spPixelConstantMap.m_pObject->SortByEnabled();
    }
}

void ShadowLightShader::SetupGeometryConstants_ObjToCube(
    BSShaderProperty::RenderPass* apRenderPass,
    uint32_t epass,
    const NiSkinInstance* apSkin,
    const NiTransform& aWorld
) {
    alignas(16) D3DXMATRIX kobjtoworld;
    alignas(16) D3DXMATRIX kobjtoshadow;
    if (ShadowLightShader::spVertexConstantMapEntries[1]->m_bEnabled) {
        uint8_t ucNumLights;
        ShadowSceneLight** ppSceneLights = apRenderPass->ppSceneLights;
        if (!ppSceneLights ||
            (((ucNumLights = apRenderPass->ucNumLights) != 0 && (*ppSceneLights && (*ppSceneLights)->bCubeShadowMap)) ||
             ((epass >= 580 && epass <= 587) || ((epass >= 728 && epass <= 751) || (epass == 254 || epass == 267))))) {
            kobjtoshadow = m_pkD3DRenderer->m_kD3DMat;
            D3DXMatrixTranspose_Thunk(&ShadowLightShader::kObjectToWorldTranspose, &kobjtoshadow);
        }
        else {
            BSShaderProperty::RenderPass* pRenderPass = BSShaderManager::pCurrentRenderPass;
            if (pRenderPass->ppSceneLights && ucNumLights) {
                alignas(16) D3DXMATRIX kLightMatrixCopy;
                kobjtoworld._43 = 0.0f;
                kobjtoworld._42 = 0.0f;
                kobjtoworld._41 = 0.0f;
                kobjtoworld._34 = 0.0f;
                kobjtoworld._32 = 0.0f;
                kobjtoworld._31 = 0.0f;
                kobjtoworld._24 = 0.0f;
                kobjtoworld._23 = 0.0f;
                kobjtoworld._21 = 0.0f;
                kobjtoworld._14 = 0.0f;
                kobjtoworld._13 = 0.0f;
                kobjtoworld._12 = 0.0f;
                kobjtoworld._44 = 1.0f;
                kobjtoworld._33 = 1.0f;
                kobjtoworld._22 = 1.0f;
                kobjtoworld._11 = 1.0f;
                kLightMatrixCopy._44 = 1.0f;
                kLightMatrixCopy._33 = 1.0f;
                kLightMatrixCopy._22 = 1.0f;
                kLightMatrixCopy._11 = 1.0f;
                kLightMatrixCopy._43 = 0.0f;
                kLightMatrixCopy._42 = 0.0f;
                kLightMatrixCopy._41 = 0.0f;
                kLightMatrixCopy._34 = 0.0f;
                kLightMatrixCopy._32 = 0.0f;
                kLightMatrixCopy._31 = 0.0f;
                kLightMatrixCopy._24 = 0.0f;
                kLightMatrixCopy._23 = 0.0f;
                kLightMatrixCopy._21 = 0.0f;
                kLightMatrixCopy._14 = 0.0f;
                kLightMatrixCopy._13 = 0.0f;
                kLightMatrixCopy._12 = 0.0f;
                if (!apSkin) {
                    NiD3DUtility::GetD3DFromNi(kobjtoworld, aWorld);
                }
                kobjtoworld._41 = NiRenderer::PosAdjust.x + kobjtoworld._41;
                kobjtoworld._42 = NiRenderer::PosAdjust.y + kobjtoworld._42;
                kobjtoworld._43 = NiRenderer::PosAdjust.z + kobjtoworld._43;
                D3DXMatrixMultiply_Thunk(&kobjtoshadow, &kobjtoworld, &(*pRenderPass->ppSceneLights)->kViewProjection);
                kLightMatrixCopy = kobjtoshadow;
                D3DXMatrixTranspose_Thunk(&kLightMatrixCopy, &kLightMatrixCopy);
                rt::memcpy(&ShadowLightShader::kObjectToWorldTranspose, &kLightMatrixCopy, sizeof(kLightMatrixCopy));
            }
        }
    }
}

// Passes 561-564: the first scene light's six shadow camera planes as D3D clip planes. Built once per light: the
// planes go through the inverse transpose of the view (moved by NiRenderer::PosAdjust) times projection into clip
// space and are kept in the light's pfClipSpacePlanes.
void ShadowLightShader::SetupGeometryConstants_SimpleShadowClipPlanes(uint32_t epass) {
    alignas(16) D3DXMATRIX pM1;
    alignas(16) D3DXMATRIX kworldtoclip;
    alignas(16) D3DXMATRIX kinv;
    if (epass - 561 <= 3) {
        ShadowSceneLight* pP = BSShaderManager::pCurrentRenderPass->ppSceneLights[0];
        if (pP->bClipSpacePlanesReady) {
            NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
            for (int32_t i = 0; i < 6; ++i) {
                pRenderer->m_pkD3DDevice9->SetClipPlane(i, &pP->pfClipSpacePlanes[i * 4]);
            }
        }
        else {
            alignas(16) float afClipPlanes[24];
            alignas(16) D3DXMATRIX pM2;
            NiDX9Renderer* m_pkD3DRenderer = this->m_pkD3DRenderer;
            pM1 = m_pkD3DRenderer->m_kD3DView;
            float v17 = NiRenderer::PosAdjust.y * pM1._21 + NiRenderer::PosAdjust.x * pM1._11 +
                NiRenderer::PosAdjust.z * pM1._31;
            pM1._41 = -v17;
            float v18 = NiRenderer::PosAdjust.y * pM1._22 + NiRenderer::PosAdjust.x * pM1._12 +
                NiRenderer::PosAdjust.z * pM1._32;
            pM1._42 = -v18;
            float v19 = NiRenderer::PosAdjust.y * pM1._23 + NiRenderer::PosAdjust.x * pM1._13 +
                NiRenderer::PosAdjust.z * pM1._33;
            pM1._43 = -v19;
            pM2 = m_pkD3DRenderer->m_kD3DProj;
            D3DXMatrixMultiply_Thunk(&kworldtoclip, &pM1, &pM2);
            D3DXMatrixInverse_Thunk(&kinv, nullptr, &kworldtoclip);
            D3DXMatrixTranspose_Thunk(&kworldtoclip, &kinv);
            for (uint32_t i = 0; i < 6; ++i) {
                const NiPlane& rPlane = pP->kShadowCameraPlanes.m_akCullingPlanes[i];
                D3DXPLANE kplane;
                alignas(16) D3DXPLANE kClipPlane;
                kplane.a = rPlane.m_kNormal.x;
                kplane.b = rPlane.m_kNormal.y;
                kplane.c = rPlane.m_kNormal.z;
                kplane.d = -rPlane.m_fConstant;
                D3DXPlaneNormalize_Thunk(&kplane, &kplane);
                D3DXPlaneTransform_Thunk(&kClipPlane, &kplane, &kworldtoclip);
                afClipPlanes[i * 4] = kClipPlane.a;
                afClipPlanes[i * 4 + 1] = kClipPlane.b;
                afClipPlanes[i * 4 + 2] = kClipPlane.c;
                afClipPlanes[i * 4 + 3] = kClipPlane.d;
            }
            NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
            for (int32_t i = 0; i < 6; ++i) {
                pRenderer->m_pkD3DDevice9->SetClipPlane(i, &afClipPlanes[i * 4]);
            }
            rt::memcpy(pP->pfClipSpacePlanes, afClipPlanes, sizeof(pP->pfClipSpacePlanes));
            pP->bClipSpacePlanesReady = true;
        }
    }
}

void ShadowLightShader::SetupGeometryOpt_WorldViewProj(NiGeometry* apGeometry, const NiTransform& aWorld) {
    alignas(16) D3DXMATRIX mat;
    m_pkD3DRenderer->SetModelTransform(aWorld, false);
    D3DXMatrixMultiplyTranspose_Thunk(&mat, &m_pkD3DRenderer->m_kD3DMat, &m_pkD3DRenderer->m_kViewProj);
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(0, &mat._11, 4);
}

void ShadowLightShader::Setup_ADT_Opt(NiPropertyState* a1) {
    _D3DMATRIX a1a;
    BSShaderPPLightingProperty* m_pObject = static_cast<BSShaderPPLightingProperty*>(a1->m_aspProps[3].m_pObject);
    NiD3DTextureStage* v4 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
    v4->m_spTexture = m_pObject->spTextures[0]->m_pObject;
    NiTexturingProperty::ClampMode v5 = m_pObject->GetClampNI();
    v4->SetClampMode(v5);
    NiD3DTextureStage* v6 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
    v6->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(m_pObject, 0);
    NiTexturingProperty::ClampMode v7 = m_pObject->GetClampNI();
    v6->SetClampMode(v7);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->ConfigureStage(false);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject->ConfigureStage(false);
    NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSUtilities::GetInverseWorldMatrix(BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld, nullptr, a1a);
    SetupGeometryOpt_WorldViewProj(pGeometry, pGeometry->m_kWorld);
    SetLightDirection(&a1a);
}

void ShadowLightShader::Setup_ADT4_Opt(NiPropertyState* a1) {
    _D3DMATRIX pM;
    BSShaderPPLightingProperty* m_pObject = static_cast<BSShaderPPLightingProperty*>(a1->m_aspProps[3].m_pObject);
    NiD3DTextureStage* v4 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
    v4->m_spTexture = m_pObject->spTextures[0]->m_pObject;
    NiTexturingProperty::ClampMode v5 = m_pObject->GetClampNI();
    v4->SetClampMode(v5);
    NiD3DTextureStage* v6 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
    v6->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(m_pObject, 0);
    NiTexturingProperty::ClampMode v7 = m_pObject->GetClampNI();
    v6->SetClampMode(v7);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->ConfigureStage(false);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject->ConfigureStage(false);
    NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSUtilities::GetInverseWorldMatrix(BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld, nullptr, pM);
    SetupGeometryOpt_WorldViewProj(pGeometry, pGeometry->m_kWorld);
    SetupGeometryOpt_Lights(pM, pGeometry->m_kWorld.m_fScale);
    NiD3DShaderConstantMap* v9 = m_spPixelConstantMap.m_pObject;
    if ((v9) && (NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject)) {
        NiBound v10;
        NiTransform v12;
        v9->SetShaderConstants(
            NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, v12, v10, 0, true
        );
    }
}

void ShadowLightShader::SetupGeometryConstants_Lights(
    BSShaderPPLightingProperty* pprop,
    BSShaderProperty::RenderPass* apRenderPass,
    _D3DMATRIX d3dInvWorld,
    const NiTransform& aWorld,
    uint32_t epass,
    const NiSkinInstance* apSkin
) {
    const NiTransform* pWorld = &aWorld;
    _D3DVECTOR vLightPos;
    _D3DVECTOR vLightObjectPos;
    _D3DVECTOR vLightDir;
    _D3DVECTOR vLightObjectDir;
    NiPoint4 kDimmed;
    BSShaderPPLightingProperty* v7 = pprop;
    float fdarkness = pprop->fForcedDarkness;
    if (BSShaderManager::eRenderMode == 15) {
        BSShaderAccumulator* pAccumulator = BSShaderManager::GetCurrentAccumulator();
        BSShaderLightingProperty::pShaderLightConstants[0].x = pAccumulator->kSilhouetteColor.r;
        BSShaderLightingProperty::pShaderLightConstants[0].y = pAccumulator->kSilhouetteColor.g;
        BSShaderLightingProperty::pShaderLightConstants[0].z = pAccumulator->kSilhouetteColor.b;
        BSShaderLightingProperty::pShaderLightConstants[0].w = pAccumulator->kSilhouetteColor.a;
        return;
    }
    if (ShadowLightShader::spVertexConstantMapEntries[6]->m_bEnabled) {
        uint16_t usPassEnum = apRenderPass->usPassEnum;
        if (usPassEnum < 16u || usPassEnum > 19u) {
            ShadowLightShader::spVertexConstantMapEntries[6]->m_uiRegisterCount = apRenderPass->ucNumLights;
        }
        else {
            ShadowLightShader::spVertexConstantMapEntries[6]->m_uiRegisterCount = 1;
        }
    }
    if (ShadowLightShader::spPixelConstantMapEntries[2]->m_bEnabled) {
        ShadowLightShader::spPixelConstantMapEntries[2]->m_uiRegisterCount = apRenderPass->ucNumLights;
    }
    if (ShadowLightShader::spPixelConstantMapEntries[7]->m_bEnabled) {
        ShadowLightShader::spPixelConstantMapEntries[7]->m_uiRegisterCount = apRenderPass->ucNumLights - 1;
    }
    uint8_t i = 0;
    if (apRenderPass->ucNumLights) {
        int32_t v13;
        double w;
        float v15;
        float v16;
        float v17;
        while (true) {
            ShadowSceneLight* psslight;
            uint16_t v12;
            if (i >= 10u) {
            LABEL_28:;
                v7 = pprop;
                goto LABEL_29;
            }
            psslight = apRenderPass->ppSceneLights[i];
            BSShaderLightingProperty::SetLight1x2x(i, psslight, fdarkness);
            v12 = apRenderPass->usPassEnum;
            if (v12 >= 16u && v12 <= 19u) {
                break;
            }
            if (psslight) {
                v13 = i;
                if (psslight->bPointLight) {
                    vLightPos.x = BSShaderLightingProperty::pShaderLightConstants[i + 11].X();
                    vLightPos.y = BSShaderLightingProperty::pShaderLightConstants[i + 11].Y();
                    vLightPos.z = BSShaderLightingProperty::pShaderLightConstants[i + 11].Z();
                    float y = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y;
                    float z = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z;
                    float xOffset = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x;
                    float v29 = y;
                    float v30 = z;
                    vLightPos.x = xOffset + vLightPos.x;
                    vLightPos.y = y + vLightPos.y;
                    vLightPos.z = z + vLightPos.z;
                    D3DXVec3TransformCoord_Thunk(&vLightObjectPos, &vLightPos, &d3dInvWorld);
                    float apRenderPassa = BSShaderLightingProperty::pShaderLightConstants[i + 19].X();
                    if (!apSkin) {
                        uint16_t v20 = apRenderPass->usPassEnum;
                        if (v20 < 561u || v20 > 563u) {
                            apRenderPassa = apRenderPassa / pWorld->m_fScale;
                        }
                        else {
                            float fScale = pWorld->m_fScale;
                            vLightObjectPos.x = fScale * vLightObjectPos.x;
                            vLightObjectPos.y = fScale * vLightObjectPos.y;
                            vLightObjectPos.z = fScale * vLightObjectPos.z;
                        }
                    }
                    ShadowLightShader::pLightData[i].r = vLightObjectPos.x;
                    ShadowLightShader::pLightData[i].g = vLightObjectPos.y;
                    ShadowLightShader::pLightData[i].b = vLightObjectPos.z;
                    w = apRenderPassa;
                }
                else {
                    vLightDir.x = -BSShaderLightingProperty::pShaderLightConstants[i + 19].X();
                    vLightDir.y = -BSShaderLightingProperty::pShaderLightConstants[i + 19].Y();
                    vLightDir.z = -BSShaderLightingProperty::pShaderLightConstants[i + 19].Z();
                    D3DXVec3TransformNormal_Thunk(&vLightObjectDir, &vLightDir, &d3dInvWorld);
                    D3DXVec3Normalize_Thunk(&vLightDir, &vLightObjectDir);
                    ShadowLightShader::pLightData[i].r = vLightDir.x;
                    ShadowLightShader::pLightData[i].g = vLightDir.y;
                    ShadowLightShader::pLightData[i].b = vLightDir.z;
                    w = BSShaderLightingProperty::pShaderLightConstants[i + 19].W();
                }
                goto LABEL_26;
            }
        LABEL_27:;
            if (++i >= apRenderPass->ucNumLights) {
                goto LABEL_28;
            }
        }
        v13 = i;
        w = BSShaderLightingProperty::pShaderLightConstants[v13 + 19].X();
        v15 = BSShaderLightingProperty::pShaderLightConstants[v13 + 11].Y();
        v16 = BSShaderLightingProperty::pShaderLightConstants[v13 + 11].Z();
        ShadowLightShader::pLightData[v13].r = BSShaderLightingProperty::pShaderLightConstants[v13 + 11].X();
        v17 = BSShaderLightingProperty::pShaderLightConstants[v13 + 11].W();
        ShadowLightShader::pLightData[v13].g = v15;
        ShadowLightShader::pLightData[v13].b = v16;
        ShadowLightShader::pLightData[v13].a = v17;
    LABEL_26:;
        ShadowLightShader::pLightData[v13].a = w;
        goto LABEL_27;
    }
LABEL_29:;
    if ((BSShaderManager::bHDR && (v7->ulFlags[0] & BSS1_FaceGen) != 0) && (apRenderPass->ucNumLights) &&
        (!(*apRenderPass->ppSceneLights)->bPointLight)) {
        kDimmed.x = BSShaderLightingProperty::pShaderLightConstants[1].X() * BSShaderManager::fSkinDimmer;
        kDimmed.y = BSShaderLightingProperty::pShaderLightConstants[1].Y() * BSShaderManager::fSkinDimmer;
        kDimmed.z = BSShaderLightingProperty::pShaderLightConstants[1].Z() * BSShaderManager::fSkinDimmer;
        kDimmed.w = BSShaderManager::fSkinDimmer * BSShaderLightingProperty::pShaderLightConstants[1].W();
        BSShaderLightingProperty::pShaderLightConstants[1] = kDimmed;
    }

    if (epass == 587) {
        void* pIterator = nullptr;
        if (BSShaderManager::eRenderMode == 10) {
            ShadowSceneLight* plight_2 = v7->GetFirstActiveLight(pIterator);
            if (!plight_2 || !plight_2->bPointLight) {
                BSShaderLightingProperty::pShaderLightConstants[1].x = 0.5f;
                BSShaderLightingProperty::pShaderLightConstants[1].y = 0.5f;
                BSShaderLightingProperty::pShaderLightConstants[1].z = 0.5f;
                BSShaderLightingProperty::pShaderLightConstants[1].w = 1.0f;
            }
        }
        else {
            ShadowSceneLight* plight = v7->GetFirstActiveLight(pIterator);
            if (!plight || !plight->bPointLight) {
                BSShaderLightingProperty::pShaderLightConstants[1] = BSShaderLightingProperty::pShaderLightConstants[0];
            }
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_EyePos(
    uint32_t epass,
    const NiTransform& aWorld,
    _D3DMATRIX d3dInvWorld
) {
    _D3DVECTOR vEyePos;
    _D3DVECTOR vEyeObjectPos;
    NiPoint3 vEyeDir;
    if (ShadowLightShader::spVertexConstantMapEntries[9]->m_bEnabled ||
        ShadowLightShader::spPixelConstantMapEntries[5]->m_bEnabled) {
        if (epass < BSShaderManager::BSSM_ENVMAP || epass > BSShaderManager::BSSM_2x_ENVMAP_EYE) {
            vEyePos.x = BSShaderLightingProperty::pShaderLightConstants[30].X();
            vEyePos.y = BSShaderLightingProperty::pShaderLightConstants[30].Y();
            vEyePos.z = BSShaderLightingProperty::pShaderLightConstants[30].Z();
            D3DXVec3TransformCoord_Thunk(&vEyeObjectPos, &vEyePos, &d3dInvWorld);
            ShadowLightShader::pEyePosition.x = vEyeObjectPos.x;
            ShadowLightShader::pEyePosition.y = vEyeObjectPos.y;
            ShadowLightShader::pEyePosition.z = vEyeObjectPos.z;
            ShadowLightShader::pEyePosition.w = aWorld.m_fScale;
        }
        else {
            d3dInvWorld._11 = BSShaderLightingProperty::pShaderLightConstants[30].X();
            d3dInvWorld._12 = BSShaderLightingProperty::pShaderLightConstants[30].Y();
            d3dInvWorld._13 = BSShaderLightingProperty::pShaderLightConstants[30].Z();
            vEyeDir.x = BSShaderLightingProperty::pShaderLightConstants[30].X() - aWorld.m_Translate.x;
            vEyeDir.y = BSShaderLightingProperty::pShaderLightConstants[30].Y() - aWorld.m_Translate.y;
            vEyeDir.z = BSShaderLightingProperty::pShaderLightConstants[30].Z() - aWorld.m_Translate.z;
            vEyeDir.Unitize();
            ShadowLightShader::pEyeVector.x = vEyeDir.x;
            ShadowLightShader::pEyeVector.y = vEyeDir.y;
            ShadowLightShader::pEyeVector.z = vEyeDir.z;
            ShadowLightShader::pEyeVector.w = 0.0f;
            d3dInvWorld._11 = d3dInvWorld._11 - NiRenderer::PosAdjust.x;
            d3dInvWorld._12 = d3dInvWorld._12 - NiRenderer::PosAdjust.y;
            d3dInvWorld._13 = d3dInvWorld._13 - NiRenderer::PosAdjust.z;
            ShadowLightShader::pEyePosition.x = d3dInvWorld._11;
            ShadowLightShader::pEyePosition.y = d3dInvWorld._12;
            ShadowLightShader::pEyePosition.z = d3dInvWorld._13;
            ShadowLightShader::pEyePosition.w = 1.0f;
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_SkinToCube(const NiSkinInstance* apSkin) {
    alignas(16) D3DMATRIX kMatrix;
    if ((ShadowLightShader::spVertexConstantMapEntries[12]->m_bEnabled) && (apSkin)) {
        kMatrix._43 = 0.0f;
        kMatrix._42 = 0.0f;
        kMatrix._41 = 0.0f;
        kMatrix._34 = 0.0f;
        kMatrix._32 = 0.0f;
        kMatrix._31 = 0.0f;
        kMatrix._24 = 0.0f;
        kMatrix._23 = 0.0f;
        kMatrix._21 = 0.0f;
        kMatrix._14 = 0.0f;
        kMatrix._13 = 0.0f;
        kMatrix._12 = 0.0f;
        kMatrix._44 = 1.0f;
        kMatrix._33 = 1.0f;
        kMatrix._22 = 1.0f;
        kMatrix._11 = 1.0f;
        rt::memcpy(ShadowLightShader::kSkinWorldTranspose, &kMatrix, sizeof(kMatrix));
    }
}

void ShadowLightShader::SetupGeometryConstants_RefractionTime(BSShaderPPLightingProperty* pprop) {
    if (ShadowLightShader::spVertexConstantMapEntries[15]->m_bEnabled) {
        ShadowLightShader::fConstantfRefractionTime = BSShaderManager::fTimerValues[0] / 3600.0 * 6.283180236816406 *
            static_cast<double>(pprop->iRefractionFirePeriod);
    }
}

void ShadowLightShader::SetupGeometryConstants_HighDetailRange(NiGeometry* apGeometry) {
    if (ShadowLightShader::spVertexConstantMapEntries[16]->m_bEnabled) {
        NiPoint4 kRange;
        kRange.x = BSShaderManager::kLoadedRange.x - NiRenderer::PosAdjust.x;
        kRange.y = BSShaderManager::kLoadedRange.y - NiRenderer::PosAdjust.y;
        kRange.z = BSShaderManager::kLoadedRange.z - 15.0;
        kRange.w = BSShaderManager::kLoadedRange.w - 15.0;
        // Register 34 of the lighting shaders' shared constants.
        BSShaderLightingProperty::pShaderLightConstants[34] = kRange;
    }
}

void ShadowLightShader::SetupGeometryConstants_LODLandParams(
    uint32_t epass,
    BSShaderPPLightingProperty* pprop,
    NiGeometry* pgeometry
) {
    if (ShadowLightShader::spVertexConstantMapEntries[21]->m_bEnabled) {
        switch (epass) {
            case 254u: {
                BSShaderLightingProperty::kLODLandParams.x = pprop->fGeomorphParam;
                BSShaderLightingProperty::kLODLandParams.y = BSShaderManager::fLODLandDrop;
                BSShaderLightingProperty::kLODLandParams.z = 0.0;
                BSShaderLightingProperty::kLODLandParams.w = 0.0;
                break;
            }
            case 97u: {
                BSShaderLightingProperty::kLODLandParams.x = 1.0;
                BSShaderLightingProperty::kLODLandParams.y = 0.0;
                BSShaderLightingProperty::kLODLandParams.z = 0.0;
                BSShaderLightingProperty::kLODLandParams.w = 0.0;
                break;
            }
            case 560u: {
                NiPoint3 kPos = pgeometry->m_kWorld.m_Translate;
                float fTime = (BSShaderManager::fTimerValues[0] - BSShaderManager::fGriddArrayLerpStart) /
                    (BSShaderManager::fLandLOFadeSeconds * 5.0);
                fTime = BSMAX(static_cast<float>(0.0), fTime);
                fTime = BSMIN(static_cast<float>(1.0), fTime);
                if (1.0 == fTime) {
                    BSShaderManager::bLODFadeInProgress = false;
                }
                BSShaderLightingProperty::kLODLandParams.x = pprop->LandBlendParamsA.X();
                BSShaderLightingProperty::kLODLandParams.y = pprop->LandBlendParamsA.Y();
                BSShaderLightingProperty::kLODLandParams.z = pprop->LandBlendParamsA.Z();
                float w = pprop->LandBlendParamsA.W();
                float fDiffX = BSShaderManager::kGridArrayCenter.x - BSShaderManager::kOldGridArrayCenter.x;
                float v10 = fDiffX * fTime;
                v10 = v10 + BSShaderManager::kOldGridArrayCenter.x;
                BSShaderLightingProperty::kLODLandParams.z = v10 - kPos.x;
                float fDiffY = BSShaderManager::kGridArrayCenter.y - BSShaderManager::kOldGridArrayCenter.y;
                float v11 = fDiffY * fTime;
                v11 = v11 + BSShaderManager::kOldGridArrayCenter.y;
                BSShaderLightingProperty::kLODLandParams.w = v11 - kPos.y;
                break;
            }
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_LandTextureMask(
    uint32_t epass,
    BSShaderProperty::RenderPass* apRenderPass
) {
    int32_t v3;
    int32_t v4;
    if (epass > 501) {
        double v8;
        double v9;
        double v10;
        double v12;
        double v13;
        double v15;
        double v16;
        double v19;
        double v20;
        double v22;
        double v23;
        float blue;
        float bluea;
        float alpha;
        float alphaa;
        float aeRenderPassType;
        float apRenderPassa;
        float apRenderPassb;
        if (epass != 502) {
            return;
        }
        v3 = 0;
        v4 = 27;
    LABEL_9:;
        int32_t ucExtraParam = apRenderPass->ucExtraParam;
        double v7 = 0.0;
        if (ucExtraParam == 4) {
            v8 = 1.0;
            v9 = 0.0;
            alpha = 1.0;
        }
        else {
            alpha = 0.0;
            if (ucExtraParam == 3) {
                blue = 1.0;
                v10 = 1.0;
                goto LABEL_13;
            }
            v8 = 1.0;
            v9 = 0.0;
        }
        v7 = v9;
        v10 = v8;
        blue = v7;
        if (ucExtraParam == 2) {
            v12 = v10;
            v13 = v7;
            apRenderPassa = v12;
            goto LABEL_22;
        }
    LABEL_13:;
        apRenderPassa = v7;
        v12 = v10;
        v13 = v7;
        if (ucExtraParam == 1) {
            aeRenderPassType = v12;
            v15 = v13;
            v16 = v12;
            goto LABEL_15;
        }
    LABEL_22:;
        v15 = v13;
        v16 = v12;
        aeRenderPassType = v15;
    LABEL_15:;
        NiPoint4* v17 = &BSShaderLightingProperty::pShaderLightConstants[v3];
        v17->x = aeRenderPassType;
        v17->y = apRenderPassa;
        v17->z = blue;
        v17->w = alpha;
        if (ucExtraParam == 8) {
            v19 = v16;
            v20 = v15;
            bluea = v19;
        }
        else {
            bluea = v15;
            v19 = v16;
            v20 = v15;
            if (ucExtraParam == 7) {
                alphaa = v19;
                v22 = v20;
                v23 = v19;
                goto LABEL_25;
            }
        }
        v22 = v20;
        v23 = v19;
        alphaa = v22;
        if (ucExtraParam == 6) {
            double v25;
            v25 = v23;
            v23 = v22;
            apRenderPassb = v25;
        LABEL_27:;
            float aeRenderPassTypea = v23;
            NiPoint4* v31 = &BSShaderLightingProperty::pShaderLightConstants[v4];
            v31->x = aeRenderPassTypea;
            v31->y = apRenderPassb;
            v31->z = alphaa;
            v31->w = bluea;
            return;
        }
    LABEL_25:;
        apRenderPassb = v22;
        if (ucExtraParam != 5) {
            v23 = v22;
        }
        goto LABEL_27;
    }
    if (epass >= 497 || epass == 147) {
        v3 = 2;
        v4 = 3;
        int32_t v5 = apRenderPass->ucNumLights + 2;
        if (v5 <= 3) {
            v5 = 3;
        }
        ShadowLightShader::spPixelConstantMapEntries[2]->m_uiRegisterCount = v5;
        goto LABEL_9;
    }
}

void ShadowLightShader::SetupGeometryConstants_Toggles(
    uint32_t epass,
    NiGeometry* pgeometry,
    BSShaderPPLightingProperty* pprop,
    NiMaterialProperty* pmaterialProperty,
    BSShaderProperty::RenderPass* apRenderPass,
    NiAlphaProperty* palphaProp
) {
    if (ShadowLightShader::spPixelConstantMapEntries[4]->m_bEnabled) {
        if (epass - 503 > 55) {
            // Slot 85 (+0x154), through the vtable.
            UpdateTogglesConstant(
                pgeometry, BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph), pmaterialProperty
            );
        }
        else {
            UpdateTogglesConstantLand(pgeometry, nullptr, pprop, 0);
        }
        double v10 = 0.0;
        if ((apRenderPass) && (!apRenderPass->bLastPass)) {
            BSShaderLightingProperty::pShaderLightConstants[33].y = 0.0f;
        }

        if ((pprop->ulFlags[0] & BSS1_Landscape) != 0 || (epass >= 584 && epass <= 587)) {
            if (epass - 584 <= 3) {
                double v12;
                if (pprop->GetTexture(BSShaderProperty::TEXTYPE_ENV_MASK, 0)) {
                    v12 = 1.0;
                }
                else {
                    v12 = 0.0;
                }
                BSShaderLightingProperty::pShaderLightConstants[33].w = v12;
                BSShaderLightingProperty::pShaderLightConstants[33].z = pprop->fEnvmapScale;
            }
        }
        else {
            if ((palphaProp->m_uFlags & 512) != 0) {
                float apGeometry = static_cast<float>(palphaProp->m_ucAlphaTestRef);
                double v11 = apGeometry;
                if (apGeometry < 1.0) {
                    v11 = 1.0;
                }
                float apGeometrya = v11;
                BSShaderLightingProperty::pShaderLightConstants[33].w = apGeometrya / 255.0;
            }
            else {
                if ((palphaProp->m_uFlags & 1) != 0) {
                    v10 = 0.00390625f;
                }
                BSShaderLightingProperty::pShaderLightConstants[33].w = v10;
            }
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_LODTexParams(BSShaderPPLightingProperty* pprop) {
    if (ShadowLightShader::spPixelConstantMapEntries[20u]->m_bEnabled) {
        float fTerrainTexOffsetY = pprop->fTerrainTexOffsetY;
        float fFadeTime = pprop->fTerrainTexFade;
        double fTerrainDetailTexScale = pprop->fTerrainDetailTexScale;
        BSShaderLightingProperty::pShaderLightConstants[36].x = pprop->fTerrainTexOffsetX;
        float fDetailTextureScale = fTerrainDetailTexScale;
        BSShaderLightingProperty::pShaderLightConstants[36].y = fTerrainTexOffsetY;
        BSShaderLightingProperty::pShaderLightConstants[36].z = fDetailTextureScale;
        BSShaderLightingProperty::pShaderLightConstants[36].w = fFadeTime;
    }
}

void ShadowLightShader::SetupGeometryConstants_ShadowProj(
    const NiPropertyState* apState,
    const NiSkinInstance* apSkin,
    const NiTransform& aWorld
) {
    D3DMATRIX v6;
    if (ShadowLightShader::spVertexConstantMapEntries[3]->m_bEnabled) {
        BSShaderPPLightingProperty* m_pObject =
            static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
        if ((m_pObject) && (m_pObject->iShaderPropertyType != -1) &&
            (BSShaderManager::GetAccumulator()->pActiveShadowSceneNode)) {
            double m_fScale;
            if (apSkin) {
                v6._11 = 1.0;
                v6._12 = 0.0;
                v6._13 = 0.0;
                v6._14 = NiRenderer::PosAdjust.x;
                v6._21 = 0.0;
                v6._23 = 0.0;
                v6._22 = 1.0;
                v6._24 = NiRenderer::PosAdjust.y;
                v6._31 = 0.0;
                v6._32 = 0.0;
                v6._33 = 1.0;
                v6._34 = NiRenderer::PosAdjust.z;
                m_fScale = 1.0;
                v6._41 = 0.0;
                v6._42 = 0.0;
                v6._43 = 0.0;
            }
            else {
                v6._11 = aWorld.m_Rotate.m_pEntry[0][0];
                v6._12 = aWorld.m_Rotate.m_pEntry[0][1];
                v6._13 = aWorld.m_Rotate.m_pEntry[0][2];
                v6._14 = aWorld.m_Translate.x;
                v6._21 = aWorld.m_Rotate.m_pEntry[1][0];
                v6._22 = aWorld.m_Rotate.m_pEntry[1][1];
                v6._23 = aWorld.m_Rotate.m_pEntry[1][2];
                v6._24 = aWorld.m_Translate.y;
                v6._31 = aWorld.m_Rotate.m_pEntry[2][0];
                v6._32 = aWorld.m_Rotate.m_pEntry[2][1];
                v6._33 = aWorld.m_Rotate.m_pEntry[2][2];
                v6._34 = aWorld.m_Translate.z;
                v6._41 = 0.0;
                v6._42 = 0.0;
                v6._43 = 0.0;
                m_fScale = aWorld.m_fScale;
            }
            v6._44 = m_fScale;
            ShadowLightShader::ShadowProjMatrix = v6;
        }
    }
}

void ShadowLightShader::SetupGeometryConstants_AmbientEmittance(
    BSShaderPPLightingProperty* pprop,
    BSShaderProperty::RenderPass* apRenderPass,
    NiGeometry* pgeometry,
    NiMaterialProperty* pmat
) {
    NiPoint4& kAmbient = BSShaderLightingProperty::pShaderLightConstants[0];
    kAmbient.w = pprop->fAlpha;
    // Loads the current render pass once, before the emittance calls.
    BSShaderProperty::RenderPass* pCurrentPass = BSShaderManager::pCurrentRenderPass;
    uint16_t usPassEnum;
    if (ShadowLightShader::spPixelConstantMapEntries[3]->m_bEnabled &&
        ((usPassEnum = apRenderPass->usPassEnum) < BSShaderManager::BSSM_ADT10 ||
         usPassEnum > BSShaderManager::BSSM_ADTS10_SFg)) {
        SetEmitConstant(pmat);
    }
    else {
        AddEmitToAmbient(pmat);
    }
    usPassEnum = apRenderPass->usPassEnum;
    if (usPassEnum >= BSShaderManager::BSSM_ADT10 && usPassEnum <= BSShaderManager::BSSM_ADTS10_SFg) {
        ShadowLightShader::kFarNear.z = static_cast<float>(apRenderPass->ucNumLights);
        BSShaderLightingProperty::pShaderLightConstants[27].w = static_cast<float>(apRenderPass->ucNumLights);
    }
    if (BSShaderManager::bLiteBrite) {
        kAmbient.x = 1.0f;
        kAmbient.y = 1.0f;
        kAmbient.z = 1.0f;
        kAmbient.w = 1.0f;
    }
    if ((pprop->ulFlags[1] & BSS2_Envmap_Light_Fade) != 0 && (pprop->ulFlags[0] & BSS1_Eye_Environment_Mapping) == 0) {
        usPassEnum = pCurrentPass->usPassEnum;
        if (usPassEnum >= BSShaderManager::BSSM_ENVMAP && usPassEnum <= BSShaderManager::BSSM_2x_ENVMAP_EYE) {
            float fLightLuminance = 0.0f;
            void* pIterator = nullptr;
            ShadowSceneLight* psslight = pprop->GetFirstActiveNonShadowLight(pIterator);
            NiColor ksunColor = BSShaderManager::pShadowSceneNode[0]->pSunLight->spLight.m_pObject->m_kDiff;
            // EXE-INLINED 0xB719E0
            float fSunLuminance =
                ksunColor.g * 0.36000001430511475 + ksunColor.r * 0.3100000023841858 + ksunColor.b * 0.3199999928474426;
            if (psslight) {
                const NiBound* pBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
                if (!pBound) {
                    pBound = &NiAVObject::NullBoundS;
                }
                NiLight* pLight = psslight->spLight.m_pObject;
                NiPoint3 vlight = pLight->m_kWorld.m_Translate - pBound->m_kCenter;
                NiColor kdiffColor = pLight->m_kDiff * pLight->m_fDimmer;
                float fRatio = vlight.Length() / pLight->m_kSpec.r;
                float fClamped = fMin_(1.0, BSMAX_0(0.0, fRatio));
                float fFalloff = 1.0 - fClamped;
                fLightLuminance = kdiffColor.Luminance() * fFalloff;
            }
            float fLuminance = fLightLuminance;
            if (!(fSunLuminance < fLightLuminance)) {
                fLuminance = fSunLuminance;
            }
            kAmbient.w = fLuminance * kAmbient.w;
        }
    }
    if (BSShaderManager::eRenderMode == BSShaderManager::BSSM_RENDER_CONSTALPHA) {
        float fAlpha = (pprop->ulFlags[1] & BSS2_VATS_Selection) != 0 ? 0.0f : 1.0f;
        NiAlphaProperty* pAlphaProp =
            static_cast<NiAlphaProperty*>(pgeometry->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
        float fBlue = 1.0f;
        if ((pAlphaProp->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0 ||
            (pAlphaProp->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK) != 0) {
            fBlue = 0.0f;
        }
        kAmbient.x = 0.0f;
        kAmbient.y = 0.0f;
        kAmbient.z = fBlue;
        kAmbient.w = fAlpha;
    }
    if (BSShaderManager::eCurrentPass >= BSShaderManager::BSSM_ENVMAP &&
        BSShaderManager::eCurrentPass <= BSShaderManager::BSSM_2x_ENVMAP_EYE &&
        BSShaderManager::eRenderMode != BSShaderManager::BSSM_RENDER_INTERFACE) {
        kAmbient.w = pprop->GetEnvMapLODFade() * kAmbient.w;
    }
}

void ShadowLightShader::SetLODNoiseMap(int32_t auiTexturePass) {
    NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[auiTexturePass].m_pObject;
    NiPointer<NiSourceTexture>* v3 = &BSShaderManager::pNoiseMap;
    if (!BSShaderManager::bUseNoiseMap) {
        v3 = &BSShaderManager::pEmptyNoiseMap;
    }
    m_pObject->m_spTexture = v3->m_pObject;
    if (BSShaderManager::bLODNoiseAniso) {
        m_pObject->SetFilterMode(NiTexturingProperty::FILTER_MAGLERP_MINANISO);
    }
    // Passes the float's bits as the sampler state's value.
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)
        ->m_pkD3DDevice9->SetSamplerState(
            auiTexturePass, D3DSAMP_MIPMAPLODBIAS, std::bit_cast<uint32_t>(BSShaderManager::fLODNoiseMipBias)
        );
}

void ShadowLightShader::SetParallaxTexture(BSShaderPPLightingProperty* a1, int32_t passStage, uint32_t a3, char a4) {
    NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[passStage].m_pObject;
    if (a1->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, a3)) {
        m_pObject->m_spTexture = a1->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, a3);
    }
    else {
        m_pObject->m_spTexture = a4 != 0 ? BSShaderManager::spDefaultHeightMap.m_pObject : nullptr;
    }
}

void ShadowLightShader::SetupGeometryOpt_Lights(_D3DMATRIX& aInvworld, float afWorldScale) {
    _D3DVECTOR vLightPos;
    _D3DVECTOR vLightObjectPos;
    NiPoint4 v32;
    BSShaderProperty::RenderPass* v3 = BSShaderManager::pCurrentRenderPass;
    BSShaderProperty::RenderPass* pRenderPass = BSShaderManager::pCurrentRenderPass;
    float v15 = static_cast<double>(BSShaderManager::pCurrentRenderPass->ucNumLights) - 1.0;
    v32.x = 9.0f;
    v32.y = 9.0f;
    v32.z = 9.0f;
    v32.w = v15;
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
        25, &(&v32)->x, 1
    );
    ShadowLightShader::spPixelConstantMapEntries[2]->m_uiRegisterCount = v3->ucNumLights - 1;
    if (ShadowLightShader::spPixelConstantMapEntries[7]->m_bEnabled) {
        ShadowLightShader::spPixelConstantMapEntries[7]->m_uiRegisterCount = v3->ucNumLights - 1;
    }
    uint8_t ucNumLights = v3->ucNumLights;
    uint8_t i = 1;
    if (ucNumLights > 1u) {
        float v16 = static_cast<double>(ucNumLights) - 1.0;
        while (true) {
            NiColor v30;
            ShadowSceneLight* psslLight = v3->ppSceneLights[i];
            NiLight* m_pObject = psslLight->spLight.m_pObject;
            vLightPos.x = m_pObject->m_kWorld.m_Translate.x;
            vLightPos.y = m_pObject->m_kWorld.m_Translate.y;
            vLightPos.z = m_pObject->m_kWorld.m_Translate.z;
            float y = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y;
            float z = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z;
            float x = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x;
            float v27 = y;
            float v28 = z;
            vLightPos.x = vLightPos.x + x;
            vLightPos.y = vLightPos.y + y;
            vLightPos.z = vLightPos.z + z;
            D3DXVec3TransformCoord_Thunk(&vLightObjectPos, &vLightPos, (&aInvworld));
            NiD3DPass* v9 = NiD3DShader::m_spCurrentPass;
            int32_t i_2 = i;
            float v17 = m_pObject->m_kSpec.r / afWorldScale;
            ShadowLightShader::pLightData[i_2].r = vLightObjectPos.x;
            ShadowLightShader::pLightData[i_2].g = vLightObjectPos.y;
            ShadowLightShader::pLightData[i_2].b = vLightObjectPos.z;
            ShadowLightShader::pLightData[i_2].a = v17;
            v9->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
                i + 25, &ShadowLightShader::pLightData[i_2].r, 1
            );
            double m_fDimmer = m_pObject->m_fDimmer;
            if (!BSShaderManager::bHDR && m_fDimmer > 1.0) {
                m_fDimmer = 1.0f;
            }
            float kDimmedLight = m_pObject->m_kDiff.r * m_fDimmer;
            float kDimmedLight_4 = m_pObject->m_kDiff.g * m_fDimmer;
            float kDimmedLight_8 = m_fDimmer * m_pObject->m_kDiff.b;
            double fLODDimmer = psslLight->fLODDimmer;
            float kLightColor = kDimmedLight * fLODDimmer;
            float kLightColor_4 = fLODDimmer * kDimmedLight_4;
            float kLightColor_8 = fLODDimmer * kDimmedLight_8;
            v30.r = kLightColor;
            BSShaderLightingProperty::pShaderLightConstants[i].x = kLightColor;
            v30.g = kLightColor_4;
            BSShaderLightingProperty::pShaderLightConstants[i].y = kLightColor_4;
            v30.b = kLightColor_8;
            BSShaderLightingProperty::pShaderLightConstants[i].z = kLightColor_8;
            BSShaderLightingProperty::pShaderLightConstants[i++].w = v16;
            if (i >= pRenderPass->ucNumLights) {
                break;
            }
            v3 = pRenderPass;
        }
    }
}

void ShadowLightShader::SetupGeometryOpt_LightsSpecular(
    _D3DMATRIX& aInvworld,
    float afWorldScale,
    NiMaterialProperty* apMat
) {
    double m_fShine;
    _D3DVECTOR vLightPos;
    _D3DVECTOR vLightObjectPos;
    NiVector4 v34;
    BSShaderProperty::RenderPass* v4 = BSShaderManager::pCurrentRenderPass;
    BSShaderProperty::RenderPass* v17 = BSShaderManager::pCurrentRenderPass;
    // The shade property slot holds the BSShaderPPLightingProperty.
    float fSpecFadea = static_cast<BSShaderPPLightingProperty*>(
                           BSShaderManager::pCurrentRenderPass->pGeometry->m_PropertyState.m_aspProps[3].m_pObject
    )
                           ->GetSpecularLODFade();
    v34.x = 99.0f;
    v34.y = 99.0f;
    v34.z = 99.0f;
    v34.w = fSpecFadea;
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
        25, &(&v34)->x, 1
    );
    int32_t v5 = v4->ucNumLights - 1;
    if (v5 < 2) {
        v5 = 2;
    }
    ShadowLightShader::spPixelConstantMapEntries[2]->m_uiRegisterCount = v5;
    if (ShadowLightShader::spPixelConstantMapEntries[7]->m_bEnabled) {
        ShadowLightShader::spPixelConstantMapEntries[7]->m_uiRegisterCount = v4->ucNumLights - 1;
    }
    if (apMat) {
        m_fShine = apMat->m_fShine;
    }
    else {
        m_fShine = 30.0f;
    }
    uint8_t ucNumLights = v4->ucNumLights;
    float v18 = m_fShine;
    uint8_t i = 1;
    float fSpecFade = static_cast<double>(ucNumLights) - 1.0;
    if (ucNumLights > 1u) {
        do {
            NiColor v32;
            ShadowSceneLight* psslLight = v4->ppSceneLights[i];
            NiLight* m_pObject = psslLight->spLight.m_pObject;
            vLightPos.x = m_pObject->m_kWorld.m_Translate.x;
            vLightPos.y = m_pObject->m_kWorld.m_Translate.y;
            vLightPos.z = m_pObject->m_kWorld.m_Translate.z;
            float y = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y;
            float z = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z;
            float x = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x;
            float v29 = y;
            float v30 = z;
            vLightPos.x = vLightPos.x + x;
            vLightPos.y = vLightPos.y + y;
            vLightPos.z = vLightPos.z + z;
            D3DXVec3TransformCoord_Thunk(&vLightObjectPos, &vLightPos, (&aInvworld));
            NiD3DPass* v12 = NiD3DShader::m_spCurrentPass;
            int32_t i_2 = i;
            float fRadius = m_pObject->m_kSpec.r / afWorldScale;
            ShadowLightShader::pLightData[i_2].r = vLightObjectPos.x;
            ShadowLightShader::pLightData[i_2].g = vLightObjectPos.y;
            ShadowLightShader::pLightData[i_2].b = vLightObjectPos.z;
            ShadowLightShader::pLightData[i_2].a = fRadius;
            v12->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
                i + 25, &ShadowLightShader::pLightData[i_2].r, 1
            );
            double m_fDimmer = m_pObject->m_fDimmer;
            if (!BSShaderManager::bHDR && m_fDimmer > 1.0) {
                m_fDimmer = 1.0f;
            }
            float kDimmedLight = m_pObject->m_kDiff.r * m_fDimmer;
            float kDimmedLight_4 = m_pObject->m_kDiff.g * m_fDimmer;
            float kDimmedLight_8 = m_fDimmer * m_pObject->m_kDiff.b;
            double fLODDimmer = psslLight->fLODDimmer;
            float kLightColor = kDimmedLight * fLODDimmer;
            float kLightColor_4 = fLODDimmer * kDimmedLight_4;
            float kLightColor_8 = fLODDimmer * kDimmedLight_8;
            v32.r = kLightColor;
            BSShaderLightingProperty::pShaderLightConstants[i].x = kLightColor;
            v32.g = kLightColor_4;
            BSShaderLightingProperty::pShaderLightConstants[i].y = kLightColor_4;
            v32.b = kLightColor_8;
            BSShaderLightingProperty::pShaderLightConstants[i].z = kLightColor_8;
            float v33 = fSpecFade;
            fSpecFade = v18;
            BSShaderLightingProperty::pShaderLightConstants[i++].w = v33;
            v4 = v17;
        }
        while (i < v17->ucNumLights);
    }
    if (v4->ucNumLights == 2) {
        BSShaderLightingProperty::pShaderLightConstants[2].x = 0.0f;
        BSShaderLightingProperty::pShaderLightConstants[2].y = 0.0f;
        BSShaderLightingProperty::pShaderLightConstants[2].z = 0.0f;
        BSShaderLightingProperty::pShaderLightConstants[2].w = v18;
    }
}

void ShadowLightShader::SetupPass_ADT_Opt() {
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[5].m_pObject->ConfigureStage(false);
    SetupPassOpt_FogConstants();
    ShadowLightShader::spVertexConstantMapEntries[6]->m_uiRegisterCount = 1;
    ShadowLightShader::spPixelConstantMapEntries[2]->m_uiRegisterCount = 1;
    SetupPassOpt_PreparePixelShaderConstants();
    SetupPassOpt_AmbientLight();
    SetupPassOpt_LightColor();
    NiD3DShaderConstantMap* m_pObject = m_spPixelConstantMap.m_pObject;
    if ((m_pObject) && (NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject)) {
        NiBound v3;
        NiTransform v4;
        m_pObject->SetShaderConstants(
            NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, v4, v3, 0, true
        );
    }
}

void ShadowLightShader::Setup_ADTS10_Opt(NiPropertyState* a2) {
    NiBound pOut;
    _D3DMATRIX pM;
    BSShaderPPLightingProperty* m_pObject = static_cast<BSShaderPPLightingProperty*>(a2->m_aspProps[3].m_pObject);
    NiD3DTextureStage* v4 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
    v4->m_spTexture = m_pObject->spTextures[0]->m_pObject;
    NiTexturingProperty::ClampMode v5 = m_pObject->GetClampNI();
    v4->SetClampMode(v5);
    NiD3DTextureStage* v6 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
    v6->m_spTexture = (NiTexture*)BSShaderPPLightingProperty::GetNormalMap(m_pObject, 0);
    NiTexturingProperty::ClampMode v7 = m_pObject->GetClampNI();
    v6->SetClampMode(v7);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->ConfigureStage(false);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject->ConfigureStage(false);
    BSShaderProperty::RenderPass* v8 = BSShaderManager::pCurrentRenderPass;
    NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSUtilities::GetInverseWorldMatrix(BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld, nullptr, pM);
    SetupGeometryOpt_WorldViewProj(pGeometry, pGeometry->m_kWorld);
    SetupGeometryOpt_LightsSpecular(
        pM, pGeometry->m_kWorld.m_fScale, static_cast<NiMaterialProperty*>(a2->m_aspProps[2].m_pObject)
    );
    float v11 = static_cast<double>(v8->ucNumLights) - 1.0;
    // The eye position (shared constant 30) in object space, into the bound's centre: D3DX is handed the xyz of both
    // objects in place.
    D3DXVec3TransformCoord_Thunk(
        reinterpret_cast<D3DXVECTOR3*>(&pOut.m_kCenter),
        reinterpret_cast<const D3DXVECTOR3*>(&BSShaderLightingProperty::pShaderLightConstants[30]), &pM
    );
    pOut.m_fRadius = v11;
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(
        16, &(&pOut)->m_kCenter.x, 1
    );
    NiD3DShaderConstantMap* v10 = m_spPixelConstantMap.m_pObject;
    if ((v10) && (NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject)) {
        NiTransform v14;
        v10->SetShaderConstants(
            NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, v14, pOut, 0, true
        );
    }
}

void ShadowLightShader::Setup_ADT_PxOpt(NiPropertyState* apProperties) {
    // The eye position in object space and the world scale: one four-float vertex shader constant.
    NiPoint4 pOut;
    _D3DMATRIX pM;
    BSShaderPPLightingProperty* m_pObject =
        static_cast<BSShaderPPLightingProperty*>(apProperties->m_aspProps[3].m_pObject);
    NiD3DTextureStage* v4 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
    v4->m_spTexture = m_pObject->spTextures[0]->m_pObject;
    NiTexturingProperty::ClampMode v5 = m_pObject->GetClampNI();
    v4->SetClampMode(v5);
    NiD3DTextureStage* v6 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
    v6->m_spTexture = (NiTexture*)BSShaderPPLightingProperty::GetNormalMap(m_pObject, 0);
    NiTexturingProperty::ClampMode v7 = m_pObject->GetClampNI();
    v6->SetClampMode(v7);
    NiD3DTextureStage* v8 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[3].m_pObject;
    if (m_pObject->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, 0)) {
        v8->m_spTexture = m_pObject->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, 0);
    }
    else {
        v8->m_spTexture = (NiTexture*)BSShaderManager::spDefaultHeightMap;
    }
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->ConfigureStage(false);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject->ConfigureStage(false);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[3].m_pObject->ConfigureStage(false);
    NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSUtilities::GetInverseWorldMatrix(BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld, nullptr, pM);
    SetupGeometryOpt_WorldViewProj(pGeometry, pGeometry->m_kWorld);
    SetLightDirection(&pM);
    float apPropertiesa = pGeometry->m_kWorld.m_fScale;
    // Hands D3DX the xyz of both objects in place.
    D3DXVec3TransformCoord_Thunk(
        reinterpret_cast<D3DXVECTOR3*>(&pOut),
        reinterpret_cast<const D3DXVECTOR3*>(&BSShaderLightingProperty::pShaderLightConstants[30]), &pM
    );
    pOut.w = apPropertiesa;
    NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->m_pkD3DDevice->SetVertexShaderConstantF(16, &pOut.x, 1);
}

void ShadowLightShader::PresetAmbientPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v69 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[9].m_pObject) {
            m_pObject = ShadowLightShader::spPass[9].m_pObject;
            v69 = ShadowLightShader::spPass[9].m_pObject;
            ++ShadowLightShader::spPass[9].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[0];
        if (v2 != ShadowLightShader::spVertexShaders[0]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(22u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[9] = 2048;
        ShadowLightShader::ulPixelConstantFlags[9] = 4;
        if (m_pObject != ShadowLightShader::spPass[88].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[88].m_pObject;
            v69 = ShadowLightShader::spPass[88].m_pObject;
            if (ShadowLightShader::spPass[88].m_pObject) {
                ++ShadowLightShader::spPass[88u].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[0];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[88] = 0;
        ShadowLightShader::ulPixelConstantFlags[88] = 4;
        if (m_pObject != ShadowLightShader::spPass[97].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[97].m_pObject;
            v69 = ShadowLightShader::spPass[97].m_pObject;
            if (ShadowLightShader::spPass[97].m_pObject) {
                ++ShadowLightShader::spPass[97u].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[55];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[197];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[97] = 0x420304;
        ShadowLightShader::ulPixelConstantFlags[97] = 4;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[97] = 0x20304;
        if (m_pObject != ShadowLightShader::spPass[98].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[98].m_pObject;
            v69 = ShadowLightShader::spPass[98].m_pObject;
            if (ShadowLightShader::spPass[98].m_pObject) {
                ++ShadowLightShader::spPass[98].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[17];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[198];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[98] = 768;
        ShadowLightShader::ulPixelConstantFlags[98] = 4;
        if (m_pObject != ShadowLightShader::spPass[99].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[99].m_pObject;
            v69 = ShadowLightShader::spPass[99].m_pObject;
            if (ShadowLightShader::spPass[99].m_pObject) {
                ++ShadowLightShader::spPass[99].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[155];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[199];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[99] = 768;
        ShadowLightShader::ulPixelConstantFlags[99] = 4;
        if (m_pObject != ShadowLightShader::spPass[89].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[89].m_pObject;
            v69 = ShadowLightShader::spPass[89].m_pObject;
            if (ShadowLightShader::spPass[89].m_pObject) {
                ++ShadowLightShader::spPass[89].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[24];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[36];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[89] = 0;
        ShadowLightShader::ulPixelConstantFlags[89] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[89] = 4;
        if (m_pObject != ShadowLightShader::spPass[90].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[90].m_pObject;
            v69 = ShadowLightShader::spPass[90].m_pObject;
            if (ShadowLightShader::spPass[90].m_pObject) {
                ++ShadowLightShader::spPass[90].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v27 = ShadowLightShader::spVertexShaders[0];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[90] = 0;
        ShadowLightShader::ulPixelConstantFlags[90] = 4;
        if (m_pObject != ShadowLightShader::spPass[91].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[91].m_pObject;
            v69 = ShadowLightShader::spPass[91].m_pObject;
            if (ShadowLightShader::spPass[91].m_pObject) {
                ++ShadowLightShader::spPass[91].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v31 = ShadowLightShader::spVertexShaders[24];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[36];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[91] = 0;
        ShadowLightShader::ulPixelConstantFlags[91] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[91] = 4;
        if (m_pObject != ShadowLightShader::spPass[92].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[92].m_pObject;
            v69 = ShadowLightShader::spPass[92].m_pObject;
            if (ShadowLightShader::spPass[92].m_pObject) {
                ++ShadowLightShader::spPass[92u].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v35 = ShadowLightShader::spVertexShaders[31];
        m_pObject->m_spVertexShader = v35;
        NiD3DPixelShader* v37 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v37;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[92] = 2048;
        ShadowLightShader::ulPixelConstantFlags[92] = 4;
        if (m_pObject != ShadowLightShader::spPass[93].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[93].m_pObject;
            v69 = ShadowLightShader::spPass[93].m_pObject;
            if (ShadowLightShader::spPass[93].m_pObject) {
                ++ShadowLightShader::spPass[93].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v39 = ShadowLightShader::spVertexShaders[51];
        m_pObject->m_spVertexShader = v39;
        NiD3DPixelShader* v41 = ShadowLightShader::spPixelShaders[36];
        m_pObject->m_spPixelShader = v41;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[93] = 2048;
        ShadowLightShader::ulPixelConstantFlags[93] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[93] = 4;
        if (m_pObject != ShadowLightShader::spPass[94].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[94].m_pObject;
            v69 = ShadowLightShader::spPass[94].m_pObject;
            if (ShadowLightShader::spPass[94].m_pObject) {
                ++ShadowLightShader::spPass[94].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v43 = ShadowLightShader::spVertexShaders[31];
        m_pObject->m_spVertexShader = v43;
        NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v45;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[94] = 2048;
        ShadowLightShader::ulPixelConstantFlags[94] = 4;
        if (m_pObject != ShadowLightShader::spPass[95].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[95].m_pObject;
            v69 = ShadowLightShader::spPass[95].m_pObject;
            if (ShadowLightShader::spPass[95].m_pObject) {
                ++ShadowLightShader::spPass[95].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v47 = ShadowLightShader::spVertexShaders[51];
        m_pObject->m_spVertexShader = v47;
        NiD3DPixelShader* v49 = ShadowLightShader::spPixelShaders[36];
        m_pObject->m_spPixelShader = v49;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[95] = 2048;
        ShadowLightShader::ulPixelConstantFlags[95] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[95] = 4;
        if (m_pObject != ShadowLightShader::spPass[726].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[726].m_pObject;
            v69 = ShadowLightShader::spPass[726].m_pObject;
            if (ShadowLightShader::spPass[726].m_pObject) {
                ++ShadowLightShader::spPass[726].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v51 = ShadowLightShader::spVertexShaders[24];
        m_pObject->m_spVertexShader = v51;
        NiD3DPixelShader* v53 = ShadowLightShader::spPixelShaders[37];
        m_pObject->m_spPixelShader = v53;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[726] = 0;
        ShadowLightShader::ulPixelConstantFlags[726] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[726] = 4;
        if (m_pObject != ShadowLightShader::spPass[727].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[727].m_pObject;
            v69 = ShadowLightShader::spPass[727].m_pObject;
            if (ShadowLightShader::spPass[727].m_pObject) {
                ++ShadowLightShader::spPass[727].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v55 = ShadowLightShader::spVertexShaders[51];
        m_pObject->m_spVertexShader = v55;
        NiD3DPixelShader* v57 = ShadowLightShader::spPixelShaders[37];
        m_pObject->m_spPixelShader = v57;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[727] = 2048;
        ShadowLightShader::ulPixelConstantFlags[727] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[727] = 4;
        if (m_pObject != ShadowLightShader::spPass[20].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[20].m_pObject;
            v69 = ShadowLightShader::spPass[20].m_pObject;
            if (ShadowLightShader::spPass[20].m_pObject) {
                ++ShadowLightShader::spPass[20u].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v59 = ShadowLightShader::spVertexShaders[24];
        m_pObject->m_spVertexShader = v59;
        NiD3DPixelShader* v61 = ShadowLightShader::spPixelShaders[38];
        m_pObject->m_spPixelShader = v61;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[20] = 0;
        ShadowLightShader::ulPixelConstantFlags[20] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[20] = 4;
        if (m_pObject != ShadowLightShader::spPass[21].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[21].m_pObject;
            v69 = ShadowLightShader::spPass[21].m_pObject;
            if (ShadowLightShader::spPass[21].m_pObject) {
                ++ShadowLightShader::spPass[21].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v63 = ShadowLightShader::spVertexShaders[51];
        m_pObject->m_spVertexShader = v63;
        NiD3DPixelShader* v65 = ShadowLightShader::spPixelShaders[38];
        m_pObject->m_spPixelShader = v65;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[21] = 2048;
        ShadowLightShader::ulPixelConstantFlags[21] = 20;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[21] = 4;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetAmbDiffTexPasses_1x() {
    bool v6;
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v67 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (ShadowLightShader::spPass[100].m_pObject) {
            m_pObject = ShadowLightShader::spPass[100].m_pObject;
            v67 = ShadowLightShader::spPass[100].m_pObject;
            ++ShadowLightShader::spPass[100u].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[10];
        if (v2 != ShadowLightShader::spVertexShaders[10]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[9];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_AMBDIFFTEX] = 128;
        ShadowLightShader::ulPixelConstantFlags[100] = 12;
        if (m_pObject != ShadowLightShader::spPass[101].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[101].m_pObject;
            v67 = ShadowLightShader::spPass[101].m_pObject;
            if (ShadowLightShader::spPass[101].m_pObject) {
                ++ShadowLightShader::spPass[101].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[12];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[13];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[101] = 128;
        ShadowLightShader::ulPixelConstantFlags[101] = 12;
        if (m_pObject != ShadowLightShader::spPass[127].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[127].m_pObject;
            v67 = ShadowLightShader::spPass[127].m_pObject;
            if (ShadowLightShader::spPass[127].m_pObject) {
                ++ShadowLightShader::spPass[127].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[11];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[10];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[127] = 896;
        ShadowLightShader::ulPixelConstantFlags[127] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[127] = 768;
        if (m_pObject != ShadowLightShader::spPass[128].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[128].m_pObject;
            v67 = ShadowLightShader::spPass[128].m_pObject;
            if (ShadowLightShader::spPass[128].m_pObject) {
                ++ShadowLightShader::spPass[128].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[13];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[14];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[128] = 896;
        ShadowLightShader::ulPixelConstantFlags[128] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[128] = 768;
        if (m_pObject != ShadowLightShader::spPass[102].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[102].m_pObject;
            v67 = ShadowLightShader::spPass[102].m_pObject;
            if (ShadowLightShader::spPass[102].m_pObject) {
                ++ShadowLightShader::spPass[102].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[10];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[21];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[102] = 128;
        ShadowLightShader::ulPixelConstantFlags[102] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[102] = 4;
        if (m_pObject != ShadowLightShader::spPass[103].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[103].m_pObject;
            v67 = ShadowLightShader::spPass[103].m_pObject;
            if (ShadowLightShader::spPass[103].m_pObject) {
                ++ShadowLightShader::spPass[103].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[12];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[25];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[103] = 128;
        ShadowLightShader::ulPixelConstantFlags[103] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[103] = 4;
        if (m_pObject != ShadowLightShader::spPass[129].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[129].m_pObject;
            v67 = ShadowLightShader::spPass[129].m_pObject;
            if (ShadowLightShader::spPass[129].m_pObject) {
                ++ShadowLightShader::spPass[129].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v27 = ShadowLightShader::spVertexShaders[11];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[22];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[129] = 896;
        ShadowLightShader::ulPixelConstantFlags[129] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[129] = 768;
        if (m_pObject != ShadowLightShader::spPass[130].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[130].m_pObject;
            v67 = ShadowLightShader::spPass[130].m_pObject;
            if (ShadowLightShader::spPass[130].m_pObject) {
                ++ShadowLightShader::spPass[130].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v31 = ShadowLightShader::spVertexShaders[13];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[26];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[130] = 896;
        ShadowLightShader::ulPixelConstantFlags[130] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[130] = 768;
        if (m_pObject != ShadowLightShader::spPass[104].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[104].m_pObject;
            v67 = ShadowLightShader::spPass[104].m_pObject;
            if (ShadowLightShader::spPass[104].m_pObject) {
                ++ShadowLightShader::spPass[104u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v35 = ShadowLightShader::spVertexShaders[14];
        m_pObject->m_spVertexShader = v35;
        NiD3DPixelShader* v37 = ShadowLightShader::spPixelShaders[17];
        m_pObject->m_spPixelShader = v37;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[104] = 128;
        ShadowLightShader::ulPixelConstantFlags[104] = 12;
        if (m_pObject != ShadowLightShader::spPass[105].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[105].m_pObject;
            v67 = ShadowLightShader::spPass[105].m_pObject;
            if (ShadowLightShader::spPass[105].m_pObject) {
                ++ShadowLightShader::spPass[105].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v39 = ShadowLightShader::spVertexShaders[10];
        m_pObject->m_spVertexShader = v39;
        NiD3DPixelShader* v41 = ShadowLightShader::spPixelShaders[11];
        m_pObject->m_spPixelShader = v41;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[105] = 128;
        ShadowLightShader::ulPixelConstantFlags[105] = 12;
        if (m_pObject != ShadowLightShader::spPass[106].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[106].m_pObject;
            v67 = ShadowLightShader::spPass[106].m_pObject;
            if (ShadowLightShader::spPass[106].m_pObject) {
                ++ShadowLightShader::spPass[106].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v43 = ShadowLightShader::spVertexShaders[12];
        m_pObject->m_spVertexShader = v43;
        NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[15];
        m_pObject->m_spPixelShader = v45;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[106] = 128;
        ShadowLightShader::ulPixelConstantFlags[106] = 12;
        if (m_pObject != ShadowLightShader::spPass[131].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[131].m_pObject;
            v67 = ShadowLightShader::spPass[131].m_pObject;
            if (ShadowLightShader::spPass[131].m_pObject) {
                ++ShadowLightShader::spPass[131].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v47 = ShadowLightShader::spVertexShaders[11];
        m_pObject->m_spVertexShader = v47;
        NiD3DPixelShader* v49 = ShadowLightShader::spPixelShaders[12];
        m_pObject->m_spPixelShader = v49;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[131] = 896;
        ShadowLightShader::ulPixelConstantFlags[131] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[131] = 768;
        if (m_pObject != ShadowLightShader::spPass[132].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[132].m_pObject;
            v67 = ShadowLightShader::spPass[132].m_pObject;
            if (ShadowLightShader::spPass[132].m_pObject) {
                ++ShadowLightShader::spPass[132].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v51 = ShadowLightShader::spVertexShaders[13];
        m_pObject->m_spVertexShader = v51;
        NiD3DPixelShader* v53 = ShadowLightShader::spPixelShaders[16];
        m_pObject->m_spPixelShader = v53;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[132] = 896;
        ShadowLightShader::ulPixelConstantFlags[132] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[132] = 768;
        if (m_pObject != ShadowLightShader::spPass[107].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[107].m_pObject;
            v67 = ShadowLightShader::spPass[107].m_pObject;
            if (ShadowLightShader::spPass[107].m_pObject) {
                ++ShadowLightShader::spPass[107].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v55 = ShadowLightShader::spVertexShaders[10];
        m_pObject->m_spVertexShader = v55;
        NiD3DPixelShader* v57 = ShadowLightShader::spPixelShaders[23];
        m_pObject->m_spPixelShader = v57;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[107] = 128;
        ShadowLightShader::ulPixelConstantFlags[107] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[107] = 4;
        if (m_pObject != ShadowLightShader::spPass[108].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[108].m_pObject;
            v67 = ShadowLightShader::spPass[108].m_pObject;
            if (ShadowLightShader::spPass[108].m_pObject) {
                ++ShadowLightShader::spPass[108].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        NiD3DVertexShader* v59 = ShadowLightShader::spVertexShaders[12];
        m_pObject->m_spVertexShader = v59;
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[27]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[108] = 128;
        ShadowLightShader::ulPixelConstantFlags[108] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[108] = 4;
        if (m_pObject != ShadowLightShader::spPass[133].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[133].m_pObject;
            v67 = ShadowLightShader::spPass[133].m_pObject;
            if (ShadowLightShader::spPass[133].m_pObject) {
                ++ShadowLightShader::spPass[133].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[15]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[18]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[133] = 896;
        ShadowLightShader::ulPixelConstantFlags[133] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[133] = 768;
        if (m_pObject != ShadowLightShader::spPass[109].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[109].m_pObject;
            v67 = ShadowLightShader::spPass[109].m_pObject;
            if (ShadowLightShader::spPass[109].m_pObject) {
                ++ShadowLightShader::spPass[109].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[14]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[19]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        NiD3DPass* v61 = ShadowLightShader::spPass[110].m_pObject;
        ShadowLightShader::ulVertexConstantFlags[109] = 128;
        ShadowLightShader::ulPixelConstantFlags[109] = 12;
        if (ShadowLightShader::spPass[110].m_pObject) {
            v6 = ShadowLightShader::spPass[110].m_pObject->m_uiRefCount-- == 1;
            if (v6) {
                v61->ReturnPassToPool();
            }
            ShadowLightShader::spPass[110].m_pObject = nullptr;
        }
        NiD3DPass* v62 = ShadowLightShader::spPass[136].m_pObject;
        if (ShadowLightShader::spPass[136].m_pObject) {
            v6 = ShadowLightShader::spPass[136].m_pObject->m_uiRefCount-- == 1;
            if (v6) {
                v62->ReturnPassToPool();
            }
            ShadowLightShader::spPass[136].m_pObject = nullptr;
        }
        if (m_pObject != ShadowLightShader::spPass[111].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[111].m_pObject;
            v67 = ShadowLightShader::spPass[111].m_pObject;
            if (ShadowLightShader::spPass[111].m_pObject) {
                ++ShadowLightShader::spPass[111].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[41]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[9]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[111] = 2176;
        ShadowLightShader::ulPixelConstantFlags[111] = 12;
        if (m_pObject != ShadowLightShader::spPass[112].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[112].m_pObject;
            v67 = ShadowLightShader::spPass[112].m_pObject;
            if (ShadowLightShader::spPass[112].m_pObject) {
                ++ShadowLightShader::spPass[112].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[42]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[13]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[112] = 2176;
        ShadowLightShader::ulPixelConstantFlags[112] = 12;
        if (m_pObject != ShadowLightShader::spPass[137].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[137].m_pObject;
            v67 = ShadowLightShader::spPass[137].m_pObject;
            if (ShadowLightShader::spPass[137].m_pObject) {
                ++ShadowLightShader::spPass[137].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[43]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[10]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[137] = 2944;
        ShadowLightShader::ulPixelConstantFlags[137] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[137] = 768;
        if (m_pObject != ShadowLightShader::spPass[138].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[138].m_pObject;
            v67 = ShadowLightShader::spPass[138].m_pObject;
            if (ShadowLightShader::spPass[138].m_pObject) {
                ++ShadowLightShader::spPass[138].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[44]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[14]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[138] = 2944;
        ShadowLightShader::ulPixelConstantFlags[138] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[138] = 768;
        if (m_pObject != ShadowLightShader::spPass[113].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[113].m_pObject;
            v67 = ShadowLightShader::spPass[113].m_pObject;
            if (ShadowLightShader::spPass[113].m_pObject) {
                ++ShadowLightShader::spPass[113].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[41]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[21]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[113] = 2176;
        ShadowLightShader::ulPixelConstantFlags[113] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[113] = 4;
        if (m_pObject != ShadowLightShader::spPass[139].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[139].m_pObject;
            v67 = ShadowLightShader::spPass[139].m_pObject;
            if (ShadowLightShader::spPass[139].m_pObject) {
                ++ShadowLightShader::spPass[139].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[43]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[22]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[139] = 2944;
        ShadowLightShader::ulPixelConstantFlags[139] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[139] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[139] = 4;
        if (m_pObject != ShadowLightShader::spPass[140].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[140].m_pObject;
            v67 = ShadowLightShader::spPass[140].m_pObject;
            if (ShadowLightShader::spPass[140].m_pObject) {
                ++ShadowLightShader::spPass[140].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[44]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[26]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[139] = 2944;
        ShadowLightShader::ulPixelConstantFlags[139] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[139] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[139] = 4;
        if (m_pObject != ShadowLightShader::spPass[115].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[115].m_pObject;
            v67 = ShadowLightShader::spPass[115].m_pObject;
            if (ShadowLightShader::spPass[115].m_pObject) {
                ++ShadowLightShader::spPass[115].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[45]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[17]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[115] = 2176;
        ShadowLightShader::ulPixelConstantFlags[115] = 12;
        if (m_pObject != ShadowLightShader::spPass[143].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[143].m_pObject;
            v67 = ShadowLightShader::spPass[143].m_pObject;
            if (ShadowLightShader::spPass[143].m_pObject) {
                ++ShadowLightShader::spPass[143].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[46]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[18]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[143] = 2944;
        ShadowLightShader::ulPixelConstantFlags[143] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[143] = 768;
        if (m_pObject != ShadowLightShader::spPass[116].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[116].m_pObject;
            v67 = ShadowLightShader::spPass[116].m_pObject;
            if (ShadowLightShader::spPass[116].m_pObject) {
                ++ShadowLightShader::spPass[116].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[41]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[11]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[116] = 2176;
        ShadowLightShader::ulPixelConstantFlags[116] = 12;
        if (m_pObject != ShadowLightShader::spPass[117].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[117].m_pObject;
            v67 = ShadowLightShader::spPass[117].m_pObject;
            if (ShadowLightShader::spPass[117].m_pObject) {
                ++ShadowLightShader::spPass[117].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[42]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[15]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[117] = 2176;
        ShadowLightShader::ulPixelConstantFlags[117] = 12;
        if (m_pObject != ShadowLightShader::spPass[141].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[141].m_pObject;
            v67 = ShadowLightShader::spPass[141].m_pObject;
            if (ShadowLightShader::spPass[141].m_pObject) {
                ++ShadowLightShader::spPass[141].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[43]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[12]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[141] = 2944;
        ShadowLightShader::ulPixelConstantFlags[141] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[141] = 768;
        if (m_pObject != ShadowLightShader::spPass[142].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[142].m_pObject;
            v67 = ShadowLightShader::spPass[142].m_pObject;
            if (ShadowLightShader::spPass[142].m_pObject) {
                ++ShadowLightShader::spPass[142].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[44]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[16]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[142] = 2944;
        ShadowLightShader::ulPixelConstantFlags[142] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[142] = 768;
        if (m_pObject != ShadowLightShader::spPass[118].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[118].m_pObject;
            v67 = ShadowLightShader::spPass[118].m_pObject;
            if (ShadowLightShader::spPass[118].m_pObject) {
                ++ShadowLightShader::spPass[118].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[41]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[23]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[118] = 2176;
        ShadowLightShader::ulPixelConstantFlags[118] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[118] = 4;
        if (m_pObject != ShadowLightShader::spPass[119].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[119].m_pObject;
            v67 = ShadowLightShader::spPass[119].m_pObject;
            if (ShadowLightShader::spPass[119].m_pObject) {
                ++ShadowLightShader::spPass[119].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[42]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[27]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[119] = 2176;
        ShadowLightShader::ulPixelConstantFlags[119] = 28;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[119] = 4;
        if (m_pObject != ShadowLightShader::spPass[120].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[120].m_pObject;
            v67 = ShadowLightShader::spPass[120].m_pObject;
            if (ShadowLightShader::spPass[120].m_pObject) {
                ++ShadowLightShader::spPass[120].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[45]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[19]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[120] = 2176;
        ShadowLightShader::ulPixelConstantFlags[120] = 12;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[120] = 4;
        if (m_pObject != ShadowLightShader::spPass[144].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[144].m_pObject;
            v67 = ShadowLightShader::spPass[144].m_pObject;
            if (ShadowLightShader::spPass[144].m_pObject) {
                ++ShadowLightShader::spPass[144].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[43]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[24]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[144] = 2944;
        ShadowLightShader::ulPixelConstantFlags[144] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[144] = 768;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[144] = 4;
        if (m_pObject != ShadowLightShader::spPass[145].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[145].m_pObject;
            v67 = ShadowLightShader::spPass[145].m_pObject;
            if (ShadowLightShader::spPass[145].m_pObject) {
                ++ShadowLightShader::spPass[145].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(ShadowLightShader::spVertexShaders[44]);
        m_pObject->SetPixelShader(ShadowLightShader::spPixelShaders[28]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        NiD3DPass* v63 = ShadowLightShader::spPass[121].m_pObject;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[145] = 4;
        ShadowLightShader::ulVertexConstantFlags[145] = 2944;
        ShadowLightShader::ulPixelConstantFlags[145] = 28;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[145] = 768;
        if (ShadowLightShader::spPass[121].m_pObject) {
            v6 = ShadowLightShader::spPass[121].m_pObject->m_uiRefCount-- == 1;
            if (v6) {
                v63->ReturnPassToPool();
            }
            ShadowLightShader::spPass[121].m_pObject = nullptr;
        }
        NiD3DPass* v64 = ShadowLightShader::spPass[146].m_pObject;
        if (ShadowLightShader::spPass[146].m_pObject) {
            v6 = ShadowLightShader::spPass[146].m_pObject->m_uiRefCount-- == 1;
            if (v6) {
                v64->ReturnPassToPool();
            }
            ShadowLightShader::spPass[146].m_pObject = nullptr;
        }
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetAmbDiffDirAndPtPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v13 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[124].m_pObject) {
            m_pObject = ShadowLightShader::spPass[124].m_pObject;
            v13 = ShadowLightShader::spPass[124].m_pObject;
            ++ShadowLightShader::spPass[124].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[16];
        if (v2 != ShadowLightShader::spVertexShaders[16]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[29];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[124] = 128;
        ShadowLightShader::ulPixelConstantFlags[124] = 12;
        if (m_pObject != ShadowLightShader::spPass[125].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[125].m_pObject;
            v13 = ShadowLightShader::spPass[125].m_pObject;
            if (ShadowLightShader::spPass[125].m_pObject) {
                ++ShadowLightShader::spPass[125].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[47];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[29];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[125] = 2176;
        ShadowLightShader::ulPixelConstantFlags[125] = 12;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetDiffuseLightPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v22 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[300].m_pObject) {
            m_pObject = ShadowLightShader::spPass[300].m_pObject;
            v22 = ShadowLightShader::spPass[300].m_pObject;
            ++ShadowLightShader::spPass[300].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[4];
        if (v2 != ShadowLightShader::spVertexShaders[4]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v5 = ShadowLightShader::spPixelShaders[2];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[300] = 128;
        ShadowLightShader::ulPixelConstantFlags[300] = 8;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[300] = 8;
        if (m_pObject != ShadowLightShader::spPass[302].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[302].m_pObject;
            v22 = ShadowLightShader::spPass[302].m_pObject;
            if (ShadowLightShader::spPass[302].m_pObject) {
                ++ShadowLightShader::spPass[302].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v8 = ShadowLightShader::spVertexShaders[35];
        m_pObject->m_spVertexShader = v8;
        NiD3DPixelShader* v10 = ShadowLightShader::spPixelShaders[2];
        m_pObject->m_spPixelShader = v10;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[302] = 2176;
        ShadowLightShader::ulPixelConstantFlags[302] = 8;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[302] = 8;
        if (m_pObject != ShadowLightShader::spPass[301].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[301].m_pObject;
            v22 = ShadowLightShader::spPass[301].m_pObject;
            if (ShadowLightShader::spPass[301].m_pObject) {
                ++ShadowLightShader::spPass[301].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v12 = ShadowLightShader::spVertexShaders[3];
        m_pObject->m_spVertexShader = v12;
        NiD3DPixelShader* v14 = ShadowLightShader::spPixelShaders[1];
        m_pObject->m_spPixelShader = v14;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[301] = 128;
        ShadowLightShader::ulPixelConstantFlags[301] = 8;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[301] = 8;
        if (m_pObject != ShadowLightShader::spPass[303].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[303].m_pObject;
            v22 = ShadowLightShader::spPass[303].m_pObject;
            if (ShadowLightShader::spPass[303].m_pObject) {
                ++ShadowLightShader::spPass[303].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v16 = ShadowLightShader::spVertexShaders[34];
        m_pObject->m_spVertexShader = v16;
        NiD3DPixelShader* v18 = ShadowLightShader::spPixelShaders[1];
        m_pObject->m_spPixelShader = v18;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulPixelConstantFlags[303] = 8;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[303] = 8;
        ShadowLightShader::ulVertexConstantFlags[303] = 2176;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetTexturePasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v37 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[330].m_pObject) {
            m_pObject = ShadowLightShader::spPass[330].m_pObject;
            v37 = ShadowLightShader::spPass[330].m_pObject;
            ++ShadowLightShader::spPass[330].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[5];
        if (v2 != ShadowLightShader::spVertexShaders[5]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[3];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[330] = 0;
        ShadowLightShader::ulPixelConstantFlags[330] = 0;
        if (m_pObject != ShadowLightShader::spPass[331].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[331].m_pObject;
            v37 = ShadowLightShader::spPass[331].m_pObject;
            if (ShadowLightShader::spPass[331].m_pObject) {
                ++ShadowLightShader::spPass[331].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[7];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[5];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[331] = 0;
        ShadowLightShader::ulPixelConstantFlags[331] = 0;
        if (m_pObject != ShadowLightShader::spPass[332].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[332].m_pObject;
            v37 = ShadowLightShader::spPass[332].m_pObject;
            if (ShadowLightShader::spPass[332].m_pObject) {
                ++ShadowLightShader::spPass[332].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[6];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[6];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[332] = 0;
        ShadowLightShader::ulPixelConstantFlags[332] = 16;
        if (m_pObject != ShadowLightShader::spPass[333].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[333].m_pObject;
            v37 = ShadowLightShader::spPass[333].m_pObject;
            if (ShadowLightShader::spPass[333].m_pObject) {
                ++ShadowLightShader::spPass[333].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[36];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[3];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[333] = 2048;
        ShadowLightShader::ulPixelConstantFlags[333] = 0;
        if (m_pObject != ShadowLightShader::spPass[334].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[334].m_pObject;
            v37 = ShadowLightShader::spPass[334].m_pObject;
            if (ShadowLightShader::spPass[334].m_pObject) {
                ++ShadowLightShader::spPass[334].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[6];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[4];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[334] = 0;
        ShadowLightShader::ulPixelConstantFlags[334] = 0;
        if (m_pObject != ShadowLightShader::spPass[335].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[335].m_pObject;
            v37 = ShadowLightShader::spPass[335].m_pObject;
            if (ShadowLightShader::spPass[335].m_pObject) {
                ++ShadowLightShader::spPass[335].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[37];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[4];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[335] = 2048;
        ShadowLightShader::ulPixelConstantFlags[335] = 0;
        if (m_pObject != ShadowLightShader::spPass[336].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[336].m_pObject;
            v37 = ShadowLightShader::spPass[336].m_pObject;
            if (ShadowLightShader::spPass[336].m_pObject) {
                ++ShadowLightShader::spPass[336].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 3) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v27 = ShadowLightShader::spVertexShaders[38];
        m_pObject->m_spVertexShader = v27;
        NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[5];
        m_pObject->m_spPixelShader = v29;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[336] = 2048;
        ShadowLightShader::ulPixelConstantFlags[336] = 0;
        if (m_pObject != ShadowLightShader::spPass[338].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[338].m_pObject;
            v37 = ShadowLightShader::spPass[338].m_pObject;
            if (ShadowLightShader::spPass[338].m_pObject) {
                ++ShadowLightShader::spPass[338].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v31 = ShadowLightShader::spVertexShaders[37];
        m_pObject->m_spVertexShader = v31;
        NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[6];
        m_pObject->m_spPixelShader = v33;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 9u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[338] = 2048;
        ShadowLightShader::ulPixelConstantFlags[338] = 16;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetSpecularPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v21 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[340].m_pObject) {
            m_pObject = ShadowLightShader::spPass[340].m_pObject;
            v21 = ShadowLightShader::spPass[340].m_pObject;
            ++ShadowLightShader::spPass[340].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[9];
        if (v2 != ShadowLightShader::spVertexShaders[9]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[8];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_SPECULARDIR] = 1152;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_SPECULARDIR] = 8;
        if (m_pObject != ShadowLightShader::spPass[341].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[341].m_pObject;
            v21 = ShadowLightShader::spPass[341].m_pObject;
            if (ShadowLightShader::spPass[341].m_pObject) {
                ++ShadowLightShader::spPass[341].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[8];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[7];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[341] = 1152;
        ShadowLightShader::ulPixelConstantFlags[341] = 8;
        if (m_pObject != ShadowLightShader::spPass[342].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[342].m_pObject;
            v21 = ShadowLightShader::spPass[342].m_pObject;
            if (ShadowLightShader::spPass[342].m_pObject) {
                ++ShadowLightShader::spPass[342].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[40];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[8];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[342] = 3200;
        ShadowLightShader::ulPixelConstantFlags[342] = 8;
        if (m_pObject != ShadowLightShader::spPass[343].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[343].m_pObject;
            v21 = ShadowLightShader::spPass[343].m_pObject;
            if (ShadowLightShader::spPass[343].m_pObject) {
                ++ShadowLightShader::spPass[343].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[39];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[7];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        ShadowLightShader::ulVertexConstantFlags[343] = 3200;
        ShadowLightShader::ulPixelConstantFlags[343] = 8;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetEnvMapPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v21 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[580].m_pObject) {
            m_pObject = ShadowLightShader::spPass[580].m_pObject;
            v21 = ShadowLightShader::spPass[580].m_pObject;
            ++ShadowLightShader::spPass[580].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[19];
        if (v2 != ShadowLightShader::spVertexShaders[19]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[32];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[580] = 1028;
        ShadowLightShader::ulPixelConstantFlags[580] = 36;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[580] = 4;
        if (m_pObject != ShadowLightShader::spPass[581].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[581].m_pObject;
            v21 = ShadowLightShader::spPass[581].m_pObject;
            if (ShadowLightShader::spPass[581].m_pObject) {
                ++ShadowLightShader::spPass[581].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[19];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[33];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[581] = 1028;
        ShadowLightShader::ulPixelConstantFlags[581] = 36;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[581] = 4;
        if (m_pObject != ShadowLightShader::spPass[582].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[582].m_pObject;
            v21 = ShadowLightShader::spPass[582].m_pObject;
            if (ShadowLightShader::spPass[582].m_pObject) {
                ++ShadowLightShader::spPass[582].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[48];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[32];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[582] = 3076;
        ShadowLightShader::ulPixelConstantFlags[582] = 36;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[582] = 4;
        if (m_pObject != ShadowLightShader::spPass[583].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[583].m_pObject;
            v21 = ShadowLightShader::spPass[583].m_pObject;
            if (ShadowLightShader::spPass[583].m_pObject) {
                ++ShadowLightShader::spPass[583].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 2u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[48];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[33];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[583] = 3076;
        ShadowLightShader::ulPixelConstantFlags[583] = 36;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[583] = 4;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetFogPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v21 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[594].m_pObject) {
            m_pObject = ShadowLightShader::spPass[594].m_pObject;
            v21 = ShadowLightShader::spPass[594].m_pObject;
            ++ShadowLightShader::spPass[594].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[17];
        if (v2 != ShadowLightShader::spVertexShaders[17]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[30];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[594] = 768;
        ShadowLightShader::ulPixelConstantFlags[594] = 0;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[594] = 768;
        if (m_pObject != ShadowLightShader::spPass[595].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[595].m_pObject;
            v21 = ShadowLightShader::spPass[595].m_pObject;
            if (ShadowLightShader::spPass[595].m_pObject) {
                ++ShadowLightShader::spPass[595].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[18];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[31];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[595] = 768;
        ShadowLightShader::ulPixelConstantFlags[595] = 0;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[595] = 768;
        if (m_pObject != ShadowLightShader::spPass[596].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[596].m_pObject;
            v21 = ShadowLightShader::spPass[596].m_pObject;
            if (ShadowLightShader::spPass[596].m_pObject) {
                ++ShadowLightShader::spPass[596].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[32];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[30];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[596] = 2816;
        ShadowLightShader::ulPixelConstantFlags[596] = 0;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[596] = 768;
        if (m_pObject != ShadowLightShader::spPass[597].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[597].m_pObject;
            v21 = ShadowLightShader::spPass[597].m_pObject;
            if (ShadowLightShader::spPass[597].m_pObject) {
                ++ShadowLightShader::spPass[597].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[33];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[31];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[597] = 2816;
        ShadowLightShader::ulPixelConstantFlags[597] = 0;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[597] = 768;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetLandscape_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v29 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[147].m_pObject) {
            m_pObject = ShadowLightShader::spPass[147].m_pObject;
            v29 = ShadowLightShader::spPass[147].m_pObject;
            ++ShadowLightShader::spPass[147].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[25];
        if (v2 != ShadowLightShader::spVertexShaders[25]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[40];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        ShadowLightShader::ulVertexConstantFlags[BSShaderManager::BSSM_LANDAD] = 128;
        ShadowLightShader::ulPixelConstantFlags[147] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[147] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[147] = 12;
        if (m_pObject != ShadowLightShader::spPass[498].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[498].m_pObject;
            v29 = ShadowLightShader::spPass[498].m_pObject;
            if (ShadowLightShader::spPass[498].m_pObject) {
                ++ShadowLightShader::spPass[498].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[26];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[41];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[498] = 128;
        ShadowLightShader::ulPixelConstantFlags[498] = 12;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[498] = 128;
        ShadowLightShader::uiPixelNonVolatileConstantFlags[498] = 4;
        if (m_pObject != ShadowLightShader::spPass[497].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[497].m_pObject;
            v29 = ShadowLightShader::spPass[497].m_pObject;
            if (ShadowLightShader::spPass[497].m_pObject) {
                ++ShadowLightShader::spPass[497].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[27];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[42];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[497] = 0;
        ShadowLightShader::ulPixelConstantFlags[497] = 8;
        if (m_pObject != ShadowLightShader::spPass[500].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[500].m_pObject;
            v29 = ShadowLightShader::spPass[500].m_pObject;
            if (ShadowLightShader::spPass[500].m_pObject) {
                ++ShadowLightShader::spPass[500].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[27];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[43];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[500] = 0;
        ShadowLightShader::ulPixelConstantFlags[500] = 8;
        if (m_pObject != ShadowLightShader::spPass[495].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[495].m_pObject;
            v29 = ShadowLightShader::spPass[495].m_pObject;
            if (ShadowLightShader::spPass[495].m_pObject) {
                ++ShadowLightShader::spPass[495].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[28];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[44];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::uiVertexNonVolatileConstantFlags[147] = 128;
        ShadowLightShader::ulVertexConstantFlags[495] = 128;
        ShadowLightShader::ulPixelConstantFlags[495] = 8;
        if (m_pObject != ShadowLightShader::spPass[501].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[501].m_pObject;
            v29 = ShadowLightShader::spPass[501].m_pObject;
            if (ShadowLightShader::spPass[501].m_pObject) {
                ++ShadowLightShader::spPass[501].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v23 = ShadowLightShader::spVertexShaders[29];
        m_pObject->m_spVertexShader = v23;
        NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[45];
        m_pObject->m_spPixelShader = v25;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[501] = 128;
        ShadowLightShader::ulPixelConstantFlags[501] = 8;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetTextureEffectPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v13 = nullptr;
    {
        NiPointer<NiD3DTextureStage> a1;
        if (ShadowLightShader::spPass[590].m_pObject) {
            m_pObject = ShadowLightShader::spPass[590].m_pObject;
            v13 = ShadowLightShader::spPass[590].m_pObject;
            ++ShadowLightShader::spPass[590].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[30];
        if (v2 != ShadowLightShader::spVertexShaders[30]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[46];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[590] = 0;
        ShadowLightShader::ulPixelConstantFlags[590] = 0;
        if (m_pObject != ShadowLightShader::spPass[591].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[591].m_pObject;
            v13 = ShadowLightShader::spPass[591].m_pObject;
            if (ShadowLightShader::spPass[591].m_pObject) {
                ++ShadowLightShader::spPass[591].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 2) {
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
            NiD3DTextureStage::CreateNewStage(a1);
            BSShader::SetupDefaultStage(a1.m_pObject, 1u, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, a1.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[52];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[46];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 3u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[591] = 0;
        ShadowLightShader::ulPixelConstantFlags[591] = 0;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetZOnlyPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v25 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spStage;
        if (ShadowLightShader::spPass[1].m_pObject) {
            m_pObject = ShadowLightShader::spPass[1].m_pObject;
            v25 = ShadowLightShader::spPass[1].m_pObject;
            ++ShadowLightShader::spPass[1].m_pObject->m_uiRefCount;
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[0];
        if (v2 != ShadowLightShader::spVertexShaders[0]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[1] = 0;
        ShadowLightShader::ulPixelConstantFlags[BSShaderManager::BSSM_ZONLY] = 0;
        if (m_pObject != ShadowLightShader::spPass[2].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[2].m_pObject;
            v25 = ShadowLightShader::spPass[2].m_pObject;
            if (ShadowLightShader::spPass[2].m_pObject) {
                ++ShadowLightShader::spPass[2].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[0];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v9;
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
        m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[2] = 0;
        ShadowLightShader::ulPixelConstantFlags[2] = 0;
        if (m_pObject != ShadowLightShader::spPass[3].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[3].m_pObject;
            v25 = ShadowLightShader::spPass[3].m_pObject;
            if (ShadowLightShader::spPass[3].m_pObject) {
                ++ShadowLightShader::spPass[3].m_pObject->m_uiRefCount;
            }
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[31];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
        ShadowLightShader::ulVertexConstantFlags[3] = 2048;
        ShadowLightShader::ulPixelConstantFlags[3] = 0;
        if (m_pObject != ShadowLightShader::spPass[4].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[4].m_pObject;
            v25 = ShadowLightShader::spPass[4].m_pObject;
            if (ShadowLightShader::spPass[4].m_pObject) {
                ++ShadowLightShader::spPass[4u].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v15 = ShadowLightShader::spVertexShaders[0];
        m_pObject->m_spVertexShader = v15;
        NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v17;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
        float uiValue = BSShaderProperty::fDepthBiasUnit;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, std::bit_cast<uint32_t>(uiValue), true);
        ShadowLightShader::ulVertexConstantFlags[4] = 0;
        ShadowLightShader::ulPixelConstantFlags[4] = 0;
        if (m_pObject != ShadowLightShader::spPass[5].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[5].m_pObject;
            v25 = ShadowLightShader::spPass[5].m_pObject;
            if (ShadowLightShader::spPass[5].m_pObject) {
                ++ShadowLightShader::spPass[5].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v19 = ShadowLightShader::spVertexShaders[31];
        m_pObject->m_spVertexShader = v19;
        NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[0];
        m_pObject->m_spPixelShader = v21;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
        uiValue = BSShaderProperty::fDepthBiasUnit;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(195u, std::bit_cast<uint32_t>(uiValue), true);
        ShadowLightShader::ulVertexConstantFlags[5] = 2048;
        ShadowLightShader::ulPixelConstantFlags[5] = 0;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::PresetConstAlphaPasses_1x() {
    NiD3DPass* m_pObject = nullptr;
    NiD3DPass* v17 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spStage;
        if (ShadowLightShader::spPass[6].m_pObject) {
            m_pObject = ShadowLightShader::spPass[6].m_pObject;
            v17 = ShadowLightShader::spPass[6].m_pObject;
            ++ShadowLightShader::spPass[6].m_pObject->m_uiRefCount;
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v2 = m_pObject->m_spVertexShader.m_pObject;
        NiD3DVertexShader* v3 = ShadowLightShader::spVertexShaders[0];
        if (v2 != ShadowLightShader::spVertexShaders[0]) {
            if (v2) {
                NiDecRefCount(v2);
            }
            m_pObject->m_spVertexShader.m_pObject = v3;
            if (v3) {
                NiIncRefCount(v3);
            }
        }
        NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[47];
        m_pObject->m_spPixelShader = v4;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, true);
        ShadowLightShader::ulVertexConstantFlags[6] = 0;
        ShadowLightShader::ulPixelConstantFlags[6] = 4;
        if (m_pObject != ShadowLightShader::spPass[7].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[7].m_pObject;
            v17 = ShadowLightShader::spPass[7].m_pObject;
            if (ShadowLightShader::spPass[7].m_pObject) {
                ++ShadowLightShader::spPass[7].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v7 = ShadowLightShader::spVertexShaders[0];
        m_pObject->m_spVertexShader = v7;
        NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[47];
        m_pObject->m_spPixelShader = v9;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, true);
        ShadowLightShader::ulVertexConstantFlags[7] = 0;
        ShadowLightShader::ulPixelConstantFlags[7] = 4;
        if (m_pObject != ShadowLightShader::spPass[8].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = ShadowLightShader::spPass[8].m_pObject;
            v17 = ShadowLightShader::spPass[8].m_pObject;
            if (ShadowLightShader::spPass[8].m_pObject) {
                ++ShadowLightShader::spPass[8].m_pObject->m_uiRefCount;
            }
        }
        if (!m_pObject->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            m_pObject->SetStage(m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v11 = ShadowLightShader::spVertexShaders[31];
        m_pObject->m_spVertexShader = v11;
        NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[47];
        m_pObject->m_spPixelShader = v13;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 3u, true);
        ShadowLightShader::ulVertexConstantFlags[8] = 2048;
        ShadowLightShader::ulPixelConstantFlags[8] = 4;
    }
    m_pObject->DecRefCount();
}

void ShadowLightShader::AddEmitToAmbient(NiMaterialProperty* apMat) {
    NiColor* m_external_emit = apMat->m_external_emit;
    if (!m_external_emit) {
        m_external_emit = &apMat->m_emit;
    }
    double b = m_external_emit->b;
    double g = m_external_emit->g;
    if (NiColor_BLACK.r != m_external_emit->r || (NiColor_BLACK.g != g || NiColor_BLACK.b != b)) {
        double m_fEmitMult;
        double v6;
        double r;
        float fReda;
        float fGreena;
        float fBluea;
        float apMatProp;
        if (BSShaderManager::bHDR) {
            // EXE-INLINED 0x4EBBC0
            ImageSpaceEffectHDR* pHDR = static_cast<ImageSpaceEffectHDR*>(
                BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_HDR]
            );
            m_fEmitMult = pHDR->pfData[3] * apMat->m_fEmitMult;
        }
        else {
            if (!(apMat->m_fEmitMult < 1.0f)) {
                r = m_external_emit->r;
                v6 = 1.0;
                goto LABEL_12;
            }
            m_fEmitMult = apMat->m_fEmitMult;
        }
        apMatProp = m_fEmitMult;
        fReda = m_external_emit->r * apMatProp;
        fGreena = g * apMatProp;
        v6 = 1.0;
        fBluea = b * apMatProp;
        b = fBluea;
        g = fGreena;
        r = fReda;
    LABEL_12:;
        float fRed = r + BSShaderLightingProperty::pShaderLightConstants[0].X();
        float fGreen = g + BSShaderLightingProperty::pShaderLightConstants[0].Y();
        float fBlue = b + BSShaderLightingProperty::pShaderLightConstants[0].Z();
        if (!BSShaderManager::bHDR) {
            if (!(v6 > fRed)) {
                fRed = v6;
            }
            if (!(v6 > fGreen)) {
                fGreen = v6;
            }
            if (!(v6 > fBlue)) {
                fBlue = v6;
            }
        }
        BSShaderLightingProperty::pShaderLightConstants[0].x = fRed;
        BSShaderLightingProperty::pShaderLightConstants[0].y = fGreen;
        BSShaderLightingProperty::pShaderLightConstants[0].z = fBlue;
    }
}

void ShadowLightShader::SetEmitConstant(NiMaterialProperty* apMat) {
    NiColor* m_external_emit = apMat->m_external_emit;
    if (!m_external_emit) {
        m_external_emit = &apMat->m_emit;
    }
    float kColor = m_external_emit->r;
    float kColor_4 = m_external_emit->g;
    float kColor_8 = m_external_emit->b;
    if (BSShaderManager::bHDR) {
        // EXE-INLINED 0x4EBBC0
        ImageSpaceEffectHDR* pHDR = static_cast<ImageSpaceEffectHDR*>(
            BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_HDR]
        );
        float apMatProp = pHDR->pfData[3] * apMat->m_fEmitMult;
        kColor = apMatProp * kColor;
        kColor_4 = apMatProp * kColor_4;
        kColor_8 = apMatProp * kColor_8;
    }
    BSShaderLightingProperty::pShaderLightConstants[27].x = kColor;
    BSShaderLightingProperty::pShaderLightConstants[27].y = kColor_4;
    BSShaderLightingProperty::pShaderLightConstants[27].z = kColor_8;
    BSShaderLightingProperty::pShaderLightConstants[27].w = 1.0f;
}

void ShadowLightShader::SetProjectedShadowScale(uint32_t aiScale) {
    ShadowLightShader::kShadowProjTransform.w = ((float)aiScale);
}

uint32_t ShadowLightShader::GetProjectedShadowScale() {
    return (uint32_t)((int64_t)ShadowLightShader::kShadowProjTransform.w);
}

void ShadowLightShader::SetProjectedShadowMaskWidth(float afWidth) {
    ShadowLightShader::kShadowProjData.w = afWidth;
}

void ShadowLightShader::UpdateFarNear() {
    ShadowLightShader::kFarNear.x = (float)(1.0 / ::fRenderDepthMaxDepth.uValue.f);
    ShadowLightShader::kFarNear.y = (float)0.0;
}

void ShadowLightShader::EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass) {
    ShadowLightShader* pkshadowLightShader = static_cast<ShadowLightShader*>(BSShaderManager::GetShader(1));
    SkinShader* pkskinShader = static_cast<SkinShader*>(BSShaderManager::GetShader(14));
    ParallaxShader* pkparralaxShader = static_cast<ParallaxShader*>(BSShaderManager::GetShader(15));
    HairShader* pkhairShader = static_cast<HairShader*>(BSShaderManager::GetShader(13));
    if (auiPassEnum - 590 > 3) {
        if (pkshadowLightShader) {
            pkshadowLightShader->m_spPixelConstantMap.operator=(pkshadowLightShader->spDefaultPixelConstantMap);
            pkshadowLightShader->m_spVertexConstantMap.operator=(pkshadowLightShader->spDefaultVertexConstantMap);
        }
        if (pkskinShader) {
            pkskinShader->m_spPixelConstantMap.operator=(pkskinShader->spDefaultPixelConstantMap);
            pkskinShader->m_spVertexConstantMap.operator=(pkskinShader->spDefaultVertexConstantMap);
        }
        if (pkparralaxShader) {
            pkparralaxShader->m_spPixelConstantMap.operator=(pkparralaxShader->spDefaultPixelConstantMap);
            pkparralaxShader->m_spVertexConstantMap.operator=(pkparralaxShader->spDefaultVertexConstantMap);
        }
        if (pkhairShader) {
            pkhairShader->m_spPixelConstantMap.operator=(pkhairShader->spDefaultPixelConstantMap);
            pkhairShader->m_spVertexConstantMap.operator=(pkhairShader->spDefaultVertexConstantMap);
        }
        if (BSShaderManager::eShaderLevel < 2) {
            if (BSShaderManager::eShaderLevel == 1) {
                ShadowLightShader::EnableConstantsForPass_1x(auiPassEnum);
            }
        }
        else {
            ShadowLightShader::EnableConstantsForPass_2x(auiPassEnum);
        }
    }
    else {
        if (pkshadowLightShader) {
            pkshadowLightShader->m_spPixelConstantMap.operator=(pkshadowLightShader->spTexEffectPConstantMap);
            pkshadowLightShader->m_spVertexConstantMap.operator=(pkshadowLightShader->spTexEffectVConstantMap);
        }
        if (pkskinShader) {
            pkskinShader->m_spPixelConstantMap.operator=(pkskinShader->spTexEffectPConstantMap);
            pkskinShader->m_spVertexConstantMap.operator=(pkskinShader->spTexEffectVConstantMap);
        }
        if (pkparralaxShader) {
            pkparralaxShader->m_spPixelConstantMap.operator=(pkparralaxShader->spTexEffectPConstantMap);
            pkparralaxShader->m_spVertexConstantMap.operator=(pkparralaxShader->spTexEffectVConstantMap);
        }
        if (pkhairShader) {
            pkhairShader->m_spPixelConstantMap.operator=(pkhairShader->spTexEffectPConstantMap);
            pkhairShader->m_spVertexConstantMap.operator=(pkhairShader->spTexEffectVConstantMap);
        }
    }
    if (auiPassEnum < 759) {
        uint32_t v16;
        NiD3DPass* m_pObject = ShadowLightShader::spPass[auiPassEnum].m_pObject;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(28u, 0, true);
        if ((BSShaderManager::usRenderFlags & 1) != 0) {
            NiD3DPass* v6 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
            if (!v6->m_pkRenderStateGroup) {
                v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v6->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
            goto LABEL_48;
        }
        if ((auiPassEnum < 482 || auiPassEnum > 486) && auiPassEnum - 492 > 2) {
            if (auiPassEnum - 340 > 35) {
                if (auiPassEnum == 726 || (auiPassEnum == 727 || (auiPassEnum == 20 || auiPassEnum == 21))) {
                    v16 = 8;
                }
                else {
                    if (auiPassEnum - 1 > 4) {
                        if (auiPassEnum - 6 > 2) {
                            if (auiPassEnum == 9) {
                                ShadowLightShader::spPass[9].m_pObject->SetRenderState(168u, 0, false);
                            LABEL_48:;
                                if (BSShaderManager::eRenderMode == 6) {
                                    NiD3DPass* v7 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    if (!v7->m_pkRenderStateGroup) {
                                        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                                    }
                                    v7->m_pkRenderStateGroup->SetRenderState(52u, 1u, true);
                                    NiD3DPass* v8 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    if (!v8->m_pkRenderStateGroup) {
                                        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                                    }
                                    v8->m_pkRenderStateGroup->SetRenderState(56u, 8u, true);
                                    NiD3DPass* v9 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    if (!v9->m_pkRenderStateGroup) {
                                        v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                                    }
                                    v9->m_pkRenderStateGroup->SetRenderState(55u, 7u, true);
                                    NiD3DPass* v10 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    if (!v10->m_pkRenderStateGroup) {
                                        v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                                    }
                                    v10->m_pkRenderStateGroup->SetRenderState(53u, 1u, true);
                                    NiD3DPass* v11 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    if (!v11->m_pkRenderStateGroup) {
                                        v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                                    }
                                    v11->m_pkRenderStateGroup->SetRenderState(54u, 1u, true);
                                }
                                if (auiPassEnum == 565) {
                                    BSShaderLightingProperty::pShaderLightConstants[35] = kDefaultRefractionParams;
                                }
                                else {
                                    NiD3DPass* v12 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    for (uint32_t i = 0; i < v12->m_uiStageCount; ++i) {
                                        NiD3DTextureStage* pstage = v12->m_kStages.m_pBase[i].m_pObject;
                                        if (pstage) {
                                            NiSourceTexture* m_pkTexture =
                                                static_cast<NiSourceTexture*>(pstage->m_spTexture);
                                            if (m_pkTexture) {
                                                if (m_pkTexture != BSShaderManager::pAttenuationXY && i != 7) {
                                                    pstage->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
                                                }
                                            }
                                        }
                                        v12 = ShadowLightShader::spPass[auiPassEnum].m_pObject;
                                    }
                                }
                                return;
                            }
                            v16 = 7;
                        }
                        else {
                            v16 = 15;
                        }
                    }
                    else {
                        v16 = 0;
                    }
                }
            }
            else {
                v16 = 15;
            }
        }
        else {
            v16 = 15;
        }
        ShadowLightShader::spPass[auiPassEnum].m_pObject->SetRenderState(168u, v16, false);
        goto LABEL_48;
    }
}

// Entry i's flag is bit i + 1. The 22 entries never wrap the bit, so a shift is enough.
void ShadowLightShader::EnableConstantsForPass_1x(uint32_t auiPassEnum) {
    uint32_t uiBit = 1;
    for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spVertexConstantMapEntries) {
        uiBit <<= 1;
        if (pEntry) {
            pEntry->m_bEnabled = (uiBit & ShadowLightShader::ulVertexConstantFlags[auiPassEnum]) != 0;
        }
    }
    uiBit = 1;
    for (NiShaderConstantMapEntry* pEntry : ShadowLightShader::spPixelConstantMapEntries) {
        uiBit <<= 1;
        if (pEntry) {
            pEntry->m_bEnabled = (uiBit & ShadowLightShader::ulPixelConstantFlags[auiPassEnum]) != 0;
        }
    }
}

// It reads no member.
void ShadowLightShader::SetupGeometryConstants_WaterFog() {
    if (ShadowLightShader::spPixelConstantMapEntries[19]->m_bEnabled) {
        ShadowLightShader::kFogPlane.z = BSShaderManager::kWaterPlane.m_kNormal.z;
        ShadowLightShader::kFogPlane.x = BSShaderManager::kWaterPlane.m_kNormal.x;
        ShadowLightShader::kFogPlane.y = BSShaderManager::kWaterPlane.m_kNormal.y;
        // Register 30 of the lighting shaders' shared constants.
        const NiPoint4& kConstant = BSShaderLightingProperty::pShaderLightConstants[30];
        float v2 = BSShaderManager::kWaterPlane.m_kNormal.y * kConstant.y +
            BSShaderManager::kWaterPlane.m_kNormal.x * kConstant.x +
            BSShaderManager::kWaterPlane.m_kNormal.z * kConstant.z;
        float v4 = v2 - BSShaderManager::kWaterPlane.m_fConstant;
        ShadowLightShader::kFogPlane.w = v4;
        double x = BSShaderManager::kWaterFogDistances.x;
        if (BSShaderManager::kWaterFogDistances.y == 0.0 && 0.0 == x) {
            ShadowLightShader::kFogParams.x = 0.0;
            ShadowLightShader::kFogParams.y = 0.0;
            ShadowLightShader::kFogParams.z = -1.0f;
            ShadowLightShader::kFogParams.w = 0.0;
        }
        else {
            ShadowLightShader::kFogParams.x = BSShaderManager::kWaterFogDistances.y;
            float v3 = BSShaderManager::kWaterFogDistances.y - x;
            ShadowLightShader::kFogParams.y = v3;
            ShadowLightShader::kFogParams.z = 1.0;
            ShadowLightShader::kFogParams.w = 1.0;
        }
    }
}

BSShader* ShadowLightShader::CreateShader() {
    ShadowLightShader* pshader;
    NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pSkinnedShaderDecl =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    NiDX9ShaderDeclaration* pModelSpaceNormShaderDecl =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
    NiDX9ShaderDeclaration* pSplatTexturingShaderDecl =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    spShaderDecl->SetEntry(
        3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    spShaderDecl->SetEntry(4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(
        5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    spShaderDecl->GetD3DDeclaration();
    pSkinnedShaderDecl->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pSkinnedShaderDecl->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pSkinnedShaderDecl->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pSkinnedShaderDecl->SetEntry(
        5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pSkinnedShaderDecl->SetEntry(
        6u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        7u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->GetD3DDeclaration();
    pModelSpaceNormShaderDecl->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pModelSpaceNormShaderDecl->SetEntry(
        1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pModelSpaceNormShaderDecl->SetEntry(
        0, 2u, 5u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK, NiShaderDeclaration::SPTYPE_FLOAT1,
        NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u, NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    pModelSpaceNormShaderDecl->GetD3DDeclaration();
    pSplatTexturingShaderDecl->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSplatTexturingShaderDecl->SetEntry(
        0, 6u, 8u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK, NiShaderDeclaration::SPTYPE_FLOAT4,
        NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u, NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    // Gamebryo's extra-data form: SHADERPARAM_EXTRA_DATA_MASK with the extra data index (1) ORed in.
    pSplatTexturingShaderDecl->SetEntry(
        0, 7u, 9u,
        static_cast<NiShaderDeclaration::ShaderParameter>(NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK | 1),
        NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_TEXCOORD, 2u,
        NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    pSplatTexturingShaderDecl->GetD3DDeclaration();
    ShadowLightShader* v4 = static_cast<ShadowLightShader*>(NiMemObject::operator new(sizeof(ShadowLightShader)));
    if (v4) {
        pshader = ::new (v4)
            ShadowLightShader(spShaderDecl, pSkinnedShaderDecl, pModelSpaceNormShaderDecl, pSplatTexturingShaderDecl);
    }
    else {
        pshader = nullptr;
    }
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(spShaderDecl);
    return (BSShader*)pshader;
}
