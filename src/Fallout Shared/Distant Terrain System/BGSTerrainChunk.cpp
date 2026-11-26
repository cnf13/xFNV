#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/stdio.h"

BGSTerrainChunk::LoadedChunks BGSTerrainChunk::ChunksLoaded;

BGSTerrainChunk::BGSTerrainChunk(
    BGSTerrainNode* apNode,
    const int32_t aiLevel,
    const int16_t asCellX,
    const int16_t asCellY
)
  : pNode(apNode), spMesh(nullptr), spWater(nullptr), spWaterAtReflectionHeight(nullptr), spNode(nullptr),
    spWaterNode(nullptr), spWaterReflNode(nullptr), pLODWaterRef(nullptr), pLODWaterAtReflectionHeightRef(nullptr),
    spLoadTask(nullptr), bDoneLoading(false), bAttached(false), bPrepared(false), spLightingProp(nullptr) {
    AutoMemContext kContext(MEM_CONTEXT_LODLANDSCAPE);
    TESWorldSpace* pWorld = apNode->pManager->pWorld;
    spLoadTask = new BGSTerrainChunkLoadTask(this, pWorld, asCellX, asCellY, aiLevel);
    spLoadTask->QueueMe();
}

BGSTerrainChunk::~BGSTerrainChunk() {
    if (spMesh.m_pObject) {
        RemoveFromStaticList();
    }
    if (spMesh.m_pObject) {
        --BGSTerrainChunk::iChunkCount;
        if (spNode.m_pObject) {
            pNode->pManager->GetLODLandRoot()->DetachChild(spNode.m_pObject);
        }
        else {
            pNode->pManager->GetLODLandRoot()->DetachChild(spMesh.m_pObject);
        }
    }
    else if (spLoadTask.m_pObject) {
        pIOManager->CancelTask(spLoadTask.m_pObject, nullptr);
    }
    RemoveWaterFromSystem(false);
    NiObjectGarbage::Add(spNode.m_pObject);
    NiObjectGarbage::Add(spWaterNode.m_pObject);
    NiObjectGarbage::Add(spWaterReflNode.m_pObject);
    // The references are dropped before the texture purge, which then frees what they held.
    spLoadTask = nullptr;
    spMesh = nullptr;
    spWater = nullptr;
    spNode = nullptr;
    spWaterNode = nullptr;
    spWaterReflNode = nullptr;
    spLightingProp = nullptr;
    if (appMain->bFlyCamera) {
        BSTexturePalette::PurgeUnusedTextures();
    }
}

void BGSTerrainChunk::SetMorphParameter(float afMorphParam) {
    if (spMesh.m_pObject) {
        BSShaderPPLightingProperty* pShadeProperty = GetShadeProperty();
        if (pShadeProperty) {
            pShadeProperty->fGeomorphParam = afMorphParam;
        }
    }
}

NiTexture* BGSTerrainChunk::GetTexture(BGSTerrainChunk::ChunkTextureType aTexType) {
    NiTexture* ptex = nullptr;
    if (spMesh.m_pObject) {
        int32_t iIsPPLighting;
        int32_t iType = NiShadeProperty::GetType();
        BSShaderProperty* psp = static_cast<BSShaderProperty*>(spMesh->GetProperty(iType));
        if (psp) {
            iIsPPLighting = psp->iShaderPropertyType >= 8 && psp->iShaderPropertyType <= 12;
        }
        else {
            iIsPPLighting = 0;
        }
        BSShaderPPLightingProperty* pplp = iIsPPLighting ? static_cast<BSShaderPPLightingProperty*>(psp) : nullptr;
        if (pplp) {
            if (aTexType) {
                return pplp->GetNormalTexture(0);
            }

            return pplp->GetDiffuseTextures(0);
        }

        // The cell coordinates are sign-extended.
        Warning(
            "MODELS: Terrain chunk at level %i coords %i,%i has a mesh but no PP lighting property.",
            pNode->iLevel,
            static_cast<int32_t>(pNode->sBaseCellX),
            static_cast<int32_t>(pNode->sBaseCellY)
        );
    }
    return ptex;
}

bool BGSTerrainChunk::GetAttached() {
    return bAttached;
}

// spMesh's object.
NiGeometry* BGSTerrainChunk::GetMesh() {
    return spMesh;
}

void BGSTerrainChunk::AddToStaticList() {
    BGSTerrainChunk* pChunk = this;
    ChunksLoaded.AddTail(pChunk);
}

