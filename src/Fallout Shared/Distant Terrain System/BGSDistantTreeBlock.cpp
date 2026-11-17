#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"

#include "BSCore/BSMap.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "SDK/MSVC/string.h"

#include <new>

NiPointer<NiSourceTexture> BGSDistantTreeBlock::spAtlasTexture;
NiPointer<NiStencilProperty> BGSDistantTreeBlock::spSharedStencil;
NiPointer<NiAlphaProperty> BGSDistantTreeBlock::spSharedAlpha;
BGSDistantTreeBlock::TreeTypes_t BGSDistantTreeBlock::TreeTypes;
SettingT<INISettingCollection>
    uDistantTreeBlockCacheSizePerCell("uDistantTreeBlockCacheSizePerCell:TerrainManager", 1u);

using BSSimpleArray_BGSDistantTreeBlock_InstanceData_1024 = BSSimpleArray<BGSDistantTreeBlock::InstanceData, 1024>;

void FormatTreeDTL(char* apPath, TESWorldSpace* apWorld, int32_t aiLevel, int16_t aiX, int16_t aiY) {
    const char* v5 = apWorld->GetFormEditorID();
    BSsprintf(
        apPath, 260u, "Data\\Meshes\\Landscape\\LOD\\%s\\Trees\\%s.Level%i.X%i.Y%i.DTL", apWorld->GetFormEditorID(), v5,
        aiLevel, aiX, aiY
    );
}

BGSDistantTreeBlock::TreeType::TreeType() : spSharedModelData(nullptr) {}

BGSDistantTreeBlock::TreeType::~TreeType() {}

BGSDistantTreeBlock::TreeType* BGSDistantTreeBlock::TreeType::Equal(const BGSDistantTreeBlock::TreeType* a2) {
    uiNumber = a2->uiNumber;
    kSize = a2->kSize;
    kPosMin = a2->kPosMin;
    kPosMax = a2->kPosMax;
    spSharedModelData.operator=(a2->spSharedModelData);
    return this;
}

