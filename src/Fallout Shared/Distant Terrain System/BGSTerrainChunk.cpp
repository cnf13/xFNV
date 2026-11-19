#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
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
        BSShaderPPLightingProperty* ShadeProperty = GetShadeProperty();
        if (ShadeProperty) {
            ShadeProperty->fGeomorphParam = afMorphParam;
        }
    }
}

NiTexture* BGSTerrainChunk::GetTexture(BGSTerrainChunk::ChunkTextureType aTexType) {
    NiTexture* ptex = nullptr;
    if (spMesh.m_pObject) {
        int32_t v7;
        int32_t Type = NiShadeProperty::GetType();
        BSShaderProperty* psp = static_cast<BSShaderProperty*>(spMesh.m_pObject->GetProperty(Type));
        if (psp) {
            int32_t v6 = psp->iShaderPropertyType >= 8 && psp->iShaderPropertyType <= 12;
            v7 = v6;
        }
        else {
            v7 = 0;
        }
        BSShaderPPLightingProperty* pplp = v7 ? static_cast<BSShaderPPLightingProperty*>(psp) : nullptr;
        if (pplp) {
            if (aTexType) {
                return pplp->GetNormalTexture(0);
            }

            return pplp->GetDiffuseTextures(0);
        }

        // The cell coordinates are sign-extended.
        Warning(
            "MODELS: Terrain chunk at level %i coords %i,%i has a mesh but no PP lighting property.", pNode->iLevel,
            static_cast<int32_t>(pNode->sBaseCellX), static_cast<int32_t>(pNode->sBaseCellY)
        );
    }
    return ptex;
}

bool BGSTerrainChunk::GetAttached() {
    return (bool)((uint8_t)bAttached);
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
    BGSTerrainChunk* v1 = this;
    (&BGSTerrainChunk::ChunksLoaded)->Remove(v1);
}

void BGSTerrainChunk::Prepare() {
    if ((!bPrepared) && (spMesh.m_pObject)) {
        int32_t Type = NiShadeProperty::GetType();
        BSShaderPPLightingProperty* pShaderProperty =
            static_cast<BSShaderPPLightingProperty*>(spMesh.m_pObject->GetProperty(Type));
        pShaderProperty->SetFlag(33u, 1);
        pShaderProperty->SetTerrainTexParams(0.0f, 0.0f, 1.0f);
        BSShaderManager::PrepareObject(spMesh.m_pObject, 0, 0);
        spLightingProp = pShaderProperty;
        spMesh.m_pObject->GetModelData()->m_ucKeepFlags = 0;
        spMesh.m_pObject->GetModelData()->SetConsistency(NiGeometryData::STATIC);
        spMesh.m_pObject->GetModelData()->SetCompressFlags(1u);
        BSShader* Shader = spMesh.m_pObject->GetShader();
        NiRenderer* Renderer_ = NiRenderer::GetRenderer();
        NiD3DShaderDeclaration* v2 = Shader->GetShaderDecl(spMesh.m_pObject, pShaderProperty);
        // This is a virtual of the platform renderer that NiRenderer does not declare.
        static_cast<NiDX9Renderer*>(Renderer_)->PrecacheGeometry(spMesh.m_pObject, 0, 0, v2);
        NiDX9Renderer* v3 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        v3->PerformPrecache();
        bPrepared = true;
    }
}

void BGSTerrainChunk::OnLoadFinished() {
    spLoadTask.m_pObject->GetMesh(spMesh);
    spLoadTask.m_pObject->GetWater(spWater);
    spLoadTask.m_pObject->GetWaterAtReflectionHeight(spWaterAtReflectionHeight);
    spLoadTask.m_pObject->GetNode(spNode);
    spLoadTask = static_cast<BGSTerrainChunkLoadTask*>(nullptr);
    bDoneLoading = true;
    pNode->pManager->SetNeedsImmediateUpdate(true);
}

