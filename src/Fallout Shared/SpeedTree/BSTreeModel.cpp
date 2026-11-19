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
#include "SDK/MSVC/ehvec.h"
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
    uint32_t ientries;
    CSpeedTreeRT::SGeometry geom;
    geom.Init_00B029F0();
    ientries = 0;
    if (apNode && (apNode->GetModel() && (pSpeedTree && BSTreeManager::GetTreesEnabled()))) {
        float flod;
        NiPoint3 ecx0 = apNode->GetWorldTranslate();
        pSpeedTree->SetTreePosition(ecx0.x, ecx0.y, ecx0.z);
        if (BSTreeManager::GetForceFullLOD()) {
            pSpeedTree->SetLodLevel(1.0f);
            flod = 1.0f;
        }
        else {
            pSpeedTree->ComputeLodLevel();
            flod = pSpeedTree->GetLodLevel();
        }
        pSpeedTree->GetBSGeometryLODData(geom, flod);
        bool bdrawnode = false;
        if (geom.m_sBillboard0.m_bIsActive) {
            bdrawnode = true;
            apNode->SetVisibilityOnBillboard(static_cast<int32_t>(geom.m_sBillboard0.m_fAlphaTestValue));
        }
        apNode->SetVisibilityForBillboards(bdrawnode);
        if (!(flod <= 0.0)) {
            int16_t slevel;
            bdrawnode = false;
            int16_t slevels = GetNumBranchLODLevels();
            if (slevels > 0) {
                float a4 = pSpeedTree->GetLodLevel();
                int16_t slevelVisible = pSpeedTree->GetDiscreteBranchLodLevel(a4);
                for (slevel = 0; slevel < slevels; ++slevel) {
                    if (slevelVisible == slevel) {
                        bdrawnode = true;
                        apNode->SetVisibilityOnBranch(slevel, static_cast<int32_t>(geom.m_fBranchAlphaTestValue));
                    }
                    else {
                        apNode->SetVisibilityOnBranch(slevel, 255);
                    }
                }
            }
            apNode->SetVisibilityForBranches(bdrawnode);
            if (BSTreeManager::AreLeavesVisible()) {
                bdrawnode = false;
                int16_t slevels_2 = GetNumLeafLODLevels();
                if (slevels_2 > 0) {
                    if (geom.m_sLeaves0.m_bIsActive || geom.m_sLeaves1.m_bIsActive) {
                        for (slevel = 0; slevel < slevels_2; ++slevel) {
                            if (geom.m_sLeaves0.m_bIsActive && geom.m_sLeaves0.m_nDiscreteLodLevel == slevel) {
                                bdrawnode = true;
                                apNode->SetVisibilityOnLeaf(
                                    slevel, static_cast<int32_t>(geom.m_sLeaves0.m_fAlphaTestValue)
                                );
                            }
                            else {
                                if (geom.m_sLeaves1.m_bIsActive && geom.m_sLeaves1.m_nDiscreteLodLevel == slevel) {
                                    bdrawnode = true;
                                    apNode->SetVisibilityOnLeaf(
                                        slevel, static_cast<int32_t>(geom.m_sLeaves1.m_fAlphaTestValue)
                                    );
                                }
                                else {
                                    apNode->SetVisibilityOnLeaf(slevel, 255);
                                }
                            }
                        }
                    }
                    if (spLeafShaderData.m_pObject) {
                        const float* pmaps;
                        if (pSpeedTree->InstanceOf()) {
                            const CSpeedTreeRT* v2 = pSpeedTree->InstanceOf();
                            pmaps = v2->GetLeafBillboardTable(ientries);
                        }
                        else {
                            pmaps = pSpeedTree->GetLeafBillboardTable(ientries);
                        }
                        spLeafShaderData.m_pObject->SetLeafMaps(pmaps, ientries);
                    }
                }
                apNode->SetVisibilityForLeaves(bdrawnode);
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
        MEM_CONTEXT_TREEART, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp",
        901
    );
    BSTreeNode* v26 = nullptr;
    // Holds the branch and billboard nodes and is never read, as the game does.
    [[maybe_unused]] NiNode* pnode = nullptr;
    NiPointer<bhkRigidBody> sprigid(static_cast<bhkRigidBody*>(nullptr));
    if (pSpeedTree) {
        BSTreeNode* pretval;
        uint16_t slevels;
        uint16_t slevel;
        NiD3DShader* pshader;
        NiTriBasedGeom* pgeom_2;
        NiAlphaProperty* ppropAlpha;
        if (!(fTrunkHeight > 64.0)) {
            sprigid.operator=(static_cast<bhkRigidBody*>(nullptr));
        }
        else {
            if (afScale == 1.0) {
                sprigid.operator=(GetRigidBody());
            }
            else {
                float v16 = afScale * fTrunkWidth / 2.0;
                float afHeight = afScale * fTrunkHeight;
                sprigid.operator=(bhkUtilFunctions::CreateCapsuleBody(afHeight, v16));
            }
        }
        BSTreeNode* v21 = static_cast<BSTreeNode*>(NiMemObject::operator new(sizeof(BSTreeNode)));
        if (v21) {
            pretval = ::new (v21) BSTreeNode(this, sprigid.m_pObject);
        }
        else {
            pretval = nullptr;
        }
        if ((pspBranchData) && (pspBranchShaderProperties)) {
            slevels = GetNumBranchLODLevels();
            pshader = (NiD3DShader*)BSShaderManager::GetShader(4);
            pnode = pretval->GetBranchNode();
            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                if (pspBranchData[slevel].m_pObject) {
                    NiTriBasedGeom* pgeom;
                    NiTriStrips* v20 = static_cast<NiTriStrips*>(NiMemObject::operator new(sizeof(NiTriStrips)));
                    if (v20) {
                        pgeom = (NiTriBasedGeom*)::new (v20) NiTriStrips(pspBranchData[slevel].m_pObject);
                    }
                    else {
                        pgeom = nullptr;
                    }
                    pgeom_2 = pgeom;
                    ppropAlpha = BSTreeModel::CreateAlphaProperty();
                    if (ppropAlpha) {
                        ppropAlpha->SetAlphaTesting(false);
                        pgeom_2->AttachProperty(ppropAlpha);
                    }
                    if (pspBranchShaderProperties[slevel].m_pObject) {
                        pspBranchShaderProperties[slevel].m_pObject->FreeRenderPasses();
                        SpeedTreeBranchShaderProperty* a5 = pspBranchShaderProperties[slevel].m_pObject;
                        pgeom_2->AttachProperty(a5);
                        if (!pgeom_2->GetModelData()->GetAdditionalGeometryData()) {
                            SpeedTreeBranchShaderProperty* v12 = pspBranchShaderProperties[slevel].m_pObject;
                            NiAdditionalGeometryData* pdata = v12->CreateRendererSpecificProperty(pgeom_2);
                            pgeom_2->SetAdditionalGeometryData(pdata);
                        }
                    }
                    if (IsInstance()) {
                        pgeom_2->SetShader(static_cast<BSShader*>(pshader));
                    }
                    pretval->AddBranchLevel(slevel, pgeom_2);
                }
            }
            if (!IsInstance()) {
                for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                    pgeom_2 = pretval->GetBranchLevel(slevel);
                    if (pgeom_2) {
                        BSShaderTextureSet* a5a = spBranchTextureSet.m_pObject;
                        pspBranchShaderProperties[slevel].m_pObject->LoadTextureSet("STBranch", a5a);
                        BSShaderManager::PrepareObject(pgeom_2, 0, 0);
                        SpeedTreeBranchShaderProperty* v11 = pspBranchShaderProperties[slevel].m_pObject;
                        NiAdditionalGeometryData* pdata_2 = v11->CreateRendererSpecificProperty(pgeom_2);
                        pgeom_2->SetAdditionalGeometryData(pdata_2);
                        if (!pgeom_2->GetModelData()->m_pkBuffData) {
                            NiRenderer* Renderer_ = NiRenderer::GetRenderer();
                            Renderer_->PrecacheGeometry(pgeom_2, 0, 0, nullptr);
                        }
                    }
                }
            }
        }

        if ((pspLeafData) && (pspLeafShaderProperties)) {
            slevels = GetNumLeafLODLevels();
            pshader = (NiD3DShader*)BSShaderManager::GetShader(6);
            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                if (pspLeafData[slevel].m_pObject) {
                    NiTriBasedGeom* pgeom_3;
                    NiTriShape* v19 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
                    if (v19) {
                        pgeom_3 = (NiTriBasedGeom*)::new (v19) NiTriShape(pspLeafData[slevel].m_pObject);
                    }
                    else {
                        pgeom_3 = nullptr;
                    }
                    pgeom_2 = pgeom_3;
                    ppropAlpha = BSTreeModel::CreateAlphaProperty();
                    if (ppropAlpha) {
                        pgeom_2->AttachProperty(ppropAlpha);
                    }
                    if (pspLeafShaderProperties[slevel].m_pObject) {
                        pspLeafShaderProperties[slevel].m_pObject->FreeRenderPasses();
                        SpeedTreeLeafShaderProperty* a5b = pspLeafShaderProperties[slevel].m_pObject;
                        pgeom_2->AttachProperty(a5b);
                    }
                    pretval->AddLeafLevel(slevel, pgeom_2);
                    pgeom_2->SetShader(static_cast<BSShader*>(pshader));
                    NiRenderer* v4 = NiRenderer::GetRenderer();
                    v4->PrecacheGeometry(pgeom_2, 0, 0, nullptr);
                }
            }
        }

        pnode = pretval->GetBillboardNode();
        if (spBillboard.m_pObject) {
            NiTriShape* a5c = spBillboard.m_pObject;
            pretval->SetBillboard(a5c);
        }
        pretval->UpdateProperties();
        NiUpdateData UpdateData(0.0f, false, false);
        pretval->UpdateSelected(UpdateData);
        pretval->MakeHavokLeaves(pSpeedTree, afScale);
        return pretval;
    }

    return nullptr;
}