void BGSDistantTreeBlock::TreeType::InitValues(BGSDistantTreeBlock::TreeType* apTreeType) {
    NiTriShapeData* v2;
    NiAdditionalGeometryData* additionalGeo;
    NiPoint3* v4;
    NiPoint2* v5;
    NiPoint3* v6;
    AutoMemContext apMemory(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp", 149
    );
    // The triangle count, compared unsigned and passed on as a count.
    uint32_t v18 = 4 * BGSDistantTreeBlock::GetMaxInstanceCount();
    int32_t MaxInstanceCount = BGSDistantTreeBlock::GetMaxInstanceCount();
    uint32_t v19 = 8 * MaxInstanceCount;
    // The vertex arrays' operator new[] overflow checks run on the vertex count v19.
    void* ptr = ourMalloc(v19 <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * v19 : SIZE_MAX);
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), v19, &emptyPointConstructor);
        v6 = static_cast<NiPoint3*>(ptr);
    }
    else {
        v6 = nullptr;
    }
    void* v10 = ourMalloc(v19 <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * v19 : SIZE_MAX);
    if (v10) {
        rt::eh_vector_constructor_iterator(v10, sizeof(NiPoint2), v19, &emptyPointConstructor);
        v5 = static_cast<NiPoint2*>(v10);
    }
    else {
        v5 = nullptr;
    }
    void* v9 = ourMalloc(v19 <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * v19 : SIZE_MAX);
    if (v9) {
        rt::eh_vector_constructor_iterator(v9, sizeof(NiPoint3), v19, &emptyPointConstructor);
        v4 = static_cast<NiPoint3*>(v9);
    }
    else {
        v4 = nullptr;
    }
    uint16_t* pusTriList = static_cast<uint16_t*>(
        ourMalloc(3 * v18 <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * (3 * v18) : SIZE_MAX)
    );
    float v32 = apTreeType->kSize.x * 0.5;
    float y = apTreeType->kSize.y;
    // Half the height is computed and never read, as the game does.
    [[maybe_unused]] float v24 = y * 0.5;
    float x = apTreeType->kPosMin.x;
    float v17 = apTreeType->kPosMin.y;
    float v16 = apTreeType->kPosMax.x;
    float v30 = apTreeType->kPosMax.y;
    NiSqrt(2.0f);
    for (uint32_t i = 0; i < v19; i += 8) {
        v6[i].x = -v32;
        v6[i].y = 0.0f;
        v6[i].z = y;
        v5[i].x = x;
        v5[i].y = v17;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 1].x = v32;
        v6[i + 1].y = 0.0f;
        v6[i + 1].z = y;
        v5[i + 1].x = v16;
        v5[i + 1].y = v17;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 2].x = -v32;
        v6[i + 2].y = 0.0f;
        v6[i + 2].z = 0.0f;
        v5[i + 2].x = x;
        v5[i + 2].y = v30;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 3].x = v32;
        v6[i + 3].y = 0.0f;
        v6[i + 3].z = 0.0f;
        v5[i + 3].x = v16;
        v5[i + 3].y = v30;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 4].x = 0.0f;
        v6[i + 4].y = -v32;
        v6[i + 4].z = y;
        v5[i + 4].x = x;
        v5[i + 4].y = v17;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 5].x = 0.0f;
        v6[i + 5].y = v32;
        v6[i + 5].z = y;
        v5[i + 5].x = v16;
        v5[i + 5].y = v17;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 6].x = 0.0f;
        v6[i + 6].y = -v32;
        v6[i + 6].z = 0.0f;
        v5[i + 6].x = x;
        v5[i + 6].y = v30;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
        v6[i + 7].x = 0.0f;
        v6[i + 7].y = v32;
        v6[i + 7].z = 0.0f;
        v5[i + 7].x = v16;
        v5[i + 7].y = v30;
        v4[i].x = -1.0f;
        v4[i].y = 1.0f;
        v4[i].z = 1.0f;
        v4[i].Unitize();
    }
    uint16_t v21 = 0;
    for (uint32_t j = 0; j < 3 * v18; j += 6) {
        pusTriList[j] = v21;
        pusTriList[j + 1] = v21 + 1;
        pusTriList[j + 2] = v21 + 2;
        pusTriList[j + 3] = v21 + 1;
        pusTriList[j + 4] = v21 + 3;
        pusTriList[j + 5] = v21 + 2;
        v21 += 4;
    }
    // Each vertex's instance index, as a float (k >> 3).
    float* a2 = static_cast<float*>(ourMalloc(v19 <= SIZE_MAX / sizeof(float) ? sizeof(float) * v19 : SIZE_MAX));
    for (uint32_t k = 0; k < v19; ++k) {
        a2[k] = static_cast<float>(k >> 3);
    }
    NiAdditionalGeometryData* v8 =
        static_cast<NiAdditionalGeometryData*>(NiMemObject::operator new(sizeof(NiAdditionalGeometryData)));
    if (v8) {
        additionalGeo = ::new (v8) NiAdditionalGeometryData(v19);
    }
    else {
        additionalGeo = nullptr;
    }
    additionalGeo->SetDataBlockCount(1u);
    additionalGeo->SetDataStreamCount(1u);
    // The data block is raw bytes to the geometry data: the float buffer goes in as uint8_t*.
    additionalGeo->SetDataBlock(0, reinterpret_cast<uint8_t*>(a2), 4 * v19, true);
    additionalGeo->SetDataStream(0, 0, 0, 1u, v19, 4u, 4u);
    ourFree(a2);
    NiTriShapeData* v7 = static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
    if (v7) {
        v2 = ::new (v7) NiTriShapeData(v19, v6, v4, nullptr, v5, 1, NiGeometryData::NBT_METHOD_NONE, v18, pusTriList);
    }
    else {
        v2 = nullptr;
    }
    NiPointer<NiTriShapeData> v22(v2);
    v22->SetAdditionalGeometryData(additionalGeo);
    apTreeType->spSharedModelData.operator=(v22);
    v22.operator=(static_cast<NiTriShapeData*>(nullptr));
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
    uint32_t a7 = BGSDistantTreeBlock::GetMaxInstanceCount();
    uint32_t FormFlags = aGroup.Instances.iSize;
    uint32_t inum = BSMIN(FormFlags, a7);
    for (uint32_t i = 0; i < inum; ++i) {
        NiPoint3 a3;
        float fScale;
        BGSDistantTreeBlock::InstanceData* inst = &aGroup.Instances.GetAt(i);
        if (inst->bHidden) {
            fScale = 0.0f;
        }
        else {
            fScale = inst->fScale;
        }
        a3.z = inst->Pos.z;
        a3.y = inst->Pos.y;
        a3.x = inst->Pos.x;
        apProp->SetInstanceData(i, a3, inst->fRotZ, fScale);
    }
    for (uint32_t i_2 = inum; i_2 < BGSDistantTreeBlock::GetMaxInstanceCount(); ++i_2) {
        NiPoint3 a3a;
        a3a.z = 0.0f;
        a3a.y = 0.0f;
        a3a.x = 0.0f;
        apProp->SetInstanceData(i_2, a3a, 0.0f, 0.0f);
    }
    aGroup.bShaderPropertyUpToDate = true;
}

