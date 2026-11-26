#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"

#include "BSCore/BSMap.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSDistantTreeShader.h"
#include "BSShader/BSDistantTreeShaderProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/string.h"

#include <limits>
#include <new>

NiPointer<NiSourceTexture> BGSDistantTreeBlock::spAtlasTexture;
NiPointer<NiStencilProperty> BGSDistantTreeBlock::spSharedStencil;
NiPointer<NiAlphaProperty> BGSDistantTreeBlock::spSharedAlpha;
BGSDistantTreeBlock::TreeTypes_t BGSDistantTreeBlock::TreeTypes;
SettingT<INISettingCollection>
    uDistantTreeBlockCacheSizePerCell("uDistantTreeBlockCacheSizePerCell:TerrainManager", 1u);

using BSSimpleArray_BGSDistantTreeBlock_InstanceData_1024 = BSSimpleArray<BGSDistantTreeBlock::InstanceData, 1024>;

void FormatTreeDTL(char* apPath, TESWorldSpace* apWorld, int32_t aiLevel, int16_t aiX, int16_t aiY) {
    const char* pcWorldName = apWorld->GetFormEditorID();
    BSsprintf(
        apPath,
        260u,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Trees\\%s.Level%i.X%i.Y%i.DTL",
        apWorld->GetFormEditorID(),
        pcWorldName,
        aiLevel,
        aiX,
        aiY
    );
}

BGSDistantTreeBlock::TreeType::TreeType() : spSharedModelData(nullptr) {}

BGSDistantTreeBlock::TreeType::~TreeType() {}

BGSDistantTreeBlock::TreeType* BGSDistantTreeBlock::TreeType::Equal(const BGSDistantTreeBlock::TreeType* apTreeType) {
    uiNumber = apTreeType->uiNumber;
    kSize = apTreeType->kSize;
    kPosMin = apTreeType->kPosMin;
    kPosMax = apTreeType->kPosMax;
    spSharedModelData = apTreeType->spSharedModelData;
    return this;
}