void BSTreeModel::ApplyBaseObject(TESObjectTREE* apTreeObj) {
    NiPointer<NiSourceTexture> sptex(nullptr);
    if (apTreeObj && (pSpeedTree && !IsInstance())) {
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
        float fval = SpeedTree::fTreeForceCS::Get();
        if (fval < 0.0) {
            fval = apTreeObj->GetCurveScalar();
        }
        if (fval >= 0.0) {
            fCurveScalar = fval;
        }
        float fmin = SpeedTree::fTreeForceMinBudAngle::Get();
        float fmax = SpeedTree::fTreeForceMaxBudAngle::Get();
        if (fmin < 0.0 || (fmin > 90.0 || (fmax < 0.0 || (fmax > 90.0 || fmax < static_cast<double>(fmin))))) {
            fmin = apTreeObj->GetMinimumLeafAngle();
            fmax = apTreeObj->GetMaximumLeafAngle();
        }
        if (fmin >= 0.0 && fmin <= 90.0 && fmax >= 0.0 && fmax <= 90.0 && fmax >= static_cast<double>(fmin)) {
            pSpeedTree->SetBudAngleMin(fmin);
            pSpeedTree->SetBudAngleMax(fmax);
        }
        AutoMemContext tempMemContext(
            MEM_CONTEXT_TREEART, true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp", 145
        );
        if (!spBranchTextureSet.m_pObject) {
            BSShaderTextureSet* v6;
            char Src[264];
            char a1[268];
            CSpeedTreeRT::STextures stex;
            int32_t v12 = 0;
            pSpeedTree->GetTextures(stex);
            BSstrcpy(Src, 260, "Textures\\Trees\\Branches\\");
            BSstrcat(Src, 260, stex.m_pBranchTextureFilename);
            uint32_t itemp = BSstrlen(Src);
            Src[itemp - 3] = 100;
            Src[itemp - 2] = 100;
            Src[itemp - 1] = 115;
            BSShaderManager::GetModifiedTextureFilename(a1, Src, "_n", true);
            BSShaderTextureSet* v9 =
                static_cast<BSShaderTextureSet*>(NiMemObject::operator new(sizeof(BSShaderTextureSet)));
            if (v9) {
                v6 = ::new (v9) BSShaderTextureSet;
            }
            else {
                v6 = nullptr;
            }
            spBranchTextureSet.operator=(v6);
            BSShaderTextureSet* v5 = spBranchTextureSet.m_pObject;
            v5->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, Src);
            BSShaderTextureSet* v4 = spBranchTextureSet.m_pObject;
            v4->SetTextureFilename(BSShaderProperty::TEXTYPE_NORMAL, a1);
        }
        if ((!spLeafFrondTexture.m_pObject) && (apTreeObj->GetTextureNameLength())) {
            char Dst[268];
            BSstrcpy(Dst, 260, "Textures\\Trees\\Leaves\\");
            BSstrcat(Dst, 260, apTreeObj->GetTextureName());
            TES::pInstance->CreateTextureImage(Dst, spLeafFrondTexture, true, false);
        }

        if (!spBillboardTextureSet.m_pObject) {
            BSShaderTextureSet* v3;
            char path[264];
            apTreeObj->GetBillboardTextureFilename(path);
            BSShaderTextureSet* v8 =
                static_cast<BSShaderTextureSet*>(NiMemObject::operator new(sizeof(BSShaderTextureSet)));
            if (v8) {
                v3 = ::new (v8) BSShaderTextureSet;
            }
            else {
                v3 = nullptr;
            }
            spBillboardTextureSet.operator=(v3);
            BSShaderTextureSet* v2 = spBillboardTextureSet.m_pObject;
            v2->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, path);
        }
    }
    else {}
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
    spBillboard.operator=(static_cast<NiTriShape*>(nullptr));
    spLeafShaderData.operator=(static_cast<STLSPData*>(nullptr));
    if (pspBranchShaderProperties) {
        delete[] pspBranchShaderProperties;
        pspBranchShaderProperties = nullptr;
    }
    if (pspLeafShaderProperties) {
        delete[] pspLeafShaderProperties;
        pspLeafShaderProperties = nullptr;
    }
    spBranchTextureSet.operator=(static_cast<BSShaderTextureSet*>(nullptr));
    spLeafFrondTexture.operator=(static_cast<NiSourceTexture*>(nullptr));
    spBillboardTextureSet.operator=(static_cast<BSShaderTextureSet*>(nullptr));
    spRigidBody.operator=(static_cast<bhkRigidBody*>(nullptr));
}

