#include "Fallout Shared/TESWater.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSBatchRenderer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectNoise.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceEffectWaterDisplacement.h"
#include "BSShader/ImageSpaceEffectWaterFFT.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceShaderStencilCopyNoTextureNoDepth.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "BSShader/WaterShader.h"
#include "BSShader/WaterShaderProperty.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/Sun.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/WadingWaterData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

SettingT<INISettingCollection> TESWaterSystem::fSurfaceTileSize("fSurfaceTileSize:Water", 2048.0f);
SettingT<INISettingCollection> TESWaterSystem::fTileTextureDivisor("fTileTextureDivisor:Water", 4.75f);
SettingT<INISettingCollection> TESWaterSystem::bUseWaterHDR("bUseWaterHDR", true);
SettingT<RendererSettingCollection> gs_bUseWaterReflectionBlur_Water("bUseWaterReflectionBlur:Water", false);
SettingT<RendererSettingCollection> gs_iWaterBlurAmount_Water("iWaterBlurAmount:Water", 1);
SettingT<INISettingCollection> gs_bReflectExplosions_Water("bReflectExplosions:Water", false);
SettingT<INISettingCollection> gs_bForceHighDetailLandReflections_Water("bForceHighDetailLandReflections:Water", false);
SettingT<INISettingCollection> gs_fWadingWaterQuadSize_Water("fWadingWaterQuadSize:Water", 2048.0f);
SettingT<INISettingCollection> TESWaterSystem::fWadingWaterTextureRes("fWadingWaterTextureRes:Water", 512.0f);
SettingT<INISettingCollection> gs_fWaterGroupHeightRange_Water("fWaterGroupHeightRange:Water", 10.0f);
SettingT<INISettingCollection> gs_uMaxExteriorWaterReflections_Water("uMaxExteriorWaterReflections:Water", 2u);
SettingT<INISettingCollection> gs_uMaxInteriorWaterReflections_Water("uMaxInteriorWaterReflections:Water", 4u);
SettingT<INISettingCollection>
    TESWaterSystem::fExteriorWaterReflectionThreshold("fExteriorWaterReflectionThreshold:Water", 300.0f);
SettingT<INISettingCollection>
    TESWaterSystem::fInteriorWaterReflectionThreshold("fInteriorWaterReflectionThreshold:Water", 10.0f);
NiPointer<NiNode> TESWaterSystem::spWaterRoot;
SettingT<INISettingCollection> gs_bUseBulletWaterDisplacements_Water("bUseBulletWaterDisplacements:Water", true);
SettingT<INISettingCollection> TESWaterSystem::bUseWaterLOD("bUseWaterLOD:Water", true);
SettingT<INISettingCollection> TESWaterSystem::iAnimFPS("uSurfaceFPS:Water", 12u);
SettingT<INISettingCollection> TESWaterSystem::fNearWaterUnderFreq("fNearWaterUnderwaterFreq:Water", 0.3f);
SettingT<INISettingCollection> TESWaterSystem::fNearWaterUnderVolMult("fNearWaterUnderwaterVolume:Water", 0.9f);
NiPointer<BSRenderedTexture> PlaceableWaterGroup::spWadingWaterHeightMap;
NiPointer<BSRenderedTexture> PlaceableWaterGroup::spDepthMap;
NiPointer<BSRenderedTexture> PlaceableWaterGroup::spWorldReflectionMap;
NiPointer<BSRenderedTexture> PlaceableWaterGroup::spSkyReflectionMap;
NiPointer<BSShaderAccumulator> TESWaterSystem::spWaterHeightShaderAccumulator;
NiPointer<BSShaderAccumulator> PlaceableWaterGroup::spSkyReflectionSorter;
NiPointer<NiCamera> TESWaterSystem::spWaterHeightCamera;
NiPointer<NiCamera> PlaceableWaterGroup::spSkyReflectionCamera;
static SettingT<INISettingCollection> sSurfaceTexture("sSurfaceTexture:Water", "water");
SettingT<INISettingCollection> gs_fRefractionWaterPlaneBias_Water("fRefractionWaterPlaneBias:Water", 3.0f);
SettingT<INISettingCollection> TESWaterSystem::bUseWater("bUseWater:Water", true);
SettingT<INISettingCollection> TESWaterSystem::bUseWaterShader("bUseWaterShader:Water", true);
SettingT<INISettingCollection> TESWaterSystem::bUseWaterHiRes("bUseWaterHiRes:Water", false);
SettingT<RendererSettingCollection> TESWaterSystem::bUseWaterDepth("bUseWaterDepth:Water", true);
SettingT<INISettingCollection> TESWaterSystem::bUsePerWorldSpaceWaterNoise("bUsePerWorldSpaceWaterNoise:Water", true);
SettingT<RendererSettingCollection> TESWaterSystem::bUseWaterReflections("bUseWaterReflections:Water", true);
SettingT<RendererSettingCollection> TESWaterSystem::bUseWaterRefractions("bUseWaterRefractions:Water", true);
SettingT<RendererSettingCollection>
    TESWaterSystem::bAutoWaterSilhouetteReflections("bAutoWaterSilhouetteReflections:Water", true);
SettingT<RendererSettingCollection>
    TESWaterSystem::bForceHighDetailReflections("bForceHighDetailReflections:Water", false);
SettingT<INISettingCollection> TESWaterSystem::bForceLowDetailReflections("bForceLowDetailReflections:Water", false);

// The profiling timer around the water update.
static BSPerformanceTimer TimerRenderWater("Renderer", "Water System", true, true);
SettingT<RendererSettingCollection> TESWaterSystem::bUseWaterDisplacements("bUseWaterDisplacements:Water", true);
SettingT<INISettingCollection> TESWaterSystem::iNearWaterRadius("uNearWaterRadius:Water", 1000u);
SettingT<INISettingCollection> TESWaterSystem::iNearWaterPoints("uNearWaterPoints:Water", 8u);
SettingT<INISettingCollection> TESWaterSystem::fNearWaterIndoorTol("fNearWaterIndoorTolerance:Water", 512.0f);
SettingT<INISettingCollection> TESWaterSystem::fNearWaterOutdoorTol("fNearWaterOutdoorTolerance:Water", 1024.0f);
NiTPointerList<NiAVObject*> TESWaterSystem::WaterLevelReflect_Nodes;
NiTPointerList<NiAVObject*> TESWaterSystem::WaterLevelReflect_Empty;
NiTPointerList<NiAVObject*> PlaceableWaterGroup::StaticSkyReflectiveObjects;
NiTPointerList<NiAVObject*> PlaceableWaterGroup::DynamicSkyReflectiveObjects;
NiTPointerList<TESObjectREFR*> PlaceableWaterGroup::ExplosionsList;

using NiTArray_lNiTexturingProperty_nFilterMode_cNiTMallocInterface_lNiTexturingProperty_nFilterMode = NiTArray<
    NiTexturingProperty::FilterMode,
    NiTMallocInterface<NiTexturingProperty::FilterMode>>;

// Returns true only while the renderer is outside a frame.
bool BSRenderedTexture::QOutsideFrame() {
    return NiRenderer::GetRenderer()->GetFrameState() == NiRenderer::FRAMESTATE_OUTSIDE_FRAME;
}

int32_t BSShader::GetShaderType() {
    return iShaderType;
}

void BSShaderAccumulator::SetHasWater(bool abHasWater) {
    bCellHasWater = abHasWater;
}

void BSShaderLightingProperty::SetShaderConstant(int32_t aiConstant, NiColorA aValue) {
    // The table holds NiPoint4s; the four floats of the NiColorA are copied as they are.
    pShaderLightConstants[aiConstant] = NiPoint4{aValue.r, aValue.g, aValue.b, aValue.a};
}

ImageSpaceEffectParam::ImageSpaceEffectParam() : pBorrowTextureTypes(0, 1), pBorrowTextureFilters(0, 1) {}

// The two arrays free their own buffers.
ImageSpaceEffectParam::~ImageSpaceEffectParam() {}

void ImageSpaceEffectParam::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ImageSpaceEffect* ImageSpaceManager::GetEffect(ImageSpaceManager::EffectID aiIndex) {
    return pEffectList[aiIndex];
}

void WaterShader::SetWaterShaderReflectionTexture(NiTexture* ptexture) {
    pReflectionTexture = ptexture;
}

void WaterShader::SetWaterShaderNormalMapTexture(NiTexture* ptexture) {
    pNormalMapTexture = ptexture;
}

void WaterShader::SetWaterShaderDetailTexture(NiTexture* ptexture) {
    pDetailTexture = ptexture;
}

void WaterShader::SetWaterShaderNoiseMap(NiTexture* ptexture) {
    pNoiseMap = ptexture;
}

NiNode* BGSTerrainManager::GetLODLandRoot() {
    return spRootLandLODNode;
}

NiNode* BGSTerrainManager::GetLODObjectRoot() {
    return spRootObjectLODNode;
}

NiNode* BGSTerrainManager::GetTreeNode() {
    return BGSTerrainManager::spTreeNode.m_pObject;
}

NiColor* Sky::GetSunLightColor() {
    return &SkyColor[TESWeather::SUNLIGHT];
}

NiBillboardNode* Sun::GetSunBaseNode() {
    return spSunBaseNode;
}

TESSound::SOUND_DATA TESSound::GetData() {
    return data;
}

TESSound* TESWaterForm::GetWaterSound() {
    return pWaterSound;
}

float TESWaterForm::GetHeightUVScale0() {
    return Data.fHeightUVScale0;
}

// Returns afA only when it is above adB.
double MaxOrEq(float afA, double adB) {
    return afA > adB ? afA : adB;
}

// Returns afA only when it is below adB.
double MinOrEq(float afA, double adB) {
    return afA < adB ? afA : adB;
}

PlaceableWaterGroup::PlaceableWaterGroup()
  : spGroupReflectionMap(nullptr), spWadingWaterGeometry(nullptr), bGroupAtWorldSpaceWaterHeight(false),
    bRenderGroupReflections(false), bRenderSilhouetteReflections(false), spGroupReflectionSorter(nullptr),
    spDepthSorter(nullptr), spReflectionCamera(nullptr), spDepthCamera(nullptr) {
    spGroupReflectionSorter = static_cast<BSShaderAccumulator*>(nullptr);
    spDepthSorter = static_cast<BSShaderAccumulator*>(nullptr);
    spWadingWaterGeometry = static_cast<NiRefObject*>(nullptr);
}

PlaceableWaterGroup::~PlaceableWaterGroup() {
    spGroupReflectionSorter = static_cast<BSShaderAccumulator*>(nullptr);
    spDepthSorter = static_cast<BSShaderAccumulator*>(nullptr);
    TES::pInstance->GetWaterSystem()->ClearReflectionTexturePointers(this, true, false);
    if (pWaterType && pWaterType->spNoiseNormalMap.m_pObject && pWaterType->spNoiseNormalMap->m_uiRefCount == 2) {
        BSRenderedTexture* pTexture = pWaterType->spNoiseNormalMap.m_pObject;
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        pTextureManager->ReturnRenderedTexture(pTexture);
        pWaterType->spNoiseNormalMap = static_cast<BSRenderedTexture*>(nullptr);
    }

    ObjectInWaterList.RemoveAll();
    ActorsInWaterList.RemoveAll();
    if (spWadingWaterGeometry.m_pObject) {
        NiNode* pWaterRoot = TESWaterSystem::GetWaterRoot();
        pWaterRoot->DetachChild(static_cast<NiAVObject*>(spWadingWaterGeometry.m_pObject));
        spWadingWaterGeometry = static_cast<NiRefObject*>(nullptr);
    }
}

void PlaceableWaterGroup::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESWaterSystem::TESWaterSystem()
  : spWaterHeightMapTexture(nullptr), spWaterHeightMapTextureFiltered(nullptr), spWaterNormalMapTexture(nullptr),
    spRainHeightMapTexture(nullptr), spRefractionDepthStencilBuffer(nullptr), spWaterNoiseTexture(nullptr),
    spWaterNormalMap(nullptr), fBlendTimer(0.0f), ReflectionRefMap(37u), DepthRefMap(37u), WaterTypeUpdateMap(37u),
    WadingWaterMap(37u) {
    WaterShader::bDepth = TESWaterSystem::bUseWaterDepth.Bool();
    WaterShaderProperty::bEnabled = false;
    if (!TESWaterSystem::GetWaterRoot()) {
        TESWaterSystem::spWaterRoot = new NiNode(0);
        NiNode* pObjectLODRoot = TES::pInstance->pObjRoot;
        pObjectLODRoot->AttachChild(TESWaterSystem::spWaterRoot.m_pObject, 1);
    }
    WaterShader::kShallowColor = NiColorA(0.0f, 0.5f, 0.5f, 1.0f);
    WaterShader::kDeepColor = NiColorA(0.0f, 0.0f, 0.1f, 1.0f);
    WaterShader::kReflectionColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f);
    TESWaterSystem::pCurrentWaterType = nullptr;
    pBlendToWaterType = nullptr;
    fWaterTypeBlendAmount = 0.0f;
    bUpdateWaterBlend = false;
    spRefractionDepthStencilBuffer = static_cast<NiSourceTexture*>(nullptr);
    fLastDisplaceOffset = NiPoint2::ZERO;
    iAccumulationCount = 0;
    TESWaterSystem::pTag = new NiFixedString("WATER");
    pLODWaterGroup = nullptr;
    fTimeSinceLastRipplePlaced = 0.0f;
    bCull3rdPerson = false;
    TESWaterSystem::uiNumExteriorReflectionsAllowed = gs_uMaxExteriorWaterReflections_Water.Int();
    TESWaterSystem::uiNumInteriorReflectionsAllowed = gs_uMaxInteriorWaterReflections_Water.Int();
}

TESWaterSystem::~TESWaterSystem() {
    DisableWaterSystem(true, true);
    PlaceableWaterGroup::spSkyReflectionSorter = static_cast<BSShaderAccumulator*>(nullptr);
    TESWaterSystem::spWaterHeightShaderAccumulator = static_cast<BSShaderAccumulator*>(nullptr);
    if (PlaceableWaterGroup::spSkyReflectionMap.m_pObject) {
        BSRenderedTexture* pTexture = PlaceableWaterGroup::spSkyReflectionMap.m_pObject;
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        pTextureManager->ReturnRenderedTexture(pTexture);
    }
    PlaceableWaterGroup::spSkyReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
    if (PlaceableWaterGroup::spWorldReflectionMap.m_pObject) {
        BSRenderedTexture* pTexture = PlaceableWaterGroup::spWorldReflectionMap.m_pObject;
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        pTextureManager->ReturnRenderedTexture(pTexture);
    }
    PlaceableWaterGroup::spWorldReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
    PlaceableWaterGroup::spDepthMap = static_cast<BSRenderedTexture*>(nullptr);
    PlaceableWaterGroup::StaticSkyReflectiveObjects.RemoveAll();
    PlaceableWaterGroup::DynamicSkyReflectiveObjects.RemoveAll();
    TESWaterSystem::WaterLevelReflect_Nodes.RemoveAll();
    TESWaterSystem::WaterLevelReflect_Empty.RemoveAll();
    delete TESWaterSystem::pTag;
    TESWaterSystem::pTag = nullptr;
}

void TESWaterSystem::RemoveSubmergedReferenceFromGroup(TESObjectREFR* apRefr, PlaceableWaterGroup* apWaterGroup) {
    void* pIter;
    if (apRefr->IsActor()) {
        pIter = apWaterGroup->ActorsInWaterList.m_pkHead;
    }
    else {
        pIter = apWaterGroup->ObjectInWaterList.m_pkHead;
    }
    void* pFoundPos = nullptr;
    bool bFound = false;
    while (pIter && !bFound) {
        void* pNext;
        if (apRefr->IsActor()) {
            pNext = apWaterGroup->ActorsInWaterList.GetNextPos(static_cast<NiTListItem<Actor*>*>(pIter));
        }
        else {
            pNext = apWaterGroup->ObjectInWaterList.GetNextPos(static_cast<NiTListItem<TESObjectREFR*>*>(pIter));
        }
        TESObjectREFR* pObjRef = nullptr;
        if (apRefr->IsActor()) {
            pObjRef = apWaterGroup->ActorsInWaterList.Get(static_cast<NiTListItem<Actor*>*>(pIter));
        }
        else {
            pObjRef = apWaterGroup->ObjectInWaterList.Get(static_cast<NiTListItem<TESObjectREFR*>*>(pIter));
        }
        if (pObjRef == apRefr) {
            bFound = true;
            pFoundPos = pIter;
        }
        pIter = pNext;
    }
    if (bFound && pFoundPos) {
        if (apRefr->IsActor()) {
            NiTListItem<Actor*>* pActorPos = static_cast<NiTListItem<Actor*>*>(pFoundPos);
            apWaterGroup->ActorsInWaterList.RemovePos(pActorPos);
            WadingWaterData* pData = nullptr;
            if (WadingWaterMap.GetAt(apRefr, pData)) {
                WadingWaterMap.RemoveAt(apRefr);
            }
        }
        else {
            NiTListItem<TESObjectREFR*>* pRefPos = static_cast<NiTListItem<TESObjectREFR*>*>(pFoundPos);
            apWaterGroup->ObjectInWaterList.RemovePos(pRefPos);
        }
    }
}

NiAVObject* TESWaterSystem::GetWaterGeometry(TESObjectREFR* apWaterRef) {
    NiNode* pNode;
    NiAVObject* pChildChildNode;
    NiNode* pChildNode;
    if (!apWaterRef || !apWaterRef->Get3D()) {
        return nullptr;
    }
    BGSPlaceableWater* pBaseForm = static_cast<BGSPlaceableWater*>(apWaterRef->TESObjectREFR::GetObjectReference());
    ++iGlobalGetWaterGeometryCount;
    if (apWaterRef->Get3D()) {
        pNode = apWaterRef->Get3D();
        if (pNode->IsNode()) {
            pNode = apWaterRef->Get3D();
            pChildNode = static_cast<NiNode*>(pNode->GetAt(0));
            if (pChildNode && pChildNode->IsNode()) {
                pChildChildNode = pChildNode->GetAt(0);
                if (!pBaseForm || !pBaseForm->IsAutomaticWater()) {
                    return pChildChildNode;
                }
                if (pChildChildNode && pChildChildNode->IsNode()) {
                    return static_cast<NiNode*>(pChildChildNode)->GetAt(0);
                }
            }
        }
    }
    return nullptr;
}

void TESWaterSystem::DisplayWater(bool abVisible, bool abLOD, bool abInverseCull) {
    NiTListItem<PlaceableWaterGroup*>* pNextGroupPos;
    for (NiTListItem<PlaceableWaterGroup*>* pGroupPos = PlaceableWaterGroupList.m_pkHead; pGroupPos;
         pGroupPos = pNextGroupPos) {
        pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
        NiTListItem<TESObjectREFR*>* pNextWaterPos;
        for (NiTListItem<TESObjectREFR*>* pWaterPos = pGroup->PlaceableWaterList.m_pkHead; pWaterPos;
             pWaterPos = pNextWaterPos) {
            pNextWaterPos = pGroup->PlaceableWaterList.GetNextPos(pWaterPos);
            if (abLOD) {
                TESObjectREFR* pRef = pGroup->PlaceableWaterList.Get(pWaterPos);
                if (static_cast<BGSPlaceableWater*>(pRef->GetObjectReference())->BGSPlaceableWater::IsLODWater()) {
                    if (abInverseCull) {
                        bool bCulled = !pRef->Get3D()->GetAppCulled();
                        pRef->Get3D()->SetAppCulled(bCulled);
                    }
                    else {
                        pRef->Get3D()->SetAppCulled(!abVisible);
                    }
                }
            }
            else {
                TESObjectREFR* pRef = pGroup->PlaceableWaterList.Get(pWaterPos);
                NiNode* pNode = pRef->Get3D();
                if (abInverseCull) {
                    bool bCulled = !pNode->GetAppCulled();
                    pRef->Get3D()->SetAppCulled(bCulled);
                }
                else {
                    pNode->SetAppCulled(!abVisible);
                }
            }
        }
    }
}

PlaceableWaterGroup*
TESWaterSystem::FindWaterGroup(TESObjectREFR* apWaterRef, TESObjectREFR* apObjectRef, float afWaterHeight) {
    NiPoint3 n(0.0f, 0.0f, 1.0f);
    NiPoint3 p = NiPoint3::ZERO;
    TESWaterForm* pWaterType = nullptr;
    if (apWaterRef) {
        if (!apWaterRef->Get3D()) {
            return nullptr;
        }
        n = apWaterRef->Get3D()->GetWorldTransform().m_Rotate * n;
        n.Unitize();
        p = apWaterRef->Get3D()->GetWorldTranslate();
        pWaterType = apWaterRef->GetObjectReference()->GetWaterType();
    }
    else {
        p = NiPoint3(0.0f, 0.0f, afWaterHeight);
        pWaterType = apObjectRef->GetParentCell()->GetWaterType();
    }
    NiPlane reflplane(n, p);
    void* pIter = PlaceableWaterGroupList.m_pkHead;
    NiTListItem<PlaceableWaterGroup*>* pNext = nullptr;
    while (pIter) {
        pNext = PlaceableWaterGroupList.GetNextPos(static_cast<NiTListItem<PlaceableWaterGroup*>*>(pIter));
        if (pIter) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(
                static_cast<NiTListItem<PlaceableWaterGroup*>*>(pIter)
            );
            // GetNormal is a trivial accessor; GetConstant, an x87 getter, is called, the
            // lookup plane's first.
            if (BSUtilities::ComparePoints(&pGroup->ReflectWaterPlane.m_kNormal, &reflplane.m_kNormal, 0.01f)) {
                float fPlaneConstant = reflplane.GetConstant();
                if (BSUtilities::CompareFloat(pGroup->ReflectWaterPlane.GetConstant(), fPlaneConstant, 0.01f) &&
                    pGroup->pWaterType == pWaterType) {
                    if (!apWaterRef) {
                        return pGroup;
                    }
                    NiTListItem<TESObjectREFR*>* pIterWater = pGroup->PlaceableWaterList.m_pkHead;
                    NiTListItem<TESObjectREFR*>* pNextWater = nullptr;
                    while (pIterWater) {
                        pNextWater = pGroup->PlaceableWaterList.GetNextPos(pIterWater);
                        if (pIterWater) {
                            TESObjectREFR* pWaterRef = pGroup->PlaceableWaterList.Get(pIterWater);
                            if (pWaterRef == apWaterRef) {
                                return pGroup;
                            }
                            pIterWater = pNextWater;
                        }
                    }
                }
            }
        }
        pIter = pNext;
    }
    return nullptr;
}

