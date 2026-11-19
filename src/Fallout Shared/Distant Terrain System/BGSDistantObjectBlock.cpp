#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSShader.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlockLoadTask.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/eBool.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

// Path formats that the functions pass to BSsprintf.
using BGSDistantObjectBlock_Format59_t = const char[59];
// "Data\Meshes\Landscape\LOD\%s\Blocks\%s.Level%i.X%i.Y%i.NIF"
static BGSDistantObjectBlock_Format59_t BGSDistantObjectBlock_BlockFormat{
    "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.X%i.Y%i.NIF"
};
using BGSDistantObjectBlock_Format64_t = const char[64];
// "Data\Meshes\Landscape\LOD\%s\Blocks\%s.Level%i.High.X%i.Y%i.NIF"
static BGSDistantObjectBlock_Format64_t BGSDistantObjectBlock_BlockHighFormat{
    "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.High.X%i.Y%i.NIF"
};
using BGSDistantObjectBlock_Format74_t = const char[74];
// "Data\Meshes\Landscape\LOD\%s\Blocks\%s.Level%i.X%i.Y%i.PostApocalypse.NIF"
static BGSDistantObjectBlock_Format74_t BGSDistantObjectBlock_BlockPAFormat{
    "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.X%i.Y%i.PostApocalypse.NIF"
};
using BGSDistantObjectBlock_Format60_t = const char[60];
// "Data\Meshes\Landscape\LOD\%s\Stinger\%s.Level%i.X%i.Y%i.NIF"
static BGSDistantObjectBlock_Format60_t BGSDistantObjectBlock_StingerFormat{
    "Data\\Meshes\\Landscape\\LOD\\%s\\Stinger\\%s.Level%i.X%i.Y%i.NIF"
};
using BGSDistantObjectBlock_Format65_t = const char[65];
// "Data\Meshes\Landscape\LOD\%s\Stinger\%s.Level%i.High.X%i.Y%i.NIF"
static BGSDistantObjectBlock_Format65_t BGSDistantObjectBlock_StingerHighFormat{
    "Data\\Meshes\\Landscape\\LOD\\%s\\Stinger\\%s.Level%i.High.X%i.Y%i.NIF"
};
BSSimpleList<BGSDistantObjectBlock*> BGSDistantObjectBlock::BlocksLoaded;

BGSDistantObjectBlock::BGSDistantObjectBlock(
    BGSTerrainNode* apNode,
    const int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY,
    bool abLoadLowPriority
)
  : pNode(apNode), spMesh(nullptr), spNode(nullptr), spOldMesh(nullptr), spNodeStinger(nullptr),
    spNodeStingerOld(nullptr), spLoadTask(nullptr), spLoadTaskStinger(nullptr), bDoneLoading(false), bAttached(false),
    bPrepared(false), bHighPriority(!abLoadLowPriority), bDetachFlag(false), nDetachCount(0) {
    AutoMemContext kContext(MEM_CONTEXT_LODBUILDINGS);
    TESWorldSpace* pWorld = apNode->GetManager()->GetWorld();
    spLoadTask =
        new BGSDistantObjectBlockLoadTask(this, pWorld, asCellX, asCellY, aiLevel, false, !abLoadLowPriority, false);
    spLoadTask->QueueMe();
}

BGSDistantObjectBlock::~BGSDistantObjectBlock() {
    RemoveFromList();
    if (spNode.m_pObject || spMesh.m_pObject) {
        --BGSDistantObjectBlock::uiHighPrioBlocks;
        pNode->pManager->GetLODObjectRoot()->DetachChild(spNode.m_pObject);
        pNode->pManager->GetLODObjectRoot()->DetachChild(spMesh.m_pObject);
    }
    if (spNodeStinger.m_pObject) {
        --BGSDistantObjectBlock::uiHighPrioBlocks;
        pNode->pManager->GetLODObjectRoot()->DetachChild(spNodeStinger.m_pObject);
    }
    if (spLoadTask.m_pObject) {
        pIOManager->CancelTask(spLoadTask.m_pObject, nullptr);
    }
    if (spLoadTaskStinger.m_pObject) {
        pIOManager->CancelTask(spLoadTaskStinger.m_pObject, nullptr);
    }
    NiObjectGarbage::Add(spNode.m_pObject);
    NiObjectGarbage::Add(spNodeStinger.m_pObject);
    // The references are dropped before the texture purge, which then frees what they held.
    spLoadTask = nullptr;
    spMesh = nullptr;
    spOldMesh = nullptr;
    spNode = nullptr;
    spLoadTaskStinger = nullptr;
    spNodeStingerOld = nullptr;
    spNodeStinger = nullptr;
    if (appMain->bFlyCamera) {
        BSTexturePalette::PurgeUnusedTextures();
    }
}