void BGSDistantTreeBlock::TreeType::InitValues(BGSDistantTreeBlock::TreeType* apTreeType) {
    NiTriShapeData* pshapeData;
    NiAdditionalGeometryData* additionalGeo;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp",
        149
    );
    // The triangle count, compared unsigned and passed on as a count.
    uint32_t uiTriangleCount = 4 * BGSDistantTreeBlock::GetMaxInstanceCount();
    int32_t iMaxInstanceCount = BGSDistantTreeBlock::GetMaxInstanceCount();
    uint32_t uiVertexCount = 8 * iMaxInstanceCount;
    NiPoint3* pkVertex = new NiPoint3[uiVertexCount];
    NiPoint2* pkTexture = new NiPoint2[uiVertexCount];
    NiPoint3* pkNormal = new NiPoint3[uiVertexCount];
    uint16_t* pusTriList = static_cast<uint16_t*>(ourMalloc(
        3 * uiTriangleCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (3 * uiTriangleCount) : SIZE_MAX
    ));
    float fhalfWidth = apTreeType->kSize.x * 0.5;
    float fheight = apTreeType->kSize.y;
    float fminU = apTreeType->kPosMin.x;
    float fminV = apTreeType->kPosMin.y;
    float fmaxU = apTreeType->kPosMax.x;
    float fmaxV = apTreeType->kPosMax.y;
    NiSqrt(2.0f);
    for (uint32_t i = 0; i < uiVertexCount; i += 8) {
        pkVertex[i].x = -fhalfWidth;
        pkVertex[i].y = 0.0f;
        pkVertex[i].z = fheight;
        pkTexture[i].x = fminU;
        pkTexture[i].y = fminV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 1].x = fhalfWidth;
        pkVertex[i + 1].y = 0.0f;
        pkVertex[i + 1].z = fheight;
        pkTexture[i + 1].x = fmaxU;
        pkTexture[i + 1].y = fminV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 2].x = -fhalfWidth;
        pkVertex[i + 2].y = 0.0f;
        pkVertex[i + 2].z = 0.0f;
        pkTexture[i + 2].x = fminU;
        pkTexture[i + 2].y = fmaxV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 3].x = fhalfWidth;
        pkVertex[i + 3].y = 0.0f;
        pkVertex[i + 3].z = 0.0f;
        pkTexture[i + 3].x = fmaxU;
        pkTexture[i + 3].y = fmaxV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 4].x = 0.0f;
        pkVertex[i + 4].y = -fhalfWidth;
        pkVertex[i + 4].z = fheight;
        pkTexture[i + 4].x = fminU;
        pkTexture[i + 4].y = fminV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 5].x = 0.0f;
        pkVertex[i + 5].y = fhalfWidth;
        pkVertex[i + 5].z = fheight;
        pkTexture[i + 5].x = fmaxU;
        pkTexture[i + 5].y = fminV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 6].x = 0.0f;
        pkVertex[i + 6].y = -fhalfWidth;
        pkVertex[i + 6].z = 0.0f;
        pkTexture[i + 6].x = fminU;
        pkTexture[i + 6].y = fmaxV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
        pkVertex[i + 7].x = 0.0f;
        pkVertex[i + 7].y = fhalfWidth;
        pkVertex[i + 7].z = 0.0f;
        pkTexture[i + 7].x = fmaxU;
        pkTexture[i + 7].y = fmaxV;
        pkNormal[i].x = -1.0f;
        pkNormal[i].y = 1.0f;
        pkNormal[i].z = 1.0f;
        pkNormal[i].Unitize();
    }
    uint16_t usVertex = 0;
    for (uint32_t j = 0; j < 3 * uiTriangleCount; j += 6) {
        pusTriList[j] = usVertex;
        pusTriList[j + 1] = usVertex + 1;
        pusTriList[j + 2] = usVertex + 2;
        pusTriList[j + 3] = usVertex + 1;
        pusTriList[j + 4] = usVertex + 3;
        pusTriList[j + 5] = usVertex + 2;
        usVertex += 4;
    }
    // Each vertex's instance index, as a float (k >> 3).
    float* pinstanceIndices = static_cast<float*>(
        ourMalloc(uiVertexCount <= SIZE_MAX / sizeof(float) ? sizeof(float) * uiVertexCount : SIZE_MAX)
    );
    for (uint32_t k = 0; k < uiVertexCount; ++k) {
        pinstanceIndices[k] = static_cast<float>(k >> 3);
    }
    additionalGeo = new NiAdditionalGeometryData(uiVertexCount);
    additionalGeo->SetDataBlockCount(1u);
    additionalGeo->SetDataStreamCount(1u);
    // The data block is raw bytes to the geometry data: the float buffer goes in as uint8_t*.
    additionalGeo->SetDataBlock(0, reinterpret_cast<uint8_t*>(pinstanceIndices), 4 * uiVertexCount, true);
    additionalGeo->SetDataStream(0, 0, 0, 1u, uiVertexCount, 4u, 4u);
    ourFree(pinstanceIndices);
    pshapeData = new NiTriShapeData(
        uiVertexCount,
        pkVertex,
        pkNormal,
        nullptr,
        pkTexture,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        uiTriangleCount,
        pusTriList
    );
    NiPointer<NiTriShapeData> spshapeData(pshapeData);
    spshapeData->SetAdditionalGeometryData(additionalGeo);
    apTreeType->spSharedModelData = spshapeData;
    spshapeData = static_cast<NiTriShapeData*>(nullptr);
}

void BGSDistantTreeBlock::TreeType::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSDistantTreeBlock::TreeGroup::TreeGroup() : spGeometry(nullptr) {}

BGSDistantTreeBlock::TreeGroup::~TreeGroup() {}

void BGSDistantTreeBlock::TreeGroup::Free() {}

void BGSDistantTreeBlock::TreeGroup::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Sets up the block's containers, registers the block as a LOD level's, and queues the task that loads its trees.
BGSDistantTreeBlock::BGSDistantTreeBlock(BGSTerrainNode* apNode, uint32_t auiLevel, int16_t asCellX, int16_t asCellY)
  : spLoadTask(nullptr), InstanceMap(0x25), NextGroup(0x25), pNode(apNode), bDoneLoading(false), bAttached(false) {
    AutoMemContext kContext(MEM_CONTEXT_LODTREES);
    BGSDistantTreeBlock::uiLevel = auiLevel;
    BGSDistantTreeBlock::iCount++;
    TESWorldSpace* pWorld = apNode->GetManager()->GetWorld();
    spLoadTask = new BGSDistantTreeBlockLoadTask(this, pWorld, asCellX, asCellY, auiLevel, false);
    spLoadTask->QueueMe();
}