NiTriShapeData* TESWaterSystem::CreateQuadData(
    float afMeshSizeWidth,
    float afMeshSizeHeight,
    uint32_t auiTextureSize,
    uint32_t auiTile,
    bool abNormals,
    bool abColors
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        2988
    );
    NiPoint3* vertices = new NiPoint3[4];
    if (vertices) {
        float fTop = afMeshSizeHeight / 2.0;
        float fRight = afMeshSizeWidth / 2.0;
        float fLeft = -afMeshSizeWidth / 2.0;
        float fBottom = -afMeshSizeHeight / 2.0;
        vertices[0] = NiPoint3(fRight, fTop, 0.0f);
        vertices[1] = NiPoint3(fLeft, fTop, 0.0f);
        vertices[2] = NiPoint3(fLeft, fBottom, 0.0f);
        vertices[3] = NiPoint3(fRight, fBottom, 0.0f);
        static NiPoint3 waterNormal(0.0f, 0.0f, 1.0f);
        NiPoint2* textures = new NiPoint2[4];
        if (textures) {
            uint16_t* indices = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
            if (indices) {
                NiTriShapeData* pData;
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;
                indices[3] = 0;
                indices[4] = 2;
                indices[5] = 3;
                NiPoint3* normals = nullptr;
                if (abNormals) {
                    normals = new NiPoint3[4];
                    if (normals) {
                        normals[0] = waterNormal;
                        normals[1] = waterNormal;
                        normals[2] = waterNormal;
                        normals[3] = waterNormal;
                    }
                }
                NiColorA* pcolors = nullptr;
                if (abColors) {
                    pcolors = new NiColorA[4];
                    if (pcolors) {
                        static SettingT<INISettingCollection> fAlpha("fAlpha:Water", Setting::Info{.f = 1.0f});
                        if (fAlpha.Float() > 1.0) {
                            fAlpha = 1.0f;
                        }
                        else if (fAlpha.Float() < 0.0) {
                            fAlpha = 0.0f;
                        }
                        pcolors[0] = NiColorA(1.0f, 1.0f, 1.0f, fAlpha);
                        pcolors[1] = NiColorA(1.0f, 1.0f, 1.0f, fAlpha);
                        pcolors[2] = NiColorA(1.0f, 1.0f, 1.0f, fAlpha);
                        pcolors[3] = NiColorA(1.0f, 1.0f, 1.0f, fAlpha);
                    }
                }
                // The texture coordinates' upper bound: 1, or the tile count.
                float maxtexcoord = 1.0f;
                if (auiTile != 1) {
                    maxtexcoord = static_cast<float>(auiTile);
                }
                textures[0] = NiPoint2(maxtexcoord, 0.0f);
                textures[1] = NiPoint2(0.0f, 0.0f);
                textures[2] = NiPoint2(0.0f, maxtexcoord);
                textures[3] = NiPoint2(maxtexcoord, maxtexcoord);
                pData = new NiTriShapeData(
                    4,
                    vertices,
                    normals,
                    pcolors,
                    textures,
                    1,
                    NiGeometryData::NBT_METHOD_NONE,
                    2,
                    indices
                );
                if (!pData) {
                    delete[] vertices;
                    delete[] normals;
                    delete[] textures;
                    ourFree(indices);
                    ourFree(pcolors);
                }
                return pData;
            }

            delete[] vertices;
            delete[] textures;
            return nullptr;
        }

        delete[] vertices;
        return nullptr;
    }

    return nullptr;
}

NiTriShapeData*
TESWaterSystem::CreateQuadData(float meshSize, uint32_t textureSize, uint32_t tile, bool bNormals, bool bColors) {
    return CreateQuadData(meshSize, meshSize, textureSize, tile, bNormals, bColors);
}

void TESWaterSystem::SetWaterNoiseTexture(NiTexture* apTexture) {
    // The pointer is stored as it is; the member keeps the more specific type.
    spWaterNoiseTexture = static_cast<NiSourceTexture*>(apTexture);
}

// apNode is unused.
void TESWaterSystem::UpdateWaterSystem(NiCamera* apCamera, NiNode* apNode) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    // pContext is taken from the local buffer's address, not from the constructor's result.
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        279
    );
    iAccumulationCount = 0;
    iGlobalGetWaterGeometryCount = 0;
    if (!pPlayer->GetParentCell()) {
        pContext->~AutoMemContext();
        return;
    }
    if (bSkipNextUpdate) {
        bSkipNextUpdate = false;
        pContext->~AutoMemContext();
        return;
    }
    BSShaderManager::SetCurrentSceneGraph(0);
    TimerRenderWater.Start();
    // Copies the camera position and builds the constant with NiColorA's constructor.
    NiPoint3 kCameraPos = apCamera->GetWorldTranslate();
    NiColorA kEyePos(kCameraPos.x, kCameraPos.y, kCameraPos.z, 0.0f);
    BSShaderLightingProperty::SetShaderConstant(30, kEyePos);
    pWorldCamera = apCamera;
    if (!bWaterEnabled) {
        TimerRenderWater.Stop();
        pContext->~AutoMemContext();
        return;
    }
    BSFadeNode::SetFadeEnabled(false);
    if (pPlayer && pPlayer->GetParentCell()) {
        bool bCameraUnderWater = bForceUnderwater;
        if (!bCameraUnderWater) {
            const NiPoint3& rCameraPos = apCamera->GetWorldTranslate();
            float fWaterHeight = pPlayer->GetParentCell()->GetWaterHeight();
            // A camera above, at or unordered with the water height is not under water.
            bCameraUnderWater = rCameraPos.z < fWaterHeight;
        }
        if (bCameraUnderWater) {
            bUnderWater = true;
            appMain->QDrawWorldAccum()->SetUnderWater(true);
            SkyShader::bUnderWater = true;
            SkyShader::fPlayerDepth = apCamera->GetWorldTranslate().z;
        }
        else {
            bUnderWater = false;
            appMain->QDrawWorldAccum()->SetUnderWater(false);
            SkyShader::bUnderWater = false;
        }
        WaterShader::bUnderWater = bUnderWater;
        int32_t iWaterHeight = static_cast<int32_t>(pPlayer->GetParentCell()->GetWaterHeight());
        appMain->QDrawWorldAccum()->SetCurrentWaterHeight(iWaterHeight);
        appMain->QDrawWorldAccum()->SetHasWater(true);
    }
    else {
        appMain->QDrawWorldAccum()->SetHasWater(false);
    }
    WaterShader::bUnderWater = bUnderWater;
    NiPoint3 kSunDir = Sky::GetInstance()->GetSun()->GetSunBaseNode()->GetParent()->GetTranslate();
    kSunDir.Unitize();
    WaterShader::pSunDir.x = kSunDir.x;
    WaterShader::pSunDir.y = kSunDir.y;
    WaterShader::pSunDir.z = kSunDir.z;
    // The float intensity times the double 100.0, clamped to [0, 1] and stored as a float.
    WaterShader::pSunDir.w = static_cast<float>(
        MinOrEq(1.0f, MaxOrEq(0.0f, Sky::GetInstance()->GetSun()->GetIntensity() * 100.0))
    );
    NiColor kSunColor = *Sky::GetInstance()->GetSunLightColor();
    WaterShader::pSunColor.r = kSunColor.r;
    WaterShader::pSunColor.g = kSunColor.g;
    WaterShader::pSunColor.b = kSunColor.b;
    // Only an intensity exactly equal to 0.0 clears the alpha.
    if (WaterShader::pSunDir.w == 0.0) {
        WaterShader::pSunColor.a = 0.0f;
    }
    else {
        WaterShader::pSunColor.a = WaterShader::pSunDir.w;
    }
    if (GetPlayerUnderwater()) {
        WaterShader::pSunColor.a = 0.0f;
    }
    WaterShader::bInterior = TES::pInstance->GetInterior() != nullptr;
    UpdateLODWater(apCamera, bForceWaterUpdate);
    if (!PlaceableWaterGroupList.IsEmpty() ||
        PlaceableWaterGroup::bRenderSkyReflections ||
        PlaceableWaterGroup::bRenderWorldReflections) {
        UpdatePlaceableWater(apCamera, bForceWaterUpdate);
        bForceWaterUpdate = false;
        if (GetUseWaterShader() &&
            BSShaderManager::GetShaderLevel(BSShaderManager::VertexShader) >= 2 &&
            !PlaceableWaterGroupList.IsEmpty()) {
            if (CheckUpdateActorsInWater()) {
                UpdateActorsInWater();
            }
            UpdateWaterDisplacementSimulation();
        }
        UpdateWaterSounds();
    }
    PlaceableWaterGroup::bRenderWorldReflections = false;
    PlaceableWaterGroup::bRenderSkyReflections = false;
    BSFadeNode::SetFadeEnabled(true);
    TimerRenderWater.Stop();
    pContext->~AutoMemContext();
}

// Marks the LOD water group rendered when one of its planes is visible to apCamera (all of them with abForceUpdate),
// points each such plane's water shader at the world or sky reflection map (or clears the group's reflection textures
// when the player is underwater), clears the reflection map of the planes not drawn, and updates the shader properties.
void TESWaterSystem::UpdateLODWater(NiCamera* apCamera, bool abForceUpdate) {
    if (pLODWaterGroup) {
        if (TES::pInstance->pInteriorCell) {
            Warning("WATER: LOD Water objects still trying to update in an interior when they should not be.");
        }
        else {
            NiTListItem<TESObjectREFR*>* pNextPos;
            pLODWaterGroup->bRenderGroup = false;
            for (NiTListItem<TESObjectREFR*>* pPos = pLODWaterGroup->PlaceableWaterList.m_pkHead; pPos;
                 pPos = pNextPos) {
                pNextPos = pLODWaterGroup->PlaceableWaterList.GetNextPos(pPos);
                TESObjectREFR* pRef = pLODWaterGroup->PlaceableWaterList.Get(pPos);
                if ((pRef->Get3D() && !pRef->Get3D()->GetAppCulled() && IsWaterNodeVisible(pRef, apCamera)) ||
                    abForceUpdate) {
                    pLODWaterGroup->bRenderGroup = true;
                    NiAVObject* pGeometry = GetWaterGeometry(pRef);
                    if (pGeometry) {
                        WaterShaderProperty* pWaterProp = static_cast<WaterShaderProperty*>(
                            pGeometry->GetProperty(NiProperty::SHADE)
                        );
                        if (TESWaterSystem::GetPlayerUnderwater()) {
                            ClearReflectionTexturePointers(pLODWaterGroup, false, false);
                        }
                        else if (pWaterProp->bFullReflections) {
                            PlaceableWaterGroup::bRenderWorldReflections = true;
                            pWaterProp->spReflectionMap = PlaceableWaterGroup::spWorldReflectionMap;
                        }
                        else {
                            PlaceableWaterGroup::bRenderSkyReflections = true;
                            pWaterProp->spReflectionMap = PlaceableWaterGroup::spSkyReflectionMap;
                        }
                    }
                }
                else if (pRef->Get3D()) {
                    NiAVObject* pGeometry = GetWaterGeometry(pRef);
                    if (pGeometry) {
                        WaterShaderProperty* pWaterProp = static_cast<WaterShaderProperty*>(
                            pGeometry->GetProperty(NiProperty::SHADE)
                        );
                        pWaterProp->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                    }
                }
            }
            if (pLODWaterGroup->bRenderGroup) {
                UpdateWaterShaderProperties(pLODWaterGroup);
            }
        }
    }
}

// Marks the placeable water groups apCamera sees (all of them with abForceUpdate), sets up and renders their reflection
// and depth textures, and updates their shader properties; with no group visible it returns the shared depth,
// wading-water and noise textures.
void TESWaterSystem::UpdatePlaceableWater(NiCamera* apCamera, bool abForceUpdate) {
    bool bIsVisible = false;
    iActiveWaterGroups = 0;
    NiTListItem<PlaceableWaterGroup*>* pGroupPos = PlaceableWaterGroupList.GetHeadPos();
    while (pGroupPos) {
        NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        if (pGroupPos) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
            pGroup->bRenderGroup = false;
            pGroup->bRenderGroupReflections = false;
            pGroup->bRenderDepth = false;
            pGroup->bRenderSilhouetteReflections = false;
            NiTListItem<TESObjectREFR*>* pPlanePos = pGroup->PlaceableWaterList.GetHeadPos();
            while (pPlanePos) {
                NiTListItem<TESObjectREFR*>* pNextPlanePos = pGroup->PlaceableWaterList.GetNextPos(pPlanePos);
                if (pPlanePos) {
                    TESObjectREFR* pPlane = pGroup->PlaceableWaterList.Get(pPlanePos);
                    BGSPlaceableWater* pBaseForm = static_cast<BGSPlaceableWater*>(pPlane->GetObjectReference());
                    if (pPlane &&
                        pPlane->Get3D() &&
                        ((pPlane->GetParentCell() && pPlane->GetParentCell()->GetAttached()) ||
                         (pBaseForm->IsLODWater() && !TES::pInstance->GetInterior()))) {
                        NiNode* pPlaneNode = pPlane->Get3D();
                        if ((!pPlaneNode->GetAppCulled() && IsWaterNodeVisible(pPlane, apCamera)) || abForceUpdate) {
                            if (!pGroup->bRenderGroup) {
                                ++iActiveWaterGroups;
                                pGroup->bRenderGroup = true;
                                bIsVisible = true;
                                if (!GetPlayerUnderwater()) {
                                    if (TES::pInstance->GetInterior()) {
                                        pGroup->bRenderGroupReflections = true;
                                    }
                                    else if (pGroup->bGroupAtWorldSpaceWaterHeight) {
                                        PlaceableWaterGroup::bRenderWorldReflections = true;
                                    }
                                    else {
                                        PlaceableWaterGroup::bRenderSkyReflections = true;
                                    }
                                }
                            }
                            if (pBaseForm->IsDepthEnabled() && pBaseForm->GetRefractionEnabled()) {
                                if (!pGroup->ActorsInWaterList.IsEmpty() ||
                                    !pGroup->ObjectInWaterList.IsEmpty() ||
                                    (pPlane->GetExtra()->GetReflectedRefs() &&
                                     !pPlane->GetExtra()->GetReflectedRefs()->IsEmpty()) ||
                                    pBaseForm->GetRefractsLand() ||
                                    (pBaseForm->IsAutomaticWater() &&
                                     !TESObjectCELL::GetAutoWaterReflectList()->IsEmpty())) {
                                    pGroup->bRenderDepth = true;
                                }
                            }
                            if (!TES::pInstance->GetInterior() &&
                                bAutoWaterSilhouetteReflections.Bool() &&
                                (pBaseForm->IsAutomaticWater() ||
                                 bForceLowDetailReflections.Bool() ||
                                 pBaseForm->GetSilhouetteReflections())) {
                                pGroup->bRenderSilhouetteReflections = true;
                            }
                        }
                    }
                    if (!pGroup || (pPlane && !pGroup->bRenderGroup)) {
                        // A hidden plane's water shader drops every texture.
                        NiAVObject* pGeometry = GetWaterGeometry(pPlane);
                        if (pGeometry) {
                            BSShader* pShader = static_cast<NiGeometry*>(pGeometry)->GetShader();
                            if (pShader &&
                                pShader->GetShaderType() ==
                                    static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                                WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                                    pGeometry->GetProperty(NiProperty::SHADE)
                                );
                                pProperty->spReflectionMap = nullptr;
                                pProperty->spRefractionMap = nullptr;
                                pProperty->spNoiseNormalMap = nullptr;
                                pProperty->spDepthMap = nullptr;
                            }
                        }
                    }
                    else {
                        // A visible plane's water shader drops the textures its property or the settings turn off.
                        NiAVObject* pGeometry = GetWaterGeometry(pPlane);
                        if (pGeometry) {
                            BSShader* pShader = static_cast<NiGeometry*>(pGeometry)->GetShader();
                            if (pShader &&
                                pShader->GetShaderType() ==
                                    static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                                WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                                    pGeometry->GetProperty(NiProperty::SHADE)
                                );
                                if (!pProperty->bReflections || !bUseWaterReflections.Bool()) {
                                    pProperty->spReflectionMap = nullptr;
                                }
                                if (!pProperty->bRefractions || !bUseWaterRefractions.Bool()) {
                                    pProperty->spRefractionMap = nullptr;
                                }
                                if (!pProperty->bDepth || !bUseWaterDepth.Bool()) {
                                    pProperty->spDepthMap = nullptr;
                                }
                                pProperty->spNoiseNormalMap = nullptr;
                            }
                        }
                    }
                }
                pPlanePos = pNextPlanePos;
            }
            if (pGroup && !pGroup->bRenderGroup) {
                if (pGroup->spWadingWaterGeometry) {
                    NiAVObject* pWading = NiSmartPointerCast<NiAVObject>(pGroup->spWadingWaterGeometry);
                    WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                        pWading->GetProperty(NiProperty::SHADE)
                    );
                    pProperty->spReflectionMap = nullptr;
                    pProperty->spRefractionMap = nullptr;
                    pProperty->spDepthMap = nullptr;
                    pProperty->spNoiseNormalMap = nullptr;
                }
                ClearReflectionTexturePointers(pGroup, false, false);
            }
            else if (pGroup) {
                if (pGroup->spWadingWaterGeometry) {
                    NiAVObject* pWading = NiSmartPointerCast<NiAVObject>(pGroup->spWadingWaterGeometry);
                    WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                        pWading->GetProperty(NiProperty::SHADE)
                    );
                    if (!pProperty->bReflections || !bUseWaterReflections.Bool()) {
                        pProperty->spReflectionMap = nullptr;
                    }
                    if (!pProperty->bRefractions || !bUseWaterRefractions.Bool()) {
                        pProperty->spRefractionMap = nullptr;
                    }
                    if (!pProperty->bDepth || !bUseWaterDepth.Bool()) {
                        pProperty->spDepthMap = nullptr;
                    }
                    pProperty->spNoiseNormalMap = nullptr;
                }
                if (!bUseWaterReflections.Bool()) {
                    ClearReflectionTexturePointers(pGroup, false, false);
                }
                if (!bUseWaterDepth.Bool()) {
                    if (PlaceableWaterGroup::spDepthMap) {
                        BSRenderedTexture* pDepthMap = PlaceableWaterGroup::spDepthMap;
                        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pDepthMap);
                    }
                    PlaceableWaterGroup::spDepthMap = nullptr;
                }
            }
        }
        pGroupPos = pNextGroupPos;
    }

    if (!bIsVisible && !PlaceableWaterGroup::bRenderWorldReflections && !PlaceableWaterGroup::bRenderSkyReflections) {
        // No water to draw: return the shared textures and each water type's noise normal map.
        if (PlaceableWaterGroup::spDepthMap) {
            BSRenderedTexture* pDepthMap = PlaceableWaterGroup::spDepthMap;
            BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pDepthMap);
        }
        if (PlaceableWaterGroup::spWadingWaterHeightMap) {
            BSRenderedTexture* pHeightMap = PlaceableWaterGroup::spWadingWaterHeightMap;
            BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pHeightMap);
        }
        pGroupPos = PlaceableWaterGroupList.GetHeadPos();
        while (pGroupPos) {
            NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
            if (pGroupPos) {
                PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
                if (pGroup->pWaterType && pGroup->pWaterType->spNoiseNormalMap) {
                    BSRenderedTexture* pNoiseMap = pGroup->pWaterType->spNoiseNormalMap;
                    BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pNoiseMap);
                    pGroup->pWaterType->spNoiseNormalMap = nullptr;
                }
            }
            pGroupPos = pNextGroupPos;
        }
        PlaceableWaterGroup::spDepthMap = nullptr;
        PlaceableWaterGroup::spWadingWaterHeightMap = nullptr;
        return;
    }

    if (!TES::pInstance->GetInterior()) {
        if (PlaceableWaterGroup::bRenderWorldReflections) {
            PlaceableWaterGroup::iWorldReflectionThreadStage = iAccumulationCount++;
            MTSetupWorldReflectionTexture(apCamera);
        }
        if (PlaceableWaterGroup::bRenderSkyReflections) {
            PlaceableWaterGroup::iSkyReflectionThreadStage = iAccumulationCount++;
            MTSetupSkyReflectionTexture(apCamera);
        }
    }

    // Set up each visible group's reflection and depth passes, at most 16 thread stages in all.
    pGroupPos = PlaceableWaterGroupList.GetHeadPos();
    while (pGroupPos) {
        NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        if (pGroupPos) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
            if (pGroup->bRenderGroup && !GetPlayerUnderwater()) {
                ReflectionRefMap.RemoveAll_1();
                DepthRefMap.RemoveAll_1();
                pGroup->StaticDepthObjects.RemoveAll();
                pGroup->DynamicDepthObjects.RemoveAll();
                if (iAccumulationCount + 1 >= 16) {
                    // The format arguments are evaluated last to first.
                    uint32_t uiGroupCount = PlaceableWaterGroupList.GetSize();
                    const char* pWaterTypeName = pGroup->pWaterType->GetFormEditorID();
                    float fHeight = pGroup->ReflectWaterPlane.GetConstant();
                    // The buffer is 260 bytes but 0x100 is passed as its size.
                    char acMessage[260];
                    BSsprintf(
                        acMessage,
                        0x100,
                        "WATER: Can not render water group at height %f using water type %s. "
                        "You are using too many water groups : %d",
                        fHeight,
                        pWaterTypeName,
                        uiGroupCount
                    );
                    Warning("%s", acMessage);
                    pGroup->bRenderGroup = false;
                }
                else {
                    if (bUseWaterReflections.Bool() && pGroup->bRenderGroupReflections) {
                        pGroup->iReflectionThreadStage = iAccumulationCount++;
                        if (bForceHighDetailReflections.Bool() || TES::pInstance->GetInterior()) {
                            MTSetupSelectiveReflectionTexture(apCamera, pGroup, false);
                        }
                        else {
                            MTSetupSelectiveReflectionTexture(apCamera, pGroup, true);
                        }
                    }
                    if (pGroup->bRenderDepth && bUseWaterDepth.Bool()) {
                        pGroup->iDepthThreadStage = iAccumulationCount++;
                        MTSetupSelectiveDepthTexture(apCamera, pGroup);
                    }
                }
            }
        }
        pGroupPos = pNextGroupPos;
    }

    if (!TES::pInstance->GetInterior()) {
        if (PlaceableWaterGroup::bRenderWorldReflections && !GetPlayerUnderwater()) {
            MTRenderWorldReflectionTexture();
        }
        if (PlaceableWaterGroup::bRenderSkyReflections && !GetPlayerUnderwater()) {
            MTRenderSkyReflectionTexture();
        }
        pGroupPos = PlaceableWaterGroupList.GetHeadPos();
        while (pGroupPos) {
            NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
            if (pGroupPos) {
                PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
                if (pGroup->bRenderGroup && !GetPlayerUnderwater()) {
                    UpdateReflectionTexturePointers(pGroup);
                }
                else {
                    ClearReflectionTexturePointers(pGroup, false, false);
                }
            }
            pGroupPos = pNextGroupPos;
        }
    }
    else {
        pGroupPos = PlaceableWaterGroupList.GetHeadPos();
        while (pGroupPos) {
            NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
            if (pGroupPos) {
                PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
                if (pGroup->bRenderGroup && !GetPlayerUnderwater()) {
                    if (bUseWaterReflections.Bool() && pGroup->bRenderGroupReflections) {
                        MTRenderSelectiveReflectionTexture(pGroup);
                    }
                    UpdateReflectionTexturePointers(pGroup);
                }
                else {
                    ClearReflectionTexturePointers(pGroup, false, false);
                }
            }
            pGroupPos = pNextGroupPos;
        }
    }

    // Render the depth of each group that needs it into the shared depth map, clearing it for the first one.
    bool bClearDepth = true;
    pGroupPos = PlaceableWaterGroupList.GetHeadPos();
    NiColorA kBackground(0.0f, 0.0f, 0.0f, 0.0f);
    BSShaderManager::GetRenderer()->GetBackgroundColor(kBackground);
    NiColorA kBlack = NiColorA::BLACK;
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(kBlack);
    // Copies the camera position and builds the constant with NiColorA's constructor.
    NiPoint3 kCameraPos = apCamera->GetWorldTranslate();
    NiColorA kEyePos(kCameraPos.x, kCameraPos.y, kCameraPos.z, 0.0f);
    BSShaderLightingProperty::SetShaderConstant(30, kEyePos);
    while (pGroupPos) {
        NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        if (pGroupPos) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
            if (pGroup->bRenderGroup && bUseWaterDepth.Bool() && pGroup->bRenderDepth && !GetPlayerUnderwater()) {
                if (bClearDepth) {
                    NiRenderTargetGroup* pTarget = PlaceableWaterGroup::spDepthMap->GetGroup();
                    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, pTarget);
                }
                else {
                    NiRenderTargetGroup* pTarget = PlaceableWaterGroup::spDepthMap->GetGroup();
                    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_NONE, pTarget);
                }
                RenderWaterGroupStencil(
                    pGroup->spDepthCamera,
                    pGroup,
                    bClearDepth,
                    static_cast<uint16_t>(pGroup->iStencilBitMask)
                );
                BSBatchRenderer::EndPass();
                ImageSpaceShaderStencilCopyNoTextureNoDepth::uiStencilRef = pGroup->iStencilBitMask;
                // Constructs the parameters on the stack and uses the local's address (the same pointer Construct
                // returns).
                alignas(ImageSpaceShaderParam) uint8_t aucParam[sizeof(ImageSpaceShaderParam)];
                ImageSpaceShaderParam* pParam = ::new (aucParam) ImageSpaceShaderParam;
                pParam->InitConstantMap(0, 1);
                pParam->SetPixelConstant(0, 1.0f, 0.0f, 0.0f, 1.0f);
                BSRenderState::SetStencilFunc(
                    NiStencilProperty::TEST_EQUAL,
                    0xFF,
                    pGroup->iStencilBitMask,
                    BSRS_NOLOCK
                );
                BSRenderedTexture* pDepthMap = PlaceableWaterGroup::spDepthMap;
                NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
                BSShaderManager::GetImageSpaceManager()->RenderEffect(
                    ImageSpaceManager::EFFECT_SHADER_COPY_STENCIL_NO_TEXTURE_NO_DEPTH,
                    pRenderer,
                    pDepthMap,
                    pParam,
                    false
                );
                MTRenderSelectiveDepthTexture(pGroup, static_cast<uint16_t>(pGroup->iStencilBitMask));
                BSRenderedTexture::StopOffscreen();
                bClearDepth = false;
                pParam->~ImageSpaceShaderParam();
            }
        }
        pGroupPos = pNextGroupPos;
    }
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(kBackground);

    WaterTypeUpdateMap.RemoveAll_1();
    pGroupPos = PlaceableWaterGroupList.GetHeadPos();
    while (pGroupPos) {
        NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        if (pGroupPos) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(pGroupPos);
            if (pGroup->bRenderGroup) {
                UpdateWaterShaderProperties(pGroup);
                UpdateWaterNoise(pGroup);
            }
        }
        pGroupPos = pNextGroupPos;
    }
}