void BGSDistantObjectBlock::TriggerApocalypse(
    TESWorldSpace* apWorld,
    const int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    alignas(AutoMemContext) uint8_t acContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (acContext) AutoMemContext(
        MEM_CONTEXT_LODBUILDINGS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\"
        "BGSDistantObjectBlock.cpp",
        576
    );
    spOldMesh = spMesh;
    void* pMemory = ourMalloc(sizeof(BGSDistantObjectBlockLoadTask));
    BGSDistantObjectBlockLoadTask* pTask = nullptr;
    if (pMemory) {
        pTask =
            ::new (pMemory) BGSDistantObjectBlockLoadTask(this, apWorld, asCellX, asCellY, aiLevel, true, true, false);
    }
    spLoadTask = pTask;
    spLoadTask->QueueMe();
    bDoneLoading = false;
    bAttached = false;
    pContext->~AutoMemContext();
}

void BGSDistantObjectBlock::SwitchToHigh(
    TESWorldSpace* apWorld,
    const int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    alignas(AutoMemContext) uint8_t acContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (acContext) AutoMemContext(
        MEM_CONTEXT_LODBUILDINGS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\"
        "BGSDistantObjectBlock.cpp",
        626
    );
    spOldMesh = spMesh;
    spNodeStingerOld = spNodeStinger;
    if (spLoadTask) {
        pIOManager->CancelTask(spLoadTask, nullptr);
        spLoadTask = nullptr;
    }
    if (spLoadTaskStinger) {
        pIOManager->CancelTask(spLoadTaskStinger, nullptr);
        spLoadTaskStinger = nullptr;
    }
    void* pMemory = ourMalloc(sizeof(BGSDistantObjectBlockLoadTask));
    BGSDistantObjectBlockLoadTask* pTask = nullptr;
    if (pMemory) {
        pTask =
            ::new (pMemory) BGSDistantObjectBlockLoadTask(this, apWorld, asCellX, asCellY, aiLevel, false, true, false);
    }
    spLoadTask = pTask;
    nDetachCount = 0;
    spLoadTask->QueueMe();
    bDoneLoading = false;
    bAttached = false;
    bHighPriority = true;
    bPrepared = false;
    pContext->~AutoMemContext();
}

bool BGSDistantObjectBlock::IsInList() {
    BGSDistantObjectBlock* v3;
    BSSimpleList<BGSDistantObjectBlock*>* ecx0;
    for (ecx0 = &BGSDistantObjectBlock::BlocksLoaded; ecx0; ecx0 = ecx0->m_pkNext) {
        v3 = ecx0->m_item;
        if (v3 && (v3 == this)) {
            return true;
        }
    }
    return false;
}

void BGSDistantObjectBlock::EnableAllStingerVisibility(NiAVObject* apGeomNode) {
    uint32_t i;
    NiNode* v3;
    uint32_t v5;
    NiNode* pNode;
    if (apGeomNode) {
        apGeomNode->NiAVObject::SetAppCulled(false);
    }
    if (apGeomNode->IsNode()) {
        pNode = apGeomNode->IsNode();
        v5 = 0;
        for (i = pNode->GetChildCount(); v5 < i; i = pNode->GetChildCount()) {
            v3 = static_cast<NiNode*>(pNode->GetAtChecked(v5));
            EnableAllStingerVisibility(v3);
            ++v5;
        }
    }
}

bool BGSDistantObjectBlock::CheckDetach() {
    return bDetachFlag;
}

void BGSDistantObjectBlock::AddToList() {
    BGSDistantObjectBlock* pBlock = this;
    BlocksLoaded.AddTail(pBlock);
}

BSMultiBoundNode* BGSDistantObjectBlock::GetNode(bool abStingerVersion) {
    NiPointer<BSMultiBoundNode>* p_spStingerBlock;
    if (abStingerVersion) {
        p_spStingerBlock = &spNodeStinger;
    }
    else {
        p_spStingerBlock = &spNode;
    }
    return *p_spStingerBlock;
}

bool BGSDistantObjectBlock::GetHighPriority() {
    return (bool)((uint8_t)bHighPriority);
}