BGSDistantTreeBlock::~BGSDistantTreeBlock() {
    --BGSDistantTreeBlock::iCount;
    ClearTrees();
    ClearQueuedTrees();
    if (spLoadTask.m_pObject) {
        pIOManager->CancelTask(spLoadTask.m_pObject, nullptr);
    }
}

void BGSDistantTreeBlock::ShowTree(uint32_t aID) {
    BGSDistantTreeBlock::InstanceData* pinst = nullptr;
    if (InstanceMap.GetAt(aID, pinst)) {
        pinst->bHidden = false;
        // GetAt takes an int; the index is stored unsigned.
        TreeGroups.GetAt(static_cast<int32_t>(pinst->uiGroup))->bShaderPropertyUpToDate = false;
    }
}

void BGSDistantTreeBlock::SetupShaderProperty(
    BSDistantTreeShaderProperty* apProp,
    BGSDistantTreeBlock::TreeGroup& aGroup
) {
    uint32_t uiMaxInstances = BGSDistantTreeBlock::GetMaxInstanceCount();
    uint32_t inum = BSMIN(aGroup.Instances.iSize, uiMaxInstances);
    for (uint32_t i = 0; i < inum; ++i) {
        BGSDistantTreeBlock::InstanceData* pinst = &aGroup.Instances.GetAt(i);
        float fScale = pinst->bHidden ? 0.0f : pinst->fScale;
        NiPoint3 pos = pinst->Pos;
        apProp->SetInstanceData(i, pos, pinst->fRotZ, fScale);
    }
    for (uint32_t i = inum; i < BGSDistantTreeBlock::GetMaxInstanceCount(); ++i) {
        NiPoint3 pos;
        pos.z = 0.0f;
        pos.y = 0.0f;
        pos.x = 0.0f;
        apProp->SetInstanceData(i, pos, 0.0f, 0.0f);
    }
    aGroup.bShaderPropertyUpToDate = true;
}

void BGSDistantTreeBlock::ClearQueuedTrees() {
    TreesQueuedToHide.Clear(true);
}

void BGSDistantTreeBlock::HideTree(const uint32_t aID, bool abQueue) {
    BGSDistantTreeBlock::InstanceData* pinst = nullptr;
    if (InstanceMap.GetAt(aID, pinst)) {
        pinst->bHidden = true;
        TreeGroups.GetAt(pinst->uiGroup)->bShaderPropertyUpToDate = false;
    }
    else if (abQueue) {
        TreesQueuedToHide.Add(aID);
    }
}

void BGSDistantTreeBlock::FinishLoad() {
    bDoneLoading = true;
    pNode->GetManager()->SetNeedsImmediateUpdate(true);
    spLoadTask = static_cast<QueuedFile*>(nullptr);
}

void BGSDistantTreeBlock::AddTree(const uint32_t uiType, const BGSDistantTreeBlock::InstanceData& apInstanceData) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp",
        331
    );
    BGSDistantTreeBlock::TreeGroup* pgroup = nullptr;
    bool bfound = NextGroup.GetAt(uiType, pgroup);
    if (!bfound || !pgroup || pgroup->Instances.iSize == BGSDistantTreeBlock::GetMaxInstanceCount()) {
        if (uiType >= BGSDistantTreeBlock::TreeTypes.iSize) {
            Warning("Invalid LOD tree type %i found. TreeTypes file may be out of date or corrupt.", uiType);
            return;
        }
        pgroup = new BGSDistantTreeBlock::TreeGroup;
        pgroup->pTreeType = &BGSDistantTreeBlock::TreeTypes.GetAt(uiType);
        TreeGroups.Add(pgroup);
        NextGroup.SetAt(uiType, pgroup);
        pgroup->uiNum = TreeGroups.iSize - 1;
        pgroup->Instances.SetReservedSize(BGSDistantTreeBlock::GetMaxInstanceCount());
        pgroup->bShaderPropertyUpToDate = false;
    }
    pgroup->Instances.Add(apInstanceData);
    uint32_t uinum = pgroup->Instances.iSize - 1;
    BGSDistantTreeBlock::InstanceData* pinst = &pgroup->Instances.GetAt(uinum);
    pinst->uiGroup = pgroup->uiNum;
    pinst->bHidden = false;
    InstanceMap.SetAt(pinst->id, pinst);
}