bool TESWaterSystem::CheckUpdateActorsInWater() {
    if (TESWaterSystem::GetUseWaterDisplacements()) {
        void* pNextPos;
        for (NiTListItem<PlaceableWaterGroup*>* pPos = PlaceableWaterGroupList.m_pkHead; pPos;
             pPos = static_cast<NiTListItem<PlaceableWaterGroup*>*>(pNextPos)) {
            pNextPos = PlaceableWaterGroupList.GetNextPos(pPos);
            if (ActorWithinRangeOfWadingWater(PlaceableWaterGroupList.Get(pPos))) {
                return true;
            }
        }
    }
    return false;
}

void TESWaterSystem::UpdateActorsInWater() {
    TESObjectCELL* pCell = TES::pInstance->pInteriorCell;
    if (!pCell || pCell->HasWater()) {
        void* iter = PlaceableWaterGroupList.m_pkHead;
        while (iter) {
            void* next = PlaceableWaterGroupList.GetNextPos(static_cast<NiTListItem<PlaceableWaterGroup*>*>(iter));
            if (iter) {
                PlaceableWaterGroup* pgroup = PlaceableWaterGroupList.Get(
                    static_cast<NiTListItem<PlaceableWaterGroup*>*>(iter)
                );
                if (pgroup->bRenderGroup &&
                    !pgroup->ActorsInWaterList.IsEmpty() &&
                    ActorWithinRangeOfWadingWater(pgroup)) {
                    NiTListItem<Actor*>* actoriter = pgroup->ActorsInWaterList.m_pkHead;
                    NiTListItem<Actor*>* actornext = nullptr;
                    while (actoriter) {
                        actornext = pgroup->ActorsInWaterList.GetNextPos(actoriter);
                        if (actoriter) {
                            Actor* pActor = pgroup->ActorsInWaterList.Get(actoriter);
                            TESObjectCELL* pParentCell = pActor->pParentCell;
                            bool bDeep = pActor->IsPointDeepUnderWater(
                                pActor->GetLocationOnReference(),
                                pParentCell,
                                1.0f
                            );
                            if (bDeep || !pActor->IsMoving()) {
                                WadingWaterData* pdata = nullptr;
                                if (WadingWaterMap.GetAt(pActor, pdata)) {
                                    WadingWaterMap.RemoveAt(pActor);
                                }
                            }
                            else {
                                if (!pgroup->spWadingWaterGeometry.m_pObject) {
                                    float fMeshSize = gs_fWadingWaterQuadSize_Water.Float();
                                    NiNode* pWaterRoot = TESWaterSystem::GetWaterRoot();
                                    NiAVObject* pWadingGeometry = CreateWadingWaterGeometry(
                                        pgroup,
                                        pWaterRoot,
                                        fMeshSize
                                    );
                                    pgroup->spWadingWaterGeometry = static_cast<NiRefObject*>(pWadingGeometry);
                                }
                                NiPoint3 kPlayerPos = pPlayer->GetLocationOnReference();
                                NiPoint3 kPosition = pActor->GetLocationOnReference();
                                kPosition.z = pgroup->ReflectWaterPlane.m_fConstant;
                                WadingWaterData* pdata = nullptr;
                                if (!WadingWaterMap.GetAt(pActor, pdata)) {
                                    pdata = new WadingWaterData;
                                    pdata->fLastPosition = kPosition;
                                    WadingWaterMap.SetAt(pActor, pdata);
                                }
                                NiPoint2 kOffset;
                                NiPoint2 kLastOffset;
                                kOffset.x = kPosition.x - kPlayerPos.x;
                                kOffset.y = kPosition.y - kPlayerPos.y;
                                kLastOffset.x = pdata->fLastPosition.x - kPlayerPos.x;
                                kLastOffset.y = pdata->fLastPosition.y - kPlayerPos.y;
                                pdata->fLastPosition = kPosition;
                                pdata->fDisplaceOffset.x = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, kOffset.x);
                                pdata->fDisplaceOffset.y = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, kOffset.y);
                                pdata->fLastDisplaceOffset.x = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, kLastOffset.x);
                                pdata->fLastDisplaceOffset.y = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, kLastOffset.y);
                            }
                        }
                        actoriter = actornext;
                    }
                }
            }
            iter = next;
        }
    }
}

// The first group is fetched before the list is tested, and the search loop fetches the group at the current position,
// not the next.
void TESWaterSystem::UpdateWaterDisplacementSimulation() {
    if (fTimeSinceLastRipplePlaced < 10.0) {
        NiPoint3 oldmeshpos;
        if (!BSRenderedTexture::QOutsideFrame()) {
            BSRenderedTexture::StopOffscreen();
        }
        if (!PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject) {
            PlaceableWaterGroup::spWadingWaterHeightMap = InitializeGreyTexture();
            ImageSpaceEffectWaterDisplacement::bClearSimulation = true;
        }
        NiRenderTargetGroup* pTarget = PlaceableWaterGroup::spWadingWaterHeightMap->GetGroup();
        BSRenderedTexture::StartOffscreen(6u, pTarget);
        if (BSShaderManager::bUseDisplacementWaterStencil) {
            RenderDisplacementWaterStencil();
        }
        if (!BSRenderedTexture::QOutsideFrame()) {
            BSRenderedTexture::StopOffscreen();
        }
        NiTListItem<PlaceableWaterGroup*>* iter = PlaceableWaterGroupList.m_pkHead;
        NiTListItem<PlaceableWaterGroup*>* next = nullptr;
        PlaceableWaterGroup* pgroup = PlaceableWaterGroupList.Get(iter);
        while (iter && !pgroup->spWadingWaterGeometry.m_pObject) {
            next = PlaceableWaterGroupList.GetNextPos(iter);
            if (iter) {
                pgroup = PlaceableWaterGroupList.Get(iter);
            }
            iter = next;
        }
        NiPoint3 playerpos = pPlayer->GetLocationOnReference();
        if (pgroup->spWadingWaterGeometry.m_pObject) {
            oldmeshpos = static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject)->GetTranslate();
        }
        else {
            oldmeshpos = playerpos;
        }
        NiPoint2 dir;
        dir.x = playerpos.x - oldmeshpos.x;
        dir.y = playerpos.y - oldmeshpos.y;
        bool bupdate = false;
        if (std::fabs(dir.x) > 4.0 || std::fabs(dir.y) > 4.0) {
            bupdate = true;
            float fX = playerpos.x / 4.0;
            playerpos.x = std::floor(fX) * 4.0;
            float fY = playerpos.y / 4.0;
            playerpos.y = std::floor(fY) * 4.0;
        }
        iter = PlaceableWaterGroupList.m_pkHead;
        next = nullptr;
        while (iter) {
            next = PlaceableWaterGroupList.GetNextPos(iter);
            if (iter) {
                pgroup = PlaceableWaterGroupList.Get(iter);
                if (pgroup->spWadingWaterGeometry.m_pObject) {
                    // BSTimer::GetSeconds, a trivial accessor of the plane's m_fConstant.
                    playerpos.z = pgroup->ReflectWaterPlane.m_fConstant;
                    NiAVObject* pGeometry = static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject);
                    if (bupdate) {
                        pGeometry->NiAVObject::SetTranslate(playerpos);
                        NiUpdateData UpdateData(0.0f, false, false);
                        pGeometry->NiAVObject::Update(UpdateData);
                    }
                    WaterShaderProperty* pkWadingWaterProp = static_cast<WaterShaderProperty*>(
                        pGeometry->NiAVObject::GetProperty(3)
                    );
                    pkWadingWaterProp->iStencilMask = pgroup->iStencilBitMask;
                    pkWadingWaterProp->spDepthMap = PlaceableWaterGroup::spDepthMap;
                    pkWadingWaterProp->spNoiseNormalMap = pgroup->pWaterType->spNoiseNormalMap;
                    pkWadingWaterProp->spDisplacementNormalMap = PlaceableWaterGroup::spWadingWaterHeightMap
                                                                     ->GetTexture(0);
                }
            }
            iter = next;
        }
        if (bupdate) {
            float fDeltaX = playerpos.x - oldmeshpos.x;
            float fMaxOffset = gs_fWadingWaterQuadSize_Water.Float() / 2.0;
            float fMinOffset = -(gs_fWadingWaterQuadSize_Water.Float() / 2.0);
            ImageSpaceEffectWaterDisplacement::fTextureOffset.x = FLerp(-0.5f, 0.5f, fMinOffset, fMaxOffset, fDeltaX);
            float fDeltaY = playerpos.y - oldmeshpos.y;
            fMaxOffset = gs_fWadingWaterQuadSize_Water.Float() / 2.0;
            fMinOffset = -(gs_fWadingWaterQuadSize_Water.Float() / 2.0);
            ImageSpaceEffectWaterDisplacement::fTextureOffset.y = FLerp(0.5f, -0.5f, fMinOffset, fMaxOffset, fDeltaY);
        }
        else {
            ImageSpaceEffectWaterDisplacement::fTextureOffset.x = 0.0f;
            ImageSpaceEffectWaterDisplacement::fTextureOffset.y = 0.0f;
        }
        if (BSShaderManager::GetShaderLevel(false) >= BSShaderManager::BSSM_SV_2_0) {
            ImageSpaceEffectWaterDisplacement::bWadingWater = true;
            ImageSpaceEffectWaterDisplacement::WadingVariables.r = 0.4f;
            ImageSpaceEffectWaterDisplacement::WadingVariables.g = 0.6f;
            ImageSpaceEffectWaterDisplacement::WadingVariables.b = 0.985f;
            ImageSpaceEffectWaterDisplacement::WadingVariables.a = 0.01f;
            NiTMapItem<TESObjectREFR*, WadingWaterData*>* pos = WadingWaterMap.GetFirstPos();
            while (pos) {
                WadingWaterData* pdata = nullptr;
                TESObjectREFR* pref = nullptr;
                WadingWaterMap.GetNext(pos, pref, pdata);
                ImageSpaceEffectWaterDisplacement::fDisplaceOffset.Add(pdata->fDisplaceOffset);
                ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.Add(pdata->fLastDisplaceOffset);
            }
            if (ImageSpaceEffectWaterDisplacement::fDisplaceOffset.iSize) {
                fTimeSinceLastRipplePlaced = 0.0f;
            }
            else {
                fTimeSinceLastRipplePlaced = BSShaderManager::GetTimer(true) + fTimeSinceLastRipplePlaced;
            }
            BSRenderedTexture* pTexture0 = PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject;
            BSRenderedTexture* pTexture1 = PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject;
            NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
            BSShaderManager::GetImageSpaceManager()->RenderEffect(
                ImageSpaceManager::EFFECT_WATER_DISPLACEMENT,
                pRenderer,
                pTexture1,
                pTexture0,
                nullptr,
                1
            );
            ImageSpaceEffectWaterDisplacement::bWadingWater = false;
        }
    }
    else {
        NiTListItem<PlaceableWaterGroup*>* next;
        for (NiTListItem<PlaceableWaterGroup*>* iter = PlaceableWaterGroupList.m_pkHead; iter; iter = next) {
            next = PlaceableWaterGroupList.GetNextPos(iter);
            PlaceableWaterGroup* pgroup = PlaceableWaterGroupList.Get(iter);
            if (pgroup->spWadingWaterGeometry.m_pObject) {
                NiNode* pWaterRoot = TESWaterSystem::GetWaterRoot();
                pWaterRoot->DetachChild(static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject));
                pgroup->spWadingWaterGeometry = static_cast<NiRefObject*>(nullptr);
            }
        }
        if (PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject) {
            BSRenderedTexture* pTexture = PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
            PlaceableWaterGroup::spWadingWaterHeightMap = static_cast<BSRenderedTexture*>(nullptr);
        }
        ImageSpaceEffectWaterDisplacement::bRanSimulationThisFrame = false;
    }
}

void TESWaterSystem::UpdateWaterSounds() {
    static char bWaterSoundPlaying;
    NiPoint3 playerloc;
    TESObjectCELL* currentCell;
    if (pPlayer) {
        float fNearWaterRadius = static_cast<float>(GetNearWaterRadius());
        currentCell = pPlayer->GetWaterCell(fNearWaterRadius);
    }
    else {
        currentCell = TES::pInstance->GetCurrentCell();
    }
    if (currentCell && currentCell->HasWater()) {
        float underwaterRatio;
        float waterHeight = currentCell->GetWaterHeight();
        NiPoint2 waterDirection = NiPoint2::ZERO;
        if (currentCell->GetInterior()) {
            double nearIndoorTol = GetNearWaterIndoorTol();
            NiPoint3 kPlayerPos = *pPlayer->QLocationOnReference();
            if (TESWaterSystem::GetPlayerUnderwater()) {
                underwaterRatio = 0.0f;
            }
            else if (std::fabs(waterHeight - kPlayerPos.z) < nearIndoorTol) {
                underwaterRatio = (nearIndoorTol - std::fabs(waterHeight - kPlayerPos.z)) / nearIndoorTol;
            }
            else {
                underwaterRatio = 0.0f;
            }
        }
        else {
            float numPoints = static_cast<float>(GetNearWaterPoints());
            float nearRadius = static_cast<float>(GetNearWaterRadius());
            double nearOutdoorTol = GetNearWaterOutdoorTol();
            uint32_t uiNumWaterPoints = 0;
            float afHeight = 0.0f;
            int32_t i = 0;
            int32_t j = 0;
            float tmpVal = nearRadius * 2.0 / (numPoints - 1.0);
            NiPoint3 testPt = NiPoint3::ZERO;
            NiPoint3 kPlayerPos = *pPlayer->QLocationOnReference();
            TESObjectCELL* pcell = nullptr;
            for (i = 0; numPoints > static_cast<double>(i); ++i) {
                for (j = 0; numPoints > static_cast<double>(j); ++j) {
                    testPt = *pPlayer->QLocationOnReference();
                    afHeight = 0.0f;
                    testPt.x = nearRadius * -1.0 + static_cast<double>(i) * tmpVal + testPt.x;
                    testPt.y = nearRadius * -1.0 + static_cast<double>(j) * tmpVal + testPt.y;
                    if (!pcell || !pcell->IsPointInCellExterior(NiPoint3(testPt.x, testPt.y, 0.0f))) {
                        pcell = TES::pInstance->pWorldSpace->GetCellFromWorldCoord(NiPoint3(testPt.x, testPt.y, 0.0f));
                    }
                    float ptWaterHeight;
                    if (pcell) {
                        ptWaterHeight = pcell->GetWaterHeight();
                    }
                    else {
                        ptWaterHeight = -std::numeric_limits<float>::max();
                    }
                    if (TES::pInstance->GetLandHeight(testPt, afHeight) &&
                        ptWaterHeight > static_cast<double>(afHeight)) {
                        ++uiNumWaterPoints;
                        waterDirection.x = waterDirection.x + testPt.x;
                        waterDirection.y = waterDirection.y + testPt.y;
                        if (TESWaterSystem::bDisplayWaterSoundPlacement) {
                            NiTriShape* ppoint = MakeTriPoint(10.0f, NiColorA(0.0f, 1.0f, 0.0f, 0.0f), true);
                            float fLandHeight = afHeight;
                            float exteriorWaterHeight = currentCell->GetWaterHeight();
                            float fPointZ = BSMAX(exteriorWaterHeight, fLandHeight) + 10.0;
                            ppoint->SetTranslate(NiPoint3(testPt.x, testPt.y, fPointZ));
                            TES::pInstance->AddTempDebugObject(ppoint, 10.0f);
                        }
                    }
                    else if (TESWaterSystem::bDisplayWaterSoundPlacement) {
                        NiTriShape* ppoint = MakeTriPoint(10.0f, NiColorA(0.0f, 0.0f, 1.0f, 0.0f), true);
                        float fLandHeight = afHeight;
                        float exteriorWaterHeight = currentCell->GetWaterHeight();
                        float fPointZ = BSMAX(exteriorWaterHeight, fLandHeight) + 10.0;
                        ppoint->SetTranslate(NiPoint3(testPt.x, testPt.y, fPointZ));
                        TES::pInstance->AddTempDebugObject(ppoint, 10.0f);
                    }
                }
            }
            waterDirection /= static_cast<float>(uiNumWaterPoints);
            NiPoint3* pPlayerPos = pPlayer->QLocationOnReference();
            waterDirection.x = waterDirection.x - pPlayerPos->x;
            pPlayerPos = pPlayer->QLocationOnReference();
            waterDirection.y = waterDirection.y - pPlayerPos->y;
            underwaterRatio = static_cast<double>(uiNumWaterPoints) * 2.0 / (numPoints * numPoints);
            if (TESWaterSystem::GetPlayerUnderwater()) {
                underwaterRatio = 0.0f;
            }
            else if (std::fabs(waterHeight - kPlayerPos.z) < nearOutdoorTol) {
                float fFalloff = (nearOutdoorTol - std::fabs(waterHeight - kPlayerPos.z)) / nearOutdoorTol;
                underwaterRatio = fFalloff * underwaterRatio;
            }
            else {
                underwaterRatio = 0.0f;
            }
        }
        if (underwaterRatio > 1.0) {
            underwaterRatio = 1.0f;
        }
        waterDirection.Unitize();
        double dAboveWaterRatio = 1.0 - underwaterRatio;
        float fSoundDistance = static_cast<double>(GetNearWaterRadius()) * dAboveWaterRatio;
        NiPoint2 kScaledDirection = waterDirection * fSoundDistance;
        waterDirection = kScaledDirection;
        NiPoint3* pPlayerPos = pPlayer->QLocationOnReference();
        waterDirection.x = waterDirection.x + pPlayerPos->x;
        pPlayerPos = pPlayer->QLocationOnReference();
        waterDirection.y = waterDirection.y + pPlayerPos->y;
        if (WaterSound.IsValid()) {
            if (TESWaterSystem::bDisplayWaterSoundPlacement) {
                const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
                playerloc.x = kPlayerPos.x;
                playerloc.y = kPlayerPos.y;
                playerloc.z = kPlayerPos.z + 10.0;
                NiPoint3 soundloc(waterDirection.x, waterDirection.y, playerloc.z);
                NiTriShape* ppoint = MakeTriPoint(10.0f, NiColorA(1.0f, 0.0f, 0.0f, 0.0f), true);
                ppoint->SetTranslate(soundloc);
                TES::pInstance->AddTempDebugObject(ppoint, 10.0f);
                NiLines* plines = MakeLine(
                    playerloc,
                    NiColorA(1.0f, 1.0f, 0.0f, 0.0f),
                    soundloc,
                    NiColorA(1.0f, 1.0f, 0.0f, 0.0f),
                    true
                );
                TES::pInstance->AddTempDebugObject(plines, 10.0f);
            }
            float fSoundHeight = currentCell->GetWaterHeight();
            WaterSound.SetPosition(waterDirection.x, waterDirection.y, fSoundHeight);
            if (WaterSound.IsPlaying() && underwaterRatio <= 0.0) {
                bWaterSoundPlaying = 0;
                WaterSound.Stop();
                WaterSound.Release();
            }
            else if (!WaterSound.IsPlaying() && underwaterRatio > 0.0) {
                bWaterSoundPlaying = 1;
                WaterSound.Play(true);
            }
        }
        else if (
            underwaterRatio > 0.0 &&
            pPlayer &&
            pPlayer->GetParentCell() &&
            pPlayer->GetParentCell()->GetWaterType() &&
            pPlayer->GetParentCell()->GetWaterType()->pWaterSound
        ) {
            TESSound* pwtrSound = pPlayer->GetParentCell()->GetWaterType()->pWaterSound;
            uint32_t iflags = TESSound::ConvertLoopFlag(pwtrSound->GetData().iFlags);
            if (pwtrSound->GetData().iFlags & TESSound::FLAG_2D) {
                iflags |= BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_IS_2D_RADIUS;
            }
            else {
                iflags |= BSGameSound::SOUND_FLAG_IS_3D;
            }
            uint32_t uiSoundID = pPlayer->GetParentCell()->GetWaterType()->pWaterSound->iFormID;
            WaterSound = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, iflags);
        }
    }
    else if (bWaterSoundPlaying && WaterSound.IsValid()) {
        bWaterSoundPlaying = 0;
        WaterSound.Stop();
    }
    TESWaterSystem::bDisplayWaterSoundPlacement = false;
}