void BGSDistantObjectBlock::FlagDetach() {
    bDetachFlag = true;
}

void BGSDistantObjectBlock::UpdateStingerVisibility(
    NiAVObject* apGeomNode,
    int32_t aiLODLevel,
    int16_t aiBaseCellx,
    int16_t aiBaseCelly
) {
    NiPoint3 pos;
    if (apGeomNode->IsNode()) {
        NiNode* pNode = apGeomNode->IsNode();
        for (uint32_t auiIndex = 0; auiIndex < pNode->GetChildCount(); ++auiIndex) {
            NiNode* pChild = static_cast<NiNode*>(pNode->GetAtChecked(auiIndex));
            if (pChild) {
                UpdateStingerVisibility(pChild, aiLODLevel, aiBaseCellx, aiBaseCelly);
            }
        }
    }
    else {
        pos = apGeomNode->GetWorldTranslate();
        TESWorldSpace* WorldSpace = TES::pInstance->GetWorldSpace();
        TESObjectCELL* cell = WorldSpace->GetCellFromWorldCoord(pos);
        if (TES::pInstance->IsCellLoaded(cell, true) && cell->All3DWithVisibleDistantFadingIn()) {
            BGSDistantObjectBlock::EnableStingerRenderAll(apGeomNode, false);
        }
        else {
            BGSDistantObjectBlock::EnableStingerRenderAll(apGeomNode, true);
        }
    }
}

void BGSDistantObjectBlock::RemoveFromList() {
    BGSDistantObjectBlock* v2;
    while (IsInList()) {
        v2 = this;
        (&BGSDistantObjectBlock::BlocksLoaded)->Remove(v2);
    }
}

void BGSDistantObjectBlock::Prepare() {
    bool bUseNormalLOD = !bPrepared || (spMesh.m_pObject && (spOldMesh.m_pObject && spMesh != spOldMesh));
    bool bUseStinger =
        !bPrepared || (spNodeStinger.m_pObject && (spNodeStingerOld.m_pObject && spNodeStinger != spNodeStingerOld));
    if ((bUseNormalLOD) && (spMesh.m_pObject)) {
        NiAlphaProperty* palpha;
        spMesh.m_pObject->RemoveProperty(NiAlphaProperty::GetType());
        NiAlphaProperty* apMemory = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
        if (apMemory) {
            palpha = ::new (apMemory) NiAlphaProperty;
        }
        else {
            palpha = nullptr;
        }
        palpha->SetAlphaTesting(true);
        palpha->SetAlphaBlending(false);
        palpha->SetTestRef(128);
        palpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
        spMesh.m_pObject->AttachProperty(palpha);
        int32_t Type = NiShadeProperty::GetType();
        BSShaderProperty* pshadeProp = static_cast<BSShaderProperty*>(spMesh.m_pObject->GetProperty(Type));
        pshadeProp->SetFlag(34u, 1);
        pshadeProp->SetFlag(33u, 0);
        pshadeProp->SetFlag(0, 0);
        BSShaderProperty* pshadeProp_2 = pshadeProp->ClarifyShader(spMesh.m_pObject, false, true);
        if (pshadeProp_2) {
            spMesh.m_pObject->RemoveProperty(3);
            pshadeProp = pshadeProp_2;
            spMesh.m_pObject->AttachProperty(pshadeProp_2);
        }
        int32_t NumberOfItemsActivated = pshadeProp->iShader;
        BSShader* Shader = BSShaderManager::GetShader(NumberOfItemsActivated);
        spMesh.m_pObject->SetShader(Shader);
        NiUpdateData foo(0.0f, false, false);
        spMesh.m_pObject->UpdateProperties();
        spMesh.m_pObject->Update(foo);
        pNode->UpdateBlockVisibility(false);
        spMesh.m_pObject->GetModelData()->m_ucKeepFlags = 0;
        spMesh.m_pObject->GetModelData()->SetConsistency(NiGeometryData::STATIC);
        spMesh.m_pObject->GetModelData()->SetCompressFlags(1u);
        BSShader* pShader = spMesh.m_pObject->GetShader();
        // The renderer is the platform one; its PrecacheGeometry and PerformPrecache are called.
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        NiD3DShaderDeclaration* v4 = pShader->GetShaderDecl(spMesh.m_pObject, pshadeProp);
        pRenderer->PrecacheGeometry(spMesh.m_pObject, 0, 0, v4);
        NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        Renderer_->PerformPrecache();
        bPrepared = true;
    }

    if ((bUseStinger) && (spNodeStinger.m_pObject)) {
        PrepareStingerObject(spNodeStinger.m_pObject);
        NiDX9Renderer* v6 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        v6->PerformPrecache();
        bPrepared = true;
    }
}

