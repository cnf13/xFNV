#include "Fallout Shared/SpeedTree/BSTreeModel.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeBillboardShaderProperty.h"
#include "BSShader/SpeedTreeBranchShaderProperty.h"
#include "BSShader/SpeedTreeLeafShaderProperty.h"
#include "BSShader/SpeedTreeShaderProperty.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/WindEngine.h"

#include <bit>
#include <cmath>
#include <locale>

BSCriticalSection BSTreeModel::kCloneLock;

BSTreeModel::BSTreeModel()
  : eType(BTMT_NONE), pSpeedTree(nullptr), spTreeParent(nullptr), pspBranchData(nullptr), pspLeafData(nullptr),
    spBillboard(nullptr), spLeafShaderData(nullptr), pspBranchShaderProperties(nullptr),
    pspLeafShaderProperties(nullptr), spBranchTextureSet(nullptr), spLeafFrondTexture(nullptr),
    spBillboardTextureSet(nullptr), spRigidBody(nullptr), fCurveScalar(0.0f), iSeed(1), bInitialized(false),
    fTrunkHeight(0.0f), fTrunkWidth(0.0f) {}

BSTreeModel::~BSTreeModel() {
    BSTreeModel::ClearModel();
    delete pSpeedTree;
    pSpeedTree = nullptr;
}

void BSTreeModel::Update(BSTreeNode* apNode) {
    uint32_t uiEntries;
    CSpeedTreeRT::SGeometry geom;
    uiEntries = 0;
    if (apNode && apNode->GetModel() && pSpeedTree && BSTreeManager::GetTreesEnabled()) {
        float fLOD;
        NiPoint3 kPosition = apNode->GetWorldTranslate();
        pSpeedTree->SetTreePosition(kPosition.x, kPosition.y, kPosition.z);
        if (BSTreeManager::GetForceFullLOD()) {
            pSpeedTree->SetLodLevel(1.0f);
            fLOD = 1.0f;
        }
        else {
            pSpeedTree->ComputeLodLevel();
            fLOD = pSpeedTree->GetLodLevel();
        }
        pSpeedTree->GetBSGeometryLODData(geom, fLOD);
        bool bDrawNode = false;
        if (geom.m_sBillboard0.m_bIsActive) {
            bDrawNode = true;
            apNode->SetVisibilityOnBillboard(static_cast<int32_t>(geom.m_sBillboard0.m_fAlphaTestValue));
        }
        apNode->SetVisibilityForBillboards(bDrawNode);
        if (!(fLOD <= 0.0)) {
            int16_t sLevel;
            bDrawNode = false;
            int16_t sLevels = GetNumBranchLODLevels();
            if (sLevels > 0) {
                float fLodLevel = pSpeedTree->GetLodLevel();
                int16_t sLevelVisible = pSpeedTree->GetDiscreteBranchLodLevel(fLodLevel);
                for (sLevel = 0; sLevel < sLevels; ++sLevel) {
                    if (sLevelVisible == sLevel) {
                        bDrawNode = true;
                        apNode->SetVisibilityOnBranch(sLevel, static_cast<int32_t>(geom.m_fBranchAlphaTestValue));
                    }
                    else {
                        apNode->SetVisibilityOnBranch(sLevel, 255);
                    }
                }
            }
            apNode->SetVisibilityForBranches(bDrawNode);
            if (BSTreeManager::AreLeavesVisible()) {
                bDrawNode = false;
                sLevels = GetNumLeafLODLevels();
                if (sLevels > 0) {
                    if (geom.m_sLeaves0.m_bIsActive || geom.m_sLeaves1.m_bIsActive) {
                        for (sLevel = 0; sLevel < sLevels; ++sLevel) {
                            if (geom.m_sLeaves0.m_bIsActive && geom.m_sLeaves0.m_nDiscreteLodLevel == sLevel) {
                                bDrawNode = true;
                                apNode->SetVisibilityOnLeaf(
                                    sLevel,
                                    static_cast<int32_t>(geom.m_sLeaves0.m_fAlphaTestValue)
                                );
                            }
                            else if (geom.m_sLeaves1.m_bIsActive && geom.m_sLeaves1.m_nDiscreteLodLevel == sLevel) {
                                bDrawNode = true;
                                apNode->SetVisibilityOnLeaf(
                                    sLevel,
                                    static_cast<int32_t>(geom.m_sLeaves1.m_fAlphaTestValue)
                                );
                            }
                            else {
                                apNode->SetVisibilityOnLeaf(sLevel, 255);
                            }
                        }
                    }
                    if (spLeafShaderData.m_pObject) {
                        const float* pMaps;
                        if (pSpeedTree->InstanceOf()) {
                            pMaps = pSpeedTree->InstanceOf()->GetLeafBillboardTable(uiEntries);
                        }
                        else {
                            pMaps = pSpeedTree->GetLeafBillboardTable(uiEntries);
                        }
                        spLeafShaderData->SetLeafMaps(pMaps, uiEntries);
                    }
                }
                apNode->SetVisibilityForLeaves(bDrawNode);
                geom.Destroy();
            }
            else {
                apNode->SetVisibilityForLeaves(false);
                geom.Destroy();
            }
        }
        else {
            apNode->SetVisibilityForBranches(false);
            apNode->SetVisibilityForLeaves(false);
            geom.Destroy();
        }
    }
    else {
        geom.Destroy();
    }
}