void BGSDistantTreeBlock::ClearQueuedTrees() {
    (&TreesQueuedToHide)->Clear(true);
}

void BGSDistantTreeBlock::HideTree(const uint32_t aID, bool abQueue) {
    BGSDistantTreeBlock::InstanceData* pinst = nullptr;
    if (InstanceMap.GetAt(aID, pinst)) {
        pinst->bHidden = true;
        TreeGroups.GetAt(pinst->uiGroup)->bShaderPropertyUpToDate = false;
    }
    else {
        if (abQueue) {
            TreesQueuedToHide.Add(aID);
        }
    }
}

void BGSDistantTreeBlock::FinishLoad() {
    bDoneLoading = true;
    BGSTerrainManager* v1 = pNode->GetManager();
    v1->SetNeedsImmediateUpdate(true);
    spLoadTask.operator=(static_cast<QueuedFile*>(nullptr));
}

void BGSDistantTreeBlock::AddTree(const uint32_t uiType, const BGSDistantTreeBlock::InstanceData& apInstanceData) {
    uint32_t v3;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp", 331
    );
    BGSDistantTreeBlock::TreeGroup* pgroup = nullptr;
    bool bfound = NextGroup.GetAt(uiType, pgroup);
    if (!bfound || (!pgroup || (v3 = pgroup->Instances.iSize, v3 == BGSDistantTreeBlock::GetMaxInstanceCount()))) {
        BGSDistantTreeBlock::TreeGroup* pgroup_2;
        if (uiType >= BGSDistantTreeBlock::TreeTypes.iSize) {
            Warning("Invalid LOD tree type %i found. TreeTypes file may be out of date or corrupt.", uiType);
            return;
        }
        BGSDistantTreeBlock::TreeGroup* v9 =
            static_cast<BGSDistantTreeBlock::TreeGroup*>(ourMalloc(sizeof(BGSDistantTreeBlock::TreeGroup)));
        if (v9) {
            pgroup_2 = ::new (v9) BGSDistantTreeBlock::TreeGroup;
        }
        else {
            pgroup_2 = nullptr;
        }
        pgroup = pgroup_2;
        BGSDistantTreeBlock::TreeType* v4 = &BGSDistantTreeBlock::TreeTypes.GetAt(uiType);
        pgroup->pTreeType = v4;
        TreeGroups.Add(pgroup);
        NextGroup.SetAt(uiType, pgroup);
        uint32_t FormFlags = TreeGroups.iSize;
        pgroup->uiNum = FormFlags - 1;
        uint32_t MaxInstanceCount = BGSDistantTreeBlock::GetMaxInstanceCount();
        pgroup->Instances.SetReservedSize(MaxInstanceCount);
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
    for (uint32_t i = 0; i < TreeGroups.iSize; ++i) {
        TreeGroups.GetAt(i)->Instances.Clear(true);
        if (TreeGroups.GetAt(i)->spGeometry.m_pObject) {
            NiNode* TreeNode = pNode->pManager->GetTreeNode();
            TreeNode->DetachChild(TreeGroups.GetAt(i)->spGeometry.m_pObject);
            TreeGroups.GetAt(i)->spGeometry.operator=(static_cast<BSResizableTriShape*>(nullptr));
        }
        BGSDistantTreeBlock::TreeGroup* v3 = TreeGroups.GetAt(i);
        if (v3) {
            delete v3;
        }
    }
    TreeGroups.Clear(true);
}

