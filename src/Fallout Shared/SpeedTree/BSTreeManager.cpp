#include "Fallout Shared/SpeedTree/BSTreeManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/SpeedTreeShader.h"
#include "BSShader/TallGrassShader.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/Region System/TESRegionDataLandscape.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <cmath>
#include <cstdint>

SettingT<GameSettingCollection> gs_fTreeNearDistanceBase("fTreeNearDistanceBase", 2048.0f);
SettingT<GameSettingCollection> gs_fTreeFarDistanceBase("fTreeFarDistanceBase", 16384.0f);
SettingT<GameSettingCollection> gs_fTreeSizeConversion("fTreeSizeConversion", 10.0f);
SettingT<GameSettingCollection> gs_fLeafRustleTimeScale("fLeafRustleTimeScale", 0.5f);
SettingT<GameSettingCollection> gs_fLeafRockSpeedSwayInfluence("fLeafRockSpeedSwayInfluence", 1.0f);
SettingT<GameSettingCollection> gs_fLeafRustleSpeedSwayInfluence("fLeafRustleSpeedSwayInfluence", 1.0f);
SettingT<GameSettingCollection> gs_fLeafRockAmountSwayInfluence("fLeafRockAmountSwayInfluence", 1.0f);
SettingT<GameSettingCollection> gs_fLeafRustleAmountSwayInfluence("fLeafRustleAmountSwayInfluence", 1.0f);
SettingT<INISettingCollection> gs_bEnableTrees_SpeedTree("bEnableTrees:SpeedTree", true);
SettingT<INISettingCollection> gs_fTreeForceCS_SpeedTree("fTreeForceCS:SpeedTree", -1.0f);
SettingT<INISettingCollection> gs_fTreeForceBranchDimming_SpeedTree("fTreeForceBranchDimming:SpeedTree", -1.0f);
SettingT<INISettingCollection> gs_fTreeForceLeafDimming_SpeedTree("fTreeForceLeafDimming:SpeedTree", -1.0f);
SettingT<INISettingCollection> gs_fTreeForceMinBudAngle_SpeedTree("fTreeForceMinBudAngle:SpeedTree", -1.0f);
SettingT<INISettingCollection> gs_fTreeForceMaxBudAngle_SpeedTree("fTreeForceMaxBudAngle:SpeedTree", -1.0f);
SettingT<GameSettingCollection> BSTreeManager::fTreeWindVariance("fTreeWindVariance", 0.05f);
SettingT<GameSettingCollection> BSTreeManager::fLeafRockTimeScale("fLeafRockTimeScale", 2.0f);
SettingT<INISettingCollection> BSTreeManager::bForceFullLOD("bForceFullLOD:SpeedTree", false);
SettingT<INISettingCollection> BSTreeManager::fTreeLODExponent("fTreeLODExponent:SpeedTree", 1.0f);
SettingT<INISettingCollection> BSTreeManager::fTreeForceLLA("fTreeForceLLA:SpeedTree", -1.0f);
SettingT<INISettingCollection> gs_iCanopyShadowScale_SpeedTree("iCanopyShadowScale:SpeedTree", 512);
SettingT<INISettingCollection> gs_fCanopyShadowGrassMult_SpeedTree("fCanopyShadowGrassMult:SpeedTree", 1.0f);
BSCriticalSection BSTreeManager::kModelCriticalSection;

char SetWindMatrix(uint32_t auiMatrixIndex, float Yaw, float Pitch) {
    D3DXMATRIX kMatrix;
    if (auiMatrixIndex >= 4) {
        return 0;
    }
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
    D3DXMatrixRotationYawPitchRoll_Thunk(&kMatrix, Yaw, Pitch, 0.0f);
    D3DXMatrixTranspose_Thunk(&kMatrix, &kMatrix);
    SpeedTreeShader_pWindMatrices[auiMatrixIndex] = kMatrix;
    return 1;
}