void BGSTerrainChunk::Hide() {
    if (spMesh) {
        spMesh->SetAppCulled(true);
    }
    if (spWater) {
        spWater->SetAppCulled(true);
    }
    if (spWaterAtReflectionHeight) {
        spWaterAtReflectionHeight->SetAppCulled(true);
    }
}

void BGSTerrainChunk::Show() {
    if (spMesh) {
        spMesh->SetAppCulled(false);
    }
    if (spWater) {
        spWater->SetAppCulled(false);
    }
    if (spWaterAtReflectionHeight) {
        spWaterAtReflectionHeight->SetAppCulled(false);
    }
}

void BGSTerrainChunk::RemoveFromStaticList() {
    BGSTerrainChunk* pChunk = this;
    BGSTerrainChunk::ChunksLoaded.Remove(pChunk);
}

void BGSTerrainChunk::Prepare() {
    if (!bPrepared && spMesh.m_pObject) {
        int32_t iType = NiShadeProperty::GetType();
        BSShaderPPLightingProperty* pShaderProperty = static_cast<BSShaderPPLightingProperty*>(
            spMesh->GetProperty(iType)
        );
        pShaderProperty->SetFlag(BSShaderProperty::BSSP_FLAG_LODLANDSCAPE, 1);
        pShaderProperty->SetTerrainTexParams(0.0f, 0.0f, 1.0f);
        BSShaderManager::PrepareObject(spMesh.m_pObject, 0, 0);
        spLightingProp = pShaderProperty;
        spMesh->GetModelData()->m_ucKeepFlags = 0;
        spMesh->GetModelData()->SetConsistency(NiGeometryData::STATIC);
        spMesh->GetModelData()->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
        BSShader* pShader = spMesh->GetShader();
        NiRenderer* pRenderer = NiRenderer::GetRenderer();
        NiD3DShaderDeclaration* pShaderDecl = pShader->GetShaderDecl(spMesh.m_pObject, pShaderProperty);
        // This is a virtual of the platform renderer that NiRenderer does not declare.
        static_cast<NiDX9Renderer*>(pRenderer)->PrecacheGeometry(spMesh.m_pObject, 0, 0, pShaderDecl);
        NiDX9Renderer* pDX9Renderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pDX9Renderer->PerformPrecache();
        bPrepared = true;
    }
}

void BGSTerrainChunk::OnLoadFinished() {
    spLoadTask->GetMesh(spMesh);
    spLoadTask->GetWater(spWater);
    spLoadTask->GetWaterAtReflectionHeight(spWaterAtReflectionHeight);
    spLoadTask->GetNode(spNode);
    spLoadTask = static_cast<BGSTerrainChunkLoadTask*>(nullptr);
    bDoneLoading = true;
    pNode->pManager->SetNeedsImmediateUpdate(true);
}

void BGSTerrainChunk::Attach() {
    if (bDoneLoading && !bAttached) {
        BSMultiBoundAABB* pwaterMB = nullptr;
        ++BGSTerrainChunk::iChunkCount;
        if (spNode.m_pObject) {
            NiNode* pLODLandRoot = pNode->pManager->GetLODLandRoot();
            pLODLandRoot->AttachChild(spNode.m_pObject, 1);
            BSMultiBoundAABB* pMB = static_cast<BSMultiBoundAABB*>(spNode->QMultiBound()->QShape());
            NiPoint3 c = pMB->Center;
            NiPoint3 he = pMB->HalfExtents;
            pwaterMB = new BSMultiBoundAABB;
            c.z = pNode->pManager->pWorld->GetDefaultWaterHeight();
            he.z = 0.0f;
            pwaterMB->SetCenter(c);
            pwaterMB->SetHalfExtents(he);
            spWaterNode = new BSMultiBoundNode;
            BSMultiBound* pbound = new BSMultiBound;
            pbound->SetShape(pwaterMB);
            spWaterNode->SetMultiBound(pbound);
            pwaterMB = new BSMultiBoundAABB;
            c.z = pNode->pManager->pWorld->GetDefaultWaterHeight();
            he.z = 0.0f;
            pwaterMB->SetCenter(c);
            pwaterMB->SetHalfExtents(he);
            spWaterReflNode = new BSMultiBoundNode;
            pbound = new BSMultiBound;
            pbound->SetShape(pwaterMB);
            spWaterReflNode->SetMultiBound(pbound);
        }
        else {
            NiNode* pLODLandRoot = pNode->pManager->GetLODLandRoot();
            pLODLandRoot->AttachChild(spMesh.m_pObject, 1);
        }
        spMesh->UpdateProperties();
        NiUpdateData foobar(0.0f, false, false);
        spMesh->Update(foobar);
        AddWaterToSystem(false);
        NiUpdateData bar(0.0f, false, false);
        NiNode* pLODLandRoot = pNode->pManager->GetLODLandRoot();
        pLODLandRoot->Update(bar);
        Prepare();
        AddToStaticList();
        bAttached = true;
        if (pNode->QPersistentNode() && pNode->QSplitNode()) {
            Hide();
        }
    }
}

