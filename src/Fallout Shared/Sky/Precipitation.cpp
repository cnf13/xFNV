#include "Fallout Shared/Sky/Precipitation.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/PrecipitationShader.h"
#include "BSShader/PrecipitationShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
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
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBoxEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCylinderEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshUpdateModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysRotationModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSphereEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

#include <cstdint>

Precipitation::Precipitation()
  : spCurrentPrecipRoot(nullptr), spLastPrecipRoot(nullptr), pParent(nullptr), fBirthRate(0.0f), pQueuedModel(nullptr) {
}

Precipitation::~Precipitation() {
    pQueuedModel = nullptr;
}

void Precipitation::Load(const char* apFilename) {
    if (spCurrentPrecipRoot.m_pObject) {
        pParent->DetachChild(spCurrentPrecipRoot.m_pObject);
        spLastPrecipRoot = spCurrentPrecipRoot;
        pParent->AttachChild(spLastPrecipRoot.m_pObject, 1);
        spCurrentPrecipRoot = static_cast<NiNode*>(nullptr);
    }
    if (apFilename && *apFilename) {
        NiNode* pprecipNode = pModelLoader->LoadFile(apFilename, LOD_MULT_NONE, true, false, false, false);
        if (pprecipNode) {
            NiNode* pRoot;
            bool bsnow = BSstricmp(apFilename, "Sky\\Snow.NIF") == 0;
            NiUpdateData UpdateData(0.0f, false, false);
            pprecipNode->UpdateDownwardPass(UpdateData, 0);
            pRoot = new NiNode(0);
            spCurrentPrecipRoot = pRoot;
            fBirthRate = 0.0f;
            for (uint32_t i = 0; i < pprecipNode->GetChildCount(); ++i) {
                NiParticleSystem* pps;
                NiNode* pNode = pprecipNode->GetAt(i)->IsNode();
                if (pNode) {
                    NiAVObject* pChild = pNode->GetAtChecked(0);
                    pps = static_cast<NiParticleSystem*>(
                        NiObject::DynamicCastStatic(&NiParticleSystem::ms_RTTI, pChild)
                    );
                }
                else {
                    pps = nullptr;
                }
                if (pps) {
                    NiGeometry* pprecipGeom = CreateShaderGeometry(pps, pParent->GetWorldTransform(), bsnow);
                    if (pprecipGeom) {
                        spCurrentPrecipRoot->AttachChild(pprecipGeom, 1);
                    }
                }
            }
            pParent->AttachChild(spCurrentPrecipRoot.m_pObject, 1);
            spCurrentPrecipRoot->UpdateProperties();
        }
    }
}

void Precipitation::Initialize() {
    pParent = Main::WeatherRootNode();
}

void Precipitation::FlushAll() {
    if (spLastPrecipRoot.m_pObject != nullptr) {
        pParent->DetachChild(spLastPrecipRoot.m_pObject);
        spLastPrecipRoot = nullptr;
    }
    if (spCurrentPrecipRoot.m_pObject != nullptr) {
        pParent->DetachChild(spCurrentPrecipRoot.m_pObject);
        spCurrentPrecipRoot = nullptr;
    }
}