BSTreeManager::BSTreeManager()
  : pModelMap(nullptr), spCanopyShadow(nullptr), spBillboardMatProp(nullptr), spDistantAlphaProp(nullptr),
    spHavokFlags(nullptr), fWindSpeed(0.0f), bShowTrees(true), bShowLeaves(true), bForceCompute(false),
    m_bForceFullLOD(bForceFullLOD.Bool()) {
    spBillboardMatProp = new NiMaterialProperty();
    spBillboardMatProp->SetEmittance(NiColor(1.0f, 1.0f, 1.0f));
    spDistantAlphaProp = new NiAlphaProperty();
    spDistantAlphaProp->SetAlphaTesting(true);
    spDistantAlphaProp->SetTestMode(NiAlphaProperty::TEST_GREATER);
    spDistantAlphaProp->SetTestRef(84);
    spDistantAlphaProp->SetAlphaBlending(false);
    spHavokFlags = new BSXFlags(3);
    CSpeedTreeRT::SetNumWindMatrices(4);
    CSpeedTreeRT::SetDropToBillboard(true);
    CSpeedTreeRT::SetTextureFlip(true);
    ShadowLightShader::SetProjectedShadowScale(static_cast<uint32_t>(gs_iCanopyShadowScale_SpeedTree.Int()));
    pBackgroundLoadedTreeMap = new LockFreeMap<TESObjectREFR*, BSTreeNode*>(2u, 37u, 12u);
}

BSTreeManager::~BSTreeManager() {
    ClearModelCache(false);
    BSShaderManager::SetProjectedShadowTexture(nullptr);
    BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
    GridCellArray::SetRecreateCanopyShadowMaskTexture(true);
    TESRegionDataLandscape::ClearDefaultTextureProps();
    delete pBackgroundLoadedTreeMap;
}

void BSTreeManager::RemoveBackgroundLoadedTree(TESObjectREFR* apRef) {
    BSTreeNode* pTreeNode = nullptr;
    if (pBackgroundLoadedTreeMap->GetAt(apRef, pTreeNode)) {
        pBackgroundLoadedTreeMap->RemoveAt(apRef);
        if (pTreeNode) {
            NiObjectGarbage::Add(pTreeNode);
        }
    }
}

BSTreeNode* BSTreeManager::CreateTreeForReference(TESObjectREFR* apTreeRefr, TESObjectTREE* apTreeObj) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp",
        462
    );
    NiPointer<BSTreeModel> spBaseModel(static_cast<BSTreeModel*>(nullptr));
    BSTreeModel* pModel = nullptr;
    if (!SpeedTree::bEnableTrees::Get() ||
        !apTreeObj ||
        !apTreeObj->GetModel() ||
        !apTreeObj->GetModelLength() ||
        (apTreeRefr && apTreeRefr->GetObjectReference() != apTreeObj)) {
        return nullptr;
    }
    BSTreeNode* pTreeNode = nullptr;
    if (pBackgroundLoadedTreeMap->GetAt(apTreeRefr, pTreeNode)) {
        pBackgroundLoadedTreeMap->RemoveAt(apTreeRefr);
        return pTreeNode;
    }
    uint32_t uiSeed = apTreeRefr ? apTreeRefr->GetSeed() : 1;
    BSTreeManager::GetInstance(true)->GetModel(apTreeObj, uiSeed, spBaseModel);
    if (!spBaseModel.m_pObject) {
        return nullptr;
    }
    if (spBaseModel->m_uiRefCount <= 2) {
        if (!spBaseModel->InitFromBase(apTreeObj, uiSeed)) {
            return nullptr;
        }
        pModel = spBaseModel.m_pObject;
    }
    else {
        pModel = new BSTreeModel;
        if (!pModel->InitAsInstance(spBaseModel.m_pObject)) {
            if (pModel) {
                delete pModel;
            }
            return nullptr;
        }
    }
    float fScale = apTreeRefr ? apTreeRefr->GetScale() : 1.0f;
    BSTreeNode* pRetNode = pModel->CreateArt(fScale);
    if (pRetNode) {
        NiFixedString kName(apTreeObj->GetModel());
        pRetNode->SetName(kName);
    }
    pModel->SetInitialized(true);
    return pRetNode;
}