void BSTreeModel::CreateGeometry(TESObjectTREE* apTreeObj) {
    if ((pSpeedTree) && (!IsInstance())) {
        char Dst[264];
        char pcSrc[268];
        CreateBranchGeometry();
        CreateLeafGeometry(apTreeObj);
        if (apTreeObj->IsLargeEnoughForDistantLOD()) {
            CreateBillboardGeometry(apTreeObj);
        }
        spRigidBody.operator=(static_cast<bhkRigidBody*>(nullptr));
        const char* v2 = apTreeObj->GetModel();
        BSstrcpy(Dst, 260, v2);
        int32_t ilen = BSstrlen(Dst);
        if (ilen > 4) {
            Dst[ilen - 4] = 0;
        }
        BSStream stream;
        BSstrcpy(pcSrc, 260, "Data\\Meshes\\Trees");
        BSstrcat(pcSrc, 260, Dst);
        BSstrcat(pcSrc, 260, ".NIF");
        if ((FileFinder::Locate(pcSrc, nullptr, 6u, 1u)) &&
            (stream.BSStream::Load(pcSrc, static_cast<NiFile*>(nullptr)) && stream.GetObjectCount() == 1)) {
            bhkRigidBody* WorldObject;
            NiObject* pniObj = stream.GetObjectAt(0);
            NiAVObject* pniAVObj = static_cast<NiAVObject*>(NiObject::DynamicCastStatic(&NiAVObject::ms_RTTI, pniObj));
            bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(pniAVObj);
            if (pcol) {
                WorldObject = static_cast<bhkRigidBody*>(pcol->GetWorldObject());
            }
            else {
                WorldObject = nullptr;
            }
            if (WorldObject) {
                spRigidBody.operator=(WorldObject);
            }
        }

        if (!spRigidBody.m_pObject && fTrunkHeight > 64.0) {
            float afRadius = fTrunkWidth / 2.0;
            spRigidBody.operator=(bhkUtilFunctions::CreateCapsuleBody(fTrunkHeight, afRadius));
        }
    }
}

uint16_t BSTreeModel::GetNumBranchLODLevels() {
    if (pSpeedTree) {
        return pSpeedTree->GetNumBranchLodLevels();
    }

    return (uint16_t)0;
}

uint16_t BSTreeModel::GetNumLeafLODLevels() {
    if (pSpeedTree) {
        return pSpeedTree->GetNumLeafLodLevels();
    }

    return (uint16_t)0;
}

void BSTreeModel::SetInitialized(bool abVal) {
    bInitialized = (bool)((uint8_t)abVal);
}

bhkRigidBody* BSTreeModel::GetRigidBody() const {
    return (&spRigidBody)->operator->();
}