BSTreeNode* BSTreeModel::CreateArt(float afScale) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREEART,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp",
        901
    );
    NiPointer<bhkRigidBody> spRigid(static_cast<bhkRigidBody*>(nullptr));
    if (pSpeedTree) {
        BSTreeNode* pRetVal;
        uint16_t sLevels;
        uint16_t sLevel;
        NiD3DShader* pShader;
        NiTriBasedGeom* pGeom;
        NiAlphaProperty* pPropAlpha;
        if (!(fTrunkHeight > 64.0)) {
            spRigid = static_cast<bhkRigidBody*>(nullptr);
        }
        else if (afScale == 1.0) {
            spRigid = GetRigidBody();
        }
        else {
            float fRadius = afScale * fTrunkWidth / 2.0;
            float fHeight = afScale * fTrunkHeight;
            spRigid = bhkUtilFunctions::CreateCapsuleBody(fHeight, fRadius);
        }
        pRetVal = new BSTreeNode(this, spRigid.m_pObject);
        if (pspBranchData && pspBranchShaderProperties) {
            sLevels = GetNumBranchLODLevels();
            pShader = BSShaderManager::GetShader(4);
            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                if (pspBranchData[sLevel].m_pObject) {
                    pGeom = new NiTriStrips(pspBranchData[sLevel].m_pObject);
                    pPropAlpha = BSTreeModel::CreateAlphaProperty();
                    if (pPropAlpha) {
                        pPropAlpha->SetAlphaTesting(false);
                        pGeom->AttachProperty(pPropAlpha);
                    }
                    if (pspBranchShaderProperties[sLevel].m_pObject) {
                        pspBranchShaderProperties[sLevel]->FreeRenderPasses();
                        SpeedTreeBranchShaderProperty* pProperty = pspBranchShaderProperties[sLevel].m_pObject;
                        pGeom->AttachProperty(pProperty);
                        if (!pGeom->GetModelData()->GetAdditionalGeometryData()) {
                            NiAdditionalGeometryData* pData = pspBranchShaderProperties[sLevel]
                                                                  ->CreateRendererSpecificProperty(pGeom);
                            pGeom->SetAdditionalGeometryData(pData);
                        }
                    }
                    if (IsInstance()) {
                        pGeom->SetShader(static_cast<BSShader*>(pShader));
                    }
                    pRetVal->AddBranchLevel(sLevel, pGeom);
                }
            }
            if (!IsInstance()) {
                for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                    pGeom = pRetVal->GetBranchLevel(sLevel);
                    if (pGeom) {
                        BSShaderTextureSet* pTextureSet = spBranchTextureSet.m_pObject;
                        pspBranchShaderProperties[sLevel]->LoadTextureSet("STBranch", pTextureSet);
                        BSShaderManager::PrepareObject(pGeom, 0, 0);
                        NiAdditionalGeometryData* pData = pspBranchShaderProperties[sLevel]
                                                              ->CreateRendererSpecificProperty(pGeom);
                        pGeom->SetAdditionalGeometryData(pData);
                        if (!pGeom->GetModelData()->m_pkBuffData) {
                            NiRenderer::GetRenderer()->PrecacheGeometry(pGeom, 0, 0, nullptr);
                        }
                    }
                }
            }
        }

        if (pspLeafData && pspLeafShaderProperties) {
            sLevels = GetNumLeafLODLevels();
            pShader = BSShaderManager::GetShader(6);
            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                if (pspLeafData[sLevel].m_pObject) {
                    pGeom = new NiTriShape(pspLeafData[sLevel].m_pObject);
                    pPropAlpha = BSTreeModel::CreateAlphaProperty();
                    if (pPropAlpha) {
                        pGeom->AttachProperty(pPropAlpha);
                    }
                    if (pspLeafShaderProperties[sLevel].m_pObject) {
                        pspLeafShaderProperties[sLevel]->FreeRenderPasses();
                        SpeedTreeLeafShaderProperty* pProperty = pspLeafShaderProperties[sLevel].m_pObject;
                        pGeom->AttachProperty(pProperty);
                    }
                    pRetVal->AddLeafLevel(sLevel, pGeom);
                    pGeom->SetShader(static_cast<BSShader*>(pShader));
                    NiRenderer::GetRenderer()->PrecacheGeometry(pGeom, 0, 0, nullptr);
                }
            }
        }

        if (spBillboard.m_pObject) {
            pRetVal->SetBillboard(spBillboard.m_pObject);
        }
        pRetVal->UpdateProperties();
        NiUpdateData UpdateData(0.0f, false, false);
        pRetVal->UpdateSelected(UpdateData);
        pRetVal->MakeHavokLeaves(pSpeedTree, afScale);
        return pRetVal;
    }

    return nullptr;
}

void BSTreeModel::ApplyBaseObject(TESObjectTREE* apTreeObj) {
    NiPointer<NiSourceTexture> spTex(nullptr);
    if (apTreeObj && pSpeedTree && !IsInstance()) {
        float fTreeForceBranchDimming = SpeedTree::fTreeForceBranchDimming::Get();
        if (fTreeForceBranchDimming < 0.0 || fTreeForceBranchDimming > 1.0) {
            fTreeForceBranchDimming = apTreeObj->GetBranchDimming();
        }
        if (fTreeForceBranchDimming >= 0.0 && fTreeForceBranchDimming <= 1.0) {
            pSpeedTree->SetBranchDimming(fTreeForceBranchDimming);
        }
        fTreeForceBranchDimming = SpeedTree::fTreeForceLeafDimming::Get();
        if (fTreeForceBranchDimming < 0.0 || fTreeForceBranchDimming > 1.0) {
            fTreeForceBranchDimming = apTreeObj->GetLeafDimming();
        }
        if (fTreeForceBranchDimming >= 0.0 && fTreeForceBranchDimming <= 1.0) {
            pSpeedTree->SetLeafDimming(fTreeForceBranchDimming);
        }
        float fVal = SpeedTree::fTreeForceCS::Get();
        if (fVal < 0.0) {
            fVal = apTreeObj->GetCurveScalar();
        }
        if (fVal >= 0.0) {
            fCurveScalar = fVal;
        }
        float fMin = SpeedTree::fTreeForceMinBudAngle::Get();
        float fMax = SpeedTree::fTreeForceMaxBudAngle::Get();
        if (fMin < 0.0 || fMin > 90.0 || fMax < 0.0 || fMax > 90.0 || fMax < static_cast<double>(fMin)) {
            fMin = apTreeObj->GetMinimumLeafAngle();
            fMax = apTreeObj->GetMaximumLeafAngle();
        }
        if (fMin >= 0.0 && fMin <= 90.0 && fMax >= 0.0 && fMax <= 90.0 && fMax >= static_cast<double>(fMin)) {
            pSpeedTree->SetBudAngleMin(fMin);
            pSpeedTree->SetBudAngleMax(fMax);
        }
        AutoMemContext tempMemContext(
            MEM_CONTEXT_TREEART,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp",
            145
        );
        if (!spBranchTextureSet.m_pObject) {
            char acDiffuse[264];
            char acNormal[268];
            CSpeedTreeRT::STextures stex;
            pSpeedTree->GetTextures(stex);
            BSstrcpy(acDiffuse, 260, "Textures\\Trees\\Branches\\");
            BSstrcat(acDiffuse, 260, stex.m_pBranchTextureFilename);
            uint32_t uiLength = BSstrlen(acDiffuse);
            acDiffuse[uiLength - 3] = 100;
            acDiffuse[uiLength - 2] = 100;
            acDiffuse[uiLength - 1] = 115;
            BSShaderManager::GetModifiedTextureFilename(acNormal, acDiffuse, "_n", true);
            spBranchTextureSet = new BSShaderTextureSet;
            spBranchTextureSet->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, acDiffuse);
            spBranchTextureSet->SetTextureFilename(BSShaderProperty::TEXTYPE_NORMAL, acNormal);
        }
        if (!spLeafFrondTexture.m_pObject && apTreeObj->GetTextureNameLength()) {
            char acPath[268];
            BSstrcpy(acPath, 260, "Textures\\Trees\\Leaves\\");
            BSstrcat(acPath, 260, apTreeObj->GetTextureName());
            TES::pInstance->CreateTextureImage(acPath, spLeafFrondTexture, true, false);
        }

        if (!spBillboardTextureSet.m_pObject) {
            char acPath[264];
            apTreeObj->GetBillboardTextureFilename(acPath);
            spBillboardTextureSet = new BSShaderTextureSet;
            spBillboardTextureSet->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, acPath);
        }
    }
}