void BSTreeManager::AddBackgroundLoadedTree(TESObjectREFR* apRef, BSTreeNode* apTreeNode) {
    if (!pBackgroundLoadedTreeMap->SetAt(apRef, apTreeNode, false) && apTreeNode) {
        delete apTreeNode;
    }
}

NiPointer<BSTreeModel>* BSTreeManager::GetModelArray(TESObjectTREE* apTree) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp",
        216
    );
    NiPointer<BSTreeModel>* pRetVal = nullptr;
    if (apTree) {
        if (!pModelMap) {
            NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>*
                pMap = ::new NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>(37u);
            pModelMap = pMap;
        }
        if (pModelMap->GetAt(apTree, pRetVal)) {
            return pRetVal;
        }

        pRetVal = new NiPointer<BSTreeModel>[1];
        pModelMap->SetAt(apTree, pRetVal);
        return pRetVal;
    }

    return nullptr;
}

void BSTreeManager::GetModel(TESObjectTREE* apTree, uint32_t aiSeed, NiPointer<BSTreeModel>& aspBSTreeModel) {
    BSTreeManager::kModelCriticalSection.Enter(nullptr);
    NiPointer<BSTreeModel> spRetTree(static_cast<BSTreeModel*>(nullptr));
    if (apTree) {
        NiPointer<BSTreeModel>* pModelArray = GetModelArray(apTree);
        int32_t iEmptySlot = -1;
        bool bFound = false;
        for (int32_t i = 0; i < 1; ++i) {
            BSTreeModel* pTree = pModelArray[i].m_pObject;
            if (pTree) {
                if (pTree->iSeed == aiSeed) {
                    spRetTree = pTree;
                    bFound = true;
                    break;
                }
            }
            else if (iEmptySlot == -1) {
                iEmptySlot = i;
            }
        }
        if (!bFound || !spRetTree.m_pObject) {
            if (iEmptySlot >= 0) {
                spRetTree = new BSTreeModel;
                pModelArray[iEmptySlot] = spRetTree;
            }
            else {
                spRetTree = *pModelArray;
            }
        }
    }
    BSTreeManager::kModelCriticalSection.Leave();
    aspBSTreeModel = spRetTree;
}

void BSTreeManager::UpdateWindMatrices() {
    static LeafSwayBaseTable ffrequencies{0.15f, 0.17f, 0.25f, 0.15f, 0.19f, 0.05f, 0.15f, 0.22f};
    static LeafSwayPhaseTable fmatrixTimes{};
    static float foldStrength = fWindSpeed;
    float fbaseAngle = fWindSpeed * 0.6100000143051147;
    float fSwayAmount = 0.0f;
    for (int32_t i = 0; i < 4; ++i) {
        fmatrixTimes[i] = appTimer.fDelta + fmatrixTimes[i];
        if (fWindSpeed != 0.0) {
            fmatrixTimes[i] = fmatrixTimes[i] * foldStrength / fWindSpeed;
        }
        float fbaseFreq = fWindSpeed * 20.0;
        float fXPhase = fbaseFreq * ffrequencies[2 * i] * fmatrixTimes[i];
        float fxPercent = NiSinQ(fXPhase);
        float fYPhase = fbaseFreq * ffrequencies[2 * i + 1] * fmatrixTimes[i];
        float fyPercent = NiCosQ(fYPhase);
        if (i == 2) {
            float fSwayX = std::fabs(fxPercent) + fSwayAmount;
            fSwayAmount = std::fabs(fyPercent) + fSwayX;
        }
        float fPitch = fbaseAngle * fyPercent;
        float fYaw = fbaseAngle * fxPercent;
        SetWindMatrix(i, fYaw, fPitch);
    }
    double dRockAmountBase = 1.0 - gs_fLeafRockAmountSwayInfluence.Float();
    float frockAmountSway = gs_fLeafRockAmountSwayInfluence.Float() * fSwayAmount / 2.0 + dRockAmountBase;
    double dRustleAmountBase = 1.0 - gs_fLeafRustleAmountSwayInfluence.Float();
    float frustleAmountSway = gs_fLeafRustleAmountSwayInfluence.Float() * fSwayAmount / 2.0 + dRustleAmountBase;
    double dRockSpeedBase = 1.0 - gs_fLeafRockSpeedSwayInfluence.Float();
    float frockSpeedSway = gs_fLeafRockSpeedSwayInfluence.Float() * fSwayAmount / 2.0 + dRockSpeedBase;
    double dRustleSpeedBase = 1.0 - gs_fLeafRustleSpeedSwayInfluence.Float();
    float frustleSpeedSway = gs_fLeafRustleSpeedSwayInfluence.Float() * fSwayAmount / 2.0 + dRustleSpeedBase;
    double dSeconds = appTimer.fDelta;
    float fRustleTimeScale = gs_fLeafRustleTimeScale.Float() * dSeconds * frustleSpeedSway;
    dSeconds = appTimer.fDelta;
    float fRockTimeScale = BSTreeManager::fLeafRockTimeScale.Float() * dSeconds * frockSpeedSway;
    float fRustleAmount = fWindSpeed * frustleAmountSway;
    float fRockAmount = fWindSpeed * frockAmountSway;
    SpeedTreeLeafShader::UpdateWindTimers(fRockAmount, fRustleAmount, fRockTimeScale, fRustleTimeScale);
    foldStrength = fWindSpeed;
}