void BSTreeModel::CreateBranchGeometry() {
    uint32_t uiUsedVert;
    uint16_t slevels;
    uint16_t sverts;
    CSpeedTreeRT::SGeometry geom;
    geom.Init_00B029F0();
    if (!pSpeedTree) {
        goto LABEL_21;
    }
    if (IsInstance()) {
        goto LABEL_21;
    }
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
    slevels = GetNumBranchLODLevels();
    if (slevels &&
        ((pSpeedTree->GetGeometry(geom, 1u, -1, -1, -1), geom.m_sBranches.m_pCoords) &&
         (geom.m_sBranches.m_pNormals &&
          (geom.m_sBranches.m_pTexCoords0 &&
           (geom.m_sBranches.m_pColors &&
            (geom.m_sBranches.m_pWindWeights &&
             (geom.m_sBranches.m_pWindMatrixIndices && (sverts = geom.m_sBranches.m_usVertexCount) != 0))))))) {
        uint16_t slevel;
        this->pspBranchData = new NiPointer<NiTriStripsData>[slevels];
        this->pspBranchShaderProperties = new NiPointer<SpeedTreeBranchShaderProperty>[slevels];
        for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
            this->pspBranchData[slevel].operator=(static_cast<NiTriStripsData*>(nullptr));
            this->pspBranchShaderProperties[slevel].operator=(static_cast<SpeedTreeBranchShaderProperty*>(nullptr));
        }
        if ((geom.m_sBranches.m_pTangents) && (geom.m_sBranches.m_pBinormals)) {
            NiPoint3* pnormals = reinterpret_cast<NiPoint3*>(const_cast<float*>(geom.m_sBranches.m_pNormals));
            for (int32_t iv = 0; iv < sverts; ++iv) {
                NiPoint3* v3 = &pnormals[iv];
                float x_1 = v3->x;
                float y = v3->y;
                float z = v3->z;
                if (std::isnan(x_1) || (std::isnan(y) || std::isnan(z))) {
                    pnormals[iv] = NiPoint3_UNIT_Z;
                }
            }
        }

        for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
            pSpeedTree->GetGeometry(geom, 1u, static_cast<int16_t>(slevel), -1, -1);
            sverts = geom.m_sBranches.m_usVertexCount;
            if ((geom.m_sBranches.m_usVertexCount) && (geom.m_sBranches.m_pStripLengths) &&
                (geom.m_sBranches.m_pStrips)) {
                SpeedTreeBranchShaderProperty* SpeedTreeBranchShaderProperty_;
                NiTriStripsData* NiTriStripsData_;
                STSPData* STSPData_;
                bool btangentSpace;
                uint16_t svert;
                uint32_t index;
                uint16_t* pcompositeStripLength = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t)));
                *pcompositeStripLength = *geom.m_sBranches.m_pStripLengths;
                uint16_t* pcompositeStrip =
                    static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * *pcompositeStripLength));
                ourMemcpy(
                    pcompositeStrip, *reinterpret_cast<const void* const*>(geom.m_sBranches.m_pStrips),
                    2 * *pcompositeStripLength
                );
                bool* vertFlags = static_cast<bool*>(ourMalloc(geom.m_sBranches.m_usVertexCount));
                ourMemset(vertFlags, 0, geom.m_sBranches.m_usVertexCount);
                for (index = 0; index < *pcompositeStripLength; ++index) {
                    vertFlags[pcompositeStrip[index]] = true;
                }
                NiTPrimitiveArray<uint32_t> vertsUsed(0, 1u);
                for (svert = 0; svert < static_cast<int32_t>(geom.m_sBranches.m_usVertexCount); ++svert) {
                    if (vertFlags[svert]) {
                        uiUsedVert = svert;
                        vertsUsed.Add(uiUsedVert);
                    }
                }
                ourFree(vertFlags);
                for (svert = 0;; ++svert) {
                    uint32_t svert_1 = svert;
                    if (svert_1 >= vertsUsed.m_usSize) {
                        break;
                    }
                    uint32_t vertIndex = vertsUsed.operator[](svert);
                    for (index = 0; index < *pcompositeStripLength; ++index) {
                        if ((*geom.m_sBranches.m_pStrips)[index] == vertIndex) {
                            pcompositeStrip[index] = svert;
                        }
                    }
                }
                // new NiPoint3[n], new NiPoint2[n] and new D3DXVECTOR4[n] below: the size is all ones when it
                // overflows 32 bits, as new[] computes it, and each element gets the empty constructor.
                uint32_t numObjs = vertsUsed.m_usSize;
                NiPoint3* pcoords = static_cast<NiPoint3*>(
                    ourMalloc(numObjs <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * numObjs : SIZE_MAX)
                );
                if (pcoords) {
                    rt::eh_vector_constructor_iterator(pcoords, sizeof(NiPoint3), numObjs, &emptyPointConstructor);
                }
                uint32_t count = vertsUsed.m_usSize;
                NiPoint2* ptex = static_cast<NiPoint2*>(
                    ourMalloc(count <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * count : SIZE_MAX)
                );
                if (ptex) {
                    rt::eh_vector_constructor_iterator(ptex, sizeof(NiPoint2), count, &emptyPointConstructor);
                }
                NiPoint3* pnormals_2 = nullptr;
                NiPoint3* ptangents = nullptr;
                NiPoint3* pbinormals = nullptr;
                if (geom.m_sBranches.m_pTangents && geom.m_sBranches.m_pBinormals) {
                    // One array holds the normals, then the binormals, then the tangents.
                    uint32_t aiCount = 3 * vertsUsed.m_usSize;
                    pnormals_2 = static_cast<NiPoint3*>(
                        ourMalloc(aiCount <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * aiCount : SIZE_MAX)
                    );
                    if (pnormals_2) {
                        rt::eh_vector_constructor_iterator(
                            pnormals_2, sizeof(NiPoint3), aiCount, &emptyPointConstructor
                        );
                    }
                    uint32_t v6 = vertsUsed.m_usSize;
                    pbinormals = &pnormals_2[v6];
                    uint32_t v7 = vertsUsed.m_usSize;
                    ptangents = &pnormals_2[2 * v7];
                    btangentSpace = true;
                }
                else {
                    uint32_t aiCount_1 = vertsUsed.m_usSize;
                    pnormals_2 = static_cast<NiPoint3*>(
                        ourMalloc(aiCount_1 <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * aiCount_1 : SIZE_MAX)
                    );
                    if (pnormals_2) {
                        rt::eh_vector_constructor_iterator(
                            pnormals_2, sizeof(NiPoint3), aiCount_1, &emptyPointConstructor
                        );
                    }
                    btangentSpace = false;
                }
                for (svert = 0;; ++svert) {
                    uint32_t svert_2 = svert;
                    if (svert_2 >= vertsUsed.m_usSize) {
                        break;
                    }
                    uint32_t vertIndex_2 = vertsUsed.operator[](svert);
                    uint32_t offset3_x = 3 * vertIndex_2;
                    uint32_t offset3_y = 3 * vertIndex_2 + 1;
                    uint32_t offset3_z = 3 * vertIndex_2 + 2;
                    uint32_t offset2_x = 2 * vertIndex_2;
                    uint32_t offset2_y = 2 * vertIndex_2 + 1;
                    pcoords[svert] = NiPoint3(
                        geom.m_sBranches.m_pCoords[3 * vertIndex_2], geom.m_sBranches.m_pCoords[offset3_y],
                        geom.m_sBranches.m_pCoords[offset3_z]
                    );
                    pnormals_2[svert] = NiPoint3(
                        geom.m_sBranches.m_pNormals[offset3_x], geom.m_sBranches.m_pNormals[offset3_y],
                        geom.m_sBranches.m_pNormals[offset3_z]
                    );
                    ptex[svert] =
                        NiPoint2(geom.m_sBranches.m_pTexCoords0[offset2_x], geom.m_sBranches.m_pTexCoords0[offset2_y]);
                    if (btangentSpace) {
                        ptangents[svert] = NiPoint3(
                            geom.m_sBranches.m_pTangents[offset3_x], geom.m_sBranches.m_pTangents[offset3_y],
                            geom.m_sBranches.m_pTangents[offset3_z]
                        );
                        pbinormals[svert] = NiPoint3(
                            geom.m_sBranches.m_pBinormals[offset3_x], geom.m_sBranches.m_pBinormals[offset3_y],
                            geom.m_sBranches.m_pBinormals[offset3_z]
                        );
                    }
                }
                uint32_t aiCount_2 = vertsUsed.m_usSize;
                D3DXVECTOR4* pwindinfo = static_cast<D3DXVECTOR4*>(
                    ourMalloc(aiCount_2 <= SIZE_MAX / sizeof(D3DXVECTOR4) ? sizeof(D3DXVECTOR4) * aiCount_2 : SIZE_MAX)
                );
                if (pwindinfo) {
                    rt::eh_vector_constructor_iterator(
                        pwindinfo, sizeof(D3DXVECTOR4), aiCount_2, &emptyPointConstructor
                    );
                }
                for (svert = 0;; ++svert) {
                    uint32_t svert_4 = svert;
                    if (svert_4 >= vertsUsed.m_usSize) {
                        break;
                    }
                    uint32_t vertIndex_3 = vertsUsed.operator[](svert);
                    float fcolor = static_cast<double>((geom.m_sBranches.m_pColors[vertIndex_3] & 0xFF00) >> 8) / 255.0;
                    NiColorA kWindInfo(
                        geom.m_sBranches.m_pWindWeights[vertIndex_3],
                        static_cast<float>(4 * geom.m_sBranches.m_pWindMatrixIndices[vertIndex_3]), fcolor, 0.0f
                    );
                    NiColorA* pkWindInfo = &kWindInfo;
                    // The NiColorA is reinterpreted as a D3DXVECTOR4, copying the colour's four components into
                    // the vector.
                    pwindinfo[svert] = std::bit_cast<D3DXVECTOR4>(*pkWindInfo);
                }
                STSPData* v43 = static_cast<STSPData*>(NiMemObject::operator new(sizeof(STSPData)));
                if (v43) {
                    STSPData_ = ::new (v43) STSPData;
                }
                else {
                    STSPData_ = nullptr;
                }
                NiPointer<STSPData> spshaderData(STSPData_);
                uint16_t asVertexCount = vertsUsed.m_usSize;
                spshaderData.m_pObject->SetData(pwindinfo, asVertexCount);
                NiTriStripsData* v42 =
                    static_cast<NiTriStripsData*>(NiMemObject::operator new(sizeof(NiTriStripsData)));
                if (v42) {
                    int32_t iTriangles = *geom.m_sBranches.m_pStripLengths - 2;
                    int32_t eNBTMethod =
                        btangentSpace ? NiGeometryData::NBT_METHOD_NDL : NiGeometryData::NBT_METHOD_NONE;
                    uint32_t usVertices = vertsUsed.m_usSize;
                    NiTriStripsData_ = ::new (v42) NiTriStripsData(
                        usVertices, pcoords, pnormals_2, nullptr, ptex, 1, eNBTMethod, iTriangles, 1,
                        pcompositeStripLength, pcompositeStrip
                    );
                }
                else {
                    NiTriStripsData_ = nullptr;
                }
                this->pspBranchData[slevel].operator=(NiTriStripsData_);
                this->pspBranchData[slevel].m_pObject->SetConsistency(NiGeometryData::STATIC);
                this->pspBranchData[slevel].m_pObject->m_ucKeepFlags = 17u;
                this->pspBranchData[slevel].m_pObject->SetCompressFlags(1u);
                SpeedTreeBranchShaderProperty* apMemory = static_cast<SpeedTreeBranchShaderProperty*>(
                    NiMemObject::operator new(sizeof(SpeedTreeBranchShaderProperty))
                );
                if (apMemory) {
                    SpeedTreeBranchShaderProperty_ =
                        ::new (apMemory) SpeedTreeBranchShaderProperty(spshaderData.m_pObject);
                }
                else {
                    SpeedTreeBranchShaderProperty_ = nullptr;
                }
                this->pspBranchShaderProperties[slevel].operator=(SpeedTreeBranchShaderProperty_);
            }
        }
        pSpeedTree->FreeBranchColors();
        geom.m_sBranches.m_pColors = nullptr;
        pSpeedTree->FreeBranchWindData();
        geom.m_sBranches.m_pWindWeights = nullptr;
        geom.m_sBranches.m_pWindMatrixIndices = nullptr;
        geom.Destroy();
    }
    else {
    LABEL_21:;
        geom.Destroy();
    }
}