void BSTreeModel::ClearModel() {
    if (pspBranchData) {
        IsInstance();
        if (pspBranchData) {
            delete[] pspBranchData;
        }
        pspBranchData = nullptr;
    }
    if (pspLeafData) {
        delete[] pspLeafData;
        pspLeafData = nullptr;
    }
    spBillboard = static_cast<NiTriShape*>(nullptr);
    spLeafShaderData = static_cast<STLSPData*>(nullptr);
    if (pspBranchShaderProperties) {
        delete[] pspBranchShaderProperties;
        pspBranchShaderProperties = nullptr;
    }
    if (pspLeafShaderProperties) {
        delete[] pspLeafShaderProperties;
        pspLeafShaderProperties = nullptr;
    }
    spBranchTextureSet = static_cast<BSShaderTextureSet*>(nullptr);
    spLeafFrondTexture = static_cast<NiSourceTexture*>(nullptr);
    spBillboardTextureSet = static_cast<BSShaderTextureSet*>(nullptr);
    spRigidBody = static_cast<bhkRigidBody*>(nullptr);
}

void BSTreeModel::CreateGeometry(TESObjectTREE* apTreeObj) {
    if (pSpeedTree && !IsInstance()) {
        char acModel[264];
        char pcSrc[268];
        CreateBranchGeometry();
        CreateLeafGeometry(apTreeObj);
        if (apTreeObj->IsLargeEnoughForDistantLOD()) {
            CreateBillboardGeometry(apTreeObj);
        }
        spRigidBody = static_cast<bhkRigidBody*>(nullptr);
        BSstrcpy(acModel, 260, apTreeObj->GetModel());
        int32_t iLen = BSstrlen(acModel);
        if (iLen > 4) {
            acModel[iLen - 4] = 0;
        }
        BSStream stream;
        BSstrcpy(pcSrc, 260, "Data\\Meshes\\Trees");
        BSstrcat(pcSrc, 260, acModel);
        BSstrcat(pcSrc, 260, ".NIF");
        if (FileFinder::Locate(pcSrc, nullptr, 6u, 1u) &&
            stream.BSStream::Load(pcSrc, static_cast<NiFile*>(nullptr)) &&
            stream.GetObjectCount() == 1) {
            bhkRigidBody* pRigidBody;
            NiObject* pNiObj = stream.GetObjectAt(0);
            NiAVObject* pNiAVObj = static_cast<NiAVObject*>(NiObject::DynamicCastStatic(&NiAVObject::ms_RTTI, pNiObj));
            bhkNiCollisionObject* pCol = bhkNiCollisionObject::GetbhkNiCollisionObject(pNiAVObj);
            if (pCol) {
                pRigidBody = static_cast<bhkRigidBody*>(pCol->GetWorldObject());
            }
            else {
                pRigidBody = nullptr;
            }
            if (pRigidBody) {
                spRigidBody = pRigidBody;
            }
        }

        if (!spRigidBody.m_pObject && fTrunkHeight > 64.0) {
            float fRadius = fTrunkWidth / 2.0;
            spRigidBody = bhkUtilFunctions::CreateCapsuleBody(fTrunkHeight, fRadius);
        }
    }
}

uint16_t BSTreeModel::GetNumBranchLODLevels() {
    if (pSpeedTree) {
        return pSpeedTree->GetNumBranchLodLevels();
    }

    return 0;
}

uint16_t BSTreeModel::GetNumLeafLODLevels() {
    if (pSpeedTree) {
        return pSpeedTree->GetNumLeafLodLevels();
    }

    return 0;
}

void BSTreeModel::SetInitialized(bool abVal) {
    bInitialized = abVal;
}

bhkRigidBody* BSTreeModel::GetRigidBody() const {
    return spRigidBody;
}

