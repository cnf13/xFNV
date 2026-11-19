#include "Fallout/AI/HighActorCuller.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSMultiBoundSphere.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortalGraph.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSFadeNodeCuller.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ProcessLists.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

void HighActorCuller::Operate(MobileObject* apMob) {
    NiAVObject* pobj = (NiAVObject*)apMob->TESObjectREFR::Get3D();
    if (pobj) {
        // Process is called through a pointer so it goes through the vtable; a call on the member itself would be
        // devirtualised.
        NiCullingProcess* pCuller = &Culler;
        pCuller->Process(pobj);
        BSFadeNode* pn = pobj->IsFadeNode();
        if ((pn) && (!static_cast<uint8_t>(pn->GetNotVisible()))) {
            NiBound kbound = pn->GetWorldBound();
            BSMultiBoundSphere* pSphere = spMBSphere.m_pObject;
            pSphere->SetCenter(kbound.m_kCenter);
            float fBoundRadius = kbound.m_fRadius;
            spMBSphere.m_pObject->SetRadius(fBoundRadius);
            BSPortalGraph* pgraph = nullptr;
            if (TES::pInstance->pInteriorCell) {
                pgraph = TES::pInstance->pInteriorCell->GetPortalGraph();
            }
            else {
                if (TES::pInstance->pWorldSpace) {
                    pgraph = TES::pInstance->pWorldSpace->QPortalGraph();
                }
            }
            if (pgraph) {
                const NiTPointerList<BSOcclusionPlane*>* poccluders = &pgraph->lOccluderList;
                NiTListItem<BSOcclusionPlane*>* kiter = poccluders->m_pkHead;
                while (kiter) {
                    BSOcclusionPlane* poccluder = poccluders->GetNext(kiter);
                    if ((poccluder) &&
                        (poccluder->kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE &&
                         !spMBNode.m_pObject->VisibleAgainstActiveOccluders(
                             Culler.m_pkCamera->GetWorldTranslate(), Culler, poccluder
                         ))) {
                        pn->SetNotVisible(true);
                    }
                }
            }
        }
    }
}

void HighActorCuller::Init(NiCamera* apCamera) {
    BSMultiBoundNode* v7;
    BSMultiBound* v8;
    BSMultiBoundSphere* v9;
    (&Culler)->NiCullingProcess::SetCamera(apCamera);
    const NiFrustum* viewFrustum = &apCamera->GetViewFrustum();
    (&Culler)->SetFrustum(*viewFrustum);
    void* v13 = NiMemObject::operator new(sizeof(BSMultiBoundSphere));
    if (v13) {
        v9 = ::new (v13) BSMultiBoundSphere;
    }
    else {
        v9 = nullptr;
    }
    spMBSphere = v9;
    void* v12 = NiMemObject::operator new(sizeof(BSMultiBound));
    if (v12) {
        v8 = ::new (v12) BSMultiBound;
    }
    else {
        v8 = nullptr;
    }
    spMB = v8;
    void* apMemory = NiMemObject::operator new(sizeof(BSMultiBoundNode));
    if (apMemory) {
        v7 = ::new (apMemory) BSMultiBoundNode;
    }
    else {
        v7 = nullptr;
    }
    spMBNode = v7;
    BSMultiBoundSphere* v5 = (&spMBSphere)->m_pObject;
    BSMultiBound* v3 = (&spMB)->m_pObject;
    v3->SetShape(v5);
    BSMultiBound* v6 = (&spMB)->m_pObject;
    BSMultiBoundNode* v4 = (&spMBNode)->m_pObject;
    v4->BSMultiBoundNode::SetMultiBound(v6);
}

void HighActorCuller::Run(NiCamera* apCamera) {
    if (spMBSphere == nullptr) {
        Init(apCamera);
    }
    else {
        Culler.m_pkCamera = apCamera;
        const NiFrustum* ViewFrustum = &apCamera->GetViewFrustum();
        Culler.SetFrustum(*ViewFrustum);
    }
    ReferenceProcessLists.IterateMobs(*this);
}