void BSTreeManager::SetCamera(NiCamera* apCamera) {
    pWorldCamera = apCamera;
}

NiCamera* BSTreeManager::GetCamera() {
    return BSTreeManager::pWorldCamera;
}

BSTreeManager* BSTreeManager::GetInstance(bool abCreate) {
    if (pTreeManager == nullptr && abCreate) {
        Create(false);
    }
    return pTreeManager;
}

void BSTreeManager::Create(bool abRecreateIfFound) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_TREESYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp",
        177
    );
    if (pTreeManager) {
        if (!abRecreateIfFound) {
            return;
        }
        BSTreeManager::Destroy();
    }
    pTreeManager = new BSTreeManager;
}

void BSTreeManager::ClearCanopyShadow() {
    // GetInstance is called again for the release and the assignment goes through that result.
    if (GetInstance(true)->spCanopyShadow.m_pObject != nullptr) {
        GetInstance(true)->spCanopyShadow = nullptr;
        BSShaderManager::SetProjectedShadowTexture(nullptr);
        TES::pInstance->RemoveTextureImage("Data\\Textures\\Trees\\CanopyShadow.dds", 0);
    }
}

// Calls GetInstance again before each use of the model map.
void BSTreeManager::ClearModelCache(bool abClearManagerOnlyHeldOnly) {
    if (BSTreeManager::GetInstance(true)->pModelMap) {
        BSTreeManager::kModelCriticalSection.Enter(nullptr);
        if (abClearManagerOnlyHeldOnly) {
            NiTMapItem<TESObjectTREE*, NiPointer<BSTreeModel>*>* pPos = BSTreeManager::GetInstance(true)
                                                                            ->pModelMap->GetFirstPos();
            while (pPos) {
                TESObjectTREE* pKey;
                NiPointer<BSTreeModel>* pModelArray;
                BSTreeManager::GetInstance(true)->pModelMap->GetNext(pPos, pKey, pModelArray);
                if (pModelArray) {
                    // Only models no one but the manager holds are released.
                    bool bOnlyHeldHere = true;
                    for (int32_t i = 0; i < 1; ++i) {
                        if (pModelArray[i].m_pObject) {
                            if (pModelArray[i]->m_uiRefCount > 1) {
                                bOnlyHeldHere = false;
                            }
                            else {
                                pModelArray[i] = static_cast<BSTreeModel*>(nullptr);
                            }
                        }
                    }
                    if (bOnlyHeldHere) {
                        if (pModelArray) {
                            delete[] pModelArray;
                        }
                        pModelArray = nullptr;
                        BSTreeManager::GetInstance(true)->pModelMap->RemoveAt(pKey);
                        pPos = BSTreeManager::GetInstance(true)->pModelMap->GetFirstPos();
                    }
                }
            }
            if (BSTreeManager::GetInstance(true)->pModelMap->IsEmpty()) {
                BSTreeManager::GetInstance(true)->pModelMap->RemoveAll();
                NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* pModelMap = BSTreeManager::GetInstance(true)
                                                                                        ->pModelMap;
                if (pModelMap) {
                    delete pModelMap;
                }
                BSTreeManager::GetInstance(true)->pModelMap = nullptr;
            }
        }
        else {
            NiTMapItem<TESObjectTREE*, NiPointer<BSTreeModel>*>* pPos = BSTreeManager::GetInstance(true)
                                                                            ->pModelMap->GetFirstPos();
            while (pPos) {
                TESObjectTREE* pKey;
                NiPointer<BSTreeModel>* pModelArray;
                BSTreeManager::GetInstance(true)->pModelMap->GetNext(pPos, pKey, pModelArray);
                if (pModelArray) {
                    delete[] pModelArray;
                }
            }
            BSTreeManager::GetInstance(true)->pModelMap->RemoveAll();
            NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* pModelMap = BSTreeManager::GetInstance(true)
                                                                                    ->pModelMap;
            if (pModelMap) {
                delete pModelMap;
            }
            BSTreeManager::GetInstance(true)->pModelMap = nullptr;
        }
        BSTreeManager::kModelCriticalSection.Leave();
    }
}