void BSTreeModel::CreateBranchGeometry() {
    uint32_t uiUsedVert;
    uint16_t sLevels;
    uint16_t sVerts;
    CSpeedTreeRT::SGeometry geom;
    if (pSpeedTree && !IsInstance()) {
        if (this->pspBranchData) {
            NiPointer<NiTriStripsData>* pspOldBranchData = this->pspBranchData;
            if (pspOldBranchData) {
                delete[] pspOldBranchData;
            }
            this->pspBranchData = nullptr;
        }
        if (this->pspBranchShaderProperties) {
            NiPointer<SpeedTreeBranchShaderProperty>* pspOldProperties = this->pspBranchShaderProperties;
            if (pspOldProperties) {
                delete[] pspOldProperties;
            }
            this->pspBranchShaderProperties = nullptr;
        }
        sLevels = GetNumBranchLODLevels();
        if (sLevels) {
            pSpeedTree->GetGeometry(geom, 1u, -1, -1, -1);
        }
        if (sLevels &&
            geom.m_sBranches.m_pCoords &&
            geom.m_sBranches.m_pNormals &&
            geom.m_sBranches.m_pTexCoords0 &&
            geom.m_sBranches.m_pColors &&
            geom.m_sBranches.m_pWindWeights &&
            geom.m_sBranches.m_pWindMatrixIndices &&
            (sVerts = geom.m_sBranches.m_usVertexCount) != 0) {
            uint16_t sLevel;
            this->pspBranchData = new NiPointer<NiTriStripsData>[sLevels];
            this->pspBranchShaderProperties = new NiPointer<SpeedTreeBranchShaderProperty>[sLevels];
            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                this->pspBranchData[sLevel] = static_cast<NiTriStripsData*>(nullptr);
                this->pspBranchShaderProperties[sLevel] = static_cast<SpeedTreeBranchShaderProperty*>(nullptr);
            }
            if (geom.m_sBranches.m_pTangents && geom.m_sBranches.m_pBinormals) {
                NiPoint3* pNormals = reinterpret_cast<NiPoint3*>(const_cast<float*>(geom.m_sBranches.m_pNormals));
                for (int32_t iv = 0; iv < sVerts; ++iv) {
                    NiPoint3* pNormal = &pNormals[iv];
                    float fX = pNormal->x;
                    float fY = pNormal->y;
                    float fZ = pNormal->z;
                    if (std::isnan(fX) || std::isnan(fY) || std::isnan(fZ)) {
                        pNormals[iv] = NiPoint3::UNIT_Z;
                    }
                }
            }

            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                pSpeedTree->GetGeometry(geom, 1u, static_cast<int16_t>(sLevel), -1, -1);
                sVerts = geom.m_sBranches.m_usVertexCount;
                if (geom.m_sBranches.m_usVertexCount &&
                    geom.m_sBranches.m_pStripLengths &&
                    geom.m_sBranches.m_pStrips) {
                    bool bTangentSpace;
                    uint16_t sVert;
                    uint32_t index;
                    uint16_t* pCompositeStripLength = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t)));
                    *pCompositeStripLength = *geom.m_sBranches.m_pStripLengths;
                    uint16_t* pCompositeStrip = static_cast<uint16_t*>(
                        _NiMalloc(sizeof(uint16_t) * *pCompositeStripLength)
                    );
                    ourMemcpy(
                        pCompositeStrip,
                        *reinterpret_cast<const void* const*>(geom.m_sBranches.m_pStrips),
                        2 * *pCompositeStripLength
                    );
                    bool* vertFlags = static_cast<bool*>(ourMalloc(geom.m_sBranches.m_usVertexCount));
                    ourMemset(vertFlags, 0, geom.m_sBranches.m_usVertexCount);
                    for (index = 0; index < *pCompositeStripLength; ++index) {
                        vertFlags[pCompositeStrip[index]] = true;
                    }
                    NiTPrimitiveArray<uint32_t> vertsUsed(0, 1u);
                    for (sVert = 0; sVert < static_cast<int32_t>(geom.m_sBranches.m_usVertexCount); ++sVert) {
                        if (vertFlags[sVert]) {
                            uiUsedVert = sVert;
                            vertsUsed.Add(uiUsedVert);
                        }
                    }
                    ourFree(vertFlags);
                    for (sVert = 0; static_cast<uint32_t>(sVert) < vertsUsed.m_usSize; ++sVert) {
                        uint32_t uiVertIndex = vertsUsed[sVert];
                        for (index = 0; index < *pCompositeStripLength; ++index) {
                            if ((*geom.m_sBranches.m_pStrips)[index] == uiVertIndex) {
                                pCompositeStrip[index] = sVert;
                            }
                        }
                    }
                    NiPoint3* pCoords = new NiPoint3[vertsUsed.m_usSize];
                    NiPoint2* pTex = new NiPoint2[vertsUsed.m_usSize];
                    NiPoint3* pNormals = nullptr;
                    NiPoint3* pTangents = nullptr;
                    NiPoint3* pBinormals = nullptr;
                    if (geom.m_sBranches.m_pTangents && geom.m_sBranches.m_pBinormals) {
                        // One array holds the normals, then the binormals, then the tangents.
                        pNormals = new NiPoint3[3 * vertsUsed.m_usSize];
                        uint32_t uiVertCount = vertsUsed.m_usSize;
                        pBinormals = &pNormals[uiVertCount];
                        pTangents = &pNormals[2 * uiVertCount];
                        bTangentSpace = true;
                    }
                    else {
                        pNormals = new NiPoint3[vertsUsed.m_usSize];
                        bTangentSpace = false;
                    }
                    for (sVert = 0; static_cast<uint32_t>(sVert) < vertsUsed.m_usSize; ++sVert) {
                        uint32_t uiVertIndex = vertsUsed[sVert];
                        uint32_t offset3_x = 3 * uiVertIndex;
                        uint32_t offset3_y = 3 * uiVertIndex + 1;
                        uint32_t offset3_z = 3 * uiVertIndex + 2;
                        uint32_t offset2_x = 2 * uiVertIndex;
                        uint32_t offset2_y = 2 * uiVertIndex + 1;
                        pCoords[sVert] = NiPoint3(
                            geom.m_sBranches.m_pCoords[3 * uiVertIndex],
                            geom.m_sBranches.m_pCoords[offset3_y],
                            geom.m_sBranches.m_pCoords[offset3_z]
                        );
                        pNormals[sVert] = NiPoint3(
                            geom.m_sBranches.m_pNormals[offset3_x],
                            geom.m_sBranches.m_pNormals[offset3_y],
                            geom.m_sBranches.m_pNormals[offset3_z]
                        );
                        pTex[sVert] = NiPoint2(
                            geom.m_sBranches.m_pTexCoords0[offset2_x],
                            geom.m_sBranches.m_pTexCoords0[offset2_y]
                        );
                        if (bTangentSpace) {
                            pTangents[sVert] = NiPoint3(
                                geom.m_sBranches.m_pTangents[offset3_x],
                                geom.m_sBranches.m_pTangents[offset3_y],
                                geom.m_sBranches.m_pTangents[offset3_z]
                            );
                            pBinormals[sVert] = NiPoint3(
                                geom.m_sBranches.m_pBinormals[offset3_x],
                                geom.m_sBranches.m_pBinormals[offset3_y],
                                geom.m_sBranches.m_pBinormals[offset3_z]
                            );
                        }
                    }
                    D3DXVECTOR4* pWindInfo = new D3DXVECTOR4[vertsUsed.m_usSize];
                    for (sVert = 0; static_cast<uint32_t>(sVert) < vertsUsed.m_usSize; ++sVert) {
                        uint32_t uiVertIndex = vertsUsed[sVert];
                        float fColor = static_cast<double>((geom.m_sBranches.m_pColors[uiVertIndex] & 0xFF00) >> 8) /
                            255.0;
                        NiColorA kWindInfo(
                            geom.m_sBranches.m_pWindWeights[uiVertIndex],
                            static_cast<float>(4 * geom.m_sBranches.m_pWindMatrixIndices[uiVertIndex]),
                            fColor,
                            0.0f
                        );
                        // The NiColorA is reinterpreted as a D3DXVECTOR4, copying the colour's four components into
                        // the vector.
                        pWindInfo[sVert] = std::bit_cast<D3DXVECTOR4>(kWindInfo);
                    }
                    NiPointer<STSPData> spShaderData(new STSPData);
                    spShaderData->SetData(pWindInfo, vertsUsed.m_usSize);
                    this->pspBranchData[sLevel] = new NiTriStripsData(
                        vertsUsed.m_usSize,
                        pCoords,
                        pNormals,
                        nullptr,
                        pTex,
                        1,
                        bTangentSpace ? NiGeometryData::NBT_METHOD_NDL : NiGeometryData::NBT_METHOD_NONE,
                        *geom.m_sBranches.m_pStripLengths - 2,
                        1,
                        pCompositeStripLength,
                        pCompositeStrip
                    );
                    this->pspBranchData[sLevel]->SetConsistency(NiGeometryData::STATIC);
                    this->pspBranchData[sLevel]->m_ucKeepFlags = 17u;
                    this->pspBranchData[sLevel]->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
                    this->pspBranchShaderProperties[sLevel] = new SpeedTreeBranchShaderProperty(spShaderData.m_pObject);
                }
            }
            pSpeedTree->FreeBranchColors();
            geom.m_sBranches.m_pColors = nullptr;
            pSpeedTree->FreeBranchWindData();
            geom.m_sBranches.m_pWindWeights = nullptr;
            geom.m_sBranches.m_pWindMatrixIndices = nullptr;
        }
    }
    geom.Destroy();
}