void BGSDistantTreeBlock::ClearTrees() {
    for (BGSDistantTreeBlock::TreeGroup* pGroup : TreeGroups) {
        pGroup->Instances.Clear(true);
        if (pGroup->spGeometry.m_pObject) {
            NiNode* pTreeNode = pNode->pManager->GetTreeNode();
            pTreeNode->DetachChild(pGroup->spGeometry.m_pObject);
            pGroup->spGeometry = static_cast<BSResizableTriShape*>(nullptr);
        }
        if (pGroup) {
            delete pGroup;
        }
    }
    TreeGroups.Clear(true);
}

NiBound* BGSDistantTreeBlock::CalculateBound(NiBound* apBound, BGSDistantTreeBlock::TreeGroup* apGroup) {
    NiBound kBound;
    NiPoint3 kMin(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    float flowest = -std::numeric_limits<float>::max();
    NiPoint3 kMax(flowest, flowest, flowest);
    uint32_t uiInstanceCount = apGroup->Instances.iSize;
    for (uint32_t i = 0; i < uiInstanceCount; ++i) {
        kMin.x = BSMIN(kMin.x, apGroup->Instances.GetAt(i).Pos.x);
        kMax.x = BSMAX(kMax.x, apGroup->Instances.GetAt(i).Pos.x);
        kMin.y = BSMIN(kMin.y, apGroup->Instances.GetAt(i).Pos.y);
        kMax.y = BSMAX(kMax.y, apGroup->Instances.GetAt(i).Pos.y);
        kMin.z = BSMIN(kMin.z, apGroup->Instances.GetAt(i).Pos.z);
        kMax.z = BSMAX(kMax.z, apGroup->Instances.GetAt(i).Pos.z);
    }
    NiPoint3 kCenter = kMin + (kMax - kMin) / 2.0f;
    BSResizableTriShape* pgeometry = apGroup->spGeometry.m_pObject;
    double dradius = pgeometry->GetWorldBound().m_fRadius;
    float fradius = (kMax - kCenter).Length() + dradius;
    kBound.SetCenterAndRadius(kCenter, fradius);
    *apBound = kBound;
    return apBound;
}

void BGSDistantTreeBlock::Read(NiFile* apInputFile) {
    BGSDistantTreeBlock::InstanceData instanceData;
    uint32_t uiType;
    uint32_t uiNumInstances;
    uint32_t uiNumTypes;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp",
        516
    );
    ClearTrees();
    apInputFile->Read(&uiNumTypes, 4u);
    for (uint32_t i = 0; i < uiNumTypes; ++i) {
        apInputFile->Read(&uiType, 4u);
        apInputFile->Read(&uiNumInstances, 4u);
        for (uint32_t j = 0; j < uiNumInstances; ++j) {
            ::new (&instanceData) NiBound;
            apInputFile->Read(&instanceData, 32u);
            AddTree(uiType, instanceData);
        }
    }
}

void BGSDistantTreeBlock::Attach() {
    NiBound kBound;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp",
        576
    );
    if (!bDoneLoading || bAttached) {
        if (bDoneLoading && bAttached) {
            HideQueuedTrees();
        }
    }
    else {
        for (uint32_t i = 0; i < TreeGroups.iSize; ++i) {
            BGSDistantTreeBlock::TreeGroup* group = TreeGroups.GetAt(static_cast<int32_t>(i));
            BSResizableTriShape* pnewGeometry = new BSResizableTriShape(
                TreeGroups.GetAt(static_cast<int32_t>(i))->pTreeType->spSharedModelData.m_pObject
            );
            group->spGeometry = pnewGeometry;
            group->spGeometry->uiNumPrimitives = 4 * group->Instances.iSize;
            BSDistantTreeShaderProperty* psp = new BSDistantTreeShaderProperty;
            SetupShaderProperty(psp, *group);
            group->spGeometry->AttachProperty(psp);
            BSShaderManager::PrepareObject(group->spGeometry.m_pObject, 0, 0);
            group->spGeometry->AttachProperty(BGSDistantTreeBlock::spSharedStencil.m_pObject);
            group->spGeometry->AttachProperty(BGSDistantTreeBlock::spSharedAlpha.m_pObject);
            NiNode* pTreeNode = pNode->pManager->GetTreeNode();
            pTreeNode->AttachChild(group->spGeometry.m_pObject, 1);
            group->spGeometry->UpdateProperties();
            NiUpdateData foobar(0.0f, false, false);
            group->spGeometry->Update(foobar);
            NiUpdateData bar(0.0f, false, false);
            pNode->pManager->GetLODRoot()->Update(bar);
            BSResizableTriShape* pgeometry = group->spGeometry.m_pObject;
            NiBound kLocalBound = *CalculateBound(&kBound, group);
            pgeometry->SetLocalBound(kLocalBound);
            NiUpdateData foo(0.0f, false, false);
            group->spGeometry->Update(foo);
        }
        bAttached = true;
    }
}

