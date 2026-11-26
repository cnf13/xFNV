#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
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
    spLoadTask = new BGSDistantObjectBlockLoadTask(
        this,
        pWorld,
        asCellX,
        asCellY,
        aiLevel,
        false,
        !abLoadLowPriority,
        false
    );
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
        MEM_CONTEXT_LODBUILDINGS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\"
        "BGSDistantObjectBlock.cpp",
        576
    );
    spOldMesh = spMesh;
    BGSDistantObjectBlockLoadTask*
        pTask = new BGSDistantObjectBlockLoadTask(this, apWorld, asCellX, asCellY, aiLevel, true, true, false);
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
        MEM_CONTEXT_LODBUILDINGS,
        true,
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
    BGSDistantObjectBlockLoadTask*
        pTask = new BGSDistantObjectBlockLoadTask(this, apWorld, asCellX, asCellY, aiLevel, false, true, false);
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
    for (BSSimpleList<BGSDistantObjectBlock*>* pList = &BGSDistantObjectBlock::BlocksLoaded; pList;
         pList = pList->m_pkNext) {
        BGSDistantObjectBlock* pBlock = pList->m_item;
        if (pBlock && pBlock == this) {
            return true;
        }
    }
    return false;
}

void BGSDistantObjectBlock::EnableAllStingerVisibility(NiAVObject* apGeomNode) {
    if (apGeomNode) {
        apGeomNode->NiAVObject::SetAppCulled(false);
    }
    if (apGeomNode->IsNode()) {
        NiNode* pNode = apGeomNode->IsNode();
        for (uint32_t uiIndex = 0; uiIndex < pNode->GetChildCount(); ++uiIndex) {
            NiNode* pChild = static_cast<NiNode*>(pNode->GetAtChecked(uiIndex));
            EnableAllStingerVisibility(pChild);
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
    NiPointer<BSMultiBoundNode>* pspBlock = abStingerVersion ? &spNodeStinger : &spNode;
    return *pspBlock;
}

bool BGSDistantObjectBlock::GetHighPriority() {
    return bHighPriority;
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
    NiPoint3 kPos;
    if (apGeomNode->IsNode()) {
        NiNode* pNode = apGeomNode->IsNode();
        for (uint32_t uiIndex = 0; uiIndex < pNode->GetChildCount(); ++uiIndex) {
            NiNode* pChild = static_cast<NiNode*>(pNode->GetAtChecked(uiIndex));
            if (pChild) {
                UpdateStingerVisibility(pChild, aiLODLevel, aiBaseCellx, aiBaseCelly);
            }
        }
    }
    else {
        kPos = apGeomNode->GetWorldTranslate();
        TESWorldSpace* pWorldSpace = TES::pInstance->GetWorldSpace();
        TESObjectCELL* pCell = pWorldSpace->GetCellFromWorldCoord(kPos);
        if (TES::pInstance->IsCellLoaded(pCell, true) && pCell->All3DWithVisibleDistantFadingIn()) {
            BGSDistantObjectBlock::EnableStingerRenderAll(apGeomNode, false);
        }
        else {
            BGSDistantObjectBlock::EnableStingerRenderAll(apGeomNode, true);
        }
    }
}

void BGSDistantObjectBlock::RemoveFromList() {
    while (IsInList()) {
        BGSDistantObjectBlock* pBlock = this;
        BGSDistantObjectBlock::BlocksLoaded.Remove(pBlock);
    }
}

void BGSDistantObjectBlock::Prepare() {
    bool bUseNormalLOD = !bPrepared || (spMesh.m_pObject && (spOldMesh.m_pObject && spMesh != spOldMesh));
    bool bUseStinger = !bPrepared ||
        (spNodeStinger.m_pObject && (spNodeStingerOld.m_pObject && spNodeStinger != spNodeStingerOld));
    if (bUseNormalLOD && spMesh.m_pObject) {
        spMesh->RemoveProperty(NiAlphaProperty::GetType());
        NiAlphaProperty* pAlpha = new NiAlphaProperty;
        pAlpha->SetAlphaTesting(true);
        pAlpha->SetAlphaBlending(false);
        pAlpha->SetTestRef(128);
        pAlpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
        spMesh->AttachProperty(pAlpha);
        int32_t iShadeType = NiShadeProperty::GetType();
        BSShaderProperty* pShadeProp = static_cast<BSShaderProperty*>(spMesh->GetProperty(iShadeType));
        pShadeProp->SetFlag(BSShaderProperty::BSSP_FLAG_LODBUILDING, 1);
        pShadeProp->SetFlag(BSShaderProperty::BSSP_FLAG_LODLANDSCAPE, 0);
        pShadeProp->SetFlag(0, 0);
        BSShaderProperty* pClarifiedProp = pShadeProp->ClarifyShader(spMesh.m_pObject, false, true);
        if (pClarifiedProp) {
            spMesh->RemoveProperty(3);
            pShadeProp = pClarifiedProp;
            spMesh->AttachProperty(pClarifiedProp);
        }
        BSShader* pShader = BSShaderManager::GetShader(pShadeProp->iShader);
        spMesh->SetShader(pShader);
        NiUpdateData kUpdateData(0.0f, false, false);
        spMesh->UpdateProperties();
        spMesh->Update(kUpdateData);
        pNode->UpdateBlockVisibility(false);
        spMesh->GetModelData()->m_ucKeepFlags = 0;
        spMesh->GetModelData()->SetConsistency(NiGeometryData::STATIC);
        spMesh->GetModelData()->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
        pShader = spMesh->GetShader();
        // The renderer is the platform one; its PrecacheGeometry and PerformPrecache are called.
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        NiD3DShaderDeclaration* pDecl = pShader->GetShaderDecl(spMesh.m_pObject, pShadeProp);
        pRenderer->PrecacheGeometry(spMesh.m_pObject, 0, 0, pDecl);
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->PerformPrecache();
        bPrepared = true;
    }

    if (bUseStinger && spNodeStinger.m_pObject) {
        PrepareStingerObject(spNodeStinger.m_pObject);
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->PerformPrecache();
        bPrepared = true;
    }
}

void BGSDistantObjectBlock::PrepareStingerObject(NiAVObject* apGeom) {
    if (apGeom) {
        if (apGeom->IsNode()) {
            NiNode* pNode = apGeom->IsNode();
            for (int32_t iChild = 0; iChild < pNode->GetChildCount(); ++iChild) {
                NiAVObject* pChild = pNode->GetAtChecked(static_cast<uint32_t>(iChild));
                PrepareStingerObject(pChild);
            }
        }
        else {
            NiGeometry* pGeom = apGeom->IsGeometry();
            int32_t iShadeType = NiShadeProperty::GetType();
            pGeom->GetProperty(iShadeType);
            iShadeType = NiShadeProperty::GetType();
            BSShaderProperty* pShadeProp = static_cast<BSShaderProperty*>(pGeom->GetProperty(iShadeType));
            pGeom->RemoveProperty(NiAlphaProperty::GetType());
            NiAlphaProperty* pAlpha = new NiAlphaProperty;
            pAlpha->SetAlphaTesting(true);
            pAlpha->SetAlphaBlending(false);
            pAlpha->SetTestRef(128);
            pAlpha->SetTestMode(NiAlphaProperty::TEST_GREATER);
            pGeom->AttachProperty(pAlpha);
            pShadeProp->SetFlag(BSShaderProperty::BSSP_FLAG_LODBUILDING, 1);
            pShadeProp->SetFlag(BSShaderProperty::BSSP_FLAG_STINGER, 1);
            BSShaderProperty* pClarifiedProp = pShadeProp->ClarifyShader(pGeom, false, true);
            if (pClarifiedProp) {
                pGeom->RemoveProperty(3);
                pShadeProp = pClarifiedProp;
                pGeom->AttachProperty(pClarifiedProp);
            }
            pGeom->SetShader(BSShaderManager::GetShader(pShadeProp->iShader));
            NiUpdateData kUpdateData(0.0f, false, false);
            pGeom->UpdateProperties();
            pGeom->Update(kUpdateData);
            this->pNode->UpdateBlockVisibility(false);
            pGeom->GetModelData()->m_ucKeepFlags = 0;
            pGeom->GetModelData()->SetConsistency(NiGeometryData::STATIC);
            pGeom->GetModelData()->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
            BSShader* pShader = pGeom->GetShader();
            NiRenderer* pRenderer = NiRenderer::GetRenderer();
            NiD3DShaderDeclaration* pDecl = pShader->GetShaderDecl(pGeom, pShadeProp);
            static_cast<NiDX9Renderer*>(pRenderer)->PrecacheGeometry(pGeom, 0, 0, pDecl);
        }
    }
}

void BGSDistantObjectBlock::Detach() {
    RemoveFromList();
    if (spNode.m_pObject || spMesh.m_pObject || spNodeStinger.m_pObject) {
        --BGSDistantObjectBlock::uiLowPrioBlocks;
    }
    if (spNode.m_pObject || spMesh.m_pObject) {
        pNode->GetManager()->GetLODObjectRoot()->DetachChild(spNode.m_pObject);
        pNode->GetManager()->GetLODObjectRoot()->DetachChild(spMesh.m_pObject);
        pNode->GetManager()->GetLODObjectRoot()->DetachChild(spOldMesh.m_pObject);
        spNode = static_cast<BSMultiBoundNode*>(nullptr);
        spMesh = static_cast<NiGeometry*>(nullptr);
        spOldMesh = static_cast<NiGeometry*>(nullptr);
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    if (spNodeStinger.m_pObject) {
        pNode->GetManager()->GetLODObjectRoot()->DetachChild(spNodeStinger.m_pObject);
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
                pNode->GetManager()->GetLODObjectRoot()->DetachChild(spOldMesh->GetParent());
                pNode->GetManager()->GetLODObjectRoot()->AttachChild(spNode.m_pObject, 1);
            }
            else {
                pNode->GetManager()->GetLODObjectRoot()->DetachChild(spOldMesh.m_pObject);
                pNode->GetManager()->GetLODObjectRoot()->AttachChild(spMesh.m_pObject, 1);
            }
        }
        else if (spNode.m_pObject) {
            pNode->GetManager()->GetLODObjectRoot()->AttachChild(spNode.m_pObject, 1);
        }
        else if (spMesh.m_pObject) {
            pNode->GetManager()->GetLODObjectRoot()->AttachChild(spMesh.m_pObject, 1);
        }
        if (spNodeStingerOld.m_pObject) {
            if (spNodeStinger.m_pObject) {
                pNode->GetManager()->GetLODObjectRoot()->DetachChild(spNodeStingerOld.m_pObject);
                pNode->GetManager()->GetLODObjectRoot()->AttachChild(spNodeStinger.m_pObject, 1);
            }
        }
        else if (spNodeStinger.m_pObject) {
            pNode->GetManager()->GetLODObjectRoot()->AttachChild(spNodeStinger.m_pObject, 1);
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
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantObjectBlock.cpp",
        592
    );
    spOldMesh = spMesh;
    spNodeStingerOld = spNodeStinger;
    if (spLoadTask.m_pObject) {
        pIOManager->CancelTask(spLoadTask.m_pObject, nullptr);
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    if (spLoadTaskStinger.m_pObject) {
        pIOManager->CancelTask(spLoadTaskStinger.m_pObject, nullptr);
        spLoadTaskStinger = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
    }
    BGSDistantObjectBlockLoadTask* pTask = new BGSDistantObjectBlockLoadTask(
        this,
        apWorld,
        asCellX,
        asCellY,
        aiLevel,
        pPlayer->GetMegatonDestroyed(),
        false,
        false
    );
    spLoadTask = pTask;
    nDetachCount = 0;
    spLoadTask->QueueMe();
    bDoneLoading = false;
    bAttached = false;
    bHighPriority = false;
    bPrepared = false;
}

void BGSDistantObjectBlock::LoadFromTask(bool abLoadData) {
    if (spLoadTask.m_pObject && spLoadTask->IsTaskFinished()) {
        if (abLoadData) {
            spLoadTask->GetMesh(spMesh);
            spLoadTask->GetNode(spNode);
        }
        spLoadTask = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
        if (!spLoadTaskStinger.m_pObject) {
            bDoneLoading = true;
        }
    }

    if (spLoadTaskStinger.m_pObject && spLoadTaskStinger->IsTaskFinished()) {
        if (abLoadData) {
            spLoadTaskStinger->GetNode(spNodeStinger);
        }
        spLoadTaskStinger = static_cast<BGSDistantObjectBlockLoadTask*>(nullptr);
        if (!spLoadTask.m_pObject) {
            bDoneLoading = true;
        }
    }
}

void BGSDistantObjectBlock::EnableStingerRenderAll(NiAVObject* apGeom, bool abEnable) {
    if (apGeom) {
        apGeom->NiAVObject::SetAppCulled(!abEnable);
    }
    if (apGeom->IsNode()) {
        NiNode* pNode = apGeom->IsNode();
        for (uint32_t uiIndex = 0; uiIndex < pNode->GetChildCount(); ++uiIndex) {
            NiNode* pChild = static_cast<NiNode*>(pNode->GetAtChecked(uiIndex));
            BGSDistantObjectBlock::EnableStingerRenderAll(pChild, abEnable);
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
        apOutput,
        0x104,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.X%i.Y%i.NIF",
        apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(),
        aiLevel,
        static_cast<int32_t>(asCellX),
        static_cast<int32_t>(asCellY)
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
        apOutput,
        0x104,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.High.X%i.Y%i.NIF",
        apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(),
        aiLevel,
        static_cast<int32_t>(asCellX),
        static_cast<int32_t>(asCellY)
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
        apOutput,
        0x104,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Blocks\\%s.Level%i.X%i.Y%i.PostApocalypse.NIF",
        apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(),
        aiLevel,
        static_cast<int32_t>(asCellX),
        static_cast<int32_t>(asCellY)
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
        apOutput,
        0x104,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Stinger\\%s.Level%i.X%i.Y%i.NIF",
        apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(),
        aiLevel,
        static_cast<int32_t>(asCellX),
        static_cast<int32_t>(asCellY)
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
        apOutput,
        0x104,
        "Data\\Meshes\\Landscape\\LOD\\%s\\Stinger\\%s.Level%i.High.X%i.Y%i.NIF",
        apWorld->GetFormEditorID(),
        apWorld->GetFormEditorID(),
        aiLevel,
        static_cast<int32_t>(asCellX),
        static_cast<int32_t>(asCellY)
    );
}

void BGSDistantObjectBlock::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSDistantObjectBlock::ResetCullResults() {
    for (BGSDistantObjectBlock* pBlock : BGSDistantObjectBlock::BlocksLoaded) {
        if (pBlock && !pBlock->CheckDetach()) {
            BSMultiBoundNode* pBlockNode = pBlock->GetNode(false);
            if (pBlockNode) {
                pBlockNode->QMultiBound()->QShape()->ResetCullResult();
            }
            pBlockNode = pBlock->GetNode(true);
            if (pBlockNode) {
                pBlockNode->QMultiBound()->QShape()->ResetCullResult();
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
    char cPath[268];
    if (abStingerVersion) {
        if (abHighPriority) {
            BGSDistantObjectBlock::BuildStingerBlockFileNameHighPriority(cPath, pWorldSpace, aiLevel, aiX, aiY);
        }
        else {
            BGSDistantObjectBlock::BuildStingerBlockFileName(cPath, pWorldSpace, aiLevel, aiX, aiY);
        }
    }
    else if (abHighPriority) {
        BGSDistantObjectBlock::BuildBlockFileNameHighPriority(cPath, pWorldSpace, aiLevel, aiX, aiY);
    }
    else if (!abPostApocalypse && !pPlayer->GetMegatonDestroyed()) {
        BGSDistantObjectBlock::BuildBlockFileName(cPath, pWorldSpace, aiLevel, aiX, aiY);
    }
    else {
        // The post-apocalypse mesh, or the normal one where the world has none.
        BGSDistantObjectBlock::BuildBlockFileNamePA(cPath, pWorldSpace, aiLevel, aiX, aiY);
        if (!FileFinder::Locate(cPath, nullptr, 0, 0xFFFFFFFFu)) {
            BGSDistantObjectBlock::BuildBlockFileName(cPath, pWorldSpace, aiLevel, aiX, aiY);
        }
    }
    SetFileName(&cPath[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

BGSDistantObjectBlockLoadTask::~BGSDistantObjectBlockLoadTask() {}

void BGSDistantObjectBlockLoadTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODBUILDINGS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSDistantObjectBlock.cpp",
        922
    );
    BSFile* pFile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
    uint32_t uiLoad = 1;
    if (!pFile) {
        uiLoad = FileFinder::Locate(pFileName, nullptr, 6u, 1u) != 0;
    }
    BSStream kStream;
    if (uiLoad && kStream.BSStream::Load(pFileName, pFile)) {
        NiObject* pObject = kStream.GetObjectAt(0);
        if (pObject->IsNode()) {
            spNode = static_cast<BSMultiBoundNode*>(kStream.GetObjectAt(0));
            spMesh = static_cast<NiGeometry*>(spNode->GetAtChecked(0));
        }
        else {
            spMesh = static_cast<NiGeometry*>(kStream.GetObjectAt(0));
        }
    }
    else if (pBlock->GetHighPriority()) {
        pBlock->FlagDetach();
    }
}

void BGSDistantObjectBlockLoadTask::Finish() {
    if (spMesh.m_pObject) {
        if (bCanceled) {
            spMesh = static_cast<NiGeometry*>(nullptr);
            spNode = static_cast<BSMultiBoundNode*>(nullptr);
        }
        else {
            pBlock->LoadFromTask(true);
            pBlock->pNode->pManager->SetNeedsImmediateUpdate(true);
            if (!spMesh->IsSegmentedTriShape()) {
                Warning("MODELS: Found a distant object block for %i:%i,%i but it is not segmented.", iLevel, iX, iY);
            }
        }
    }
    else if (!bCanceled && pBlock) {
        pBlock->LoadFromTask(false);
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
    int8_t cBuildingsPath[264];
    int8_t cBuildingsNormalPath[268];
    if (!bStingerVersion) {
        BGSDistantObjectBlockLoadTask::GetBuildingsPath(
            static_cast<char*>(static_cast<void*>(cBuildingsPath)),
            pWorldSpace
        );
        int32_t iPriority = static_cast<int32_t>(IOTask::GetIOTaskPriority());
        pModelLoader->QueueTexture(
            static_cast<const char*>(static_cast<const void*>(cBuildingsPath)),
            static_cast<IO_TASK_PRIORITY>(iPriority),
            this
        );
        BGSDistantObjectBlockLoadTask::GetBuildingsNormalPath(
            static_cast<char*>(static_cast<void*>(cBuildingsNormalPath)),
            pWorldSpace
        );
        iPriority = static_cast<int32_t>(IOTask::GetIOTaskPriority());
        pModelLoader->QueueTexture(
            static_cast<const char*>(static_cast<const void*>(cBuildingsNormalPath)),
            static_cast<IO_TASK_PRIORITY>(iPriority),
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
    const char* pFileEditorID = apWorldspace->GetFormEditorID();
    const char* pFolderEditorID = apWorldspace->GetFormEditorID();
    BSsprintf(
        apDest,
        0x104u,
        "Data\\Textures\\Landscape\\LOD\\%s\\Blocks\\%s.Buildings.dds",
        pFolderEditorID,
        pFileEditorID
    );
}

void BGSDistantObjectBlockLoadTask::GetBuildingsNormalPath(char* apDest, TESWorldSpace* apWorldspace) {
    const char* pFileEditorID = apWorldspace->GetFormEditorID();
    const char* pFolderEditorID = apWorldspace->GetFormEditorID();
    BSsprintf(
        apDest,
        0x104u,
        "Data\\Textures\\Landscape\\LOD\\%s\\Blocks\\%s.Buildings_n.dds",
        pFolderEditorID,
        pFileEditorID
    );
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