void BSTreeModel::CreateLeafGeometry(TESObjectTREE* apTreeObj) {
    uint16_t sLevels;
    CSpeedTreeRT::SGeometry geom;
    uint16_t sMaps;
    CSpeedTreeRT::STextures textures;
    const NiPoint3* pCoords = nullptr;
    const NiPoint3* pNormals = nullptr;
    {
        NiPointer<STSPData> spShaderData;
        bool bFlip = false;
        if (pSpeedTree && pSpeedTree->GetLeafLodSizeAdjustments() && !IsInstance()) {
            if (this->pspLeafData) {
                NiPointer<NiTriShapeData>* pspOldLeafData = this->pspLeafData;
                if (pspOldLeafData) {
                    delete[] pspOldLeafData;
                }
                this->pspLeafData = nullptr;
            }
            if (this->pspLeafShaderProperties) {
                NiPointer<SpeedTreeLeafShaderProperty>* pspOldProperties = this->pspLeafShaderProperties;
                if (pspOldProperties) {
                    delete[] pspOldProperties;
                }
                this->pspLeafShaderProperties = nullptr;
            }
            sLevels = GetNumLeafLODLevels();
            if (sLevels) {
                pSpeedTree->GetTextures(textures);
            }
            if (sLevels && (sMaps = textures.m_uiLeafTextureCount) != 0) {
                uint16_t sLevel;
                for (uint16_t sMap = 0; sMap < static_cast<int32_t>(sMaps); ++sMap) {
                    for (uint16_t sMap2 = sMap + 1; sMap2 < static_cast<int32_t>(sMaps); ++sMap2) {
                        if (BSstrcmp(textures.m_pLeafTextureFilenames[sMap], textures.m_pLeafTextureFilenames[sMap2])) {
                            bFlip = true;
                        }
                    }
                }
                this->pspLeafData = new NiPointer<NiTriShapeData>[sLevels];
                this->pspLeafShaderProperties = new NiPointer<SpeedTreeLeafShaderProperty>[sLevels];
                for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                    this->pspLeafData[sLevel] = static_cast<NiTriShapeData*>(nullptr);
                    this->pspLeafShaderProperties[sLevel] = static_cast<SpeedTreeLeafShaderProperty*>(nullptr);
                }
                spLeafShaderData = new STLSPData;
                spLeafShaderData->SetCurveScalar(fCurveScalar);
                if (pSpeedTree && pSpeedTree->m_pWindEngine) {
                    spLeafShaderData->SetRock(pSpeedTree->m_pWindEngine->m_fSpeedWindRockScalar);
                    spLeafShaderData->SetRustle(pSpeedTree->m_pWindEngine->m_fSpeedWindRustleScalar);
                    if (apTreeObj) {
                        spLeafShaderData->SetRockSpeed(apTreeObj->GetRockSpeed());
                        spLeafShaderData->SetRustleSpeed(apTreeObj->GetRustleSpeed());
                    }
                }

                for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                    pSpeedTree->GetGeometry(geom, 4u, -1, -1, static_cast<int16_t>(sLevel));
                    uint16_t sLeafs = geom.m_sLeaves0.m_usLeafCount;
                    uint16_t sVerts = 4 * geom.m_sLeaves0.m_usLeafCount;
                    if (geom.m_sLeaves0.m_usLeafCount) {
                        pCoords = reinterpret_cast<const NiPoint3*>(geom.m_sLeaves0.m_pCenterCoords);
                        pNormals = reinterpret_cast<const NiPoint3*>(geom.m_sLeaves0.m_pNormals);
                        NiPoint3* pCoordsCopy = new NiPoint3[sVerts];
                        NiPoint3* pNormalsCopy = new NiPoint3[sVerts];
                        NiPoint2* pTexCopy = new NiPoint2[sVerts];
                        D3DXVECTOR4* pWindInfo = new D3DXVECTOR4[sVerts];
                        uint16_t* pTriListCopy = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * sLeafs));
                        uint16_t* pTriListMove = pTriListCopy;
                        for (uint16_t sLeaf = 0; sLeaf < static_cast<int32_t>(sLeafs); ++sLeaf) {
                            uint16_t sVertStart = 4 * sLeaf;
                            uint16_t sTriStart = 6 * sLeaf;
                            // The leaf's centre goes into its fourth corner, and is then copied down corner by corner.
                            pCoordsCopy[sVertStart + 3] = pCoords[sLeaf];
                            pCoordsCopy[sVertStart + 2] = pCoordsCopy[sVertStart + 3];
                            pCoordsCopy[sVertStart + 1] = pCoordsCopy[sVertStart + 2];
                            pCoordsCopy[sVertStart] = pCoordsCopy[sVertStart + 1];
                            pNormalsCopy[sVertStart + 3] = pNormals[sLeaf];
                            pNormalsCopy[sVertStart + 2] = pNormalsCopy[sVertStart + 3];
                            pNormalsCopy[sVertStart + 1] = pNormalsCopy[sVertStart + 2];
                            pNormalsCopy[sVertStart] = pNormalsCopy[sVertStart + 1];
                            float fColor = static_cast<double>((geom.m_sLeaves0.m_pColors[sLeaf] & 0xFF00) >> 8) /
                                255.0;
                            if (fColor >= 1.0) {
                                fColor = 0.99f;
                            }
                            const float* pTex = geom.m_sLeaves0.m_pLeafMapTexCoords[sLeaf];
                            for (uint16_t sVert = 0; sVert < 4u; ++sVert) {
                                if (bFlip) {
                                    pTexCopy[sVert + sVertStart] = NiPoint2(pTex[2 * sVert], pTex[2 * (3 - sVert) + 1]);
                                }
                                else {
                                    pTexCopy[sVert + sVertStart] = NiPoint2(pTex[2 * sVert], pTex[2 * sVert + 1]);
                                }
                                float fColorClusterIndex = static_cast<float>(
                                    (sVert + 2) % 4 + 4 * geom.m_sLeaves0.m_pLeafClusterIndices[sLeaf]
                                );
                                float fClusterColor = fColorClusterIndex + fColor;
                                const float* pLodSizeAdjustments = pSpeedTree->GetLeafLodSizeAdjustments();
                                NiColorA kWindInfo(
                                    geom.m_sLeaves0.m_pWindWeights[sLeaf],
                                    static_cast<float>(4 * geom.m_sLeaves0.m_pWindMatrixIndices[sLeaf]),
                                    fClusterColor,
                                    pLodSizeAdjustments[sLevel]
                                );
                                // The NiColorA is reinterpreted as a D3DXVECTOR4, copying the colour's four components
                                // into the vector.
                                pWindInfo[sVert + sVertStart] = std::bit_cast<D3DXVECTOR4>(kWindInfo);
                            }
                            pTriListMove[sTriStart] = sVertStart + 3;
                            pTriListMove[sTriStart + 1] = sVertStart + 1;
                            pTriListMove[sTriStart + 2] = sVertStart + 2;
                            pTriListMove[sTriStart + 3] = sVertStart;
                            pTriListMove[sTriStart + 4] = sVertStart + 1;
                            pTriListMove[sTriStart + 5] = sVertStart + 3;
                        }
                        this->pspLeafData[sLevel] = new NiTriShapeData(
                            sVerts,
                            pCoordsCopy,
                            pNormalsCopy,
                            nullptr,
                            pTexCopy,
                            1,
                            NiGeometryData::NBT_METHOD_NONE,
                            2 * sLeafs,
                            pTriListCopy
                        );
                        spShaderData = new STSPData;
                        spShaderData->SetData(pWindInfo, sVerts);
                        this->pspLeafShaderProperties[sLevel] = new SpeedTreeLeafShaderProperty(
                            sLevel,
                            spShaderData.m_pObject,
                            spLeafShaderData.m_pObject
                        );
                        this->pspLeafShaderProperties[sLevel]->SetBaseTexture(spLeafFrondTexture.m_pObject);
                        NiAdditionalGeometryData* pAdditionalGeomData = this->pspLeafShaderProperties[sLevel]
                                                                            ->CreateRendererSpecificProperty(nullptr);
                        this->pspLeafData[sLevel]->SetAdditionalGeometryData(pAdditionalGeomData);
                        this->pspLeafData[sLevel]->SetConsistency(NiGeometryData::STATIC);
                        this->pspLeafData[sLevel]->m_ucKeepFlags = 17u;
                        this->pspLeafData[sLevel]->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
                    }
                }
                pSpeedTree->FreeLeafLODDataArrays();
            }
        }
    }
    geom.Destroy();
}