void BGSTerrainChunk::AddWaterToSystem(bool abForce) {
    if (BGSTerrainChunk::bLODWaterProcessing || abForce) {
        if (spWaterAtReflectionHeight.m_pObject) {
            BSMultiBoundNode* pMBNode = spWaterReflNode.m_pObject;
            NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
            TESWorldSpace* pWorld = pNode->pManager->pWorld;
            NiGeometry* pGeometry = spWaterAtReflectionHeight.m_pObject;
            pLODWaterAtReflectionHeightRef = TES::pInstance->GetWaterSystem()
                                                 ->AddLODWater(pGeometry, pWorld, pWaterLODRoot, pMBNode, true);
            spWaterReflNode = static_cast<BSMultiBoundNode*>(nullptr);
            if (pLODWaterAtReflectionHeightRef) {
                NiNode* pWaterPlanesNode = pLODWaterAtReflectionHeightRef->Get3D();
                NiNode* pRootNode = pWaterPlanesNode->IsNode();
                NiAVObject* pChild = pRootNode->GetAt(0);
                spWaterReflNode = pChild->IsMultiBoundNode();
            }
            NiObject* pObject = spWaterAtReflectionHeight.m_pObject;
            if (pObject->IsSegmentedTriShape()) {
                pObject = spWaterAtReflectionHeight.m_pObject;
                BSSegmentedTriShape* pSegmentedShape = pObject->IsSegmentedTriShape();
                pSegmentedShape->EnableAllSegments();
            }
        }
        if (spWater.m_pObject) {
            BSMultiBoundNode* pMBNode = spWaterNode.m_pObject;
            NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
            TESWorldSpace* pWorld = pNode->pManager->pWorld;
            NiGeometry* pGeometry = spWater.m_pObject;
            pLODWaterRef = TES::pInstance->GetWaterSystem()
                               ->AddLODWater(pGeometry, pWorld, pWaterLODRoot, pMBNode, false);
            spWaterNode = static_cast<BSMultiBoundNode*>(nullptr);
            if (pLODWaterRef) {
                NiNode* pWaterPlanesNode = pLODWaterRef->Get3D();
                NiNode* pRootNode = pWaterPlanesNode->IsNode();
                NiAVObject* pChild = pRootNode->GetAt(0);
                spWaterNode = pChild->IsMultiBoundNode();
            }
            NiObject* pObject = spWater.m_pObject;
            if (pObject->IsSegmentedTriShape()) {
                pGeometry = spWater.m_pObject;
                BSSegmentedTriShape* pSegmentedShape = pGeometry->IsSegmentedTriShape();
                pSegmentedShape->EnableAllSegments();
            }
        }
    }
}

void BGSTerrainChunk::RemoveWaterFromSystem(bool abForce) {
    if (BGSTerrainChunk::bLODWaterProcessing || abForce) {
        if (spWater.m_pObject && pLODWaterRef) {
            if (TES::pInstance->GetWaterSystem()) {
                NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
                TESWorldSpace* pWorld = pNode->pManager->pWorld;
                TES::pInstance->GetWaterSystem()->RemoveLODWater(pLODWaterRef, pWorld, pWaterLODRoot);
            }
            if (spWaterNode.m_pObject) {
                NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
                pWaterLODRoot->DetachChild(spWaterNode.m_pObject);
            }
            if (pLODWaterRef) {
                delete pLODWaterRef;
            }
            pLODWaterRef = nullptr;
        }

        if (spWaterAtReflectionHeight.m_pObject && pLODWaterAtReflectionHeightRef) {
            if (TES::pInstance->GetWaterSystem()) {
                NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
                TESWorldSpace* pWorld = pNode->pManager->pWorld;
                TES::pInstance->GetWaterSystem()->RemoveLODWater(pLODWaterAtReflectionHeightRef, pWorld, pWaterLODRoot);
            }
            if (spWaterReflNode.m_pObject) {
                NiNode* pWaterLODRoot = pNode->pManager->GetWaterLODRoot();
                pWaterLODRoot->DetachChild(spWaterReflNode.m_pObject);
            }
            if (pLODWaterAtReflectionHeightRef) {
                delete pLODWaterAtReflectionHeightRef;
            }
            pLODWaterAtReflectionHeightRef = nullptr;
        }
    }
}