void BGSDistantObjectBlock::PrepareStingerObject(NiAVObject* apGeom) {
    if (apGeom) {
        if (apGeom->IsNode()) {
            NiNode* pNode = apGeom->IsNode();
            for (int32_t child_iter = 0; child_iter < pNode->GetChildCount(); ++child_iter) {
                NiAVObject* v2 = pNode->GetAtChecked(static_cast<uint32_t>(child_iter));
                PrepareStingerObject(v2);
            }
        }
        else {
            NiAlphaProperty* palpha;
            NiGeometry* pGeom = apGeom->IsGeometry();
            int32_t Type = NiShadeProperty::GetType();
            pGeom->GetProperty(Type);
            int32_t v4 = NiShadeProperty::GetType();
            BSShaderProperty* pshadeProp = static_cast<BSShaderProperty*>(pGeom->GetProperty(v4));
            pGeom->RemoveProperty(NiAlphaProperty::GetType());
            NiAlphaProperty* apMemory =
                static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
            if (apMemory) {
                palpha = ::new (apMemory) NiAlphaProperty;
            }
            else {
                palpha = nullptr;
            }
            palpha->SetAlphaTesting(true);
            palpha->SetAlphaBlending(false);
            palpha->SetTestRef(128);
            palpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
            pGeom->AttachProperty(palpha);
            pshadeProp->SetFlag(34u, 1);
            pshadeProp->SetFlag(55u, 1);
            BSShaderProperty* pshadeProp_2 = pshadeProp->ClarifyShader(pGeom, false, true);
            if (pshadeProp_2) {
                pGeom->RemoveProperty(3);
                pshadeProp = pshadeProp_2;
                pGeom->AttachProperty(pshadeProp_2);
            }
            int32_t NumberOfItemsActivated = pshadeProp->iShader;
            pGeom->SetShader(BSShaderManager::GetShader(NumberOfItemsActivated));
            NiUpdateData foo(0.0f, false, false);
            pGeom->UpdateProperties();
            pGeom->Update(foo);
            this->pNode->UpdateBlockVisibility(false);
            pGeom->GetModelData()->m_ucKeepFlags = 0;
            pGeom->GetModelData()->SetConsistency(NiGeometryData::STATIC);
            pGeom->GetModelData()->SetCompressFlags(1u);
            BSShader* Shader = pGeom->GetShader();
            NiRenderer* Renderer_ = NiRenderer::GetRenderer();
            NiD3DShaderDeclaration* v6 = Shader->GetShaderDecl(pGeom, pshadeProp);
            static_cast<NiDX9Renderer*>(Renderer_)->PrecacheGeometry(pGeom, 0, 0, v6);
        }
    }
}

