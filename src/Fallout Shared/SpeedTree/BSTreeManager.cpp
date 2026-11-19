#include "Fallout Shared/SpeedTree/BSTreeManager.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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

// Releases the named texture, if it is not still in use.
bool TES_FreeTexture(TES* apTES, const char* apPath, uint32_t aiUser) {
    NiPointer<NiTexture> spTexture(nullptr);
    BSTexturePalette::GetTexture(apPath, spTexture);
    if (spTexture.m_pObject && spTexture.m_pObject->m_uiRefCount <= aiUser + 2) {
        BSTexturePalette::RemoveTexture(spTexture.m_pObject);
        return true;
    }

    return false;
}

char sub_BB2FC0(uint32_t a1, float Yaw, float Pitch) {
    D3DXMATRIX pOut;
    if (a1 >= 4) {
        return 0;
    }
    pOut._43 = 0.0f;
    pOut._42 = 0.0f;
    pOut._41 = 0.0f;
    pOut._34 = 0.0f;
    pOut._32 = 0.0f;
    pOut._31 = 0.0f;
    pOut._24 = 0.0f;
    pOut._23 = 0.0f;
    pOut._21 = 0.0f;
    pOut._14 = 0.0f;
    pOut._13 = 0.0f;
    pOut._12 = 0.0f;
    pOut._44 = 1.0f;
    pOut._33 = 1.0f;
    pOut._22 = 1.0f;
    pOut._11 = 1.0f;
    D3DXMatrixRotationYawPitchRoll_Thunk(&pOut, Yaw, Pitch, 0.0f);
    D3DXMatrixTranspose_Thunk(&pOut, &pOut);
    SpeedTreeShader_pWindMatrices[a1] = pOut;
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
    ClearCanopyShadow_004F3310();
    delete pBackgroundLoadedTreeMap;
}

void BSTreeManager::RemoveBackgroundLoadedTree(TESObjectREFR* apRef) {
    BSTreeNode* a1 = nullptr;
    if (pBackgroundLoadedTreeMap->GetAt(apRef, a1)) {
        pBackgroundLoadedTreeMap->RemoveAt(apRef);
        if (a1) {
            NiObjectGarbage::Add(a1);
        }
    }
}

BSTreeNode* BSTreeManager::CreateTreeForReference(TESObjectREFR* apTreeRefr, TESObjectTREE* apTreeObj) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp", 462
    );
    NiPointer<BSTreeModel> spbasemodel(static_cast<BSTreeModel*>(nullptr));
    BSTreeModel* pmodel = nullptr;
    if (!SpeedTree::bEnableTrees::Get() ||
        (!apTreeObj ||
         (!apTreeObj->GetModel() ||
          (!apTreeObj->GetModelLength() || (apTreeRefr && apTreeRefr->GetObjectReference() != apTreeObj))))) {
        return nullptr;
    }
    BSTreeNode* ptreeNode = nullptr;
    if (pBackgroundLoadedTreeMap->GetAt(apTreeRefr, ptreeNode)) {
        pBackgroundLoadedTreeMap->RemoveAt(apTreeRefr);
        BSTreeNode* v15 = ptreeNode;
        return v15;
    }
    uint32_t iseed = 1;
    if (apTreeRefr) {
        iseed = apTreeRefr->GetSeed();
    }
    BSTreeManager* Singleton = BSTreeManager::GetInstance(true);
    Singleton->GetModel(apTreeObj, iseed, spbasemodel);
    if (!spbasemodel.m_pObject) {
        goto LABEL_23;
    }
    if (spbasemodel.m_pObject->m_uiRefCount <= 2) {
        if (!spbasemodel.m_pObject->InitFromBase(apTreeObj, iseed)) {
        LABEL_23:;
            return nullptr;
        }
        pmodel = spbasemodel.m_pObject;
    }
    else {
        BSTreeModel* pmodel_2;
        BSTreeModel* v13 = static_cast<BSTreeModel*>(NiMemObject::operator new(sizeof(BSTreeModel)));
        if (v13) {
            pmodel_2 = ::new (v13) BSTreeModel;
        }
        else {
            pmodel_2 = nullptr;
        }
        pmodel = pmodel_2;
        if (!pmodel_2->InitAsInstance(spbasemodel.m_pObject)) {
            if (pmodel) {
                delete pmodel;
            }
            goto LABEL_23;
        }
    }
    float fscale = 1.0f;
    if (apTreeRefr) {
        fscale = apTreeRefr->GetScale();
    }
    BSTreeNode* pretnode = pmodel->CreateArt(fscale);
    if (pretnode) {
        const char* v5 = apTreeObj->GetModel();
        NiFixedString apString(v5);
        NiFixedString* a2 = &apString;
        pretnode->SetName(*a2);
    }
    pmodel->SetInitialized(true);
    return pretnode;
}