void BGSTerrainChunk::Attach() {
    if (bDoneLoading && !bAttached) {
        BSMultiBoundAABB* pwaterMB = nullptr;
        ++BGSTerrainChunk::iChunkCount;
        if (spNode.m_pObject) {
            BSMultiBound* pbound_4;
            BSMultiBoundNode* v5;
            BSMultiBoundAABB* pwaterMB_3;
            BSMultiBound* pbound_2;
            BSMultiBoundNode* v8;
            BSMultiBoundAABB* pwaterMB_2;
            NiNode* LODLandRoot = pNode->pManager->GetLODLandRoot();
            LODLandRoot->AttachChild(spNode.m_pObject, 1);
            BSMultiBoundAABB* pMB = static_cast<BSMultiBoundAABB*>(spNode.m_pObject->QMultiBound()->QShape());
            NiPoint3 c = *&pMB->Center;
            NiPoint3 he = *&pMB->HalfExtents;
            BSMultiBoundAABB* apMemory =
                static_cast<BSMultiBoundAABB*>(NiMemObject::operator new(sizeof(BSMultiBoundAABB)));
            if (apMemory) {
                pwaterMB_2 = ::new (apMemory) BSMultiBoundAABB;
            }
            else {
                pwaterMB_2 = nullptr;
            }
            pwaterMB = pwaterMB_2;
            c.z = pNode->pManager->pWorld->GetDefaultWaterHeight();
            he.z = 0.0f;
            pwaterMB->SetCenter(c);
            pwaterMB->SetHalfExtents(he);
            BSMultiBoundNode* v15 = static_cast<BSMultiBoundNode*>(NiMemObject::operator new(sizeof(BSMultiBoundNode)));
            if (v15) {
                v8 = ::new (v15) BSMultiBoundNode;
            }
            else {
                v8 = nullptr;
            }
            spWaterNode = v8;
            BSMultiBound* v14 = static_cast<BSMultiBound*>(NiMemObject::operator new(sizeof(BSMultiBound)));
            if (v14) {
                pbound_2 = ::new (v14) BSMultiBound;
            }
            else {
                pbound_2 = nullptr;
            }
            BSMultiBound* pbound = pbound_2;
            pbound_2->SetShape(pwaterMB);
            spWaterNode.m_pObject->SetMultiBound(pbound);
            BSMultiBoundAABB* v13 = static_cast<BSMultiBoundAABB*>(NiMemObject::operator new(sizeof(BSMultiBoundAABB)));
            if (v13) {
                pwaterMB_3 = ::new (v13) BSMultiBoundAABB;
            }
            else {
                pwaterMB_3 = nullptr;
            }
            pwaterMB = pwaterMB_3;
            c.z = pNode->pManager->pWorld->GetDefaultWaterHeight();
            he.z = 0.0f;
            pwaterMB->SetCenter(c);
            pwaterMB->SetHalfExtents(he);
            BSMultiBoundNode* v12 = static_cast<BSMultiBoundNode*>(NiMemObject::operator new(sizeof(BSMultiBoundNode)));
            if (v12) {
                v5 = ::new (v12) BSMultiBoundNode;
            }
            else {
                v5 = nullptr;
            }
            spWaterReflNode = v5;
            BSMultiBound* v11 = static_cast<BSMultiBound*>(NiMemObject::operator new(sizeof(BSMultiBound)));
            if (v11) {
                pbound_4 = ::new (v11) BSMultiBound;
            }
            else {
                pbound_4 = nullptr;
            }
            pbound = pbound_4;
            pbound_4->SetShape(pwaterMB);
            spWaterReflNode.m_pObject->SetMultiBound(pbound);
        }
        else {
            NiNode* v2 = pNode->pManager->GetLODLandRoot();
            v2->AttachChild(spMesh.m_pObject, 1);
        }
        spMesh.m_pObject->UpdateProperties();
        NiUpdateData foobar(0.0f, false, false);
        spMesh.m_pObject->Update(foobar);
        AddWaterToSystem(false);
        NiUpdateData bar(0.0f, false, false);
        NiNode* v3 = pNode->pManager->GetLODLandRoot();
        v3->Update(bar);
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
            BSMultiBoundNode* v12 = spWaterReflNode.m_pObject;
            NiNode* WaterLODRoot = pNode->pManager->GetWaterLODRoot();
            TESWorldSpace* World = pNode->pManager->pWorld;
            NiGeometry* v6 = spWaterAtReflectionHeight.m_pObject;
            pLODWaterAtReflectionHeightRef =
                TES::pInstance->GetWaterSystem()->AddLODWater(v6, World, WaterLODRoot, v12, true);
            spWaterReflNode = static_cast<BSMultiBoundNode*>(nullptr);
            if (pLODWaterAtReflectionHeightRef) {
                NiNode* pWaterPlanesNode = pLODWaterAtReflectionHeightRef->Get3D();
                NiNode* v2 = pWaterPlanesNode->IsNode();
                NiAVObject* pChild = v2->GetAt(0);
                spWaterReflNode = pChild->IsMultiBoundNode();
            }
            NiObject* v19 = spWaterAtReflectionHeight.m_pObject;
            if (v19->IsSegmentedTriShape()) {
                NiObject* v18 = spWaterAtReflectionHeight.m_pObject;
                BSSegmentedTriShape* v3 = v18->IsSegmentedTriShape();
                v3->EnableAllSegments();
            }
        }
        if (spWater.m_pObject) {
            BSMultiBoundNode* v13 = spWaterNode.m_pObject;
            NiNode* v11 = pNode->pManager->GetWaterLODRoot();
            TESWorldSpace* v9 = pNode->pManager->pWorld;
            NiGeometry* v7 = spWater.m_pObject;
            pLODWaterRef = TES::pInstance->GetWaterSystem()->AddLODWater(v7, v9, v11, v13, false);
            spWaterNode = static_cast<BSMultiBoundNode*>(nullptr);
            if (pLODWaterRef) {
                NiNode* v17 = pLODWaterRef->Get3D();
                NiNode* v4 = v17->IsNode();
                NiAVObject* v16 = v4->GetAt(0);
                spWaterNode = v16->IsMultiBoundNode();
            }
            NiObject* v15 = spWater.m_pObject;
            if (v15->IsSegmentedTriShape()) {
                NiGeometry* v14 = spWater.m_pObject;
                BSSegmentedTriShape* v5 = v14->IsSegmentedTriShape();
                v5->EnableAllSegments();
            }
        }
    }
}