NiBound* BGSDistantTreeBlock::CalculateBound(NiBound* apBound, BGSDistantTreeBlock::TreeGroup* apGroup) {
    NiBound apMemory;
    float v23 = 3.4028235e+38f;
    NiPoint3 kMin(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
    float a2 = -3.4028235e+38f;
    NiPoint3 kMax(a2, a2, a2);
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
    BSResizableTriShape* v16 = apGroup->spGeometry.m_pObject;
    const NiBound* v11 = &v16->GetWorldBound();
    double radius = v11->m_fRadius;
    float v24 = (kMax - kCenter).Length() + radius;
    apMemory.SetCenterAndRadius(kCenter, v24);
    *apBound = apMemory;
    return apBound;
}

void BGSDistantTreeBlock::Read(NiFile* apInputFile) {
    BGSDistantTreeBlock::InstanceData apInstanceData;
    uint32_t uiType;
    uint32_t v6;
    uint32_t pvBuffer;
    AutoMemContext apMemory(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp", 516
    );
    ClearTrees();
    apInputFile->Read(&pvBuffer, 4u);
    for (uint32_t i = 0; i < pvBuffer; ++i) {
        apInputFile->Read(&uiType, 4u);
        apInputFile->Read(&v6, 4u);
        for (uint32_t j = 0; j < v6; ++j) {
            ::new (&apInstanceData) NiBound;
            apInputFile->Read(&apInstanceData, 32u);
            AddTree(uiType, apInstanceData);
        }
    }
}

void BGSDistantTreeBlock::Attach() {
    NiBound arg0;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp", 576
    );
    if (!bDoneLoading || bAttached) {
        if (bDoneLoading && bAttached) {
            HideQueuedTrees();
        }
    }
    else {
        for (uint32_t i = 0;; ++i) {
            BSDistantTreeShaderProperty* psp_2;
            BSResizableTriShape* v9;
            uint32_t FormFlags = TreeGroups.iSize;
            if (i >= FormFlags) {
                break;
            }
            BGSDistantTreeBlock::TreeGroup* group = TreeGroups.GetAt(static_cast<int32_t>(i));
            BSResizableTriShape* v14 =
                static_cast<BSResizableTriShape*>(NiMemObject::operator new(sizeof(BSResizableTriShape)));
            if (v14) {
                NiTriShapeData* v4 = TreeGroups.GetAt(static_cast<int32_t>(i))->pTreeType->spSharedModelData.m_pObject;
                v9 = ::new (v14) BSResizableTriShape(v4);
            }
            else {
                v9 = nullptr;
            }
            NiRefObject* v15 = v9;
            group->spGeometry.operator=(v9);
            uint32_t v2 = group->Instances.iSize;
            group->spGeometry.m_pObject->uiNumPrimitives = 4 * v2;
            BSDistantTreeShaderProperty* v12 = static_cast<BSDistantTreeShaderProperty*>(
                NiMemObject::operator new(sizeof(BSDistantTreeShaderProperty))
            );
            if (v12) {
                psp_2 = ::new (v12) BSDistantTreeShaderProperty;
            }
            else {
                psp_2 = nullptr;
            }
            BSDistantTreeShaderProperty* v13 = psp_2;
            BSDistantTreeShaderProperty* psp = psp_2;
            SetupShaderProperty(psp_2, *group);
            group->spGeometry.m_pObject->AttachProperty(psp);
            BSShaderManager::PrepareObject(group->spGeometry.m_pObject, 0, 0);
            NiStencilProperty* psp_4 = BGSDistantTreeBlock::spSharedStencil.m_pObject;
            group->spGeometry.m_pObject->AttachProperty(psp_4);
            NiAlphaProperty* psp_5 = BGSDistantTreeBlock::spSharedAlpha.m_pObject;
            group->spGeometry.m_pObject->AttachProperty(psp_5);
            NiNode* TreeNode = pNode->pManager->GetTreeNode();
            TreeNode->AttachChild(group->spGeometry.m_pObject, 1);
            group->spGeometry.m_pObject->UpdateProperties();
            NiUpdateData foobar(0.0f, false, false);
            group->spGeometry.m_pObject->Update(foobar);
            NiUpdateData bar(0.0f, false, false);
            pNode->pManager->GetLODRoot()->Update(bar);
            BSResizableTriShape* v7 = group->spGeometry.m_pObject;
            NiBound v24 = *CalculateBound(&arg0, group);
            v7->SetLocalBound(v24);
            NiUpdateData foo(0.0f, false, false);
            group->spGeometry.m_pObject->Update(foo);
        }
        bAttached = true;
    }
}