void BGSDistantObjectBlock::Detach() {
    RemoveFromList();
    if (spNode.m_pObject || (spMesh.m_pObject || spNodeStinger.m_pObject)) {
        --BGSDistantObjectBlock::uiLowPrioBlocks;
    }
    if (spNode.m_pObject || spMesh.m_pObject) {
        BGSTerrainManager* v1 = pNode->GetManager();
        NiNode* LODObjectRoot = v1->GetLODObjectRoot();
        NiAVObject* v2 = spNode.m_pObject;
        LODObjectRoot->DetachChild(v2);
        BGSTerrainManager* v3 = pNode->GetManager();
        NiNode* v11 = v3->GetLODObjectRoot();
        NiAVObject* v4 = spMesh.m_pObject;
        v11->DetachChild(v4);
        BGSTerrainManager* v5 = pNode->GetManager();
        NiNode* v10 = v5->GetLODObjectRoot();
        NiAVObject* v6 = spOldMesh.m_pObject;
        v10->DetachChild(v6);
        spNode = static_cast<BSMultiBoundNode*>(nullptr);
        spMesh = static_cast<NiGeometry*>(nullptr);
        spOldMesh = static_cast<NiGeometry*>(nullptr);
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    if (spNodeStinger.m_pObject) {
        BGSTerrainManager* v7 = pNode->GetManager();
        NiNode* v9 = v7->GetLODObjectRoot();
        NiAVObject* v8 = spNodeStinger.m_pObject;
        v9->DetachChild(v8);
        spNodeStinger = static_cast<BSMultiBoundNode*>(nullptr);
        spNodeStingerOld = static_cast<BSMultiBoundNode*>(nullptr);
        spLoadTaskStinger = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    bDetachFlag = false;
}

void BGSDistantObjectBlock::Attach() {
    if ((spMesh.m_pObject || spNodeStinger.m_pObject) && !bAttached) {
        RemoveFromList();
        if (spOldMesh.m_pObject) {
            if (spNode.m_pObject) {
                BGSTerrainNode* v1 = pNode;
                BGSTerrainManager* v2 = v1->GetManager();
                NiNode* LODObjectRoot = v2->GetLODObjectRoot();
                NiGeometry* v3 = spOldMesh.m_pObject;
                NiAVObject* Parent = v3->GetParent();
                LODObjectRoot->DetachChild(Parent);
                BGSTerrainNode* v5 = pNode;
                BGSTerrainManager* v6 = v5->GetManager();
                NiNode* pLODObjectRoot = v6->GetLODObjectRoot();
                NiAVObject* v7 = spNode.m_pObject;
                pLODObjectRoot->AttachChild(v7, 1);
            }
            else {
                BGSTerrainNode* v8 = pNode;
                BGSTerrainManager* v9 = v8->GetManager();
                NiNode* v35 = v9->GetLODObjectRoot();
                NiAVObject* v10 = spOldMesh.m_pObject;
                v35->DetachChild(v10);
                BGSTerrainNode* v11 = pNode;
                BGSTerrainManager* v12 = v11->GetManager();
                NiNode* v34 = v12->GetLODObjectRoot();
                NiAVObject* v13 = spMesh.m_pObject;
                v34->AttachChild(v13, 1);
            }
        }
        else {
            if (spNode.m_pObject) {
                BGSTerrainNode* v14 = pNode;
                BGSTerrainManager* v15 = v14->GetManager();
                NiNode* v33 = v15->GetLODObjectRoot();
                NiAVObject* v16 = spNode.m_pObject;
                v33->AttachChild(v16, 1);
            }
            else {
                if (spMesh.m_pObject) {
                    BGSTerrainNode* v17 = pNode;
                    BGSTerrainManager* v18 = v17->GetManager();
                    NiNode* v32 = v18->GetLODObjectRoot();
                    NiAVObject* v19 = spMesh.m_pObject;
                    v32->AttachChild(v19, 1);
                }
            }
        }
        if (spNodeStingerOld.m_pObject) {
            if (spNodeStinger.m_pObject) {
                BGSTerrainNode* v20 = pNode;
                BGSTerrainManager* v21 = v20->GetManager();
                NiNode* v31 = v21->GetLODObjectRoot();
                NiAVObject* v22 = spNodeStingerOld.m_pObject;
                v31->DetachChild(v22);
                BGSTerrainNode* v23 = pNode;
                BGSTerrainManager* v24 = v23->GetManager();
                NiNode* v30 = v24->GetLODObjectRoot();
                NiAVObject* v25 = spNodeStinger.m_pObject;
                v30->AttachChild(v25, 1);
            }
        }
        else {
            if (spNodeStinger.m_pObject) {
                BGSTerrainNode* v26 = pNode;
                BGSTerrainManager* v27 = v26->GetManager();
                NiNode* v29 = v27->GetLODObjectRoot();
                NiAVObject* v28 = spNodeStinger.m_pObject;
                v29->AttachChild(v28, 1);
            }
        }
        if (GetHighPriority()) {
            if (spOldMesh.m_pObject || spNodeStingerOld.m_pObject) {
                --BGSDistantObjectBlock::uiLowPrioBlocks;
            }
            if (spMesh.m_pObject || spNodeStinger.m_pObject) {
                ++BGSDistantObjectBlock::uiHighPrioBlocks;
            }
        }
        else {
            if (spOldMesh.m_pObject || spNodeStingerOld.m_pObject) {
                --BGSDistantObjectBlock::uiHighPrioBlocks;
            }
            if (spMesh.m_pObject || spNodeStinger.m_pObject) {
                ++BGSDistantObjectBlock::uiLowPrioBlocks;
            }
        }
        Prepare();
        if ((spMesh.m_pObject || spNodeStinger.m_pObject) && !IsInList()) {
            AddToList();
        }
        spOldMesh = static_cast<NiGeometry*>(nullptr);
        spNodeStingerOld = static_cast<BSMultiBoundNode*>(nullptr);
        bAttached = true;
    }
}

void BGSDistantObjectBlock::SwitchToLow(
    TESWorldSpace* apWorld,
    const int32_t aiLevel,
    const int16_t asCellX,
    const int16_t asCellY
) {
    BGSDistantObjectBlockLoadTask* Task;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantObjectBlock.cpp", 592
    );
    spOldMesh = spMesh;
    spNodeStingerOld = spNodeStinger;
    if (spLoadTask.m_pObject) {
        BGSDistantObjectBlockLoadTask* v6 = spLoadTask.m_pObject;
        pIOManager->CancelTask(v6, nullptr);
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    if (spLoadTaskStinger.m_pObject) {
        BGSDistantObjectBlockLoadTask* v7 = spLoadTaskStinger.m_pObject;
        pIOManager->CancelTask(v7, nullptr);
        spLoadTaskStinger = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    BGSDistantObjectBlockLoadTask* v11 =
        static_cast<BGSDistantObjectBlockLoadTask*>(ourMalloc(sizeof(BGSDistantObjectBlockLoadTask)));
    if (v11) {
        bool MegatonDestroyed = pPlayer->GetMegatonDestroyed();
        Task = ::new (v11)
            BGSDistantObjectBlockLoadTask(this, apWorld, asCellX, asCellY, aiLevel, MegatonDestroyed, false, false);
    }
    else {
        Task = nullptr;
    }
    spLoadTask = Task;
    nDetachCount = 0;
    BGSDistantObjectBlockLoadTask* v8 = spLoadTask.m_pObject;
    v8->QueueMe();
    bDoneLoading = false;
    bAttached = false;
    bHighPriority = false;
    bPrepared = false;
}

void BGSDistantObjectBlock::LoadFromTask(eBool abLoadData) {
    if ((spLoadTask.m_pObject) && (spLoadTask.m_pObject->IsTaskFinished())) {
        if (abLoadData) {
            spLoadTask.m_pObject->GetMesh(spMesh);
            spLoadTask.m_pObject->GetNode(spNode);
        }
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
        if (!spLoadTaskStinger.m_pObject) {
            bDoneLoading = true;
        }
    }

    if ((spLoadTaskStinger.m_pObject) && (spLoadTaskStinger.m_pObject->IsTaskFinished())) {
        if (abLoadData) {
            spLoadTaskStinger.m_pObject->GetNode(spNodeStinger);
        }
        spLoadTaskStinger = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
        if (!spLoadTask.m_pObject) {
            bDoneLoading = true;
        }
    }
}

void BGSDistantObjectBlock::EnableStingerRenderAll(NiAVObject* apGeom, bool abEnable) {
    uint32_t i;
    NiNode* v3;
    uint32_t v4;
    NiNode* v5;
    if (apGeom) {
        apGeom->NiAVObject::SetAppCulled((uint8_t)(!abEnable));
    }
    if (apGeom->IsNode()) {
        v5 = apGeom->IsNode();
        v4 = 0;
        for (i = v5->GetChildCount(); v4 < i; i = v5->GetChildCount()) {
            v3 = static_cast<NiNode*>(v5->GetAtChecked(v4));
            BGSDistantObjectBlock::EnableStingerRenderAll(v3, abEnable);
            ++v4;
        }
    }
}

void BGSDistantObjectBlock::BuildBlockFileName(
    char* apOutput,
    TESWorldSpace* apWorld,
    int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    BSsprintf(
        apOutput, 0x104, BGSDistantObjectBlock_BlockFormat, apWorld->GetFormEditorID(), apWorld->GetFormEditorID(),
        aiLevel, static_cast<int32_t>(asCellX), static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::BuildBlockFileNameHighPriority(
    char* apOutput,
    TESWorldSpace* apWorld,
    int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    BSsprintf(
        apOutput, 0x104, BGSDistantObjectBlock_BlockHighFormat, apWorld->GetFormEditorID(), apWorld->GetFormEditorID(),
        aiLevel, static_cast<int32_t>(asCellX), static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::BuildBlockFileNamePA(
    char* apOutput,
    TESWorldSpace* apWorld,
    int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    BSsprintf(
        apOutput, 0x104, BGSDistantObjectBlock_BlockPAFormat, apWorld->GetFormEditorID(), apWorld->GetFormEditorID(),
        aiLevel, static_cast<int32_t>(asCellX), static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::BuildStingerBlockFileName(
    char* apOutput,
    TESWorldSpace* apWorld,
    int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    BSsprintf(
        apOutput, 0x104, BGSDistantObjectBlock_StingerFormat, apWorld->GetFormEditorID(), apWorld->GetFormEditorID(),
        aiLevel, static_cast<int32_t>(asCellX), static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::BuildStingerBlockFileNameHighPriority(
    char* apOutput,
    TESWorldSpace* apWorld,
    int32_t aiLevel,
    int16_t asCellX,
    int16_t asCellY
) {
    BSsprintf(
        apOutput, 0x104, BGSDistantObjectBlock_StingerHighFormat, apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(), aiLevel, static_cast<int32_t>(asCellX), static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSDistantObjectBlock::ResetCullResults() {
    for (BGSDistantObjectBlock* m_item : BGSDistantObjectBlock::BlocksLoaded) {
        if ((m_item) && (!m_item->CheckDetach())) {
            BSMultiBoundNode* Node = m_item->GetNode(false);
            if (Node) {
                Node->QMultiBound()->QShape()->ResetCullResult();
            }
            BSMultiBoundNode* v1 = m_item->GetNode(true);
            if (v1) {
                v1->QMultiBound()->QShape()->ResetCullResult();
            }
        }
    }
}

BGSDistantObjectBlockLoadTask::BGSDistantObjectBlockLoadTask(
    BGSDistantObjectBlock* apBlock,
    TESWorldSpace* apWorld,
    int32_t aiX,
    int32_t aiY,
    int32_t aiLevel,
    bool abPostApocalypse,
    bool abHighPriority,
    bool abStingerVersion
)
  : QueuedFileEntry(IO_TASK_PRIORITY_MID), bFinishedQueuing(false), iX(aiX), iY(aiY), iLevel(aiLevel), pBlock(apBlock),
    pWorldSpace(apWorld), spMesh(nullptr), spNode(nullptr), bCanceled(false), bStingerVersion(abStingerVersion) {
    char path[268];
    if (abStingerVersion) {
        if (abHighPriority) {
            BGSDistantObjectBlock::BuildStingerBlockFileNameHighPriority(path, pWorldSpace, aiLevel, aiX, aiY);
        }
        else {
            BGSDistantObjectBlock::BuildStingerBlockFileName(path, pWorldSpace, aiLevel, aiX, aiY);
        }
    }
    else if (abHighPriority) {
        BGSDistantObjectBlock::BuildBlockFileNameHighPriority(path, pWorldSpace, aiLevel, aiX, aiY);
    }
    else if (!abPostApocalypse && !pPlayer->GetMegatonDestroyed()) {
        BGSDistantObjectBlock::BuildBlockFileName(path, pWorldSpace, aiLevel, aiX, aiY);
    }
    else {
        // The post-apocalypse mesh, or the normal one where the world has none.
        BGSDistantObjectBlock::BuildBlockFileNamePA(path, pWorldSpace, aiLevel, aiX, aiY);
        if (!FileFinder::Locate(path, nullptr, 0, 0xFFFFFFFFu)) {
            BGSDistantObjectBlock::BuildBlockFileName(path, pWorldSpace, aiLevel, aiX, aiY);
        }
    }
    SetFileName(&path[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

BGSDistantObjectBlockLoadTask::~BGSDistantObjectBlockLoadTask() {}

void BGSDistantObjectBlockLoadTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantObjectBlock.cpp", 922
    );
    BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
    uint32_t iload = 1;
    if (!pfile) {
        iload = FileFinder::Locate(pFileName, nullptr, 6u, 1u) != 0;
    }
    BSStream stream;
    if (iload && stream.BSStream::Load(pFileName, pfile)) {
        NiObject* ObjectAt = stream.GetObjectAt(0);
        if (ObjectAt->IsNode()) {
            spNode = static_cast<BSMultiBoundNode*>(stream.GetObjectAt(0));
            spMesh = static_cast<NiGeometry*>(spNode.m_pObject->GetAtChecked(0));
        }
        else {
            spMesh = static_cast<NiGeometry*>(stream.GetObjectAt(0));
        }
    }
    else {
        if (pBlock->GetHighPriority()) {
            pBlock->FlagDetach();
        }
    }
}

void BGSDistantObjectBlockLoadTask::Finish() {
    if (spMesh.m_pObject) {
        if (bCanceled) {
            spMesh = static_cast<NiGeometry*>(nullptr);
            spNode = static_cast<BSMultiBoundNode*>(nullptr);
        }
        else {
            pBlock->LoadFromTask(eTrue);
            pBlock->pNode->pManager->SetNeedsImmediateUpdate(true);
            NiGeometry* v1 = spMesh.m_pObject;
            if (!v1->IsSegmentedTriShape()) {
                Warning("MODELS: Found a distant object block for %i:%i,%i but it is not segmented.", iLevel, iX, iY);
            }
        }
    }
    else {
        if ((!bCanceled) && (pBlock)) {
            pBlock->LoadFromTask(eFalse);
        }
    }
}

void BGSDistantObjectBlockLoadTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    if (!bCanceled) {
        bCanceled = true;
        QueuedFile::Cancel(aePreviousState, apTaskOwner);
    }
}

bool BGSDistantObjectBlockLoadTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    // "Object LOD Task for block ( %i, %i ) at level ( %i )"
    BSsprintf(apDescription, aiBufferSize, "Object LOD Task for block ( %i, %i ) at level ( %i )", iX, iY, iLevel);
    return true;
}

void BGSDistantObjectBlockLoadTask::QueueMe() {
    int32_t iOTaskPriority;
    int32_t v2;
    int8_t apFilePath[264];
    int8_t apTextureFileName[268];
    if (!bStingerVersion) {
        BGSDistantObjectBlockLoadTask::GetBuildingsPath(
            static_cast<char*>(static_cast<void*>(apFilePath)), pWorldSpace
        );
        iOTaskPriority = (int32_t)IOTask::GetIOTaskPriority();
        pModelLoader->QueueTexture(
            static_cast<const char*>(static_cast<const void*>(apFilePath)),
            static_cast<IO_TASK_PRIORITY>(iOTaskPriority), this
        );
        BGSDistantObjectBlockLoadTask::GetBuildingsNormalPath(
            static_cast<char*>(static_cast<void*>(apTextureFileName)), pWorldSpace
        );
        v2 = (int32_t)IOTask::GetIOTaskPriority();
        pModelLoader->QueueTexture(
            static_cast<const char*>(static_cast<const void*>(apTextureFileName)), static_cast<IO_TASK_PRIORITY>(v2),
            this
        );
    }
    bFinishedQueuing = true;
    CheckFinished();
}

void BGSDistantObjectBlockLoadTask::CheckFinished() {
    if (GetAllChildrenFinished() && bFinishedQueuing) {
        if (IsPending()) {
            pIOManager->AddTask(this);
        }
        else {
            QueuedFile::CheckFinished();
        }
    }
}

// A block in an archive loads on that archive's file index (its runtime ID past the first four); a loose one on
// index 1.
uint8_t BGSDistantObjectBlockLoadTask::GetFileIndex() {
    if (GetFileEntry()) {
        Archive* pArchive = ArchiveManager::GetArchiveForFileEntry(GetFileEntry(), ARCHIVE_TYPE_MESHES);
        if (pArchive) {
            return pArchive->GetRuntimeID() + 4;
        }
    }
    return 1;
}

void BGSDistantObjectBlockLoadTask::GetMesh(NiPointer<NiGeometry>& aspMesh) {
    aspMesh = spMesh;
}

void BGSDistantObjectBlockLoadTask::GetNode(NiPointer<BSMultiBoundNode>& aspNode) {
    aspNode = spNode;
}

void BGSDistantObjectBlockLoadTask::GetBuildingsPath(char* apDest, TESWorldSpace* apWorldspace) {
    const char* v3 = apWorldspace->GetFormEditorID();
    const char* v2 = apWorldspace->GetFormEditorID();
    BSsprintf(apDest, 0x104u, "Data\\Textures\\Landscape\\LOD\\%s\\Blocks\\%s.Buildings.dds", v2, v3);
}

void BGSDistantObjectBlockLoadTask::GetBuildingsNormalPath(char* apDest, TESWorldSpace* a2) {
    const char* v3 = a2->GetFormEditorID();
    const char* v2 = a2->GetFormEditorID();
    BSsprintf(apDest, 0x104u, "Data\\Textures\\Landscape\\LOD\\%s\\Blocks\\%s.Buildings_n.dds", v2, v3);
}

void BGSDistantObjectBlockLoadTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESWorldSpace* BGSTerrainManager::GetWorld() {
    return pWorld;
}

BGSTerrainManager* BGSTerrainNode::GetManager() {
    return pManager;
}
