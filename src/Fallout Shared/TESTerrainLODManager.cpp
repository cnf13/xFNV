#include "Fallout Shared/TESTerrainLODManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShader.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESGrassAreaParam.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bDrawShaderGrass("bDrawShaderGrass:Grass", true);
SettingT<INISettingCollection> bGrassPointLighting("bGrassPointLighting:Grass", false);
SettingT<INISettingCollection> fGrassMinStartFadeDistance("fGrassMinStartFadeDistance:Grass", 400.0f);
SettingT<INISettingCollection> fGrassMaxStartFadeDistance("fGrassMaxStartFadeDistance:Grass", 7000.0f);
SettingT<INISettingCollection> fGrassDefaultStartFadeDistance("fGrassDefaultStartFadeDistance:Grass", 3500.0f);
SettingT<INISettingCollection> fGrassFadeRange("fGrassFadeRange:Grass", 1000.0f);
SettingT<RendererSettingCollection>
    fGrassStartFadeDistance("fGrassStartFadeDistance:Grass", fGrassDefaultStartFadeDistance.Float());
SettingT<INISettingCollection> iMinGrassSize("iMinGrassSize:Grass", 80);
SettingT<INISettingCollection> fGrassWindMagnitudeMin("fGrassWindMagnitudeMin:Grass", 5.0f);
SettingT<INISettingCollection> fGrassWindMagnitudeMax("fGrassWindMagnitudeMax:Grass", 125.0f);

// The first manager allocates the shared low-detail grass buffers, the last one frees them.
TESTerrainLODManager::TESTerrainLODManager() {
    if (!iManagerCount) {
        int32_t iSizeCap = (TESObjectLAND::GetGrassEvalSize() << 8) / iMinGrassSize.Int();
        uint32_t uiCount = static_cast<uint32_t>(iSizeCap * iSizeCap);
        if (!pLowpositions) {
            pLowpositions = static_cast<NiPoint3*>(ourMalloc(uiCount * sizeof(NiPoint3)));
        }
        if (!pLowcolors) {
            pLowcolors = static_cast<float*>(ourMalloc(uiCount * sizeof(float)));
        }
    }
    ++iManagerCount;
    TallGrassShader::SetWindRanges(fGrassWindMagnitudeMin.Float(), fGrassWindMagnitudeMax.Float());
}

TESTerrainLODManager::~TESTerrainLODManager() {
    if (!--iManagerCount) {
        if (pLowpositions) {
            ourFree(pLowpositions);
            pLowpositions = nullptr;
        }
        if (pLowcolors) {
            ourFree(pLowcolors);
            pLowcolors = nullptr;
        }
    }
}