void BGSDistantTreeBlock::UpdateShaderProperties() {
    for (uint32_t i = 0; i < TreeGroups.iSize; ++i) {
        BGSDistantTreeBlock::TreeGroup* pgroup = TreeGroups.GetAt(i);
        if ((!pgroup->bShaderPropertyUpToDate) && (pgroup->spGeometry.m_pObject)) {
            int32_t Type = NiShadeProperty::GetType();
            BSDistantTreeShaderProperty* pprop =
                static_cast<BSDistantTreeShaderProperty*>(pgroup->spGeometry.m_pObject->GetProperty(Type));
            if (pprop) {
                SetupShaderProperty(pprop, *pgroup);
            }
        }
    }
}

void BGSDistantTreeBlock::HideQueuedTrees() {
    for (uint32_t i = 0; i < TreesQueuedToHide.iSize; ++i) {
        HideTree(TreesQueuedToHide.GetAt(i), false);
    }
    ClearQueuedTrees();
}

// Reads the world's TreeTypes.lst, loads the tree texture atlas (the swamp one for the DLC4 worlds) and creates the
// stencil and alpha properties every LOD tree shares.
void BGSDistantTreeBlock::AllocateStaticData(TESWorldSpace* apWorld) {
    char acTreeTypes[0x104];
    BSsprintf(
        acTreeTypes, sizeof(acTreeTypes), "Data\\Meshes\\Landscape\\LOD\\%s\\Trees\\TreeTypes.lst",
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
    if (!spAtlasTexture.operator->()) {
        Warning("LOD tree texture atlas not found!");
    }
    BSDistantTreeShader::SetTextureAtlas(spAtlasTexture.operator->());
    bStaticDataLoaded = true;

    void* pStencilMemory = NiMemObject::operator new(sizeof(NiStencilProperty));
    NiStencilProperty* pStencil = pStencilMemory ? ::new (pStencilMemory) NiStencilProperty : nullptr;
    spSharedStencil = pStencil;
    spSharedStencil.operator->()->SetDrawMode(NiStencilProperty::DRAW_BOTH);

    void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
    NiAlphaProperty* pAlpha = pAlphaMemory ? ::new (pAlphaMemory) NiAlphaProperty : nullptr;
    spSharedAlpha = pAlpha;
    spSharedAlpha.operator->()->SetAlphaTesting(true);
    spSharedAlpha.operator->()->SetAlphaBlending(false);
    spSharedAlpha.operator->()->SetTestRef(0x80);
    spSharedAlpha.operator->()->SetTestMode(NiAlphaProperty::TEST_GREATER);
}

void BGSDistantTreeBlock::ReadTypes(NiFile* apInputFile) {
    uint32_t uinumTypes;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODTREES, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantTreeBlock.cpp", 638
    );
    BGSDistantTreeBlock::TreeTypes.Clear(true);
    uinumTypes = 0;
    apInputFile->Read(&uinumTypes, 4u);
    for (uint32_t i = 0; i < uinumTypes; ++i) {
        BGSDistantTreeBlock::TreeType newType;
        apInputFile->Read(&newType, 32u);
        newType.spSharedModelData.operator=(static_cast<NiTriShapeData*>(nullptr));
        BGSDistantTreeBlock::AddType(newType);
    }
}