void BSTreeModel::CreateBillboardGeometry(TESObjectTREE* apTreeObj) {
    float fDirection[3];
    CSpeedTreeRT::SGeometry geom;
    float fCameraPos[3];
    fCameraPos[0] = 0.0f;
    fCameraPos[1] = 0.0f;
    fCameraPos[2] = 0.0f;
    fDirection[0] = 0.0f;
    fDirection[1] = -1.0f;
    fDirection[2] = 0.0f;
    if (pSpeedTree && !IsInstance() && apTreeObj) {
        ShadowLightShader* pSLShader;
        SpeedTreeBillboardShaderProperty* pBillboardProp;
        // Filled by GetCamera.
        float fCameraDirOld[3];
        float fCameraPosOld[3];
        CSpeedTreeRT::GetCamera(fCameraPosOld, fCameraDirOld);
        CSpeedTreeRT::SetCamera(fCameraPos, fDirection);
        pSpeedTree->SetLodLevel(0.0f);
        NiPointer<NiTriShapeData> spBillboardData;
        apTreeObj->CreateBillboardGeomData(false, spBillboardData);
        spBillboardData->SetConsistency(NiGeometryData::STATIC);
        spBillboardData->m_ucKeepFlags = 17u;
        spBillboardData->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
        spBillboard = new NiTriShape(spBillboardData.m_pObject);
        NiPointer<NiAlphaProperty> spPropAlpha(BSTreeModel::CreateAlphaProperty());
        if (spPropAlpha.m_pObject) {
            spBillboard->AttachProperty(spPropAlpha.m_pObject);
        }
        pBillboardProp = new SpeedTreeBillboardShaderProperty;
        spBillboard->AttachProperty(pBillboardProp);
        pBillboardProp->LoadTextureSet("STBB", spBillboardTextureSet.m_pObject);
        NiSourceTexture* pDefNormalMap = BSShader::GetDefNormalMap();
        pBillboardProp->SetNormalTexture(0, pDefNormalMap);
        pBillboardProp->SetClampNI(NiTexturingProperty::CLAMP_S_CLAMP_T);
        pBillboardProp->SetFlag(BSShaderProperty::BSSP_FLAG_NOPROJSHADOW, 1);
        pBillboardProp->SetFlag(BSShaderProperty::BSSP_FLAG_BILLBOARDTREE, 1);
        BSShaderManager::PrepareObject(spBillboard.m_pObject, 0, 0);
        BSShader* pShader = spBillboard->GetShader();
        if (pShader) {
            // Shader types 1 to 5 are the ShadowLightShader family; the type getter is called twice.
            if (pShader->GetShaderType() < BSShaderManager::BSSM_SHADERTYPE_SHADOWLIGHT_FIRST ||
                pShader->GetShaderType() > BSShaderManager::BSSM_SHADERTYPE_SHADOWLIGHT_LAST) {
                pSLShader = nullptr;
            }
            else {
                pSLShader = static_cast<ShadowLightShader*>(pShader);
            }
        }
        else {
            pSLShader = nullptr;
        }
        if (pSLShader) {
            NiRenderer* pRenderer = NiRenderer::GetRenderer();
            NiDX9ShaderDeclaration* pDecl = pSLShader->GetDefaultShaderDecl();
            pRenderer->PrecacheGeometry(spBillboard.m_pObject, 0, 0, pDecl);
            NiDX9Renderer* pDX9Renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
            pDX9Renderer->PerformPrecache();
        }
        CSpeedTreeRT::SetCamera(fCameraPosOld, fCameraDirOld);
    }
    geom.Destroy();
}