void TESTerrainLODManager::UpdateGrass(NiPoint3 apCameraPosition, NiPoint3 apCameraDirection, float afUpdateAngle) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_GRASS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp",
        177
    );
    if (!TES::pInstance->pInteriorCell && bDrawShaderGrass.Bool() && bIsToggleGrassUpdate) {
        if (!TESTerrainLODManager::bGrassInitialized) {
            TESTerrainLODManager::InitGrass();
        }
        TallGrassShaderProperty::bPointLighting = bGrassPointLighting.Bool();
        float fMaxRange = fGrassStartFadeDistance.Float() + fGrassFadeRange.Float();
        float fFadeRange = fGrassStartFadeDistance.Float();
        if (!(fGrassStartFadeDistance.Float() < fGrassMinStartFadeDistance.Float() + 0.5)) {
            if (!TESTerrainLODManager::pGrassNode) {
                NiNode* pBaseNode = BSShaderManager::GetShadowSceneNode(0);
                NiAVObject* pGrassObj = pBaseNode->GetObjectByName(FixedStrings::QGrass());
                TESTerrainLODManager::pGrassNode = static_cast<NiNode*>(pGrassObj);
                if (!pGrassObj) {
                    NiNode* pNode = new NiNode(0);
                    TESTerrainLODManager::pGrassNode = pNode;
                    pNode->SetName(FixedStrings::QGrass());
                    pBaseNode->AttachChild(TESTerrainLODManager::pGrassNode, 1);
                }
            }
            TallGrassShaderProperty::SetFadeParameters(0.0f, 0.0f, fFadeRange, fMaxRange);
            int32_t iCellX = static_cast<int32_t>(apCameraPosition.x) / 0x1000;
            int32_t iCellY = static_cast<int32_t>(apCameraPosition.y) / 0x1000;
            if (apCameraPosition.x < 0.0) {
                --iCellX;
            }
            if (apCameraPosition.y < 0.0) {
                --iCellY;
            }
            TESObjectCELL* pCell = TES::pInstance->GetGridCellCell(iCellX, iCellY);
            NiPoint2 kCameraCellPos(apCameraPosition.x, apCameraPosition.y);
            kCameraCellPos.x = kCameraCellPos.x - static_cast<double>(iCellX << 12);
            kCameraCellPos.y = kCameraCellPos.y - static_cast<double>(iCellY << 12);
            for (int32_t iCitX = 0; iCitX < 5; ++iCitX) {
                for (int32_t iCitY = 0; iCitY < 5; ++iCitY) {
                    pCell = TES::pInstance->GetGridCellCell(iCellX + iCitX - 2, iCellY + iCitY - 2);
                    if (pCell) {
                        bool bRemove = true;
                        if (iCitX && iCitX != 4 && iCitY && iCitY != 4) {
                            double dDistance = pCell->GetDistanceFromPoint(apCameraPosition);
                            if (fMaxRange > dDistance) {
                                TESTerrainLODManager::UpdateGrassForCell(
                                    pCell,
                                    TESTerrainLODManager::pGrassNode,
                                    apCameraPosition,
                                    apCameraDirection,
                                    fFadeRange,
                                    fMaxRange,
                                    afUpdateAngle
                                );
                                bRemove = false;
                            }
                        }

                        if (bRemove) {
                            int32_t iDataY = pCell->GetDataY();
                            int32_t iDataX = pCell->GetDataX();
                            TallGrassShaderProperty::RemoveFromCell(iDataX, iDataY);
                        }
                    }
                }
            }
            if (bGrassPointLighting) {
                NiNode* pNode = TESTerrainLODManager::pGrassNode;
                NiNode* pBaseNode = BSShaderManager::GetShadowSceneNode(0);
                static_cast<ShadowSceneNode*>(pBaseNode)->ShadowSceneNode::UpdateObjectLighting(pNode, false);
            }
            if (TESTerrainLODManager::pGrassNode) {
                NiUpdateData kUpdateData(0.0f, false, false);
                TESTerrainLODManager::pGrassNode->Update(kUpdateData);
                TESTerrainLODManager::pGrassNode->UpdateProperties();
            }
        }
    }
    else {
        TallGrassShaderProperty::RemoveAllGrass();
    }
}

void TESTerrainLODManager::InitGrass() {
    AutoMemContext kMemContext(
        MEM_CONTEXT_GRASS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp",
        144
    );
    float fRadius = 0.0f;
    if (fGrassStartFadeDistance.Float() >= fGrassMinStartFadeDistance.Float() + 0.5) {
        fRadius = fGrassStartFadeDistance.Float() + fGrassFadeRange.Float();
    }
    if (fRadius > static_cast<double>(uGridsToLoad.UInt() << 12)) {
        fRadius = static_cast<float>(uGridsToLoad.UInt() << 12);
    }
    TESTerrainLODManager::fGrassRadius = fRadius;
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
    NiNode* pNode = new NiNode(0);
    TESTerrainLODManager::pGrassNode = pNode;
    pNode->SetName(FixedStrings::QGrass());
    pShadowSceneNode->SetAt(6u, TESTerrainLODManager::pGrassNode);
    TESTerrainLODManager::pGrassNode->UpdateProperties();
    TESTerrainLODManager::bGrassInitialized = true;
}

NiNode* TESTerrainLODManager::GetGrassNode() {
    return TESTerrainLODManager::pGrassNode;
}