BSShaderPPLightingProperty* BGSTerrainChunk::GetShadeProperty() {
    return spLightingProp;
}

void BGSTerrainChunk::EnableLODWaterProcessing(bool abEnable) {
    if (BGSTerrainChunk::bLODWaterProcessing != abEnable) {
        for (BGSTerrainChunk* pChunk : BGSTerrainChunk::ChunksLoaded) {
            if (pChunk) {
                if (abEnable) {
                    pChunk->AddWaterToSystem(true);
                }
                else {
                    pChunk->RemoveWaterFromSystem(true);
                }
            }
        }
        BGSTerrainChunk::bLODWaterProcessing = abEnable;
    }
}

void BGSTerrainChunk::ShowMinimumDetail() {
    for (BSSimpleList<BGSTerrainChunk*>* pEntry = &BGSTerrainChunk::ChunksLoaded; pEntry && !pEntry->IsEmpty();
         pEntry = pEntry->m_pkNext) {
        BGSTerrainChunk* pChunk = pEntry->m_item;
        BGSTerrainNode* pTerrainNode = pChunk->pNode;
        if (pTerrainNode->QPersistentNode()) {
            pChunk->Show();
        }
        else {
            pChunk->Hide();
        }
    }
}

void BGSTerrainChunk::ShowNormalDetail() {
    for (BSSimpleList<BGSTerrainChunk*>* pEntry = &BGSTerrainChunk::ChunksLoaded; pEntry && !pEntry->IsEmpty();
         pEntry = pEntry->m_pkNext) {
        BGSTerrainChunk* pChunk = pEntry->m_item;
        BGSTerrainNode* pTerrainNode = pChunk->pNode;
        if (pTerrainNode->QPersistentNode() &&
            pChunk->pNode->QSplitNode() &&
            pChunk->pNode->CheckMeshSolidUnderMe(false)) {
            pChunk->Hide();
        }
        else {
            pChunk->Show();
        }
    }
}

void BGSTerrainChunk::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSTerrainChunk::UpdateMultiBoundVisibility(NiCamera* apCamera, NiCullingProcess& aCullProc) {
    NiPoint3 kviewPoint = apCamera->GetWorldTranslate();
    for (BGSTerrainChunk* item : BGSTerrainChunk::ChunksLoaded) {
        if (item && !item->pNode->QSplitNode()) {
            BSMultiBoundNode* pnode = item->GetNode();
            if (pnode) {
                ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
                pShadowSceneNode->UpdateMultiboundVisibility(pnode, kviewPoint, aCullProc);
            }
            pnode = item->spWaterNode.m_pObject;
            if (pnode) {
                ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
                pShadowSceneNode->UpdateMultiboundVisibility(pnode, kviewPoint, aCullProc);
            }
            pnode = item->spWaterReflNode.m_pObject;
            if (pnode) {
                ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
                pShadowSceneNode->UpdateMultiboundVisibility(pnode, kviewPoint, aCullProc);
            }
        }
    }
}

void BGSTerrainChunk::ResetMultiBoundResults() {
    for (BGSTerrainChunk* item : BGSTerrainChunk::ChunksLoaded) {
        if (item) {
            BSMultiBoundNode* pnode = item->GetNode();
            if (pnode) {
                pnode->QMultiBound()->QShape()->ResetCullResult();
            }
            pnode = item->spWaterNode.m_pObject;
            if (pnode) {
                pnode->QMultiBound()->QShape()->ResetCullResult();
            }
            pnode = item->spWaterReflNode.m_pObject;
            if (pnode) {
                pnode->QMultiBound()->QShape()->ResetCullResult();
            }
        }
    }
}