uint32_t BGSDistantTreeBlock::GetBlockCount() {
    return (uint32_t)BGSDistantTreeBlock::iCount;
}

int32_t BGSDistantTreeBlock::GetMaxInstanceCount() {
    int32_t result = BSDistantTreeShader::uiMaxInstanceCount;
    if (BSDistantTreeShader::uiMaxInstanceCount) {
        return result;
    }
    result = (BSShaderManager::eShaderLevel < 2) ? 40 : 114;
    BSDistantTreeShader::uiMaxInstanceCount = result;
    return result;
}

bool BGSDistantTreeBlock::QStaticDataLoaded() {
    return (bool)((int8_t)BGSDistantTreeBlock::bStaticDataLoaded);
}

void BGSDistantTreeBlock::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSDistantTreeBlock::DeallocateStaticData() {
    for (uint32_t i = 0; i < BGSDistantTreeBlock::TreeTypes.iSize; ++i) {
        BGSDistantTreeBlock::TreeType* v0 = &BGSDistantTreeBlock::TreeTypes.GetAt(i);
        v0->spSharedModelData.operator=(static_cast<NiTriShapeData*>(nullptr));
    }
    BGSDistantTreeBlock::spSharedStencil.operator=(static_cast<NiStencilProperty*>(nullptr));
    BGSDistantTreeBlock::spSharedAlpha.operator=(static_cast<NiAlphaProperty*>(nullptr));
    BGSDistantTreeBlock::spAtlasTexture.operator=(static_cast<NiSourceTexture*>(nullptr));
    BSDistantTreeShader::SetTextureAtlas(nullptr);
    BGSDistantTreeBlock::bStaticDataLoaded = false;
}

void BGSDistantTreeBlock::AddType(const BGSDistantTreeBlock::TreeType& aType) {
    BGSDistantTreeBlock::TreeTypes.Add(aType);
    BGSDistantTreeBlock::TreeType* Last = &BGSDistantTreeBlock::TreeTypes.GetLast();
    BGSDistantTreeBlock::TreeType::InitValues(Last);
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
    ::FormatTreeDTL(cFilePath, apWorld, aiLevel, aiX, aiY);
    SetFileName(&cFilePath[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
    bFileExists = false;
}

BGSDistantTreeBlockLoadTask::~BGSDistantTreeBlockLoadTask() {}

void BGSDistantTreeBlockLoadTask::Run() {
    char* ProcessLevel = pFileName;
    NiFile* apInputFile = FileFinder::GetFile(ProcessLevel, NiFile::READ_ONLY, 2048u, 1u);
    if (!apInputFile || bCanceled) {
        bFileExists = false;
    }
    else {
        pBlock->Read(apInputFile);
        delete apInputFile;
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

void BSDistantTreeShader::SetTextureAtlas(NiTexture* apTextureAtlas) {
    BSDistantTreeShader::spTextureAtlas.operator=(apTextureAtlas);
}