void TESTerrainLODManager::UpdateGrassForCell(
    TESObjectCELL* apCell,
    NiNode* apGrassNode,
    NiPoint3 apCameraPosition,
    NiPoint3 apCameraDirection,
    float fFadeRange,
    float fMaxRange,
    float afUpdateAngle
) {
    NiPoint3 kVertexLocation;
    NiFrustum kCurrentFrustum;
    if (!bTempTest) {
        BSSimpleList<TESLandTexture*> kLandTextures;
        COORD_DATA coordData;
        uint16_t usHeightLimit = 0;
        uint8_t ucMaxSlope = 0;
        uint8_t ucMinSlope = 0;
        int32_t iGrassEvalSize = TESObjectLAND::GetGrassEvalSize();
        int32_t iGrassEvalSquared = 2 * iGrassEvalSize;
        TESGrass::GRASS_WATER_STATE eWaterState = TESGrass::GWS_COUNT;
        if (apCell && apCell->GetWorldSpace() && apCell->GetLand()) {
            alignas(16) uint8_t kVertexPosStorage[sizeof(NiPlane)];
            NiPlane& kVertexPos = *reinterpret_cast<NiPlane*>(kVertexPosStorage);
            kVertexPos.m_fConstant = apCell->GetWaterHeight();
            TESObjectLAND* pLand = apCell->GetLand();
            int32_t iCellX = apCell->GetDataX();
            int32_t iCellY = apCell->GetDataY();
            apCameraDirection.z = 0.0f;
            apCameraDirection.Unitize();
            afUpdateAngle = afUpdateAngle + afUpdateAngle;
            NiFrustum kFrustum(false);
            NiRenderer* pRenderer = NiRenderer::GetRenderer();
            float fHeight = static_cast<float>(pRenderer->GetScreenHeight());
            pRenderer = NiRenderer::GetRenderer();
            float fWidth = static_cast<float>(pRenderer->GetScreenWidth());
            float fAspect = fHeight / fWidth;
            float fAspectDelta = 0.75 / (fHeight / fWidth);
            SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
            double dCameraFOV = pSceneGraph->GetCameraFOV();
            float fTheta = dCameraFOV * 0.01745329238474369 * 0.5 + afUpdateAngle;
            double dTan = std::tan(fTheta);
            kFrustum.m_fLeft = -dTan * fAspectDelta;
            dTan = std::tan(fTheta);
            kFrustum.m_fRight = dTan * fAspectDelta;
            dTan = std::tan(fTheta);
            kFrustum.m_fBottom = -dTan * fAspect * fAspectDelta;
            dTan = std::tan(fTheta);
            kFrustum.m_fTop = dTan * fAspect * fAspectDelta;
            pSceneGraph = Main::GetWorldSceneGraph();
            kCurrentFrustum = pSceneGraph->GetCamera()->GetViewFrustum();
            kFrustum.m_fFar = kCurrentFrustum.m_fFar;
            kFrustum.m_fNear = kCurrentFrustum.m_fNear;
            NiFrustumPlanes kPlanes;
            pSceneGraph = Main::GetWorldSceneGraph();
            kPlanes.Set(kFrustum, pSceneGraph->GetCamera()->GetWorldTransform());
            for (int32_t iQuad = 0; iQuad < 4; ++iQuad) {
                NiNode* pLandQuad = apCell->GetLandNode(iQuad);
                if (pLandQuad) {
                    int32_t iVertIndex;
                    int32_t iSeed = 0;
                    NiBound kBound = pLandQuad->GetWorldBound();
                    double dDistance = (apCameraPosition - kBound.GetCenter()).Length();
                    // Rounded to float before the compare, as above.
                    float fDist = dDistance - kBound.m_fRadius;
                    NiNode* pChildNode = apCell->GetChildNode(TESObjectCELL::CN_LAND);
                    BSMultiBoundNode* pMBNode = static_cast<BSMultiBoundNode*>(pChildNode->GetAt(iQuad));
                    BSMultiBoundShape::BSMBCullResult eCullResult = pMBNode->QMultiBound()->QShape()->kCullResult;
                    BSMultiBoundAABB* pAABB = static_cast<BSMultiBoundAABB*>(pMBNode->QMultiBound()->QShape());
                    bool bVisible = pAABB->WithinFrustum(kPlanes);
                    if ((eCullResult == BSMultiBoundShape::BS_CULL_VISIBLE ||
                         eCullResult == BSMultiBoundShape::BS_CULL_UNTESTED ||
                         bVisible) &&
                        fMaxRange > static_cast<double>(fDist)) {
                        for (int32_t iXVert = iGrassEvalSize; iXVert < 16; iXVert += iGrassEvalSquared) {
                            for (int32_t iYVert = iGrassEvalSize; iYVert < 16; iYVert += iGrassEvalSquared) {
                                iVertIndex = iXVert + 17 * iYVert;
                                NiPoint3* pVertexLocation = ::new (&kVertexLocation) NiPoint3(pLand->GetVertexLocation(
                                    static_cast<uint8_t>(iQuad),
                                    iXVert + 17 * iYVert,
                                    nullptr,
                                    nullptr
                                ));
                                kVertexPos.m_kNormal.x = pVertexLocation->x;
                                kVertexPos.m_kNormal.y = pVertexLocation->y;
                                kVertexPos.m_kNormal.z = apCameraPosition.z;
                                kVertexPos.m_kNormal.x = kVertexPos.m_kNormal.x - 256.0;
                                kVertexPos.m_kNormal.y = kVertexPos.m_kNormal.y - 256.0;
                                iSeed = TallGrassShaderProperty::Hash_2DPosition(kVertexPos.m_kNormal);
                                TESGrassAreaParam** pParam = pLand->GetGrassParameters(iQuad, iVertIndex);
                                for (int32_t l = 0;
                                     pParam && pParam[l] && l < 16 && pParam[l]->kGrassParam.pcGeomFilename;
                                     ++l) {
                                    if (pParam[l] &&
                                        !TallGrassShaderProperty::ContainsChunk(
                                            pParam[l]->kGrassParam.iGrassID,
                                            iSeed
                                        )) {
                                        TESGrass* pGrass = static_cast<TESGrass*>(
                                            TESForm::GetFormByNumericID(pParam[l]->kGrassParam.iGrassID)
                                        );
                                        if (pGrass) {
                                            usHeightLimit = pGrass->GetDistanceFromWaterLevel();
                                            eWaterState = pGrass->GetUnderwaterState();
                                            ucMinSlope = pGrass->GetMaxSlopeDegrees();
                                            ucMaxSlope = pGrass->GetMinSlopeDegrees();
                                        }
                                        TESTerrainLODManager::CreateGrass(
                                            iCellX,
                                            iCellY,
                                            iQuad,
                                            pLand,
                                            iSeed,
                                            kVertexPos.m_kNormal,
                                            apGrassNode,
                                            apCameraPosition,
                                            &pParam[l]->kGrassParam,
                                            pParam[l]->pfDensity,
                                            fFadeRange,
                                            fMaxRange,
                                            usHeightLimit,
                                            eWaterState,
                                            kVertexPos.m_fConstant,
                                            ucMaxSlope,
                                            ucMinSlope
                                        );
                                    }
                                }
                            }
                        }
                    }
                    else {
                        for (int32_t iXVert = iGrassEvalSize; iXVert < 16; iXVert += iGrassEvalSquared) {
                            for (int32_t iYVert = iGrassEvalSize; iYVert < 16; iYVert += iGrassEvalSquared) {
                                iVertIndex = iXVert + 17 * iYVert;
                                kVertexPos.m_kNormal = pLand->GetVertexLocation(
                                    static_cast<uint8_t>(iQuad),
                                    iXVert + 17 * iYVert,
                                    nullptr,
                                    nullptr
                                );
                                kVertexPos.m_kNormal.x = kVertexPos.m_kNormal.x - 256.0;
                                kVertexPos.m_kNormal.y = kVertexPos.m_kNormal.y - 256.0;
                                iSeed = TallGrassShaderProperty::Hash_2DPosition(kVertexPos.m_kNormal);
                                TallGrassShaderProperty::RemoveFromTerrainChunk(iSeed);
                            }
                        }
                    }
                }
                kLandTextures.RemoveAll();
            }
        }
    }
}