void BSTreeModel::CreateLeafGeometry(TESObjectTREE* apTreeObj) {
    CSpeedTreeRT::STextures textures;
    uint16_t slevels;
    CSpeedTreeRT::SGeometry geom;
    uint16_t smaps;
    geom.Init_00B029F0();
    textures.~STextures();
    ::new (&textures) CSpeedTreeRT::STextures;
    const NiPoint3* pcoords = nullptr;
    const NiPoint3* pnormals = nullptr;
    {
        NiPointer<STSPData> spshaderData;
        bool bflip = false;
        if (!pSpeedTree) {
            goto LABEL_15;
        }
        if (!pSpeedTree->GetLeafLodSizeAdjustments()) {
            goto LABEL_15;
        }
        if (IsInstance()) {
            goto LABEL_15;
        }
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
        slevels = GetNumLeafLODLevels();
        if (slevels && (pSpeedTree->GetTextures(textures), (smaps = textures.m_uiLeafTextureCount) != 0)) {
            STLSPData* STLSPData_;
            uint16_t slevel;
            for (uint16_t smap = 0; smap < static_cast<int32_t>(smaps); ++smap) {
                for (uint16_t smap2 = smap + 1; smap2 < static_cast<int32_t>(smaps); ++smap2) {
                    if (BSstrcmp(textures.m_pLeafTextureFilenames[smap], textures.m_pLeafTextureFilenames[smap2])) {
                        bflip = true;
                    }
                }
            }
            this->pspLeafData = new NiPointer<NiTriShapeData>[slevels];
            this->pspLeafShaderProperties = new NiPointer<SpeedTreeLeafShaderProperty>[slevels];
            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                this->pspLeafData[slevel].operator=(static_cast<NiTriShapeData*>(nullptr));
                this->pspLeafShaderProperties[slevel].operator=(static_cast<SpeedTreeLeafShaderProperty*>(nullptr));
            }
            STLSPData* v71 = static_cast<STLSPData*>(NiMemObject::operator new(sizeof(STLSPData)));
            if (v71) {
                STLSPData_ = ::new (v71) STLSPData;
            }
            else {
                STLSPData_ = nullptr;
            }
            spLeafShaderData.operator=(STLSPData_);
            spLeafShaderData.m_pObject->SetCurveScalar(fCurveScalar);
            if ((pSpeedTree) && (pSpeedTree->m_pWindEngine)) {
                spLeafShaderData.m_pObject->SetRock(pSpeedTree->m_pWindEngine->m_fSpeedWindRockScalar);
                spLeafShaderData.m_pObject->SetRustle(pSpeedTree->m_pWindEngine->m_fSpeedWindRustleScalar);
                if (apTreeObj) {
                    spLeafShaderData.m_pObject->SetRockSpeed(apTreeObj->GetRockSpeed());
                    spLeafShaderData.m_pObject->SetRustleSpeed(apTreeObj->GetRustleSpeed());
                }
            }

            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                pSpeedTree->GetGeometry(geom, 4u, -1, -1, static_cast<int16_t>(slevel));
                uint16_t sleafs = geom.m_sLeaves0.m_usLeafCount;
                uint16_t sverts = 4 * geom.m_sLeaves0.m_usLeafCount;
                if (geom.m_sLeaves0.m_usLeafCount) {
                    SpeedTreeLeafShaderProperty* SpeedTreeLeafShaderProperty_;
                    STSPData* STSPData_;
                    NiTriShapeData* NiTriShapeData_;
                    pcoords = reinterpret_cast<const NiPoint3*>(geom.m_sLeaves0.m_pCenterCoords);
                    pnormals = reinterpret_cast<const NiPoint3*>(geom.m_sLeaves0.m_pNormals);
                    // new NiPoint3[sverts] (twice), new NiPoint2[sverts] and new D3DXVECTOR4[sverts]: the size is all
                    // ones when it overflows 32 bits, as new[] computes it, and each element gets the empty
                    // constructor.
                    NiPoint3* pcoordsCopy = static_cast<NiPoint3*>(
                        ourMalloc(sverts <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * sverts : SIZE_MAX)
                    );
                    if (pcoordsCopy) {
                        rt::eh_vector_constructor_iterator(
                            pcoordsCopy, sizeof(NiPoint3), sverts, &emptyPointConstructor
                        );
                    }
                    NiPoint3* pnormalsCopy = static_cast<NiPoint3*>(
                        ourMalloc(sverts <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * sverts : SIZE_MAX)
                    );
                    if (pnormalsCopy) {
                        rt::eh_vector_constructor_iterator(
                            pnormalsCopy, sizeof(NiPoint3), sverts, &emptyPointConstructor
                        );
                    }
                    NiPoint2* ptexCopy = static_cast<NiPoint2*>(
                        ourMalloc(sverts <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * sverts : SIZE_MAX)
                    );
                    if (ptexCopy) {
                        rt::eh_vector_constructor_iterator(ptexCopy, sizeof(NiPoint2), sverts, &emptyPointConstructor);
                    }
                    D3DXVECTOR4* pwindinfo = static_cast<D3DXVECTOR4*>(
                        ourMalloc(sverts <= SIZE_MAX / sizeof(D3DXVECTOR4) ? sizeof(D3DXVECTOR4) * sverts : SIZE_MAX)
                    );
                    if (pwindinfo) {
                        rt::eh_vector_constructor_iterator(
                            pwindinfo, sizeof(D3DXVECTOR4), sverts, &emptyPointConstructor
                        );
                    }
                    uint16_t* ptrilistCopy = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * sleafs));
                    uint16_t* ptrilistMove = ptrilistCopy;
                    for (uint16_t sleaf = 0; sleaf < static_cast<int32_t>(sleafs); ++sleaf) {
                        uint16_t svertStart = 4 * sleaf;
                        uint16_t striStart = 6 * sleaf;
                        // The leaf's centre goes into its fourth corner, and is then copied down corner by corner.
                        const NiPoint3* v4 = &pcoords[sleaf];
                        NiPoint3* v5 = &pcoordsCopy[svertStart + 3];
                        v5->x = v4->x;
                        v5->y = v4->y;
                        v5->z = v4->z;
                        NiPoint3* v6 = &pcoordsCopy[svertStart + 3];
                        NiPoint3* v7 = &pcoordsCopy[svertStart + 2];
                        v7->x = v6->x;
                        v7->y = v6->y;
                        v7->z = v6->z;
                        NiPoint3* v8 = &pcoordsCopy[svertStart + 2];
                        NiPoint3* v9 = &pcoordsCopy[svertStart + 1];
                        v9->x = v8->x;
                        v9->y = v8->y;
                        v9->z = v8->z;
                        NiPoint3* v10 = &pcoordsCopy[svertStart + 1];
                        NiPoint3* v11 = &pcoordsCopy[svertStart];
                        v11->x = v10->x;
                        v11->y = v10->y;
                        v11->z = v10->z;
                        const NiPoint3* v12 = &pnormals[sleaf];
                        NiPoint3* v13 = &pnormalsCopy[svertStart + 3];
                        v13->x = v12->x;
                        v13->y = v12->y;
                        v13->z = v12->z;
                        NiPoint3* v14 = &pnormalsCopy[svertStart + 3];
                        NiPoint3* v15 = &pnormalsCopy[svertStart + 2];
                        v15->x = v14->x;
                        v15->y = v14->y;
                        v15->z = v14->z;
                        NiPoint3* v16 = &pnormalsCopy[svertStart + 2];
                        NiPoint3* v17 = &pnormalsCopy[svertStart + 1];
                        v17->x = v16->x;
                        v17->y = v16->y;
                        v17->z = v16->z;
                        NiPoint3* v18 = &pnormalsCopy[svertStart + 1];
                        NiPoint3* v19 = &pnormalsCopy[svertStart];
                        v19->x = v18->x;
                        v19->y = v18->y;
                        v19->z = v18->z;
                        float fcolor = static_cast<double>((geom.m_sLeaves0.m_pColors[sleaf] & 0xFF00) >> 8) / 255.0;
                        if (fcolor >= 1.0) {
                            fcolor = 0.99f;
                        }
                        const float* ptex = geom.m_sLeaves0.m_pLeafMapTexCoords[sleaf];
                        for (uint16_t svert = 0; svert < 4u; ++svert) {
                            if (bflip) {
                                ptexCopy[svert + svertStart] = NiPoint2(ptex[2 * svert], ptex[2 * (3 - svert) + 1]);
                            }
                            else {
                                ptexCopy[svert + svertStart] = NiPoint2(ptex[2 * svert], ptex[2 * svert + 1]);
                            }
                            float fcolorclusterindex =
                                static_cast<float>((svert + 2) % 4 + 4 * geom.m_sLeaves0.m_pLeafClusterIndices[sleaf]);
                            float fB_1 = fcolorclusterindex + fcolor;
                            const float* LeafLodSizeAdjustments = pSpeedTree->GetLeafLodSizeAdjustments();
                            NiColorA kWindInfo(
                                geom.m_sLeaves0.m_pWindWeights[sleaf],
                                static_cast<float>(4 * geom.m_sLeaves0.m_pWindMatrixIndices[sleaf]), fB_1,
                                LeafLodSizeAdjustments[slevel]
                            );
                            NiColorA* pkWindInfo = &kWindInfo;
                            // The NiColorA is reinterpreted as a D3DXVECTOR4, copying the colour's four components into
                            // the vector.
                            pwindinfo[svert + svertStart] = std::bit_cast<D3DXVECTOR4>(*pkWindInfo);
                        }
                        ptrilistMove[striStart] = svertStart + 3;
                        ptrilistMove[striStart + 1] = svertStart + 1;
                        ptrilistMove[striStart + 2] = svertStart + 2;
                        ptrilistMove[striStart + 3] = svertStart;
                        ptrilistMove[striStart + 4] = svertStart + 1;
                        ptrilistMove[striStart + 5] = svertStart + 3;
                    }
                    NiTriShapeData* v55 =
                        static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
                    if (v55) {
                        NiTriShapeData_ = ::new (v55) NiTriShapeData(
                            sverts, pcoordsCopy, pnormalsCopy, nullptr, ptexCopy, 1, NiGeometryData::NBT_METHOD_NONE,
                            2 * sleafs, ptrilistCopy
                        );
                    }
                    else {
                        NiTriShapeData_ = nullptr;
                    }
                    this->pspLeafData[slevel].operator=(NiTriShapeData_);
                    STSPData* v54 = static_cast<STSPData*>(NiMemObject::operator new(sizeof(STSPData)));
                    if (v54) {
                        STSPData_ = ::new (v54) STSPData;
                    }
                    else {
                        STSPData_ = nullptr;
                    }
                    spshaderData.operator=(STSPData_);
                    spshaderData.m_pObject->SetData(pwindinfo, sverts);
                    SpeedTreeLeafShaderProperty* v53 = static_cast<SpeedTreeLeafShaderProperty*>(
                        NiMemObject::operator new(sizeof(SpeedTreeLeafShaderProperty))
                    );
                    if (v53) {
                        STLSPData* pLeafData = spLeafShaderData.m_pObject;
                        STSPData* pShaderData = spshaderData.m_pObject;
                        SpeedTreeLeafShaderProperty_ =
                            ::new (v53) SpeedTreeLeafShaderProperty(slevel, pShaderData, pLeafData);
                    }
                    else {
                        SpeedTreeLeafShaderProperty_ = nullptr;
                    }
                    this->pspLeafShaderProperties[slevel].operator=(SpeedTreeLeafShaderProperty_);
                    SpeedTreeLeafShaderProperty* v41 = this->pspLeafShaderProperties[slevel].m_pObject;
                    v41->SetBaseTexture(spLeafFrondTexture.m_pObject);
                    SpeedTreeLeafShaderProperty* v40 = this->pspLeafShaderProperties[slevel].m_pObject;
                    NiAdditionalGeometryData* apAdditionalGeomData = v40->CreateRendererSpecificProperty(nullptr);
                    this->pspLeafData[slevel].m_pObject->SetAdditionalGeometryData(apAdditionalGeomData);
                    this->pspLeafData[slevel].m_pObject->SetConsistency(NiGeometryData::STATIC);
                    this->pspLeafData[slevel].m_pObject->m_ucKeepFlags = 17u;
                    this->pspLeafData[slevel].m_pObject->SetCompressFlags(1u);
                }
            }
            pSpeedTree->FreeLeafLODDataArrays();
        }
    LABEL_15:;
    }
    geom.Destroy();
}