uint32_t TESWaterSystem::GetNearWaterRadius() {
    return iNearWaterRadius.UInt();
}

uint32_t TESWaterSystem::GetNearWaterPoints() {
    return iNearWaterPoints.UInt();
}

float TESWaterSystem::GetNearWaterIndoorTol() {
    return fNearWaterIndoorTol.Float();
}

float TESWaterSystem::GetNearWaterOutdoorTol() {
    return fNearWaterOutdoorTol.Float();
}

void TESWaterSystem::RemoveTESObjectFromWaterGroup(
    TESObjectREFR* apWaterRef,
    TESObjectREFR* apObjectRef,
    float afWaterHeight
) {
    if (apWaterRef || (apObjectRef && apObjectRef->GetParentCell())) {
        PlaceableWaterGroup* pWaterGroup = FindWaterGroup(apWaterRef, apObjectRef, afWaterHeight);
        if (pWaterGroup) {
            RemoveSubmergedReferenceFromGroup(apObjectRef, pWaterGroup);
        }
    }
}

// The positions in the group's reference lists are NiTListIterator (void*): the actor list or the object list is walked
// through one local, choosing the list by IsActor at each step. When the reference's water zone is one of the group's
// waters, control goes back to the group loop's test without advancing, so the same group is examined again.
void TESWaterSystem::RemoveTESObjectFromAllAutoWaterGroups(TESObjectREFR* apObjectRef) {
    NiTMap<WaterZone*, int32_t>* pWaterZoneMap = apObjectRef->m_Extra.QWaterZoneMap();
    NiTListItem<PlaceableWaterGroup*>* pGroupPos = PlaceableWaterGroupList.m_pkHead;
    NiTListItem<PlaceableWaterGroup*>* pNextGroupPos = nullptr;
    bool bDone = false;
    PlaceableWaterGroup* pGroup = nullptr;
    while (pGroupPos && !bDone) {
        pNextGroupPos = PlaceableWaterGroupList.GetNextPos(pGroupPos);
        if (!pGroupPos) {
            continue;
        }
        bool bRemove = true;
        pGroup = PlaceableWaterGroupList.Get(pGroupPos);
        if (pWaterZoneMap) {
            NiTMapItem<WaterZone*, int32_t>* pZonePos = pWaterZoneMap->GetFirstPos();
            while (pZonePos) {
                WaterZone* pZone;
                int32_t iValue;
                pWaterZoneMap->GetNext(pZonePos, pZone, iValue);
                TESObjectREFR* pZoneRef = pZone->pRef;
                NiTListItem<TESObjectREFR*>* pWaterPos = pGroup->PlaceableWaterList.m_pkHead;
                NiTListItem<TESObjectREFR*>* pNextWaterPos = nullptr;
                while (pWaterPos) {
                    pNextWaterPos = pGroup->PlaceableWaterList.GetNextPos(pWaterPos);
                    if (pWaterPos) {
                        TESObjectREFR* pWater = pGroup->PlaceableWaterList.Get(pWaterPos);
                        if (pWater == pZoneRef) {
                            bRemove = false;
                            break;
                        }
                        pWaterPos = pNextWaterPos;
                    }
                }
                if (!bRemove) {
                    break;
                }
            }
            if (!bRemove) {
                continue;
            }
        }
        if (bRemove) {
            void* pPos;
            if (apObjectRef->IsActor()) {
                pPos = pGroup->ActorsInWaterList.m_pkHead;
            }
            else {
                pPos = pGroup->ObjectInWaterList.m_pkHead;
            }
            void* pNextPos = nullptr;
            void* pFoundPos = nullptr;
            bool bFound = false;
            while (pPos && !bFound) {
                if (apObjectRef->IsActor()) {
                    pNextPos = pGroup->ActorsInWaterList.GetNextPos(static_cast<NiTListItem<Actor*>*>(pPos));
                }
                else {
                    pNextPos = pGroup->ObjectInWaterList.GetNextPos(static_cast<NiTListItem<TESObjectREFR*>*>(pPos));
                }
                if (pPos) {
                    TESObjectREFR* pRef = nullptr;
                    if (apObjectRef->IsActor()) {
                        pRef = pGroup->ActorsInWaterList.Get(static_cast<NiTListItem<Actor*>*>(pPos));
                    }
                    else {
                        pRef = pGroup->ObjectInWaterList.Get(static_cast<NiTListItem<TESObjectREFR*>*>(pPos));
                    }
                    if (pRef == apObjectRef) {
                        bFound = true;
                        pFoundPos = pPos;
                    }
                    pPos = pNextPos;
                }
            }
            if (bFound && pFoundPos) {
                while (apObjectRef->QUnderwater()) {
                    apObjectRef->SetUnderwater(false);
                }
                if (apObjectRef->IsActor()) {
                    NiTListItem<Actor*>* pActorPos = static_cast<NiTListItem<Actor*>*>(pFoundPos);
                    pGroup->ActorsInWaterList.RemovePos(pActorPos);
                    WadingWaterData* pData = nullptr;
                    if (WadingWaterMap.GetAt(apObjectRef, pData)) {
                        WadingWaterMap.RemoveAt(apObjectRef);
                    }
                }
                else {
                    NiTListItem<TESObjectREFR*>* pObjectPos = static_cast<NiTListItem<TESObjectREFR*>*>(pFoundPos);
                    pGroup->ObjectInWaterList.RemovePos(pObjectPos);
                }
            }
        }
        pGroupPos = pNextGroupPos;
    }
}

bool TESWaterSystem::AddWaterRef(TESObjectREFR* apRef, NiPoint3* apPos, NiMatrix3* apRot) {
    // Tests the whole byte Setting::Bool returns, read here as the byte it is.
    if (!static_cast<uint8_t>(TESWaterSystem::bUseWater.Bool())) {
        return false;
    }
    NiPoint3 kNormal(0.0f, 0.0f, 1.0f);
    NiPoint3 kInvNormal(0.0f, 0.0f, -1.0f);
    kNormal = *apRot * kNormal;
    kNormal.Unitize();
    kInvNormal = *apRot * kInvNormal;
    kInvNormal.Unitize();
    NiPlane kPlane(kNormal, *apPos);
    NiPlane kInvPlane(kInvNormal, *apPos);
    TESBoundObject* pPlaceableWater = apRef->GetObjectReference();
    NiTListItem<PlaceableWaterGroup*>* kIter = PlaceableWaterGroupList.m_pkHead;
    NiTListItem<PlaceableWaterGroup*>* pNextPos = nullptr;
    NiTListItem<PlaceableWaterGroup*>* kLastIter = nullptr;
    bool bAddedToGroup = false;
    while (kIter) {
        pNextPos = PlaceableWaterGroupList.GetNextPos(kIter);
        if (kIter) {
            float fGroupHeight;
            float fPlaneHeight;
            float fHeightRange;
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(kIter);
            if (BSUtilities::ComparePoints(&pGroup->ReflectWaterPlane.m_kNormal, &kPlane.m_kNormal, 0.01f) &&
                (fHeightRange = gs_fWaterGroupHeightRange_Water.Float(),
                 fPlaneHeight = kPlane.m_fConstant,
                 fGroupHeight = pGroup->ReflectWaterPlane.m_fConstant,
                 BSUtilities::CompareFloat(fGroupHeight, fPlaneHeight, fHeightRange))) {
                TESWaterForm* pWaterType = pPlaceableWater->GetWaterType();
                if (pWaterType == pGroup->pWaterType) {
                    if (!apRef->IsQuestObject()) {
                        float fPlaneConstant = kPlane.m_fConstant;
                        float fGroupConstant = pGroup->ReflectWaterPlane.m_fConstant;
                        if (!BSUtilities::CompareFloat(fGroupConstant, fPlaneConstant, 0.01f)) {
                            NiPoint3 kPosition = apRef->Get3D()->GetWorldTranslate();
                            kPosition.z = pGroup->ReflectWaterPlane.m_fConstant;
                            apRef->Get3D()->SetTranslate(kPosition);
                            apRef->SetLocationOnReference(kPosition);
                            NiUpdateData kUpdateData(0.0f, false, false);
                            apRef->Get3D()->UpdateWorldData(kUpdateData);
                            if (apRef->GetParentCell()) {
                                apRef->GetParentCell()->SetWaterHeight(kPosition.z);
                            }
                            double dNewHeight = kPosition.z;
                            double dOldHeight = kPlane.m_fConstant;
                            NiNode* p3D = apRef->Get3D();
                            Warning(
                                "WATER: Adjusting water object ('%s') height from %f to closeby water group at "
                                "height : %f",
                                p3D->m_kName.GetHandleString(),
                                dOldHeight,
                                dNewHeight
                            );
                        }
                    }
                    bAddedToGroup = true;
                    UpdateWaterShaderProperties(pGroup, apRef);
                    pGroup->PlaceableWaterList.AddHead(apRef);
                }
            }
            else {
                double dRefPlaneHeight = kPlane.m_fConstant;
                if (pGroup->ReflectWaterPlane.m_fConstant < dRefPlaneHeight) {
                    kLastIter = kIter;
                    break;
                }
            }
        }
        kIter = pNextPos;
    }
    if (!bAddedToGroup) {
        PlaceableWaterGroup* pNewGroup = new PlaceableWaterGroup;
        pNewGroup->ReflectWaterPlane = kPlane;
        pNewGroup->RefractWaterPlane = kInvPlane;
        pNewGroup->pWaterType = pPlaceableWater->GetWaterType();
        UpdateWaterShaderProperties(pNewGroup, apRef);
        pNewGroup->PlaceableWaterList.AddHead(apRef);
        if (kLastIter) {
            PlaceableWaterGroupList.InsertBefore(kLastIter, pNewGroup);
        }
        else {
            PlaceableWaterGroupList.AddTail(pNewGroup);
        }
        if (!TES::pInstance->pInteriorCell && TES::pInstance->pWorldSpace) {
            float fGroupConstant = pNewGroup->ReflectWaterPlane.m_fConstant;
            float fLODWaterHeight = TES::pInstance->pWorldSpace->fLODWaterHeight;
            if (BSUtilities::CompareFloat(fLODWaterHeight, fGroupConstant, 0.01f)) {
                pNewGroup->bGroupAtWorldSpaceWaterHeight = true;
            }
        }
    }
    EnableWaterSystem();
    ResetStencilBitRefs();
    return true;
}

// Adds placeable water apWaterRef to the water groups at its world position and rotation.
bool TESWaterSystem::AddPlaceableWater(TESObjectREFR* apWaterRef) {
    NiMatrix3 rot;
    NiNode* pNode = apWaterRef->Get3D();
    NiPoint3 pos = pNode->GetWorldTranslate();
    NiNode* pNode2 = apWaterRef->Get3D();
    std::memcpy(&rot, &pNode2->GetWorldTransform(), sizeof(rot));
    return AddWaterRef(apWaterRef, &pos, &rot);
}

bool TESWaterSystem::EnableWaterSystem() {
    WaterShaderProperty::bEnabled = true;
    if (TESWaterSystem::bWaterEnabled || !TESWaterSystem::bUseWater.Bool()) {
        return false;
    }
    TESWaterSystem::bWaterEnabled = true;
    DisplayWater(true, false, false);
    return true;
}

bool TESWaterSystem::DisableWaterSystem(bool abFreeMemory, bool abFinal) {
    TESWaterSystem::bWaterEnabled = false;
    WaterShaderProperty::bEnabled = false;
    if (abFreeMemory) {
        NiTListItem<PlaceableWaterGroup*>* next;
        for (NiTListItem<PlaceableWaterGroup*>* iter = PlaceableWaterGroupList.m_pkHead; iter; iter = next) {
            NiTListItem<TESObjectREFR*>* waternext;
            next = PlaceableWaterGroupList.GetNextPos(iter);
            PlaceableWaterGroup* pgroup = PlaceableWaterGroupList.Get(iter);
            if (pgroup->spWadingWaterGeometry.m_pObject) {
                NiAVObject* pobj = static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject);
                WaterShaderProperty* pprop = static_cast<WaterShaderProperty*>(pobj->GetProperty(3));
                pprop->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                pprop->spRefractionMap = static_cast<BSRenderedTexture*>(nullptr);
                pprop->spNoiseNormalMap = static_cast<BSRenderedTexture*>(nullptr);
                pprop->spDepthMap = static_cast<BSRenderedTexture*>(nullptr);
            }
            for (NiTListItem<TESObjectREFR*>* wateriter = pgroup->PlaceableWaterList.m_pkHead; wateriter;
                 wateriter = waternext) {
                waternext = pgroup->PlaceableWaterList.GetNextPos(wateriter);
                TESObjectREFR* pref = pgroup->PlaceableWaterList.Get(wateriter);
                if (pref) {
                    NiTriBasedGeom* pgeom = static_cast<NiTriBasedGeom*>(GetWaterGeometry(pref));
                    if (pgeom) {
                        BSShader* pshader = pgeom->GetShader();
                        if (pshader && pshader->iShaderType == 13) {
                            WaterShaderProperty* pprop = static_cast<WaterShaderProperty*>(pgeom->GetProperty(3));
                            pprop->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                            pprop->spRefractionMap = static_cast<BSRenderedTexture*>(nullptr);
                            pprop->spNoiseNormalMap = static_cast<BSRenderedTexture*>(nullptr);
                            pprop->spDepthMap = static_cast<BSRenderedTexture*>(nullptr);
                        }
                    }
                }
            }
        }
        if (spWaterHeightMapTexture.m_pObject) {
            BSRenderedTexture* pTexture = spWaterHeightMapTexture.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
        }
        if (spWaterNormalMapTexture.m_pObject) {
            BSRenderedTexture* pTexture = spWaterNormalMapTexture.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
        }
        if (spRainHeightMapTexture.m_pObject) {
            BSRenderedTexture* pTexture = spRainHeightMapTexture.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
        }
        if (PlaceableWaterGroup::spDepthMap.m_pObject) {
            BSRenderedTexture* pTexture = PlaceableWaterGroup::spDepthMap.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
        }
        spWaterHeightMapTexture = static_cast<BSRenderedTexture*>(nullptr);
        spWaterNormalMapTexture = static_cast<BSRenderedTexture*>(nullptr);
        spRainHeightMapTexture = static_cast<BSRenderedTexture*>(nullptr);
        spRefractionDepthStencilBuffer = static_cast<NiSourceTexture*>(nullptr);
        PlaceableWaterGroup::spDepthMap = static_cast<BSRenderedTexture*>(nullptr);
        if (PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject) {
            BSRenderedTexture* pTexture = PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
        }
        PlaceableWaterGroup::spWadingWaterHeightMap = static_cast<BSRenderedTexture*>(nullptr);
        if (WaterShader::GetInstance()) {
            WaterShader::GetInstance()->SetWaterShaderNormalMapTexture(nullptr);
            WaterShader::GetInstance()->SetWaterShaderReflectionTexture(nullptr);
            WaterShader::GetInstance()->SetWaterShaderDetailTexture(nullptr);
        }
        if (ImageSpaceEffectWaterFFT::GetInstance()) {
            ImageSpaceEffectWaterFFT::GetInstance()->FreeUpTextureMemory();
        }
        // The game then calls the empty BSShaderProperty::PrecacheTextures on the instance.
        if (ImageSpaceEffectWaterDisplacement::GetInstance()) {
            ImageSpaceEffectWaterDisplacement::GetInstance();
        }
        if (WaterSound.IsValid()) {
            WaterSound.Release();
        }
        if (WaterShader::GetInstance()) {
            WaterShader::GetInstance()->SetWaterShaderNoiseMap(nullptr);
        }
    }
    TESWaterSystem::iActiveWaterGroups = 0;
    if (!abFinal) {
        DisplayWater(false, false, false);
    }
    return true;
}

bool TESWaterSystem::IsWaterNodeVisible(TESObjectREFR* apWaterRef, NiCamera* apCamera) {
    NiNode* pNode = apWaterRef->Get3D();
    if (!pNode) {
        return false;
    }
    NiAVObject* pChildNode = pNode->GetAt(0);
    if (!NiObject::IsExactKindOf(&BSMultiBoundNode::ms_RTTI, pChildNode)) {
        return NodeInFrustum(pNode, apCamera);
    }
    BSMultiBoundNode* pMultiBoundNode = static_cast<BSMultiBoundNode*>(pNode->GetAt(0));
    BSMultiBound* pMultiBound = pMultiBoundNode->QMultiBound();
    BSMultiBoundShape* pShape = pMultiBound->QShape();
    return pShape->kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE;
}

bool TESWaterSystem::RemovePlaceableWater(TESObjectREFR* apRef, NiPoint3* apPos, NiMatrix3* apRot) {
    if (TESWaterSystem::bWaterEnabled && TESWaterSystem::bUseWater.Bool()) {
        NiPoint3 kNormal(0.0f, 0.0f, 1.0f);
        NiPoint3 kPos = NiPoint3::ZERO;
        kNormal = *apRot * kNormal;
        kNormal.Unitize();
        kPos = *apPos;
        NiPlane kPlane(kNormal, kPos);
        NiTListItem<PlaceableWaterGroup*>* kIter = PlaceableWaterGroupList.m_pkHead;
        NiTListItem<PlaceableWaterGroup*>* kNextIter = nullptr;
        while (kIter) {
            kNextIter = PlaceableWaterGroupList.GetNextPos(kIter);
            if (kIter) {
                PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(kIter);
                if (BSUtilities::ComparePoints(&pGroup->ReflectWaterPlane.m_kNormal, &kPlane.m_kNormal, 0.01f)) {
                    float fPlaneConstant = kPlane.m_fConstant;
                    float fGroupConstant = pGroup->ReflectWaterPlane.m_fConstant;
                    if (BSUtilities::CompareFloat(fGroupConstant, fPlaneConstant, 0.01f)) {
                        NiTListItem<TESObjectREFR*>* kWaterIter = pGroup->PlaceableWaterList.m_pkHead;
                        NiTListItem<TESObjectREFR*>* kNextWater = nullptr;
                        while (kWaterIter) {
                            kNextWater = pGroup->PlaceableWaterList.GetNextPos(kWaterIter);
                            if (kWaterIter) {
                                TESObjectREFR* pWaterRef = pGroup->PlaceableWaterList.Get(kWaterIter);
                                if (pWaterRef == apRef) {
                                    pGroup->PlaceableWaterList.RemovePos(kWaterIter);
                                    NiAVObject* pWaterGeometry = GetWaterGeometry(pWaterRef);
                                    if (pWaterGeometry) {
                                        WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                                            pWaterGeometry->GetProperty(3)
                                        );
                                        pProperty->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                                        pProperty->spRefractionMap = static_cast<BSRenderedTexture*>(nullptr);
                                        pProperty->spNoiseNormalMap = static_cast<BSRenderedTexture*>(nullptr);
                                        pProperty->spDepthMap = static_cast<BSRenderedTexture*>(nullptr);
                                    }
                                    if (!pGroup->PlaceableWaterList.m_uiCount) {
                                        PlaceableWaterGroupList.RemovePos(kIter);
                                        if (pGroup) {
                                            delete pGroup;
                                        }
                                        pGroup = nullptr;
                                    }
                                    if (PlaceableWaterGroupList.IsEmpty()) {
                                        DisableWaterSystem(true, false);
                                    }
                                    ResetStencilBitRefs();
                                    return true;
                                }
                                kWaterIter = kNextWater;
                            }
                        }
                    }
                }
            }
            kIter = kNextIter;
        }
    }
    return false;
}

void TESWaterSystem::RemoveWaterRefFromWaterSystem(TESObjectREFR* apRef) {
    NiMatrix3 kRot;
    NiPoint3 kPos = apRef->Get3D()->GetWorldTranslate();
    std::memcpy(&kRot, &apRef->Get3D()->GetWorldTransform(), sizeof(kRot));
    RemovePlaceableWater(apRef, &kPos, &kRot);
}