void BGSTerrainChunk::RemoveWaterFromSystem(bool abForce) {
    if (BGSTerrainChunk::bLODWaterProcessing || abForce) {
        if ((spWater.m_pObject) && (pLODWaterRef)) {
            if (TES::pInstance->GetWaterSystem()) {
                NiNode* WaterLODRoot = pNode->pManager->GetWaterLODRoot();
                TESWorldSpace* World = pNode->pManager->pWorld;
                TES::pInstance->GetWaterSystem()->RemoveLODWater(pLODWaterRef, World, WaterLODRoot);
            }
            if (spWaterNode.m_pObject) {
                NiNode* v7 = pNode->pManager->GetWaterLODRoot();
                v7->DetachChild(spWaterNode.m_pObject);
            }
            if (pLODWaterRef) {
                delete pLODWaterRef;
            }
            pLODWaterRef = nullptr;
        }

        if ((spWaterAtReflectionHeight.m_pObject) && (pLODWaterAtReflectionHeightRef)) {
            if (TES::pInstance->GetWaterSystem()) {
                NiNode* v5 = pNode->pManager->GetWaterLODRoot();
                TESWorldSpace* v3 = pNode->pManager->pWorld;
                TES::pInstance->GetWaterSystem()->RemoveLODWater(pLODWaterAtReflectionHeightRef, v3, v5);
            }
            if (spWaterReflNode.m_pObject) {
                NiNode* v6 = pNode->pManager->GetWaterLODRoot();
                v6->DetachChild(spWaterReflNode.m_pObject);
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
    for (BSSimpleList<BGSTerrainChunk*>* ecx0 = &BGSTerrainChunk::ChunksLoaded; ecx0 && !ecx0->IsEmpty();
         ecx0 = ecx0->m_pkNext) {
        BGSTerrainChunk* v1 = ecx0->m_item;
        BGSTerrainNode* v0 = v1->pNode;
        if (v0->QPersistentNode()) {
            v1->Show();
        }
        else {
            v1->Hide();
        }
    }
}

void BGSTerrainChunk::ShowNormalDetail() {
    for (BSSimpleList<BGSTerrainChunk*>* ecx0 = &BGSTerrainChunk::ChunksLoaded; ecx0 && !ecx0->IsEmpty();
         ecx0 = ecx0->m_pkNext) {
        BGSTerrainNode* v1;
        BGSTerrainChunk* v2 = ecx0->m_item;
        BGSTerrainNode* v0 = v2->pNode;
        if (v0->QPersistentNode() && ((v1 = v2->pNode, v1->QSplitNode()) && v2->pNode->CheckMeshSolidUnderMe(false))) {
            v2->Hide();
        }
        else {
            v2->Show();
        }
    }
}

void BGSTerrainChunk::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSTerrainChunk::UpdateMultiBoundVisibility(NiCamera* apCamera, NiCullingProcess& aCullProc) {
    NiPoint3 kviewPoint = apCamera->GetWorldTranslate();
    for (BGSTerrainChunk* item : BGSTerrainChunk::ChunksLoaded) {
        if ((item) && (!item->pNode->QSplitNode())) {
            BSMultiBoundNode* pnode = item->GetNode();
            if (pnode) {
                ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
                ShadowSceneNode_->UpdateMultiboundVisibility(pnode, kviewPoint, aCullProc);
            }
            BSMultiBoundNode* pnode_2 = item->spWaterNode.m_pObject;
            if (pnode_2) {
                ShadowSceneNode* v3 = BSShaderManager::GetShadowSceneNode(0);
                v3->UpdateMultiboundVisibility(pnode_2, kviewPoint, aCullProc);
            }
            BSMultiBoundNode* pnode_3 = item->spWaterReflNode.m_pObject;
            if (pnode_3) {
                ShadowSceneNode* v4 = BSShaderManager::GetShadowSceneNode(0);
                v4->UpdateMultiboundVisibility(pnode_3, kviewPoint, aCullProc);
            }
        }
    }
}

void BGSTerrainChunk::ResetMultiBoundResults() {
    for (BGSTerrainChunk* item : BGSTerrainChunk::ChunksLoaded) {
        if (item) {
            BSMultiBoundNode* pnode_1 = item->GetNode();
            if (pnode_1) {
                pnode_1->QMultiBound()->QShape()->ResetCullResult();
            }
            BSMultiBoundNode* pnode = item->spWaterNode.m_pObject;
            if (pnode) {
                pnode->QMultiBound()->QShape()->ResetCullResult();
            }
            BSMultiBoundNode* pnode_2 = item->spWaterReflNode.m_pObject;
            if (pnode_2) {
                pnode_2->QMultiBound()->QShape()->ResetCullResult();
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
    const char* v5 = apWorldSpace->GetFormEditorID();
    BSsprintf(
        apDest, 260u, "Data\\Meshes\\Landscape\\LOD\\%s\\%s.Level%i.X%i.Y%i.NIF", apWorldSpace->GetFormEditorID(), v5,
        lodLevel, static_cast<int16_t>(x), static_cast<int16_t>(y)
    );
}

int32_t BGSTerrainChunk::BuildLODObjectDiffuseFilename(
    char* a1,
    TESWorldSpace* apWorld,
    int32_t auiLODLevel,
    int16_t sPosX,
    int16_t sPoxY
) {
    const char* v6 = apWorld->GetFormEditorID();
    return std::sprintf(
        a1, "Data\\Textures\\Landscape\\LOD\\%s\\Diffuse\\%s.n.Level%i.X%i.Y%i.dds", apWorld->GetFormEditorID(), v6,
        auiLODLevel, sPosX, sPoxY
    );
}

int32_t BGSTerrainChunk::BuildLODObjectNormalFilename(char* a1, TESWorldSpace* a2, int32_t a3, int16_t a4, int16_t a5) {
    const char* v6 = a2->GetFormEditorID();
    return std::sprintf(
        a1, "Data\\Textures\\Landscape\\LOD\\%s\\Normals\\%s.n.Level%i.X%i.Y%i.dds", a2->GetFormEditorID(), v6, a3, a4,
        a5
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
        path, pWorldSpace, aiLevel, static_cast<uint16_t>(aiX), static_cast<uint16_t>(aiY)
    );
    SetFileName(&path[5]);
    LookupFileEntry(ARCHIVE_TYPE_INDEX_MESHES);
}

BGSTerrainChunkLoadTask::~BGSTerrainChunkLoadTask() {}

void BGSTerrainChunkLoadTask::Run() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Distant Terrain System\\BGSTerrainChunk.cpp", 664
    );
    BSFile* pfile = GetFile(ARCHIVE_TYPE_INDEX_MESHES, ARCHIVE_TYPE_MESHES);
    BSStream stream;
    if (stream.BSStream::Load(static_cast<const char*>(pFileName), pfile)) {
        NiObject* pobj = stream.GetObjectAt(0);
        if (pobj->IsNode()) {
            spNode = static_cast<BSMultiBoundNode*>(stream.GetObjectAt(0));
            spMesh = static_cast<NiGeometry*>(spNode.m_pObject->GetAt(0));
            if (spNode.m_pObject->GetChildCount() > 1 && TESWaterSystem::bUseWater.Bool()) {
                NiAVObject* v5 = spNode.m_pObject->GetAt(1u);
                spWater = v5->IsGeometry();
                NiObject* v4 = spWater.m_pObject;
                if (v4->IsSegmentedTriShape()) {
                    NiObject* v3 = spWater.m_pObject;
                    BSSegmentedTriShape* v1 = v3->IsSegmentedTriShape();
                    if (v1->GetNumSegments() == 3) {
                        spWater = static_cast<NiGeometry*>(nullptr);
                    }
                }
            }
            if (spNode.m_pObject->GetChildCount() > 2 && TESWaterSystem::bUseWater.Bool()) {
                NiAVObject* v2 = spNode.m_pObject->GetAt(2u);
                spWaterAtReflectionHeight = v2->IsGeometry();
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
            int32_t bCanceled = this->bCanceled;
            spMesh = static_cast<NiGeometry*>(nullptr);
            spWater = static_cast<NiGeometry*>(nullptr);
            spWaterAtReflectionHeight = static_cast<NiGeometry*>(nullptr);
        }
        else {
            char pcString[268];
            int32_t v14 = this->iY + this->iLevel - 1;
            int32_t v11 = this->iX + this->iLevel - 1;
            int32_t iY = this->iY;
            int32_t iX = this->iX;
            int32_t iLevel = this->iLevel;
            std::sprintf(
                pcString, NiFixedString("Terrain LOD | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(), iLevel, iX,
                iY, v11, v14
            );
            spMesh.m_pObject->SetName(pcString);
            if (spWater.m_pObject) {
                int32_t v15 = this->iY + this->iLevel - 1;
                int32_t v12 = this->iX + this->iLevel - 1;
                int32_t v9 = this->iY;
                int32_t v6 = this->iX;
                int32_t v3 = this->iLevel;
                std::sprintf(
                    pcString, NiFixedString("Water LOD | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(), v3, v6,
                    v9, v12, v15
                );
                NiFixedString v26(static_cast<const char*>(pcString));
                NiFixedString* kName = &v26;
                spWater.m_pObject->SetName(*kName);
            }
            if (spWaterAtReflectionHeight.m_pObject) {
                int32_t v16 = this->iY + this->iLevel - 1;
                int32_t v13 = this->iX + this->iLevel - 1;
                int32_t v10 = this->iY;
                int32_t v7 = this->iX;
                int32_t v4 = this->iLevel;
                std::sprintf(
                    pcString, NiFixedString("Water LOD w/ refl.) | Level %i | Cells (%i,%i)-(%i,%i)").GetHandleString(),
                    v4, v7, v10, v13, v16
                );
                NiFixedString v24(static_cast<const char*>(pcString));
                NiFixedString* v17 = &v24;
                spWaterAtReflectionHeight.m_pObject->SetName(*v17);
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
    IO_TASK_PRIORITY IOTaskPriority = GetIOTaskPriority();
    pModelLoader->QueueTexture(apFilePath, IOTaskPriority, this);
    BGSTerrainChunk::BuildLODObjectNormalFilename(apTextureFileName, pWorldSpace, iLevel, iX, iY);
    IO_TASK_PRIORITY v2 = GetIOTaskPriority();
    pModelLoader->QueueTexture(apTextureFileName, v2, this);
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