void BSTreeManager::AddBackgroundLoadedTree(TESObjectREFR* apRef, BSTreeNode* apTreeNode) {
    if ((!pBackgroundLoadedTreeMap->SetAt(apRef, apTreeNode, false)) && (apTreeNode)) {
        delete apTreeNode;
    }
}

NiPointer<BSTreeModel>* BSTreeManager::GetModelArray(TESObjectTREE* apTree) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp", 216
    );
    NiPointer<BSTreeModel>* pretval = nullptr;
    if (apTree) {
        if (!pModelMap) {
            NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* v4;
            NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* v9 =
                static_cast<NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>*>(
                    ourMalloc(sizeof(NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>))
                );
            if (v9) {
                v4 = ::new (v9) NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>(37u);
            }
            else {
                v4 = nullptr;
            }
            pModelMap = v4;
        }
        if (pModelMap->GetAt(apTree, pretval)) {
            NiPointer<BSTreeModel>* v8 = pretval;
            return v8;
        }

        NiPointer<BSTreeModel>* pretval_2 = new NiPointer<BSTreeModel>[1];
        pretval = pretval_2;
        pModelMap->SetAt(apTree, pretval_2);
        NiPointer<BSTreeModel>* v6 = pretval;
        return v6;
    }

    return nullptr;
}

void BSTreeManager::GetModel(TESObjectTREE* apTree, uint32_t aiSeed, NiPointer<BSTreeModel>& aspBSTreeModel) {
    BSTreeManager::kModelCriticalSection.Enter(nullptr);
    NiPointer<BSTreeModel> sprettree(static_cast<BSTreeModel*>(nullptr));
    if (apTree) {
        NiPointer<BSTreeModel>* pmodelarray = GetModelArray(apTree);
        int32_t iemptyslot = -1;
        bool bfound = false;
        for (int32_t i = 0; i < 1; ++i) {
            BSTreeModel* ptree = pmodelarray[i].m_pObject;
            if (ptree) {
                if (ptree->iSeed == aiSeed) {
                    sprettree = ptree;
                    bfound = true;
                    break;
                }
            }
            else {
                if (iemptyslot == -1) {
                    iemptyslot = i;
                }
            }
        }
        if (!bfound || !sprettree.m_pObject) {
            if (iemptyslot >= 0) {
                BSTreeModel* v4;
                BSTreeModel* apMemory = static_cast<BSTreeModel*>(NiMemObject::operator new(sizeof(BSTreeModel)));
                if (apMemory) {
                    v4 = ::new (apMemory) BSTreeModel;
                }
                else {
                    v4 = nullptr;
                }
                sprettree = v4;
                pmodelarray[iemptyslot] = sprettree;
            }
            else {
                sprettree = *pmodelarray;
            }
        }
    }
    BSTreeManager::kModelCriticalSection.Leave();
    aspBSTreeModel = sprettree;
}