// Updates the shader properties of every water plane in apWaterGroup.
void TESWaterSystem::UpdateWaterShaderProperties(PlaceableWaterGroup* apWaterGroup) {
    NiTListItem<TESObjectREFR*>* pNextPos;
    for (NiTListItem<TESObjectREFR*>* pPos = apWaterGroup->PlaceableWaterList.m_pkHead; pPos; pPos = pNextPos) {
        pNextPos = apWaterGroup->PlaceableWaterList.GetNextPos(pPos);
        if (pPos) {
            UpdateWaterShaderProperties(apWaterGroup, apWaterGroup->PlaceableWaterList.Get(pPos));
        }
    }
}

// For each water plane of apWaterGroup whose water type has a noise texture: loads the plane's noise height map (the
// shared one with bUsePerWorldSpaceWaterNoise, else the type's), borrows the type's noise normal map, and once per
// water type and frame scrolls the noise and renders the noise normal map.
void TESWaterSystem::UpdateWaterNoise(PlaceableWaterGroup* apWaterGroup) {
    NiTListItem<TESObjectREFR*>* pPos = apWaterGroup->PlaceableWaterList.m_pkHead;
    while (pPos) {
        NiTListItem<TESObjectREFR*>* pNextPos = apWaterGroup->PlaceableWaterList.GetNextPos(pPos);
        TESObjectREFR* pRef = apWaterGroup->PlaceableWaterList.Get(pPos);
        BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(pRef->GetObjectReference());
        TESWaterForm* pType = pWater->GetWaterType();
        if (pType && pType->GetNoiseTextureNameLength()) {
            if (!ImageSpaceEffectNoise::GetInstance()->IsLoaded()) {
                ImageSpaceEffectNoise::GetInstance()->ShufflePermuationTable();
            }
            NiTriBasedGeom* pGeometry = static_cast<NiTriBasedGeom*>(GetWaterGeometry(pRef));
            if (pGeometry) {
                BSShader* pShader = pGeometry->GetShader();
                if (pShader && pShader->iShaderType == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                    WaterShaderProperty* pProp = static_cast<WaterShaderProperty*>(
                        pGeometry->GetProperty(NiProperty::SHADE)
                    );
                    if (TESWaterSystem::bUsePerWorldSpaceWaterNoise.Bool()) {
                        if (!pProp->spNoiseHeightMap.m_pObject) {
                            GetWaterNoiseTexture(pProp->spNoiseHeightMap);
                        }
                    }
                    else if (!pProp->spNoiseHeightMap.m_pObject || pType->ShouldResetNoiseTextures()) {
                        NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(nullptr));
                        pType->GetNoiseTexture(spTempTexture);
                        if (spTempTexture.m_pObject) {
                            pType->GetNoiseTexture(pProp->spNoiseHeightMap);
                        }
                        else {
                            TES::pInstance->CreateTextureImage(
                                pType->GetNoiseTextureName(),
                                reinterpret_cast<NiPointer<NiSourceTexture>&>(pProp->spNoiseHeightMap),
                                true,
                                false
                            );
                        }
                        pType->bResetNoiseTexture = false;
                    }
                    if (pProp->spNoiseHeightMap.m_pObject && !pType->spNoiseNormalMap.m_pObject) {
                        NiTexture* pHeightMap = pProp->spNoiseHeightMap.m_pObject;
                        BSShaderManager::iWaterNoiseWidth = pHeightMap->GetWidth();
                        pHeightMap = pProp->spNoiseHeightMap.m_pObject;
                        BSShaderManager::iWaterNoiseHeight = pHeightMap->GetHeight();
                        NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
                        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
                        BSRenderedTexture* pNormalMap = pTextureManager->BorrowRenderedTexture(
                            pRenderer,
                            BSTextureManager::TEX_TESWATER_NOISE_NORMALMAP,
                            0,
                            nullptr,
                            0
                        );
                        pType->spNoiseNormalMap = pNormalMap;
                    }

                    pProp->spNoiseNormalMap = pType->spNoiseNormalMap;
                    bool bfound = false;
                    if (!WaterTypeUpdateMap.GetAt(pType, bfound)) {
                        if (pProp->spNoiseHeightMap.m_pObject) {
                            ImageSpaceEffectNoise::fNoiseScale = pProp->fNoiseScale;
                            float ftime = BSShaderManager::GetTimer(true);
                            float dir0 = pType->Data.fNoiseWindDirection0;
                            float dir1 = pType->Data.fNoiseWindDirection1;
                            float dir2 = pType->Data.fNoiseWindDirection2;
                            float f0 = dir0 * 0.01745329238474369;
                            double dSin0 = std::sin(f0);
                            double dSpeed0 = pType->Data.fNoiseWindSpeed0;
                            pType->fTexScroll0.x = dSpeed0 * ftime * dSin0 + pType->fTexScroll0.X();
                            double dCos0 = std::cos(f0);
                            dSpeed0 = pType->Data.fNoiseWindSpeed0;
                            pType->fTexScroll0.y = dSpeed0 * ftime * dCos0 + pType->fTexScroll0.Y();
                            float f1 = dir1 * 0.01745329238474369;
                            double dSin1 = std::sin(f1);
                            double dSpeed1 = pType->Data.fNoiseWindSpeed1;
                            pType->fTexScroll1.x = dSpeed1 * ftime * dSin1 + pType->fTexScroll1.X();
                            double dCos1 = std::cos(f1);
                            dSpeed1 = pType->Data.fNoiseWindSpeed1;
                            pType->fTexScroll1.y = dSpeed1 * ftime * dCos1 + pType->fTexScroll1.Y();
                            float f2 = dir2 * 0.01745329238474369;
                            double dSin2 = std::sin(f2);
                            double dSpeed2 = pType->Data.fNoiseWindSpeed2;
                            pType->fTexScroll2.x = dSpeed2 * ftime * dSin2 + pType->fTexScroll2.X();
                            double dCos2 = std::cos(f2);
                            dSpeed2 = pType->Data.fNoiseWindSpeed2;
                            pType->fTexScroll2.y = dSpeed2 * ftime * dCos2 + pType->fTexScroll2.Y();
                            BSWrap(pType->fTexScroll0.x, 0.0f, 1.0f);
                            BSWrap(pType->fTexScroll0.y, 0.0f, 1.0f);
                            BSWrap(pType->fTexScroll1.x, 0.0f, 1.0f);
                            BSWrap(pType->fTexScroll1.y, 0.0f, 1.0f);
                            BSWrap(pType->fTexScroll2.x, 0.0f, 1.0f);
                            BSWrap(pType->fTexScroll2.y, 0.0f, 1.0f);
                            // Copies each NiPoint4 scroll into the NiColorA constant member by member.
                            ImageSpaceEffectNoise::fTexScroll0.r = pType->fTexScroll0.x;
                            ImageSpaceEffectNoise::fTexScroll0.g = pType->fTexScroll0.y;
                            ImageSpaceEffectNoise::fTexScroll0.b = pType->fTexScroll0.z;
                            ImageSpaceEffectNoise::fTexScroll0.a = pType->fTexScroll0.w;
                            ImageSpaceEffectNoise::fTexScroll1.r = pType->fTexScroll1.x;
                            ImageSpaceEffectNoise::fTexScroll1.g = pType->fTexScroll1.y;
                            ImageSpaceEffectNoise::fTexScroll1.b = pType->fTexScroll1.z;
                            ImageSpaceEffectNoise::fTexScroll1.a = pType->fTexScroll1.w;
                            ImageSpaceEffectNoise::fTexScroll2.r = pType->fTexScroll2.x;
                            ImageSpaceEffectNoise::fTexScroll2.g = pType->fTexScroll2.y;
                            ImageSpaceEffectNoise::fTexScroll2.b = pType->fTexScroll2.z;
                            ImageSpaceEffectNoise::fTexScroll2.a = pType->fTexScroll2.w;
                            float fScale0 = pType->GetHeightUVScale0() / 100.0;
                            float fCeil0 = std::ceil(fScale0);
                            ImageSpaceEffectNoise::fTexScale.r = BSMAX(1.0f, fCeil0);
                            float fScale1 = pType->Data.fHeightUVScale1 / 100.0;
                            float fCeil1 = std::ceil(fScale1);
                            ImageSpaceEffectNoise::fTexScale.g = BSMAX(1.0f, fCeil1);
                            float fScale2 = pType->Data.fHeightUVScale2 / 100.0;
                            float fCeil2 = std::ceil(fScale2);
                            ImageSpaceEffectNoise::fTexScale.b = BSMAX(1.0f, fCeil2);
                            ImageSpaceEffectNoise::fAmplitude.r = pType->Data.fAmplitude0;
                            ImageSpaceEffectNoise::fAmplitude.g = pType->Data.fAmplitude1;
                            ImageSpaceEffectNoise::fAmplitude.b = pType->Data.fAmplitude2;
                            BSRenderedTexture* pNormalMap = pType->spNoiseNormalMap.m_pObject;
                            NiTexture* pHeightMap = pProp->spNoiseHeightMap.m_pObject;
                            NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
                            BSShaderManager::GetImageSpaceManager()->RenderEffect(
                                ImageSpaceManager::EFFECT_NOISE,
                                pRenderer,
                                pHeightMap,
                                pNormalMap,
                                nullptr,
                                1
                            );
                        }
                        WaterTypeUpdateMap.SetAt(pType, true);
                    }
                }
            }
        }

        pPos = pNextPos;
    }
}

void TESWaterSystem::UpdateReflectionTexturePointers(PlaceableWaterGroup* apGroup) {
    NiTListItem<TESObjectREFR*>* pIter = apGroup->PlaceableWaterList.m_pkHead;
    while (pIter) {
        TESObjectREFR* pRef = apGroup->PlaceableWaterList.GetNext(pIter);
        BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(pRef->GetObjectReference());
        if (pRef &&
            ((pRef->GetParentCell() && pRef->GetParentCell()->GetAttached()) ||
             (pWater && pWater->IsLODWater() && !TES::pInstance->pInteriorCell))) {
            NiAVObject* pObj = GetWaterGeometry(pRef);
            if (pObj) {
                NiProperty* pProp = pObj->GetProperty(NiProperty::SHADE);
                WaterShaderProperty* pWaterProp = nullptr;
                if (NiObject::IsExactKindOf(&WaterShaderProperty::ms_RTTI, pProp)) {
                    pWaterProp = static_cast<WaterShaderProperty*>(pProp);
                }
                if (pWaterProp) {
                    if (TES::pInstance->pInteriorCell) {
                        pWaterProp->spReflectionMap = apGroup->spGroupReflectionMap;
                    }
                    else if (apGroup->bGroupAtWorldSpaceWaterHeight) {
                        pWaterProp->spReflectionMap = PlaceableWaterGroup::spWorldReflectionMap;
                    }
                    else {
                        pWaterProp->spReflectionMap = PlaceableWaterGroup::spSkyReflectionMap;
                    }
                }
            }
        }
    }
    if (apGroup->spWadingWaterGeometry.m_pObject) {
        NiAVObject* pWading = static_cast<NiAVObject*>(apGroup->spWadingWaterGeometry.m_pObject);
        WaterShaderProperty* pWadingProp = static_cast<WaterShaderProperty*>(pWading->GetProperty(NiProperty::SHADE));
        if (TES::pInstance->pInteriorCell) {
            pWadingProp->spReflectionMap = apGroup->spGroupReflectionMap;
        }
        else if (apGroup->bGroupAtWorldSpaceWaterHeight) {
            pWadingProp->spReflectionMap = PlaceableWaterGroup::spWorldReflectionMap;
        }
        else {
            pWadingProp->spReflectionMap = PlaceableWaterGroup::spSkyReflectionMap;
        }
    }
}

// Reads neither abRemovingGroup nor abRecurse.
void TESWaterSystem::ClearReflectionTexturePointers(
    PlaceableWaterGroup* apGroup,
    bool abRemovingGroup,
    bool abRecurse
) {
    if (apGroup) {
        NiTListItem<TESObjectREFR*>* pNextWater;
        if (apGroup->spGroupReflectionMap.m_pObject) {
            BSRenderedTexture* pTexture = apGroup->spGroupReflectionMap.m_pObject;
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            pTextureManager->ReturnRenderedTexture(pTexture);
            apGroup->spGroupReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
        }
        for (NiTListItem<TESObjectREFR*>* pIterWater = apGroup->PlaceableWaterList.m_pkHead; pIterWater;
             pIterWater = pNextWater) {
            pNextWater = apGroup->PlaceableWaterList.GetNextPos(pIterWater);
            TESObjectREFR* pWaterRef = apGroup->PlaceableWaterList.Get(pIterWater);
            if (pWaterRef && pWaterRef->Get3D()) {
                NiTriBasedGeom* pGeom = static_cast<NiTriBasedGeom*>(GetWaterGeometry(pWaterRef));
                if (pGeom) {
                    BSShader* pShader = pGeom->GetShader();
                    if (pShader &&
                        pShader->iShaderType == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                        WaterShaderProperty* pProp = static_cast<WaterShaderProperty*>(
                            pGeom->GetProperty(NiProperty::SHADE)
                        );
                        pProp->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                    }
                }
            }
        }
        if (apGroup->spWadingWaterGeometry.m_pObject) {
            NiAVObject* pWading = static_cast<NiAVObject*>(apGroup->spWadingWaterGeometry.m_pObject);
            WaterShaderProperty* pWadingProp = static_cast<WaterShaderProperty*>(
                pWading->GetProperty(NiProperty::SHADE)
            );
            pWadingProp->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
        }
    }
}

// Sets up apWaterGroup's reflection camera and sorter, collects the objects its water reflects into the group's static
// and dynamic lists, and queues the accumulation on the first rendering thread. The explosion pass deliberately looks
// at the list's first explosion only: it takes the next position but never loops.
void TESWaterSystem::MTSetupSelectiveReflectionTexture(
    NiCamera* apCamera,
    PlaceableWaterGroup* apWaterGroup,
    bool abLOD
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        3835
    );
    if (!TESWaterSystem::GetUseWaterShader() || !apCamera) {
        return;
    }
    NiCamera* pNewCamera = new NiCamera;
    apWaterGroup->spReflectionCamera = pNewCamera;
    BSUtilities::ReflectCameraAboutArbitraryPlane(
        apCamera,
        &apWaterGroup->ReflectWaterPlane,
        apWaterGroup->spReflectionCamera.m_pObject
    );
    if (!apWaterGroup->spGroupReflectionSorter.m_pObject) {
        BSShaderAccumulator* pNewSorter = new BSShaderAccumulator(99, 1u, 759u);
        apWaterGroup->spGroupReflectionSorter = pNewSorter;
    }
    ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
    apWaterGroup->spGroupReflectionSorter->SetShadowSceneNode(pSceneNode);
    apWaterGroup->spGroupReflectionSorter->SetWaterReflection(true);
    NiAccumulator* pAccumulator = apWaterGroup->spGroupReflectionSorter.m_pObject;
    pAccumulator->StartAccumulating(apWaterGroup->spReflectionCamera.m_pObject);
    // SetAccumulate is a trivial one-byte setter, so bAccumulate is written directly.
    apWaterGroup->spGroupReflectionSorter->bAccumulate = true;
    if (apWaterGroup->bRenderSilhouetteReflections) {
        apWaterGroup->spGroupReflectionSorter->SetRenderMode(BSShaderManager::BSSM_RENDER_SILHOUETTE);
        // SkyColor[3] is the ambient light color; the fog color is the fog property's m_kColor.
        NiColor kAmbientColor = TES::pInstance->pSky->SkyColor[3];
        ShadowSceneNode* pFogSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        NiColor kFogColor = pFogSceneNode->GetFogProperty()->m_kColor;
        float fBlue = kAmbientColor.b * kFogColor.b;
        float fGreen = kAmbientColor.g * kFogColor.g;
        float fRed = kAmbientColor.r * kFogColor.r;
        NiColorA kSilhouetteColor(fRed, fGreen, fBlue, 1.0f);
        apWaterGroup->spGroupReflectionSorter->SetSilhouetteColor(kSilhouetteColor);
    }
    bool bRenderSky = false;
    bool bRenderLand = false;
    bool bRenderLODLand = false;
    bool bRenderLODBuildings = false;
    bool bRenderLODTrees = false;
    apWaterGroup->StaticReflectiveObjects.RemoveAll();
    apWaterGroup->DynamicReflectiveObjects.RemoveAll();
    NiTListItem<TESObjectREFR*>* pWaterPos = apWaterGroup->PlaceableWaterList.m_pkHead;
    while (pWaterPos) {
        TESObjectREFR* pRef = apWaterGroup->PlaceableWaterList.GetNext(pWaterPos);
        BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(pRef->GetObjectReference());
        if (!pRef) {
            continue;
        }
        if (!(pRef->GetParentCell() && pRef->GetParentCell()->GetAttached()) &&
            !(pWater && pWater->IsLODWater() && !TES::pInstance->pInteriorCell)) {
            continue;
        }
        WaterShaderProperty* pProperty = nullptr;
        NiAVObject* pGeometry = GetWaterGeometry(pRef);
        if (pGeometry) {
            NiProperty* pShaderProperty = pGeometry->GetProperty(3);
            if (NiObject::IsExactKindOf(&WaterShaderProperty::ms_RTTI, pShaderProperty)) {
                pProperty = static_cast<WaterShaderProperty*>(pShaderProperty);
            }
        }
        if (!pProperty) {
            continue;
        }
        pProperty->spReflectionMap = apWaterGroup->spGroupReflectionMap;
        if (pWater && pWater->GetReflectsSky() && !bRenderSky) {
            bRenderSky = true;
            NiAVObject* pSkyRoot = TES::pInstance->pSky->GetRoot();
            apWaterGroup->StaticReflectiveObjects.AddHead(pSkyRoot);
        }
        if (!abLOD &&
            (gs_bForceHighDetailLandReflections_Water.Bool() ||
             (pWater && pWater->GetReflectsLand() && !bRenderLand && !TES::pInstance->pInteriorCell))) {
            bRenderLand = true;
            // The grid indices are compared unsigned.
            for (int32_t x = 0; static_cast<uint32_t>(x) < static_cast<uint32_t>(uGridsToLoad.Int()); ++x) {
                for (int32_t y = 0; static_cast<uint32_t>(y) < static_cast<uint32_t>(uGridsToLoad.Int()); ++y) {
                    GridCell* pGridCell = TES::pInstance->GetGridCell(x, y);
                    if (pGridCell->pCell) {
                        for (int32_t iQuad = 0; iQuad < 4; ++iQuad) {
                            NiAVObject* pLandNode = pGridCell->pCell->GetLandNode(static_cast<uint32_t>(iQuad));
                            apWaterGroup->StaticReflectiveObjects.AddHead(pLandNode);
                        }
                    }
                }
            }
        }
        if (TES::pInstance->pWorldSpace) {
            BGSTerrainManager* pTerrainManager = TES::pInstance->pWorldSpace->GetTerrainManager();
            if (pTerrainManager) {
                bool bLOD = BGSTerrainManager::GetCurrentWorldHasLOD();
                if (!TES::pInstance->pInteriorCell && bLOD) {
                    BGSTerrainManager::ShowMinimumDetail();
                    if (pWater && pWater->GetReflectsLODLand() && !bRenderLODLand) {
                        bRenderLODLand = true;
                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pTerrainManager->GetLODLandRoot()) {
                            NiAVObject* pLODLandRoot = pTerrainManager->GetLODLandRoot();
                            apWaterGroup->StaticReflectiveObjects.AddHead(pLODLandRoot);
                        }
                    }
                    BGSTerrainManager::ShowNormalDetail();
                    if (pWater && pWater->GetReflectsLODBuildings() && !bRenderLODBuildings) {
                        bRenderLODBuildings = true;
                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pTerrainManager->GetLODObjectRoot()) {
                            NiAVObject* pLODObjectRoot = pTerrainManager->GetLODObjectRoot();
                            apWaterGroup->StaticReflectiveObjects.AddHead(pLODObjectRoot);
                        }
                    }
                    if (pWater && pWater->GetReflectsLODTrees() && !bRenderLODTrees) {
                        bRenderLODTrees = true;
                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pTerrainManager->GetTreeNode()) {
                            NiAVObject* pTreeNode = pTerrainManager->GetTreeNode();
                            apWaterGroup->StaticReflectiveObjects.AddHead(pTreeNode);
                        }
                    }
                }
            }
        }
        if (!abLOD && IsWaterNodeVisible(pRef, apCamera)) {
            for (BSSimpleList<REF_REFLECTED_DATA*>* pList = pRef->m_Extra.GetReflectedRefs();
                 pList && !pList->IsEmpty();
                 pList = pList->m_pkNext) {
                if ((pList->m_item->iEffectFlags & REF_REFLECTED_DATA::FLAG_REFLECTION) != 0) {
                    TESObjectREFR* pReflectedRef = pList->m_item->pRef;
                    if (pReflectedRef->Get3D() &&
                        !ReflectionRefMap.GetAt(pReflectedRef, pReflectedRef) &&
                        (TES::pInstance->pInteriorCell ||
                         (!TES::pInstance->pInteriorCell && !pReflectedRef->GetVisibleDistant()))) {
                        NiAVObject* pReflected3D = pReflectedRef->Get3D();
                        apWaterGroup->StaticReflectiveObjects.AddHead(pReflected3D);
                        ReflectionRefMap.SetAt(pReflectedRef, pReflectedRef);
                    }
                }
            }
        }
        if (!TES::pInstance->pInteriorCell && !abLOD && pWater && pWater->IsAutomaticWater()) {
            for (BSSimpleList<TESObjectREFR*>* pAutoList = TESObjectCELL::GetAutoWaterReflectList();
                 pAutoList && !pAutoList->IsEmpty();
                 pAutoList = pAutoList->m_pkNext) {
                TESObjectREFR* pAutoRef = pAutoList->m_item;
                if (pAutoRef &&
                    pAutoRef->Get3D() &&
                    !ReflectionRefMap.GetAt(pAutoRef, pAutoRef) &&
                    !pAutoRef->GetVisibleDistant()) {
                    NiAVObject* pAuto3D = pAutoRef->Get3D();
                    apWaterGroup->StaticReflectiveObjects.AddHead(pAuto3D);
                    ReflectionRefMap.SetAt(pAutoRef, pAutoRef);
                }
            }
        }
    }
    if (gs_bReflectExplosions_Water.Bool()) {
        NiTListItem<TESObjectREFR*>* pExplosionPos = PlaceableWaterGroup::ExplosionsList.m_pkHead;
        NiTListItem<TESObjectREFR*>* pNextPos = nullptr;
        pNextPos = PlaceableWaterGroup::ExplosionsList.GetNextPos(pExplosionPos);
        if (pExplosionPos) {
            TESObjectREFR* pExplosion = PlaceableWaterGroup::ExplosionsList.Get(pExplosionPos);
            if (pExplosion->Get3D() && !ReflectionRefMap.GetAt(pExplosion, pExplosion)) {
                NiAVObject* pExplosion3D = pExplosion->Get3D();
                apWaterGroup->DynamicReflectiveObjects.AddHead(pExplosion3D);
                ReflectionRefMap.SetAt(pExplosion, pExplosion);
            }

            pExplosionPos = pNextPos;
        }
    }
    MTRenderingSystem::QInstance()->AddAccumTask(
        apWaterGroup->spReflectionCamera.m_pObject,
        nullptr,
        nullptr,
        &apWaterGroup->StaticReflectiveObjects,
        &apWaterGroup->DynamicReflectiveObjects,
        apWaterGroup->spGroupReflectionSorter.m_pObject,
        BSCullingProcess::BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE,
        static_cast<int32_t>(apWaterGroup->iReflectionThreadStage + 1),
        0
    );
    MTRenderingSystem::QInstance()->SetThreadStage(
        MTRenderingSystem::THREAD_1,
        static_cast<int32_t>(apWaterGroup->iReflectionThreadStage + 1)
    );
}