// Tests the flag's whole byte, so the byte is read as an integer.
bool BSTreeManager::GetTreesEnabled() {
    return SpeedTree::bEnableTrees::Get() && reinterpret_cast<const uint8_t&>(GetInstance(true)->bShowTrees) != 0;
}

// Tests the flag's whole byte, so the byte is read as an integer.
bool BSTreeManager::AreLeavesVisible() {
    return GetTreesEnabled() && reinterpret_cast<const uint8_t&>(GetInstance(true)->bShowLeaves) != 0;
}

// Keeps the base distance as a double and rounds the product to a float.
float BSTreeManager::GetNearDistance() {
    double dBase = GameSetting::fTreeNearDistanceBase::Get();
    return static_cast<float>(gs_fLODMultTrees_LOD.Float() * dBase);
}

// Keeps the base distance as a double and rounds the product to a float.
float BSTreeManager::GetFarDistance() {
    double dBase = GameSetting::fTreeFarDistanceBase::Get();
    return static_cast<float>(gs_fLODMultTrees_LOD.Float() * dBase);
}

NiTexture* BSTreeManager::GetCanopyShadow() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp",
        629
    );
    if (!BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject) {
        if (TES::pInstance) {
            BSTreeManager* pManager = BSTreeManager::GetInstance(true);
            TES::pInstance
                ->CreateTextureImage("Data\\Textures\\Trees\\CanopyShadow.dds", pManager->spCanopyShadow, true, false);
        }
        else {
            BSTreeManager::GetInstance(true)->spCanopyShadow = static_cast<NiSourceTexture*>(nullptr);
        }
        BSShaderManager::SetProjectedShadowTexture(BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject);
    }
    if (!BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject) {
        NiPixelData* pPixelData = new NiPixelData(16u, 16u, NiPixelFormat::RGB24, 1u, 1u);
        uint8_t* pPixels = pPixelData->GetPixels(0u, 0u);
        for (uint32_t iy = 0; iy < 16; ++iy) {
            for (uint32_t ix = 0; ix < 16; ++ix) {
                pPixels[48 * iy + 3 * ix] = -1;
                pPixels[48 * iy + 1 + 3 * ix] = -1;
                pPixels[48 * iy + 2 + 3 * ix] = -1;
            }
        }
        pPixelData->MarkAsChanged();
        NiFixedString kName("CanopyShadowTexture");
        const char* pHandleString = kName.GetHandleString();
        NiSourceTexture* pTexture = NiSourceTexture::Create(
            pPixelData,
            pHandleString,
            NiSourceTexture::ms_kDefaultPrefs
        );
        BSTreeManager::GetInstance(true)->spCanopyShadow = pTexture;
        BSShaderManager::SetProjectedShadowTexture(BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject);
    }
    return BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject;
}