void BGSDistantTreeBlock::UpdateShaderProperties() {
    for (BGSDistantTreeBlock::TreeGroup* pgroup : TreeGroups) {
        if (!pgroup->bShaderPropertyUpToDate && pgroup->spGeometry.m_pObject) {
            int32_t itype = NiShadeProperty::GetType();
            BSDistantTreeShaderProperty* pprop = static_cast<BSDistantTreeShaderProperty*>(
                pgroup->spGeometry->GetProperty(itype)
            );
            if (pprop) {
                SetupShaderProperty(pprop, *pgroup);
            }
        }
    }
}

void BGSDistantTreeBlock::HideQueuedTrees() {
    for (uint32_t uiID : TreesQueuedToHide) {
        HideTree(uiID, false);
    }
    ClearQueuedTrees();
}

// Reads the world's TreeTypes.lst, loads the tree texture atlas (the swamp one for the DLC4 worlds) and creates the
// stencil and alpha properties every LOD tree shares.
void BGSDistantTreeBlock::AllocateStaticData(TESWorldSpace* apWorld) {
    char acTreeTypes[0x104];
    BSsprintf(
        acTreeTypes,
        sizeof(acTreeTypes),
        "Data\\Meshes\\Landscape\\LOD\\%s\\Trees\\TreeTypes.lst",
        apWorld->GetFormEditorID()
    );
    BSFile* pTreeTypesFile = static_cast<BSFile*>(FileFinder::GetFile(acTreeTypes, NiFile::READ_ONLY, 0x800));
    if (pTreeTypesFile) {
        ReadTypes(pTreeTypesFile);
        delete pTreeTypesFile;
    }
    const char* pTextureName = "TreeDeadLOD.dds";
    if (!std::strncmp(apWorld->GetFormEditorID(), "DLC4", 4)) {
        pTextureName = "TreeSwampLOD.dds";
    }
    char acPath[0x104];
    BSsprintf(acPath, sizeof(acPath), "\\Data\\Textures\\Landscape\\Trees\\%s", pTextureName);
    TES::pInstance->CreateTextureImage(acPath, spAtlasTexture, true, true);
    if (!spAtlasTexture) {
        Warning("LOD tree texture atlas not found!");
    }
    BSDistantTreeShader::SetTextureAtlas(spAtlasTexture);
    bStaticDataLoaded = true;

    NiStencilProperty* pStencil = new NiStencilProperty;
    spSharedStencil = pStencil;
    spSharedStencil->SetDrawMode(NiStencilProperty::DRAW_BOTH);

    NiAlphaProperty* pAlpha = new NiAlphaProperty;
    spSharedAlpha = pAlpha;
    spSharedAlpha->SetAlphaTesting(true);
    spSharedAlpha->SetAlphaBlending(false);
    spSharedAlpha->SetTestRef(0x80);
    spSharedAlpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
}

void BGSDistantTreeBlock::ReadTypes(NiFile* apInputFile) {
    uint32_t uinumTypes;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp",
        638
    );
    BGSDistantTreeBlock::TreeTypes.Clear(true);
    uinumTypes = 0;
    apInputFile->Read(&uinumTypes, 4u);
    for (uint32_t i = 0; i < uinumTypes; ++i) {
        BGSDistantTreeBlock::TreeType newType;
        apInputFile->Read(&newType, 32u);
        newType.spSharedModelData = static_cast<NiTriShapeData*>(nullptr);
        BGSDistantTreeBlock::AddType(newType);
    }
}

uint32_t BGSDistantTreeBlock::GetBlockCount() {
    return static_cast<uint32_t>(BGSDistantTreeBlock::iCount);
}