void TESWaterSystem::MTSetupWorldReflectionTexture(NiCamera* apCamera) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4067
    );
    if (TESWaterSystem::GetUseWaterShader() && apCamera) {
        NiCamera* camera = new NiCamera;
        TESWaterSystem::spWaterHeightCamera = camera;
        NiPoint3 n(0.0f, 0.0f, 1.0f);
        float z = TES::pInstance->pWorldSpace->fLODWaterHeight;
        NiPoint3 p(0.0f, 0.0f, z);
        NiPlane waterplane(n, p);
        BSUtilities::ReflectCameraAboutArbitraryPlane(
            apCamera,
            &waterplane,
            TESWaterSystem::spWaterHeightCamera.m_pObject
        );
        if (!TESWaterSystem::spWaterHeightShaderAccumulator.m_pObject) {
            BSShaderAccumulator* pAccumulator = new BSShaderAccumulator(99, 1u, 759u);
            TESWaterSystem::spWaterHeightShaderAccumulator = pAccumulator;
        }
        ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(0);
        TESWaterSystem::spWaterHeightShaderAccumulator->SetShadowSceneNode(pSceneNode);
        TESWaterSystem::spWaterHeightShaderAccumulator->SetWaterReflection(true);
        TESWaterSystem::spWaterHeightShaderAccumulator->StartAccumulating(
            TESWaterSystem::spWaterHeightCamera.m_pObject
        );
        TESWaterSystem::spWaterHeightShaderAccumulator->SetAccumulate(true);
        if (TESWaterSystem::bAutoWaterSilhouetteReflections.Bool() ||
            TESWaterSystem::bForceLowDetailReflections.Bool()) {
            TESWaterSystem::spWaterHeightShaderAccumulator->SetRenderMode(BSShaderManager::BSSM_RENDER_SILHOUETTE);
            // SkyColor[3] is the sky's ambient light color; it is copied.
            NiColor kambientColor = TES::pInstance->pSky->SkyColor[3];
            ShadowSceneNode* pFogSceneNode = BSShaderManager::GetShadowSceneNode(0);
            NiColor fogColor = pFogSceneNode->GetFogProperty()->m_kColor;
            float fBlue = kambientColor.b * fogColor.b;
            float fGreen = kambientColor.g * fogColor.g;
            float fRed = kambientColor.r * fogColor.r;
            NiColorA ksilhouetteColor(fRed, fGreen, fBlue, 1.0f);
            TESWaterSystem::spWaterHeightShaderAccumulator->SetSilhouetteColor(ksilhouetteColor);
        }
        TESWaterSystem::WaterLevelReflect_Nodes.RemoveAll();
        TESWaterSystem::WaterLevelReflect_Empty.RemoveAll();
        if (TESWaterSystem::bForceHighDetailReflections.Bool()) {
            if (!pPlayer->Is3rdPerson()) {
                bCull3rdPerson = true;
                pPlayer->Get3D()->SetAppCulled(true);
            }
            if (TES::pInstance->pWorldSpace) {
                TESWaterSystem::WaterLevelReflect_Nodes.AddHead(Main::GetWorldSceneGraph());
            }
        }
        else {
            TESWaterSystem::WaterLevelReflect_Nodes.AddHead(TES::pInstance->pSky->GetRoot());
            if (TES::pInstance->pWorldSpace) {
                BGSTerrainManager* pterrainmanager = TES::pInstance->pWorldSpace->GetTerrainManager();
                if (pterrainmanager) {
                    bool bLOD = BGSTerrainManager::GetCurrentWorldHasLOD();
                    if (!TES::pInstance->pInteriorCell && bLOD) {
                        BGSTerrainManager::ShowMinimumDetail();
                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pterrainmanager->GetLODLandRoot()) {
                            TESWaterSystem::WaterLevelReflect_Nodes.AddHead(pterrainmanager->GetLODLandRoot());
                        }

                        BGSTerrainManager::ShowNormalDetail();
                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pterrainmanager->GetLODObjectRoot()) {
                            TESWaterSystem::WaterLevelReflect_Nodes.AddHead(pterrainmanager->GetLODObjectRoot());
                        }

                        if (!TES::pInstance->pInteriorCell &&
                            TES::pInstance->pWorldSpace &&
                            pterrainmanager->GetTreeNode()) {
                            TESWaterSystem::WaterLevelReflect_Nodes.AddHead(pterrainmanager->GetTreeNode());
                        }
                    }
                }
            }
        }
        MTRenderingSystem::QInstance()->AddAccumTask(
            TESWaterSystem::spWaterHeightCamera.m_pObject,
            nullptr,
            nullptr,
            &TESWaterSystem::WaterLevelReflect_Nodes,
            &TESWaterSystem::WaterLevelReflect_Empty,
            TESWaterSystem::spWaterHeightShaderAccumulator.m_pObject,
            BSCullingProcess::BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE,
            static_cast<int32_t>(PlaceableWaterGroup::iWorldReflectionThreadStage + 1),
            0
        );
        MTRenderingSystem::QInstance()->SetThreadStage(
            MTRenderingSystem::THREAD_1,
            static_cast<int32_t>(PlaceableWaterGroup::iWorldReflectionThreadStage + 1)
        );
    }
}

void TESWaterSystem::MTSetupSkyReflectionTexture(NiCamera* apCamera) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4180
    );
    if (TESWaterSystem::GetUseWaterShader() && apCamera) {
        NiCamera* camera = new NiCamera;
        PlaceableWaterGroup::spSkyReflectionCamera = camera;
        NiPoint3 n(0.0f, 0.0f, 1.0f);
        float z = TES::pInstance->pWorldSpace->fLODWaterHeight;
        NiPoint3 p(0.0f, 0.0f, z);
        NiPlane waterplane(n, p);
        BSUtilities::ReflectCameraAboutArbitraryPlane(
            apCamera,
            &waterplane,
            PlaceableWaterGroup::spSkyReflectionCamera.m_pObject
        );
        if (!PlaceableWaterGroup::spSkyReflectionSorter.m_pObject) {
            BSShaderAccumulator* pAccumulator = new BSShaderAccumulator(99, 1u, 759u);
            PlaceableWaterGroup::spSkyReflectionSorter = pAccumulator;
        }
        ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(0);
        PlaceableWaterGroup::spSkyReflectionSorter->SetShadowSceneNode(pSceneNode);
        PlaceableWaterGroup::spSkyReflectionSorter->SetWaterReflection(true);
        PlaceableWaterGroup::spSkyReflectionSorter->StartAccumulating(
            PlaceableWaterGroup::spSkyReflectionCamera.m_pObject
        );
        PlaceableWaterGroup::spSkyReflectionSorter->SetAccumulate(true);
        PlaceableWaterGroup::StaticSkyReflectiveObjects.RemoveAll();
        PlaceableWaterGroup::DynamicSkyReflectiveObjects.RemoveAll();
        PlaceableWaterGroup::StaticSkyReflectiveObjects.AddHead(TES::pInstance->pSky->GetRoot());
        MTRenderingSystem::QInstance()->AddAccumTask(
            PlaceableWaterGroup::spSkyReflectionCamera.m_pObject,
            nullptr,
            nullptr,
            &PlaceableWaterGroup::StaticSkyReflectiveObjects,
            &PlaceableWaterGroup::DynamicSkyReflectiveObjects,
            PlaceableWaterGroup::spSkyReflectionSorter.m_pObject,
            BSCullingProcess::BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE,
            static_cast<int32_t>(PlaceableWaterGroup::iSkyReflectionThreadStage + 1),
            0
        );
        MTRenderingSystem::QInstance()->SetThreadStage(
            MTRenderingSystem::THREAD_1,
            static_cast<int32_t>(PlaceableWaterGroup::iSkyReflectionThreadStage + 1)
        );
    }
}

void TESWaterSystem::MTRenderSelectiveReflectionTexture(PlaceableWaterGroup* apWaterGroup) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4226
    );
    if (TESWaterSystem::GetUseWaterShader() && apWaterGroup->spReflectionCamera.m_pObject) {
        NiColorA oldClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        BSShaderManager::GetRenderer()->GetBackgroundColor(oldClearColor);
        NiColorA newClearColor(oldClearColor.r, oldClearColor.g, oldClearColor.b, 0.0f);
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(newClearColor);
        if (!apWaterGroup->spGroupReflectionMap.m_pObject) {
            NiDX9Renderer* pTextureRenderer = BSShaderManager::GetRenderer();
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            BSRenderedTexture* pTexture = pTextureManager->BorrowRenderedTexture(
                pTextureRenderer,
                BSTextureManager::TEX_TESWATER_REFLECTION,
                0,
                nullptr,
                0
            );
            apWaterGroup->spGroupReflectionMap = pTexture;
        }
        bool bLOD = BGSTerrainManager::GetCurrentWorldHasLOD();
        if (!TES::pInstance->pInteriorCell && bLOD) {
            BGSTerrainManager::DisableBlockVisibility();
        }
        NiRenderTargetGroup* pGroup = apWaterGroup->spGroupReflectionMap->GetGroup();
        BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, pGroup);
        NiRenderer::GetRenderer()->SetScreenSpaceCameraData(&apWaterGroup->spReflectionCamera->m_kPort);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(
            MTRenderingSystem::THREAD_2,
            static_cast<int32_t>(apWaterGroup->iReflectionThreadStage + 1)
        );
        NiRenderer::GetRenderer()->SetCameraData(apWaterGroup->spReflectionCamera.m_pObject);
        // The clip plane is the reflection plane's (n, -c).
        NiPlane* pPlane = &apWaterGroup->ReflectWaterPlane;
        NiColorA kClipPlane(pPlane->m_kNormal.x, pPlane->m_kNormal.y, pPlane->m_kNormal.z, -pPlane->m_fConstant);
        SetClipPlane(kClipPlane, 0);
        BSRenderState::SetUserClipPlaneEnables(true, false, false, false, false, false, BSRS_NOLOCK);
        BSShaderAccumulator* pSorter = apWaterGroup->spGroupReflectionSorter.m_pObject;
        NiCamera* pCamera = apWaterGroup->spReflectionCamera.m_pObject;
        BSShaderUtil::RenderScene(pCamera, pSorter, false);
        BSRenderState::RestoreUserClipPlaneEnables(BSRS_NOLOCK);
        BSRenderedTexture::StopOffscreen();
        if (!TES::pInstance->pInteriorCell && bLOD) {
            BGSTerrainManager::EnableBlockVisibility();
        }
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(oldClearColor);
        apWaterGroup->spReflectionCamera = static_cast<NiCamera*>(nullptr);
    }
}

void TESWaterSystem::MTRenderWorldReflectionTexture() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4280
    );
    if (TESWaterSystem::GetUseWaterShader() && TESWaterSystem::spWaterHeightCamera.m_pObject) {
        NiColorA oldClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        BSShaderManager::GetRenderer()->GetBackgroundColor(oldClearColor);
        NiColorA newClearColor(oldClearColor.r, oldClearColor.g, oldClearColor.b, 0.0f);
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(newClearColor);
        if (!PlaceableWaterGroup::spWorldReflectionMap.m_pObject) {
            NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
            BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
            PlaceableWaterGroup::spWorldReflectionMap = pTextureManager->BorrowRenderedTexture(
                pRenderer,
                BSTextureManager::TEX_TESWATER_REFLECTION,
                0,
                nullptr,
                0
            );
        }
        // Tests the whole byte Setting::Bool returns, read here as the byte it is.
        bool bHasWater = BGSTerrainManager::GetCurrentWorldHasLOD() &&
            !static_cast<uint8_t>(TESWaterSystem::bForceHighDetailReflections.Bool());
        if (!TES::pInstance->pInteriorCell && bHasWater) {
            BGSTerrainManager::DisableBlockVisibility();
        }
        NiRenderTargetGroup* pTarget = PlaceableWaterGroup::spWorldReflectionMap->GetGroup();
        BSRenderedTexture::StartOffscreen(7u, pTarget);
        const NiRect<float>* pViewport = &TESWaterSystem::spWaterHeightCamera->GetViewPort();
        NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pViewport);
        // aiStage is int32_t; the uint32_t stage plus 1 is passed as it is.
        MTRenderingSystem::QInstance()->WaitOnThreadStage(
            MTRenderingSystem::THREAD_2,
            static_cast<int32_t>(PlaceableWaterGroup::iWorldReflectionThreadStage + 1)
        );
        if (bCull3rdPerson && pPlayer->Is3rdPerson()) {
            pPlayer->Get3D()->SetAppCulled(false);
        }
        NiPoint3 n(0.0f, 0.0f, 1.0f);
        float fWaterHeight = TES::pInstance->pWorldSpace->fLODWaterHeight;
        NiPoint3 p(0.0f, 0.0f, fWaterHeight);
        NiPlane waterplane(n, p);
        NiCamera* pCamera = TESWaterSystem::spWaterHeightCamera.m_pObject;
        NiRenderer::GetRenderer()->SetCameraData(pCamera);
        // The clip plane is the water plane's normal and negated constant, built by NiColorA's constructor
        // and passed by value.
        float fNegConstant = -waterplane.m_fConstant;
        NiColorA kClipPlane(waterplane.m_kNormal.x, waterplane.m_kNormal.y, waterplane.m_kNormal.z, fNegConstant);
        SetClipPlane(kClipPlane, 0);
        BSRenderState::SetUserClipPlaneEnables(true, false, false, false, false, false, BSRS_NOLOCK);
        BSShaderUtil::RenderScene(
            TESWaterSystem::spWaterHeightCamera.m_pObject,
            TESWaterSystem::spWaterHeightShaderAccumulator.m_pObject,
            false
        );
        BSRenderState::RestoreUserClipPlaneEnables(BSRS_NOLOCK);
        BSRenderedTexture::StopOffscreen();
        if (!TES::pInstance->pInteriorCell && bHasWater) {
            BGSTerrainManager::EnableBlockVisibility();
        }
        BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(oldClearColor);
        TESWaterSystem::spWaterHeightCamera = static_cast<NiCamera*>(nullptr);
        // Tests the whole byte Setting::Bool returns, read here as the byte it is.
        if (static_cast<uint8_t>(gs_bUseWaterReflectionBlur_Water.Bool())) {
            NiDX9Renderer* pBlurRenderer = BSShaderManager::GetRenderer();
            BSRenderedTexture* pblur = BSShaderManager::TextureManagerPtr()->BorrowRenderedTexture(
                pBlurRenderer,
                BSTextureManager::TEX_TESWATER_BLUR,
                0,
                nullptr,
                0
            );
            ImageSpaceTexture kblurtexture;
            kblurtexture.SetTexture(pblur);
            ImageSpaceEffectParam kparam;
            // The setting is converted as unsigned, then 1.0 is added.
            ImageSpaceEffectBlur::fBlurRadius = static_cast<double>(
                                                    static_cast<uint32_t>(gs_iWaterBlurAmount_Water.Int())
                                                ) +
                1.0;
            int32_t iblur = gs_iWaterBlurAmount_Water.Int();
            ImageSpaceManager::EffectID eblur = static_cast<ImageSpaceManager::EffectID>(
                iblur + ImageSpaceManager::EFFECT_BLUR_START
            );
            ImageSpaceEffect* pEffect = BSShaderManager::GetImageSpaceManager()->GetEffect(eblur);
            pEffect->SetTexture(2, &kblurtexture, NiTexturingProperty::FILTER_NEAREST);
            BSRenderedTexture* pDest = PlaceableWaterGroup::spWorldReflectionMap.m_pObject;
            BSRenderedTexture* pSource = PlaceableWaterGroup::spWorldReflectionMap.m_pObject;
            NiDX9Renderer* pEffectRenderer = BSShaderManager::GetRenderer();
            BSShaderManager::GetImageSpaceManager()->RenderEffect(eblur, pEffectRenderer, pSource, pDest, &kparam, 1);
            BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(pblur);
        }
    }
}

// Sets up apWaterGroup's depth camera and sorter, collects the objects its water refracts into the group's static and
// dynamic depth lists, and queues the accumulation on the first rendering thread. ObjectInWaterList is walked but each
// next position is taken from PlaceableWaterList's GetNextPos; both lists share the node type.
void TESWaterSystem::MTSetupSelectiveDepthTexture(NiCamera* apCamera, PlaceableWaterGroup* apWaterGroup) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4431
    );
    if (!TESWaterSystem::GetUseWaterShader() || !apCamera) {
        return;
    }
    NiCamera* pNewCamera = new NiCamera;
    apWaterGroup->spDepthCamera = pNewCamera;
    const NiPoint3* pCamPos = &apCamera->GetWorldTranslate();
    apWaterGroup->spDepthCamera->SetTranslate(*pCamPos);
    const NiTransform* pCamTransform = &apCamera->GetWorldTransform();
    apWaterGroup->spDepthCamera->SetRotate(pCamTransform->m_Rotate);
    float fCamScale = apCamera->m_kWorld.m_fScale;
    apWaterGroup->spDepthCamera->SetScale(fCamScale);
    float fMaxFarNearRatio = apCamera->m_fMaxFarNearRatio;
    apWaterGroup->spDepthCamera->SetMaxFarNearRatio(fMaxFarNearRatio);
    const NiFrustum* pFrustum = &apCamera->GetViewFrustum();
    apWaterGroup->spDepthCamera->SetViewFrustum(*pFrustum);
    NiUpdateData UpdateData(0.0f, false, false);
    apWaterGroup->spDepthCamera->Update(UpdateData);
    if (!apWaterGroup->spDepthSorter.m_pObject) {
        BSShaderAccumulator* pNewSorter = new BSShaderAccumulator(99, 1u, 759u);
        apWaterGroup->spDepthSorter = pNewSorter;
    }
    ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
    apWaterGroup->spDepthSorter->SetShadowSceneNode(pSceneNode);
    apWaterGroup->spDepthSorter->SetWaterRefraction(true);
    NiAccumulator* pAccumulator = apWaterGroup->spDepthSorter.m_pObject;
    pAccumulator->StartAccumulating(apWaterGroup->spDepthCamera.m_pObject);
    // SetAccumulate is a trivial one-byte setter, so bAccumulate is written directly.
    apWaterGroup->spDepthSorter->bAccumulate = true;
    if (!PlaceableWaterGroup::spDepthMap.m_pObject) {
        NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        PlaceableWaterGroup::spDepthMap = pTextureManager->BorrowRenderedTexture(
            pRenderer,
            BSTextureManager::TEX_TESWATER_DEPTH,
            0,
            nullptr,
            0
        );
    }
    bool bRenderLand = false;
    bool bRenderActors = false;
    NiTListItem<TESObjectREFR*>* pWaterPos = apWaterGroup->PlaceableWaterList.m_pkHead;
    while (pWaterPos) {
        TESObjectREFR* pRef = apWaterGroup->PlaceableWaterList.GetNext(pWaterPos);
        if (!pRef || !pRef->GetParentCell() || !pRef->GetParentCell()->GetAttached()) {
            continue;
        }
        NiAVObject* pGeometry = GetWaterGeometry(pRef);
        NiProperty* pShaderProperty = nullptr;
        if (pGeometry) {
            pShaderProperty = pGeometry->GetProperty(3);
        }
        if (!pShaderProperty || !IsWaterNodeVisible(pRef, apCamera)) {
            continue;
        }
        // The water geometry's shader property is taken as a WaterShaderProperty without a type test.
        static_cast<WaterShaderProperty*>(pShaderProperty)->spDepthMap = PlaceableWaterGroup::spDepthMap;
        BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(pRef->GetObjectReference());
        if (!pWater->IsDepthEnabled()) {
            continue;
        }
        if (pWater && pWater->GetRefractsLand() && !bRenderLand) {
            bRenderLand = true;
            // The grid indices are compared unsigned.
            for (int32_t x = 0; static_cast<uint32_t>(x) < static_cast<uint32_t>(uGridsToLoad.Int()); ++x) {
                for (int32_t y = 0; static_cast<uint32_t>(y) < static_cast<uint32_t>(uGridsToLoad.Int()); ++y) {
                    GridCell* pGridCell = TES::pInstance->GetGridCell(x, y);
                    if (pGridCell->pCell) {
                        for (int32_t iQuad = 0; iQuad < 4; ++iQuad) {
                            NiAVObject* pLandNode = pGridCell->pCell->GetLandNode(static_cast<uint32_t>(iQuad));
                            apWaterGroup->StaticDepthObjects.AddHead(pLandNode);
                        }
                    }
                }
            }
        }
        if (pWater && pWater->GetRefractsActors() && !bRenderActors) {
            bRenderActors = true;
            NiTListItem<Actor*>* pActorPos = apWaterGroup->ActorsInWaterList.m_pkHead;
            NiTListItem<Actor*>* pNextActorPos = nullptr;
            while (pActorPos) {
                pNextActorPos = apWaterGroup->ActorsInWaterList.GetNextPos(pActorPos);
                TESObjectREFR* pActor = apWaterGroup->ActorsInWaterList.Get(pActorPos);
                if ((pActor != pPlayer || (pActor == pPlayer && pPlayer->Is3rdPerson())) &&
                    pActor->Get3D() &&
                    !DepthRefMap.GetAt(pActor, pActor)) {
                    NiAVObject* pActor3D = pActor->Get3D();
                    apWaterGroup->DynamicDepthObjects.AddHead(pActor3D);
                    DepthRefMap.SetAt(pActor, pActor);
                }

                pActorPos = pNextActorPos;
            }
        }
        for (BSSimpleList<REF_REFLECTED_DATA*>* pList = pRef->m_Extra.GetReflectedRefs(); pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            if ((pList->m_item->iEffectFlags & REF_REFLECTED_DATA::FLAG_REFRACTION) != 0) {
                TESObjectREFR* pRefractedRef = pList->m_item->pRef;
                if (pRefractedRef->Get3D() && !DepthRefMap.GetAt(pRefractedRef, pRefractedRef)) {
                    NiAVObject* pRefracted3D = pRefractedRef->Get3D();
                    apWaterGroup->StaticDepthObjects.AddHead(pRefracted3D);
                    DepthRefMap.SetAt(pRefractedRef, pRefractedRef);
                }
            }
        }
        NiTListItem<TESObjectREFR*>* pObjectPos = apWaterGroup->ObjectInWaterList.m_pkHead;
        NiTListItem<TESObjectREFR*>* pNextObjectPos = nullptr;
        while (pObjectPos) {
            pNextObjectPos = apWaterGroup->PlaceableWaterList.GetNextPos(pObjectPos);
            if (pObjectPos) {
                TESObjectREFR* pObject = apWaterGroup->ObjectInWaterList.Get(pObjectPos);
                if (pObject->Get3D() && !DepthRefMap.GetAt(pObject, pObject)) {
                    NiAVObject* pObject3D = pObject->Get3D();
                    apWaterGroup->DynamicDepthObjects.AddHead(pObject3D);
                    DepthRefMap.SetAt(pObject, pObject);
                }
            }
            pObjectPos = pNextObjectPos;
        }
        if (pWater && pWater->IsAutomaticWater()) {
            for (BSSimpleList<TESObjectREFR*>* pAutoList = TESObjectCELL::GetAutoWaterRefractList();
                 pAutoList && !pAutoList->IsEmpty();
                 pAutoList = pAutoList->m_pkNext) {
                TESObjectREFR* pAutoRef = pAutoList->m_item;
                if (pAutoRef && pAutoRef->Get3D() && !DepthRefMap.GetAt(pAutoRef, pAutoRef)) {
                    NiAVObject* pAuto3D = pAutoRef->Get3D();
                    apWaterGroup->StaticDepthObjects.AddHead(pAuto3D);
                    DepthRefMap.SetAt(pAutoRef, pAutoRef);
                }
            }
        }
    }
    MTRenderingSystem::QInstance()->AddAccumTask(
        apWaterGroup->spDepthCamera.m_pObject,
        nullptr,
        nullptr,
        &apWaterGroup->StaticDepthObjects,
        &apWaterGroup->DynamicDepthObjects,
        apWaterGroup->spDepthSorter.m_pObject,
        BSCullingProcess::BSCP_CULL_NORMAL,
        static_cast<int32_t>(apWaterGroup->iDepthThreadStage + 1),
        0
    );
    MTRenderingSystem::QInstance()->SetThreadStage(
        MTRenderingSystem::THREAD_1,
        static_cast<int32_t>(apWaterGroup->iDepthThreadStage + 1)
    );
}