void Precipitation::Update(float afSecondsPassed) {
    float fEndPct;
    float fBeginPct;
    Sky* pSky = Sky::GetInstance();
    bool bcull = pSky->eMode == Sky::SM_INTERIOR || !pSky->eMode;
    TESWeather* pCurrentWeather = pSky->pCurrentWeather;
    TESWeather* pLastWeather = pSky->pLastWeather;
    float fCurrentPct = pSky->GetWeatherPercent();
    if (pCurrentWeather) {
        fBeginPct = pCurrentWeather->GetWeatherData(TESWeather::WD_DIVIDER1, 0.999f, 0.0f);
    }
    else {
        fBeginPct = 0.0f;
    }
    if (pLastWeather) {
        fEndPct = pLastWeather->GetWeatherData(TESWeather::WD_END_PRECIP, 1.0f, 0.001f);
    }
    else {
        fEndPct = 0.001f;
    }
    if (pSaveLoadGameOLD->GetLoading()) {
        if (spLastPrecipRoot.m_pObject) {
            pParent->DetachChild(spLastPrecipRoot.m_pObject);
            spLastPrecipRoot = static_cast<NiNode*>(nullptr);
        }
        if (spCurrentPrecipRoot.m_pObject) {
            pParent->DetachChild(spCurrentPrecipRoot.m_pObject);
            spCurrentPrecipRoot = static_cast<NiNode*>(nullptr);
        }
        if (pSky->pLastWeather) {
            TESModel* pModel = &pSky->pLastWeather->precipNIF;
            Load(pModel->GetModel());
        }
        if (pSky->pCurrentWeather) {
            TESModel* pModel = &pSky->pCurrentWeather->precipNIF;
            Load(pModel->GetModel());
        }
    }
    else if (pQueuedModel) {
        TESModel* pcurModel = &pCurrentWeather->precipNIF;
        if (pcurModel &&
            pQueuedModel->GetModelLength() == pcurModel->GetModelLength() &&
            !BSstricmp(pQueuedModel->GetModel(), pcurModel->GetModel())) {
            TESModel* pModel = &pCurrentWeather->precipNIF;
            const char* pfileName = pModel->GetModel();
            NiPointer<Model> spmodel;
            if (pModelLoader->FindModel(pfileName, spmodel)) {
                pQueuedModel = nullptr;
                Load(pfileName);
                pModelLoader->ReleaseFile(pfileName);
            }
        }
        else {
            pQueuedModel = nullptr;
            Load_0(pcurModel);
        }
    }
    else if (pSky->GetWeatherUpdateFlag() && pCurrentWeather) {
        Load_0(&pCurrentWeather->precipNIF);
    }
    if (fCurrentPct >= 1.0 && spLastPrecipRoot.m_pObject) {
        pParent->DetachChild(spLastPrecipRoot.m_pObject);
        spLastPrecipRoot = static_cast<NiNode*>(nullptr);
    }

    fBirthRate = 0.0f;
    if (spCurrentPrecipRoot.m_pObject || spLastPrecipRoot.m_pObject) {
        PrecipitationShader::fTime += afSecondsPassed;
        if (pPlayer && pPlayer->GetParentCell() && pPlayer->GetParentCell()->HasWater()) {
            const NiPoint3* pCameraPos = &Main::GetWorldSceneGraph()->GetCameraRoot()->GetWorldTranslate();
            if (static_cast<double>(pCameraPos->z) < pPlayer->GetRelevantWaterHeight()) {
                bcull = true;
            }
        }

        if (spCurrentPrecipRoot.m_pObject) {
            float fBR = 0.0f;
            if (fBeginPct < static_cast<double>(fCurrentPct)) {
                fBR = (fCurrentPct - fBeginPct) / (1.0 - fBeginPct);
            }
            int32_t ichild = 0;
            while (true) {
                PrecipitationShaderProperty* pprecipProp;
                int32_t iPrecipShader;
                NiAVObject* pchild = spCurrentPrecipRoot->GetAt(ichild++);
                if (!pchild) {
                    break;
                }
                if (pchild->GetProperty(3)) {
                    NiProperty* pProperty = pchild->GetProperty(3);
                    iPrecipShader = static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                        BSShaderManager::BSSM_PROPERTYTYPE_PRECIPITATION;
                }
                else {
                    iPrecipShader = 0;
                }
                if (iPrecipShader) {
                    pprecipProp = static_cast<PrecipitationShaderProperty*>(pchild->GetProperty(3));
                }
                else {
                    pprecipProp = nullptr;
                }
                if (pprecipProp) {
                    NiTriBasedGeomData* pgeomData = static_cast<NiTriBasedGeomData*>(
                        static_cast<NiGeometry*>(pchild)->GetModelData()
                    );
                    int32_t iTriCount = pgeomData->GetTriCount();
                    int16_t sTrisPerParticle = iTriCount / pprecipProp->iParticleCount;
                    int16_t sParticles = static_cast<int32_t>(static_cast<double>(pprecipProp->iParticleCount) * fBR);
                    if (static_cast<uint16_t>(sParticles * sTrisPerParticle) != pgeomData->GetActiveTriangleCount()) {
                        pgeomData->SetActiveTriangleCount(sParticles * sTrisPerParticle);
                        if (sParticles) {
                            pgeomData->SetActiveVertexCount(pgeomData->m_usVertices);
                        }
                        else {
                            pgeomData->SetActiveVertexCount(0);
                        }
                    }
                    fBirthRate = static_cast<double>(pprecipProp->iParticleCount) * fBR + fBirthRate;
                }
            }
        }
        if (spLastPrecipRoot.m_pObject) {
            float fBR = 0.0f;
            if (fEndPct > static_cast<double>(fCurrentPct)) {
                fBR = 1.0 - fCurrentPct / fEndPct;
            }
            uint32_t uichild = 0;
            while (true) {
                PrecipitationShaderProperty* pprecipProp;
                int32_t iPrecipShader;
                NiAVObject* pchild = spLastPrecipRoot->GetAt(uichild++);
                if (!pchild) {
                    break;
                }
                if (pchild->GetProperty(3)) {
                    NiProperty* pProperty = pchild->GetProperty(3);
                    iPrecipShader = static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                        BSShaderManager::BSSM_PROPERTYTYPE_PRECIPITATION;
                }
                else {
                    iPrecipShader = 0;
                }
                if (iPrecipShader) {
                    pprecipProp = static_cast<PrecipitationShaderProperty*>(pchild->GetProperty(3));
                }
                else {
                    pprecipProp = nullptr;
                }
                if (pprecipProp) {
                    NiTriBasedGeomData* pgeomData = static_cast<NiTriBasedGeomData*>(
                        static_cast<NiGeometry*>(pchild)->GetModelData()
                    );
                    int32_t iTriCount = pgeomData->GetTriCount();
                    int16_t sTrisPerParticle = iTriCount / pprecipProp->iParticleCount;
                    int16_t sParticles = static_cast<int32_t>(static_cast<double>(pprecipProp->iParticleCount) * fBR);
                    if (static_cast<uint16_t>(sParticles * sTrisPerParticle) != pgeomData->GetActiveTriangleCount()) {
                        pgeomData->SetActiveTriangleCount(sParticles * sTrisPerParticle);
                        if (sParticles) {
                            pgeomData->SetActiveVertexCount(pgeomData->m_usVertices);
                        }
                        else {
                            pgeomData->SetActiveVertexCount(0);
                        }
                    }
                    fBirthRate = static_cast<double>(pprecipProp->iParticleCount) * fBR + fBirthRate;
                }
            }
        }
    }
    if (spCurrentPrecipRoot.m_pObject) {
        if (bcull) {
            spCurrentPrecipRoot->SetAppCulled(true);
        }
        else {
            spCurrentPrecipRoot->SetAppCulled(false);
        }
    }
    if (spLastPrecipRoot.m_pObject) {
        if (bcull) {
            spLastPrecipRoot->SetAppCulled(true);
        }
        else {
            spLastPrecipRoot->SetAppCulled(false);
        }
    }
}