void BGSTerrainChunk::BuildLODObjectMeshFilename(
    char* apDest,
    TESWorldSpace* apWorldSpace,
    int32_t lodLevel,
    int32_t x,
    int32_t y
) {
    const char* pcEditorID = apWorldSpace->GetFormEditorID();
    BSsprintf(
        apDest,
        260u,
        "Data\\Meshes\\Landscape\\LOD\\%s\\%s.Level%i.X%i.Y%i.NIF",
        apWorldSpace->GetFormEditorID(),
        pcEditorID,
        lodLevel,
        static_cast<int16_t>(x),
        static_cast<int16_t>(y)
    );
}

int32_t BGSTerrainChunk::BuildLODObjectDiffuseFilename(
    char* apDest,
    TESWorldSpace* apWorld,
    int32_t auiLODLevel,
    int16_t sPosX,
    int16_t sPoxY
) {
    const char* pcEditorID = apWorld->GetFormEditorID();
    return std::sprintf(
        apDest,
        "Data\\Textures\\Landscape\\LOD\\%s\\Diffuse\\%s.n.Level%i.X%i.Y%i.dds",
        apWorld->GetFormEditorID(),
        pcEditorID,
        auiLODLevel,
        sPosX,
        sPoxY
    );
}

int32_t BGSTerrainChunk::BuildLODObjectNormalFilename(
    char* apDest,
    TESWorldSpace* apWorld,
    int32_t aiLODLevel,
    int16_t asPosX,
    int16_t asPosY
) {
    const char* pcEditorID = apWorld->GetFormEditorID();
    return std::sprintf(
        apDest,
        "Data\\Textures\\Landscape\\LOD\\%s\\Normals\\%s.n.Level%i.X%i.Y%i.dds",
        apWorld->GetFormEditorID(),
        pcEditorID,
        aiLODLevel,
        asPosX,
        asPosY
    );
}