void TESWaterSystem::MTRenderSelectiveDepthTexture(PlaceableWaterGroup* apWaterGroup, uint16_t auReadMask) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4617
    );
    if (TESWaterSystem::GetUseWaterShader() && apWaterGroup->spDepthCamera.m_pObject) {
        const NiRect<float>* pViewport = &apWaterGroup->spDepthCamera->m_kPort;
        NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pViewport);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(
            MTRenderingSystem::THREAD_2,
            static_cast<int32_t>(apWaterGroup->iDepthThreadStage + 1)
        );
        NiCamera* pCamera = apWaterGroup->spDepthCamera.m_pObject;
        NiRenderer::GetRenderer()->SetCameraData(pCamera);
        float fFogFar = apWaterGroup->pWaterType->Data.fFogDistFar;
        float fFogNear = apWaterGroup->pWaterType->Data.fFogDistNear;
        BSShaderManager::kWaterFogDistances = NiPoint2(fFogNear, fFogFar);
        apWaterGroup->spDepthSorter->SetRenderMode(BSShaderManager::BSSM_RENDER_WATER_DEPTH);
        BSRenderState::SetStencilEnable(true, BSRS_LOCK);
        BSRenderState::SetStencilFunc(NiStencilProperty::TEST_EQUAL, 255u, auReadMask, BSRS_LOCK);
        BSRenderState::SetStencilAction(
            NiStencilProperty::ACTION_KEEP,
            NiStencilProperty::ACTION_KEEP,
            NiStencilProperty::ACTION_KEEP,
            BSRS_LOCK
        );
        BSRenderState::SetStencilWriteMask(0, BSRS_LOCK);
        pCamera = apWaterGroup->spDepthCamera.m_pObject;
        NiRenderer::GetRenderer()->SetCameraData(pCamera);
        BSShaderManager::kWaterPlane = apWaterGroup->ReflectWaterPlane;
        double dConstant = -apWaterGroup->RefractWaterPlane.m_fConstant;
        float refractionPlaneBias = gs_fRefractionWaterPlaneBias_Water.Float() + dConstant;
        float normal_z = apWaterGroup->RefractWaterPlane.m_kNormal.z;
        float normal_y = apWaterGroup->RefractWaterPlane.m_kNormal.y;
        NiColorA kClipPlane(apWaterGroup->RefractWaterPlane.m_kNormal.x, normal_y, normal_z, refractionPlaneBias);
        SetClipPlane(kClipPlane, 0);
        BSRenderState::SetUserClipPlaneEnables(true, false, false, false, false, false, BSRS_NOLOCK);
        BSShaderAccumulator* pSorter = apWaterGroup->spDepthSorter.m_pObject;
        pCamera = apWaterGroup->spDepthCamera.m_pObject;
        BSShaderUtil::RenderScene(pCamera, pSorter, false);
        BSRenderState::RestoreUserClipPlaneEnables(BSRS_NOLOCK);
        NiDX9RenderState* pRenderState = BSShaderManager::GetRenderer()->GetRenderState();
        pRenderState->SetRenderState(D3DRS_CLIPPLANEENABLE, 0, 0, false);
        BSRenderState::RestoreStencilEnable(BSRS_LOCK);
        BSRenderState::UnlockRenderState(BSRS_STENCILFAILZFAILPASS);
        BSRenderState::UnlockRenderState(BSRS_STENCILFUNCREFMASK);
        BSRenderState::UnlockRenderState(BSRS_STENCILWRITEMASK);
        apWaterGroup->spDepthCamera = static_cast<NiCamera*>(nullptr);
    }
}

void TESWaterSystem::RenderWaterGroupStencil(
    NiCamera* apCamera,
    PlaceableWaterGroup* apWaterGroup,
    bool abClearAll,
    uint16_t auWriteMask
) {
    if (TESWaterSystem::GetUseWaterShader() && apCamera) {
        TESWaterSystem::bIsRendering = true;
        BSRenderState::SetColorWriteEnable(0, BSRS_LOCK);
        NiCamera refractionCamera;
        refractionCamera.SetTranslate(apCamera->GetWorldTranslate());
        refractionCamera.SetRotate(apCamera->GetWorldTransform().m_Rotate);
        float fCamScale = apCamera->m_kWorld.m_fScale;
        refractionCamera.SetScale(fCamScale);
        const NiRect<float>* pViewport = &refractionCamera.GetViewPort();
        NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pViewport);
        float fMaxFarNearRatio = apCamera->m_fMaxFarNearRatio;
        refractionCamera.SetMaxFarNearRatio(fMaxFarNearRatio);
        refractionCamera.SetViewFrustum(apCamera->GetViewFrustum());
        NiUpdateData UpdateData(0.0f, false, false);
        refractionCamera.Update(UpdateData);
        BSCullingProcess myCullingProcess(nullptr);
        myCullingProcess.m_pkCamera = &refractionCamera;
        NiRenderer::GetRenderer()->SetCameraData(&refractionCamera);
        myCullingProcess.SetFrustum(refractionCamera.GetViewFrustum());
        NiTListItem<TESObjectREFR*>* iter = apWaterGroup->PlaceableWaterList.m_pkHead;
        while (iter) {
            NiTListItem<TESObjectREFR*>* next = apWaterGroup->PlaceableWaterList.GetNextPos(iter);
            if (iter) {
                TESObjectREFR* pref = apWaterGroup->PlaceableWaterList.Get(iter);
                if (pref && pref->GetParentCell() && pref->GetParentCell()->GetAttached()) {
                    NiTriBasedGeom* pgeom = static_cast<NiTriBasedGeom*>(GetWaterGeometry(pref));
                    if (pgeom) {
                        BSShader* pshader = pgeom->GetShader();
                        if (pshader &&
                            pshader->iShaderType == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                            WaterShaderProperty* pkWaterProp = static_cast<WaterShaderProperty*>(
                                pgeom->GetProperty(NiProperty::SHADE)
                            );
                            pkWaterProp->iStencilMask = auWriteMask;
                            if (IsWaterNodeVisible(pref, apCamera) && pkWaterProp && pkWaterProp->GetWaterPass(pgeom)) {
                                BSShaderProperty::RenderPass* ppass = pkWaterProp->GetWaterPass(pgeom);
                                BSBatchRenderer::RenderPassImmediately(ppass, ppass->usPassEnum, 0, 0, 0);
                            }
                        }
                    }
                }
            }
            iter = next;
        }
        myCullingProcess.m_pkCamera = nullptr;
        BSRenderState::RestoreColorWriteEnable(BSRS_LOCK);
        TESWaterSystem::bIsRendering = false;
    }
}

void TESWaterSystem::AddTESObjectToWaterGroup(
    TESObjectREFR* apWaterRef,
    TESObjectREFR* apObjectRef,
    float afWaterHeight
) {
    if (apWaterRef || apObjectRef->GetParentCell()) {
        PlaceableWaterGroup* pWaterGroup = FindWaterGroup(apWaterRef, apObjectRef, afWaterHeight);
        if (pWaterGroup) {
            // Looks for the reference in the object list only, also for an actor.
            NiTListItem<TESObjectREFR*>* pIter = pWaterGroup->ObjectInWaterList.m_pkHead;
            NiTListItem<TESObjectREFR*>* pNext = nullptr;
            bool bFound = false;
            while (pIter && !bFound) {
                pNext = pWaterGroup->ObjectInWaterList.GetNextPos(pIter);
                if (pIter) {
                    if (pWaterGroup->ObjectInWaterList.Get(pIter) == apObjectRef) {
                        bFound = true;
                    }
                    pIter = pNext;
                }
            }
            if (!bFound) {
                if (apObjectRef->IsActor()) {
                    Actor* pActor = static_cast<Actor*>(apObjectRef);
                    if (!pWaterGroup->ActorsInWaterList.FindPos(pActor, nullptr)) {
                        pWaterGroup->ActorsInWaterList.AddHead(pActor);
                    }
                }
                else {
                    pWaterGroup->ObjectInWaterList.AddHead(apObjectRef);
                }
                if (apObjectRef->GetDestructible() &&
                    apObjectRef->Get3D() &&
                    apObjectRef->GetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS)) {
                    NiNode* p3D = apObjectRef->Get3D();
                    NiNode* pNode = p3D->IsNode();
                    TESObjectREFR::RemoveMasterParticleAddonNodes(pNode);
                }
            }
        }
    }
}

void TESWaterSystem::AddRipple(NiPoint3 arPosition, float afSize) {
    if (gs_bUseBulletWaterDisplacements_Water.Bool() && TESWaterSystem::bUseWaterDisplacements.Bool()) {
        NiPoint2 ripplepos;
        NiPoint2 playerpos;
        playerpos.x = pPlayer->GetLocationOnReference().x;
        playerpos.y = pPlayer->GetLocationOnReference().y;
        ripplepos.x = arPosition.x;
        ripplepos.y = arPosition.y;
        float dist = (playerpos - ripplepos).Length();
        if (gs_fWadingWaterQuadSize_Water.Float() >= static_cast<double>(dist)) {
            void* iter = PlaceableWaterGroupList.m_pkHead;
            while (iter) {
                void* next = PlaceableWaterGroupList.GetNextPos(static_cast<NiTListItem<PlaceableWaterGroup*>*>(iter));
                if (iter) {
                    PlaceableWaterGroup* pgroup = PlaceableWaterGroupList.Get(
                        static_cast<NiTListItem<PlaceableWaterGroup*>*>(iter)
                    );
                    float fWaterGroupHeightRange = gs_fWaterGroupHeightRange_Water.Float();
                    if (BSUtilities::CompareFloat(
                            pgroup->ReflectWaterPlane.m_fConstant,
                            arPosition.z,
                            fWaterGroupHeightRange
                        ) &&
                        !pgroup->spWadingWaterGeometry.m_pObject) {
                        if (!PlaceableWaterGroup::spWadingWaterHeightMap.m_pObject) {
                            PlaceableWaterGroup::spWadingWaterHeightMap = InitializeGreyTexture();
                            ImageSpaceEffectWaterDisplacement::bClearSimulation = true;
                        }
                        float fMeshSize = gs_fWadingWaterQuadSize_Water.Float();
                        NiNode* pWaterRoot = TESWaterSystem::GetWaterRoot();
                        NiAVObject* pWadingGeometry = CreateWadingWaterGeometry(pgroup, pWaterRoot, fMeshSize);
                        pgroup->spWadingWaterGeometry = static_cast<NiRefObject*>(pWadingGeometry);
                        NiPoint3 kMeshPos = pPlayer->GetLocationOnReference();
                        kMeshPos.z = pgroup->ReflectWaterPlane.m_fConstant;
                        static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject)
                            ->NiAVObject::SetTranslate(kMeshPos);
                        NiUpdateData UpdateData(0.0f, false, false);
                        static_cast<NiAVObject*>(pgroup->spWadingWaterGeometry.m_pObject)
                            ->NiAVObject::Update(UpdateData);
                    }
                }
                iter = next;
            }
            ripplepos.x = arPosition.x - pPlayer->GetLocationOnReference().x;
            ripplepos.y = arPosition.y - pPlayer->GetLocationOnReference().y;
            ripplepos.x = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, ripplepos.x);
            ripplepos.y = FLerp(-0.5f, 0.5f, -512.0f, 512.0f, ripplepos.y);
            float fHalfSize = afSize / 2.0;
            ripplepos += NiPoint2(fHalfSize, fHalfSize);
            ImageSpaceEffectWaterDisplacement::fDisplaceOffset.Add(ripplepos);
            ripplepos -= NiPoint2(fHalfSize, fHalfSize);
            ImageSpaceEffectWaterDisplacement::fLastDisplaceOffset.Add(ripplepos);
        }
    }
}

void TESWaterSystem::MTRenderSkyReflectionTexture() {
    alignas(AutoMemContext) uint8_t kMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (kMemContext) AutoMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        4372
    );
    if (!GetUseWaterShader() || !PlaceableWaterGroup::spSkyReflectionCamera) {
        pMemContext->~AutoMemContext();
        return;
    }

    NiColorA kOldBackground(0.0f, 0.0f, 0.0f, 0.0f);
    BSShaderManager::GetRenderer()->GetBackgroundColor(kOldBackground);
    NiColorA kBackground = kOldBackground;
    kBackground.a = 0.0f;
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(kBackground);
    if (!PlaceableWaterGroup::spSkyReflectionMap) {
        NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        BSRenderedTexture* pReflectionTexture = pTextureManager->BorrowRenderedTexture(
            pRenderer,
            BSTextureManager::TEX_TESWATER_REFLECTION,
            0,
            nullptr,
            0
        );
        PlaceableWaterGroup::spSkyReflectionMap = pReflectionTexture;
    }

    bool bHasLOD = BGSTerrainManager::GetCurrentWorldHasLOD();
    if (!TES::pInstance->GetInterior() && bHasLOD) {
        BGSTerrainManager::DisableBlockVisibility();
    }
    NiRenderTargetGroup* pTarget = PlaceableWaterGroup::spSkyReflectionMap->GetGroup();
    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, pTarget);
    const NiRect<float>* pPort = &PlaceableWaterGroup::spSkyReflectionCamera->GetViewPort();
    NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pPort);
    int32_t iStage = PlaceableWaterGroup::iSkyReflectionThreadStage + 1;
    MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, iStage);

    NiPoint3 kNormal;
    ::new (&kNormal) NiPoint3(0.0f, 0.0f, 1.0f);
    float fLODHeight = TES::pInstance->GetWorldSpace()->GetLODWaterHeight();
    NiPoint3 kPoint;
    ::new (&kPoint) NiPoint3(0.0f, 0.0f, fLODHeight);
    NiPlane kPlane(kNormal, kPoint);
    NiCamera* pCamera = PlaceableWaterGroup::spSkyReflectionCamera;
    NiRenderer::GetRenderer()->SetCameraData(pCamera);
    // The clip plane is (n, -c).
    float fD = -kPlane.GetConstant();
    float fC = kPlane.GetNormal().z;
    float fB = kPlane.GetNormal().y;
    float fA = kPlane.GetNormal().x;
    SetClipPlane(NiColorA(fA, fB, fC, fD), 0);
    BSRenderState::SetUserClipPlaneEnables(true, false, false, false, false, false, BSRS_NOLOCK);
    BSShaderAccumulator* pSorter = PlaceableWaterGroup::spSkyReflectionSorter;
    pCamera = PlaceableWaterGroup::spSkyReflectionCamera;
    BSShaderUtil::RenderScene(pCamera, pSorter, false);
    BSRenderState::RestoreUserClipPlaneEnables(BSRS_NOLOCK);
    BSRenderedTexture::StopOffscreen();
    if (!TES::pInstance->GetInterior() && bHasLOD) {
        BGSTerrainManager::EnableBlockVisibility();
    }
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(kOldBackground);
    PlaceableWaterGroup::spSkyReflectionCamera = nullptr;
    pMemContext->~AutoMemContext();
}

// aiPlane is unused; plane 0 is always set.
void TESWaterSystem::SetClipPlane(NiColorA afData, int32_t aiPlane) {
    D3DXMATRIX kInverse;

    // The view matrix with its translation recomputed from the camera position along each axis.
    alignas(16) D3DXMATRIX kView(*BSShaderManager::GetRenderer()->GetD3DView());
    NiPoint3 kAxis0;
    NiPoint3* pAxis0 = ::new (&kAxis0) NiPoint3(kView._11, kView._21, kView._31);
    kView._41 = -BSShader::GetCameraWorldTranslate()->Dot(*pAxis0);
    NiPoint3 kAxis1;
    NiPoint3* pAxis1 = ::new (&kAxis1) NiPoint3(kView._12, kView._22, kView._32);
    kView._42 = -BSShader::GetCameraWorldTranslate()->Dot(*pAxis1);
    NiPoint3 kAxis2;
    NiPoint3* pAxis2 = ::new (&kAxis2) NiPoint3(kView._13, kView._23, kView._33);
    kView._43 = -BSShader::GetCameraWorldTranslate()->Dot(*pAxis2);

    alignas(16) D3DXMATRIX kProj(*BSShaderManager::GetRenderer()->GetD3DProj());
    alignas(16) D3DXMATRIX kViewProj = kView * kProj;
    D3DXMatrixInverse_Thunk(&kInverse, nullptr, &kViewProj);
    D3DXMatrixTranspose_Thunk(&kViewProj, &kInverse);

    alignas(16) D3DXPLANE kPlane;
    alignas(16) D3DXPLANE kClipPlane;
    kPlane.a = afData.r;
    kPlane.b = afData.g;
    kPlane.c = afData.b;
    kPlane.d = afData.a;
    D3DXPlaneNormalize_Thunk(&kPlane, &kPlane);
    D3DXPlaneTransform_Thunk(&kClipPlane, &kPlane, &kViewProj);
    alignas(16) float afPlane[4];
    afPlane[0] = kClipPlane.a;
    afPlane[1] = kClipPlane.b;
    afPlane[2] = kClipPlane.c;
    afPlane[3] = kClipPlane.d;
    static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->SetClippingPlanes(0, 1, afPlane);
}