bool BSTreeModel::InitAsInstance(BSTreeModel* apInstanceFrom) {
    BSTreeModel::kCloneLock.Enter(nullptr);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREESYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp",
        1041
    );
    if (apInstanceFrom &&
        apInstanceFrom->pSpeedTree &&
        !apInstanceFrom->IsInstance() &&
        eType == BSTreeModel::BTMT_NONE &&
        (pSpeedTree = apInstanceFrom->pSpeedTree->MakeInstance()) != nullptr) {
        uint16_t sLevel;
        spTreeParent = apInstanceFrom;
        eType = BSTreeModel::BTMT_INSTANCE;
        spBranchTextureSet = apInstanceFrom->spBranchTextureSet;
        spLeafFrondTexture = apInstanceFrom->spLeafFrondTexture;
        spBillboardTextureSet = apInstanceFrom->spBillboardTextureSet;
        spLeafShaderData = apInstanceFrom->spLeafShaderData;
        fCurveScalar = apInstanceFrom->fCurveScalar;
        iSeed = apInstanceFrom->iSeed;
        fTrunkHeight = apInstanceFrom->fTrunkHeight;
        fTrunkWidth = apInstanceFrom->fTrunkWidth;
        if (apInstanceFrom->spRigidBody.m_pObject) {
            BSCloningProcess cloneData(1.0f);
            spRigidBody = static_cast<bhkRigidBody*>(apInstanceFrom->spRigidBody->Clone(cloneData));
        }
        else {
            spRigidBody = static_cast<bhkRigidBody*>(nullptr);
        }
        uint16_t sLevels = GetNumBranchLODLevels();
        if (sLevels && apInstanceFrom->pspBranchData && apInstanceFrom->pspBranchShaderProperties) {
            pspBranchData = new NiPointer<NiTriStripsData>[sLevels];
            pspBranchShaderProperties = new NiPointer<SpeedTreeBranchShaderProperty>[sLevels];
            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                pspBranchData[sLevel] = apInstanceFrom->pspBranchData[sLevel];
                if (apInstanceFrom->pspBranchShaderProperties[sLevel].m_pObject) {
                    pspBranchShaderProperties[sLevel] = static_cast<SpeedTreeBranchShaderProperty*>(
                        apInstanceFrom->pspBranchShaderProperties[sLevel]->Clone()
                    );
                }
                else {
                    pspBranchShaderProperties[sLevel] = static_cast<SpeedTreeBranchShaderProperty*>(nullptr);
                }
            }
        }

        sLevels = GetNumLeafLODLevels();
        if (sLevels && apInstanceFrom->pspLeafData && apInstanceFrom->pspLeafShaderProperties) {
            pspLeafData = new NiPointer<NiTriShapeData>[sLevels];
            pspLeafShaderProperties = new NiPointer<SpeedTreeLeafShaderProperty>[sLevels];
            for (sLevel = 0; sLevel < static_cast<int32_t>(sLevels); ++sLevel) {
                pspLeafData[sLevel] = apInstanceFrom->pspLeafData[sLevel];
                if (apInstanceFrom->pspLeafShaderProperties[sLevel].m_pObject) {
                    pspLeafShaderProperties[sLevel] = static_cast<SpeedTreeLeafShaderProperty*>(
                        apInstanceFrom->pspLeafShaderProperties[sLevel]->Clone()
                    );
                }
                else {
                    pspLeafShaderProperties[sLevel] = static_cast<SpeedTreeLeafShaderProperty*>(nullptr);
                }
            }
        }

        if (apInstanceFrom->spBillboard.m_pObject) {
            NiAlphaProperty::abCloneUnique = 1;
            spBillboard = static_cast<NiTriShape*>(apInstanceFrom->spBillboard->Clone());
            NiAlphaProperty::abCloneUnique = 0;
        }
        BSTreeModel::kCloneLock.Leave();
        return true;
    }

    BSTreeModel::kCloneLock.Leave();
    return false;
}