void TESTerrainLODManager::CreateGrass(
    int32_t aiCellx,
    int32_t aiCelly,
    int32_t aiBlock,
    TESObjectLAND* apLand,
    int32_t aiSeed,
    const NiPoint3& aCoord,
    NiNode* apGrassNode,
    NiPoint3 apPosition,
    TallGrassShaderProperty::GrassParam* apGrassParam,
    float* apfDensity,
    float fFadeRange,
    float fMaxRange,
    uint16_t iHeightLimit,
    TESGrass::GRASS_WATER_STATE iWaterState,
    float afCellWaterHeight,
    uint8_t aiMinSlope,
    uint8_t aiMaxSlope
) {
    NiPoint3 kOffset;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_GRASS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp",
        522
    );
    NiPointer<NiGeometry> spBlock(static_cast<NiGeometry*>(nullptr));
    float fMinSlopeAngle = static_cast<double>(aiMinSlope) * 0.01745329238474369;
    float fMinSlope = NiCosQ(fMinSlopeAngle);
    float fMaxSlopeAngle = static_cast<double>(aiMaxSlope) * 0.01745329238474369;
    float fMaxSlope = NiCosQ(fMaxSlopeAngle);
    fMaxRange = fMaxRange * fMaxRange;
    bool bContainsChunk = TallGrassShaderProperty::ContainsChunk(apGrassParam, aiSeed);
    if (!bContainsChunk && apLand->IsLoaded()) {
        float fGrassArea = static_cast<float>(TESObjectLAND::GetGrassEvalSize() << 8);
        uint16_t usGrassSize = static_cast<int32_t>(fGrassArea / apGrassParam->fPositionRange);
        int32_t iGrassEvalSize = TESObjectLAND::GetGrassEvalSize();
        uint16_t usMaxSize = (iGrassEvalSize << 8) / iMinGrassSize.Int();
        if (usGrassSize > static_cast<int32_t>(usMaxSize)) {
            usGrassSize = usMaxSize;
        }
        COORD_DATA aCoordData;
        uint16_t usLowGrassCount = 0;
        for (int32_t i = 0; i < usGrassSize; ++i) {
            for (int32_t j = 0; j < usGrassSize; ++j) {
                float fY = static_cast<double>(j) * (fGrassArea / static_cast<double>(usGrassSize));
                float fX = static_cast<double>(i) * (fGrassArea / static_cast<double>(usGrassSize));
                NiPoint3* pOffset = ::new (&kOffset) NiPoint3(fX, fY, 0.0f);
                NiPoint3 kNewPos = aCoord + *pOffset;
                float fDensY = static_cast<double>(j) * (3.0 / static_cast<double>(usGrassSize));
                float fDensX = static_cast<double>(i) * (3.0 / static_cast<double>(usGrassSize));
                int32_t iDensX = FloatToInt(fDensX);
                int32_t iDensY = FloatToInt(fDensY);
                fDensX = fDensX - static_cast<double>(iDensX);
                fDensY = fDensY - static_cast<double>(iDensY);
                float fd1 = 0.0f;
                float fd2 = 0.0f;
                float fd3 = 0.0f;
                float fd4 = 0.0f;
                fd1 = apfDensity[3 * iDensY + iDensX];
                if (fDensX > 0.01) {
                    if (iDensX >= 2) {
                        fd2 = apfDensity[3 * iDensY + iDensX];
                    }
                    else {
                        fd2 = apfDensity[3 * iDensY + 1 + iDensX];
                    }
                    if (fDensY > 0.01) {
                        if (iDensY >= 2) {
                            if (iDensX >= 2) {
                                fd4 = apfDensity[3 * iDensY + iDensX];
                            }
                            else {
                                fd4 = apfDensity[3 * iDensY + 1 + iDensX];
                            }
                        }
                        else {
                            if (iDensX >= 2) {
                                fd4 = apfDensity[3 * iDensY + 3 + iDensX];
                            }
                            else {
                                fd4 = apfDensity[3 * iDensY + 4 + iDensX];
                            }
                        }
                    }
                }
                if (fDensY > 0.01) {
                    if (iDensY >= 2) {
                        fd3 = apfDensity[3 * iDensY + iDensX];
                    }
                    else {
                        fd3 = apfDensity[3 * iDensY + 3 + iDensX];
                    }
                }
                float fDens = ((1.0 - fDensX) * fd1 + fd2 * fDensX) * (1.0 - fDensY) +
                    ((1.0 - fDensX) * fd3 + fd4 * fDensX) * fDensY;
                int32_t iDens = static_cast<int64_t>(fDens * 32768.0);
                if (iDens) {
                    int32_t iRandom = RandomInt(0, 0x7FFF);
                    if (iRandom < iDens) {
                        NiPoint3 kVect;
                        NiPoint3 kLandNormal;
                        NiPoint3 kFaceNormal;
                        float fHeight = 0.0f;
                        float fRandom = RandomFloatNeg1To1();
                        kNewPos.x = fRandom * apGrassParam->fPositionRange + kNewPos.x;
                        fRandom = RandomFloatNeg1To1();
                        kNewPos.y = fRandom * apGrassParam->fPositionRange + kNewPos.y;
                        kNewPos.x = FloatFloor(kNewPos.x);
                        kNewPos.y = FloatFloor(kNewPos.y);
                        kNewPos.z = 0.0f;
                        bool bThisCell = apLand->GetCoordData(&aCoordData, kNewPos, false);
                        TES::pInstance->GetLandHeight(kNewPos, fHeight);
                        switch (iWaterState) {
                            case TESGrass::GWS_ABOVE_ONLY_ATLEAST: {
                                if (!(fHeight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
                                    break;
                                }
                                continue;
                            }
                            case TESGrass::GWS_ABOVE_ONLY_ATMOST: {
                                if (!(afCellWaterHeight > static_cast<double>(fHeight)) &&
                                    !(fHeight > static_cast<double>(iHeightLimit) + afCellWaterHeight)) {
                                    break;
                                }
                                continue;
                            }
                            case TESGrass::GWS_BELOW_ONLY_ATLEAST: {
                                if (!(fHeight > afCellWaterHeight - static_cast<double>(iHeightLimit))) {
                                    break;
                                }
                                continue;
                            }
                            case TESGrass::GWS_BELOW_ONLY_ATMOST: {
                                if (!(afCellWaterHeight < static_cast<double>(fHeight)) &&
                                    !(fHeight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
                                    break;
                                }
                                continue;
                            }
                            case TESGrass::GWS_BOTH_ATLEAST: {
                                if (!(fHeight < static_cast<double>(iHeightLimit) + afCellWaterHeight) ||
                                    !(fHeight > afCellWaterHeight - static_cast<double>(iHeightLimit))) {
                                    break;
                                }
                                continue;
                            }
                            case TESGrass::GWS_BOTH_ATMOST: {
                                if (!(fHeight > static_cast<double>(iHeightLimit) + afCellWaterHeight) &&
                                    !(fHeight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
                                    break;
                                }
                                continue;
                            }
                            default: {
                                break;
                            }
                        }
                        ::new (&kVect) NiPoint3(kNewPos - apPosition);
                        kNewPos.z = FloatFloor(fHeight);
                        if (bThisCell) {
                            apLand->GetLandNormal(aCoordData, kLandNormal, kFaceNormal);
                        }
                        else {
                            TES::pInstance->GetLandNormal(kNewPos, kLandNormal, kFaceNormal);
                        }
                        if (apGrassParam->bFitToSlope && apGrassParam->bVertexLighting) {
                            kLandNormal = kFaceNormal;
                        }
                        else if (apGrassParam->bFitToSlope && !apGrassParam->bVertexLighting) {
                            kLandNormal = kLandNormal + kFaceNormal;
                            kLandNormal.Unitize();
                        }
                        if (!(fMaxSlope > static_cast<double>(kLandNormal.z)) &&
                            !(fMinSlope < static_cast<double>(kLandNormal.z))) {
                            kLandNormal.x = kLandNormal.x * 0.5 + 0.5;
                            if (kLandNormal.x >= 0.9700000286102295) {
                                kLandNormal.x = 0.97f;
                            }
                            kLandNormal.y = kLandNormal.y * 0.5 + 0.5;
                            if (kLandNormal.y >= 0.9700000286102295) {
                                kLandNormal.y = 0.97f;
                            }
                            kLandNormal.z = kLandNormal.z * 0.5 + 0.5;
                            if (kLandNormal.z >= 0.9700000286102295) {
                                kLandNormal.z = 0.97f;
                            }
                            kNewPos.x = kNewPos.x + kLandNormal.x;
                            kNewPos.y = kNewPos.y + kLandNormal.y;
                            kNewPos.z = kNewPos.z + kLandNormal.z;
                            NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
                            if (bThisCell) {
                                apLand->GetLandColor(aCoordData, kColor);
                            }
                            else {
                                TES::pInstance->GetLandColor(kNewPos, kColor);
                            }
                            float fLum = kColor.r * 0.31 + kColor.g * 0.37 + kColor.b * 0.32;
                            kVect = kNewPos - apPosition;
                            TESTerrainLODManager::pLowpositions[usLowGrassCount] = kNewPos;
                            TESTerrainLODManager::pLowcolors[usLowGrassCount++] = fLum;
                        }
                    }
                }
            }
        }
        if (usLowGrassCount) {
            TallGrassShaderProperty::AddGrass(
                aiSeed,
                aiCellx,
                aiCelly,
                apGrassNode,
                apGrassParam,
                TESTerrainLODManager::pLowpositions,
                TESTerrainLODManager::pLowcolors,
                usLowGrassCount
            );
        }
    }
}