// Copies apRef's water type (colours, amounts, fog, depth falloff) into the water shader property of apRef's water
// geometry when the property asks for new constants; a change of its reflection, refraction or depth use clears its
// render passes and those of apWaterGroup's wading water.
void TESWaterSystem::UpdateWaterShaderProperties(PlaceableWaterGroup* apWaterGroup, TESObjectREFR* apRef) {
    NiTriBasedGeom* pGeometry = static_cast<NiTriBasedGeom*>(GetWaterGeometry(apRef));
    if (pGeometry && pGeometry->IsGeometry()) {
        BSShader* pShader = pGeometry->GetShader();
        if (pShader && pShader->iShaderType == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
            NiProperty* pProp = pGeometry->GetProperty(NiProperty::SHADE);
            WaterShaderProperty* pkWaterProp = static_cast<WaterShaderProperty*>(pProp);
            BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(apRef->GetObjectReference());
            TESWaterForm* pType = pWater->GetWaterType();
            if (pProp && pType && pkWaterProp->bUpdateConstants) {
                // Each colour is packed 0x00BBGGRR; each channel is divided by 255.0 (double).
                uint32_t uiColor = pType->Data.iShallowWaterColor;
                pkWaterProp->pShallowColor = NiColorA(
                    static_cast<float>(static_cast<uint8_t>(uiColor) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 8) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 16) / 255.0),
                    1.0f
                );
                uiColor = pType->Data.iDeepWaterColor;
                pkWaterProp->pDeepColor = NiColorA(
                    static_cast<float>(static_cast<uint8_t>(uiColor) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 8) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 16) / 255.0),
                    1.0f
                );
                uiColor = pType->Data.iReflectionWaterColor;
                pkWaterProp->pReflectionColor = NiColorA(
                    static_cast<float>(static_cast<uint8_t>(uiColor) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 8) / 255.0),
                    static_cast<float>(static_cast<uint8_t>(uiColor >> 16) / 255.0),
                    1.0f
                );
                pkWaterProp->pVarAmounts.x = pType->Data.fSunPower;
                pkWaterProp->pVarAmounts.y = pType->Data.fReflectionAmount;
                pkWaterProp->pVarAmounts.z = pType->GetAlpha();
                pkWaterProp->pVarAmounts.w = pType->Data.fRefractivityAmount;
                pkWaterProp->fFresnelAmount = pType->Data.fFresnelAmount;
                pkWaterProp->pFresnelRI.z = pType->Data.fShininess;
                pkWaterProp->pFresnelRI.w = pType->Data.fHDRMultiplier;
                pkWaterProp->pBlendRadius.z = pType->GetLightRadius();
                pkWaterProp->pBlendRadius.w = pType->GetLightBrightness();
                pkWaterProp->fNoiseScale = pType->Data.fNoiseScale;
                TESWaterSystem::fDisplacementDampener = pType->Data.fDisplacementDampener;
                if (TESWaterSystem::GetPlayerUnderwater() || TESWaterListener::GetUnderwater()) {
                    pkWaterProp->fFogFar = pType->Data.fUnderwaterFogDistFar;
                    double dFar = pType->Data.fUnderwaterFogDistFar;
                    double dNear = pType->Data.fUnderwaterFogDistNear;
                    pkWaterProp->fFogRange = dFar - dNear;
                    if (static_cast<uint8_t>(pWater->GetUnderwaterFog())) {
                        pkWaterProp->fFogAmount = pType->Data.fUnderwaterFogAmount;
                    }
                }
                else {
                    pkWaterProp->fFogFar = pType->Data.fFogDistFar;
                    double dFar = pType->Data.fFogDistFar;
                    double dNear = pType->Data.fFogDistNear;
                    pkWaterProp->fFogRange = dFar - dNear;
                    if (static_cast<uint8_t>(pWater->GetUnderwaterFog())) {
                        pkWaterProp->fFogAmount = pType->GetFogScale();
                    }
                    else {
                        pkWaterProp->fFogAmount = 0.0f;
                    }
                }
                pkWaterProp->fUVScale = pType->Data.fUVScale;
                bool bReflections = pkWaterProp->bReflections;
                bool bRefractions = pkWaterProp->bRefractions;
                bool bDepth = pkWaterProp->bDepth;
                bool bUsesReflections = pWater->GetReflectionEnabled() && TESWaterSystem::bUseWaterReflections.Bool();
                pkWaterProp->bReflections = bUsesReflections;
                bool bUsesRefractions = pWater->GetRefractionEnabled() && TESWaterSystem::bUseWaterRefractions.Bool();
                pkWaterProp->bRefractions = bUsesRefractions;
                bool bUsesDepth = pWater->IsDepthEnabled() && TESWaterSystem::bUseWaterDepth.Bool();
                pkWaterProp->bDepth = bUsesDepth;
                if (pkWaterProp->bReflections != bReflections ||
                    pkWaterProp->bRefractions != bRefractions ||
                    pkWaterProp->bDepth != bDepth) {
                    pkWaterProp->ClearRenderPasses();
                    if (apWaterGroup->spWadingWaterGeometry.m_pObject) {
                        NiAVObject* pWading = static_cast<NiAVObject*>(apWaterGroup->spWadingWaterGeometry.m_pObject);
                        WaterShaderProperty* pkWadingWaterProp = static_cast<WaterShaderProperty*>(
                            pWading->GetProperty(NiProperty::SHADE)
                        );
                        if (pkWadingWaterProp->GetNumberofPasses(nullptr)) {
                            pkWadingWaterProp->ClearRenderPasses();
                        }
                    }
                }
                pkWaterProp->bObjectTexCoords = pWater->GetObjectTexCoordsEnabled();
                double dFalloffStart = pType->Data.fDepthFalloffStart;
                if (dFalloffStart == pType->GetDepthY()) {
                    pkWaterProp->pDepthFalloff.x = -1.0f;
                    pkWaterProp->pDepthFalloff.y = 0.0f;
                }
                else {
                    pkWaterProp->pDepthFalloff.x = pType->Data.fDepthFalloffStart;
                    pkWaterProp->pDepthFalloff.y = pType->GetDepthY();
                }
            }
        }
    }
}

void TESWaterSystem::GetWaterNoiseTexture(NiPointer<NiTexture>& aspTexture) {
    aspTexture = spWaterNoiseTexture;
}

TESObjectREFR* TESWaterSystem::AddLODWater(
    NiGeometry* apLODWater,
    TESWorldSpace* apWorldSpace,
    NiNode* apWaterLODRoot,
    BSMultiBoundNode* apBound,
    bool abLODWaterHeight
) {
    NiMatrix3 rot;
    if (TES::pInstance->pWorldSpace->GetNoLODWater() || apWorldSpace->GetNoLODWater()) {
        return nullptr;
    }
    TESObjectREFR* pwaterRef = new TESObjectREFR;
    pwaterRef->SetTemporary();
    WaterShaderProperty* pwaterProp = new WaterShaderProperty;
    pwaterProp->bLOD = true;
    if (abLODWaterHeight) {
        pwaterProp->bFullReflections = true;
    }
    apLODWater->RemoveProperty(NiShadeProperty::GetType());
    apLODWater->AttachProperty(pwaterProp);
    apLODWater->UpdateProperties();
    BSShaderManager::PrepareObject(apLODWater, 0, 0);
    NiStencilProperty* pstencilprop = new NiStencilProperty;
    pstencilprop->SetDrawMode(NiStencilProperty::DRAW_BOTH);
    apLODWater->RemoveProperty(TESClass::GetTagSkillCount());
    apLODWater->AttachProperty(pstencilprop);
    pwaterRef->SetObjectReference(apWorldSpace->GetLODWaterType()->GetPlaceableLODWater());
    BSFadeNode* pfadenode = new BSFadeNode;
    NiNode* pnode = pfadenode;
    if (apBound) {
        pnode = apBound;
        pfadenode->AttachChild(apBound, 1);
        pnode->AttachChild(apLODWater, 1);
    }
    else {
        pfadenode->AttachChild(apLODWater, 1);
    }
    pwaterRef->Set3D(pfadenode, true);
    NiUpdateData foo(0.0f, false, false);
    apLODWater->Update(foo);
    pfadenode->Update(foo);
    pnode->Update(foo);
    float fZ = apWorldSpace->fLODWaterHeight;
    NiPoint3 pos(0.0f, 0.0f, fZ);
    rot = NiMatrix3::IDENTITY;
    if (pLODWaterGroup) {
        pLODWaterGroup->PlaceableWaterList.AddHead(pwaterRef);
        ++TESWaterSystem::uiLODPlanesCount;
    }
    else {
        PlaceableWaterGroup* pnewgroup = new PlaceableWaterGroup;
        NiPoint3 kNormal(0.0f, 0.0f, 1.0f);
        kNormal = rot * kNormal;
        kNormal.Unitize();
        NiPlane reflectplane(kNormal, pos);
        pnewgroup->ReflectWaterPlane = reflectplane;
        BGSPlaceableWater* pwater = static_cast<BGSPlaceableWater*>(pwaterRef->GetObjectReference());
        pnewgroup->pWaterType = pwater->GetWaterType();
        pnewgroup->PlaceableWaterList.AddHead(pwaterRef);
        if (!TESWaterSystem::bForceHighDetailReflections.Bool()) {
            pnewgroup->bRenderSilhouetteReflections = true;
        }
        pLODWaterGroup = pnewgroup;
    }
    apWaterLODRoot->AttachChild(pfadenode, 1);
    EnableWaterSystem();
    return pwaterRef;
}

void TESWaterSystem::RemoveLODWater(TESObjectREFR* apLODWaterRef, TESWorldSpace* apWorldSpace, NiNode* apWaterLODRoot) {
    apWaterLODRoot->DetachChild(apLODWaterRef->Get3D());
    if (pLODWaterGroup) {
        NiTListItem<TESObjectREFR*>* iterWater = pLODWaterGroup->PlaceableWaterList.m_pkHead;
        while (iterWater) {
            NiTListItem<TESObjectREFR*>* nextWater = pLODWaterGroup->PlaceableWaterList.GetNextPos(iterWater);
            if (iterWater) {
                TESObjectREFR* pwaterRef = pLODWaterGroup->PlaceableWaterList.Get(iterWater);
                if (pwaterRef == apLODWaterRef) {
                    pLODWaterGroup->PlaceableWaterList.RemovePos(iterWater);
                    --TESWaterSystem::uiLODPlanesCount;
                    NiAVObject* pobj = GetWaterGeometry(pwaterRef);
                    if (pobj) {
                        WaterShaderProperty* pprop = static_cast<WaterShaderProperty*>(pobj->GetProperty(3));
                        pprop->spReflectionMap = static_cast<BSRenderedTexture*>(nullptr);
                    }
                    if (!pLODWaterGroup->PlaceableWaterList.m_uiCount) {
                        if (pLODWaterGroup) {
                            delete pLODWaterGroup;
                        }
                        pLODWaterGroup = nullptr;
                    }
                    if (PlaceableWaterGroupList.IsEmpty() && !pLODWaterGroup) {
                        DisableWaterSystem(true, false);
                    }
                }
                iterWater = nextWater;
            }
        }
    }
}

void TESWaterSystem::ResetStencilBitRefs() {
    NiTListItem<PlaceableWaterGroup*>* pNextPos;
    int32_t iBit = 1;
    for (NiTListItem<PlaceableWaterGroup*>* pPos = PlaceableWaterGroupList.m_pkHead; pPos; pPos = pNextPos) {
        pNextPos = PlaceableWaterGroupList.GetNextPos(pPos);
        PlaceableWaterGroupList.Get(pPos)->iStencilBitMask = 1 << iBit++;
    }
}

NiTriShape* TESWaterSystem::CreateQuadShape(NiTriShapeData* data) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        2961
    );
    return new NiTriShape(data);
}

NiAVObject*
TESWaterSystem::CreateWadingWaterGeometry(PlaceableWaterGroup* apGroup, NiNode* pParentNode, float fMeshSize) {
    NiTriShapeData* data = CreateQuadData(fMeshSize, 512u, 1u, true, true);
    NiAVObject* pgeom = CreateQuadShape(data);
    NiPoint3 tmp(0.0f, 0.0f, 0.0f);
    if (pPlayer) {
        pgeom->SetTranslate(*pPlayer->QLocationOnReference());
    }
    else {
        pgeom->SetTranslate(tmp);
    }
    NiNode* waterLODNode = TESWaterSystem::GetWaterRoot();
    waterLODNode->AttachChild(pgeom, 1);
    WaterShaderProperty* pkWadingWaterProp = new WaterShaderProperty;
    pgeom->AttachProperty(pkWadingWaterProp);
    BSShaderManager::PrepareObject(pgeom, 0, 0);
    pkWadingWaterProp->fBlendNormalsAmount = 1.0f;
    TESObjectREFR* pwaterref = apGroup->PlaceableWaterList.GetHead();
    if (pwaterref) {
        NiTriBasedGeom* pwatergeom = static_cast<NiTriBasedGeom*>(GetWaterGeometry(pwaterref));
        if (pwatergeom) {
            BSShader* pshader = pwatergeom->GetShader();
            if (pshader && pshader->iShaderType == 13) {
                WaterShaderProperty* pkWaterProp = static_cast<WaterShaderProperty*>(pwatergeom->GetProperty(3));
                pkWaterProp->Copy(pkWadingWaterProp);
            }
        }
    }
    if (TES::pInstance->pInteriorCell) {
        pkWadingWaterProp->spReflectionMap = apGroup->spGroupReflectionMap;
    }
    else if (apGroup->bGroupAtWorldSpaceWaterHeight) {
        pkWadingWaterProp->spReflectionMap = PlaceableWaterGroup::spWorldReflectionMap;
    }
    else {
        pkWadingWaterProp->spReflectionMap = PlaceableWaterGroup::spSkyReflectionMap;
    }
    pkWadingWaterProp->spDepthMap = PlaceableWaterGroup::spDepthMap;
    pkWadingWaterProp->spNoiseNormalMap = apGroup->pWaterType->spNoiseNormalMap;
    pkWadingWaterProp->spDisplacementNormalMap = PlaceableWaterGroup::spWadingWaterHeightMap->GetTexture(0);
    pkWadingWaterProp->bDisplacement = true;
    pkWadingWaterProp->fBlendRadius = 1.0f;
    pkWadingWaterProp->iStencilMask = apGroup->iStencilBitMask;
    fTimeSinceLastRipplePlaced = 0.0f;
    return pgeom;
}

bool TESWaterSystem::ActorWithinRangeOfWadingWater(PlaceableWaterGroup* apGroup) {
    void* actoriter = apGroup->ActorsInWaterList.m_pkHead;
    void* actornext = nullptr;
    while (actoriter) {
        actornext = apGroup->ActorsInWaterList.GetNextPos(static_cast<NiTListItem<Actor*>*>(actoriter));
        if (actoriter) {
            TESObjectREFR* pactor = apGroup->ActorsInWaterList.Get(static_cast<NiTListItem<Actor*>*>(actoriter));
            if (pPlayer == pactor) {
                return true;
            }
            if (!pactor->IsDead(false)) {
                NiPoint3 actorpos;
                NiPoint3 playerpos = pPlayer->GetLocationOnReference();
                const NiPoint3& kActorPos = pactor->GetLocationOnReference();
                actorpos.x = kActorPos.x;
                actorpos.y = kActorPos.y;
                playerpos.z = 0.0f;
                actorpos.z = 0.0f;
                float dist = (playerpos - actorpos).Length();
                if (gs_fWadingWaterQuadSize_Water.Float() >= static_cast<double>(dist)) {
                    return true;
                }
            }
        }
        actoriter = actornext;
    }
    return false;
}

void TESWaterSystem::RenderDisplacementWaterStencil() {
    NiCamera cullingCamera;
    float fMaxHeight = std::numeric_limits<float>::max();
    float fMinHeight = std::numeric_limits<float>::max();
    NiTListItem<PlaceableWaterGroup*>* iter = PlaceableWaterGroupList.m_pkHead;
    NiTListItem<PlaceableWaterGroup*>* pNextPos = nullptr;
    while (iter) {
        pNextPos = PlaceableWaterGroupList.GetNextPos(iter);
        if (iter) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(iter);
            float fHeight = pGroup->ReflectWaterPlane.m_fConstant;
            if (fMaxHeight < static_cast<double>(fHeight) || fMaxHeight == 3.4028234663852886e+38) {
                fMaxHeight = fHeight;
            }
            if (fMinHeight > static_cast<double>(fHeight) || fMinHeight == 3.4028234663852886e+38) {
                fMinHeight = fHeight;
            }
        }
        iter = pNextPos;
    }
    NiFrustum newFrustum(false);
    newFrustum.m_bOrtho = true;
    newFrustum.m_fNear = 0.1f;
    newFrustum.m_fLeft = -(gs_fWadingWaterQuadSize_Water.Float() / 2.0);
    newFrustum.m_fRight = gs_fWadingWaterQuadSize_Water.Float() / 2.0;
    newFrustum.m_fTop = gs_fWadingWaterQuadSize_Water.Float() / 2.0;
    newFrustum.m_fBottom = -(gs_fWadingWaterQuadSize_Water.Float() / 2.0);
    float fHeightRange = fMinHeight - fMaxHeight;
    newFrustum.m_fFar = std::fabs(fHeightRange) + 100.0;
    float fCameraZ = fMaxHeight + 50.0;
    float playerY = pPlayer->GetLocationOnReference().y;
    cullingCamera.SetTranslate(NiPoint3(pPlayer->GetLocationOnReference().x, playerY, fCameraZ));
    cullingCamera.SetViewFrustum(newFrustum);
    NiUpdateData arData(0.0f, false, false);
    cullingCamera.Update(arData);
    playerY = pPlayer->GetLocationOnReference().y;
    NiPoint3 kWorldPt(pPlayer->GetLocationOnReference().x, playerY, fMaxHeight);
    cullingCamera.LookAtWorldPoint(kWorldPt, NiPoint3::UNIT_Y);
    const NiRect<float>* pViewport = &cullingCamera.GetViewPort();
    NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pViewport);
    TESWaterSystem::bIsRendering = true;
    BSCullingProcess cullingProcess(nullptr);
    cullingProcess.m_pkCamera = &cullingCamera;
    NiRenderer::GetRenderer()->SetCameraData(&cullingCamera);
    cullingProcess.SetFrustum(cullingCamera.GetViewFrustum());
    iter = PlaceableWaterGroupList.m_pkHead;
    pNextPos = nullptr;
    while (iter) {
        pNextPos = PlaceableWaterGroupList.GetNextPos(iter);
        if (iter) {
            PlaceableWaterGroup* pGroup = PlaceableWaterGroupList.Get(iter);
            NiTListItem<TESObjectREFR*>* pWaterPos = pGroup->PlaceableWaterList.m_pkHead;
            NiTListItem<TESObjectREFR*>* pNextWaterPos = nullptr;
            while (pWaterPos) {
                pNextWaterPos = pGroup->PlaceableWaterList.GetNextPos(pWaterPos);
                if (pWaterPos) {
                    TESObjectREFR* pRef = pGroup->PlaceableWaterList.Get(pWaterPos);
                    if (pRef && pRef->GetParentCell() && pRef->GetParentCell()->GetAttached()) {
                        NiTriStrips* pGeometry = static_cast<NiTriStrips*>(GetWaterGeometry(pRef));
                        if (pGeometry) {
                            BSShader* pShader = pGeometry->GetShader();
                            if (pShader && pShader->iShaderType == 13) {
                                WaterShaderProperty* shader = static_cast<WaterShaderProperty*>(
                                    pGeometry->GetProperty(3)
                                );
                                uint32_t iStencilMask = shader->iStencilMask;
                                shader->iStencilMask = 1;
                                if (IsWaterNodeVisible(pRef, &cullingCamera) &&
                                    shader &&
                                    shader->GetWaterPass(pGeometry)) {
                                    BSShaderProperty::RenderPass* pRenderPass = shader->GetWaterPass(pGeometry);
                                    BSBatchRenderer::RenderPassImmediately(
                                        pRenderPass,
                                        pRenderPass->usPassEnum,
                                        0,
                                        0,
                                        0
                                    );
                                }

                                shader->iStencilMask = iStencilMask;
                            }
                        }
                    }
                }
                pWaterPos = pNextWaterPos;
            }
        }
        iter = pNextPos;
    }
    cullingProcess.m_pkCamera = nullptr;
    BSShaderManager::SetRenderFlag(0, false);
    TESWaterSystem::bIsRendering = false;
}

BSRenderedTexture* TESWaterSystem::InitializeGreyTexture() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_WATER,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWater.cpp",
        0x138A
    );
    NiColorA tmpcolor(0.0f, 0.0f, 0.0f, 0.0f);
    NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
    BSRenderedTexture* pTexture = pTextureManager->BorrowRenderedTexture(
        pRenderer,
        BSTextureManager::TEX_TESWATER_GREY,
        0,
        nullptr,
        0
    );
    BSShaderManager::GetRenderer()->GetBackgroundColor(tmpcolor);
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(NiColorA(0.5f, 0.5f, 0.5f, 0.5f));
    NiRenderTargetGroup* pTarget = pTexture->GetGroup();
    BSRenderedTexture::StartOffscreen(7u, pTarget);
    BSRenderedTexture::StopOffscreen();
    BSShaderManager::GetRenderer()->SetBackgroundColorAlpha(tmpcolor);
    return pTexture;
}

void TESWaterSystem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NiNode* TESWaterSystem::GetWaterRoot() {
    return spWaterRoot.m_pObject;
}

void TESWaterSystem::ClearWaterRoot() {
    if (spWaterRoot) {
        NiNode* pRoot = spWaterRoot;
        pRoot->RemoveAllChildren();
    }
    spWaterRoot = nullptr;
}

bool TESWaterSystem::GetPlayerUnderwater() {
    return bUnderWater;
}

TESWaterForm* TESWaterSystem::GetCurrentWaterType() {
    return pCurrentWaterType;
}

bool TESWaterSystem::GetCameraInPlaceableWater() {
    return bCameraInPlaceableWater;
}

void TESWaterSystem::SetPlayerForceUnderwater(bool abVal) {
    bForceUnderwater = abVal;
}

void TESWaterSystem::SetPlayerUnderwater(bool abIsUnderWater) {
    bUnderWater = abIsUnderWater;
}

void TESWaterSystem::SetCameraRelevantWaterHeight(float afVal) {
    fCameraRelevantWaterHeight = afVal;
}

bool TESWaterSystem::GetUseWaterShader() {
    return bUseWaterShader.Bool();
}

void TESWaterSystem::SetLastWaterCell(TESObjectCELL* apCell) {
    TESWaterSystem::pLastWaterCell = apCell;
}

bool TESWaterSystem::GetWaterEnabled() {
    return TESWaterSystem::bWaterEnabled;
}

void TESWaterSystem::SetCurrentWaterType(TESWaterForm* apType) {
    TESWaterSystem::pCurrentWaterType = apType;
}

void TESWaterSystem::SetCameraInPlaceableWater(bool abVal) {
    TESWaterSystem::bCameraInPlaceableWater = abVal;
}

bool TESWaterSystem::GetUseWaterDisplacements() {
    return TESWaterSystem::bUseWaterDisplacements.Bool();
}

WadingWaterData::WadingWaterData()
  : fDisplaceOffset(NiPoint2::ZERO), fLastDisplaceOffset(NiPoint2::ZERO), fLastPosition(NiPoint3::ZERO) {}

const NiPoint3& NiPlane::GetNormal() const {
    return m_kNormal;
}

float NiPlane::GetConstant() const {
    return m_fConstant;
}

float NiPoint2::Unitize() {
    float fLength = Length();
    if (fLength > 9.999999974752427e-07) {
        float fRecip = 1.0 / fLength;
        x = x * fRecip;
        y = y * fRecip;
    }
    else {
        x = 0.0;
        y = 0.0;
        return 0.0;
    }
    return fLength;
}

NiPoint2& NiPoint2::operator/=(float fScalar) {
    x = x / fScalar;
    y = y / fScalar;
    return *this;
}

NiPoint2& NiPoint2::operator+=(const NiPoint2& pt) {
    x = x + pt.x;
    y = y + pt.y;
    return *this;
}

NiPoint2& NiPoint2::operator-=(const NiPoint2& pt) {
    x = x - pt.x;
    y = y - pt.y;
    return *this;
}