void BSTreeManager::Update(NiCamera* apCamera, bool abMenuMode) {
    static float fTreeTime;
    if (!abMenuMode) {
        fTreeTime = appTimer.fDelta + fTreeTime;
        CSpeedTreeRT::SetTime(fTreeTime);
    }
    if (apCamera) {
        float fCamDir[3];
        float fCamLoc[3];
        fCamLoc[0] = apCamera->GetWorldTranslate().x;
        fCamLoc[1] = apCamera->GetWorldTranslate().y;
        fCamLoc[2] = apCamera->GetWorldTranslate().z;
        NiPoint3 kBackDir = -1.0f * apCamera->GetWorldDirection();
        fCamDir[0] = kBackDir.x;
        fCamDir[1] = kBackDir.y;
        fCamDir[2] = kBackDir.z;
        CSpeedTreeRT::SetCamera(fCamLoc, fCamDir);
    }
    if (!abMenuMode) {
        if (TES::pInstance && TES::pInstance->pSky) {
            if (TES::pInstance->pInteriorCell) {
                BSTreeManager::GetInstance(true)->fWindSpeed = 0.0f;
            }
            else {
                float fSkyWindSpeed = TES::pInstance->pSky->fWindSpeed;
                BSTreeManager::GetInstance(true)->fWindSpeed = fSkyWindSpeed;
            }
            if (BSTreeManager::GetInstance(true)->spBillboardMatProp.m_pObject && TES::pInstance->pSky->GetFog()) {
                NiColor kFogColor = TES::pInstance->pSky->GetFog()->m_kColor;
                BSTreeManager::GetInstance(true)->spBillboardMatProp->SetEmittance(kFogColor);
            }
        }

        BSTreeManager::GetInstance(true)->UpdateWindMatrices();
    }
    // SetProjectedShadowScale takes an unsigned int.
    ShadowLightShader::SetProjectedShadowScale(static_cast<uint32_t>(gs_iCanopyShadowScale_SpeedTree.Int()));
    SpeedTree::fCanopyShadowGrassMult::Set(gs_fCanopyShadowGrassMult_SpeedTree.Float());
}

void BSTreeManager::SetForceFullLOD(bool abForce) {
    BSTreeManager::GetInstance(true)->m_bForceFullLOD = abForce;
}

bool BSTreeManager::GetForceFullLOD() {
    return BSTreeManager::GetInstance(true)->m_bForceFullLOD;
}

void BSTreeManager::SetTreesEnabled(bool abEnabled) {
    BSTreeManager::GetInstance(true)->bShowTrees = abEnabled;
}

BSXFlags* BSTreeManager::GetHavokFlags() {
    return BSTreeManager::GetInstance(true)->spHavokFlags;
}

NiAlphaProperty* BSTreeManager::GetDistantBillboardAlphaProperty() {
    return BSTreeManager::GetInstance(true)->spDistantAlphaProp;
}

void BSTreeManager::SetLeavesVisible(bool abShowLeaves) {
    BSTreeManager::GetInstance(true)->bShowLeaves = abShowLeaves;
}

void BSTreeManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BSTreeManager::Destroy() {
    if (pTreeManager) {
        delete pTreeManager;
        pTreeManager = nullptr;
    }
}

// NaN is sent to 0.
void SpeedTree::fCanopyShadowGrassMult::Set(float afValue) {
    float fValue;
    if (!(afValue >= 0.0)) {
        fValue = 0.0f;
    }
    else if (afValue > 1.0) {
        fValue = 1.0f;
    }
    else {
        fValue = afValue;
    }
    TallGrassShader::fProjShadowMult = fValue;
}

float GameSetting::fTreeNearDistanceBase::Get() {
    return gs_fTreeNearDistanceBase.Float();
}

float GameSetting::fTreeFarDistanceBase::Get() {
    return gs_fTreeFarDistanceBase.Float();
}

float GameSetting::fTreeSizeConversion::Get() {
    return gs_fTreeSizeConversion.Float();
}

void BSShaderManager::SetProjectedShadowTexture(NiTexture* apTexture) {
    pProjectedShadowTexture = apTexture;
}