void Precipitation::Load_0(TESModel* apModel) {
    if (!spLastPrecipRoot.m_pObject) {
        if (apModel && apModel->GetModelLength()) {
            const char* pFileName = apModel->GetModel();
            pModelLoader->QueueModel(
                pFileName,
                IO_TASK_PRIORITY_CRITICAL,
                static_cast<QueuedFile*>(nullptr),
                LOD_MULT_NONE,
                false,
                false,
                false
            );
            pQueuedModel = apModel;
        }
        else {
            Load(nullptr);
        }
    }
}

// akWorldTransform is unused.
NiGeometry* Precipitation::CreateShaderGeometry(
    NiParticleSystem* apParticleSystem,
    const NiTransform& akWorldTransform,
    bool abSnow
) {
    float fmaxEmissionRate = 0.0f;
    NiPSysEmitter* pemitter = nullptr;
    NiPSysMeshUpdateModifier* pmeshUpdateModifier = nullptr;
    NiPSysRotationModifier* protationModifier = nullptr;
    NiGeometry* prainGeom = nullptr;
    NiGeometry* pnewGeom = nullptr;
    for (NiTimeController* pctlr = apParticleSystem->GetControllers(); pctlr; pctlr = pctlr->GetNext()) {
        if (NiObject::IsKindOf_0(&NiPSysEmitterCtlr::ms_RTTI, pctlr)) {
            NiPSysEmitterCtlr* pemitterCtlr = static_cast<NiPSysEmitterCtlr*>(pctlr);
            pemitter = static_cast<NiPSysEmitter*>(
                NiObject::DynamicCastStatic(&NiPSysEmitter::ms_RTTI, pemitterCtlr->m_pkModifier)
            );
            NiFloatInterpolator* pfi = pemitterCtlr->GetBirthRateInterpolator();
            pfi->Update(0.0f, nullptr, fmaxEmissionRate);
            fmaxEmissionRate = pemitter->m_fLifeSpan * fmaxEmissionRate;
            break;
        }
    }
    if (!pemitter) {
        return nullptr;
    }
    uint16_t usinstanceVertices = 0;
    uint16_t inuminstances = static_cast<int32_t>(fmaxEmissionRate);
    float frotationSpeed = 0.0f;
    NiTransform kEmitterToPSys;
    NiPoint3 kMin;
    NiPoint3 kMax;
    if (NiObject::IsKindOf_0(&NiMeshParticleSystem::ms_RTTI, apParticleSystem)) {
        uint32_t imod = 0;
        while (true) {
            NiPSysModifier* pmodifier = apParticleSystem->GetModifierAt(imod++);
            if (!pmodifier) {
                break;
            }
            if (!pmeshUpdateModifier && NiObject::IsKindOf_0(&NiPSysMeshUpdateModifier::ms_RTTI, pmodifier)) {
                pmeshUpdateModifier = static_cast<NiPSysMeshUpdateModifier*>(pmodifier);
            }
            if (!protationModifier && NiObject::IsKindOf_0(&NiPSysRotationModifier::ms_RTTI, pmodifier)) {
                protationModifier = static_cast<NiPSysRotationModifier*>(pmodifier);
            }
        }
        if (pmeshUpdateModifier) {
            NiAVObject* pmesh = pmeshUpdateModifier->GetMeshAt(0);
            NiNode* pmeshNode = pmesh->IsNode();
            if (pmeshNode) {
                prainGeom = static_cast<NiGeometry*>(
                    NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, pmeshNode->GetAtChecked(0))
                );
            }
        }
        if (protationModifier) {
            frotationSpeed = protationModifier->m_fInitialRotSpeed;
        }
        if (!NiObject::DynamicCastStatic(&NiTriStrips::ms_RTTI, prainGeom)) {
            NiTriShapeData* ptrishapedata = static_cast<NiTriShapeData*>(
                NiObject::DynamicCastStatic(&NiTriShapeData::ms_RTTI, prainGeom->GetModelData())
            );
            usinstanceVertices = ptrishapedata->m_usVertices;
            uint16_t usinstanceIndices = ptrishapedata->m_uiTriListLength;
            NiPoint3* pgvertex = ptrishapedata->m_pkVertex;
            NiPoint2* pgtex = ptrishapedata->m_pkTexture;
            uint16_t* pgindex = ptrishapedata->m_pusTriList;
            inuminstances = Min(inuminstances, 0xFFFF / usinstanceVertices);
            uint32_t ivertices = inuminstances * usinstanceVertices;
            NiPoint3* pvertex = new NiPoint3[ivertices];
            NiPoint2* ptex = new NiPoint2[ivertices];
            uint16_t* pusTriList = static_cast<uint16_t*>(
                _NiMalloc(sizeof(uint16_t) * (inuminstances * usinstanceIndices))
            );
            int32_t itri = 0;
            for (int32_t i = 0; i < inuminstances; ++i) {
                int32_t ibase = i * usinstanceVertices;
                for (int32_t iindex = 0; iindex < usinstanceVertices; ++iindex) {
                    int32_t j = iindex + ibase;
                    NiPoint3* psrc = &pgvertex[iindex];
                    NiPoint3* pdst = &pvertex[iindex + ibase];
                    pdst->x = psrc->x;
                    pdst->y = psrc->y;
                    pdst->z = psrc->z;
                    float fv = pgtex[iindex].y;
                    ptex[j].x = pgtex[iindex].x;
                    ptex[j].y = fv;
                }
                for (int32_t iindex = 0; iindex < usinstanceIndices; ++iindex) {
                    pusTriList[itri++] = ibase + pgindex[iindex];
                }
            }
            pnewGeom = new NiTriShape(new NiTriShapeDynamicData(
                inuminstances * usinstanceVertices,
                pvertex,
                nullptr,
                nullptr,
                ptex,
                1,
                NiGeometryData::NBT_METHOD_NONE,
                inuminstances * (usinstanceIndices / 3),
                pusTriList,
                0,
                0
            ));
        }
    }
    else {
        uint32_t imod = 0;
        while (true) {
            NiPSysModifier* pmodifier = apParticleSystem->GetModifierAt(imod++);
            if (!pmodifier) {
                break;
            }
            if (NiObject::IsKindOf_0(&NiPSysRotationModifier::ms_RTTI, pmodifier)) {
                protationModifier = static_cast<NiPSysRotationModifier*>(pmodifier);
                break;
            }
        }
        if (protationModifier) {
            frotationSpeed = protationModifier->m_fInitialRotSpeed;
        }
        prainGeom = apParticleSystem;
        usinstanceVertices = 4;
        inuminstances = Min(inuminstances, 0x3FFF);
        uint32_t inumTriangles = 2 * inuminstances;
        uint32_t inumVertices = 4 * inuminstances;
        NiPoint3* pvertex = new NiPoint3[inumVertices];
        NiPoint2* ptex = new NiPoint2[inumVertices];
        uint16_t* pusTriList = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * inumTriangles));
        for (uint32_t k = 0; k < inuminstances; ++k) {
            float fRadii = pemitter->m_fInitialRadius;
            int32_t ivert = 4 * k;
            NiPoint3 kCorner0;
            NiPoint3 kCorner1;
            NiPoint3 kCorner2;
            NiPoint3 kCorner3;
            pvertex[ivert] = fRadii * *::new (&kCorner0) NiPoint3(-1.0f, 0.0f, -1.0f);
            pvertex[ivert + 1] = fRadii * *::new (&kCorner1) NiPoint3(1.0f, 0.0f, -1.0f);
            pvertex[ivert + 2] = fRadii * *::new (&kCorner2) NiPoint3(1.0f, 0.0f, 1.0f);
            pvertex[ivert + 3] = fRadii * *::new (&kCorner3) NiPoint3(-1.0f, 0.0f, 1.0f);
            NiPoint2 kUV0;
            NiPoint2 kUV1;
            NiPoint2 kUV2;
            NiPoint2 kUV3;
            NiPoint2* puv = ::new (&kUV0) NiPoint2(0.0f, 1.0f);
            ptex[ivert].x = puv->x;
            ptex[ivert].y = puv->y;
            puv = ::new (&kUV1) NiPoint2(1.0f, 1.0f);
            ptex[ivert + 1].x = puv->x;
            ptex[ivert + 1].y = puv->y;
            puv = ::new (&kUV2) NiPoint2(1.0f, 0.0f);
            ptex[ivert + 2].x = puv->x;
            ptex[ivert + 2].y = puv->y;
            puv = ::new (&kUV3) NiPoint2(0.0f, 0.0f);
            ptex[ivert + 3].x = puv->x;
            ptex[ivert + 3].y = puv->y;
            int32_t iindex = 6 * k;
            pusTriList[iindex] = ivert;
            pusTriList[iindex + 1] = ivert + 1;
            pusTriList[iindex + 2] = ivert + 2;
            pusTriList[iindex + 3] = ivert + 3;
            pusTriList[iindex + 4] = ivert;
            pusTriList[iindex + 5] = ivert + 2;
        }
        pnewGeom = new NiTriShape(new NiTriShapeDynamicData(
            inumVertices,
            pvertex,
            nullptr,
            nullptr,
            ptex,
            1,
            NiGeometryData::NBT_METHOD_NONE,
            inumTriangles,
            pusTriList,
            0,
            0
        ));
    }
    if (!pnewGeom) {
        return nullptr;
    }
    double fspeedBase = pemitter->m_fSpeed;
    double fspeedVar = pemitter->m_fSpeedVar;
    float fSpeed = (NiUnitRandom() - 0.5) * fspeedVar + fspeedBase;
    double fdecBase = pemitter->m_fDeclination;
    double fdecVar = pemitter->m_fDeclinationVar;
    float fDec = NiSymmetricRandom() * fdecVar + fdecBase;
    double fplanBase = pemitter->m_fPlanarAngle;
    double fplanVar = pemitter->m_fPlanarAngleVar;
    float fPlan = NiSymmetricRandom() * fplanVar + fplanBase;
    float fSinPlan;
    float fCosPlan;
    float fSinDec;
    float fCosDec;
    NiSinCosQ(fPlan, &fSinPlan, &fCosPlan);
    NiSinCosQ(fDec, &fSinDec, &fCosDec);
    float fdirY = fSinDec * fSinPlan;
    float fdirX = fSinDec * fCosPlan;
    NiPoint3 kDirection;
    ::new (&kDirection) NiPoint3(fdirX, fdirY, fCosDec);
    NiPoint3 kVelocity = fSpeed * kDirection;
    float flifeSpan = pemitter->m_fLifeSpan;
    NiPoint3 kTransformedVelocity;
    NiPSysBoxEmitter* pboxEmitter = nullptr;
    NiPSysCylinderEmitter* pcylEmitter = nullptr;
    NiPSysSphereEmitter* psphereEmitter = nullptr;
    NiPSysMeshEmitter* pmeshEmitter = nullptr;
    if (NiObject::IsKindOf_0(&NiPSysBoxEmitter::ms_RTTI, pemitter)) {
        pboxEmitter = static_cast<NiPSysBoxEmitter*>(NiObject::DynamicCastStatic(&NiPSysBoxEmitter::ms_RTTI, pemitter));
        NiTransform kEmitter = pboxEmitter->m_pkEmitterObj->GetWorldTransform();
        NiTransform kPSys = pboxEmitter->m_pkTarget->GetWorldTransform();
        NiTransform kInvPSys;
        kPSys.Invert(kInvPSys);
        kEmitterToPSys = kInvPSys * kEmitter;
        kTransformedVelocity = kEmitterToPSys.m_Rotate * kVelocity;
        float fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fminZ = BSMIN(kEmitterToPSys.m_Translate.z, fendZ);
        float fminY = -(pboxEmitter->m_fEmitterHeight / 2.0);
        float fminX = -(pboxEmitter->m_fEmitterWidth / 2.0);
        NiPoint3 kTemp;
        kMin = *::new (&kTemp) NiPoint3(fminX, fminY, fminZ);
        fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fmaxZ = BSMAX(kEmitterToPSys.m_Translate.z, fendZ);
        float fmaxY = pboxEmitter->m_fEmitterHeight / 2.0;
        float fmaxX = pboxEmitter->m_fEmitterWidth / 2.0;
        NiPoint3 kTemp2;
        kMax = *::new (&kTemp2) NiPoint3(fmaxX, fmaxY, fmaxZ);
    }
    else if (NiObject::IsKindOf_0(&NiPSysSphereEmitter::ms_RTTI, pemitter)) {
        psphereEmitter = static_cast<NiPSysSphereEmitter*>(
            NiObject::DynamicCastStatic(&NiPSysSphereEmitter::ms_RTTI, pemitter)
        );
        NiTransform kEmitter = psphereEmitter->m_pkEmitterObj->GetWorldTransform();
        NiTransform kPSys = psphereEmitter->m_pkTarget->GetWorldTransform();
        NiTransform kInvPSys;
        kPSys.Invert(kInvPSys);
        kEmitterToPSys = kInvPSys * kEmitter;
        kTransformedVelocity = kEmitterToPSys.m_Rotate * kVelocity;
        float fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fminZ = BSMIN(kEmitterToPSys.m_Translate.z, fendZ);
        float fminY = -psphereEmitter->m_fEmitterRadius;
        float fminX = -psphereEmitter->m_fEmitterRadius;
        NiPoint3 kTemp;
        kMin = *::new (&kTemp) NiPoint3(fminX, fminY, fminZ);
        fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fmaxZ = BSMAX(kEmitterToPSys.m_Translate.z, fendZ);
        float fmaxY = psphereEmitter->m_fEmitterRadius;
        float fmaxX = psphereEmitter->m_fEmitterRadius;
        NiPoint3 kTemp2;
        kMax = *::new (&kTemp2) NiPoint3(fmaxX, fmaxY, fmaxZ);
    }
    else if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pemitter)) {
        pcylEmitter = static_cast<NiPSysCylinderEmitter*>(
            NiObject::DynamicCastStatic(&NiPSysCylinderEmitter::ms_RTTI, pemitter)
        );
        NiTransform kEmitter = pcylEmitter->m_pkEmitterObj->GetWorldTransform();
        NiTransform kPSys = pcylEmitter->m_pkTarget->GetWorldTransform();
        NiTransform kInvPSys;
        kPSys.Invert(kInvPSys);
        kEmitterToPSys = kInvPSys * kEmitter;
        kTransformedVelocity = kEmitterToPSys.m_Rotate * kVelocity;
        float fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fminZ = BSMIN(kEmitterToPSys.m_Translate.z, fendZ);
        float fminY = -pcylEmitter->m_fEmitterRadius;
        float fminX = -pcylEmitter->m_fEmitterRadius;
        NiPoint3 kTemp;
        kMin = *::new (&kTemp) NiPoint3(fminX, fminY, fminZ);
        fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fmaxZ = BSMAX(kEmitterToPSys.m_Translate.z, fendZ);
        float fmaxY = pcylEmitter->m_fEmitterRadius;
        float fmaxX = pcylEmitter->m_fEmitterRadius;
        NiPoint3 kTemp2;
        kMax = *::new (&kTemp2) NiPoint3(fmaxX, fmaxY, fmaxZ);
    }
    else if (NiObject::IsKindOf_0(&NiPSysMeshEmitter::ms_RTTI, pemitter)) {
        pmeshEmitter = static_cast<NiPSysMeshEmitter*>(
            NiObject::DynamicCastStatic(&NiPSysMeshEmitter::ms_RTTI, pemitter)
        );
        NiTransform kEmitter = pmeshEmitter->GetGeometryEmitter(0)->GetWorldTransform();
        NiTransform kPSys = apParticleSystem->GetWorldTransform();
        NiTransform kInvPSys;
        kPSys.Invert(kInvPSys);
        kEmitterToPSys = kInvPSys * kEmitter;
        kTransformedVelocity = kEmitterToPSys.m_Rotate * kVelocity;
        NiBound kMeshBound = pmeshEmitter->GetGeometryEmitter(0)->GetModelBound();
        NiPoint3 kCenter = kMeshBound.m_kCenter;
        float fradius = kMeshBound.m_fRadius;
        float fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fminZ = BSMIN(kEmitterToPSys.m_Translate.z, fendZ);
        float fminY = kCenter.y - fradius;
        float fminX = kCenter.x - fradius;
        NiPoint3 kTemp;
        kMin = *::new (&kTemp) NiPoint3(fminX, fminY, fminZ);
        fendZ = flifeSpan * kTransformedVelocity.z + kEmitterToPSys.m_Translate.z;
        float fmaxZ = BSMAX(kEmitterToPSys.m_Translate.z, fendZ);
        float fmaxY = kCenter.y + fradius;
        float fmaxX = kCenter.x + fradius;
        NiPoint3 kTemp2;
        kMax = *::new (&kTemp2) NiPoint3(fmaxX, fmaxY, fmaxZ);
    }
    else {
        if (pnewGeom) {
            delete pnewGeom;
        }
        return nullptr;
    }
    uint32_t itotalVertices = inuminstances * usinstanceVertices;
    NiPoint3* pinstanceData = new NiPoint3[itotalVertices];
    // The mesh emitter's velocity output, unused.
    NiPoint3 kUnusedVelocity;
    NiPoint3 kFullLifeMovement = flifeSpan * kTransformedVelocity;
    for (int32_t i = 0; i < inuminstances; ++i) {
        NiPoint3 kPosition;
        if (pboxEmitter) {
            double fheight = pboxEmitter->m_fEmitterHeight;
            float fY = (NiUnitRandom() - 0.5) * fheight;
            double fwidth = pboxEmitter->m_fEmitterWidth;
            float fX = (NiUnitRandom() - 0.5) * fwidth;
            NiPoint3 kTemp;
            kPosition = *::new (&kTemp) NiPoint3(fX, fY, 0.0f);
            kPosition = kEmitterToPSys * kPosition;
            float fScalar = NiUnitRandom();
            kPosition = kFullLifeMovement * fScalar + kPosition;
        }
        else if (pcylEmitter) {
            float fCosPhi;
            float fSinPhi;
            double femitterRadius = pcylEmitter->m_fEmitterRadius;
            float fRadius = NiUnitRandom() * femitterRadius;
            float fPhi = NiUnitRandom() * NI_TWO_PI;
            NiSinCos(fPhi, fSinPhi, fCosPhi);
            kPosition.x = fRadius * fCosPhi;
            kPosition.y = fRadius * fSinPhi;
            double fheightScale = NiUnitRandom() - 0.5;
            kPosition.z = pcylEmitter->m_fEmitterHeight * fheightScale;
            kPosition = kEmitterToPSys * kPosition;
            float fScalar = NiUnitRandom();
            kPosition = kFullLifeMovement * fScalar + kPosition;
        }
        else if (psphereEmitter) {
            float fCosTheta;
            float fSinTheta;
            float fCosPhi;
            float fSinPhi;
            double femitterRadius = psphereEmitter->m_fEmitterRadius;
            float fRadius = NiUnitRandom() * femitterRadius;
            float fPhi = NiUnitRandom() * NI_TWO_PI;
            float fTheta = NiUnitRandom() * NI_TWO_PI;
            NiSinCos(fPhi, fSinPhi, fCosPhi);
            NiSinCos(fTheta, fSinTheta, fCosTheta);
            kPosition.x = fRadius * fSinTheta * fCosPhi;
            kPosition.y = fRadius * fSinTheta * fSinPhi;
            kPosition.z = fRadius * fCosTheta;
            kPosition = kEmitterToPSys * kPosition;
            float fScalar = NiUnitRandom();
            kPosition = kFullLifeMovement * fScalar + kPosition;
        }
        else {
            pmeshEmitter->ComputeInitialPositionAndVelocity(kPosition, kUnusedVelocity);
            float fScalar = NiUnitRandom();
            kPosition = kFullLifeMovement * fScalar + kPosition;
        }
        for (int32_t m = 0; m < usinstanceVertices; ++m) {
            pinstanceData[m + i * usinstanceVertices] = kPosition;
        }
    }
    NiAdditionalGeometryData* pdata;
    pdata = new NiAdditionalGeometryData(itotalVertices, 1u, 1);
    pdata->SetDataBlockCount(1u);
    // The data block takes the points as raw bytes.
    pdata->SetDataBlock(
        0,
        static_cast<uint8_t*>(static_cast<void*>(pinstanceData)),
        sizeof(NiPoint3) * itotalVertices,
        true
    );
    delete[] pinstanceData;
    pdata->SetDataStream(0, 0, 0, 3u, itotalVertices, sizeof(NiPoint3), sizeof(NiPoint3));
    pnewGeom->SetAdditionalGeometryData(pdata);
    NiBound kBound = pnewGeom->GetModelBound();
    float fboundRadius = (kMax.x - kMin.x) / 2.0;
    kBound.SetRadius(fboundRadius);
    kBound.SetCenter(0.0f, 0.0f, 0.0f);
    pnewGeom->SetModelBound(kBound);
    NiBillboardNode::FaceMode efaceMode = NiBillboardNode::ROTATE_ABOUT_UP;
    if (NiObject::IsExactKindOf(&NiBillboardNode::ms_RTTI, apParticleSystem->m_pkParent)) {
        NiBillboardNode* pparent = static_cast<NiBillboardNode*>(apParticleSystem->m_pkParent);
        efaceMode = pparent->GetMode();
    }
    if (prainGeom->GetProperty(5)) {
        pnewGeom->AttachProperty(prainGeom->GetProperty(5));
    }
    if (prainGeom->GetProperty(0)) {
        pnewGeom->AttachProperty(prainGeom->GetProperty(0));
    }
    pnewGeom->RemoveProperty(3);
    PrecipitationShaderProperty::PrecipitationType
        eprecipType = static_cast<PrecipitationShaderProperty::PrecipitationType>(abSnow);
    PrecipitationShaderProperty* pPSP;
    pPSP = new PrecipitationShaderProperty(
        inuminstances,
        frotationSpeed,
        kTransformedVelocity,
        kMin,
        kMax,
        eprecipType,
        efaceMode
    );
    pnewGeom->AttachProperty(pPSP);
    pPSP->SetShader(30);
    BSShaderManager::PrepareObject(pnewGeom, 0, 0);
    return pnewGeom;
}

void Precipitation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