BGSTerrainChunkLoadTask::BGSTerrainChunkLoadTask(
    BGSTerrainChunk* apChunk,
    TESWorldSpace* apWorld,
    int32_t aiX,
    int32_t aiY,
    int32_t aiLevel
)
  : QueuedFileEntry(IO_TASK_PRIORITY_MID), bFinishedQueuing(false), iX(aiX), iY(aiY), iLevel(aiLevel), pChunk(apChunk),
    pWorldSpace(apWorld), spMesh(nullptr), spWater(nullptr), spWaterAtReflectionHeight(nullptr), spNode(nullptr),
    bCanceled(false) {
    char path[268];
    BGSTerrainChunk::BuildLODObjectMeshFilename(
        path,
        pWorldSpace,
        aiLevel,
        static_cast<uint16_t>(aiX),
        static_cast<uint16_t>(aiY)
    );
    SetFileName(&path[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

BGSTerrainChunkLoadTask::~BGSTerrainChunkLoadTask() {}

void BGSTerrainChunkLoadTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainChunk.cpp",
        664
    );
    BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
    BSStream stream;
    if (stream.BSStream::Load(static_cast<const char*>(pFileName), pfile)) {
        NiObject* pobj = stream.GetObjectAt(0);
        if (pobj->IsNode()) {
            spNode = static_cast<BSMultiBoundNode*>(stream.GetObjectAt(0));
            spMesh = static_cast<NiGeometry*>(spNode->GetAt(0));
            if (spNode->GetChildCount() > 1 && TESWaterSystem::bUseWater.Bool()) {
                NiAVObject* pChild = spNode->GetAt(1u);
                spWater = pChild->IsGeometry();
                NiObject* pWaterObject = spWater.m_pObject;
                if (pWaterObject->IsSegmentedTriShape()) {
                    pWaterObject = spWater.m_pObject;
                    BSSegmentedTriShape* pSegmentedShape = pWaterObject->IsSegmentedTriShape();
                    if (pSegmentedShape->GetNumSegments() == 3) {
                        spWater = static_cast<NiGeometry*>(nullptr);
                    }
                }
            }
            if (spNode->GetChildCount() > 2 && TESWaterSystem::bUseWater.Bool()) {
                NiAVObject* pChild = spNode->GetAt(2u);
                spWaterAtReflectionHeight = pChild->IsGeometry();
            }
        }
        else {
            spMesh = static_cast<NiGeometry*>(stream.GetObjectAt(0));
            if (stream.GetObjectCount() > 1) {
                spWater = static_cast<NiGeometry*>(stream.GetObjectAt(1u));
            }
        }
    }
}

void BGSTerrainChunkLoadTask::Finish() {
    if (spMesh.m_pObject) {
        if (this->bCanceled) {
            spMesh = static_cast<NiGeometry*>(nullptr);
            spWater = static_cast<NiGeometry*>(nullptr);
            spWaterAtReflectionHeight = static_cast<NiGeometry*>(nullptr);
        }
        else {
            char pcString[268];
            int32_t iMaxY = this->iY + this->iLevel - 1;
            int32_t iMaxX = this->iX + this->iLevel - 1;
            int32_t iY = this->iY;
            int32_t iX = this->iX;
            int32_t iLevel = this->iLevel;
            std::sprintf(
                pcString,
                NiFixedString("Terrain LOD | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(),
                iLevel,
                iX,
                iY,
                iMaxX,
                iMaxY
            );
            spMesh->SetName(pcString);
            if (spWater.m_pObject) {
                iMaxY = this->iY + this->iLevel - 1;
                iMaxX = this->iX + this->iLevel - 1;
                iY = this->iY;
                iX = this->iX;
                iLevel = this->iLevel;
                std::sprintf(
                    pcString,
                    NiFixedString("Water LOD | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(),
                    iLevel,
                    iX,
                    iY,
                    iMaxX,
                    iMaxY
                );
                NiFixedString kName(static_cast<const char*>(pcString));
                spWater->SetName(kName);
            }
            if (spWaterAtReflectionHeight.m_pObject) {
                iMaxY = this->iY + this->iLevel - 1;
                iMaxX = this->iX + this->iLevel - 1;
                iY = this->iY;
                iX = this->iX;
                iLevel = this->iLevel;
                std::sprintf(
                    pcString,
                    NiFixedString("Water LOD w/ refl.) | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(),
                    iLevel,
                    iX,
                    iY,
                    iMaxX,
                    iMaxY
                );
                NiFixedString kName(static_cast<const char*>(pcString));
                spWaterAtReflectionHeight->SetName(kName);
            }
            pChunk->OnLoadFinished();
        }
    }
}

void BGSTerrainChunkLoadTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    bCanceled = true;
    QueuedFile::Cancel(aePreviousState, apTaskOwner);
}

bool BGSTerrainChunkLoadTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    BSsprintf(apDescription, aiBufferSize, "Terrain LOD Task for chunk ( %i, %i ) at level ( %i )", iX, iY, iLevel);
    return true;
}

void BGSTerrainChunkLoadTask::QueueMe() {
    char apFilePath[264];
    char apTextureFileName[268];
    BGSTerrainChunk::BuildLODObjectDiffuseFilename(apFilePath, pWorldSpace, iLevel, iX, iY);
    IO_TASK_PRIORITY eIOTaskPriority = GetIOTaskPriority();
    pModelLoader->QueueTexture(apFilePath, eIOTaskPriority, this);
    BGSTerrainChunk::BuildLODObjectNormalFilename(apTextureFileName, pWorldSpace, iLevel, iX, iY);
    eIOTaskPriority = GetIOTaskPriority();
    pModelLoader->QueueTexture(apTextureFileName, eIOTaskPriority, this);
    bFinishedQueuing = true;
    CheckFinished();
}

void BGSTerrainChunkLoadTask::CheckFinished() {
    if (GetAllChildrenFinished() && bFinishedQueuing) {
        if (IsPending()) {
            pIOManager->AddTask(this);
        }
        else {
            QueuedFile::CheckFinished();
        }
    }
}

// A chunk in an archive loads on that archive's file index (its runtime ID past the first four); a loose one on
// index 1.
uint8_t BGSTerrainChunkLoadTask::GetFileIndex() {
    if (GetFileEntry()) {
        Archive* pArchive = ArchiveManager::GetArchiveForFileEntry(GetFileEntry(), ARCHIVE_TYPE_MESHES);
        if (pArchive) {
            return pArchive->GetRuntimeID() + 4;
        }
    }
    return 1;
}

void BGSTerrainChunkLoadTask::GetMesh(NiPointer<NiGeometry>& aspGeometry) {
    aspGeometry = spMesh;
}

void BGSTerrainChunkLoadTask::GetWater(NiPointer<NiGeometry>& aspGeometry) {
    aspGeometry = spWater;
}

void BGSTerrainChunkLoadTask::GetWaterAtReflectionHeight(NiPointer<NiGeometry>& aspGeometry) {
    aspGeometry = spWaterAtReflectionHeight;
}

void BGSTerrainChunkLoadTask::GetNode(NiPointer<BSMultiBoundNode>& aspNode) {
    aspNode = spNode;
}

void BGSTerrainChunkLoadTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}