void BSTreeModel::CreateBillboardGeometry(TESObjectTREE* apTreeObj) {
    float pDirection[3];
    CSpeedTreeRT::SGeometry v16;
    float fcameraPos[3];
    v16.Init_00B029F0();
    fcameraPos[0] = 0.0f;
    fcameraPos[1] = 0.0f;
    fcameraPos[2] = 0.0f;
    pDirection[0] = 0.0f;
    pDirection[1] = -1.0f;
    pDirection[2] = 0.0f;
    if (pSpeedTree && (!IsInstance() && apTreeObj)) {
        ShadowLightShader* pslshader;
        SpeedTreeBillboardShaderProperty* pbillboardProp;
        NiTriShape* v10;
        // Filled by GetCamera.
        float fcameraDirOld[3];
        float fcameraPosOld[3];
        CSpeedTreeRT::GetCamera(fcameraPosOld, fcameraDirOld);
        CSpeedTreeRT::SetCamera(fcameraPos, pDirection);
        pSpeedTree->SetLodLevel(0.0f);
        NiPointer<NiTriShapeData> spBillboardData;
        apTreeObj->CreateBillboardGeomData(false, spBillboardData);
        spBillboardData.m_pObject->SetConsistency(NiGeometryData::STATIC);
        spBillboardData.m_pObject->m_ucKeepFlags = 17u;
        spBillboardData.m_pObject->SetCompressFlags(1u);
        NiTriShape* apMemory = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
        if (apMemory) {
            v10 = ::new (apMemory) NiTriShape(spBillboardData.m_pObject);
        }
        else {
            v10 = nullptr;
        }
        spBillboard.operator=(v10);
        NiPointer<NiAlphaProperty> ppropAlpha(BSTreeModel::CreateAlphaProperty());
        if (ppropAlpha.m_pObject) {
            spBillboard.m_pObject->AttachProperty(ppropAlpha.m_pObject);
        }
        SpeedTreeBillboardShaderProperty* v12 = static_cast<SpeedTreeBillboardShaderProperty*>(
            NiMemObject::operator new(sizeof(SpeedTreeBillboardShaderProperty))
        );
        if (v12) {
            pbillboardProp = ::new (v12) SpeedTreeBillboardShaderProperty;
        }
        else {
            pbillboardProp = nullptr;
        }
        spBillboard.m_pObject->AttachProperty(pbillboardProp);
        BSShaderTextureSet* a2 = spBillboardTextureSet.m_pObject;
        pbillboardProp->LoadTextureSet("STBB", a2);
        NiSourceTexture* DefNormalMap = BSShader::GetDefNormalMap();
        pbillboardProp->SetNormalTexture(0, DefNormalMap);
        pbillboardProp->SetClampNI(NiTexturingProperty::CLAMP_S_CLAMP_T);
        pbillboardProp->SetFlag(13u, 1);
        pbillboardProp->SetFlag(22u, 1);
        BSShaderManager::PrepareObject(spBillboard.m_pObject, 0, 0);
        BSShader* pshade = spBillboard.m_pObject->GetShader();
        if (pshade) {
            // Shader types 1 to 5 are the ShadowLightShader family; the type getter is called twice.
            if (pshade->GetShaderType() < BSShaderManager::BSSM_SHADERTYPE_SHADOWLIGHT_FIRST ||
                pshade->GetShaderType() > BSShaderManager::BSSM_SHADERTYPE_SHADOWLIGHT_LAST) {
                pslshader = nullptr;
            }
            else {
                pslshader = static_cast<ShadowLightShader*>(pshade);
            }
        }
        else {
            pslshader = nullptr;
        }
        if (pslshader) {
            NiRenderer* Renderer_ = NiRenderer::GetRenderer();
            NiDX9ShaderDeclaration* pDecl = pslshader->GetDefaultShaderDecl();
            Renderer_->PrecacheGeometry(spBillboard.m_pObject, 0, 0, pDecl);
            NiDX9Renderer* v4 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
            v4->PerformPrecache();
        }
        CSpeedTreeRT::SetCamera(fcameraPosOld, fcameraDirOld);
    }
    v16.Destroy();
}