int32_t BGSDistantTreeBlock::GetMaxInstanceCount() {
    int32_t iCount = BSDistantTreeShader::uiMaxInstanceCount;
    if (BSDistantTreeShader::uiMaxInstanceCount) {
        return iCount;
    }
    iCount = BSShaderManager::eShaderLevel < 2 ? 40 : 114;
    BSDistantTreeShader::uiMaxInstanceCount = iCount;
    return iCount;
}

bool BGSDistantTreeBlock::QStaticDataLoaded() {
    return BGSDistantTreeBlock::bStaticDataLoaded;
}

void BGSDistantTreeBlock::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSDistantTreeBlock::DeallocateStaticData() {
    for (BGSDistantTreeBlock::TreeType& rType : BGSDistantTreeBlock::TreeTypes) {
        rType.spSharedModelData = static_cast<NiTriShapeData*>(nullptr);
    }
    BGSDistantTreeBlock::spSharedStencil = static_cast<NiStencilProperty*>(nullptr);
    BGSDistantTreeBlock::spSharedAlpha = static_cast<NiAlphaProperty*>(nullptr);
    BGSDistantTreeBlock::spAtlasTexture = static_cast<NiSourceTexture*>(nullptr);
    BSDistantTreeShader::SetTextureAtlas(nullptr);
    BGSDistantTreeBlock::bStaticDataLoaded = false;
}

void BGSDistantTreeBlock::AddType(const BGSDistantTreeBlock::TreeType& aType) {
    BGSDistantTreeBlock::TreeTypes.Add(aType);
    BGSDistantTreeBlock::TreeType::InitValues(&BGSDistantTreeBlock::TreeTypes.GetLast());
}

BGSDistantTreeBlockLoadTask::BGSDistantTreeBlockLoadTask(
    BGSDistantTreeBlock* apBlock,
    TESWorldSpace* apWorld,
    int32_t aiX,
    int32_t aiY,
    int32_t aiLevel,
    bool abPostApocalypse
)
  : QueuedFileEntry(IO_TASK_PRIORITY_MID), pBlock(apBlock), bFinishedQueuing(false), iX(aiX), iY(aiY), iLevel(aiLevel),
    bCanceled(false) {
    char cFilePath[268];
    FormatTreeDTL(cFilePath, apWorld, aiLevel, aiX, aiY);
    SetFileName(&cFilePath[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
    bFileExists = false;
}

BGSDistantTreeBlockLoadTask::~BGSDistantTreeBlockLoadTask() {}

void BGSDistantTreeBlockLoadTask::Run() {
    NiFile* pinputFile = FileFinder::GetFile(pFileName, NiFile::READ_ONLY, 2048u, 1u);
    if (!pinputFile || bCanceled) {
        bFileExists = false;
    }
    else {
        pBlock->Read(pinputFile);
        delete pinputFile;
        bFileExists = true;
        pBlock->HideQueuedTrees();
    }
}

void BGSDistantTreeBlockLoadTask::Finish() {
    if (bFileExists && !bCanceled) {
        pBlock->FinishLoad();
    }
}

void BGSDistantTreeBlockLoadTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    bCanceled = true;
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
}

bool BGSDistantTreeBlockLoadTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    // "Tree LOD Task for block ( %i, %i ) at level ( %i )"
    BSsprintf(apDescription, aiBufferSize, "Tree LOD Task for block ( %i, %i ) at level ( %i )", iX, iY, iLevel);
    return true;
}

void BGSDistantTreeBlockLoadTask::QueueMe() {
    bFinishedQueuing = true;
    CheckFinished();
}

// Once every child is done and the block finished queuing, a task still pending goes back to the IO manager
// (AddTask, slot 0x48); otherwise it finishes as a queued file.
void BGSDistantTreeBlockLoadTask::CheckFinished() {
    if (QueuedFile::GetAllChildrenFinished() && bFinishedQueuing) {
        if (IOTask::IsPending()) {
            pIOManager->AddTask(this);
        }
        else {
            QueuedFile::CheckFinished();
        }
    }
}

// A block in an archive loads on that archive's file index (its runtime ID past the first four); a loose one on
// index 1.
uint8_t BGSDistantTreeBlockLoadTask::GetFileIndex() {
    if (GetFileEntry()) {
        Archive* pArchive = ArchiveManager::GetArchiveForFileEntry(GetFileEntry(), ARCHIVE_TYPE_MESHES);
        if (pArchive) {
            return pArchive->GetRuntimeID() + 4;
        }
    }
    return 1;
}

void BGSDistantTreeBlockLoadTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}