void BSTreeManager::UpdateWindMatrices() {
    static LeafSwayBaseTable ffrequencies{0.15f, 0.17f, 0.25f, 0.15f, 0.19f, 0.05f, 0.15f, 0.22f};
    static LeafSwayPhaseTable fmatrixTimes{};
    static float foldStrength = fWindSpeed;
    float fbaseAngle = fWindSpeed * 0.6100000143051147;
    float v19 = 0.0f;
    for (int32_t i = 0; i < 4; ++i) {
        fmatrixTimes[i] = appTimer.fDelta + fmatrixTimes[i];
        if (fWindSpeed != 0.0) {
            fmatrixTimes[i] = fmatrixTimes[i] * foldStrength / fWindSpeed;
        }
        float fbaseFreq = fWindSpeed * 20.0;
        float v14 = fbaseFreq * ffrequencies[2 * i] * fmatrixTimes[i];
        float fxPercent = NiSinQ(v14);
        float v13 = fbaseFreq * ffrequencies[2 * i + 1] * fmatrixTimes[i];
        float fyPercent = NiCosQ(v13);
        if (i == 2) {
            float v20 = std::fabs(fxPercent) + v19;
            v19 = std::fabs(fyPercent) + v20;
        }
        float Pitch = fbaseAngle * fyPercent;
        float Yaw = fbaseAngle * fxPercent;
        sub_BB2FC0(i, Yaw, Pitch);
    }
    double fLeafRockAmountSwayInfluence = 1.0 - gs_fLeafRockAmountSwayInfluence.Float();
    float frockAmountSway = gs_fLeafRockAmountSwayInfluence.Float() * v19 / 2.0 + fLeafRockAmountSwayInfluence;
    double fLeafRustleAmountSwayInfluence = 1.0 - gs_fLeafRustleAmountSwayInfluence.Float();
    float frustleAmountSway = gs_fLeafRustleAmountSwayInfluence.Float() * v19 / 2.0 + fLeafRustleAmountSwayInfluence;
    double v8 = 1.0 - gs_fLeafRockSpeedSwayInfluence.Float();
    float frockSpeedSway = gs_fLeafRockSpeedSwayInfluence.Float() * v19 / 2.0 + v8;
    double v7 = 1.0 - gs_fLeafRustleSpeedSwayInfluence.Float();
    float frustleSpeedSway = gs_fLeafRustleSpeedSwayInfluence.Float() * v19 / 2.0 + v7;
    double Seconds = appTimer.fDelta;
    float v5 = gs_fLeafRustleTimeScale.Float() * Seconds * frustleSpeedSway;
    double secondsPassed = appTimer.fDelta;
    float rockTimeScale = BSTreeManager::fLeafRockTimeScale.Float() * secondsPassed * frockSpeedSway;
    float a2 = fWindSpeed * frustleAmountSway;
    float v1 = fWindSpeed * frockAmountSway;
    SpeedTreeLeafShader::UpdateWindTimers(v1, a2, rockTimeScale, v5);
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
    BSTreeManager* v1;
    AutoMemContext apMemory(
        MEM_CONTEXT_TREESYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp", 177
    );
    if (pTreeManager) {
        if (!abRecreateIfFound) {
            return;
        }
        BSTreeManager::Destroy();
    }
    BSTreeManager* v2 = static_cast<BSTreeManager*>(ourMalloc(sizeof(BSTreeManager)));
    if (v2) {
        v1 = ::new (v2) BSTreeManager;
    }
    else {
        v1 = nullptr;
    }
    pTreeManager = v1;
}

void BSTreeManager::ClearCanopyShadow() {
    // GetInstance is called again for the release and the assignment goes through that result, as the game does.
    if (GetInstance(true)->spCanopyShadow.m_pObject != nullptr) {
        GetInstance(true)->spCanopyShadow = nullptr;
        BSShaderManager::SetProjectedShadowTexture(nullptr);
        TES_FreeTexture(TES::pInstance, "Data\\Textures\\Trees\\CanopyShadow.dds", 0);
    }
}