bool BSTreeModel::InitFromBase(TESObjectTREE* apTreeObj, uint32_t aiSeed) {
    CSpeedTreeRT* pspeedtree;
    char pFilename[268];
    if (!apTreeObj || !apTreeObj->GetModel() || !*apTreeObj->GetModel() || eType) {
        return true;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREESYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp",
        1139
    );
    BSstrcpy(pFilename, 260, "Trees");
    BSstrcat(pFilename, 260, apTreeObj->GetModel());
    pspeedtree = new CSpeedTreeRT;
    if (pspeedtree->LoadTree(pFilename)) {
        float fSizeVar;
        float fSize;
        CSpeedTreeRT::STextures textures;
        pspeedtree->GetTextures(textures);
        if (textures.m_uiLeafTextureCount > 3) {
            pspeedtree->SetNumLeafRockingGroups(1u);
            if (textures.m_uiLeafTextureCount > 6) {
                Warning(
                    "MODELS: Tree %s has too many leaf maps (greater than six).  Leaves may not display properly.",
                    apTreeObj->GetModel()
                );
            }
        }
        else {
            pspeedtree->SetNumLeafRockingGroups(2u);
        }
        pspeedtree->SetLeafRockingState(true);
        pspeedtree->GetTreeSize(fSize, fSizeVar);
        float fNewVariance = GameSetting::fTreeSizeConversion::Get() * fSizeVar;
        float fNewSize = GameSetting::fTreeSizeConversion::Get() * fSize;
        pspeedtree->SetTreeSize(fNewSize, fNewVariance);
        pspeedtree->SetWindStrength(0.0f, -1.0f, -1.0f);
        pspeedtree->SetLocalMatrices(0, 4u);
        pspeedtree->m_eLeafLodMethod = CSpeedTreeRT::LOD_SMOOTH;
        pspeedtree->SetBranchWindMethod(CSpeedTreeRT::WIND_GPU);
        pspeedtree->SetFrondWindMethod(CSpeedTreeRT::WIND_GPU);
        pspeedtree->SetLeafWindMethod(CSpeedTreeRT::WIND_GPU);
        pspeedtree->SetBranchLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
        pspeedtree->SetFrondLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
        pspeedtree->SetLeafLightingMethod(CSpeedTreeRT::LIGHT_DYNAMIC);
        float fFarDistance = BSTreeManager::GetFarDistance();
        float fNearDistance = BSTreeManager::GetNearDistance();
        pspeedtree->SetLodLimits(fNearDistance, fFarDistance);
        eType = BSTreeModel::BTMT_ORIGINAL;
        pSpeedTree = pspeedtree;
        ApplyBaseObject(apTreeObj);
        if (!pspeedtree->Compute(nullptr, aiSeed, true)) {
            return false;
        }
        pspeedtree->FreeProjectedShadowData();
        iSeed = pspeedtree->GetSeed();
        fTrunkHeight = pspeedtree->GetTrunkLength();
        fTrunkWidth = pspeedtree->GetTrunkWidth();
        CreateGeometry(apTreeObj);
        pspeedtree->DeleteTransientData();
        pspeedtree->DeleteBranchGeometry();
        pspeedtree->DeleteFrondGeometry();
        return true;
    }
    if (pspeedtree) {
        delete pspeedtree;
    }
    return false;
}

bool BSTreeModel::IsInstance() {
    return eType == BTMT_INSTANCE;
}

NiAlphaProperty* BSTreeModel::CreateAlphaProperty() {
    NiAlphaProperty* pAlpha = new NiAlphaProperty;
    pAlpha->SetAlphaBlending(false);
    pAlpha->SetAlphaTesting(true);
    pAlpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
    pAlpha->SetTestRef(0x54u);
    return pAlpha;
}

NiDX9ShaderDeclaration* ShadowLightShader::GetDefaultShaderDecl() {
    return spShaderDecl;
}

void STSPData::SetData(D3DXVECTOR4* apWindInfo, uint16_t asVertexCount) {
    pWindInfo = apWindInfo;
    sVertexCount = asVertexCount;
}

float SpeedTree::fTreeForceCS::Get() {
    return gs_fTreeForceCS_SpeedTree.Float();
}

float SpeedTree::fTreeForceBranchDimming::Get() {
    return gs_fTreeForceBranchDimming_SpeedTree.Float();
}

float SpeedTree::fTreeForceLeafDimming::Get() {
    return gs_fTreeForceLeafDimming_SpeedTree.Float();
}

float SpeedTree::fTreeForceMinBudAngle::Get() {
    return gs_fTreeForceMinBudAngle_SpeedTree.Float();
}

float SpeedTree::fTreeForceMaxBudAngle::Get() {
    return gs_fTreeForceMaxBudAngle_SpeedTree.Float();
}