bool BSTreeModel::InitAsInstance(BSTreeModel* apInstanceFrom) {
    BSTreeModel::kCloneLock.Enter(nullptr);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREESYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp", 1041
    );
    if (apInstanceFrom &&
        (apInstanceFrom->pSpeedTree &&
         (!apInstanceFrom->IsInstance() &&
          (eType == BSTreeModel::BTMT_NONE && (pSpeedTree = apInstanceFrom->pSpeedTree->MakeInstance()) != nullptr)))) {
        uint16_t slevel;
        spTreeParent.operator=(apInstanceFrom);
        eType = BSTreeModel::BTMT_INSTANCE;
        spBranchTextureSet.operator=(apInstanceFrom->spBranchTextureSet);
        spLeafFrondTexture.operator=(apInstanceFrom->spLeafFrondTexture);
        spBillboardTextureSet.operator=(apInstanceFrom->spBillboardTextureSet);
        spLeafShaderData.operator=(apInstanceFrom->spLeafShaderData);
        fCurveScalar = apInstanceFrom->fCurveScalar;
        iSeed = apInstanceFrom->iSeed;
        fTrunkHeight = apInstanceFrom->fTrunkHeight;
        fTrunkWidth = apInstanceFrom->fTrunkWidth;
        if (apInstanceFrom->spRigidBody.m_pObject) {
            BSCloningProcess cloneData(1.0f);
            spRigidBody.operator=(static_cast<bhkRigidBody*>(apInstanceFrom->spRigidBody.m_pObject->Clone(cloneData)));
        }
        else {
            spRigidBody.operator=(static_cast<bhkRigidBody*>(nullptr));
        }
        uint16_t slevels = GetNumBranchLODLevels();
        if ((slevels) && (apInstanceFrom->pspBranchData) && (apInstanceFrom->pspBranchShaderProperties)) {
            // new NiPointer<NiTriStripsData>[slevels] and new
            // NiPointer<SpeedTreeBranchShaderProperty>[slevels]: a count cookie, then the elements.
            NiPointer<NiTriStripsData>* pNewData = nullptr;
            size_t* pDataBlock =
                static_cast<size_t*>(ourMalloc(sizeof(size_t) + slevels * sizeof(NiPointer<NiTriStripsData>)));
            if (pDataBlock) {
                // new[]'s cookie: the element count stored just before the array.
                pDataBlock[0] = slevels;
                pNewData = ::new (static_cast<void*>(pDataBlock + 1)) NiPointer<NiTriStripsData>[slevels];
            }
            pspBranchData = pNewData;
            NiPointer<SpeedTreeBranchShaderProperty>* pNewProperties = nullptr;
            size_t* pPropertyBlock = static_cast<size_t*>(
                ourMalloc(sizeof(size_t) + slevels * sizeof(NiPointer<SpeedTreeBranchShaderProperty>))
            );
            if (pPropertyBlock) {
                // new[]'s cookie: the element count stored just before the array.
                pPropertyBlock[0] = slevels;
                pNewProperties =
                    ::new (static_cast<void*>(pPropertyBlock + 1)) NiPointer<SpeedTreeBranchShaderProperty>[slevels];
            }
            pspBranchShaderProperties = pNewProperties;
            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                pspBranchData[slevel].operator=(apInstanceFrom->pspBranchData[slevel]);
                if (apInstanceFrom->pspBranchShaderProperties[slevel].m_pObject) {
                    SpeedTreeBranchShaderProperty* a2 = static_cast<SpeedTreeBranchShaderProperty*>(
                        apInstanceFrom->pspBranchShaderProperties[slevel].m_pObject->Clone()
                    );
                    pspBranchShaderProperties[slevel].operator=(a2);
                }
                else {
                    pspBranchShaderProperties[slevel].operator=(static_cast<SpeedTreeBranchShaderProperty*>(nullptr));
                }
            }
        }

        slevels = GetNumLeafLODLevels();
        if ((slevels) && (apInstanceFrom->pspLeafData) && (apInstanceFrom->pspLeafShaderProperties)) {
            // new NiPointer<NiTriShapeData>[slevels] and new NiPointer<SpeedTreeLeafShaderProperty>[slevels]: a
            // count cookie, then the elements.
            NiPointer<NiTriShapeData>* pNewData = nullptr;
            size_t* pDataBlock =
                static_cast<size_t*>(ourMalloc(sizeof(size_t) + slevels * sizeof(NiPointer<NiTriShapeData>)));
            if (pDataBlock) {
                // new[]'s cookie: the element count stored just before the array.
                pDataBlock[0] = slevels;
                pNewData = ::new (static_cast<void*>(pDataBlock + 1)) NiPointer<NiTriShapeData>[slevels];
            }
            pspLeafData = pNewData;
            NiPointer<SpeedTreeLeafShaderProperty>* pNewProperties = nullptr;
            size_t* pPropertyBlock = static_cast<size_t*>(
                ourMalloc(sizeof(size_t) + slevels * sizeof(NiPointer<SpeedTreeLeafShaderProperty>))
            );
            if (pPropertyBlock) {
                // new[]'s cookie: the element count stored just before the array.
                pPropertyBlock[0] = slevels;
                pNewProperties =
                    ::new (static_cast<void*>(pPropertyBlock + 1)) NiPointer<SpeedTreeLeafShaderProperty>[slevels];
            }
            pspLeafShaderProperties = pNewProperties;
            for (slevel = 0; slevel < static_cast<int32_t>(slevels); ++slevel) {
                pspLeafData[slevel].operator=(apInstanceFrom->pspLeafData[slevel]);
                if (apInstanceFrom->pspLeafShaderProperties[slevel].m_pObject) {
                    SpeedTreeLeafShaderProperty* a2a = static_cast<SpeedTreeLeafShaderProperty*>(
                        apInstanceFrom->pspLeafShaderProperties[slevel].m_pObject->Clone()
                    );
                    pspLeafShaderProperties[slevel].operator=(a2a);
                }
                else {
                    pspLeafShaderProperties[slevel].operator=(static_cast<SpeedTreeLeafShaderProperty*>(nullptr));
                }
            }
        }

        if (apInstanceFrom->spBillboard.m_pObject) {
            NiAlphaProperty::abCloneUnique = 1;
            spBillboard.operator=(static_cast<NiTriShape*>(apInstanceFrom->spBillboard.m_pObject->Clone()));
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
    CSpeedTreeRT::STextures textures;
    char pFilename[268];
    if (!apTreeObj || !apTreeObj->GetModel() || !*apTreeObj->GetModel() || eType) {
        return true;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_TREESYSTEM, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\SpeedTree\\BSTreeModel.cpp", 1139
    );
    BSstrcpy(pFilename, 260, "Trees");
    const char* v3 = apTreeObj->GetModel();
    BSstrcat(pFilename, 260, v3);
    CSpeedTreeRT* v11 = static_cast<CSpeedTreeRT*>(ourMalloc(sizeof(CSpeedTreeRT)));
    if (v11) {
        pspeedtree = ::new (v11) CSpeedTreeRT;
    }
    else {
        pspeedtree = nullptr;
    }
    if (pspeedtree->LoadTree(pFilename)) {
        float fsizeVar;
        float fsize;
        textures.~STextures();
        ::new (&textures) CSpeedTreeRT::STextures;
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
        pspeedtree->GetTreeSize(fsize, fsizeVar);
        float fNewVariance = GameSetting::fTreeSizeConversion::Get() * fsizeVar;
        float fNewSize = GameSetting::fTreeSizeConversion::Get() * fsize;
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
        float a4 = BSTreeManager::GetFarDistance();
        float a3 = BSTreeManager::GetNearDistance();
        pspeedtree->SetLodLimits(a3, a4);
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
    NiAlphaProperty* v1;
    void* v2 = NiMemObject::operator new(sizeof(NiAlphaProperty));
    if (v2) {
        v1 = ::new (v2) NiAlphaProperty;
    }
    else {
        v1 = nullptr;
    }
    v1->SetAlphaBlending(false);
    v1->SetAlphaTesting(true);
    v1->SetTestMode(NiAlphaProperty::TEST_GREATER);
    v1->SetTestRef(0x54u);
    return v1;
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