// Calls GetInstance again before each use of the model map, as the game does.
void BSTreeManager::ClearModelCache(bool abClearManagerOnlyHeldOnly) {
    if (BSTreeManager::GetInstance(true)->pModelMap) {
        BSTreeManager::kModelCriticalSection.Enter(nullptr);
        if (abClearManagerOnlyHeldOnly) {
            NiTMapItem<TESObjectTREE*, NiPointer<BSTreeModel>*>* pPos =
                BSTreeManager::GetInstance(true)->pModelMap->GetFirstPos();
            while (pPos) {
                TESObjectTREE* pKey;
                NiPointer<BSTreeModel>* pModelArray;
                BSTreeManager::GetInstance(true)->pModelMap->GetNext(pPos, pKey, pModelArray);
                if (pModelArray) {
                    // Only models no one but the manager holds are released.
                    bool bOnlyHeldHere = true;
                    for (int32_t i = 0; i < 1; ++i) {
                        if (pModelArray[i].m_pObject) {
                            if (pModelArray[i].m_pObject->m_uiRefCount > 1) {
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
                NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* pModelMap =
                    BSTreeManager::GetInstance(true)->pModelMap;
                if (pModelMap) {
                    delete pModelMap;
                }
                BSTreeManager::GetInstance(true)->pModelMap = nullptr;
            }
        }
        else {
            NiTMapItem<TESObjectTREE*, NiPointer<BSTreeModel>*>* pPos =
                BSTreeManager::GetInstance(true)->pModelMap->GetFirstPos();
            while (pPos) {
                TESObjectTREE* pKey;
                NiPointer<BSTreeModel>* pModelArray;
                BSTreeManager::GetInstance(true)->pModelMap->GetNext(pPos, pKey, pModelArray);
                if (pModelArray) {
                    delete[] pModelArray;
                }
            }
            BSTreeManager::GetInstance(true)->pModelMap->RemoveAll();
            NiTPointerMap<TESObjectTREE*, NiPointer<BSTreeModel>*>* pModelMap =
                BSTreeManager::GetInstance(true)->pModelMap;
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
    double fBase = GameSetting::fTreeNearDistanceBase::Get();
    return (float)(gs_fLODMultTrees_LOD.Float() * fBase);
}

// Keeps the base distance as a double and rounds the product to a float.
float BSTreeManager::GetFarDistance() {
    double fBase = GameSetting::fTreeFarDistanceBase::Get();
    return (float)(gs_fLODMultTrees_LOD.Float() * fBase);
}

NiTexture* BSTreeManager::GetCanopyShadow() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeManager.cpp", 629
    );
    if (!BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject) {
        if (TES::pInstance) {
            BSTreeManager* Singleton = BSTreeManager::GetInstance(true);
            TES::pInstance->CreateTextureImage(
                "Data\\Textures\\Trees\\CanopyShadow.dds", Singleton->spCanopyShadow, true, false
            );
        }
        else {
            BSTreeManager::GetInstance(true)->spCanopyShadow = static_cast<NiSourceTexture*>(nullptr);
        }
        BSTreeManager* v1 = BSTreeManager::GetInstance(true);
        BSShaderManager::SetProjectedShadowTexture(v1->spCanopyShadow.m_pObject);
    }
    if (!BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject) {
        NiPixelData* ppixeldata;
        NiPixelData* v10 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
        if (v10) {
            ppixeldata = ::new (v10) NiPixelData(16u, 16u, NiPixelFormat_RGB24, 1u, 1u);
        }
        else {
            ppixeldata = nullptr;
        }
        uint8_t* ppixels = ppixeldata->GetPixels(0u, 0u);
        for (uint32_t iy = 0; iy < 16; ++iy) {
            for (uint32_t ix = 0; ix < 16; ++ix) {
                ppixels[48 * iy + 3 * ix] = -1;
                ppixels[48 * iy + 1 + 3 * ix] = -1;
                ppixels[48 * iy + 2 + 3 * ix] = -1;
            }
        }
        ppixeldata->MarkAsChanged();
        NiFixedString apString("CanopyShadowTexture");
        const char* HandleString = apString.GetHandleString();
        NiSourceTexture* v5 = NiSourceTexture::Create(ppixeldata, HandleString, NiSourceTexture::ms_kDefaultPrefs);
        BSTreeManager::GetInstance(true)->spCanopyShadow = v5;
        BSTreeManager* v3 = BSTreeManager::GetInstance(true);
        BSShaderManager::SetProjectedShadowTexture(v3->spCanopyShadow.m_pObject);
    }
    NiTexture* v8 = BSTreeManager::GetInstance(true)->spCanopyShadow.m_pObject;
    return v8;
}

void BSTreeManager::Update(NiCamera* apCamera, bool abMenuMode) {
    static float fTreeTime;
    if (!abMenuMode) {
        fTreeTime = appTimer.fDelta + fTreeTime;
        CSpeedTreeRT::SetTime(fTreeTime);
    }
    if (apCamera) {
        float fcamDir[3];
        float fcamLoc[3];
        fcamLoc[0] = (&apCamera->GetWorldTranslate())->x;
        fcamLoc[1] = (&apCamera->GetWorldTranslate())->y;
        fcamLoc[2] = (&apCamera->GetWorldTranslate())->z;
        NiPoint3 pt3temp = -1.0f * apCamera->GetWorldDirection();
        fcamDir[0] = pt3temp.x;
        fcamDir[1] = pt3temp.y;
        fcamDir[2] = pt3temp.z;
        CSpeedTreeRT::SetCamera(fcamLoc, fcamDir);
    }
    if (!abMenuMode) {
        if ((TES::pInstance) && (TES::pInstance->pSky)) {
            if (TES::pInstance->pInteriorCell) {
                BSTreeManager::GetInstance(true)->fWindSpeed = 0.0f;
            }
            else {
                float windSpeed = TES::pInstance->pSky->fWindSpeed;
                BSTreeManager::GetInstance(true)->fWindSpeed = windSpeed;
            }
            if ((BSTreeManager::GetInstance(true)->spBillboardMatProp.m_pObject) && (TES::pInstance->pSky->GetFog())) {
                NiColor col = *&TES::pInstance->pSky->GetFog()->m_kColor;
                BSTreeManager* Singleton = BSTreeManager::GetInstance(true);
                BSTreeManager* v4 = BSTreeManager::GetInstance(true);
                BSTreeManager* v5 = BSTreeManager::GetInstance(true);
                v5->spBillboardMatProp.m_pObject->SetEmittance(col);
            }
        }

        BSTreeManager* v6 = BSTreeManager::GetInstance(true);
        v6->UpdateWindMatrices();
    }
    // SetProjectedShadowScale takes an unsigned int.
    ShadowLightShader::SetProjectedShadowScale(static_cast<uint32_t>(gs_iCanopyShadowScale_SpeedTree.Int()));
    float* v7 = &gs_fCanopyShadowGrassMult_SpeedTree.Float();
    SpeedTree::fCanopyShadowGrassMult::Set(*v7);
}

void BSTreeManager::SetForceFullLOD(bool abForce) {
    BSTreeManager::GetInstance(true)->m_bForceFullLOD = (bool)((uint8_t)abForce);
}

bool BSTreeManager::GetForceFullLOD() {
    return (bool)((uint8_t)BSTreeManager::GetInstance(true)->m_bForceFullLOD);
}

void BSTreeManager::SetTreesEnabled(bool a1) {
    BSTreeManager::GetInstance(true)->bShowTrees = (bool)((uint8_t)a1);
}

BSXFlags* BSTreeManager::GetHavokFlags() {
    BSTreeManager* singleton = BSTreeManager::GetInstance(true);
    return singleton->spHavokFlags;
}

NiAlphaProperty* BSTreeManager::GetDistantBillboardAlphaProperty() {
    BSTreeManager* singleton = BSTreeManager::GetInstance(true);
    return singleton->spDistantAlphaProp;
}

void BSTreeManager::SetLeavesVisible(bool abShowLeaves) {
    BSTreeManager::GetInstance(true)->bShowLeaves = (bool)((uint8_t)abShowLeaves);
}

void BSTreeManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BSTreeManager::ClearCanopyShadow_004F3310() {
    TESRegionDataLandscape::spDefaultFullMaskTextureProp = static_cast<NiTexturingProperty*>(nullptr);
    TESRegionDataLandscape::spDefaultEmptyMaskTextureProp = static_cast<NiTexturingProperty*>(nullptr);
}

void BSTreeManager::Destroy() {
    if (pTreeManager) {
        delete pTreeManager;
        pTreeManager = nullptr;
    }
}

// NaN is sent to 0, as the game does.
void SpeedTree::fCanopyShadowGrassMult::Set(float afValue) {
    float fValue;
    if (!(afValue >= 0.0)) {
        fValue = 0.0f;
    }
    else {
        float fClamped;
        if (afValue > 1.0) {
            fClamped = 1.0f;
        }
        else {
            fClamped = afValue;
        }
        fValue = fClamped;
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
