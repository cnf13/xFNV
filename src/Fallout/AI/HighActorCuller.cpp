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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

void HighActorCuller::Operate(MobileObject* apMob) {
    NiAVObject* pObj = apMob->TESObjectREFR::Get3D();
    if (pObj) {
        // Process is called through a pointer so it goes through the vtable; a call on the member itself would be
        // devirtualised.
        NiCullingProcess* pCuller = &Culler;
        pCuller->Process(pObj);
        BSFadeNode* pn = pObj->IsFadeNode();
        if (pn && !static_cast<uint8_t>(pn->GetNotVisible())) {
            NiBound kBound = pn->GetWorldBound();
            BSMultiBoundSphere* pSphere = spMBSphere.m_pObject;
            pSphere->SetCenter(kBound.m_kCenter);
            float fBoundRadius = kBound.m_fRadius;
            spMBSphere->SetRadius(fBoundRadius);
            BSPortalGraph* pGraph = nullptr;
            if (TES::pInstance->pInteriorCell) {
                pGraph = TES::pInstance->pInteriorCell->GetPortalGraph();
            }
            else if (TES::pInstance->pWorldSpace) {
                pGraph = TES::pInstance->pWorldSpace->QPortalGraph();
            }
            if (pGraph) {
                const NiTPointerList<BSOcclusionPlane*>* pOccluders = &pGraph->lOccluderList;
                NiTListItem<BSOcclusionPlane*>* kIter = pOccluders->m_pkHead;
                while (kIter) {
                    BSOcclusionPlane* pOccluder = pOccluders->GetNext(kIter);
                    if (pOccluder &&
                        pOccluder->kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE &&
                        !spMBNode->VisibleAgainstActiveOccluders(
                            Culler.m_pkCamera->GetWorldTranslate(),
                            Culler,
                            pOccluder
                        )) {
                        pn->SetNotVisible(true);
                    }
                }
            }
        }
    }
}

void HighActorCuller::Init(NiCamera* apCamera) {
    Culler.NiCullingProcess::SetCamera(apCamera);
    Culler.SetFrustum(apCamera->GetViewFrustum());
    spMBSphere = new BSMultiBoundSphere;
    spMB = new BSMultiBound;
    spMBNode = new BSMultiBoundNode;
    spMB->SetShape(spMBSphere.m_pObject);
    spMBNode->BSMultiBoundNode::SetMultiBound(spMB.m_pObject);
}

void HighActorCuller::Run(NiCamera* apCamera) {
    if (spMBSphere == nullptr) {
        Init(apCamera);
    }
    else {
        Culler.m_pkCamera = apCamera;
        Culler.SetFrustum(apCamera->GetViewFrustum());
    }
    ReferenceProcessLists.IterateMobs(*this);
}
